#include "DownloadManager.h"

#include "CurlEasyDownloadManager.h"
#include "CurlEasyMultiManager.h"
#include "DownloadResult.h"
#include "HeaderHandler.h"
#include "throwOnError.h"

#include <fstream>
#include <functional>

#include "Logger.h"
LOG_INIT(DownloadManager);

class DownloadManager::Pimpl {
public:
  Pimpl(CURLM* const                                 multiHandle,
        DownloadElem&&                               download,
        const size_t                                 maxContentLength,
        const std::function<bool(const MediaType&)>& mediaTypeValidator,
        OpenCloseSocketConfig*                       openCloseSocketConfig)
      : m_download{std::move(download)}
      , m_maxContentLength{maxContentLength}
      , m_content{}
      , m_easyDownloadManager(const_cast<char*>(m_download.url.url.c_str()),
                              /* callback data ptr */ this,
                              /* callback header func */ &headerCb,
                              /* callback write function */ &writeCb,
                              openCloseSocketConfig)
      , m_easyMultiManager(multiHandle, m_easyDownloadManager.get())
      , m_errorStream{}
      , m_headerHandler{mediaTypeValidator, &m_errorStream} {}

  Pimpl(const Pimpl&) = delete;
  Pimpl& operator=(const Pimpl&) = delete;
  Pimpl(Pimpl&&) noexcept        = delete;
  Pimpl& operator=(Pimpl&&) noexcept = delete;

  void reuse(DownloadElem&&);

  std::function<void()> finish(const CURLcode infoResult) {
    m_errorStream << "ERROR: " << std::endl;
    if(strlen(m_easyDownloadManager.getErrorMessage())) {
      m_errorStream << m_easyDownloadManager.getErrorMessage() << std::endl;
    }

    if(CURLE_OK != infoResult) {
      m_errorStream << "Curl ERROR: " << curl_easy_strerror(infoResult) << std::endl;
    }

    double downloadSpeedByteSec = 0.;
    if(CURLE_OK != curl_easy_getinfo(m_easyDownloadManager.get(), CURLINFO_SPEED_DOWNLOAD, &downloadSpeedByteSec)) {
      LOG_ERROR("Can't read download speed.");
    }

    m_download.callback(DownloadResult{std::move(m_download.url),
                                       std::move(m_content),
                                       m_headerHandler.getMediaType(),
                                       CURLE_OK == infoResult,
                                       m_errorStream.str(),
                                       downloadSpeedByteSec});
    return std::move(m_finishedCallback);
  }

  void setFinishedCallback(std::function<void()>&& finishedCallback) {
    m_finishedCallback = std::move(finishedCallback);
  }

private:
  size_t headerCb(char* buffer, size_t size, size_t nitems) {
    LOG_DEBUG("headerCb");
    return m_headerHandler(buffer, size * nitems);
  }

  static size_t headerCb(char* buffer, size_t size, size_t nitems, void* userdata) {
    return static_cast<Pimpl*>(userdata)->headerCb(buffer, size, nitems);
  }

  size_t writeCb(char* buffer, size_t size, size_t nitems) {
    const size_t chunkSize = nitems * size;
    if(m_content.length() + chunkSize > m_maxContentLength) {
      LOG_INFO("m_maxContentLength " << m_maxContentLength << " exceeded. url: " << m_download
                                     << " mediaType: " << m_headerHandler.getMediaType());
      m_errorStream << "max_content length exceeded" << std::endl;
      return 0; // generate CURL_WRITE_ERROR
    }
    m_content.append(buffer, chunkSize);
    return chunkSize;
  }

  static size_t writeCb(char* buffer, size_t size, size_t nitems, void* userdata) {
    LOG_DEBUG("writeCb");
    return static_cast<Pimpl*>(userdata)->writeCb(buffer, size, nitems);
  }

private:
  DownloadElem            m_download;
  size_t                  m_maxContentLength;
  std::string             m_content;
  CurlEasyDownloadManager m_easyDownloadManager;
  CurlEasyMultiManager    m_easyMultiManager;
  std::ostringstream      m_errorStream;
  HeaderHandler           m_headerHandler;
  std::function<void()>   m_finishedCallback;
};

// class DownloadManager
DownloadManager::DownloadManager(CURLM* const                                 multiHandle,
                                 DownloadElem&&                               download,
                                 const size_t                                 maxContentLength,
                                 const std::function<bool(const MediaType&)>& mediaTypeValidator,
                                 OpenCloseSocketConfig*                       openCloseSocketConfig)
    : m_pimpl(
        new Pimpl(multiHandle, std::move(download), maxContentLength, mediaTypeValidator, openCloseSocketConfig)) {}

DownloadManager&
DownloadManager::operator=(DownloadManager&& other) noexcept {
  delete m_pimpl;
  m_pimpl       = other.m_pimpl;
  other.m_pimpl = nullptr;
  return *this;
}

DownloadManager::~DownloadManager() { delete m_pimpl; }

void
DownloadManager::reuse(DownloadElem&& download) {
  return m_pimpl->reuse(std::move(download));
}

// class Pimpl
void
DownloadManager::Pimpl::reuse(DownloadElem&& download) {
  m_easyMultiManager.release();
  m_download = std::move(download);
  m_easyDownloadManager.reuse(const_cast<char*>(m_download.url.url.c_str()), this, headerCb, writeCb);
  m_easyMultiManager.reuse();
  m_errorStream.str("");
  m_headerHandler.reuse();
  if(!m_content.empty()) {
    LOG_ERROR("downloaded content not consumed");
  }
  m_content.erase();
}

void
DownloadManager::setFinishedCallback(std::function<void()>&& finishedCallback) {
  m_pimpl->setFinishedCallback(std::move(finishedCallback));
}

std::function<void()>
processFinishedDownload(CURLMsg* infoMsg) {
  DownloadManager::Pimpl* downloadManager = nullptr;
  throwOnError(curl_easy_getinfo(infoMsg->easy_handle, CURLINFO_PRIVATE, &downloadManager), "curl_easy_getinfo");
  LOG_DEBUG("readDownloadIndex easyHandle: " << infoMsg->easy_handle << " downloadManager: " << downloadManager);
  if(downloadManager == nullptr) {
    throw std::runtime_error(std::string("[readDownloadIndex] invalid downloadManager returned by curl_easy_getinfo"));
  }
  return downloadManager->finish(infoMsg->data.result);
}

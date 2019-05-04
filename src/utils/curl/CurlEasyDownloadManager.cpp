#include "Logger.h"
LOG_INIT(CurlEasyDownloadManager);

#include "CurlEasyDownloadManager.h"
#include "throwOnError.h"

#include <curl/curl.h>
#include <iostream>
#include <stdexcept>

namespace {
// timeout for entire download
const long CONNECTION_TIMEOUT = 60;

// timeout for establishing a  connection
const long CONNECTION_CONNECTTIMEOUT = 15; // seconds, default is 300 seconds
} // namespace

CurlEasyDownloadManager::CurlEasyDownloadManager(char*                  url,
                                                 void*                  userdata,
                                                 HeaderCbType           headerCb,
                                                 HeaderCbType           writeCb,
                                                 OpenCloseSocketConfig* openCloseSocketConfig)
    : m_errorMessage{}, m_easyHandle{curl_easy_init()} {
  if(nullptr == m_easyHandle.get()) {
    throw std::runtime_error("CurlEasyDownloadManager: curl_easy_init return nullptr");
  }
  setGeneralOptions(openCloseSocketConfig);
  setUrlSpecific(url, userdata, headerCb, writeCb);
}

void
CurlEasyDownloadManager::reuse(char* url, void* userdata, HeaderCbType headerCb, HeaderCbType writeCb) {
  std::fill(std::begin(m_errorMessage), std::end(m_errorMessage), 0);
  setUrlSpecific(url, userdata, headerCb, writeCb);
}

void
CurlEasyDownloadManager::setGeneralOptions(OpenCloseSocketConfig* openCloseSocketConfig) {
  CURL* const easyHandle = get();
  std::string errMsg     = "CurlEasyDownloadManager::setGeneralOptions() curl_easy_setopt ";
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_ERRORBUFFER, m_errorMessage), errMsg + "CURLOPT_ERRORBUFFER");
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_VERBOSE, 0L), errMsg + "CURLOPT_VERBOSE");
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_NOPROGRESS, 1L), errMsg + "CURLOPT_NOPROGRESS");
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_CONNECTTIMEOUT, CONNECTION_CONNECTTIMEOUT),
               errMsg + "CURLOPT_CONNECTTIMEOUT");
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_TIMEOUT, CONNECTION_TIMEOUT), errMsg + "CURLOPT_TIMEOUT");
  if(nullptr != openCloseSocketConfig) {
    throwOnError(curl_easy_setopt(easyHandle, CURLOPT_OPENSOCKETFUNCTION, openCloseSocketConfig->openSocketCb),
                 errMsg + "CURLOPT_OPENSOCKETFUNCTION");
    throwOnError(curl_easy_setopt(easyHandle, CURLOPT_OPENSOCKETDATA, openCloseSocketConfig->openSocketData),
                 errMsg + "CURLOPT_OPENSOCKETDATA");
    throwOnError(curl_easy_setopt(easyHandle, CURLOPT_CLOSESOCKETFUNCTION, openCloseSocketConfig->closeSocketCb),
                 errMsg + "CURLOPT_CLOSESOCKETFUNCTION");
    throwOnError(curl_easy_setopt(easyHandle, CURLOPT_CLOSESOCKETDATA, openCloseSocketConfig->closeSocketData),
                 errMsg + "CURLOPT_CLOSESOCKETDATA");
  }
  // For future consideration:
  // curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_TIME, 3L);
  // curl_easy_setopt(conn->easy, CURLOPT_LOW_SPEED_LIMIT, 10L);
}

void
CurlEasyDownloadManager::setUrlSpecific(char* url, void* userdata, HeaderCbType headerCb, HeaderCbType writeCb) {
  CURL* const easyHandle = get();
  LOG_DEBUG("easyHandle: " << easyHandle << " userdata: " << userdata);

  std::string errMsg = "CurlEasyDownloadManager::setUrlSpecific() curl_easy_setopt ";
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_HEADERDATA, userdata), errMsg + "CURLOPT_HEADERDATA");
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_HEADERFUNCTION, headerCb), errMsg + "CURLOPT_HEADERFUNCTION");
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_WRITEDATA, userdata), errMsg + "CURLOPT_WRITEDATA");
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_WRITEFUNCTION, writeCb), errMsg + "CURLOPT_WRITEFUNCTION");
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_PRIVATE, userdata), errMsg + "CURLOPT_PRIVATE");
  throwOnError(curl_easy_setopt(easyHandle, CURLOPT_URL, url), errMsg + "CURLOPT_URL");
}

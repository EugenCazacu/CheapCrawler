#ifndef UTILS_CURL_DOWNLOADMANAGER_H_I9WKOSRH
#define UTILS_CURL_DOWNLOADMANAGER_H_I9WKOSRH

#include <curl/multi.h>
#include <functional>
#include <string>

#include "MediaType.h"
#include "Url.h"
#include "curlTypes.h"

struct DownloadResult;

class DownloadManager {
public:
  DownloadManager(CURLM*                                       multiHandle,
                  DownloadElem&&                               download,
                  size_t                                       maxContentLength,
                  const std::function<bool(const MediaType&)>& mediaTypeValidator,
                  OpenCloseSocketConfig*                       openCloseSocketConfig = nullptr);

  DownloadManager(const DownloadManager&) = delete;
  DownloadManager& operator=(const DownloadManager&) = delete;

  DownloadManager(DownloadManager&& other) noexcept : m_pimpl(other.m_pimpl) { other.m_pimpl = nullptr; }

  DownloadManager& operator=(DownloadManager&& other) noexcept;

  ~DownloadManager();

  void swap(DownloadManager& other) noexcept {
    using std::swap;
    swap(m_pimpl, other.m_pimpl);
  }

  void setFinishedCallback(std::function<void()>&&);
  void reuse(DownloadElem&&);

  class Pimpl;

private:
  Pimpl* m_pimpl;
};

inline void
swap(DownloadManager& a, DownloadManager& b) noexcept {
  a.swap(b);
}

std::function<void()> processFinishedDownload(CURLMsg* infoMsg);

#endif /* end of include guard: UTILS_CURL_DOWNLOADMANAGER_H_I9WKOSRH */

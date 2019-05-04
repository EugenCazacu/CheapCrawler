#ifndef CRAWLER_CURLASIODOWNLOADER_H_UKHIGCT4
#define CRAWLER_CURLASIODOWNLOADER_H_UKHIGCT4

#include "MediaType.h"
#include "crawler.h"

#include <memory>

class CurlAsioDownloader : public Downloader {
public:
  CurlAsioDownloader(size_t maxContentLength, std::function<bool(const MediaType&)> mediaTypeValidator);
  ~CurlAsioDownloader();

  struct Pimpl;

private:
  void                         doDownload(DownloadElem&&) override;
  const std::unique_ptr<Pimpl> m_pimpl;
};

#endif /* end of include guard: CRAWLER_CURLASIODOWNLOADER_H_UKHIGCT4 */

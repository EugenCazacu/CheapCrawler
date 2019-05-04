#ifndef CRAWLER_CRAWLER_H_H1A05R9E
#define CRAWLER_CRAWLER_H_H1A05R9E

#include <chrono>
#include <functional>
#include <memory>
#include <vector>

#include "Url.h"

class Downloader {
public:
  /**
   * Adds a new download to the download queue.
   * All downloads considered started in parallel.
   * @url The address of the URL to be downloaded.
   * @callback This function will be called when download is finished.
   *           The callback will be called asynchronously in a different thread.
   */
  void download(DownloadElem&& elem) { doDownload(std::move(elem)); }

  /**
   * On destruction abort all ongoing downloads ASAP.
   * Do not call any non-started callbacks.
   */
  virtual ~Downloader() {}

private:
  virtual void doDownload(DownloadElem&&) = 0;
};

/**
 * Flow:
 * While (keepCrawling)
 *   Pull URL list from the crawling dispatcher
 *   Prepare robots.txt download for each host and protocol
 *   Try download each robots.txt
 *      log each downloaded robots.txt result
 *      if no robots.txt => crawl all
 *      if download error => set result to download error, don't crawl
 *      else (successful download of robots.txt)
 *        (NOT IMPLEMENTED YET) For all forbidden urls by robots.txt:
 *          => remove from download queue
 *      Download allowed URLs with 2 secs default delay between download requests per same host
 *
 * Specs:
 *  - Stop and exit when keepCrawling() returns false
 *  - Pull URL list from the crawling dispatcher
 *  - Calculate overall robots.txt url list for the dispatched url bunch
 *  - Read robots.txt before each call to dispatcher (no persistent robots.txt)
 *  - One download queue per host with 2 secs timeout between downloads per host
 *  - Filter URLs by robots.txt result
 *  - Call download result for each finished download
 */
class Crawler {
public:
  /**
   * @param keepCrawling called before getting the list of downloads from the dispatcher
   *                     and stops when false is returned
   * @param dispatcher is called whenever new urls can be added to be downloaded
   *                   should return the list of url to be downloaded
   * @param downloader the software component responsible for actual downloading
   * @param maxActiveQueues Downloads are split by hosts into download queues.
   *                        This param controls the number of maximum simultaneous downloads.
   *                        There can only be maximum one download per queue
   * @param perHostTimeout timeout between subsequent download requests to the same host,
   *                       after a download is finished
   */
  Crawler(std::function<bool()>&&                      keepCrawling,
          std::function<std::vector<DownloadElem>()>&& dispatcher,
          Downloader*                                  downloader,
          size_t                                       maxActiveQueues,
          std::chrono::seconds                         perHostTimeout);
  ~Crawler();

  /**
   * Will stop getting urls from the dispatcher
   * when keepCrawling returns false
   */
  void crawl();

private:
  class Pimpl;
  const std::unique_ptr<Pimpl> m_pimpl;
};

#endif /* end of include guard: CRAWLER_CRAWLER_H_H1A05R9E */

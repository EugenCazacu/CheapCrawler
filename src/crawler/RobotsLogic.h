#ifndef CRAWLER_ROBOTSLOGIC_H_YAHWAKIP
#define CRAWLER_ROBOTSLOGIC_H_YAHWAKIP

#include "DownloadQueues.h"

/**
 * TODO UPDATE THIS AND IMPLEMENT ROBOTS.TXT FILTERING
 *  Will populate the dwQueues with downloads of the robots generated from the download list.
 *  @param urlsToCrawl list of urls to be crawled with robots.txt rules
 *  @param onFinishedDownload Will be added to each DownloadElem as callback
 *                            additionally to the initial action of the DownloadElem
 *                            Note that this also applies to the downloads automatically added by this
 *                            function, i.e. robots.txt downloads, which does not have other associated finished
 *                            actions.
 *  @returns DownloadQueues with appropriate robots.txt downloads:
 *                          - one robots.txt download per host and schema (http and https).
 *                          - downloads are split in download queue according to the hosts
 *                          Each download finish callback will populate the appropriate downloadQueue:
 *                          - after the url filtering is applied
 *                          - before the call to onFinishedDownload.
 */
void populateDownloadQueuesWithRobots(DownloadQueues*                                      dwQueues,
                                      std::vector<DownloadElem>&&                          urlsToCrawl,
                                      std::function<void(DownloadQueues::DownloadQueueIt)> onFinishedDownload);

#endif /* end of include guard: CRAWLER_ROBOTSLOGIC_H_YAHWAKIP */

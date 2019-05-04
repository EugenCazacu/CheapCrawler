#include "Logger.h"
LOG_INIT(crawlerRobotsLogic);

#include "DownloadQueues.h"
#include "DownloadResult.h"
#include "Hashable.h"
#include "RobotsLogic.h"
#include "uriUtils/uriUtils.h"

#include <memory>
#include <unordered_map>
#include <uriparser/Uri.h>

using std::begin;
using std::end;
using std::string;
using std::vector;

namespace {

struct Robot {
  string scheme;
  string hostText;
};

bool
operator==(const Robot& r1, const Robot& r2) {
  return r1.scheme == r2.scheme && r1.hostText == r2.hostText;
}

std::string
getRobotsTxtUrl(const Robot& robotId) {
  static const char* URI_PROTOCOL_HOST_DELIMITER = "://";
  static const char* URI_DELIMITER               = "/";
  static const char* ROBOTS_TXT                  = "robots.txt";
  return robotId.scheme + URI_PROTOCOL_HOST_DELIMITER + robotId.hostText + URI_DELIMITER + ROBOTS_TXT;
}

} // namespace

MAKE_HASHABLE(Robot, t.scheme, t.hostText);

void
populateDownloadQueuesWithRobots(DownloadQueues*                                      dwQueues,
                                 std::vector<DownloadElem>&&                          urlsToCrawl,
                                 std::function<void(DownloadQueues::DownloadQueueIt)> onFinishedDownload) {
  LOG_DEBUG("building downloadQueues, urlsToCrawl size: " << urlsToCrawl.size());
  const auto uriReleaser = [](UriUriA* uri) { uriFreeUriMembersA(uri); };

  UriParserStateA state;
  UriUriA         uri;
  state.uri = &uri;

  std::unordered_map<Robot, vector<DownloadElem>*> robots;

  for(auto& urlElem: urlsToCrawl) {
    std::unique_ptr<UriUriA, decltype(uriReleaser)> uriRaii(&uri, uriReleaser);
    int                                             callResult;
    std::string&                                    url = std::get<0>(urlElem.url);
    if(URI_SUCCESS != (callResult = uriParseUriA(&state, url.c_str()))) {
      LOG_ERROR("failed to parse error:" << callResult << " url: " << url);
      continue;
    }
    if(!isValid(uri)) {
      LOG_ERROR("invalid uri received: " << url);
      continue;
    }

    const string          scheme{uri.scheme.first, uri.scheme.afterLast};
    const string          host{uri.hostText.first, uri.hostText.afterLast};
    const Robot           robot{scheme, host};
    auto                  robotIt = robots.find(robot);
    vector<DownloadElem>* urlList = nullptr;
    if(end(robots) == robotIt) {
      auto urlListOwner = std::make_shared<vector<DownloadElem>>();
      urlList           = urlListOwner.get();
      robots[robot]     = urlList;

      DownloadQueues::DownloadQueueIt dwQueue = dwQueues->getQueueByHost(host);
      dwQueues->addDownload(
          dwQueue,
          DownloadElem{Url{getRobotsTxtUrl(robot), 0},
                       [urls = std::move(urlListOwner), dwQueues, dwQueue, onFinishedDownload](DownloadResult&&) {
                         // TODO filter urllist by robots.txt
                         // RobotsFilter robotsFilter;
                         // bool parseRobotsResult { };
                         // std::tie(robotsFilter, parseRobotsResult) = parse(robotsResult);
                         // if(parseRobotsResult) {
                         //   robotsFilter(urls);
                         // }
                         for(auto& url: *urls) {
                           dwQueues->addDownload(
                               dwQueue,
                               DownloadElem{std::move(url.url),
                                            [dwQueue, onFinishedDownload, dwFinishedCb = std::move(url.callback)](
                                                DownloadResult&& dwResult) {
                                              LOG_DEBUG("Finished downloading: " << dwResult.url);
                                              dwFinishedCb(std::move(dwResult));
                                              onFinishedDownload(dwQueue);
                                            }});
                         }
                         onFinishedDownload(dwQueue);
                       }});
    }
    else {
      urlList = robotIt->second;
    }
    urlList->emplace_back(std::move(urlElem));
  }
  LOG_DEBUG("Total robot download: " << robots.size());
  LOG_DEBUG("Total number of download queues: " << dwQueues->size());
}

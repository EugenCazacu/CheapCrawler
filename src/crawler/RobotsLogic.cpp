#include "Logger.h"
LOG_INIT(crawlerRobotsLogic);

#include "DownloadQueues.h"
#include "DownloadResult.h"
#include "Hashable.h"
#include "RobotsFilter.h"
#include "RobotsLogic.h"
#include "uriUtils/uriUtils.h"

#include <memory>
#include <unordered_map>
#include <uriparser/Uri.h>

using std::begin;
using std::end;
using std::string;
using std::string_view;
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

struct FinishDownloadAdapter {
  DownloadQueues::DownloadQueueIt dwQueueIt;
  std::function<void(DownloadQueues::DownloadQueueIt)> additionalCb;

  void operator()(DownloadElem& dwElem) {
    dwElem.callback = [additionalCb=this->additionalCb, dwQueueIt=this->dwQueueIt, originalCb=std::move(dwElem.callback)]
        ( DownloadResult&& dwResult) {
        LOG_DEBUG("Finished downloading: " << dwResult.url);
        originalCb(std::move(dwResult));
        additionalCb(dwQueueIt);
      };
  }
};

struct FilterAndAddDownloads {
  DownloadQueues::DownloadQueueIt dwQueue;
  std::function<void(DownloadQueues::DownloadQueueIt)> additionalCb;
  RobotsFilter robotsFilter;

  void operator()(PreparedDownloadElem& dwElem) {
    if(robotsFilter.canDownload(dwElem.downloadElem.url.url)) {
      dwElem.downloadElem.callback =
        [dwQueue=this->dwQueue, additionalCb=this->additionalCb, dwFinishedCb=std::move(dwElem.downloadElem.callback)](DownloadResult&& dwResult) {
             dwFinishedCb(std::move(dwResult));
             additionalCb(dwQueue);
           };
      addDownload(dwQueue, std::move(dwElem));
    }
  }
};

[[nodiscard]]
constexpr std::string_view toStringView(const UriTextRangeA& textRange) noexcept {
  return std::string_view { textRange.first, static_cast<std::string_view::size_type>(textRange.afterLast-textRange.first)};
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

  std::unordered_map<Robot, vector<PreparedDownloadElem>*> robots;

  for(auto& urlElem: urlsToCrawl) {
    std::unique_ptr<UriUriA, decltype(uriReleaser)> uriRaii(&uri, uriReleaser);
    int                                             callResult;
    std::string&                                    url = urlElem.url.url;
    if(URI_SUCCESS != (callResult = uriParseUriA(&state, url.c_str()))) {
      LOG_ERROR("failed to parse error:" << callResult << " url: " << url);
      continue;
    }
    if(!isValid(uri)) {
      LOG_ERROR("invalid uri received: " << url);
      continue;
    }

    const string_view scheme{toStringView(uri.scheme)};
    const string_view host{toStringView(uri.hostText)};
    const Robot       robot{std::string{scheme}, std::string{host}};
    auto              robotIt = robots.find(robot);
    vector<PreparedDownloadElem>* urlList = nullptr;
    if(end(robots) == robotIt) {
      auto urlListOwner = std::make_shared<vector<PreparedDownloadElem>>();
      urlList           = urlListOwner.get();
      robots[robot]     = urlList;

      DownloadQueues::DownloadQueueIt dwQueue = dwQueues->getQueueByHost(host);
      addDownload(
          dwQueue,
          PreparedDownloadElem{ DownloadElem{ {getRobotsTxtUrl(robot), 0},
                       [urls = std::move(urlListOwner), dwQueue, onFinishedDownload](DownloadResult&& robotsResult) {
                         // TODO filter urllist by robots.txt
                         // RobotsFilter robotsFilter;
                         // bool parseRobotsResult { };
                         // std::tie(robotsFilter, parseRobotsResult) = parse(robotsResult);
                         // if(parseRobotsResult) {
                         //   robotsFilter(urls);
                         // }
                         std::for_each(begin(*urls), end(*urls),
                             FilterAndAddDownloads { dwQueue, onFinishedDownload, RobotsFilter {robotsResult.content} } );
                         LOG_DEBUG("populateDownloadQueuesWithRobots: dwQueue: " << dwQueue);
                         onFinishedDownload(dwQueue);
                       }}, scheme, host});
    }
    else {
      urlList = robotIt->second;
    }
    urlList->push_back(PreparedDownloadElem{std::move(urlElem), scheme, host});
  }
  LOG_DEBUG("Total robot download: " << robots.size());
  LOG_DEBUG("Total number of download queues: " << dwQueues->size());
}

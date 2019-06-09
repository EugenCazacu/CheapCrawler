#include "Logger.h"
LOG_INIT(crawlerCrawler);

#include "ActionQueue.h"
#include "DownloadQueues.h"
#include "RobotsLogic.h"
#include "TimeHeap.h"
#include "crawler/crawler.h"

namespace {
bool
canAddDownload(size_t activeDownloads, size_t maxActiveDownloads) {
  return activeDownloads < maxActiveDownloads;
}
} // namespace

class Crawler::Pimpl {
public:
  Pimpl(std::function<bool()>&&                      keepCrawling,
        std::function<std::vector<DownloadElem>()>&& dispatcher,
        Downloader*                                  downloader,
        size_t                                       maxActiveQueues,
        std::chrono::seconds                         perHostTimeout)
      : m_keepCrawling{std::move(keepCrawling)}
      , m_dispatcher{std::move(dispatcher)}
      , m_downloader{downloader}
      , m_maxActiveDownloads{maxActiveQueues}
      , m_perHostTimeout{perHostTimeout} {
    if(m_maxActiveDownloads <= 0) {
      throw std::logic_error("Crawler::Crawler received invalid maxActiveQueues: "
                             + std::to_string(m_maxActiveDownloads));
    }
  }

  void crawl();

private:
  std::function<bool()>                      m_keepCrawling;
  std::function<std::vector<DownloadElem>()> m_dispatcher;
  Downloader*                                m_downloader;
  size_t                                     m_maxActiveDownloads;
  std::chrono::seconds                       m_perHostTimeout;
};

struct QueuePopper {
  DownloadQueues* downloadQueues;

  auto operator()(DownloadQueues::DownloadQueueIt dwQueue) {
    LOG_DEBUG("QueuePopper operator() downloadQueues: " << downloadQueues);
    return [downloadQueues = this->downloadQueues, dwQueue]() {
      LOG_DEBUG("downloadQueues:" << downloadQueues);
      return downloadQueues->popDownload(dwQueue);
    };
  }
};

class DownloadFinishedAction {
public:
  void operator()(DownloadQueues::DownloadQueueIt dwQueue) {
    finishActions->push([dfa = *this, dwQueue]() mutable {
      LOG_DEBUG("Download finished download: " << *dfa.downloadList);
      LOG_DEBUG("Download finished, downloadList: " << *dfa.downloadList);
      LOG_DEBUG("DownloadQueue: " << dwQueue);
      if(!dfa.downloadList->empty(dwQueue)) {
        LOG_DEBUG("DownloadQueue not empty");
        dfa.timeHeap->push(dfa.queuePopper(dwQueue), dfa.perHostTimeout);
      }
      else {
        LOG_DEBUG("DownloadQueue empty");
        dfa.downloadList->erase(dwQueue);
      }
      --(*dfa.activeDownloads);
    });
  }

public:
  QueuePopper          queuePopper;
  DownloadQueues*      downloadList;
  TimeHeap*            timeHeap;
  size_t*              activeDownloads;
  ActionQueue*         finishActions;
  std::chrono::seconds perHostTimeout;
};

void
Crawler::Pimpl::crawl() {
  LOG_DEBUG("Crawler::Pimpl::crawl start crawling");
  size_t activeDownloads = 0;
  while(m_keepCrawling()) {
    TimeHeap               timeHeap;
    DownloadQueues         downloadList;
    QueuePopper            queuePopper{&downloadList};
    ActionQueue            finishActions;
    DownloadFinishedAction dfa{
        queuePopper, &downloadList, &timeHeap, &activeDownloads, &finishActions, m_perHostTimeout};
    populateDownloadQueuesWithRobots(&downloadList, m_dispatcher(), dfa);

    timeHeap = TimeHeap{downloadList, downloadList.size(), queuePopper};

    while(!downloadList.empty() || activeDownloads > 0 || !timeHeap.empty()) {
      if(!canAddDownload(activeDownloads, m_maxActiveDownloads) || timeHeap.empty()) {
        LOG_DEBUG("Waiting for downloads to finished. activeDownloads: " << activeDownloads << " m_maxActiveDownloads: "
                                                                         << m_maxActiveDownloads
                                                                         << " empty timeHeap: " << timeHeap.empty());
        finishActions.executeOrWaitAndExecute();
      }
      else {
        auto crtTime = std::chrono::steady_clock::now();
        if(crtTime < timeHeap.topTime()) {
          LOG_DEBUG("Waiting for downloads with timeout");
          finishActions.executeOrWaitAndExecuteOrWaitUntil(timeHeap.topTime());
        }
        else {
          do {
            LOG_DEBUG("Adding new download");
            m_downloader->download(timeHeap.pop().downloadElem);
            ++activeDownloads;
          } while(::canAddDownload(activeDownloads, m_maxActiveDownloads) && !timeHeap.empty()
                  && crtTime >= timeHeap.topTime());
          finishActions.execute();
        }
      }
    }
    if(!timeHeap.empty()) {
      throw std::logic_error("Internal ERROR: downloadQueue empty but timeHeap not empty");
    }
  }
  LOG_DEBUG("Bye bye birdie");
}

// class Crawler
Crawler::Crawler(std::function<bool()>&&                      keepCrawling,
                 std::function<std::vector<DownloadElem>()>&& dispatcher,
                 Downloader*                                  downloader,
                 size_t                                       maxActiveQueues,
                 std::chrono::seconds                         perHostTimeout)
    : m_pimpl{new Pimpl{std::move(keepCrawling), std::move(dispatcher), downloader, maxActiveQueues, perHostTimeout}} {}

Crawler::~Crawler() {}

void
Crawler::crawl() {
  m_pimpl->crawl();
}

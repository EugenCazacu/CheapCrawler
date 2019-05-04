#ifndef CRAWLER_DOWNLOADQUEUES_H_3VTDLUMK
#define CRAWLER_DOWNLOADQUEUES_H_3VTDLUMK

#include "Url.h"
#include <iterator>
#include <map>
#include <ostream>

class DownloadQueues {
public:
  using DownloadQueueIt = std::map<std::string, DownloadQueue>::iterator;

  DownloadQueueIt getQueueByHost(const std::string& host) {
    auto dwListMapIt = m_downloads.find(host);
    if(end() == dwListMapIt) {
      dwListMapIt = m_downloads.insert(std::make_pair(host, DownloadQueue{})).first;
    }
    return dwListMapIt;
  }

  void addDownload(DownloadQueueIt dwQueue, DownloadElem download) {
    dwQueue->second.emplace_back(std::move(download));
  }

  DownloadElem popDownload(DownloadQueueIt dwQueue) {
    if(dwQueue->second.empty()) {
      throw std::runtime_error("DownloadQueues ERROR: poping from empty queue");
    }
    DownloadElem result = dwQueue->second.back();
    dwQueue->second.pop_back();
    return result;
  }

  DownloadQueueIt begin() { return m_downloads.begin(); }

  DownloadQueueIt end() { return m_downloads.end(); }

  void erase(DownloadQueueIt dwQueue) { m_downloads.erase(dwQueue); }

  size_t size(DownloadQueueIt dwQueue) const { return dwQueue->second.size(); }

  size_t size() const { return m_downloads.size(); }

  bool empty(DownloadQueueIt dwQueue) const { return dwQueue->second.empty(); }

  bool empty() const { return m_downloads.empty(); }

private:
  std::map<std::string, DownloadQueue> m_downloads;
};

inline std::ostream&
operator<<(std::ostream& out, DownloadQueues::DownloadQueueIt dwQueue) {
  out << "Host: " << dwQueue->first << std::endl;
  std::copy(begin(dwQueue->second), end(dwQueue->second), std::ostream_iterator<DownloadElem>(out, "\n"));
  return out;
}

inline std::ostream&
operator<<(std::ostream& out, DownloadQueues& dwQueues) {
  std::for_each(std::begin(dwQueues), std::end(dwQueues), [&](const auto& dwQueue) {
    out << "Host: " << dwQueue.first << std::endl;
    std::copy(begin(dwQueue.second), end(dwQueue.second), std::ostream_iterator<DownloadElem>(out, "\n"));
  });
  return out;
}

#endif /* end of include guard: CRAWLER_DOWNLOADQUEUES_H_3VTDLUMK */

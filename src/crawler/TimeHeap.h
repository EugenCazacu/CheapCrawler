#ifndef CRAWLER_TIMEHEAP_H_D3FKCNVI
#define CRAWLER_TIMEHEAP_H_D3FKCNVI

#include "DownloadQueues.h"

#include <algorithm>
#include <chrono>
#include <list>
#include <vector>

class TimeHeap {
public:
  using SteadyTime = std::chrono::time_point<std::chrono::steady_clock>;

  TimeHeap() {}

  template<typename Func> TimeHeap(DownloadQueues& dwQueues, const size_t sizeHint, Func functor) {
    SteadyTime now = std::chrono::steady_clock::now();
    m_heap.reserve(sizeHint);
    for(auto queueIt = std::begin(dwQueues); queueIt != std::end(dwQueues); ++queueIt) {
      m_heap.push_back(make_tuple(now, functor(queueIt)));
    }
    // No need to call make_heap as all elements are equal and should fulfull the heap requirements.
  }

  bool empty() const { return m_heap.empty(); }

  DownloadElem pop() {
    if(empty()) {
      throw std::logic_error("error trying to pop from empty TimeHeap");
    }
    std::pop_heap(begin(m_heap), end(m_heap), HeapCmp());
    auto result = std::get<1>(m_heap.back())();
    m_heap.pop_back();
    return result;
  }

  void push(std::function<DownloadElem()> queueElem, std::chrono::seconds delay = std::chrono::seconds{0}) {
    m_heap.emplace_back(delay + std::chrono::steady_clock::now(), queueElem);
    push_heap(begin(m_heap), end(m_heap), HeapCmp());
  }

  SteadyTime topTime() {
    if(empty()) {
      throw std::logic_error("error trying to get topTime from empty TimeHeap");
    }
    return std::get<0>(m_heap.front());
  }

private:
  using HeapT = std::vector<std::tuple<SteadyTime, std::function<DownloadElem()>>>;
  HeapT m_heap;
  struct HeapCmp {
    bool operator()(HeapT::const_reference e1, HeapT::const_reference e2) { return std::get<0>(e2) < std::get<0>(e1); }
  };
};

#endif /* end of include guard: CRAWLER_TIMEHEAP_H_D3FKCNVI */

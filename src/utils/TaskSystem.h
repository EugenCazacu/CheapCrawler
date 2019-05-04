#ifndef UTILS_TASKSYSTEM_H_XYCSZEGK
#define UTILS_TASKSYSTEM_H_XYCSZEGK

#include "JobQueue.h"
#include "handleExceptions.h"
#include "unique_resource.h"

#include <atomic>
#include <thread>
#include <utility>
#include <vector>

class TaskSystem {
public:
  static constexpr unsigned DEFAULT_THREADS = 2;
  static auto               calcNrThreads() {
    const auto hardware_concurrency = std::thread::hardware_concurrency();
    return 0 == hardware_concurrency ? DEFAULT_THREADS : hardware_concurrency;
  }

  /**
   * @param nrThreads number of threads to use,
   * 0 - try get from hardware concurency, if hardware concurency detection fails use 2
   */
  TaskSystem(const unsigned nrThreads = 0)
      : m_hwConcurency{0 == nrThreads ? calcNrThreads() : nrThreads}
      , m_jobIndex{0}
      , m_jobQueues{m_hwConcurency}
      , m_threads{}
      , m_jobQueuesReleaser{m_hwConcurency} {
    const auto threadReleaser   = [](std::thread& t) { handleExceptions([&] { t.join(); }); };
    const auto jobQueueReleaser = [](JobQueue* jobQueue) { handleExceptions([=] { jobQueue->done(); }); };
    for(unsigned threadIndex = 0; threadIndex != m_hwConcurency; ++threadIndex) {
      m_jobQueuesReleaser[threadIndex]
          = std::unique_ptr<JobQueue, std::function<void(JobQueue*)>>(&m_jobQueues[threadIndex], jobQueueReleaser);
      m_threads.emplace_back(std::thread([&, threadIndex] { run(threadIndex); }), threadReleaser);
    }
  }

  /**
   * Run the given task asynchronously.
   * All internally thrown exceptions are handled with 'handleExceptions'
   */
  template<typename JobType> void async_(JobType&& job) {
    auto currentJobIndex = m_jobIndex++;
    for(unsigned threadIndexOffset = 0; threadIndexOffset != m_hwConcurency; ++threadIndexOffset) {
      const unsigned threadIndex    = (currentJobIndex + threadIndexOffset) % m_hwConcurency;
      const auto     tryPushSuccess = m_jobQueues[threadIndex].tryPush(std::forward<JobType>(job));
      if(tryPushSuccess) {
        return;
      }
    }
    m_jobQueues[currentJobIndex % m_hwConcurency].push(std::forward<JobType>(job));
  }

private:
  const unsigned                                                                                 m_hwConcurency;
  std::atomic<unsigned>                                                                          m_jobIndex;
  std::vector<JobQueue>                                                                          m_jobQueues;
  std::vector<std_experimental::unique_resource<std::thread, std::function<void(std::thread&)>>> m_threads;
  std::vector<std::unique_ptr<JobQueue, std::function<void(JobQueue*)>>>                         m_jobQueuesReleaser;

  void run(const unsigned threadIndex) {
    while(true) {
      std::function<void()> job;

      for(unsigned threadIndexOffset = 0; threadIndexOffset != m_hwConcurency * 32; ++threadIndexOffset) {
        if(m_jobQueues[(threadIndex + threadIndexOffset) % m_hwConcurency].tryPop(job))
          break;
      }
      if(!job && !m_jobQueues[threadIndex].pop(job)) {
        break;
      }
      handleExceptions(job);
    }
  }
};

#endif /* end of include guard: UTILS_TASKSYSTEM_H_XYCSZEGK */

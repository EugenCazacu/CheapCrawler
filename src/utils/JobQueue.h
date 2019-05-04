#ifndef UTILS_JOBQUEUE_H_NEYISSFQ
#define UTILS_JOBQUEUE_H_NEYISSFQ

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

class JobQueue {
public:
  JobQueue() : m_jobQueue{}, m_done{false}, m_mutex{}, m_ready{} {}

  bool tryPop(std::function<void()>& o_job) {
    std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};
    if(!lock || m_jobQueue.empty()) {
      return false;
    }
    o_job = move(m_jobQueue.front());
    m_jobQueue.pop_front();
    return true;
  }

  template<typename JobType> bool tryPush(JobType&& job) {
    {
      std::unique_lock<std::mutex> lock{m_mutex, std::try_to_lock};
      if(!lock) {
        return false;
      }
      m_jobQueue.emplace_back(std::forward<JobType>(job));
    }
    m_ready.notify_one();
    return true;
  }

  /*
   * Call to make the pop fail when no more tasks in the queue
   */
  void done() {
    {
      std::unique_lock<std::mutex> lock{m_mutex};
      m_done = true;
    }
    m_ready.notify_all();
  }

  bool pop(std::function<void()>& o_job) {
    std::unique_lock<std::mutex> lock{m_mutex};
    while(m_jobQueue.empty() && !m_done) {
      m_ready.wait(lock);
    }
    if(m_jobQueue.empty()) {
      return false;
    }
    o_job = move(m_jobQueue.front());
    m_jobQueue.pop_front();
    return true;
  }

  template<typename JobType> void push(JobType&& job) {
    {
      std::unique_lock<std::mutex> lock{m_mutex};
      m_jobQueue.emplace_back(std::forward<JobType>(job));
    }
    m_ready.notify_one();
  }

private:
  std::deque<std::function<void()>> m_jobQueue;
  bool                              m_done;
  std::mutex                        m_mutex;
  std::condition_variable           m_ready;
};

#endif /* end of include guard: UTILS_JOBQUEUE_H_NEYISSFQ */

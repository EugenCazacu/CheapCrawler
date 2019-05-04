#ifndef UTILS_NOTIFYBOX_H_KVRNHLLT
#define UTILS_NOTIFYBOX_H_KVRNHLLT

#include <condition_variable>
#include <mutex>

class NotifyBox {
public:
  NotifyBox() : mtx{}, cv{}, ready{false}, lck{mtx} {}

  ~NotifyBox() { waitWithTimeout(); }

  void waitWithTimeout() {
    cv.wait_for(lck, std::chrono::seconds(10), [&]() { return ready; });
  }

  void operator()() {
    std::lock_guard<std::mutex> lck{mtx};
    ready = true;
    cv.notify_one();
  }

private:
  std::mutex                   mtx;
  std::condition_variable      cv;
  bool                         ready = false;
  std::unique_lock<std::mutex> lck;
};

#endif /* end of include guard: UTILS_NOTIFYBOX_H_KVRNHLLT */

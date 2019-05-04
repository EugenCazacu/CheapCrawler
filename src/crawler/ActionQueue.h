#ifndef CRAWLER_ACTIONQUEUE_H_QXC2K1VJ
#define CRAWLER_ACTIONQUEUE_H_QXC2K1VJ

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <vector>

/**
 * The ActionQueue is designed with multi threaded use in mind.
 * Multiple threads can simultaneously push and execute the actions from this queue.
 */
class ActionQueue {
public:
  /**
   * Waits for an action to be added and executes it.
   * If actions are already in the queue, pop them all an execute.
   */
  void executeOrWaitAndExecute() {
    Actions toBeExecuted;

    LOG_DEBUG("ActionQueue executeOrWaitAndExecute ...");
    std::unique_lock<std::mutex> lock{m_mutex};
    LOG_DEBUG("ActionQueue executeOrWaitAndExecute locked, prepared to wait ...");
    m_conditionVariable.wait(lock, [this]() { return !m_actions.empty(); });
    swap(m_actions, toBeExecuted);
    lock.unlock();

    LOG_DEBUG("ActionQueue executeOrWaitAndExecute executing");
    for(auto& action: toBeExecuted) {
      action();
    }
  }

  /**
   * Waits for an action to be added and executes it.
   * If actions are already in the queue, pop them all an execute.
   * @param time max wait absolute time
   */
  void executeOrWaitAndExecuteOrWaitUntil(std::chrono::time_point<std::chrono::steady_clock> time) {
    Actions toBeExecuted;

    LOG_DEBUG("ActionQueue executeOrWaitAndExecuteOrWaitUntil ...");
    std::unique_lock<std::mutex> lock{m_mutex};
    LOG_DEBUG("ActionQueue executeOrWaitAndExecuteOrWaitUntil locked, prepared to wait_until ...");
    m_conditionVariable.wait_until(lock, time, [this]() { return !m_actions.empty(); });
    swap(m_actions, toBeExecuted);
    lock.unlock();
    LOG_DEBUG("ActionQueue executeOrWaitAndExecuteOrWaitUntil executing");

    for(auto& action: toBeExecuted) {
      action();
    }
  }

  /**
   * Execute all actions in the queue. If empty, do nothing
   */
  void execute() {
    auto toBeExecuted = getCurrentActions();
    LOG_DEBUG("ActionQueue execute size: " << toBeExecuted.size());
    for(auto& action: toBeExecuted) {
      action();
    }
  }

  /**
   * Push a new action into the queue
   */
  void push(std::function<void()> func) {
    LOG_DEBUG("ActionQueue pushing ... ");
    std::lock_guard<std::mutex> lock{m_mutex};
    LOG_DEBUG("ActionQueue pushing locked");
    m_actions.push_back(std::move(func));
    m_conditionVariable.notify_one();
  }

private:
  using Actions = std::vector<std::function<void()>>;

  Actions getCurrentActions() {
    Actions result;
    LOG_DEBUG("ActionQueue getting actions ... ");
    std::lock_guard<std::mutex> lock{m_mutex};
    LOG_DEBUG("ActionQueue getting actions locked");
    if(!m_actions.empty()) {
      swap(result, m_actions);
    }
    return result;
  }

private:
  Actions                 m_actions;
  std::mutex              m_mutex;
  std::condition_variable m_conditionVariable;
};

#endif /* end of include guard: CRAWLER_ACTIONQUEUE_H_QXC2K1VJ */

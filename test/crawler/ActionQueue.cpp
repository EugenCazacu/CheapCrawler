#include "Logger.h"
LOG_INIT(ActionQueue_tests);

#include "ActionQueue.h"
#include "NotifyBox.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <chrono>
#include <future>
#include <thread>

class ActionMock {
public:
  MOCK_CONST_METHOD0(execute, void());
};

struct ActionQueueFixture : public ::testing::Test {
  ActionQueueFixture() : Test{} {}
  ActionMock  actionMock;
  ActionQueue actionQueue;
};

namespace {} // namespace

TEST_F(ActionQueueFixture, initializeEmptyAndExecute) { actionQueue.execute(); }

TEST_F(ActionQueueFixture, initializeAndExecute) {
  actionQueue.push([this]() { actionMock.execute(); });
  EXPECT_CALL(actionMock, execute());
  actionQueue.execute();
}

TEST_F(ActionQueueFixture, executeOrWaitAndExecute_execute) {
  actionQueue.push([this]() { actionMock.execute(); });
  EXPECT_CALL(actionMock, execute());
  actionQueue.executeOrWaitAndExecute();
}

TEST_F(ActionQueueFixture, executeOrWaitAndExecute_waitUntilReady) {
  EXPECT_CALL(actionMock, execute());
  NotifyBox         notification;
  std::future<void> actionAdder = std::async(std::launch::async, [this, &notification]() {
    notification();
    actionQueue.executeOrWaitAndExecute();
  });
  notification.waitWithTimeout();
  actionQueue.push([this]() { actionMock.execute(); });
}

TEST_F(ActionQueueFixture, executeOrWaitAndExecute_2xwaitUntilReady) {
  EXPECT_CALL(actionMock, execute()).Times(2);
  NotifyBox         notification;
  std::future<void> actionAdder1 = std::async(std::launch::async, [&, this]() {
    notification.waitWithTimeout();
    actionQueue.push([this]() { actionMock.execute(); });
  });
  notification();
  actionQueue.executeOrWaitAndExecute();
  actionQueue.push([this]() { actionMock.execute(); });
  actionQueue.executeOrWaitAndExecute();
}

TEST_F(ActionQueueFixture, executeOrWaitAndExecuteOrWaitUntil_timeout) {
  auto testStartTime          = std::chrono::steady_clock::now();
  auto expectedEarliestFinish = testStartTime + std::chrono::milliseconds(1);
  actionQueue.executeOrWaitAndExecuteOrWaitUntil(expectedEarliestFinish);
  auto testFinishTime = std::chrono::steady_clock::now();
  EXPECT_TRUE(testFinishTime >= expectedEarliestFinish);
}

TEST_F(ActionQueueFixture, executeOrWaitAndExecuteOrWaitUntil_timeoutWithAction) {
  EXPECT_CALL(actionMock, execute()).Times(0);
  auto testStartTime          = std::chrono::steady_clock::now();
  auto expectedEarliestFinish = testStartTime + std::chrono::milliseconds(1);

  actionQueue.executeOrWaitAndExecuteOrWaitUntil(expectedEarliestFinish);
  auto testFinishTime = std::chrono::steady_clock::now();
  EXPECT_TRUE(testFinishTime >= expectedEarliestFinish);

  std::future<void> actionAdder
      = std::async(std::launch::async, [this]() { actionQueue.push([this]() { actionMock.execute(); }); });
}

TEST_F(ActionQueueFixture, executeOrWaitAndExecuteOrWaitUntil_2xActionsDirectlyExecute) {
  EXPECT_CALL(actionMock, execute()).Times(2);
  auto testStartTime        = std::chrono::steady_clock::now();
  auto expectedLatestFinish = testStartTime + std::chrono::seconds(1);
  actionQueue.push([this]() { actionMock.execute(); });
  actionQueue.push([this]() { actionMock.execute(); });

  actionQueue.executeOrWaitAndExecuteOrWaitUntil(expectedLatestFinish);
  auto testFinishTime = std::chrono::steady_clock::now();
  EXPECT_TRUE(testFinishTime < expectedLatestFinish);
}

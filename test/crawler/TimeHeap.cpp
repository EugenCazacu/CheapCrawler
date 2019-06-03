#include "TimeHeap.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <algorithm>

struct TimeHeapFixture : public ::testing::Test {
  TimeHeapFixture()
      : Test{}
      , url1_1{"http://url1.com", 0}
      , url1_2{"http://url1.com/23.htm", 1}
      , url2_1{"http://url2.com", 2}
      , url2_2{"http://url2.com/23.htm", 3}
      , dwList2x2Urls{}
      , dwQueueItUrl1{dwList2x2Urls.getQueueByHost("url1.com")}
      , dwQueueItUrl2{dwList2x2Urls.getQueueByHost("url2.com")} {
    addDownload(dwQueueItUrl1, {url1_1});
    addDownload(dwQueueItUrl1, {url1_2});
    addDownload(dwQueueItUrl2, {url2_1});
    addDownload(dwQueueItUrl2, {url2_2});
  }
  Url                             url1_1;
  Url                             url1_2;
  Url                             url2_1;
  Url                             url2_2;
  DownloadQueues                  dwList2x2Urls;
  DownloadQueues::DownloadQueueIt dwQueueItUrl1;
  DownloadQueues::DownloadQueueIt dwQueueItUrl2;
};

namespace {

struct QueuePopper {
  DownloadQueues* downloadQueues;
  auto            operator()(DownloadQueues::DownloadQueueIt dwQueue) {
    return [=]() { return downloadQueues->popDownload(dwQueue); };
  }
};

} // namespace

TEST(TimeHeap, initializeEmpty) {
  DownloadQueues emptyQueue;
  TimeHeap       timeHeap{emptyQueue, 0, QueuePopper{&emptyQueue}};
  EXPECT_TRUE(timeHeap.empty());
  EXPECT_THROW(timeHeap.topTime(), std::logic_error);
  EXPECT_THROW(timeHeap.pop(), std::logic_error);
}

TEST_F(TimeHeapFixture, initialize) {
  TimeHeap timeHeap{dwList2x2Urls, 2, QueuePopper{&dwList2x2Urls}};
  EXPECT_GT(std::chrono::steady_clock::now(), timeHeap.topTime());
  EXPECT_FALSE(timeHeap.empty());
}

TEST_F(TimeHeapFixture, popOnly) {
  TimeHeap         timeHeap{dwList2x2Urls, 2, QueuePopper{&dwList2x2Urls}};
  std::vector<Url> poped;
  for(int i = 0; i < 2; ++i) {
    poped.push_back(timeHeap.pop().url);
  }
  EXPECT_TRUE(timeHeap.empty());
  EXPECT_THAT(poped, testing::UnorderedElementsAre(url1_2, url2_2));
  EXPECT_THROW(timeHeap.topTime(), std::logic_error);
  EXPECT_THROW(timeHeap.pop(), std::logic_error);
}

TEST_F(TimeHeapFixture, pushAndPop) {
  DownloadQueues emptyQueue;
  TimeHeap       timeHeap{emptyQueue, 2, QueuePopper{&emptyQueue}};
  timeHeap.push(QueuePopper{&emptyQueue}(dwQueueItUrl1));
  EXPECT_EQ(timeHeap.pop().url, url1_2);
  EXPECT_TRUE(timeHeap.empty());
}

TEST_F(TimeHeapFixture, multiplePushAndPop) {
  DownloadQueues emptyQueue;
  TimeHeap       timeHeap{emptyQueue, 2, QueuePopper{&emptyQueue}};
  timeHeap.push(QueuePopper{&emptyQueue}(dwQueueItUrl1));
  timeHeap.push(QueuePopper{&emptyQueue}(dwQueueItUrl2));
  timeHeap.push(QueuePopper{&emptyQueue}(dwQueueItUrl1));
  timeHeap.push(QueuePopper{&emptyQueue}(dwQueueItUrl2));
  EXPECT_EQ(timeHeap.pop().url, url1_2);
  EXPECT_EQ(timeHeap.pop().url, url2_2);
  EXPECT_EQ(timeHeap.pop().url, url1_1);
  EXPECT_EQ(timeHeap.pop().url, url2_1);
  EXPECT_TRUE(timeHeap.empty());
}

TEST_F(TimeHeapFixture, multipleInterleavingPushAndPop) {
  DownloadQueues emptyQueue;
  TimeHeap       timeHeap{emptyQueue, 2, QueuePopper{&emptyQueue}};

  timeHeap.push(QueuePopper{&emptyQueue}(dwQueueItUrl1));
  timeHeap.push(QueuePopper{&emptyQueue}(dwQueueItUrl1));
  EXPECT_EQ(timeHeap.pop().url, url1_2);
  timeHeap.push(QueuePopper{&emptyQueue}(dwQueueItUrl2));
  timeHeap.push(QueuePopper{&emptyQueue}(dwQueueItUrl2));
  EXPECT_EQ(timeHeap.pop().url, url1_1);
  EXPECT_EQ(timeHeap.pop().url, url2_2);
  EXPECT_EQ(timeHeap.pop().url, url2_1);
  EXPECT_TRUE(timeHeap.empty());
}

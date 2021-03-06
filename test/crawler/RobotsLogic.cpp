#include "RobotsLogic.h"
#include "DownloadResult.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <algorithm>

using ::testing::_;

struct DownloadFinishedMock {
  MOCK_METHOD1(downloadFinishedProxy, void(DownloadQueues::DownloadQueueIt));
};

struct DwFinishedCallback {
  DownloadFinishedMock* dwFinishedMock;
  void                  operator()(DownloadQueues::DownloadQueueIt it) { dwFinishedMock->downloadFinishedProxy(it); }
};

struct RobotsLogicFixture : public ::testing::Test {
  RobotsLogicFixture() : Test{}, queues{}, dwFinishedMock{}, dwFinishedCallback{&dwFinishedMock} {
    EXPECT_CALL(dwFinishedMock, downloadFinishedProxy(_)).Times(0);
  }

  DownloadQueues       queues;
  DownloadFinishedMock dwFinishedMock;
  DwFinishedCallback   dwFinishedCallback;
};

TEST_F(RobotsLogicFixture, buildQueues) {
  populateDownloadQueuesWithRobots(&queues,
                                   std::vector<DownloadElem>{
                                       {{"http://url.com", 1}, [](DownloadResult&&) {}},
                                   },
                                   dwFinishedCallback);
  ASSERT_EQ(queues.size(), 1);
  auto downloadQueueIt = std::begin(queues);
  ASSERT_EQ(queues.size(downloadQueueIt), 1);
  EXPECT_EQ(std::get<0>(queues.popDownload(downloadQueueIt).url), "http://url.com/robots.txt");
}

TEST_F(RobotsLogicFixture, noRobotsForBadUris) {
  populateDownloadQueuesWithRobots(&queues,
                                   std::vector<DownloadElem>{{{"http://url.com dsadas", 1}, [](auto) {}},
                                                             {{"ftp://url.com dsadas", 2}, [](auto) {}},
                                                             {{"ftp://url.com", 3}, [](auto) {}},
                                                             {{"http://url.com#23", 4}, [](auto) {}}},
                                   dwFinishedCallback);

  ASSERT_EQ(queues.size(), 0);
}

TEST_F(RobotsLogicFixture, onlyOneRobotPerProtocolAndHost) {
  populateDownloadQueuesWithRobots(&queues,
                                   std::vector<DownloadElem>{{{"http://url.com/dsjaklj", 1}, [](auto) {}},
                                                             {{"http://url.com/dsjaklj?q=34", 1}, [](auto) {}},
                                                             {{"http://url.com/q=34", 1}, [](auto) {}}},
                                   dwFinishedCallback);

  ASSERT_EQ(queues.size(), 1);
  auto downloadQueueIt = std::begin(queues);
  ASSERT_EQ(queues.size(downloadQueueIt), 1);
  EXPECT_EQ(std::get<0>(queues.popDownload(downloadQueueIt).url), "http://url.com/robots.txt");
}

TEST_F(RobotsLogicFixture, oneDownloadQueuePerHost) {
  populateDownloadQueuesWithRobots(&queues,
                                   std::vector<DownloadElem>{
                                       {{"http://url1.com/dsjaklj", 1}, [](auto) {}},
                                       {{"http://url1.com/dsjaklj?q=34", 2}, [](auto) {}},
                                       {{"https://url1.com/q=34", 3}, [](auto) {}},
                                   },
                                   dwFinishedCallback);

  ASSERT_EQ(1, queues.size());
  auto downloadQueueIt = queues.begin();
  ASSERT_EQ(2, queues.size(downloadQueueIt));
  std::vector<std::string> urls{std::get<0>(queues.popDownload(downloadQueueIt).url),
                                std::get<0>(queues.popDownload(downloadQueueIt).url)};

  EXPECT_THAT(urls, testing::UnorderedElementsAre("http://url1.com/robots.txt", "https://url1.com/robots.txt"));
}

TEST_F(RobotsLogicFixture, separateQueueForEachHost) {
  populateDownloadQueuesWithRobots(&queues,
                                   std::vector<DownloadElem>{
                                       {{"http://url1.com/dsjaklj", 1}, [](auto) {}},
                                       {{"http://url1.com/dsjaklj?q=34", 2}, [](auto) {}},
                                       {{"https://url2.com/q=34", 3}, [](auto) {}},
                                   },
                                   dwFinishedCallback);

  ASSERT_EQ(2, queues.size());
  std::vector<std::string> urls;
  for(auto downloadQueueIt = std::begin(queues); downloadQueueIt != std::end(queues); ++downloadQueueIt) {
    ASSERT_EQ(1, queues.size(downloadQueueIt));
    urls.push_back(std::get<0>(queues.popDownload(downloadQueueIt).url));
  }

  EXPECT_THAT(urls, testing::UnorderedElementsAre("http://url1.com/robots.txt", "https://url2.com/robots.txt"));
}

TEST_F(RobotsLogicFixture, oneRobotForEachProtocolAndHost) {
  populateDownloadQueuesWithRobots(&queues,
                                   std::vector<DownloadElem>{{{"http://url1.com/dsjaklj", 1}, [](auto) {}},
                                                             {{"http://url1.com/dsjaklj?q=34", 2}, [](auto) {}},
                                                             {{"https://url1.com/q=34", 3}, [](auto) {}},
                                                             {{"http://url2.com/dsjaklj", 4}, [](auto) {}},
                                                             {{"http://url2.com/dsjaklj?q=34", 5}, [](auto) {}},
                                                             {{"https://url2.com/q=34", 6}, [](auto) {}}},
                                   dwFinishedCallback);

  ASSERT_EQ(2, queues.size());
  auto downloadQueueIt1 = queues.begin();
  auto downloadQueueIt2 = ++queues.begin();
  ASSERT_EQ(2, queues.size(downloadQueueIt1));
  ASSERT_EQ(2, queues.size(downloadQueueIt2));
  std::vector<std::string> urls{std::get<0>(queues.popDownload(downloadQueueIt1).url),
                                std::get<0>(queues.popDownload(downloadQueueIt1).url),
                                std::get<0>(queues.popDownload(downloadQueueIt2).url),
                                std::get<0>(queues.popDownload(downloadQueueIt2).url)};

  EXPECT_THAT(urls,
              testing::UnorderedElementsAre("http://url1.com/robots.txt",
                                            "https://url1.com/robots.txt",
                                            "http://url2.com/robots.txt",
                                            "https://url2.com/robots.txt"));
}

namespace {
struct DownloadFinishedCallbackMock {
  MOCK_METHOD1(cb, void(DownloadResult&));
};
} // namespace
struct RobotsLogicWithDownloadCallback : public RobotsLogicFixture {
  DownloadFinishedCallbackMock downloadCallback;
};

TEST_F(RobotsLogicWithDownloadCallback, urlDownloadFinishedCalledByCallback) {
  populateDownloadQueuesWithRobots(
      &queues,
      std::vector<DownloadElem>{
          {{"http://url.com", 1}, [this](DownloadResult&& result) { downloadCallback.cb(result); }},
      },
      dwFinishedCallback);
  ASSERT_EQ(queues.size(), 1);
  auto downloadQueueIt = std::begin(queues);
  ASSERT_EQ(queues.size(downloadQueueIt), 1);
  DownloadElem download = queues.popDownload(downloadQueueIt);
  EXPECT_CALL(dwFinishedMock, downloadFinishedProxy(downloadQueueIt));
  EXPECT_CALL(downloadCallback, cb(_)).Times(0);
  download.callback(DownloadResult{});
}

TEST_F(RobotsLogicWithDownloadCallback, urlDownloadAfterFinishedRobots) {
  populateDownloadQueuesWithRobots(
      &queues,
      std::vector<DownloadElem>{
          {{"http://url.com", 1}, [this](DownloadResult&& result) { downloadCallback.cb(result); }},
      },
      dwFinishedCallback);
  ASSERT_EQ(queues.size(), 1);
  auto downloadQueueIt = std::begin(queues);
  ASSERT_EQ(queues.size(downloadQueueIt), 1);
  DownloadElem download = queues.popDownload(downloadQueueIt);
  EXPECT_CALL(dwFinishedMock, downloadFinishedProxy(downloadQueueIt));
  download.callback(DownloadResult{});
  ASSERT_EQ(queues.size(downloadQueueIt), 1);
  DownloadElem urlDownload = queues.popDownload(downloadQueueIt);
  EXPECT_EQ(std::get<0>(urlDownload.url), "http://url.com");
  EXPECT_CALL(downloadCallback, cb(_));
  EXPECT_CALL(dwFinishedMock, downloadFinishedProxy(downloadQueueIt));
  urlDownload.callback(DownloadResult{});
}

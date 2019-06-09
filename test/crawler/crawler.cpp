#include "Logger.h"
LOG_INIT(crawler_tests);

#include "ResultCallbackMock.h"
#include "DownloadResult.h"
#include "crawler/crawler.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#if !GTEST_IS_THREADSAFE
#error Can only run this test suit when gmock is thread safe
#endif

#include <algorithm>
#include <chrono>
#include <future>
#include <random>
#include <thread>
#include <unordered_map>

#include "uriUtils/uriUtils.h"
#include <uriparser/Uri.h>

using ::testing::_;
using ::testing::Eq;
using ::testing::Expectation;
using ::testing::Field;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::Test;

namespace {

std::string
getHost(const std::string& url) {
  const auto      uriReleaser = [](UriUriA* uri) { uriFreeUriMembersA(uri); };
  UriParserStateA state;
  UriUriA         uri;
  state.uri = &uri;

  std::unique_ptr<UriUriA, decltype(uriReleaser)> uriRaii(&uri, uriReleaser);
  int                                             callResult;
  if(URI_SUCCESS != (callResult = uriParseUriA(&state, url.c_str()))) {
    throw std::runtime_error("failed to parse error:" + std::to_string(callResult) + " url: " + url);
  }
  if(!isValid(uri)) {
    throw std::runtime_error("invalid url received: " + url);
  }

  return std::string{uri.hostText.first, uri.hostText.afterLast};
}

} // namespace
class RunControllMock {
public:
  MOCK_CONST_METHOD0(shouldRun, bool());
};

class DispatcherMock {
public:
  MOCK_CONST_METHOD0(doGetUrls, std::vector<DownloadElem>());
};

class DownloaderMock : public Downloader {
private:
  void doDownload(DownloadElem&& elem) {
    LOG_DEBUG("[DownloaderMock::doDownload] newDownload: " << elem);
    const std::string host       = getHost(elem.url.url);
    auto              hostTimeIt = m_hostTime.find(host);
    SteadyTime        now        = std::chrono::steady_clock::now();
    if(hostTimeIt != end(m_hostTime) && (now < (hostTimeIt->second + m_expectedDelay))) {
      std::chrono::steady_clock::duration remaining  = (now - hostTimeIt->second) - m_expectedDelay;
      std::chrono::milliseconds           durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
      throw std::logic_error("Too short time between downloads detected for: " + elem.url.url
                             + " Remaining expected time (ms): " + std::to_string(durationMs.count()));
    }
    m_hostTime[host]   = now;
    size_t nrDownloads = m_crtNrDownloads.fetch_add(1, std::memory_order_relaxed);
    if(maxNrDownloads != 0 && (nrDownloads + 1) > maxNrDownloads) {
      throw std::logic_error("DownloaderMock detected too many simultaneous downloads");
    }
    doDownloadProxy(elem);
    int               downloadTime = m_dist10(m_rng);
    std::future<void> callbackResult
        = std::async(std::launch::async, [elem = std::move(elem), &nrDownloads = m_crtNrDownloads, downloadTime]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(downloadTime));
            nrDownloads.fetch_sub(1, std::memory_order_relaxed);
            elem.callback(DownloadResult{elem.url});
          });
    m_downloads.emplace_back(std::move(callbackResult));
  }

public:
  void checkHostTimeouts(std::chrono::seconds expectedDelay) { m_expectedDelay = expectedDelay; }

  std::vector<std::future<void>> m_downloads;
  MOCK_METHOD1(doDownloadProxy, void(DownloadElem&));

  DownloaderMock(int rndSeed)
      : maxNrDownloads{0}
      , m_rng{static_cast<std::uint_fast32_t>(rndSeed)}
      , m_dist10{1, 10}
      , m_crtNrDownloads{0}
      , m_expectedDelay{0} {}

  void finishDownloads() {
    for(auto& download: m_downloads) {
      if(download.valid()) {
        download.get();
      }
    }
  }

  ~DownloaderMock() { finishDownloads(); }
  size_t maxNrDownloads;

private:
  using SteadyTime = std::chrono::time_point<std::chrono::steady_clock>;
  std::mt19937                                             m_rng;
  std::uniform_int_distribution<std::mt19937::result_type> m_dist10;
  std::atomic_size_t                                       m_crtNrDownloads;
  std::chrono::seconds                                     m_expectedDelay;
  std::unordered_map<std::string, SteadyTime>              m_hostTime;
};

struct CrawlerFixture : public ::testing::Test {
  CrawlerFixture(size_t activeDownloads = 1, std::chrono::seconds perHostTimeout = std::chrono::seconds{0})
      : Test{}
      , runControllMock{}
      , dispatcherMock{}
      , downloaderMock{::testing::UnitTest::GetInstance()->random_seed()}
      , sampleHost1Url1{"http://url.com", 2}
      , sampleHost1Url2{"http://url.com/0", 1}
      , sampleHost1Url3{"http://url.com/1", 1}
      , sampleHost1Url4{"http://url.com/2", 1}
      , sampleHost1Url1s{"https://url.com/3", 3}
      , sampleHost2Url1{"http://url2.com/dsjaklj", 4}
      , sampleHost2Url2{"http://url2.com/dsjaklj?q=34", 5}
      , sampleHost2Url1s{"https://url2.com/q=34", 6}
      , sampleUrl1RobotsTxt{"http://url.com/robots.txt"}
      , sampleUrl1RobotsTxts{"https://url.com/robots.txt"}
      , sampleUrl2RobotsTxt{"http://url2.com/robots.txt"}
      , sampleUrl2RobotsTxts{"https://url2.com/robots.txt"}
      , crawler{[&]() { return runControllMock.shouldRun(); },
                [&]() { return dispatcherMock.doGetUrls(); },
                &downloaderMock,
                activeDownloads,
                perHostTimeout} {}

  /**
   * DO NOT CALL crawl directly!!!
   *
   * Calling crawl directly and not waiting for download to finish might lead to the
   * mock being called after the crawl finished, therefore failing the test randomly.
   * Also such a call might lead into calling a mock which has already been destroyed
   */
  void crawlAndWaitForDownloadsToFinish() { crawler.crawl(); }

  RunControllMock runControllMock;
  DispatcherMock  dispatcherMock;
  DownloaderMock  downloaderMock;

  ResultCallbackMock sampleHost1Url1;
  ResultCallbackMock sampleHost1Url2;
  ResultCallbackMock sampleHost1Url3;
  ResultCallbackMock sampleHost1Url4;

  ResultCallbackMock sampleHost1Url1s;

  ResultCallbackMock sampleHost2Url1;
  ResultCallbackMock sampleHost2Url2;
  ResultCallbackMock sampleHost2Url1s;

  std::string sampleUrl1RobotsTxt;
  std::string sampleUrl1RobotsTxts;
  std::string sampleUrl2RobotsTxt;
  std::string sampleUrl2RobotsTxts;

private:
  Crawler crawler;
};

TEST(Crawler, initialize) {
  RunControllMock runControllMock;
  DispatcherMock  dispatcherMock;
  DownloaderMock  downloaderMock{0};

  Crawler crawler([&]() { return runControllMock.shouldRun(); },
                  [&]() { return dispatcherMock.doGetUrls(); },
                  &downloaderMock,
                  /* maxActiveQueues */ 1,
                  /* perHostTimeout */ std::chrono::seconds{0});
}

TEST_F(CrawlerFixture, exitByController) {
  EXPECT_CALL(runControllMock, shouldRun()).WillOnce(Return(false));
  EXPECT_CALL(dispatcherMock, doGetUrls()).Times(0);
  crawlAndWaitForDownloadsToFinish();
}

TEST_F(CrawlerFixture, doGetUrlsCalled) {
  EXPECT_CALL(runControllMock, shouldRun()).Times(2).WillOnce(Return(true)).WillOnce(Return(false));
  EXPECT_CALL(dispatcherMock, doGetUrls());
  crawlAndWaitForDownloadsToFinish();
}

TEST_F(CrawlerFixture, doGetUrlsCalledMultipleTimes) {
  EXPECT_CALL(runControllMock, shouldRun()).WillOnce(Return(false));
  EXPECT_CALL(runControllMock, shouldRun()).Times(4).WillRepeatedly(Return(true)).RetiresOnSaturation();
  EXPECT_CALL(dispatcherMock, doGetUrls()).Times(4);
  crawlAndWaitForDownloadsToFinish();
}

struct CrawlerOnceFixture
    : public CrawlerFixture
    , public ::testing::WithParamInterface<size_t> {
  CrawlerOnceFixture(std::chrono::seconds perHostTimeout = std::chrono::seconds{0})
      : CrawlerFixture{GetParam(), perHostTimeout}, ::testing::WithParamInterface<size_t>{} {
    EXPECT_CALL(runControllMock, shouldRun()).Times(2).WillOnce(Return(true)).WillOnce(Return(false));
  }
};

INSTANTIATE_TEST_CASE_P(VariousMaxDownloads, CrawlerOnceFixture, ::testing::Values(1, 5));

MATCHER_P(match0thOfTuple, expected, "") { return (arg.url == expected); }
MATCHER_P(robotEq, expected, "") { return expected == arg.url.url; }

TEST_P(CrawlerOnceFixture, robotsTxtForUrl) {
  EXPECT_CALL(dispatcherMock, doGetUrls())
      .WillOnce(Return(std::vector<DownloadElem>{sampleHost1Url1.enableDownload()}));

  InSequence s;
  EXPECT_CALL(downloaderMock, doDownloadProxy(robotEq(sampleUrl1RobotsTxt)));
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url1.getUrl()))));
  crawlAndWaitForDownloadsToFinish();
}

TEST_P(CrawlerOnceFixture, noRobotsForBadUris) {
  EXPECT_CALL(dispatcherMock, doGetUrls())
      .WillOnce(Return(std::vector<DownloadElem>{{{"http://url.com dsadas", 1}, [](auto) {}},
                                                 {{"ftp://url.com dsadas", 2}, [](auto) {}},
                                                 {{"ftp://url.com", 3}, [](auto) {}},
                                                 {{"http://url.com#23", 4}, [](auto) {}}}));

  EXPECT_CALL(downloaderMock, doDownloadProxy(_)).Times(0);
  crawlAndWaitForDownloadsToFinish();
}

TEST_P(CrawlerOnceFixture, onlyOneRobotPerProtocolAndHost) {
  EXPECT_CALL(dispatcherMock, doGetUrls())
      .WillOnce(Return(std::vector<DownloadElem>{
          sampleHost1Url2.enableDownload(),
          sampleHost1Url3.enableDownload(),
          sampleHost1Url4.enableDownload(),
      }));

  Expectation robotsDownload = EXPECT_CALL(downloaderMock, doDownloadProxy(robotEq(sampleUrl1RobotsTxt)));
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url2.getUrl()))))
      .After(robotsDownload);
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url3.getUrl()))))
      .After(robotsDownload);
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url4.getUrl()))))
      .After(robotsDownload);

  crawlAndWaitForDownloadsToFinish();
}

TEST_P(CrawlerOnceFixture, oneRobotForEachProtocol) {
  EXPECT_CALL(dispatcherMock, doGetUrls())
      .WillOnce(Return(std::vector<DownloadElem>{
          sampleHost1Url1.enableDownload(),
          sampleHost1Url2.enableDownload(),
          sampleHost1Url1s.enableDownload(),
      }));

  Expectation rob1  = EXPECT_CALL(downloaderMock, doDownloadProxy(robotEq(sampleUrl1RobotsTxt)));
  Expectation rob1s = EXPECT_CALL(downloaderMock, doDownloadProxy(robotEq(sampleUrl1RobotsTxts)));

  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url1.getUrl())))).After(rob1);
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url2.getUrl())))).After(rob1);
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url1s.getUrl())))).After(rob1s);

  crawlAndWaitForDownloadsToFinish();
}

TEST_P(CrawlerOnceFixture, oneRobotForEachProtocolAndHost) {
  EXPECT_CALL(dispatcherMock, doGetUrls())
      .WillOnce(Return(std::vector<DownloadElem>{
          sampleHost1Url1.enableDownload(),
          sampleHost1Url2.enableDownload(),
          sampleHost1Url1s.enableDownload(),
          sampleHost2Url1.enableDownload(),
          sampleHost2Url2.enableDownload(),
          sampleHost2Url1s.enableDownload(),
      }));

  Expectation rob1  = EXPECT_CALL(downloaderMock, doDownloadProxy(robotEq(sampleUrl1RobotsTxt)));
  Expectation rob1s = EXPECT_CALL(downloaderMock, doDownloadProxy(robotEq(sampleUrl1RobotsTxts)));
  Expectation rob2  = EXPECT_CALL(downloaderMock, doDownloadProxy(robotEq(sampleUrl2RobotsTxt)));
  Expectation rob2s = EXPECT_CALL(downloaderMock, doDownloadProxy(robotEq(sampleUrl2RobotsTxts)));

  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url1.getUrl())))).After(rob1);
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url2.getUrl())))).After(rob1);
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost1Url1s.getUrl())))).After(rob1s);
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost2Url1.getUrl())))).After(rob2);
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost2Url2.getUrl())))).After(rob2);
  EXPECT_CALL(downloaderMock, doDownloadProxy(Field(&DownloadElem::url, Eq(sampleHost2Url1s.getUrl())))).After(rob2s);

  crawlAndWaitForDownloadsToFinish();
}

TEST_P(CrawlerOnceFixture, testMaxActiveQueues) {
  constexpr size_t          nrHosts     = 50;
  constexpr size_t          urlsPerHost = 10;
  std::vector<DownloadElem> urls;
  urls.reserve(nrHosts * urlsPerHost);
  for(size_t hostNr = 0; hostNr < nrHosts; ++hostNr)
    for(size_t urlNr = 0; urlNr < urlsPerHost; ++urlNr) {
      const int id = hostNr*urlNr;
      urls.push_back(DownloadElem{
          {"https://url" + std::to_string(hostNr) + ".com/" + std::to_string(urlNr), id}, [](auto) {}});
    }
  EXPECT_CALL(dispatcherMock, doGetUrls()).WillOnce(Return(urls));
  EXPECT_CALL(downloaderMock, doDownloadProxy(_)).Times(nrHosts + nrHosts * urlsPerHost);
  downloaderMock.maxNrDownloads = GetParam();

  crawlAndWaitForDownloadsToFinish();
}

struct CrawlerWithTimeoutFixture : public CrawlerOnceFixture {
  CrawlerWithTimeoutFixture() : CrawlerOnceFixture{std::chrono::seconds{2}} {}
};

INSTANTIATE_TEST_CASE_P(PerHostTimeout, CrawlerWithTimeoutFixture, ::testing::Values(1, 5));

TEST_P(CrawlerWithTimeoutFixture, timeBetweenDownloads) {
  EXPECT_CALL(dispatcherMock, doGetUrls())
      .WillOnce(Return(std::vector<DownloadElem>{
          sampleHost1Url1.enableDownload(),
          sampleHost1Url2.enableDownload(),
          sampleHost1Url1s.enableDownload(),
          sampleHost2Url1.enableDownload(),
          sampleHost2Url2.enableDownload(),
          sampleHost2Url1s.enableDownload(),
      }));

  downloaderMock.checkHostTimeouts(std::chrono::seconds(2));
  EXPECT_CALL(downloaderMock, doDownloadProxy(_)).Times(10);

  crawlAndWaitForDownloadsToFinish();
}

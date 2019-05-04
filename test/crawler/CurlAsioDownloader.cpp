#include "crawler/CurlAsioDownloader.h"
#include "DownloadResult.h"
#include "NotifyBox.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <thread>

using ::testing::AllOf;
using ::testing::ContainsRegex;
using ::testing::Field;

struct CurlAsioDownloaderFixture : public ::testing::Test {
  CurlAsioDownloaderFixture()
      : defaultMaxContentLength{1024 * 1024}, defaultMediaTypeValidator{[](auto) { return true; }} {}
  size_t                                defaultMaxContentLength;
  std::function<bool(const MediaType&)> defaultMediaTypeValidator;
};

TEST_F(CurlAsioDownloaderFixture, instance) {
  CurlAsioDownloader inst{defaultMaxContentLength, defaultMediaTypeValidator};
}

TEST_F(CurlAsioDownloaderFixture, invalidDownload) {
  CurlAsioDownloader inst{defaultMaxContentLength, defaultMediaTypeValidator};
  struct {
    MOCK_METHOD1(downloadResultCb, void(bool));
  } cbMock;
  EXPECT_CALL(cbMock, downloadResultCb(false));

  NotifyBox notification;
  inst.download({{}, [&](DownloadResult&& dwResult) {
                   cbMock.downloadResultCb(dwResult.success);
                   notification();
                 }});
}

TEST_F(CurlAsioDownloaderFixture, download) {
  CurlAsioDownloader inst{defaultMaxContentLength, defaultMediaTypeValidator};
  struct {
    MOCK_CONST_METHOD1(cb, void(const DownloadResult&));
  } downloadMock;

  EXPECT_CALL(downloadMock,
              cb(AllOf(Field(&DownloadResult::success, true), Field(&DownloadResult::content, ContainsRegex(".+")))));
  NotifyBox notification;
  inst.download({{"http://example.com", 0}, [&](DownloadResult&& result) {
                   downloadMock.cb(result);
                   notification();
                 }});
}

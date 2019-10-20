#ifndef CRAWLER_RESULTCALLBACKMOCK_H_KI1INH8A
#define CRAWLER_RESULTCALLBACKMOCK_H_KI1INH8A

#include <string>

#include "DownloadResult.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

class ResultCallbackMock {
public:
  ResultCallbackMock(const std::string& url, int index)
      : dwElem{{url, index}, [this](DownloadResult&& dwResult) {
                 LOG_DEBUG("ResultCallbackMock callback: " << dwResult.url);
                 cb(dwResult);
               }} {}

  DownloadElem enableDownload() {
    LOG_DEBUG("ResultCallbackMock enableDownload: " << dwElem.url);
    EXPECT_CALL(*this, cb(::testing::Field(&DownloadResult::url, ::testing::Eq(this->getUrl()))));
    return dwElem;
  }

  Url getUrl() const { return dwElem.url; }

private:
  MOCK_METHOD1(cb, void(DownloadResult&));
  DownloadElem dwElem;
};

#endif /* end of include guard: CRAWLER_RESULTCALLBACKMOCK_H_KI1INH8A */

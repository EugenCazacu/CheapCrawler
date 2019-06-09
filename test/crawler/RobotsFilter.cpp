#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RobotsFilter.h"
#include "RobotsTestData.h"
#include <exception>

TEST(RobotsFilter, init) {
  [[maybe_unused]] RobotsFilter robotsFilter { RobotsTxt_Empty };
}

TEST(RobotsFilter, badProtocol) {
  RobotsFilter robotsFilter { RobotsTxt_AllUrlsAllowed };
  bool tmp;
  EXPECT_THROW(tmp = robotsFilter.canDownload("https://example.com/1"), std::exception);
}

TEST(RobotsFilter, allUrlsAllowed) {
  RobotsFilter robotsFilter {
    RobotsTxt_AllUrlsAllowed
  };
  EXPECT_TRUE(robotsFilter.canDownload("http://example.com"));
  EXPECT_TRUE(robotsFilter.canDownload("http://example.com/1"));
}

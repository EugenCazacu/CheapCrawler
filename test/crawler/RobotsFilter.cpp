#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "RobotsFilter.h"
#include "RobotsTestData.h"
#include <exception>

TEST(RobotsFilter, init) {
  [[maybe_unused]] RobotsFilter robotsFilter { "TODO", RobotsTxt_Empty };
}

TEST(RobotsFilter, allUrlsAllowed) {
  RobotsFilter robotsFilter {
    "TODO", RobotsTxt_AllUrlsAllowed
  };
  EXPECT_TRUE(robotsFilter.canDownload(""));
  EXPECT_TRUE(robotsFilter.canDownload("/1"));
}

TEST(RobotsFilter, allUrlsForbidden) {
  RobotsFilter robotsFilter {
    "TODO", RobotsTxt_AllUrlsForbidden
  };
  EXPECT_FALSE(robotsFilter.canDownload("http://example.com"));
  EXPECT_FALSE(robotsFilter.canDownload("http://example.com/1"));
}

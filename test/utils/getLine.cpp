#include "getLine.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(getLine, emptyLine) {
  std::string_view nullString{};
  auto [resBegin, resEnd] = getLine(begin(nullString), end(nullString));
  EXPECT_EQ(begin(nullString), resBegin);
  EXPECT_EQ(end(nullString), resEnd);
}

TEST(getLine, emptyString) {
  std::string_view emptyString{""};
  auto [resBegin, resEnd] = getLine(begin(emptyString), end(emptyString));
  EXPECT_EQ(begin(emptyString), resBegin);
  EXPECT_EQ(end(emptyString), resEnd);
}

TEST(getLine, noLines) {
  std::string_view input{"\n"};
  auto [resBegin, resEnd] = getLine(begin(input), end(input));
  EXPECT_EQ(end(input), resBegin);
  EXPECT_EQ(end(input), resEnd);
}

TEST(getLine, noLinesLineFeed) {
  std::string_view input{"\r"};
  auto [resBegin, resEnd] = getLine(begin(input), end(input));
  EXPECT_EQ(end(input), resBegin);
  EXPECT_EQ(end(input), resEnd);
}

TEST(getLine, noLinesLfCr) {
  std::string_view input{"\r\n"};
  auto [resBegin, resEnd] = getLine(begin(input), end(input));
  EXPECT_EQ(end(input), resBegin);
  EXPECT_EQ(end(input), resEnd);
}

TEST(getLine, noLinesCfLf) {
  std::string_view input{"\n\r"};
  auto [resBegin, resEnd] = getLine(begin(input), end(input));
  EXPECT_EQ(end(input), resBegin);
  EXPECT_EQ(end(input), resEnd);
}

TEST(getLine, oneLineNoOther) {
  std::string_view input{"123"};
  auto [resBegin, resEnd] = getLine(begin(input), end(input));
  EXPECT_EQ(begin(input), resBegin);
  EXPECT_EQ(end(input), resEnd);
}

TEST(getLine, oneLineTrailingLf) {
  std::string_view input{"123\r"};
  auto [resBegin, resEnd] = getLine(begin(input), end(input));
  EXPECT_EQ(begin(input), resBegin);
  EXPECT_EQ(end(input)-1, resEnd);
}

TEST(getLine, oneLineStartCr) {
  std::string_view input{"\n123"};
  auto [resBegin, resEnd] = getLine(begin(input), end(input));
  EXPECT_EQ(begin(input)+1, resBegin);
  EXPECT_EQ(end(input), resEnd);
}

TEST(getLine, oneLineStartAndEndCr) {
  std::string_view input{"\n123\n"};
  auto [resBegin, resEnd] = getLine(begin(input), end(input));
  EXPECT_EQ(begin(input)+1, resBegin);
  EXPECT_EQ(end(input)-1, resEnd);
}

TEST(getLine, twoLinesStartCr) {
  std::string_view input{"\n123\n 456"};
  auto [resBegin, resEnd] = getLine(begin(input), end(input));
  EXPECT_EQ(begin(input)+1, resBegin);
  EXPECT_EQ(begin(input)+4, resEnd);
}



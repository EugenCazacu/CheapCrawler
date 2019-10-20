#include "RobotsTxtParser.h"

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

TEST(tryMatchStartGroupLine, emptyLine) {
  const auto agentName = tryMatchStartGroupLine(std::string_view{});
  EXPECT_FALSE(agentName);
}

TEST(tryMatchStartGroupLine, notAgentName) {
  const auto agentName = tryMatchStartGroupLine("u");
  EXPECT_FALSE(agentName);
}

TEST(tryMatchStartGroupLine, singleLetterAgent) {
  const auto agentName = tryMatchStartGroupLine("user-agent:a");
  ASSERT_TRUE(agentName);
  EXPECT_STREQ(std::string{*agentName}.c_str(), "a");
}

TEST(tryMatchStartGroupLine, singleLetterAgentWithWs) {
  const auto agentName = tryMatchStartGroupLine(" user-agent\t: a");
  ASSERT_TRUE(agentName);
  EXPECT_STREQ(std::string{*agentName}.c_str(), "a");
}

TEST(tryMatchStartGroupLine, multiLetterAgent) {
  const auto agentName = tryMatchStartGroupLine(" user-agent\t: googlebot");
  ASSERT_TRUE(agentName);
  EXPECT_STREQ(std::string{*agentName}.c_str(), "googlebot");
}

TEST(tryMatchStartGroupLine, starAgent) {
  const auto agentName = tryMatchStartGroupLine(" user-agent\t:*");
  ASSERT_TRUE(agentName);
  EXPECT_STREQ(std::string{*agentName}.c_str(), "*");
}

TEST(tryMatchStartGroupLine, agentWithStarAndComment) {
  const auto agentName = tryMatchStartGroupLine(" user-agent:googlebot* #commment");
  ASSERT_TRUE(agentName);
  EXPECT_STREQ(std::string{*agentName}.c_str(), "googlebot");
}

TEST(matchUserAgent, noMatchLongerGroupName) {
  const std::string agentName { "google" };
  const std::string currentGroupMatch {};
  const std::string groupName { "googlebot" };
  EXPECT_EQ(AgentNameMatch::None, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, matchLongerAgentName) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch {};
  const std::string groupName { "google" };
  EXPECT_EQ(AgentNameMatch::Better, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, matchSameAgentName) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch {};
  const std::string groupName { "google" };
  EXPECT_EQ(AgentNameMatch::Better, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, emptyAgentNameThrow) {
  const std::string currentGroupMatch {};
  const std::string groupName { "*" };
  EXPECT_THROW([[maybe_unused]] auto res = matchUserAgent( "", currentGroupMatch, groupName), std::logic_error );
}

TEST(matchUserAgent, nullAgentNameThrow) {
  const std::string currentGroupMatch {};
  const std::string groupName { "*" };
  EXPECT_THROW([[maybe_unused]] auto res = matchUserAgent( {}, currentGroupMatch, groupName), std::logic_error );
}

TEST(matchUserAgent, nullGroupNameThrow) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch {};
  EXPECT_THROW([[maybe_unused]] auto res = matchUserAgent( agentName, currentGroupMatch, {}), std::logic_error );
}

TEST(matchUserAgent, emptyGroupNameThrow) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch {};
  EXPECT_THROW([[maybe_unused]] auto res = matchUserAgent( agentName, currentGroupMatch, ""), std::logic_error );
}

TEST(matchUserAgent, matchStar) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch {};
  const std::string groupName { "*" };
  EXPECT_EQ(AgentNameMatch::Better, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, matchOneCharWhenPrevStar) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch { "*" };
  const std::string groupName { "g" };
  EXPECT_EQ(AgentNameMatch::Better, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, matchMoreCharsWhenPrevStar) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch { "*" };
  const std::string groupName { "go" };
  EXPECT_EQ(AgentNameMatch::Better, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, matchMoreChars) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch { "google" };
  const std::string groupName { "googleb" };
  EXPECT_EQ(AgentNameMatch::Better, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, matchMoreExact) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch { "google" };
  const std::string groupName { "googlebot" };
  EXPECT_EQ(AgentNameMatch::Better, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, matchSameExact) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch { "googlebot" };
  const std::string groupName { "googlebot" };
  EXPECT_EQ(AgentNameMatch::Same, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, matchSame) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch { "google" };
  const std::string groupName { "google" };
  EXPECT_EQ(AgentNameMatch::Same, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, dontMatchStar) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch { "g" };
  const std::string groupName { "*" };
  EXPECT_EQ(AgentNameMatch::None, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, dontMatchShorter) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch { "googleb" };
  const std::string groupName { "google" };
  EXPECT_EQ(AgentNameMatch::None, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(matchUserAgent, dontMatchLongerThanAgentName) {
  const std::string agentName { "googlebot" };
  const std::string currentGroupMatch { "googlebot" };
  const std::string groupName { "googlebot-images" };
  EXPECT_EQ(AgentNameMatch::None, matchUserAgent(agentName, currentGroupMatch, groupName));
}

TEST(tryParseRule, allowAll) {
  const auto resultRule = tryParseRule(" allow\t:/");
  ASSERT_TRUE(resultRule);
  EXPECT_EQ(resultRule->type(), RuleType::Allow);
  EXPECT_STREQ(resultRule->path().c_str(), "/");
}

TEST(tryParseRule, disallowAll) {
  const auto resultRule = tryParseRule(" disallow\t:/");
  ASSERT_TRUE(resultRule);
  EXPECT_EQ(resultRule->type(), RuleType::Disallow);
  EXPECT_STREQ(resultRule->path().c_str(), "/");
}

TEST(tryParseRule, disallowStar) {
  const auto resultRule = tryParseRule(" disallow\t:/*");
  ASSERT_TRUE(resultRule);
  EXPECT_EQ(resultRule->type(), RuleType::Disallow);
  EXPECT_STREQ(resultRule->path().c_str(), "/*");
}

TEST(tryParseRule, allowSimple) {
  const auto resultRule = tryParseRule("allow: /xas");
  ASSERT_TRUE(resultRule);
  EXPECT_EQ(resultRule->type(), RuleType::Allow);
  EXPECT_STREQ(resultRule->path().c_str(), "/xas");
}

TEST(tryParseRule, allowWithComment) {
  const auto resultRule = tryParseRule("allow: /xas# DTES");
  ASSERT_TRUE(resultRule);
  EXPECT_EQ(resultRule->type(), RuleType::Allow);
  EXPECT_STREQ(resultRule->path().c_str(), "/xas");
}

TEST(tryParseRule, disallowWithSpaceAtEnd) {
  const auto resultRule = tryParseRule("disallow: /xas DTES");
  ASSERT_TRUE(resultRule);
  EXPECT_EQ(resultRule->type(), RuleType::Disallow);
  EXPECT_STREQ(resultRule->path().c_str(), "/xas");
}

TEST(tryParseRule, allowWithTabAtEnd) {
  const auto resultRule = tryParseRule("allow: /xas\tDTES");
  ASSERT_TRUE(resultRule);
  EXPECT_EQ(resultRule->type(), RuleType::Allow);
  EXPECT_STREQ(resultRule->path().c_str(), "/xas");
}

TEST(tryParseRule, disallowSimple) {
  const auto resultRule = tryParseRule("disallow: /s");
  ASSERT_TRUE(resultRule);
  EXPECT_EQ(resultRule->type(), RuleType::Disallow);
  EXPECT_STREQ(resultRule->path().c_str(), "/s");
}

TEST(tryParseRule, disallowEmpty) {
  const auto resultRule = tryParseRule(" disallow\t: #DJSLKA ");
  ASSERT_FALSE(resultRule);
}

TEST(tryParseRule, allowEmpty) {
  const auto resultRule = tryParseRule(" allow\t: DJSLKA ");
  ASSERT_FALSE(resultRule);
}

TEST(SpecificToGeneralComparer, ruleLength) {
  Rule r1{ RuleType::Allow, "a"};
  Rule r2{ RuleType::Allow, "abc"};
  SpecificToGeneralComparer comparer;
  EXPECT_TRUE(comparer(r2, r1));
  EXPECT_FALSE(comparer(r1, r2));
}

TEST(SpecificToGeneralComparer, equalLen) {
  Rule r1{ RuleType::Allow, "ac"};
  Rule r2{ RuleType::Allow, "ab"};
  SpecificToGeneralComparer comparer;
  EXPECT_FALSE(comparer(r2, r1));
  EXPECT_FALSE(comparer(r1, r2));
}

TEST(SpecificToGeneralComparer, equalLenWithWildcard) {
  Rule r1{ RuleType::Allow, "*a**c"};
  Rule r2{ RuleType::Allow, "ab*"};
  SpecificToGeneralComparer comparer;
  EXPECT_FALSE(comparer(r2, r1));
  EXPECT_FALSE(comparer(r1, r2));
}

TEST(SpecificToGeneralComparer, wildcardNotCounted) {
  Rule r1{ RuleType::Allow, "ab"};
  Rule r2{ RuleType::Allow, "*a"};
  SpecificToGeneralComparer comparer;
  EXPECT_FALSE(comparer(r2, r1));
  EXPECT_TRUE(comparer(r1, r2));
}

TEST(SpecificToGeneralComparer, wildcardAtEndIgnored) {
  Rule r1{ RuleType::Allow, "ab"};
  Rule r2{ RuleType::Allow, "*a**"};
  SpecificToGeneralComparer comparer;
  EXPECT_FALSE(comparer(r2, r1));
  EXPECT_TRUE(comparer(r1, r2));
}

TEST(SpecificToGeneralComparer, onlyWildcardAtEnd) {
  Rule r1{ RuleType::Allow, "ab*"};
  Rule r2{ RuleType::Allow, "**"};
  SpecificToGeneralComparer comparer;
  EXPECT_FALSE(comparer(r2, r1));
  EXPECT_TRUE(comparer(r1, r2));
}

TEST(SpecificToGeneralComparer, dollarWithWildcardAtEnd) {
  Rule r1{ RuleType::Allow, "ab*$"};
  Rule r2{ RuleType::Allow, "ab"};
  SpecificToGeneralComparer comparer;
  EXPECT_FALSE(comparer(r2, r1));
  EXPECT_FALSE(comparer(r1, r2));
}

TEST(SpecificToGeneralComparer, dollarAtEnd) {
  Rule r1{ RuleType::Allow, "$"};
  Rule r2{ RuleType::Allow, ""};
  SpecificToGeneralComparer comparer;
  EXPECT_FALSE(comparer(r2, r1));
  EXPECT_FALSE(comparer(r1, r2));
}

TEST(Rule, noSpecials) {
  Rule r1{ RuleType::Disallow, "no/specials"};
  EXPECT_STREQ("no/specials", r1.path().c_str());
  EXPECT_EQ(RuleType::Disallow, r1.type());
  EXPECT_FALSE(r1.hasEol());
  EXPECT_EQ(-1, r1.wildCardPos());
}

TEST(Rule, ignoredWildcard) {
  Rule r1{ RuleType::Disallow, "no/specials*"};
  EXPECT_STREQ("no/specials", r1.path().c_str());
  EXPECT_EQ(RuleType::Disallow, r1.type());
  EXPECT_FALSE(r1.hasEol());
  EXPECT_EQ(-1, r1.wildCardPos());
}

TEST(Rule, ignoredEolWildcard) {
  Rule r1{ RuleType::Disallow, "no/specials*$"};
  EXPECT_STREQ("no/specials", r1.path().c_str());
  EXPECT_EQ(RuleType::Disallow, r1.type());
  EXPECT_FALSE(r1.hasEol());
  EXPECT_EQ(-1, r1.wildCardPos());
}

TEST(Rule, ignoredMultiWildcards) {
  Rule r1{ RuleType::Disallow, "no/specials*****$"};
  EXPECT_STREQ("no/specials", r1.path().c_str());
  EXPECT_EQ(RuleType::Disallow, r1.type());
  EXPECT_FALSE(r1.hasEol());
  EXPECT_EQ(-1, r1.wildCardPos());
}

TEST(Rule, wildcard) {
  Rule r1{ RuleType::Allow, "*.php"};
  EXPECT_STREQ(".php", r1.path().c_str());
  EXPECT_EQ(RuleType::Allow, r1.type());
  EXPECT_FALSE(r1.hasEol());
  EXPECT_EQ(0, r1.wildCardPos());
}

TEST(Rule, pathWithEol) {
  Rule r1{ RuleType::Allow, "x.php$"};
  EXPECT_STREQ("x.php", r1.path().c_str());
  EXPECT_EQ(RuleType::Allow, r1.type());
  EXPECT_TRUE(r1.hasEol());
  EXPECT_EQ(-1, r1.wildCardPos());
}

TEST(Rule, pathWithEolAndWildcard) {
  Rule r1{ RuleType::Allow, "sss/*.php$"};
  EXPECT_STREQ("sss/.php", r1.path().c_str());
  EXPECT_EQ(RuleType::Allow, r1.type());
  EXPECT_TRUE(r1.hasEol());
  EXPECT_EQ(4, r1.wildCardPos());
}

TEST(RuleMatcher, exactSimpleMatch) {
  Rule rule { RuleType::Allow, "/fish" };
  RuleMatcher matcher { "/fish" };
  EXPECT_TRUE(matcher(rule));
}

TEST(RuleMatcher, simpleMismatch) {
  Rule rule { RuleType::Allow, "/ish" };
  RuleMatcher matcher { "fish" };
  EXPECT_FALSE(matcher(rule));
}

TEST(RuleMatcher, mismatchEol) {
  Rule rule { RuleType::Allow, "/fis$" };
  RuleMatcher matcher { "/fish" };
  EXPECT_FALSE(matcher(rule));
}

TEST(RuleMatcher, matchEol) {
  Rule rule { RuleType::Allow, "/fish$" };
  RuleMatcher matcher { "/fish" };
  EXPECT_TRUE(matcher(rule));
}

TEST(RuleMatcher, matchWildcard) {
  Rule rule { RuleType::Disallow, "/*.php" };
  RuleMatcher matcher { "/fish.php" };
  EXPECT_TRUE(matcher(rule));
}

TEST(RuleMatcher, matchWildcardWithEol) {
  Rule rule { RuleType::Disallow, "/*.php$" };
  RuleMatcher matcher { "/fish.php" };
  EXPECT_TRUE(matcher(rule));
}

TEST(RuleMatcher, mismatchWildcardWithEol) {
  Rule rule { RuleType::Disallow, "/*.php$" };
  RuleMatcher matcher { "/fish.php." };
  EXPECT_FALSE(matcher(rule));
}

TEST(RuleMatcher, matchWildcardTrailingEol) {
  Rule rule { RuleType::Disallow, "/*.php" };
  RuleMatcher matcher { "/fish.php." };
  EXPECT_TRUE(matcher(rule));
}

TEST(RuleMatcher, matchWildcardZeroMatches) {
  EXPECT_TRUE(RuleMatcher{"/fish.php"}(Rule{RuleType::Allow, "/fish*.php"}));
}

TEST(RuleMatcher, matchWildcardTrailingEol2) {
  Rule rule { RuleType::Disallow, "/abd*.php" };
  RuleMatcher matcher { "/abd_fish.php/jkl" };
  EXPECT_TRUE(matcher(rule));
}

TEST(RuleMatcher, matchGoogleSpecsExamples) {
  //"/fish"
  //Match:
  EXPECT_TRUE(RuleMatcher{"/fish"                }(Rule{ RuleType::Allow, "/fish" }));
  EXPECT_TRUE(RuleMatcher{"/fish.html"           }(Rule{ RuleType::Allow, "/fish" }));
  EXPECT_TRUE(RuleMatcher{"/fish/salmon.html"    }(Rule{ RuleType::Allow, "/fish" }));
  EXPECT_TRUE(RuleMatcher{"/fishheads"           }(Rule{ RuleType::Allow, "/fish" }));
  EXPECT_TRUE(RuleMatcher{"/fishheads/yummy.html"}(Rule{ RuleType::Allow, "/fish" }));
  EXPECT_TRUE(RuleMatcher{"/fish.php?id=anything"}(Rule{ RuleType::Allow, "/fish" }));
  //Mismatch
  EXPECT_FALSE(RuleMatcher{"/Fish.asp"           }(Rule{ RuleType::Allow, "/fish" }));
  EXPECT_FALSE(RuleMatcher{"/catfish"            }(Rule{ RuleType::Allow, "/fish" }));
  EXPECT_FALSE(RuleMatcher{"/?id=fish"           }(Rule{ RuleType::Allow, "/fish" }));

  //"/fish*"
  //Match:
  EXPECT_TRUE(RuleMatcher{"/fish"                }(Rule{ RuleType::Allow, "/fish*" }));
  EXPECT_TRUE(RuleMatcher{"/fish.html"           }(Rule{ RuleType::Allow, "/fish*" }));
  EXPECT_TRUE(RuleMatcher{"/fish/salmon.html"    }(Rule{ RuleType::Allow, "/fish*" }));
  EXPECT_TRUE(RuleMatcher{"/fishheads"           }(Rule{ RuleType::Allow, "/fish*" }));
  EXPECT_TRUE(RuleMatcher{"/fishheads/yummy.html"}(Rule{ RuleType::Allow, "/fish*" }));
  EXPECT_TRUE(RuleMatcher{"/fish.php?id=anything"}(Rule{ RuleType::Allow, "/fish*" }));
  //Mismatch
  EXPECT_FALSE(RuleMatcher{"/Fish.asp"           }(Rule{ RuleType::Allow, "/fish*" }));
  EXPECT_FALSE(RuleMatcher{"/catfish"            }(Rule{ RuleType::Allow, "/fish*" }));
  EXPECT_FALSE(RuleMatcher{"/?id=fish"           }(Rule{ RuleType::Allow, "/fish*" }));

  //"/fish/"
  //Match:
  EXPECT_TRUE(RuleMatcher{"/fish/"               }(Rule{ RuleType::Allow, "/fish/" }));
  EXPECT_TRUE(RuleMatcher{"/fish/?id=anything"   }(Rule{ RuleType::Allow, "/fish/" }));
  EXPECT_TRUE(RuleMatcher{"/fish/salmon.html"    }(Rule{ RuleType::Allow, "/fish/" }));
  //Mismatch
  EXPECT_FALSE(RuleMatcher{"/fish"               }(Rule{ RuleType::Allow, "/fish/" }));
  EXPECT_FALSE(RuleMatcher{"/fish.html"          }(Rule{ RuleType::Allow, "/fish/" }));
  EXPECT_FALSE(RuleMatcher{"/Fish.asp"           }(Rule{ RuleType::Allow, "/fish/" }));
  EXPECT_FALSE(RuleMatcher{"/Fish/Salmon.asp"    }(Rule{ RuleType::Allow, "/fish/" }));

  // "/*.php"
  //Matches:
  EXPECT_TRUE(RuleMatcher{"/filename.php"                  }(Rule{RuleType::Allow, "/*.php"}));
  EXPECT_TRUE(RuleMatcher{"/folder/filename.php"           }(Rule{RuleType::Allow, "/*.php"}));
  EXPECT_TRUE(RuleMatcher{"/folder/filename.php?parameters"}(Rule{RuleType::Allow, "/*.php"}));
  EXPECT_TRUE(RuleMatcher{"/folder/any.php.file.html"      }(Rule{RuleType::Allow, "/*.php"}));
  EXPECT_TRUE(RuleMatcher{"/filename.php/"                 }(Rule{RuleType::Allow, "/*.php"}));

  //Mismatch
  EXPECT_FALSE(RuleMatcher{"/"           }(Rule{RuleType::Allow, "/*.php"}));
  EXPECT_FALSE(RuleMatcher{"/windows.PHP"}(Rule{RuleType::Allow, "/*.php"}));


  // "/*.php$"
  //Matches:
  EXPECT_TRUE(RuleMatcher{"/filename.php"       }(Rule{RuleType::Allow, "/*.php$"}));
  EXPECT_TRUE(RuleMatcher{"/folder/filename.php"}(Rule{RuleType::Allow, "/*.php$"}));
  //Mismatch
  EXPECT_FALSE(RuleMatcher{"/filename.php?parameters"}(Rule{RuleType::Allow, "/*.php$"}));
  EXPECT_FALSE(RuleMatcher{"/filename.php/"          }(Rule{RuleType::Allow, "/*.php$"}));
  EXPECT_FALSE(RuleMatcher{"/filename.php5"          }(Rule{RuleType::Allow, "/*.php$"}));
  EXPECT_FALSE(RuleMatcher{"/windows.PHP"            }(Rule{RuleType::Allow, "/*.php$"}));

  // "/fish*.php"
  //Matches:
  EXPECT_TRUE(RuleMatcher{"/fish.php"                        }(Rule{RuleType::Allow, "/fish*.php"}));
  EXPECT_TRUE(RuleMatcher{"/fishheads/catfish.php?parameters"}(Rule{RuleType::Allow, "/fish*.php"}));
  //Mismatch
  EXPECT_FALSE(RuleMatcher{"/Fish.PHP"}(Rule{RuleType::Allow, "/fish*.php"}));
}


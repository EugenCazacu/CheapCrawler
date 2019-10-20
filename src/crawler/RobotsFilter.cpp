#include "RobotsFilter.h"

RobotsFilter::RobotsFilter(const std::string& agentName, std::string_view robotsTxt) {
  auto        robotsTxtPos = begin(robotsTxt);
  bool        matching     = false;
  std::string currentAgentNameMatch;
  do {
    std::string_view::iterator nextLineEnd;
    std::tie(robotsTxtPos, nextLineEnd) = getLine(robotsTxtPos, end(robotsTxt));
    if(end(robotsTxt) == robotsTxtPos) {
      break;
    }
    const std::string_view line(robotsTxtPos, nextLineEnd - robotsTxtPos);
    robotsTxtPos             = nextLineEnd;
    const auto userAgentName = tryMatchStartGroupLine(line);
    if(userAgentName) {
      const auto matchResult = matchUserAgent(agentName, currentAgentNameMatch, *userAgentName);
      if(AgentNameMatch::Better == matchResult) {
        matching = true;
        m_rules.clear();
      }
      else if(AgentNameMatch::Same == matchResult) {
        matching = true;
      }
      else {
        matching = false;
      }
    }
    else if(matching) {
      const auto rule = tryParseRule(line);
      if(rule) {
        m_rules.push_back(*rule);
      }
    }
  } while(true);
  std::sort(begin(m_rules), end(m_rules), SpecificToGeneralComparer{});
}

[[nodiscard]] bool
RobotsFilter::canDownload(std::string_view path) const {
  auto ruleMatch = std::find_if(begin(m_rules), end(m_rules), RuleMatcher{});
  return end(m_rules) == ruleMatch || RuleType::Allow == ruleMatch->type();
}

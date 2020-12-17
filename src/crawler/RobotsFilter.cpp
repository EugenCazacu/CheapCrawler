#include "Logger.h"
LOG_INIT(crawlerRobotsFilter);

#include "RobotsFilter.h"

RobotsFilter::RobotsFilter(const std::string& agentName, std::string_view robotsTxt) {
  LOG_DEBUG("RobotsFilter::ctor agentName: " << agentName);
  LOG_DEBUG("RobotsFilter::ctor robotsTxt: " << robotsTxt);
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
    LOG_DEBUG("ctor line: " << line);
    robotsTxtPos             = nextLineEnd;
    const auto userAgentName = tryMatchStartGroupLine(line);
    if(userAgentName) {
      LOG_DEBUG("ctor group line match for agent: " << *userAgentName);
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
  LOG_DEBUG("RobotsFilter::ctor rules: " << m_rules);
}

[[nodiscard]] bool
RobotsFilter::canDownload(std::string_view path) const {
  LOG_DEBUG("canDownload: path: " << path);
  auto ruleMatch = std::find_if(begin(m_rules), end(m_rules), RuleMatcher{});
  return end(m_rules) == ruleMatch || RuleType::Allow == ruleMatch->type();
}

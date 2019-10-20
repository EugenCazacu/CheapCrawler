#include "Logger.h"
LOG_INIT(crawlerRobotsTxtParser);

#include "RobotsTxtParser.h"
#include "ctre.hpp"

namespace {
  [[nodiscard]] constexpr bool
  isStar(std::string_view agentName) noexcept {
    return agentName == "*";
  }

  /**
   * Tell if the given path has an end of line match and if so removes it from io_param
   * @returns true if path has valid eol
   */
  bool hasValidEolAndTrim(std::string& io_param) {
    if(!std::empty(io_param) && '$' == io_param.back()) {
      io_param.pop_back();
      return std::empty(io_param) || io_param.back() != '*';
    }
    else {
      return false;
    }
  }

  /**
   * Remove all wildcards from io_param
   * @returns the position of the first non-trailing wildcard or -1
   */
  int removeWidcards(std::string& io_param) {
    while( !empty(io_param) && '*' == io_param.back()) {
      io_param.pop_back();
    }
    const auto wildcardPos = std::find(begin(io_param), end(io_param), '*');
    if(end(io_param) == wildcardPos) {
      return -1;
    }
    else {
      int result = wildcardPos - begin(io_param);
      io_param.erase(std::remove(wildcardPos, end(io_param), '*'), end(io_param));
      return result;
    }
  }
} //namespace <anonymous>

[[nodiscard]] std::tuple<std::string_view::iterator, std::string_view::iterator>
getLine(std::string_view::const_iterator begin, std::string_view::iterator end) noexcept {
  while(begin != end && (*begin == '\n' || *begin == '\r')) {
    ++begin;
  }
  auto resultEnd = begin;
  while(resultEnd != end && *resultEnd != '\n' && *resultEnd != '\r') {
    ++resultEnd;
  }
  return std::make_tuple(begin, resultEnd);
}

[[nodiscard]] std::optional<std::string_view>
tryMatchStartGroupLine(std::string_view line) noexcept {
  static constexpr auto pattern = ctll::fixed_string{ "[ \\t]*user-agent[ \\t]*:[ \\t]*(\\*|[a-zA-Z\\-_]+).*" };
  if(auto match = ctre::match<pattern>(line)) {
    return match.get<1>().to_view();
  }
  return {};
}

[[nodiscard]] AgentNameMatch
matchUserAgent(std::string_view agentName, std::string_view currentGroupMatch, std::string_view groupName) {
  if(agentName.empty()) {
    throw std::logic_error("RobotsTxtParser: agentName is empty");
  }
  if(groupName.empty()) {
    throw std::logic_error("RobotsTxtParser: groupName is empty");
  }

  if(groupName.size() > agentName.size()) {
    return AgentNameMatch::None;
  }

  if(isStar(groupName)) {
    if(isStar(currentGroupMatch)) {
      return AgentNameMatch::Same;
    }
    else if(currentGroupMatch.empty()) {
      return AgentNameMatch::Better;
    }
    else {
      return AgentNameMatch::None;
    }
  }

  const auto endMatch = std::mismatch(begin(groupName), end(groupName), begin(agentName));
  if(endMatch.first != end(groupName)) {
    return AgentNameMatch::None;
  }

  const auto newMatchSize = groupName.size();

  if(currentGroupMatch.size() < newMatchSize) {
    return AgentNameMatch::Better;
  }
  else if(currentGroupMatch.size() == newMatchSize) {
    if(isStar(currentGroupMatch)) {
      return AgentNameMatch::Better;
    }
    else {
      return AgentNameMatch::Same;
    }
  }
  else {
    return AgentNameMatch::None;
  }
}

Rule::Rule(RuleType itype, std::string ipath)
  : m_type { itype }
  , m_path { std::move(ipath) }
  , m_hasEol { hasValidEolAndTrim(m_path) }
  , m_wildCardPos { removeWidcards(m_path) }
{
}

std::ostream& operator<<(std::ostream& out, const RuleType& rule) {
  if(RuleType::Allow == rule) {
    out << "Allow";
  }
  else {
    out << "Disallow";
  }
  return out;
}

std::ostream& operator<<(std::ostream& out, const Rule& rule) {
  out << "type:" << rule.type()
    << " path: " << rule.path()
    << " hasEol: " << rule.hasEol()
    << " wildCardPos: " << rule.wildCardPos();
  return out;
}

[[nodiscard]] std::optional<Rule>
tryParseRule(std::string_view line) noexcept {
  static constexpr auto pattern = ctll::fixed_string{ "[ \\t]*(allow|disallow)[ \\t]*:[ \\t]*(/[^# \\t]*).*" };
  if(auto match = ctre::match<pattern>(line)) {
    const RuleType ruleType = (match.get<1>().to_view()[0] == 'a') ? RuleType::Allow : RuleType::Disallow;
    return Rule{ ruleType, match.get<2>().to_string() };
  }
  return {};
}

[[nodiscard]] bool
SpecificToGeneralComparer::operator()(const Rule& r1, const Rule& r2) {
  return r1.path().length() > r2.path().length();
}

bool RuleMatcher::operator()(const Rule& rule) {
  LOG_DEBUG("RuleMatcher: path:" << path << " rule: " << rule);
  if(rule.path().length() > path.length()) {
    LOG_DEBUG("RuleMatche: rule pattern longer than path");
    return false;
  }
  if(rule.wildCardPos() == -1) {
    LOG_DEBUG("RuleMatche: no wildcard");
    const auto result = std::mismatch(begin(rule.path()), end(rule.path()), begin(path));
    return result.first == end(rule.path()) && (!rule.hasEol() || result.second==end(path));
  }
  else {
    LOG_DEBUG("RuleMatche: wildcard");
    assert(rule.path().length() > rule.wildCardPos());
    auto wildCardPos = begin(rule.path()) + rule.wildCardPos();
    const auto resultForward = std::mismatch(begin(rule.path()), wildCardPos, begin(path));
    if(resultForward.first != wildCardPos) {
      LOG_DEBUG("RuleMatche: forward pass failed");
      return false;
    }
    else {
      const int remainingPathSize = path.length() - rule.wildCardPos();
      const int remainingPatternSize = rule.path().length() - rule.wildCardPos();
      const int nrAttempts = rule.hasEol() ? 1 : remainingPathSize - remainingPatternSize + 1;
      LOG_DEBUG("RuleMatche: backward possible attempts: " << nrAttempts);
      auto attemptStartPos = rbegin(path);
      for(int attempt = 0; attempt < nrAttempts; ++attempt) {
        auto rWildCardPos = std::make_reverse_iterator(wildCardPos);
        const auto resultBackwards = std::mismatch(rbegin(rule.path()), rWildCardPos, attemptStartPos);
        if(resultBackwards.first == rWildCardPos) {
          return true;
        }
        ++attemptStartPos;
      }
      LOG_DEBUG("RuleMatche: all backwards passes failed");
      return false;
    }
  }
}


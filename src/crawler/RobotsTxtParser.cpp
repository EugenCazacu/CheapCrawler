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
  if(rule.path().length() > path.length()) {
    return false;
  }
  const auto result = std::mismatch(begin(rule.path()), end(rule.path()), begin(path));
  return result.first == end(rule.path()) && (!rule.hasEol() || result.second==end(path));
}


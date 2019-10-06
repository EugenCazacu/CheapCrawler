#include "RobotsTxtParser.h"
#include "ctre.hpp"

namespace {
  [[nodiscard]] constexpr bool
  isStar(std::string_view agentName) noexcept {
    return agentName == "*";
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
  return r1.path.length() > r2.path.length();
}


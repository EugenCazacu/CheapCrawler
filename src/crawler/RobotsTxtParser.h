#ifndef CRAWLER_ROBOTSTXTPARSER_H_17QWCG2W
#define CRAWLER_ROBOTSTXTPARSER_H_17QWCG2W

#include <tuple>
#include <string_view>
#include <string>
#include <optional>

/**
 * Returns the begin iterator and the paste the end iterator of a new line
 * Result not contain \n or \r
 * When end of line is reached both iterator point to end
 */
[[nodiscard]] std::tuple<std::string_view::iterator, std::string_view::iterator>
getLine(std::string_view::const_iterator begin, std::string_view::iterator end) noexcept;

/**
 * Tries to match the input line
 * @returns
 *   bool        line is a startGroupLine
 *   string_view user-agent name
 */
[[nodiscard]] std::optional<std::string_view>
tryMatchStartGroupLine(std::string_view line) noexcept;

/**
 * Tries to match a new agent name
 */
enum class AgentNameMatch { None, Same, Better };
[[nodiscard]] AgentNameMatch
matchUserAgent(std::string_view agentName, std::string_view currentGroupMatch, std::string_view groupName);

enum class RuleType { Allow, Disallow };
struct Rule {
  RuleType type;
  std::string path;
};

struct SpecificToGeneralComparer {
  bool operator()(const Rule& r1, const Rule& r2);
};

struct RuleMatcher {
  std::string_view path;
  bool operator()(const Rule& rule) {
    return true;
  }
};

/**
 * Tries to parse the input line as a rule
 * @returns
 *   bool        if the line is a valid rule
 *   string_view rule
 */
[[nodiscard]] std::optional<Rule>
tryParseRule(std::string_view line) noexcept;

#endif /* end of include guard: CRAWLER_ROBOTSTXTPARSER_H_17QWCG2W */

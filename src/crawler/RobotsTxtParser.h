#ifndef CRAWLER_ROBOTSTXTPARSER_H_17QWCG2W
#define CRAWLER_ROBOTSTXTPARSER_H_17QWCG2W

#include <tuple>
#include <string_view>
#include <string>
#include <optional>
#include <iosfwd>

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
class Rule {
  public:
  Rule(RuleType type, std::string path);

  [[nodiscard]] RuleType
  type() const noexcept {
    return m_type;
  }

  [[nodiscard]] const std::string&
  path() const noexcept {
    return m_path;
  }

  [[nodiscard]] bool
  hasEol() const noexcept {
    return m_hasEol;
  }

  /**
   * Returns the position of the wildcard in the rule if exists of -1 otherwise
   */
  [[nodiscard]] int
  wildCardPos() const noexcept {
    return m_wildCardPos;
  }

  private:
    RuleType m_type;
    std::string m_path;
    bool m_hasEol;
    int m_wildCardPos;
};

std::ostream& operator<<(std::ostream& out, const RuleType& rule);
std::ostream& operator<<(std::ostream& out, const Rule& rule);

struct SpecificToGeneralComparer {
  bool operator()(const Rule& r1, const Rule& r2);
};

struct RuleMatcher {
  std::string_view path;
  bool operator()(const Rule& rule);
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

#ifndef CRAWLER_ROBOTSFILTER_H_XLNO14JC
#define CRAWLER_ROBOTSFILTER_H_XLNO14JC

#include "RobotsTxtParser.h"
#include "Url.h"
#include <vector>

class RobotsFilter {
public:
  RobotsFilter(const std::string& agentName, std::string_view robotsTxt);

  [[nodiscard]] bool canDownload(std::string_view path) const;

private:
  std::vector<Rule> m_rules;
};

#endif /* end of include guard: CRAWLER_ROBOTSFILTER_H_XLNO14JC */

#ifndef CRAWLER_ROBOTSFILTER_H_XLNO14JC
#define CRAWLER_ROBOTSFILTER_H_XLNO14JC

#include "Url.h"

struct RobotsFilter {
  constexpr RobotsFilter(std::string_view robotsTxt) {
  }
  [[nodiscard]] bool
  canDownload(std::string_view url) const;
};

#endif /* end of include guard: CRAWLER_ROBOTSFILTER_H_XLNO14JC */

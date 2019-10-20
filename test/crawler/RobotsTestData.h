#ifndef CRAWLER_ROBOTSTESTDATA_H_CSANOHEM
#define CRAWLER_ROBOTSTESTDATA_H_CSANOHEM

constexpr char RobotsTxt_Empty[] = "";

constexpr char RobotsTxt_AllUrlsAllowed[]   = R"( User-agent: *
Disallow:
)";
constexpr char RobotsTxt_AllUrlsForbidden[] = R"( User-agent: *
Disallow: /
)";

#endif /* end of include guard: CRAWLER_ROBOTSTESTDATA_H_CSANOHEM */

#ifndef UTILS_GETLINE_H_UY4TDQA0
#define UTILS_GETLINE_H_UY4TDQA0

#include <tuple>
#include <string_view>

/**
 * Returns the begin iterator and the paste the end iterator of a new line
 * Result not contain \n or \r
 * When end of line is reached both iterator point to end
 */
[[nodiscard]] std::tuple<std::string_view::iterator, std::string_view::iterator>
getLine(std::string_view::const_iterator begin, std::string_view::iterator end);

#endif /* end of include guard: UTILS_GETLINE_H_UY4TDQA0 */

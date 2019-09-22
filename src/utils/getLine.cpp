#include "getLine.h"

[[nodiscard]] std::tuple<std::string_view::iterator, std::string_view::iterator>
getLine(std::string_view::const_iterator begin, std::string_view::iterator end) {
  while(begin != end && (*begin == '\n' || *begin == '\r')) {
    ++begin;
  }
  auto resultEnd = begin;
  while(resultEnd != end && *resultEnd != '\n' && *resultEnd != '\r') {
    ++resultEnd;
  }
  return std::make_tuple(begin, resultEnd);
}


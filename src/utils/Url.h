#ifndef CRAWLER_URL_H_X64B8M1L
#define CRAWLER_URL_H_X64B8M1L

#include <functional>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

struct DownloadResult;

using Url = std::tuple<std::string, int>;

struct DownloadElem {
  Url                                   url;
  std::function<void(DownloadResult&&)> callback;
};

using DownloadQueue = std::vector<DownloadElem>;

inline std::ostream&
operator<<(std::ostream& out, const DownloadElem& dwElem) {
  out << '(' << std::get<0>(dwElem.url) << ',' << std::get<1>(dwElem.url) << ')';
  return out;
}

#endif /* end of include guard: CRAWLER_URL_H_X64B8M1L */

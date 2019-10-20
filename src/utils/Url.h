#ifndef CRAWLER_URL_H_X64B8M1L
#define CRAWLER_URL_H_X64B8M1L

#include <functional>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

struct DownloadResult;

struct Url {
  std::string url;
  int         id;
};

struct DownloadElem {
  Url                                   url;
  std::function<void(DownloadResult&&)> callback;
};

inline std::ostream&
operator<<(std::ostream& out, const Url& url) {
  out << '(' << url.url << ',' << url.id << ')';
  return out;
}

inline std::ostream&
operator<<(std::ostream& out, const DownloadElem& dwElem) {
  out << dwElem.url;
  return out;
}

inline bool
operator==(const Url& url1, const Url& url2) {
  return url1.url == url2.url && url1.id == url2.id;
}

#endif /* end of include guard: CRAWLER_URL_H_X64B8M1L */

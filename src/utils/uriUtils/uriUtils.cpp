#include "Logger.h"
LOG_INIT(addUrlsToDbIsValid);

#include "uriUtils/uriUtils.h"

#include <regex>

namespace {
inline bool
hasValidScheme(const UriUriA& uri) {
  static const std::regex schemaExpr("https?", std::regex_constants::ECMAScript | std::regex_constants::icase);
  return std::regex_match(uri.scheme.first, uri.scheme.afterLast, schemaExpr);
}

inline bool
hasNoFragment(const UriUriA& uri) {
  return uri.fragment.first == uri.fragment.afterLast;
}

inline bool
hasNoUserInfo(const UriUriA& uri) {
  return uri.userInfo.first == uri.userInfo.afterLast;
}

inline bool
hostIsNotIp(const UriUriA& uri) {
  const auto hostData = &uri.hostData;
  // clang-format off
  return nullptr == hostData->ip4
      && nullptr == hostData->ip6
      && nullptr == hostData->ipFuture.first
      && nullptr == hostData->ipFuture.afterLast
      ;
  // clang-format on
}

inline bool
hasNoPort(const UriUriA& uri) {
  return uri.portText.first == uri.portText.afterLast;
}

} // namespace

std::string
toString(const UriUriA& uri) {
  static const size_t MAX_URL_LENGTH = 8000;

  std::string result(MAX_URL_LENGTH, '\0');

  int callResult = uriToStringA(&result[0], &uri, result.length() + 1, NULL);
  if(URI_SUCCESS != callResult) {
    LOG_ERROR("convestion of url to string failed error:" << callResult);
    return std::string{};
  }
  return result;
}

bool
isValid(const UriUriA& uri) {
  // clang-format off
  LOG_DEBUG("hasValidScheme: " << hasValidScheme(uri)
    << " hasNoFragment: " << hasNoFragment(uri)
    << " hasNoUserInfo: " << hasNoUserInfo(uri)
    << " hostIsNotIp: " << hostIsNotIp(uri)
    << " hasNoPort: " << hasNoPort(uri));
  return  hasValidScheme(uri)
      && hasNoFragment(uri)
      && hasNoUserInfo(uri)
      && hostIsNotIp(uri)
      && hasNoPort(uri)
      ;
  // clang-format on
}

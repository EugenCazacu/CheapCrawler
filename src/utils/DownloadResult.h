#ifndef UTILS_DOWNLOADRESULT_H_DELBSWF8
#define UTILS_DOWNLOADRESULT_H_DELBSWF8

#include "MediaType.h"
#include "Url.h"

#include <iosfwd>

struct DownloadResult {
  Url         url;
  std::string content;
  MediaType   mediaType;
  bool        success;
  std::string errorMessage;
  double      downloadSpeedByteSec;

  DownloadResult()                 = default;
  DownloadResult(DownloadResult&&) = default;
  DownloadResult& operator=(DownloadResult&&) = default;

  // Copying all the content could be expensive.
  // This operation is disabled in the name of efficency
  // Using implementations must either ensure move semantics or copy members explicitly.
  DownloadResult(const DownloadResult&) = delete;
  DownloadResult& operator=(const DownloadResult&) = delete;
};

std::ostream& operator<<(std::ostream& out, const DownloadResult& page);

#endif /* end of include guard: UTILS_DOWNLOADRESULT_H_DELBSWF8 */

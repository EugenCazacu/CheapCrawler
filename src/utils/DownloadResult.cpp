#include "DownloadResult.h"
#include "Logger.h"
#include <iostream>

std::ostream&
operator<<(std::ostream& out, const DownloadResult& page) {
  out << "Url: " << page.url << " mediaType: " << page.mediaType << " success: " << page.success
      << " content: " << page.content;
  if(!page.success) {
    out << page.errorMessage;
  }
  return out;
}

#ifndef CurlEasyDownloadManager_h
#define CurlEasyDownloadManager_h

#include <memory>

#include "curlTypes.h"

namespace std {
template<> class default_delete<CURL> {
public:
  void operator()(CURL* ptr) noexcept {
    if(nullptr != ptr) {
      curl_easy_cleanup(ptr);
    }
  }
};
} // namespace std

class CurlEasyDownloadManager {
public:
  /**
   * @param url memory must be accessable while downloading
   */
  CurlEasyDownloadManager(char*                  url,
                          void*                  userdata,
                          HeaderCbType           headerCb,
                          HeaderCbType           writeCb,
                          OpenCloseSocketConfig* openCloseSocketConfig);

  /**
   * @param url memory must be accessable while downloading
   */
  void reuse(char* url, void* userdata, HeaderCbType headerCb, HeaderCbType writeCb);

  CURL* get() { return m_easyHandle.get(); }

  const char* getErrorMessage() const { return m_errorMessage; }

private:
  void setGeneralOptions(OpenCloseSocketConfig*);
  void setUrlSpecific(char* url, void* userdata, HeaderCbType headerCb, HeaderCbType writeCb);

private:
  char                  m_errorMessage[CURL_ERROR_SIZE + 1];
  std::unique_ptr<CURL> m_easyHandle;
};

#endif // CurlEasyDownloadManager_h

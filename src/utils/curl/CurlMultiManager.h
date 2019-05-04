#ifndef CurlMultiManager_h
#define CurlMultiManager_h

#include <curl/multi.h>

class CurlMultiManager {
public:
  CurlMultiManager();
  CurlMultiManager(const CurlMultiManager&) = delete;
  CurlMultiManager(CurlMultiManager&&)      = delete;
  CurlMultiManager& operator=(const CurlMultiManager&) = delete;
  CurlMultiManager& operator=(CurlMultiManager&&) = delete;

  CURLM* const get() const { return m_multi; }

  ~CurlMultiManager();

private:
  CURLM* m_multi;
};

#endif // CurlMultiManager_h

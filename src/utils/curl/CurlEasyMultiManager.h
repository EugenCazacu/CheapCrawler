#ifndef CurlEasyMultiManager_h
#define CurlEasyMultiManager_h

#include <curl/curl.h>

class CurlEasyMultiManager {
public:
  CurlEasyMultiManager(CURLM* multiHandle, CURL* easyHandle);

  CurlEasyMultiManager(const CurlEasyMultiManager&) = delete;
  CurlEasyMultiManager& operator=(const CurlEasyMultiManager&) = delete;

  CurlEasyMultiManager(CurlEasyMultiManager&&) noexcept;
  CurlEasyMultiManager& operator=(CurlEasyMultiManager&&) noexcept;

  ~CurlEasyMultiManager();

  void swap(CurlEasyMultiManager& other) noexcept;

  /**
   * Must call release before reuse of same easy is possible in the multi
   */
  void reuse();
  void release();

private:
  void addHandle();
  void removeHandle() noexcept;

private:
  CURLM* m_multiHandle;
  CURL*  m_easyHandle;
  bool   m_released;
};

inline void
swap(CurlEasyMultiManager& a, CurlEasyMultiManager& b) noexcept {
  a.swap(b);
}

#endif // CurlEasyMultiManager_h

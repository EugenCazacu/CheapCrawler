#ifndef CurlGlobalManager_h
#define CurlGlobalManager_h

class CurlGlobalManager {
public:
  CurlGlobalManager();
  ~CurlGlobalManager();
  CurlGlobalManager(const CurlGlobalManager&)  = delete;
  CurlGlobalManager(const CurlGlobalManager&&) = delete;
  CurlGlobalManager& operator=(const CurlGlobalManager&) = delete;
  CurlGlobalManager& operator=(CurlGlobalManager&&) = delete;
};

#endif // CurlGlobalManager_h

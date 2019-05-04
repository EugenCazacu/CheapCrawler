#include "CurlGlobalManager.h"
#include <curl/curl.h>
#include <stdexcept>

CurlGlobalManager::CurlGlobalManager() {
  auto res = curl_global_init(CURL_GLOBAL_DEFAULT);
  if(0 != res) {
    throw std::runtime_error("curl init error");
  }
}

CurlGlobalManager::~CurlGlobalManager() { curl_global_cleanup(); }

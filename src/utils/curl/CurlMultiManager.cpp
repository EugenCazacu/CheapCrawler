#include "CurlMultiManager.h"
#include <iostream>
#include <stdexcept>

CurlMultiManager::CurlMultiManager() : m_multi(curl_multi_init()) {
  if(nullptr == m_multi) {
    throw std::runtime_error("curl_multi_init return nullptr");
  }
}

CurlMultiManager::~CurlMultiManager() {
  auto retCode = curl_multi_cleanup(m_multi);
  if(CURLM_OK != retCode) {
    std::cerr << "curl_multi_cleanup failed: " << retCode << std::endl;
  }
}

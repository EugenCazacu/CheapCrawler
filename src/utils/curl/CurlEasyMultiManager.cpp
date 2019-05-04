#include "CurlEasyMultiManager.h"

#include <assert.h>
#include <iostream>
#include <stdexcept>

CurlEasyMultiManager::CurlEasyMultiManager(CURLM* multiHandle, CURL* easyHandle)
    : m_multiHandle(multiHandle), m_easyHandle(easyHandle), m_released(false) {
  assert(multiHandle);
  assert(easyHandle);
  addHandle();
}

CurlEasyMultiManager::CurlEasyMultiManager(CurlEasyMultiManager&& other) noexcept
    : m_multiHandle(std::move(other.m_multiHandle))
    , m_easyHandle(std::move(other.m_easyHandle))
    , m_released(std::move(other.m_released)) {
  other.m_released = true;
}

CurlEasyMultiManager&
CurlEasyMultiManager::operator=(CurlEasyMultiManager&& other) noexcept {
  swap(other);
  return *this;
}

CurlEasyMultiManager::~CurlEasyMultiManager() {
  if(!m_released) {
    removeHandle();
  }
}

void
CurlEasyMultiManager::swap(CurlEasyMultiManager& other) noexcept {
  using std::swap;
  swap(m_multiHandle, other.m_multiHandle);
  swap(m_easyHandle, other.m_easyHandle);
  swap(m_released, other.m_released);
}

void
CurlEasyMultiManager::reuse() {
  if(!m_released) {
    throw std::logic_error("[CurlEasyMultiManager::reuse] cand't reused if not released");
  }
  addHandle();
  m_released = false;
}

void
CurlEasyMultiManager::release() {
  if(m_released) {
    throw std::logic_error("[CurlEasyMultiManager::release] already released");
  }
  removeHandle();
  m_released = true;
}

void
CurlEasyMultiManager::removeHandle() noexcept {
  // std::cout << "[CurlEasyMultiManager::removeHandle] m_multiHandle: " << m_multiHandle
  //   << " m_easyHandle: " << m_easyHandle
  //   << std::endl;
  auto retCode = curl_multi_remove_handle(m_multiHandle, m_easyHandle);
  if(CURLM_OK != retCode) {
    std::cerr << "[CurlEasyMultiManager::removeHandle] curl_multi_remove_handle failed: "
              << curl_multi_strerror(retCode) << std::endl;
  }
}

void
CurlEasyMultiManager::addHandle() {
  // std::cout << "[CurlEasyMultiManager::addHandle] m_multiHandle: " << m_multiHandle
  //   << " m_easyHandle: " << m_easyHandle
  //   << std::endl;
  CURLMcode retCode = curl_multi_add_handle(m_multiHandle, m_easyHandle);
  if(CURLM_OK != retCode) {
    throw std::runtime_error(std::string("curl_multi_add_handle failed: ") + curl_multi_strerror(retCode));
  }
}

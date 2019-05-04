#ifndef UTILS_CURL_THROWONERROR_H_VK9ZHXUW
#define UTILS_CURL_THROWONERROR_H_VK9ZHXUW

#include "curl/curl.h"
#include "curl/multi.h"

#include <exception>
#include <string>

inline void
throwOnError(CURLMcode retCode, std::string functionCallName) {
  if(CURLM_OK != retCode) {
    throw std::runtime_error(functionCallName + " failed with error: " + curl_multi_strerror(retCode));
  }
}

inline void
throwOnError(CURLcode retCode, std::string functionCallName) {
  if(CURLE_OK != retCode) {
    throw std::runtime_error(functionCallName + " failed with error: " + curl_easy_strerror(retCode));
  }
}

#endif /* end of include guard: UTILS_CURL_THROWONERROR_H_VK9ZHXUW */

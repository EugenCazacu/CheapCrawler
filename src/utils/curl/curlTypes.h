#ifndef UTILS_CURL_CURLTYPES_H_AMRP81XC
#define UTILS_CURL_CURLTYPES_H_AMRP81XC

#include "curl/curl.h"

using HeaderCbType = size_t (*)(char*, size_t, size_t, void*);

struct OpenCloseSocketConfig {
  using OpenSocketCbType  = curl_socket_t (*)(void*, curlsocktype, struct curl_sockaddr*);
  using CloseSocketCbType = int (*)(void*, curl_socket_t);

  OpenSocketCbType  openSocketCb;
  void*             openSocketData;
  CloseSocketCbType closeSocketCb;
  void*             closeSocketData;
};

#endif /* end of include guard: UTILS_CURL_CURLTYPES_H_AMRP81XC */

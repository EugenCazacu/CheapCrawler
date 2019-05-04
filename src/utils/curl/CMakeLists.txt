add_library(curlutils STATIC
  CurlGlobalManager.cpp
  CurlMultiManager.cpp
  CurlEasyDownloadManager.cpp
  CurlEasyMultiManager.cpp
  HeaderHandler.cpp
  DownloadManager.cpp
)

target_include_directories(curlutils
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(curlutils
  PRIVATE
    CheapCrawlerUtils
    libcurl::libcurl
)



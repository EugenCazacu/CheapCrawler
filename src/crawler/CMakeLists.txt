add_library(crawlerLibrary
  CurlAsioDownloader.cpp
  RobotsLogic.cpp
  crawler.cpp
)

target_include_directories(crawlerLibrary
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(crawlerLibrary
  PUBLIC
    CheapCrawlerUtils
    uriUtilsLibrary
    curlutils
    asio::asio
)

add_executable(crawlerDriver
  main.cpp
)

target_link_libraries(crawlerDriver
  PRIVATE
    crawlerLibrary
    readUrlsFromFile
)

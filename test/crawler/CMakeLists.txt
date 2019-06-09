set(TEST_TARGET_DIR ${PROJECT_SOURCE_DIR}/src/crawler)

add_executable(CrawlerTests
  RobotsFilter.cpp
  RobotsLogic.cpp
  TimeHeap.cpp
  ActionQueue.cpp
  crawler.cpp
)

target_include_directories(CrawlerTests
  PRIVATE
    ${TEST_TARGET_DIR}
)

target_link_libraries(CrawlerTests
  PRIVATE
    gtestMainWithLogging
    crawlerLibrary
)

add_executable(CurlAsioDownloaderTests
  CurlAsioDownloader.cpp
)

target_include_directories(CurlAsioDownloaderTests
  PRIVATE
    ${TEST_TARGET_DIR}
)

target_link_libraries(CurlAsioDownloaderTests
  PRIVATE
    gtestMainWithLogging
    crawlerLibrary
)

add_test_with_properties(NAME CurlAsioDownloaderTests GTEST)
add_test_with_properties(NAME CrawlerTests GTEST)

add_library(CheapCrawlerUtils STATIC
  Logger.cpp
  DownloadResult.cpp
)

target_include_directories(CheapCrawlerUtils
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(CheapCrawlerUtils
  PUBLIC
    Boost::program_options
)

add_library(gtestMainWithLogging STATIC
  gtestMain.cpp
)

target_link_libraries(gtestMainWithLogging
  PUBLIC
    GTest::gtest
    GTest::gmock
  PRIVATE
    CheapCrawlerUtils
)

add_subdirectory(curl)
add_subdirectory(readUrlsFromFile)
add_subdirectory(uriUtils)


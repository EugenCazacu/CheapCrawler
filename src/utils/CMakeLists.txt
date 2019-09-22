add_library(CheapCrawlerUtils STATIC
  Logger.cpp
  DownloadResult.cpp
  getLine.cpp
)

target_include_directories(CheapCrawlerUtils
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(CheapCrawlerUtils
  PUBLIC
    boost_program_options::boost_program_options
)

add_library(gtestMainWithLogging STATIC
  gtestMain.cpp
)

target_link_libraries(gtestMainWithLogging
  PUBLIC
    GTest::GTest
    GMock::GMock
  PRIVATE
    CheapCrawlerUtils
)

add_subdirectory(curl)
add_subdirectory(readUrlsFromFile)
add_subdirectory(uriUtils)


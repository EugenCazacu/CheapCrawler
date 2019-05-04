add_library(readUrlsFromFile STATIC
  readUrlsFromFile.cpp
)

target_include_directories(readUrlsFromFile
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


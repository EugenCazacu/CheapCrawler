add_library(uriUtilsLibrary
  uriUtils.cpp
)

target_include_directories( uriUtilsLibrary
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(uriUtilsLibrary
  PUBLIC
    PkgConfig::URIPARSER
    CheapCrawlerUtils
)


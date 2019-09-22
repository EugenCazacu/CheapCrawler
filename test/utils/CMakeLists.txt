set(TEST_TARGET_DIR ${PROJECT_SOURCE_DIR}/src/utils)

add_executable(getLineUnitTests
  getLine.cpp
)

target_link_libraries(getLineUnitTests
  GTest::GTest
  GTest::Main
  CheapCrawlerUtils
)

add_test_with_properties(NAME getLineUnitTests GTEST)


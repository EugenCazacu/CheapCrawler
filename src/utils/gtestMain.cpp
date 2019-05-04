#include "Logger.h"
LOG_INIT(utilsGtestMain);

#include "handleExceptions.h"
#include "gtest/gtest.h"

namespace {
bool
readCommandLineArgs(const int argc, char** const argv) {
  namespace po = boost::program_options;

  po::options_description optionsDescription("Supported options");
  optionsDescription.add_options()("help,h", "produce help message");

  configureLogging(optionsDescription);

  po::variables_map variablesMap;
  po::store(po::parse_command_line(argc, argv, optionsDescription), variablesMap);
  po::notify(variablesMap);

  if(variablesMap.count("help")) {
    std::cout << optionsDescription << std::endl;
    return false;
  }

  if(!processLogging(variablesMap)) {
    return false;
  }

  return true;
}
} // namespace

int
main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  if(handleExceptionsReturnFuncValue([=]() { return readCommandLineArgs(argc, argv); }, false)) {
    return RUN_ALL_TESTS();
  }
  else {
    return 1;
  }
}

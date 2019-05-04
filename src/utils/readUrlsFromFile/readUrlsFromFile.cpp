#include "readUrlsFromFile.h"
#include <fstream>

std::vector<std::string>
readUrlsFromFile(const std::string& urlsFilename, const size_t maxUrls) {
  std::ifstream urlsFin;
  urlsFin.open(urlsFilename);
  if(!urlsFin.is_open()) {
    throw std::runtime_error("readUrlsFromFile could not open file: " + urlsFilename);
  }

  std::vector<std::string> result;
  std::string              tempString;
  while(result.size() < maxUrls && std::getline(urlsFin, tempString)) {
    result.push_back(tempString);
  }

  return result;
}

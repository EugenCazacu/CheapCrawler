#include "Logger.h"
LOG_INIT(DriverMain);

#include "DownloadResult.h"
#include "MediaType.h"
#include "ProgramLogic.h"
#include "TaskSystem.h"
#include "crawler/CurlAsioDownloader.h"
#include "crawler/crawler.h"
#include "handleExceptions.h"
#include "readUrlsFromFile.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sstream>

namespace {

struct DriverOptions {
  bool        shouldContinue;
  std::string urlsFilename;
  std::string url;
  std::string prefix;
  size_t      maxUrls;
  bool        printUrls;
  size_t      parallelDownloads;
  size_t      maxContentLength;
};

DriverOptions
readCommandLineArgs(const int argc, const char** argv) {
  namespace po = boost::program_options;

  DriverOptions result;

  std::string programDescription =
      R"(This program will download all the received URLs as follows:
   - All URLs from same host will be downloaded sequentially with a timeout of
     2 secs in between downloads.
   - Before downloading a URL from a host the robots.txt is downloaded from that
     host. Respecting robots.txt is not yet implemented.
   - Downloads from different hosts is done in parallel. This program is
     designed to carry as many simultaneous downloads as possible.
   - Each download result is saved in a corresponding file.

  Supported options)";
  po::options_description optionsDescription(programDescription);
  // clang-format off
  optionsDescription.add_options()
    ("urlListFile,f", po::value<std::string>(&result.urlsFilename), "sets the input file which contains the list of URLs to be downloaded")
    ("url,u", po::value<std::string>(&result.url), "download the given URLs")
    ("prefix", po::value<std::string>(&result.prefix)->default_value("_"), "All downloads will be saved in files with this prefix.")
    ("parallelDownloads", po::value<size_t>(&result.parallelDownloads)->default_value(10), "Number of simultaneous downloads.")
    ("maxContentLength", po::value<size_t>(&result.maxContentLength)->default_value(1024*1024), "Maximum allowed length of a downloaded page. Default 1Mb")
    ("maxUrls", po::value<size_t>(&result.maxUrls)->default_value(100), "The maximum number of URLs to be downloaded. Default is 100")
    ("printUrls", po::value<bool>(&result.printUrls)->default_value(false), "print all read urls")
    ("help,h", "produce help message")
    ;
  // clang-format on

  configureLogging(optionsDescription);

  po::variables_map variablesMap;
  po::store(po::parse_command_line(argc, argv, optionsDescription), variablesMap);
  po::notify(variablesMap);

  if(variablesMap.count("help")) {
    std::cout << optionsDescription << std::endl;
    return DriverOptions{false};
  }

  if(!processLogging(variablesMap)) {
    return DriverOptions{false};
  }

  if(!variablesMap.count("urlListFile") && !variablesMap.count("url")) {
    throw std::runtime_error("No urlList defined");
  }
  else {
    result.shouldContinue = true;
  }

  return result;
}

class CrawlOnce {
public:
  bool operator()() {
    const bool hasCrawled = m_hasCrawled;
    m_hasCrawled          = true;
    return !hasCrawled;
  }

private:
  bool m_hasCrawled = false;
};

std::function<bool(const MediaType&)>
getMediaTypeValidator() {
  return [](const MediaType& mediaType) {
    return boost::iequals(mediaType.type, "text") && boost::iequals(mediaType.subtype, "html");
  };
}

class WriteDownloadResultToFile {
public:
  WriteDownloadResultToFile(int nrDownloads, std::string prefix)
      : m_maxNumberOfDigits{static_cast<int>(log10(nrDownloads)) + 1}, m_prefix{std::move(prefix)}, m_seq{0} {}
  void operator()(std::shared_ptr<DownloadResult> downloadResult) {
    // Process your own downloads sequentially here
    // e.g. write each download result into separate file
    std::ofstream fout(this->generateFilename(std::get<0>(downloadResult->url)));
    if(downloadResult->success) {
      fout << downloadResult->content;
    }
    else {
      fout << *downloadResult;
    }
  }

private:
  static std::string getAlphanumOnly(const std::string& input) {
    std::string result = input;
    for(char& c: result) {
      if(!std::isalnum(c)) {
        c = '_';
      }
    }
    return result;
  }

  std::string generateFilename(const std::string url) {
    std::ostringstream filenameStream;
    filenameStream << m_prefix;
    filenameStream << std::setfill('0') << std::setw(m_maxNumberOfDigits);
    filenameStream << m_seq++;
    filenameStream << std::setw(0);
    filenameStream << getAlphanumOnly(url);

    std::string filename = filenameStream.str();
    if(filename.size() >= MAX_FILENAME_LENGTH) {
      filename.erase(filename.begin() + MAX_FILENAME_LENGTH, filename.end());
    }

    return filename;
  }

private:
  static constexpr int MAX_FILENAME_LENGTH = 50;
  int                  m_maxNumberOfDigits;
  std::string          m_prefix;
  int                  m_seq;
};

void
driverLoadLogic(const DriverOptions& options) {
  std::vector<std::string> urlList;
  if(!options.urlsFilename.empty()) {
    urlList = readUrlsFromFile(options.urlsFilename, options.maxUrls);
  }
  if(!options.url.empty()) {
    urlList.push_back(options.url);
  }
  if(options.printUrls) {
    std::copy(urlList.begin(), urlList.end(), std::ostream_iterator<std::string>(std::cout, "\n"));
  }
  // Keep the taskSystem dependent object above its definition.
  // The TaskSystem destructor waits for all the tasks to finish,
  // only after that dependent objects can be destroyed.
  TaskSystem                taskSystem{/*nrTrheads*/ 1};
  WriteDownloadResultToFile resultsProcessor{(int)urlList.size(), options.prefix};

  std::vector<DownloadElem> urlsToDownload;
  std::transform(urlList.begin(),
                 urlList.end(),
                 std::back_inserter(urlsToDownload),
                 [&taskSystem, &resultsProcessor](const std::string& url) {
                   return DownloadElem{
                       {url, /* urlIndex */ 0}, [&taskSystem, &resultsProcessor](DownloadResult&& downloadResult) {
                         // task system does not support adding move only lambdas, thus the shared ptr
                         auto dwResultPtr = std::make_shared<DownloadResult>(std::move(downloadResult));
                         taskSystem.async_([&resultsProcessor, dwResultPtr] { resultsProcessor(dwResultPtr); });
                       }};
                 });

  CurlAsioDownloader downloader{options.maxContentLength, getMediaTypeValidator()};
  Crawler            crawler{CrawlOnce{},
                  [&]() { return std::move(urlsToDownload); },
                  &downloader,
                  options.parallelDownloads,
                  std::chrono::seconds{2}};
  crawler.crawl();
}

} // namespace

int
main(int argc, const char** argv) {
  return handleExceptions(ProgramLogic<DriverOptions>(argc, argv, readCommandLineArgs, driverLoadLogic));
}

# CheapCrawler
Cheap modular c++ crawler library

This library, although STABLE is a WORK IN PROGRESS. Feel free to use is, ask about it and contribute.

This C++ library is designed to run as many simultaneous downloads as it is physically
possible on a single machine. It is implemented using Boost.Asio with libcurl MULTI_SOCKET.

A sample executable [driver](src/crawler/main.cpp) is available which uses the library to download a list of URLs.
You can use it as a guideline of how to integrate CheapCrawler in your own project.

## Spec

The library has 2 main components the *crawler* and the *downloader*.

Some utilities are also available:
- A logging library
- A task system
- An exception handler
- Other goodies (check [src/utils](src/utils)).

### crawler

Flow:
```
While (keepCrawling)
  Pull URL list from the crawling dispatcher
  Prepare robots.txt download for each host and protocol
  Try download each robots.txt
     log each downloaded robots.txt result
     if no robots.txt => crawl all
     if download error => set result to download error, don't crawl
     else (successful download of robots.txt)
       (NOT IMPLEMENTED YET) For all forbidden urls by robots.txt:
         => remove from download queue
     Download allowed URLs with 2 secs default delay between download requests per same host
```

Some additional specifications:
- Stop and exit when `keepCrawling()` returns false
- Pull URL list from the crawling dispatcher
- Calculate overall robots.txt URL list for the dispatched URL bunch
- Read robots.txt before each call to dispatcher (no persistent robots.txt)
- One download queue per host with 2 secs timeout between downloads per host
- NOT YET IMPLEMENTED: Filter URLs by robots.txt result
- Call download result for each finished download

### downloader

Implements the Downloader interface of the crawler in a performant oriented implementation using libcurl and Boost.Asio.
Multiple simultaneous downloads are possible with a fixed number of threads usage.
Should theoretically support multiple hundreds of simultaneous downloads.

## Compiling

I developed this library on macOS and haven't tested it on anything else, but it should be easy to cross
compile it on Linux.

### Prerequisites

To be able to use CheapCrawler you will need the following:
* macOS or Linux
* [cmake](https://cmake.org) 3.1 or above
* [conan](https://conan.io)
* [uriparser](https://github.com/uriparser/uriparser)

### Build separately

Here is how you can build the library and the driver executable separately.
```
git clone https://github.com/EugenCazacu/CheapCrawler.git
mkdir build && cd build
conan install ../CheapCrawler
cmake -DCHEAP_CRAWLER_RUN_TESTS=ON -DCMAKE_BUILD_TYPE=Release ../CheapCrawler
make
./bin/crawlerDriver -u "http://example.com"
```

### Integrate into a project

Here is how I integrate CheapCrawler into my project. There is definitely room for improvement here.
```
git remote add CheapCrawler https://github.com/EugenCazacu/CheapCrawler.git
git subtree add --squash --prefix=external/CheapCrawler CheapCrawler master
```
To update to a new CheapCrawler version:
`git subtree pull --squash --prefix=external/CheapCrawler CheapCrawler master`

After adding the `CheapCrawler` directory to your cmake build, you can use the `crawlerLibrary` target.

## Future development

I am developing this library along with another private project. I will be adding features and fixing issues
as I need them. If you would like to use this library and need support, feel free to contact me
and I will try to help you. This library is a work in progress and if you find any problems with it,
please submit an issue. If you want to contribute, please get in contact with me beforehand to make sure
I will be able to accept your contribution.

Here is a list of additional features I plan and hope to implement:
 - filter downloads based on robots.txt
 - Implement download bottleneck detector with auto nr downloads increase/decrease
 - Ask for more downloads when starting to starve
 - Go through each http response and think if it applies to just the downloaded page or whole queue for the host
   e.g. 4xx might mean to many requests and should slow down.
 - handle 429 http response
 - handle other error messages
 - save the header response if error

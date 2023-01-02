#include "Logger.h"
LOG_INIT(crawlerCurlAsioDownloader);

#include "crawler/CurlAsioDownloader.h"

#include "CurlGlobalManager.h"
#include "CurlMultiManager.h"
#include "DownloadManager.h"
#include "throwOnError.h"

#include "handleExceptions.h"
#include "throwOnError.h"
#include "unique_resource.h"

#include <boost/asio.hpp>
#include <thread>
#include <unordered_map>
#include <list>

using boost::asio::io_context;
using BErrorCode = boost::system::error_code;
using boost::asio::null_buffers;
using boost::asio::ip::tcp;

using SocketsMap = std::unordered_map<curl_socket_t, std::tuple<tcp::socket, int>>;
using SocketIt   = SocketsMap::iterator;

namespace {

struct ThreadReleaser {
  void operator()(std::thread& t) const {
    handleExceptions([&] { t.join(); });
  };
};

struct StopIoService {
  void operator()(io_context* ioService) { ioService->stop(); }
};

[[maybe_unused]] std::string
to_stringAction(const int action) {
  switch(action) {
    // clang-format off
    case CURL_POLL_NONE   : return std::string("CURL_POLL_NONE");
    case CURL_POLL_IN     : return std::string("CURL_POLL_IN");
    case CURL_POLL_OUT    : return std::string("CURL_POLL_OUT");
    case CURL_POLL_INOUT  : return std::string("CURL_POLL_INOUT");
    case CURL_POLL_REMOVE : return std::string("CURL_POLL_REMOVE");
    default: return std::string("Invalid action:") + std::to_string(action);
      // clang-format on
  }
}

inline bool
isReadCompatible(const int action) {
  return action == CURL_POLL_IN || action == CURL_POLL_INOUT;
}

inline bool
isWriteCompatible(const int action) {
  return action == CURL_POLL_OUT || action == CURL_POLL_INOUT;
}

inline int&
getPrevAction(SocketIt socketIt) {
  return std::get<1>(socketIt->second);
}

} // namespace

struct CurlAsioDownloader::Pimpl {
public:
  Pimpl(size_t maxContentLength, std::function<bool(const MediaType&)>&& mediaTypeValidator);

  void download(DownloadElem&& downloadElem);

private:
  void newDownload(DownloadElem&& downloadElem);

  static curl_socket_t openSocketCb(void* clientp, curlsocktype purpose, curl_sockaddr* address) {
    return static_cast<Pimpl*>(clientp)->openSocket(purpose, address);
  }
  curl_socket_t openSocket(curlsocktype purpose, curl_sockaddr* address);

  static int closeSocketCb(void* clientp, curl_socket_t item) {
    return static_cast<Pimpl*>(clientp)->closeSocket(item);
  }
  int closeSocket(curl_socket_t item);

  static int socketActionCb(CURL* e, curl_socket_t curlSocket, int what, void* userp, void* sockp) {
    static_cast<Pimpl*>(userp)->addSocketActions(e, curlSocket, what, sockp);
    // must return 0 nothing to control from this return value
    return 0;
  }
  void addSocketActions(CURL* e, curl_socket_t curlSocket, int what, void* sockp);

  void doSocketAction(curl_socket_t curlSocket, int action, const BErrorCode& error);

  static int timerCurlCb(CURLM* multi, long timeoutMs, void* userp) {
    return static_cast<Pimpl*>(userp)->timerCurl(timeoutMs);
  }
  int timerCurl(long timeoutMs);

  void timerAsioCb(const BErrorCode& error);

  void processFinishedDownloads();

  void addRead(SocketIt socketIt) {
    std::get<0>(socketIt->second)
        .async_read_some(null_buffers(),
                         [this, curlSocket = socketIt->first](const BErrorCode& error, size_t bytes_transferred) {
                           doSocketAction(curlSocket, CURL_POLL_IN, error);
                         });
  }

  void addWrite(SocketIt socketIt) {
    std::get<0>(socketIt->second)
        .async_write_some(null_buffers(),
                          [this, curlSocket = socketIt->first](const BErrorCode& error, size_t bytes_transferred) {
                            doSocketAction(curlSocket, CURL_POLL_OUT, error);
                          });
  }

private:
  io_context                                                     m_io_context;
  SocketsMap                                                     m_sockets;
  CurlGlobalManager                                              m_global;
  CurlMultiManager                                               m_multi;
  size_t                                                         m_maxContentLength;
  std::function<bool(const MediaType&)>                          m_mediaTypeValidator;
  std::list<DownloadManager>                                     m_downloads;
  boost::asio::executor_work_guard<io_context::executor_type>    m_workGuard;
  boost::asio::deadline_timer                                    m_timer;
  std_experimental::unique_resource<std::thread, ThreadReleaser> m_thread;
  std::unique_ptr<io_context, StopIoService>                     m_ioServiceStopper;
};

// class CurlAsioDownloader::Pimpl
CurlAsioDownloader::Pimpl::Pimpl(size_t maxContentLength, std::function<bool(const MediaType&)>&& mediaTypeValidator)
    : m_io_context{}
    , m_sockets{}
    , m_global{}
    , m_multi{}
    , m_maxContentLength{maxContentLength}
    , m_mediaTypeValidator{std::move(mediaTypeValidator)}
    , m_downloads{}
    , m_workGuard{boost::asio::make_work_guard(m_io_context)}
    , m_timer{m_io_context}
    , m_thread{std::thread{[this]() { m_io_context.run(); }}, ThreadReleaser{}}
    , m_ioServiceStopper{&m_io_context} {
  throwOnError(curl_multi_setopt(m_multi.get(), CURLMOPT_SOCKETFUNCTION, socketActionCb),
               "curl_multi_setopt CURLMOPT_SOCKETFUNCTION");
  throwOnError(curl_multi_setopt(m_multi.get(), CURLMOPT_SOCKETDATA, this), "curl_multi_setopt CURLMOPT_SOCKETDATA");
  throwOnError(curl_multi_setopt(m_multi.get(), CURLMOPT_TIMERFUNCTION, timerCurlCb),
               "curl_multi_setopt CURLMOPT_TIMERFUNCTION");
  throwOnError(curl_multi_setopt(m_multi.get(), CURLMOPT_TIMERDATA, this), "curl_multi_setopt CURLMOPT_TIMERDATA");
}

void
CurlAsioDownloader::Pimpl::download(DownloadElem&& downloadElem) {
  LOG_DEBUG("download: " << downloadElem);
  boost::asio::post(m_io_context,
                    [this, downloadElem = std::move(downloadElem)]() mutable { newDownload(std::move(downloadElem)); });
}

void
CurlAsioDownloader::Pimpl::newDownload(DownloadElem&& downloadElem) {
  LOG_DEBUG("newDownload: " << downloadElem);
  OpenCloseSocketConfig openCloseSocketConfig{&openSocketCb, this, &closeSocketCb, this};
  m_downloads.emplace_back(
      m_multi.get(), std::move(downloadElem), m_maxContentLength, m_mediaTypeValidator, &openCloseSocketConfig);
  LOG_DEBUG("newDownload emplaced back");
  auto addedElemIt = --end(m_downloads);
  m_downloads.back().setFinishedCallback([this, addedElemIt]() {
    LOG_DEBUG("newDownload finished callback, erasing ...");
    m_downloads.erase(addedElemIt);
  });
  LOG_DEBUG("newDownload set finished callback");
}

curl_socket_t
CurlAsioDownloader::Pimpl::openSocket(curlsocktype purpose, curl_sockaddr* address) {
  if(m_io_context.stopped()) {
    LOG_DEBUG("openSocket called after m_io_context stopped");
    return curl_socket_t{};
  }
  if(purpose != CURLSOCKTYPE_IPCXN) {
    throw std::runtime_error("openSocketCb received unexpected purpose: " + std::to_string(purpose));
  }
  tcp::socket tcpSocket{m_io_context};
  switch(address->family) {
    case AF_INET:
      tcpSocket.open(tcp::v4());
      break;
    case AF_INET6:
      tcpSocket.open(tcp::v6());
      break;
    default:
      throw std::runtime_error("openSocket received unexpected domain (family): " + std::to_string(address->family));
  }
  const curl_socket_t sockfd = tcpSocket.native_handle();
  m_sockets.emplace(sockfd, std::make_tuple(std::move(tcpSocket), CURL_POLL_NONE));
  LOG_DEBUG("openSocket:" << sockfd);
  return sockfd;
}

int
CurlAsioDownloader::Pimpl::closeSocket(curl_socket_t item) {
  if(m_io_context.stopped()) {
    LOG_DEBUG("closeSocket called after m_io_context stopped");
    return 0;
  }
  LOG_DEBUG("closeSocket:" << item);
  const auto erasedItems = m_sockets.erase(item);
  if(1 != erasedItems) {
    throw std::runtime_error("closeSocket unexpected number of sockets closed: " + std::to_string(erasedItems));
  }
  return 0;
}

/* Called by asio when there is an action on a socket */
void
CurlAsioDownloader::Pimpl::doSocketAction(curl_socket_t curlSocket, int action, const BErrorCode& error) {
  LOG_DEBUG("doSocketAction: action=" << to_stringAction(action));
  SocketIt socketIt = m_sockets.find(curlSocket);
  if(end(m_sockets) == socketIt) {
    LOG_DEBUG("doSocketAction: socket already closed");
    return;
  }

  /* make sure the event matches what are wanted */
  if(getPrevAction(socketIt) == action || getPrevAction(socketIt) == CURL_POLL_INOUT) {
    if(error) {
      action = CURL_CSELECT_ERR;
    }
    int activeDownloads;
    throwOnError(curl_multi_socket_action(m_multi.get(), curlSocket, action, &activeDownloads),
                 "curl_multi_socket_action");
    processFinishedDownloads();

    if(activeDownloads <= 0) {
      LOG_DEBUG("Last transfer done, kill timeout");
      m_timer.cancel();
    }

    /* keep on watching.
     * the socket may have been closed and/or prevActionPtr may have been changed
     * in curl_multi_socket_action(), so check them both */
    socketIt = m_sockets.find(curlSocket);
    if(socketIt != m_sockets.end()
       && (getPrevAction(socketIt) == action || getPrevAction(socketIt) == CURL_POLL_INOUT)) {
      if(action == CURL_POLL_IN) {
        addRead(socketIt);
      }
      else if(action == CURL_POLL_OUT) {
        addWrite(socketIt);
      }
    }
    else {
      LOG_DEBUG("action and prevActionPtr don't match or socket closed");
    }
  }
  else {
    LOG_DEBUG("action and prevActionPtr don't match");
  }
}

void
CurlAsioDownloader::Pimpl::addSocketActions(CURL* const         easyHandle,
                                            const curl_socket_t curlSocket,
                                            const int           curAction,
                                            void* const         socketData) {
  if(m_io_context.stopped()) {
    LOG_DEBUG("addSocketActions called after m_io_context stopped");
    return;
  }
  const auto socketIt = m_sockets.find(curlSocket);
  if(socketIt == m_sockets.end()) {
    LOG_INFO("Socket " << curlSocket << " is a c-ares socket, ignoring - C library for asynchronous DNS requests");
    return;
  }
  LOG_DEBUG("addSocketActions: "
            << "socket=" << curlSocket << ", prevAction=" << to_stringAction(getPrevAction(socketIt)) << ", curAction="
            << to_stringAction(curAction) << ", socketData=" << socketData << ", easyHandle=" << easyHandle);
  if(isReadCompatible(curAction) && !isReadCompatible(getPrevAction(socketIt))) {
    addRead(socketIt);
  }
  if(isWriteCompatible(curAction) && !isWriteCompatible(getPrevAction(socketIt))) {
    addWrite(socketIt);
  }
  getPrevAction(socketIt) = curAction;
}

int
CurlAsioDownloader::Pimpl::timerCurl(long timeoutMs) {
  if(m_io_context.stopped()) {
    LOG_DEBUG("timerCurl called after m_io_context stopped");
    return 0;
  }
  LOG_DEBUG("timerCurl: timeout (ms) :" << timeoutMs);

  m_timer.cancel();

  if(timeoutMs >= 0) {
    m_timer.expires_from_now(boost::posix_time::millisec(timeoutMs));
    m_timer.async_wait([this](const auto& errorCode) { timerAsioCb(errorCode); });
  }
  else {
    // curl doesn't need the timer any more
  }

  // return -1 on error
  return 0;
}

void
CurlAsioDownloader::Pimpl::timerAsioCb(const BErrorCode& error) {
  if(!error) {
    LOG_DEBUG("timerAsioCb");
    int activeDownloads{0};
    throwOnError(curl_multi_socket_action(m_multi.get(), CURL_SOCKET_TIMEOUT, 0, &activeDownloads),
                 "curl_multi_socket_action timeout");
    processFinishedDownloads();
  }
}

void
CurlAsioDownloader::Pimpl::processFinishedDownloads() {
  do {
    int            messagesInQueue = 0;
    CURLMsg* const infoMsg         = curl_multi_info_read(m_multi.get(), &messagesInQueue);

    if(nullptr == infoMsg || CURLMSG_DONE != infoMsg->msg) {
      break;
    }
    auto finishedCallback = processFinishedDownload(infoMsg);
    finishedCallback();
  } while(true);
}

// class CurlAsioDownloader
CurlAsioDownloader::CurlAsioDownloader(const size_t                          maxContentLength,
                                       std::function<bool(const MediaType&)> mediaTypeValidator)
    : m_pimpl{std::make_unique<Pimpl>(maxContentLength, std::move(mediaTypeValidator))} {}

CurlAsioDownloader::~CurlAsioDownloader() {}

void
CurlAsioDownloader::doDownload(DownloadElem&& downloadElem) {
  m_pimpl->download(std::move(downloadElem));
}

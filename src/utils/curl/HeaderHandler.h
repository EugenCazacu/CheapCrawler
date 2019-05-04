#ifndef UTILS_CURL_HEADERHANDLER_H_Q51KUMVB
#define UTILS_CURL_HEADERHANDLER_H_Q51KUMVB

#include "MediaType.h"

#include <functional>
#include <stddef.h>
#include <string>

class HeaderHandler {
public:
  HeaderHandler(std::function<bool(const MediaType&)> mediaTypeValidator, std::ostream* errorStream)
      : m_buffer()
      , m_state(State::READING_STATUS_LINE)
      , m_mediaType()
      , m_mediaTypeValidator{mediaTypeValidator}
      , m_errorStream{errorStream} {}

  size_t operator()(char*, size_t);

  MediaType getMediaType() const { return m_mediaType; }

  void reuse() {
    m_buffer.clear();
    m_state     = State::READING_STATUS_LINE;
    m_mediaType = MediaType();
  }

private:
  bool process(const std::string&);

private:
  std::string m_buffer;
  enum class State { READING_STATUS_LINE, READING_HEADER_FIELDS, FINISHED } m_state;
  MediaType                             m_mediaType;
  std::function<bool(const MediaType&)> m_mediaTypeValidator;
  std::ostream*                         m_errorStream;
};

#endif /* end of include guard: UTILS_CURL_HEADERHANDLER_H_Q51KUMVB */

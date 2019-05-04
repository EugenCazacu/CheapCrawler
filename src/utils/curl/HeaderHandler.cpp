#include "HeaderHandler.h"
#include "Logger.h"
LOG_INIT(HeaderHandler);

#include <boost/algorithm/string.hpp>
#include <regex>
#include <sstream>
#include <tuple>

#include <assert.h>

namespace {

std::tuple<bool, MediaType>
matchContextType(const std::string& line) {
  static const std::string token(R"([!#\$%&'\*\+-\.\^_`\|~\w]+)");
  static const std::string gtoken('(' + token + ')');
  static const std::string quotedString(R"("[^"\\]*(?:\\.[^"\\]*)*")");
  static const std::string charset("charset");
  static const std::regex  contextTypeExpr("^content-type: ?" + gtoken + '/' + gtoken, std::regex::icase);
  static const std::regex  contexTypeParameter("[ \\t]*;[ \\t]*" + gtoken + '=' + '(' + token + '|' + quotedString
                                              + ')');
  std::smatch              match;
  if(std::regex_search(line, match, contextTypeExpr)) {
    assert(3 == match.size());

    MediaType result;
    result.type    = match[1];
    result.subtype = match[2];

    std::string parameterList = match.suffix();

    while(std::regex_search(parameterList, match, contexTypeParameter)) {
      assert(3 == match.size());
      if(boost::iequals(match[1].str(), charset)) {
        result.charset = match[2];
        break;
      }
      parameterList = match.suffix();
    }
    LOG_DEBUG("found media-type: " << result);
    return std::make_tuple(true, result);
  }
  else {
    return std::make_tuple(false, MediaType());
  }
}

} // namespace

size_t
HeaderHandler::operator()(char* const buffer, const size_t size) {
  LOG_DEBUG("operator(): " << std::string(buffer, size));
  m_buffer.append(buffer, size);

  static const std::string DELIMITER("\x0d\x0a");
  size_t                   start = 0;

  do {
    size_t end = m_buffer.find(DELIMITER, start);
    if(std::string::npos == end) {
      break;
    }
    bool continueDownload = process(m_buffer.substr(start, end));
    if(!continueDownload) {
      LOG_DEBUG("Aborting download");
      return -1;
    }
    start = end + DELIMITER.size();

  } while(true);
  m_buffer.erase(0, start);

  // split buffered input into header fields
  // filter response type
  // find content type header field
  // if content type not supported
  //   return with error
  // else
  //  return save the association of the content type and url

  return size;
}

bool
HeaderHandler::process(const std::string& line) {
  static const std::regex statusLineExpr("^HTTP/1\\.1 (\\d\\d\\d) ");

  LOG_DEBUG("processing line: " << line);
  switch(m_state) {
    case State::READING_STATUS_LINE: {
      std::smatch match;
      if(std::regex_search(line, match, statusLineExpr)) {
        LOG_DEBUG("read status code: " << match[1]);
        assert(match.size() == 2);
        assert(match[1].length() == 3);
        if('2' == match[1].str()[0]) {
          m_state = State::READING_HEADER_FIELDS;
        }
        else {
          LOG_DEBUG("http responce not successful: " << match[1]);
          m_state = State::FINISHED;
          (*m_errorStream) << "http responce not successful: " << match[1].str() << std::endl;
          return false;
        }
      }
      else {
        LOG_DEBUG("could not match status line: " << line);
        m_state = State::FINISHED;
        (*m_errorStream) << "could not match status line: " << line << std::endl;
        return false;
      }
    } break;
    case State::READING_HEADER_FIELDS: {
      bool mediaTypeFound                   = false;
      std::tie(mediaTypeFound, m_mediaType) = matchContextType(line);
      if(mediaTypeFound) {
        const bool continueDownload = this->m_mediaTypeValidator(m_mediaType);
        m_state                     = State::FINISHED;
        if(!continueDownload) {
          (*m_errorStream) << "media type not validated: " << m_mediaType << std::endl;
        }
        return continueDownload;
      }
    } break;
    case State::FINISHED:
      break;
    default:
      throw std::logic_error("class invariant violated");
  }
  return true;
}

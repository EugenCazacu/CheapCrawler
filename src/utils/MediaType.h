#ifndef MEDIATYPE_H_DDPXB9H3
#define MEDIATYPE_H_DDPXB9H3

#include <ostream>
#include <string>

struct MediaType {
  std::string type;
  std::string subtype;
  std::string charset;
};

inline std::ostream&
operator<<(std::ostream& out, const MediaType& mediaType) {
  out << "type:" << mediaType.type;
  out << " subtype:" << mediaType.subtype;
  out << " charset:" << mediaType.charset;
  return out;
}

#endif /* end of include guard: MEDIATYPE_H_DDPXB9H3 */

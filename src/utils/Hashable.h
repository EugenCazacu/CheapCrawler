#ifndef UTILS_HASHABLE_H_L1PSNXHO
#define UTILS_HASHABLE_H_L1PSNXHO

#include <cstddef>
#include <functional>

inline void
hashCombine(std::size_t& seed) {}

template<typename T, typename... Rest>
inline void
hashCombine(std::size_t& seed, const T& v, Rest... rest) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  hashCombine(seed, rest...);
}

#define MAKE_HASHABLE(type, ...)                  \
  namespace std {                                 \
  template<> struct hash<type> {                  \
    std::size_t operator()(const type& t) const { \
      std::size_t ret = 0;                        \
      hashCombine(ret, __VA_ARGS__);              \
      return ret;                                 \
    }                                             \
  };                                              \
  }

#endif /* end of include guard: UTILS_HASHABLE_H_L1PSNXHO */

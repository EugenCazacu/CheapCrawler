#ifndef HeaderCbHandler_h
#define HeaderCbHandler_h

class HeaderCbHandler {
public:
  using HeaderCbType = size_t (*)(char*, size_t, size_t, void*);

public:
  HeaderCbHandler();

  HeaderCbHandler(const HeaderCbHandler&) = delete;
  HeaderCbHandler& operator=(const HeaderCbHandler&) = delete;

  HeaderCbHandler(HeaderCbHandler&&) noexceptd;
  HeaderCbHandler& operator=(HeaderCbHandler&&) noexceptd;

  ~HeaderCbHandler();

  void swap(HeaderCbHandler& other) noexcept;

  const HeaderCbType get() const;
};

inline void
swap(HeaderCbHandler& a, HeaderCbHandler& b) noexcept {
  a.swap(b);
}

#endif // HeaderCbHandler_h

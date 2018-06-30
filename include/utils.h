#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <assert.h>

namespace ch {
namespace internal {

std::string stringf(const char* format, ...);

void dbprint(int level, const char* format, ...);

void dump_stack_trace(FILE* out, uint32_t max_frames = 32);

std::string identifier_from_typeid(const std::string& name);

int char2int(char x, int base);

template <typename T>
T sign_ext(T x, unsigned bits) {
  if (sizeof(T) * 8 <= bits)
    return x;
  T k = (T(1) << bits) - 1;
  T m = T(1) << (bits - 1);
  T n = x & k;
  return (n ^ m) - m;
}

///////////////////////////////////////////////////////////////////////////////

class source_location {
public:
  constexpr source_location(const char* file = nullptr, int line = 0) noexcept
    : file_(file)
    , line_(line)
  {}

  constexpr const char* file() const noexcept {
    return file_;
  }

  constexpr int line() const noexcept {
    return line_;
  }

  bool empty() const {
    return (nullptr == file_);
  }

  void clear() {
    file_ = nullptr;
    line_ = 0;
  }

private:
  const char* file_;
  int line_;
};

#if defined(__GNUC__)
  #define CH_CUR_SLOC ch::internal::source_location(__builtin_FILE(), __builtin_LINE())
#else
  #define CH_CUR_SLOC ch::internal::source_location(__FILE__, __LINE__)
#endif

#define CH_SLOC const ch::internal::source_location& sloc = CH_CUR_SLOC

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename Arg>
void for_each_impl(const F& f, Arg&& arg) {
  f(std::forward<Arg>(arg));
}

template <typename F, typename Arg0, typename... Args>
void for_each_impl(const F& f, Arg0&& arg0, Args&&... args) {
  f(std::forward<Arg0>(arg0));
  for_each_impl(f, std::forward<Args>(args)...);
}

template <typename F, typename... Args>
void for_each(const F& f, Args&&... args) {
  for_each_impl(f, std::forward<Args>(args)...);
}

///////////////////////////////////////////////////////////////////////////////

template <typename F, typename Arg>
void for_each_reverse_impl(const F& f, Arg&& arg) {
  f(std::forward<Arg>(arg));
}

template <typename F, typename Arg0, typename... Args>
void for_each_reverse_impl(const F& f, Arg0&& arg0, Args&&... args) {
  for_each_reverse_impl(f, std::forward<Args>(args)...);
  f(std::forward<Arg0>(arg0));
}

template <typename F, typename... Args>
void for_each_reverse(const F& f, Args&&... args) {
  for_each_reverse_impl(f, std::forward<Args>(args)...);
}

///////////////////////////////////////////////////////////////////////////////

constexpr inline std::size_t hash_combine(std::size_t hash1, std::size_t hash2) {
  return hash1 ^ (hash2 * 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
}

///////////////////////////////////////////////////////////////////////////////

class unique_names {
public:
  unique_names() {}

  std::string get(const std::string& name);

private:
  std::unordered_map<std::string, uint32_t> dups_;
};

///////////////////////////////////////////////////////////////////////////////

template <bool Pred>
using is_true = std::conditional_t<Pred, std::true_type, std::false_type>;

template <bool Pred>
inline constexpr bool is_true_v = is_true<Pred>::value;

template <bool Pred>
using is_false = std::conditional_t<Pred, std::false_type, std::true_type>;

template <bool Pred>
inline constexpr bool is_false_v = is_false<Pred>::value;

///////////////////////////////////////////////////////////////////////////////

template <typename To, typename... Froms>
inline constexpr bool are_all_constructible_v = std::conjunction_v<std::is_constructible<To, Froms>...>;

///////////////////////////////////////////////////////////////////////////////

template<typename A, typename B, typename = void>
struct is_equality_comparable : std::false_type {};

template<typename A, typename B>
struct is_equality_comparable<A, B,
    std::enable_if_t<
      true,
      decltype(std::declval<A&>() == std::declval<B&>(), (void)0)
    >> : std::true_type {};

template<typename A, typename B>
inline constexpr bool is_equality_comparable_v = is_equality_comparable<A, B>::value;

///////////////////////////////////////////////////////////////////////////////

//template <typename T>
//using identity_t = T;

template<typename T>
struct identity_impl {
  typedef T type;
};

template<typename T>
using identity_t = typename identity_impl<T>::type;

struct empty_base {};

template <typename...>
using void_t = void;

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct to_function : public to_function<decltype(&T::operator())> {};

template <typename Class, typename Ret, typename... Args>
struct to_function<Ret(Class::*)(Args...) const> {
  using type = const std::function<Ret(Args...)>;
};

template <typename T>
using to_function_t = typename to_function<T>::type;

///////////////////////////////////////////////////////////////////////////////

template <typename From, typename To>
struct reference_cast {
  using type = To;
};

template <typename To>
struct reference_cast<To, To> {
  using type = const To&;
};

///////////////////////////////////////////////////////////////////////////////

class refcounted {
public:
  
  long acquire() const {
    return ++refcount_;
  }

  long release() const {
    assert(refcount_ > 0);
    long refcount = --refcount_;
    if (0 == refcount) {
      delete this;
    }
    return refcount;
  }
  
  long refcount() const {
    return refcount_;
  }
  
protected:
  
  refcounted() : refcount_(0) {}
  virtual ~refcounted() {}
  
protected:
  
  mutable long refcount_;
  
  template <typename T> friend class refcounted_ptr;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
class refcounted_ptr {
public:
  
  refcounted_ptr() : ptr_(nullptr) {}
  
  ~refcounted_ptr() {
    if (ptr_)
      ptr_->release();
  }
  
  refcounted_ptr(const refcounted_ptr& other) : refcounted_ptr(other.ptr_) {}
  
  refcounted_ptr(refcounted_ptr&& other) {
    ptr_ = other.ptr_;
    other.ptr_ == nullptr;
  }
  
  refcounted_ptr& operator=(const refcounted_ptr& other) {
    if (other.ptr_)
      other.ptr_->acquire();
    if (ptr_)
      ptr_->release();
    ptr_ = other.ptr_;
    return *this;
  }
  
  refcounted_ptr& operator=(refcounted_ptr&& other) {
    if (other.ptr_)
      other.ptr_->release();
    ptr_ = other.ptr_;
    other.ptr_ == nullptr;
  }
  
  T& operator*() const {
    return *ptr_;
  }
  
  T* operator->() const {
    return ptr_;
  }
  
  T* get() const {
    return ptr_;
  }
  
  T* release() const {
    T* ret(ptr_);
    if (ret)
     ret->release();    
    ptr_ = nullptr;
    return ret;
  }
  
  bool is_unique() const {
    return ptr_ ? (ptr_->refcount() == 1) : false;
  }
  
  uint32_t use_count() const {
    return ptr_ ? (ptr_->refcount() == 1) : false;
  }
  
  void reset(T* ptr = nullptr) {
    if (ptr)
      ptr->acquire();
    if (ptr_)
      ptr_->release();
    ptr_ = ptr;    
    return *this;
  }
  
  bool operator==(const refcounted_ptr& other) const {
    return (ptr_ == other.ptr_);
  }
  
  bool operator==(const T* other) const {
    return (ptr_ == other);
  }
  
  operator bool() const {
    return this->use_count() != 0;
  }
  
protected:    
  
  refcounted_ptr(refcounted* ptr) : ptr_(ptr) {
    if (ptr)
      ptr->acquire();
  }
  
  refcounted* ptr_;
  
  template <typename T_, typename... Args>
  friend refcounted_ptr<T_> make_ptr(const Args&... args);
};

template <typename T, typename... Args>
refcounted_ptr<T> make_ptr(const Args&... args) {
  return refcounted_ptr<T>(new T(args...));
} 

///////////////////////////////////////////////////////////////////////////////

template<class InputIt, class T>
typename std::iterator_traits<InputIt>::difference_type
find_distance(InputIt first, InputIt last, const T& value) {
  typename std::iterator_traits<InputIt>::difference_type index = 0;
  for (;first != last; ++first, ++index) {
    if (*first == value) {
      return index;
    }
  }
  return -1;
}

///////////////////////////////////////////////////////////////////////////////

class scope_exit {
public:
  scope_exit(const std::function<void()>& f) : f_(f) {}
  ~scope_exit() { f_(); }
  // force stack only allocation!
  static void *operator new   (size_t) = delete;
  static void *operator new[] (size_t) = delete;
protected:
  std::function<void()> f_;
};

///////////////////////////////////////////////////////////////////////////////

template <class CharT, class Traits = std::char_traits<CharT>>
class basic_auto_indent : public std::basic_streambuf<CharT, Traits> {
public:
  explicit basic_auto_indent(std::basic_ostream<CharT, Traits>& out, int indent = 2)
    : out_(out)
    , indent_(indent)
    , add_indent_(true) {
    buf_ = out.rdbuf();
    out.rdbuf(this);
  }
  ~basic_auto_indent() {
    out_.rdbuf(buf_);
  }
protected:
  virtual int overflow(int ch) override {
    if (add_indent_ && ch != '\n') {
      for (int n = indent_; n--;) {
        buf_->sputc(' ');
      }
    }
    add_indent_ = (ch == '\n');
    return buf_->sputc(ch);
  }
  std::basic_ostream<CharT, Traits>&   out_;
  int indent_;
  std::basic_streambuf<CharT, Traits>* buf_;
  bool add_indent_;
};

using auto_indent = basic_auto_indent<char>;

///////////////////////////////////////////////////////////////////////////////

template <class CharT, class Traits = std::char_traits<CharT>>
class basic_auto_separator {
public:
  basic_auto_separator(const std::basic_string<CharT, Traits>& sep)
    : sep_(sep)
    , enabled_(false)
  {}
  const auto& data() const {
    const auto& ret = enabled_ ? sep_ : empty_;
    enabled_ = true;
    return ret;
  }
protected:
  std::basic_string<CharT, Traits> sep_;
  std::basic_string<CharT, Traits> empty_;
  mutable bool enabled_;

  friend std::ostream& operator<<(std::ostream& out, const basic_auto_separator& sep) {
    return out << sep.data();
  }
};

using auto_separator = basic_auto_separator<char>;

///////////////////////////////////////////////////////////////////////////////

template <typename... Args>
void unused(Args&&...) {}

///////////////////////////////////////////////////////////////////////////////

// usage: static_print<type>();
template<typename T> struct static_print;

///////////////////////////////////////////////////////////////////////////////

// is power of two number ?
constexpr bool ispow2(uint32_t value) {
  return value && !(value & (value - 1)); 
}

// return ilog2
constexpr uint32_t ilog2(uint32_t x) {
  return (x <= 1) ? 0 : (ilog2(x >> 1) + 1);
}

// return ceil of log2
constexpr uint32_t log2ceil(uint32_t x) {
  return ispow2(x) ? ilog2(x) : (ilog2(x) + 1);
}

template <typename Dst, typename Src>
auto bitcast(const Src& src) {
  union merged_t { Src src; Dst dst; };
  merged_t m;
  m.dst = 0;
  m.src = src;
  return m.dst;  
}

constexpr uint32_t clz(uint32_t value) {
  return value ? __builtin_clz(value) : 32;
}

constexpr uint32_t ctz(uint32_t value) {
  return value ? __builtin_ctz(value) : 32;
}

constexpr uint32_t clz(uint64_t value) {
  return value ? __builtin_clzll(value) : 64;
}

constexpr uint32_t ctz(uint64_t value) {
  return value ? __builtin_ctzll(value) : 64;
}

constexpr uint32_t ceilpow2(uint64_t value) {
  return 64 - clz(value);
}

constexpr uint32_t ceilpow2(int64_t value) {
  return value >= 0 ? (64 - clz(uint64_t(value))) : (65 - clz(uint64_t(~value)));
}

constexpr uint32_t rotl(uint32_t value, uint32_t shift, uint32_t width) {
  assert(shift < width);
  return  (value << shift) | (value >> (width - shift));
}

constexpr uint32_t rotr(uint32_t value, uint32_t shift, uint32_t width) {
  assert(shift < width);
  return (value >> shift) | (value << (width  - shift));
}

}
}

#ifdef NDEBUG
  #define CH_ABORT(...) \
    do { \
      fprintf(stderr, "ERROR: "); \
      fprintf(stderr, __VA_ARGS__); \
      std::abort(); \
    } while (false)

  #define DBG(level, format, ...)
#else
  #define CH_ABORT(...) \
    do { \
      fprintf(stderr, "ERROR: "); \
      fprintf(stderr, __VA_ARGS__); \
      fprintf(stderr, " (" __FILE__ ":" CH_STRINGIZE(__LINE__) ")\n"); \
      std::abort(); \
    } while (false)

  #define DBG(level, ...) \
    dbprint(level, __VA_ARGS__)
#endif

#define CH_CHECK(pred, ...) \
  do { \
    if (!(pred)) { \
      fprintf(stderr, "ERROR: assertion `" CH_STRINGIZE(pred) "' failed, "); \
      fprintf(stderr, __VA_ARGS__); \
      fprintf(stderr, " (" __FILE__ ":" CH_STRINGIZE(__LINE__) ")\n"); \
      std::abort(); \
    } \
  } while (false)

#define CH_DEF_SFINAE_CHECK(type_name, predicate) \
  template<typename T, typename Enable = void> \
  struct type_name : std::false_type {}; \
  template<typename T> \
  struct type_name<T, std::enable_if_t<(predicate)>> : std::true_type {}; \
  template <typename T> \
  inline constexpr bool type_name##_v = type_name<T>::value

template <unsigned N>
struct requires_enum {
  enum class type {};
};

#define CH_REQUIRE_0(...) std::enable_if_t<(__VA_ARGS__), typename requires_enum<0>::type>* = nullptr
#define CH_REQUIRE_1(...) std::enable_if_t<(__VA_ARGS__), typename requires_enum<1>::type>* = nullptr
#define CH_REQUIRE_2(...) std::enable_if_t<(__VA_ARGS__), typename requires_enum<2>::type>* = nullptr
#define CH_REQUIRE_3(...) std::enable_if_t<(__VA_ARGS__), typename requires_enum<3>::type>* = nullptr

#define CH_UNUSED(...) ch::internal::unused(__VA_ARGS__)

#define CH_TODO() CH_ABORT("Not yet implemented");

#define CH_COUNTOF(a) (sizeof(a) / sizeof(a[0]))

#define CH_MAX(a,b) (((a) > (b)) ? (a) : (b))

#define CH_CEILDIV(a, b) (((a) + (b) - 1) / b)

#define CH_BLEND(m, a, b)  (a) ^ (((a) ^ (b)) & (m)) // 0->a, 1->b

#define CH_WIDTH_OF(a) (sizeof(a) * 8)
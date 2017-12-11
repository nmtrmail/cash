#pragma once

#include "struct.h"
#include "seq.h"

namespace ch {
namespace internal {

lnodeimpl* createInputNode(const std::string& name, uint32_t size);

lnodeimpl* createOutputNode(const std::string& name, const lnode& src);

void bindInput(const lnode& src, const lnode& input);

void bindOutput(const lnode& dst, const lnode& output);

///////////////////////////////////////////////////////////////////////////////

template <typename T> class ch_in;

template <typename T> class ch_out;

template <typename T> class scalar_in;

template <typename T> class scalar_out;

inline constexpr auto operator|(ch_direction lsh, ch_direction rhs) {
  return ch_direction((int)lsh | (int)rhs);
}

template <typename IoType,
          ch_direction Direction,
          typename FlipType,
          typename DeviceType,
          typename LogicType,
          typename Next = void>
struct io_traits {
  static constexpr traits_type type = traits_io;
  static constexpr unsigned bitwidth = bitwidth_v<LogicType>;
  static constexpr ch_direction direction = Direction;
  using io_type     = IoType;  
  using flip_type   = FlipType;
  using device_type = DeviceType;
  using logic_type  = LogicType;
  using scalar_type = scalar_type_t<LogicType>;
  using next        = Next;
};

template <typename T>
using io_type_t = typename std::decay_t<T>::traits::io_type;

template <typename T>
using flip_type_t = typename std::decay_t<T>::traits::flip_type;

template <typename T>
using device_type_t = typename std::decay_t<T>::traits::device_type;

template <typename T>
using buffer_type_t = typename std::decay_t<T>::traits::buffer_type;

template <typename T>
inline constexpr ch_direction direction_v = std::decay_t<T>::traits::direction;

template <typename T>
struct is_io_traits : std::false_type {};

template <typename IoType,
          ch_direction Direction,
          typename FlipType,
          typename DeviceType,
          typename LogicType,
          typename Next>
struct is_io_traits<io_traits<IoType,
                              Direction,
                              FlipType,
                              DeviceType,
                              LogicType,
                              Next>> : std::true_type {};

CH_DEF_SFINAE_CHECK(is_io_type, is_io_traits<typename std::decay_t<T>::traits>::value);

///////////////////////////////////////////////////////////////////////////////

template <typename T>
class ch_in final : public const_type_t<T> {
public:
  static_assert(is_logic_type<T>::value && is_value_type<T>::value, "invalid type");
  using base = const_type_t<T>;
  using traits = io_traits<ch_in<T>, ch_direction::in, ch_out<T>, scalar_in<T>,
                           T, typename base::traits>;

  ch_in(const std::string& name = "io", const source_location& sloc = CH_SOURCE_LOCATION)
    : base(bit_buffer(bitwidth_v<T>, sloc, name)) {
    input_ = createInputNode(name, bitwidth_v<T>);
    bit_accessor::set_data(*this, input_);
  }

  explicit ch_in(const ch_out<T>& out, const source_location& sloc = CH_SOURCE_LOCATION)
    : base(bit_buffer(bitwidth_v<T>, sloc)) {
    input_ = bit_accessor::get_data(*this);
    bindOutput(input_, out.output_);
  }

  void operator()(ch_out<T>& out) const {
    out = *this;
  }

private:

  ch_in(const ch_in& in) = delete;
  ch_in(ch_in&& in) = delete;

  ch_in& operator=(const ch_in&) = delete;
  ch_in& operator=(ch_in&&) = delete;

  lnode input_;

  template <typename U> friend class ch_out;
  template <typename U> friend class scalar_in;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
class ch_out final : public T {
public:
  static_assert(is_logic_type<T>::value && is_value_type<T>::value, "invalid type");
  using base = T;
  using traits = io_traits<ch_out, ch_direction::out, ch_in<T>, scalar_out<T>,
                           T, typename base::traits>;
  using base::operator=;

  ch_out(const std::string& name = "io", const source_location& sloc = CH_SOURCE_LOCATION)
    : base(bit_buffer(bitwidth_v<T>, sloc, name)) {
    output_ = createOutputNode(name, bit_accessor::get_data(*this));
  }

  explicit ch_out(const ch_in<T>& in, const source_location& sloc = CH_SOURCE_LOCATION)
    : base(bit_buffer(bitwidth_v<T>, sloc)) {
    output_ = bit_accessor::get_data(*this);
    bindInput(output_, in.input_);
  }

  ch_out& operator=(const ch_out& rhs) {
    base::operator=(rhs);
    return *this;
  }

  void operator()(const ch_in<T>& in) {
    *this = in;
  }

private:

  ch_out(const ch_out& in) = delete;
  ch_out(ch_out&& in) = delete;
  ch_out& operator=(ch_out&&) = delete;

  lnode output_;

  template <typename U> friend class ch_in;
  template <typename U> friend class scalar_out;
};

///////////////////////////////////////////////////////////////////////////////

class scalar_buffer_io : public scalar_buffer_impl {
public:
  using base = scalar_buffer_impl;

  scalar_buffer_io(const lnode& io)
    : base(bitvector(), nullptr, 0, io.get_size()), io_(io)
  {}

  const bitvector& get_data() const override {
    return io_.get_data();
  }

  void read(uint32_t dst_offset,
            void* out,
            uint32_t out_cbsize,
            uint32_t src_offset,
            uint32_t length) const override {
    io_.get_data().read(dst_offset, out, out_cbsize, src_offset, length);
  }

  void write(uint32_t dst_offset,
             const void* in,
             uint32_t in_cbsize,
             uint32_t src_offset,
             uint32_t length) override {
    io_.get_data().write(dst_offset, in, in_cbsize, src_offset, length);
  }

  lnode io_;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
class scalar_in : public scalar_type_t<T> {
public:
  static_assert(is_logic_type<T>::value && is_value_type<T>::value, "invalid type");
  using base = scalar_type_t<T>;
  using traits = io_traits<scalar_in, ch_direction::out, scalar_out<T>,
                           ch_in<T>, T, typename base::traits>;
  using base::operator=;

  explicit scalar_in(const ch_in<T>& in)
    : base(ch::internal::scalar_buffer_ptr(new scalar_buffer_io(in.input_)))
  {}

protected:

  scalar_in(const scalar_in& out) = delete;
  scalar_in(scalar_in&& out) = delete;

  scalar_in& operator=(const scalar_in&) = delete;
  scalar_in& operator=(scalar_in&&) = delete;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
class scalar_out : public const_type_t<scalar_type_t<T>> {
public:
  static_assert(is_logic_type<T>::value && is_value_type<T>::value, "invalid type");
  using base = const_type_t<scalar_type_t<T>>;
  using traits = io_traits<scalar_out, ch_direction::out, scalar_in<T>,
                           ch_out<T>, T, typename base::traits>;

  explicit scalar_out(const ch_out<T>& out)
    : base(ch::internal::scalar_buffer_ptr(new scalar_buffer_io(out.output_)))
  {}

protected:

  scalar_out(const scalar_out& out) = delete;
  scalar_out(scalar_out&& out) = delete;

  scalar_out& operator=(const scalar_out&) = delete;
  scalar_out& operator=(scalar_out&&) = delete;
};

}
}

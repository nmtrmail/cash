#pragma once

#include "numbase.h"

namespace ch {
namespace internal {

template <unsigned N = 32>
class ch_sint : public ch_snumbase<ch_sint<N>> {
public:
  using traits = system_traits<N, true, ch_sint, ch_int<N>>;
  using base = ch_snumbase<ch_sint<N>>;
  using base::operator=;

  explicit ch_sint(const system_buffer& buffer = make_system_buffer(N))
    : buffer_(buffer) {
    assert(N == buffer->size());
  }

  template <typename U,
            CH_REQUIRES(std::is_integral_v<U>)>
  ch_sint(const U& other)
    : ch_sint(make_system_buffer(N)) {
    this->operator=(other);
  }

  template <typename U,
            CH_REQUIRES(is_bitvector_extended_type_v<U>)>
  explicit ch_sint(U&& other)
    : ch_sint(make_system_buffer(N)) {
    this->operator=(ch_sint(make_system_buffer(sdata_type(N , std::forward<U>(other)))));
  }

  template <typename U,
            CH_REQUIRES(ch_width_v<U> <= N)>
  explicit ch_sint(const ch_sbitbase<U>& other)
    : ch_sint(make_system_buffer(N)) {
    this->operator=(reinterpret_cast<const U&>(other));
  }

  ch_sint(const ch_sbool& other)
    : ch_sint(make_system_buffer(N)) {
    this->operator=(other);
  }

  template <unsigned M,
            CH_REQUIRES(M <= N)>
  ch_sint(const ch_sbit<M>& other)
    : ch_sint(make_system_buffer(N)) {
    this->operator=(other);
  }

  template <unsigned M,
            CH_REQUIRES(M < N)>
  ch_sint(const ch_suint<M>& other)
    : ch_sint(make_system_buffer(N)) {
    this->operator=(other);
  }

  template <unsigned M,
            CH_REQUIRES(M < N)>
  ch_sint(const ch_sint<M>& other)
    : ch_sint(make_system_buffer(N)) {
    this->operator=(other);
  }

  ch_sint(const ch_sint& other)
    : ch_sint(make_system_buffer(N)) {
    this->operator=(other);
  }

  ch_sint(ch_sint&& other) : buffer_(std::move(other.buffer_)) {}

  ch_sint& operator=(const ch_sint& other) {
    system_accessor::assign(*this, other);
    return *this;
  }

  ch_sint& operator=(ch_sint&& other) {
    system_accessor::move(*this, std::move(other));
    return *this;
  }

protected:

  const system_buffer& __buffer() const {
    return buffer_;
  }

  system_buffer buffer_;

  friend class system_accessor;
};

///////////////////////////////////////////////////////////////////////////////

template <unsigned N = 32>
class ch_int : public ch_numbase<ch_int<N>> {
public:  
  using traits = logic_traits<N, true, ch_int, ch_sint<N>>;
  using base = ch_numbase<ch_int<N>>;
  using base::operator=;

  explicit ch_int(const logic_buffer& buffer 
    = make_logic_buffer(N, CH_CUR_SRC_INFO))
    : buffer_(buffer) {
    assert(N == buffer.size());
  }

  template <typename U,
            CH_REQUIRES(std::is_integral_v<U>)>
  ch_int(const U& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  template <typename U,
            CH_REQUIRES(ch_width_v<U> <= N)>
  explicit ch_int(const ch_sbitbase<U>& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(reinterpret_cast<const U&>(other));
  }

  ch_int(const ch_sbool& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  template <unsigned M,
            CH_REQUIRES(M <= N)>
  ch_int(const ch_sbit<M>& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  template <unsigned M,
            CH_REQUIRES(M <= N)>
  ch_int(const ch_sint<M>& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  template <unsigned M,
            CH_REQUIRES(M <= N)>
  ch_int(const ch_suint<M>& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  template <typename U,
            CH_REQUIRES(ch_width_v<U> <= N)>
  explicit ch_int(const ch_bitbase<U>& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(reinterpret_cast<const U&>(other));
  }

  ch_int(const ch_bool& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  template <unsigned M,
            CH_REQUIRES(M <= N)>
  ch_int(const ch_bit<M>& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  template <unsigned M,
            CH_REQUIRES(M < N)>
  ch_int(const ch_uint<M>& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  template <unsigned M,
            CH_REQUIRES(M < N)>
  ch_int(const ch_int<M>& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  ch_int(const ch_int& other, CH_SRC_INFO)
    : ch_int(make_logic_buffer(N, srcinfo)) {
    this->operator=(other);
  }

  ch_int(ch_int&& other) : buffer_(std::move(other.buffer_)) {}

  ch_int& operator=(const ch_int& other) {
    logic_accessor::assign(*this, other);
    return *this;
  }

  ch_int& operator=(ch_int&& other) {
    logic_accessor::move(*this, std::move(other));
    return *this;
  }

protected:

  const logic_buffer& __buffer() const {
    return buffer_;
  }

  logic_buffer buffer_;

  friend class logic_accessor;
};

}}

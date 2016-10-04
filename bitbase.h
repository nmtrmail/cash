#pragma once

#include "typebase.h"
#include "lnode.h"

namespace chdl_internal {

template <unsigned N> class ch_bitv;

template <unsigned N>
class typebase<N, lnode> {
public:   
  static const unsigned bit_count = N;
  typedef lnode data_type;
  
  typebase() {}
  virtual ~typebase() {}
  
  template <unsigned M>
  const_slice_ref<typebase, M> slice(size_t index) const {
    return const_slice_ref<typebase, M>(*this, index);
  }
  
  template <unsigned M>
  slice_ref<typebase, M> slice(size_t index) {
    return slice_ref<typebase, M>(*this, index);
  }
  
  template <unsigned M>
  const_slice_ref<typebase, M> aslice(size_t index) const {
    return const_slice_ref<typebase, M>(*this, index * M);
  }
  
  template <unsigned M>
  slice_ref<typebase, M> aslice(size_t index) {
    return slice_ref<typebase, M>(*this, index * M);
  }
  
  const_subscript_ref<typebase> operator[](size_t index) const {
    return const_subscript_ref<typebase>(*this, index); 
  }
  
  subscript_ref<typebase> operator[](size_t index) {
    return subscript_ref<typebase>(*this, index); 
  }
  
  typebase& operator=(const typebase& rhs) {
    std::vector< partition<data_type> > data;
    rhs.read(data, 0, N);
    this->write(0, data, 0, N);
    return *this;
  }
  
  typebase& operator=(const std::initializer_list<uint32_t>& value) {
    return this->operator =(ch_bitv<N>(value));
  } 
  
  typebase& operator=(int value) {
    return this->operator =({(uint32_t)value});
  }
  
  operator const lnode&() const { 
    return ch_bitv<N>(*this); 
  }
  
  operator lnode&() { 
    return ch_bitv<N>(*this);
  }
  
protected:

  virtual void read(std::vector< partition<data_type> >& out, size_t offset, size_t length) const = 0;
  virtual void write(size_t dst_offset, const std::vector< partition<data_type> >& src, size_t src_offset, size_t src_length) = 0;
  
  template <typename T_> friend void read_data(const T_& t, std::vector< partition<typename T_::data_type> >& out, size_t offset, size_t length);
  template <typename T_> friend void write_data(T_& t, size_t dst_offset, const std::vector< partition<typename T_::data_type> >& src, size_t src_offset, size_t src_length);  
};

template <>
class typebase<1, lnode> {
public:   
  static const unsigned bit_count = 1;
  typedef lnode data_type;
  
  typebase() {}
  virtual ~typebase() {}
  
  template <unsigned M> const_slice_ref<typebase, M> slice(size_t index) const {
    return const_slice_ref<typebase, M>(*this, index);
  }
  
  template <unsigned M> slice_ref<typebase, M> slice(size_t index) {
    return slice_ref<typebase, M>(*this, index);
  }
  
  template <unsigned M> const_slice_ref<typebase, M> aslice(size_t index) const {
    return const_slice_ref<typebase, M>(*this, index * M);
  }
  
  template <unsigned M> slice_ref<typebase, M> aslice(size_t index) {
    return slice_ref<typebase, M>(*this, index * M);
  }
  
  const_subscript_ref<typebase> operator[](size_t index) const {
    return const_subscript_ref<typebase>(*this, index); 
  }
  
  subscript_ref<typebase> operator[](size_t index) {
    return subscript_ref<typebase>(*this, index); 
  }
  
  typebase& operator=(const typebase& rhs) {
    std::vector< partition<data_type> > data;
    rhs.read(data, 0, 1);
    this->write(0, data, 0, 1);
    return *this;
  }
  
  typebase& operator=(const std::initializer_list<uint32_t>& value); 
  
  typebase& operator=(int value) {
    return this->operator =({(uint32_t)value});
  }
  
  typebase& operator=(bool value) {
    return this->operator =(value ? 0x1 : 0x0);
  }
  
  typebase& operator=(char value) {
    return this->operator =(value != '0');
  }
  
  operator const lnode&() const;
  
  operator lnode&();
  
protected:

  virtual void read(std::vector< partition<data_type> >& out, size_t offset, size_t length) const = 0;
  virtual void write(size_t dst_offset, const std::vector< partition<data_type> >& src, size_t src_offset, size_t src_length) = 0;
  
  template <typename T_> friend void read_data(const T_& t, std::vector< partition<typename T_::data_type> >& out, size_t offset, size_t length);
  template <typename T_> friend void write_data(T_& t, size_t dst_offset, const std::vector< partition<typename T_::data_type> >& src, size_t src_offset, size_t src_length);  
};

template <unsigned N> using ch_bitbase = typebase<N, lnode>;
using ch_logicbase = ch_bitbase<1>;

}

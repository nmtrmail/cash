#pragma once

#include "bit.h"
#include "arithm.h"

namespace cash {
namespace internal {

class ch_real : public ch_bit<32> {
public:
  using base = ch_bit<32>;
  using data_type = typename base::data_type;
  using bus_type = ch_bus<32>;  
  using base::operator=;
  using base::base;
  
  ch_real() {}
  ch_real(const base& rhs) : base(rhs) {}
  ch_real(const ch_real& rhs) : base(rhs) {}
  ch_real(float rhs) : base(bitcast<uint32_t, float>(rhs)) {}
  
  ch_real& operator=(const ch_real& rhs) {
    base::operator =(rhs);
    return *this;
  }
  
  ch_real& operator=(float rhs) {
    base::operator =(ch_real(rhs));
    return *this;
  }
};

inline ch_real operator+(const ch_real& lhs, const ch_real& rhs) {
  return make_bit<32>(createAluNode(alu_op_fadd, get_node(lhs), get_node(rhs)));
}

inline ch_real operator-(const ch_real& lhs, const ch_real& rhs) {
  return make_bit<32>(createAluNode(alu_op_fsub, get_node(lhs), get_node(rhs)));
}

inline ch_real operator*(const ch_real& lhs, const ch_real& rhs) {
  return make_bit<32>(createAluNode(alu_op_fmult, get_node(lhs), get_node(rhs)));
}

inline ch_real operator/(const ch_real& lhs, const ch_real& rhs) {
  return make_bit<32>(createAluNode(alu_op_fdiv, get_node(lhs), get_node(rhs)));
}

}
}

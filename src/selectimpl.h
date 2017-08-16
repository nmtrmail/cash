#pragma once

#include "lnodeimpl.h"

namespace cash {
namespace detail {

class selectimpl : public lnodeimpl {
public:
  selectimpl(const lnode& cond, const lnode& true_, const lnode& false_);
  
  lnodeimpl* get_cond() const {
    return srcs_[0].get_impl();
  }

  void set_cond(lnodeimpl* impl) {
    srcs_[0].set_impl(impl);
  }
  
  lnodeimpl* get_true() const {
    return srcs_[1].get_impl();
  }

  void set_true(lnodeimpl* impl) {
    srcs_[1].set_impl(impl);
  }
  
  lnodeimpl* get_false() const {
    return srcs_[2].get_impl();
  }

  void set_false(lnodeimpl* impl) {
    srcs_[2].set_impl(impl);
  }

  const bitvector& eval(ch_cycle t) override;  
  void print_vl(std::ostream& out) const override;
  
private:
  ch_cycle ctime_;
};

}
}
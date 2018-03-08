#pragma once

#include "common.h"
#include "arithm.h"

namespace ch {
namespace internal {

class context;
class lnodeimpl;
class litimpl;
class proxyimpl;
class aluimpl;
class selectimpl;
class regimpl;
class latchimpl;
class memimpl;
class bindimpl;
class bindportimpl;
class bitvector;
class cdomain;

class firrtlwriter {
public:

  firrtlwriter(std::ostream& out);
  ~firrtlwriter();

  void print(context* ctx);

protected:

  struct module_t {
    context* ctx;
    uint32_t num_temps;
    std::unordered_map<unsigned, std::unordered_set<lnodeimpl*>> uses;

    module_t(context* p_ctx);

    bool is_inline_subscript(lnodeimpl* node) const;
  };

  bool print_module(module_t& module);

  void print_header(module_t& module);

  void print_body(module_t& module);

  void print_footer(module_t& module);

  void print_port(lnodeimpl* node);

  bool print_decl(lnodeimpl* node,
                  std::unordered_set<uint32_t>& visited);

  bool print_binding(bindimpl* node);

  bool print_bindport(bindportimpl* node);

  bool print_logic(module_t& module, lnodeimpl* node);

  void print_name(lnodeimpl* node, bool force = false);

  void print_type(lnodeimpl* node);

  void print_dtype(lnodeimpl* node);

  void print_value(const bitvector& value,
                   bool skip_leading_zeros_enable = false,
                   unsigned offset = 0,
                   unsigned size = 0);

  void print_proxy(module_t& module, proxyimpl* node);

  void print_alu(module_t& module, aluimpl* node);

  void print_rotate(aluimpl* node, bool right_dir);

  void print_mux(aluimpl* node);

  void print_fmult(aluimpl* node);

  void print_fadd(aluimpl* node);

  void print_select(module_t& module, selectimpl* node);

  void print_reg(regimpl* node);

  void print_cdomain(cdomain* cd);

  void print_mem(memimpl* node);

  void print_operator(ch_alu_op op);

  std::ostream& out_;
  std::unordered_set<std::string_view> visited_;
};

}
}
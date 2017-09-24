#pragma once

#include "common.h"

namespace cash {
namespace internal {

class context;
class snodeimpl;

class deviceimpl : public refcounted {
public:
  deviceimpl();

  ~deviceimpl();

  void begin_context();

  void end_context();

  void compile();

  context* get_ctx() const {
    return ctx_;
  }

  snodeimpl* get_tap(const std::string& name, uint32_t size) const;

  void to_verilog(const std::string& module_name, std::ostream& out);

  void dump_stats(std::ostream& out);

private:
  context* ctx_;
  context* old_ctx_;
};

}
}
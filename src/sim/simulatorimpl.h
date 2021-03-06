#pragma once

#include "device.h"

namespace ch {
namespace internal {

class inputimpl;
using io_value_t = smart_ptr<sdata_type>;

class clock_driver {
public:

  clock_driver(bool value = false) : value_(value) {}

  void add_signal(inputimpl* node);

  void eval();

  bool empty() const {
    return nodes_.empty();
  }

protected:

  std::vector<io_value_t> nodes_;
  bool value_;
};

class time_driver {
public:

  time_driver(bool value = false) : value_(value) {}

  void add_signal(inputimpl* node);

  void eval();

  bool empty() const {
    return nodes_.empty();
  }

protected:

  std::vector<io_value_t> nodes_;
  uint64_t value_;
};

class sim_driver : public refcounted {
public:

  sim_driver() {}

  virtual ~sim_driver() {}

  virtual void initialize(const std::vector<lnodeimpl*>&) = 0;

  virtual void eval() = 0;
};

class simulatorimpl : public refcounted {
public:

  simulatorimpl(const std::vector<device_base>& devices);

  virtual ~simulatorimpl();

  virtual void initialize();

  ch_tick run(const std::function<bool(ch_tick)>& callback, ch_tick steps);

  void reset();

  void step(ch_tick ticks);

  void run(ch_tick ticks);

  virtual void eval();

protected:  

  std::vector<context*> contexts_;
  context*  eval_ctx_;
  clock_driver clk_driver_;
  clock_driver reset_driver_;
  sim_driver* sim_driver_;
  ch_tick ticks_;
  bool verbose_tracing_;
};

}
}

﻿#include "simulatorimpl.h"
#include "simulator.h"
#include "compile.h"
#include "deviceimpl.h"
#include "litimpl.h"
#include "ioimpl.h"
#include "cdimpl.h"
#include "simref.h"
#include "simjit.h"

using namespace ch::internal;

void clock_driver::add_signal(inputimpl* node) {
  *node->value() = value_;
  nodes_.push_back(node->value());
}

void clock_driver::eval() {
  value_ = !value_;
  for (auto node : nodes_) {
    *node = value_;
  }
}

///////////////////////////////////////////////////////////////////////////////

simulatorimpl::simulatorimpl(const ch_device_list& devices)
  : clk_driver_(true)
  , reset_driver_(false)
  , sim_driver_(nullptr) {
  // enqueue all contexts
  for (auto dev : devices) {
    auto ctx = dev.impl()->ctx();
    contexts_.emplace_back(ctx);
    ctx->acquire();
  }
  // initialize
  this->initialize();
}

simulatorimpl::~simulatorimpl() {
  if (sim_driver_)
    sim_driver_->release();
  for (auto ctx : contexts_) {
    ctx->release();
  }
}

void simulatorimpl::initialize() {
  {
    context* eval_ctx = nullptr;
    if (1 == contexts_.size()
     && 0 == contexts_[0]->bindings().size()) {
      eval_ctx = contexts_[0];
      eval_ctx->acquire();
    } else {
      eval_ctx = new context("eval");
      eval_ctx->acquire();

      // build evaluation context
      for (auto ctx : contexts_) {
        compiler compiler(ctx);
        compiler.build_eval_context(eval_ctx);
      }

      // compile evaluation context
      compiler compiler(eval_ctx);
      compiler.compile();
    }

    // build evaluation list
    std::vector<lnodeimpl*> eval_list;
    {
      compiler compiler(eval_ctx);
      compiler.build_eval_list(eval_list);
    }

    // initialize driver
  #ifndef NLIBJIT
    if (0 == (platform::self().cflags() & cflags::disable_jit)) {
      sim_driver_ = new simjit::driver();
    } else {
      sim_driver_ = new simref::driver();
    }
  #else
    sim_driver_ = new simref::driver();
  #endif
    sim_driver_->acquire();
    sim_driver_->initialize(eval_list);

    eval_ctx->release();
  }

  // bind system signals
  for (auto ctx : contexts_) {
    auto clk = ctx->sys_clk();
    if (clk) {
      clk_driver_.add_signal(clk);
    }

    auto reset = ctx->sys_reset();
    if (reset) {
      reset_driver_.add_signal(reset);
    }
  }
}

void simulatorimpl::eval() {
  sim_driver_->eval();
}

ch_tick simulatorimpl::reset(ch_tick t) {
  // reset signal
  if (!reset_driver_.empty()) {
    reset_driver_.eval();
    t = this->step(t);
    reset_driver_.eval();
  }
  return t;
}

ch_tick simulatorimpl::step(ch_tick t) {
  if (!clk_driver_.empty()) {
    clk_driver_.eval();
    this->eval();
    clk_driver_.eval();
    ++t;
  }
  this->eval();
  ++t;
  return t;
}

ch_tick simulatorimpl::step(ch_tick t, uint32_t count) {
  while (count--) {
    t = this->step(t);
  }
  return t;
}

ch_tick simulatorimpl::run(const std::function<bool(ch_tick t)>& callback) {
  auto t = this->reset(0);
  for (auto start = t; callback(t - start);) {
    t = this->step(t);
  }
  return t;
}

void simulatorimpl::run(ch_tick num_ticks) {
  for (auto t = this->reset(0); t < num_ticks;) {
    t = this->step(t);
  }
}

///////////////////////////////////////////////////////////////////////////////

ch_simulator::ch_simulator() : impl_(nullptr) {}

ch_simulator::ch_simulator(const ch_device_list& devices) {
  impl_ = new simulatorimpl(devices);
  impl_->acquire();
}

ch_simulator::ch_simulator(simulatorimpl* impl) : impl_(impl) {
  if (impl) {
    impl->acquire();
  }
}

ch_simulator::ch_simulator(const ch_simulator& other)
  : impl_(other.impl_) {
  if (impl_) {
    impl_->acquire();
  }
}

ch_simulator::ch_simulator(ch_simulator&& other)
  : impl_(std::move(other.impl_)) {
  other.impl_ = nullptr;
}

ch_simulator::~ch_simulator() {
  if (impl_) {
    impl_->release();
  }
}

ch_simulator& ch_simulator::operator=(const ch_simulator& other) {
  if (other.impl_) {
    other.impl_->acquire();
  }
  if (impl_) {
    impl_->release();
  }
  impl_ = other.impl_;
  return *this;
}

ch_simulator& ch_simulator::operator=(ch_simulator&& other) {
  impl_ = std::move(other.impl_);
  other.impl_ = nullptr;
  return *this;
}

ch_tick ch_simulator::run(const std::function<bool(ch_tick t)>& callback) {
  return impl_->run(callback);
}

void ch_simulator::run(ch_tick num_ticks) {
  impl_->run(num_ticks);
}

ch_tick ch_simulator::reset(ch_tick t) {
  return impl_->reset(t);
}

ch_tick ch_simulator::step(ch_tick t, uint32_t count) {
  return impl_->step(t, count);
}

void ch_simulator::eval() {
  impl_->eval();
}

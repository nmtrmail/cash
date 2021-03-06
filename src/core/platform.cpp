#include "platform.h"

using namespace ch::internal;

class platform::Impl {
  int dbg_level_;
  int dbg_node_;
  int cflags_;

  Impl()
    : dbg_level_(0)
    , dbg_node_(0)
    , cflags_(0) {

    auto dbg_level = std::getenv("CASH_DEBUG_LEVEL");
    if (dbg_level) {
      dbg_level_ = atoi(dbg_level);
    }

    auto dbg_node = std::getenv("CASH_DEBUG_NODE");
    if (dbg_node) {
      dbg_node_ = atoi(dbg_node);
    }

    auto ch_flags = std::getenv("CASH_CFLAGS");
    if (ch_flags) {
      cflags_ = atoi(ch_flags);
    }
  }

  friend class platform;
};

platform::platform() {
  impl_ = new Impl();
}

platform::~platform() {
  delete impl_;
}

int platform::dbg_level() const {
  return impl_->dbg_level_;
}

int platform::dbg_node() const {
  return impl_->dbg_node_;
}

ch::internal::ch_flags platform::cflags() const {
  return ch::internal::ch_flags(impl_->cflags_);
}

void platform::set_cflags(ch::internal::ch_flags value) {
  impl_->cflags_ = static_cast<int>(value);
}

platform& platform::self() {
  static platform s_instance;
  return s_instance;
}

///////////////////////////////////////////////////////////////////////////////

void ch::internal::ch_setflags(ch_flags flags) {
  return platform::self().set_cflags(flags);
}

ch_flags ch::internal::ch_getflags() {
  return platform::self().cflags();
}
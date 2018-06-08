#include "lnodeimpl.h"
#include "proxyimpl.h"
#include "context.h"

using namespace ch::internal;

const char* ch::internal::to_string(lnodetype type) {
  static const char* sc_names[] = {
    CH_LNODE_ENUM(CH_LNODE_NAME)
  };
  return sc_names[CH_LNODE_INDEX(type)];
}

lnodeimpl::lnodeimpl(context* ctx,
                     lnodetype type,
                     uint32_t size,
                     uint32_t var_id,
                     const std::string& name,
                     const source_location& sloc)
  : ctx_(ctx)
  , id_(ctx->node_id())
  , type_(type)
  , value_(size)
  , var_id_(var_id)
  , name_(to_string(type))
  , sloc_(sloc) {
  if (!name.empty()) {
    name_ = name;
  }
}

lnodeimpl::~lnodeimpl() {}

uint32_t lnodeimpl::add_src(uint32_t index, const lnode& src) {
  if (0xffffffff == index) {
    // add first entry
    index = srcs_.size();
    srcs_.push_back(src);
  } else {
    // override existing entry
    srcs_[index] = src;
  }
  return index;
}

void lnodeimpl::update_sloc(const source_location& sloc) {
  assert(!sloc.empty());
  if (sloc_.empty()) {
    sloc_ = sloc;
  }
}

bool lnodeimpl::equals(const lnodeimpl& rhs) const {
  if (this->type() == rhs.type()
   && this->size() == rhs.size()
   && this->srcs().size() == rhs.srcs().size()) {
    for (unsigned i = 0, n = this->srcs().size(); i < n; ++i) {
      if (this->src(i).id() != rhs.src(i).id())
        return false;
    }
    return true;
  }
  return false;
}

lnodeimpl* lnodeimpl::slice(uint32_t offset, uint32_t length) {
  assert(length <= value_.size());
  if (value_.size() == length)
    return this;
  return ctx_->create_node<proxyimpl>(this, offset, length);
}

void lnodeimpl::print(std::ostream& out, uint32_t level) const {
  out << "#" << id_ << " <- " << this->type() << value_.size();
  uint32_t n = srcs_.size();
  if (n > 0) {
    out << "(";
    for (uint32_t i = 0; i < n; ++i) {
      if (i > 0)
        out << ", ";
      out << "#" << srcs_[i].id();
    }
    out << ")";
  }
  if (level == 2) {
    out << " = " << value_;
  }
}

///////////////////////////////////////////////////////////////////////////////

undefimpl::undefimpl(context* ctx, uint32_t size)
  : lnodeimpl(ctx, type_undef, size)
{}

void undefimpl::eval() {
  CH_ABORT("undefined node: %s!", name_.c_str());
}

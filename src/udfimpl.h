#pragma once

#include "lnodeimpl.h"
#include "udf.h"

namespace ch {
namespace internal {

class udfimpl : public lnodeimpl {
public:

  udf_iface* udf() const {
    return udf_;
  }  

protected:

  udfimpl(context* ctx,
          lnodetype type,
          udf_iface* udf,
          const std::vector<lnode>& srcs,
          const source_location& sloc);

  ~udfimpl();

  udf_iface* udf_;

  friend class context;
};

///////////////////////////////////////////////////////////////////////////////

class udfcimpl : public udfimpl {
public:

  virtual lnodeimpl* clone(context* ctx, const clone_map& cloned_nodes) override;

protected:

  udfcimpl(context* ctx,
          udf_iface* udf,
          const std::vector<lnode>& srcs,
          const source_location& sloc);

  friend class context;
};

///////////////////////////////////////////////////////////////////////////////

class udfsimpl : public udfimpl {
public:

  const lnode& cd() const {
    return srcs_[cd_idx_];
  }

  bool has_init_data() const {
    return (reset_idx_ != -1);
  }

  const lnode& reset() const {
    return srcs_[reset_idx_];
  }

  virtual lnodeimpl* clone(context* ctx, const clone_map& cloned_nodes) override;

protected:

  udfsimpl(context* ctx,
           udf_iface* udf,
           const std::vector<lnode>& srcs,
           lnodeimpl* cd,
           lnodeimpl* reset,
           const source_location& sloc);

  int cd_idx_;
  int reset_idx_;

  friend class context;
};

}}

#pragma once

#include "ioimpl.h"

namespace ch {
namespace internal {

class timeimpl : public ioimpl {
public:

  void reset() override;

  void eval() override;
  
protected:

  timeimpl(context* ctx);

  ~timeimpl() {}

  ch_tick tick_;

  friend class context;
};

}
}
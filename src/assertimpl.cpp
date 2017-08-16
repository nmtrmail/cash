#include "assertimpl.h"
#include "assertion.h"
#include "context.h"

using namespace std;
using namespace cash::detail;

assertimpl::assertimpl(lnodeimpl* src, const std::string& msg)
  : ioimpl(op_assert, src->get_ctx(), 1)
  , msg_(msg) {
  srcs_.emplace_back(src);
}

const bitvector& assertimpl::eval(ch_cycle t) {
  const bitvector& bits = srcs_[0].eval(t);
  CH_CHECK(bits[0], "assertion failure at cycle %ld, %s", t, msg_.c_str());  
  return bits;
}

void cash::detail::ch_assert(const ch_bitbase<1>& l, const std::string& msg) {
  new assertimpl(get_node(l).get_impl(), msg);
}
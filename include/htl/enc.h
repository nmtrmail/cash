#pragma once

#include <cash.h>

namespace ch {
namespace htl {

  template <unsigned N>
  auto ch_pri_enc(const ch_bit<N>& input, CH_SLOC) {
    static_assert(N >= 2, "invalid size");
    auto sel = ch_sel<ch_bit<log2ceil(N)>>(input[N-1], N-1, sloc);
    for (int i = N-2; i > 0; --i) {
      sel(input[i], i);
    }
    return sel(0);
  }
}
}
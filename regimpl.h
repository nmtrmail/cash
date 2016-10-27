#pragma once

#include "lnodeimpl.h"
#include "tickable.h"
#include "cdomain.h"

namespace chdl_internal {

class regimpl : public tickable, public lnodeimpl {
public:
  regimpl(lnodeimpl* next);
  virtual ~regimpl();
  
  const bitvector& eval(ch_cycle t) override;  
  void print_vl(std::ostream& out) const override;

  void tick(ch_cycle t) override;
  void tick_next(ch_cycle t) override;
  
protected:

  cdomain*  m_cd;
  bitvector m_q;
  bitvector m_next_q;
  ch_cycle  m_ctime;
};

class latchimpl : public tickable, public lnodeimpl {
public:
  latchimpl(
      lnodeimpl* next,
      lnodeimpl* init,
      lnodeimpl* enable,      
      lnodeimpl* reset
  );
  virtual ~latchimpl();
  
  const bitvector& eval(ch_cycle t) override;  
  void print_vl(std::ostream& out) const override;

  void tick(ch_cycle t) override;
  void tick_next(ch_cycle t) override;
  
protected:

  cdomain*  m_cd;
  bitvector m_q;
  ch_cycle  m_ctime;
};

}

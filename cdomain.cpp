#include "cdomain.h"
#include "ioimpl.h"
#include "litimpl.h"
#include "context.h"

using namespace std;
using namespace chdl_internal;

clock_event::clock_event(lnodeimpl* signal, EDGE_DIR edgedir)
  : m_signal(signal)
  , m_edgedir(edgedir)
  , m_cval(false) 
{}

clock_event::~clock_event() {
  //--
}

bool clock_event::eval(ch_cycle t) {
  bool value = m_signal.eval(t)[0];
  if (m_cval != value) {
    m_cval = value;
    return (value  && (m_edgedir == EDGE_POS || m_edgedir == EDGE_ANY)) ||
           (!value && (m_edgedir == EDGE_NEG || m_edgedir == EDGE_ANY));
  }
  return false;
}

// LCOV_EXCL_START
void clock_event::print_vl(ostream& out) const {
  switch (m_edgedir) {
  case EDGE_POS:
    out << "posedge ";
    break;
  case EDGE_NEG:
    out << "negedge ";
    break;
  }
  out << "__x" << m_signal.get_id();
}
// LCOV_EXCL_END

///////////////////////////////////////////////////////////////////////////////

cdomain::cdomain(context* ctx, const std::vector<clock_event>& sensitivity_list)
  : m_ctx(ctx) {
  m_sensitivity_list.reserve(sensitivity_list.size());
  for (const clock_event& e : sensitivity_list) {
    assert(dynamic_cast<undefimpl*>(e.get_signal()) == nullptr);
    // constants are omitted by default
    if (dynamic_cast<litimpl*>(e.get_signal())) {
      continue;
    }
    // ensure unique signals
    auto it = std::find(m_sensitivity_list.begin(), m_sensitivity_list.end(), e.get_signal());
    CHDL_CHECK(it == m_sensitivity_list.end(), "a duplicate signal event provided");    
    m_sensitivity_list.emplace_back(e);
  }
} 

cdomain::~cdomain() {
  m_ctx->remove_cdomain(this);
}

void cdomain::add_use(tickable* reg) {
  m_regs.emplace_back(reg);
  this->acquire();
}

void cdomain::remove_use(tickable* reg) {
  m_regs.remove(reg);
  this->release();
}

bool cdomain::operator==(const std::vector<clock_event>& events) const {
  if (events.size() != m_sensitivity_list.size())
    return false;
  for (const clock_event& e1 : m_sensitivity_list) {
    bool found = false;
    for (const clock_event& e2 : events) {
      if (e1 == e2) {
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  }
  return true;
}

void cdomain::tick(ch_cycle t) {
  for (clock_event& event : m_sensitivity_list) {
    if (event.eval(t)) {
      for (tickable* reg : m_regs)
        reg->tick(t);
      return;
    }
  }
}

void cdomain::tick_next(ch_cycle t) {
  for (auto reg : m_regs) {
    reg->tick_next(t);
  }
}

// LCOV_EXCL_START
void cdomain::print_vl(ostream& out) const {
  bool first_event = true;
  for (const clock_event& event : m_sensitivity_list) {
    if (first_event)
      first_event = false;
    else
      out << ", ";
    event.print_vl(out);
  }
}
// LCOV_EXCL_STOP

﻿#include "context.h"
#include <thread>
#include "lnode.h"
#include "litimpl.h"
#include "regimpl.h"
#include "memimpl.h"
#include "ioimpl.h"
#include "bindimpl.h"
#include "selectimpl.h"
#include "proxyimpl.h"
#include "memimpl.h"
#include "aluimpl.h"
#include "assertimpl.h"
#include "timeimpl.h"
#include "cdimpl.h"
#include "ioport.h"
#include "traits.h"
#include "select.h"
#include "enum.h"
#include "udf.h"
#include "udfimpl.h"

namespace ch {
namespace internal {

typedef std::unordered_map<std::type_index, udf_iface*> udfs_t;

class context_manager {
public:
  context_manager() : ctx_(nullptr), ctx_ids_(0), node_ids_(0) {}

  ~context_manager() {
    if (ctx_) {
      ctx_->release();
    }
    for (auto item : udfs_) {
      item.second->release();
    }
  }

  context* create(const std::type_index& signature, const std::string& name) {
    std::string unique_name(name);
    auto it = module_names_.find(signature);
    if (it != module_names_.end()) {
      unique_name = it->second;
    } else {
      unique_name = unique_names_.get(unique_name.c_str());
      module_names_[signature] = unique_name;
    }
    return new context(unique_name.c_str());
  }

  context* ctx() const {
    CH_CHECK(ctx_ != nullptr, "invalid context!");
    return ctx_;
  }

  context* swap(context* ctx) {
    std::swap(ctx_, ctx);
    return ctx;
  }

  udf_iface* lookup_udf(const std::type_index& signature) {
    auto it = udfs_.find(signature);
    if (it != udfs_.end()) {
      return it->second;
    }
    return nullptr;
  }

  udf_iface* register_udf(const std::type_index& signature, udf_iface* udf) {
    assert(0 == udfs_.count(signature));
    udfs_[signature] = udf;
    udf->acquire();
    return udf;
  }

  static context_manager& instance(){
    static context_manager inst;
    return inst;
  }

  uint32_t ctx_id() const {
    return ++ctx_ids_;
  }

  uint32_t node_id() const {
    return ++node_ids_;
  }

protected:

  mutable context* ctx_;
  mutable uint32_t ctx_ids_;
  mutable uint32_t node_ids_;
  std::unordered_map<std::type_index, std::string> module_names_;
  unique_names unique_names_;
  udfs_t udfs_;  
};

}
}

using namespace ch::internal;

context* ch::internal::ctx_create(const std::type_index& signature, const std::string& name) {
  return context_manager::instance().create(signature, name);
}

context* ch::internal::ctx_swap(context* ctx) {
  return context_manager::instance().swap(ctx);
}

context* ch::internal::ctx_curr() {
  return context_manager::instance().ctx();
}

udf_iface* ch::internal::lookupUDF(const std::type_index& signature) {
  return context_manager::instance().lookup_udf(signature);
}

udf_iface* ch::internal::registerUDF(const std::type_index& signature, udf_iface* udf) {
  return context_manager::instance().register_udf(signature, udf);
}

///////////////////////////////////////////////////////////////////////////////

context::context(const std::string& name)
  : id_(context_manager::instance().ctx_id())
  , name_(name)
  , block_idx_(0)
  , default_clk_(nullptr)
  , default_reset_(nullptr)
  , time_(nullptr)
{}

context::~context() {
  // delete allocated nodes
  for (auto node : nodes_) {
    node->release();
  }
}

void context::push_cd(const lnode& clk,
                      const lnode& reset,
                      bool posedge,
                      const source_location& sloc) {
  auto cd = this->create_cd(clk, posedge, sloc);
  cd_stack_.emplace(cd, reset);
}

void context::pop_cd() {
  cd_stack_.pop();
}

cdimpl* context::current_cd(const source_location& sloc) {
  if (!cd_stack_.empty())
    return cd_stack_.top().first;

  if (nullptr == default_clk_) {
    default_clk_ = this->create_node<inputimpl>(1, "clk", sloc);
  }
  return this->create_cd(default_clk_, true, sloc);
}

lnode context::current_reset(const source_location& sloc) {
  if (!cd_stack_.empty())
    return cd_stack_.top().second;

  if (nullptr == default_reset_) {
     default_reset_ = this->create_node<inputimpl>(1, "reset", sloc);
  }
  return default_reset_;
}

lnodeimpl* context::time(const source_location& sloc) {
  if (nullptr == time_) {
    time_ = this->create_node<timeimpl>(sloc);
  }
  return time_;
}

uint32_t context::node_id() {
  auto nodeid = context_manager::instance().node_id();
#ifndef NDEBUG
  uint32_t dbg_nodeid = platform::self().dbg_node();
  if (dbg_nodeid) {
    if (nodeid == dbg_nodeid) {
      CH_ABORT("debugbreak on nodeid %d hit!", nodeid);
    }
  }
#endif
  return nodeid;
}

cdimpl* context::create_cd(const lnode& clk,
                           bool posedge,
                           const source_location& sloc) {
  // return existing match
  for (auto cd : cdomains_) {
    if (cd->clk() == clk && cd->posedge() == posedge)
      return cd;
  }
  // allocate new cdomain
  return this->create_node<cdimpl>(clk, posedge, sloc);
}

void context::add_node(lnodeimpl* node) {
  // add node to main list
  nodes_.emplace_back(node);
  node->acquire();

  // add node to special containers
  auto type = node->type();
  switch (type) {
  case type_undef:
    undefs_.emplace_back((undefimpl*)node);
    break;
  case type_proxy:
    proxies_.emplace_back((proxyimpl*)node);
    break;
  case type_lit:
    literals_.emplace_back((litimpl*)node);
    break;
  case type_reg:
  case type_mem:
  case type_udfs:
    snodes_.emplace_back(node);
    break;
  case type_input:
    inputs_.emplace_back((inputimpl*)node);
    break;  
  case type_output:
    outputs_.emplace_back((outputimpl*)node);
    break; 
  case type_cd:
    cdomains_.emplace_back((cdimpl*)node);
    break;
  case type_bind:
    bindings_.emplace_back((bindimpl*)node);
    break;
  case type_tap:
    taps_.emplace_back((tapimpl*)node);
    break;
  case type_assert:
  case type_print:
    gtaps_.emplace_back((ioimpl*)node);
    break;
  default:
    break;
  }

  // register local nodes, io objects & literals have global scope
  if (!cond_branches_.empty()
   && nullptr == dynamic_cast<ioimpl*>(node)
   && nullptr == dynamic_cast<litimpl*>(node)) {
    auto curr_branch = cond_branches_.top();
    auto curr_block = curr_branch->blocks.back();
    cond_inits_[node->id()] = curr_block;
  }  
}

node_list_t::iterator context::delete_node(const node_list_t::iterator& it) {
  auto node = *it;
  DBG(3, "*** deleting node: %s%d(#%d)\n", to_string(node->type()), node->size(), node->id());
  
  switch (node->type()) {
  case type_undef:
    undefs_.remove((undefimpl*)node);
    break;
  case type_proxy:
    proxies_.remove((proxyimpl*)node);
    break;
  case type_lit:
    literals_.remove((litimpl*)node);
    break;
  case type_reg:
  case type_mem:
  case type_udfs:
    snodes_.remove(node);
    break;
  case type_input:
    if (node == default_clk_) {
      default_clk_ = nullptr;
    } else
    if (node == default_reset_) {
      default_reset_ = nullptr;
    }
    inputs_.remove((inputimpl*)node);
    break;
  case type_output:
    outputs_.remove((outputimpl*)node);
    break;
  case type_cd:
    cdomains_.remove((cdimpl*)node);
    break;
  case type_bind:
    bindings_.remove((bindimpl*)node);
    break;
  case type_tap:
    taps_.remove((tapimpl*)node);
    break;
  case type_assert:
  case type_print:
    gtaps_.remove((ioimpl*)node);
    break;
  case type_time:
    if (node == time_) {
      time_ = nullptr;
    }
    break;
  default:
    break;
  }

  // remove node from main list
  node->release();
  return nodes_.erase(it);
}

void context::begin_branch(lnodeimpl* key, const source_location& sloc) {
  // create new conditional branch
  // add to current block and push on the stack
  cond_br_t* new_branch;
  if (!cond_branches_.empty()) {
    auto curr_branch = cond_branches_.top();
    auto curr_block = curr_branch->blocks.back();
    new_branch = new cond_br_t(key, curr_block, sloc);
    curr_block->branches.push_back(new_branch);
  } else {
    new_branch = new cond_br_t(key, nullptr, sloc);
  }
  cond_branches_.push(new_branch);
}

void context::end_branch() {
  // branch delete visitor
  std::function<void(cond_br_t*)> delete_branch = [&](cond_br_t* branch) {
    for (auto bk : branch->blocks) {
      for (auto br : bk->branches) {
        delete_branch(br);
      }
    }
    delete branch;
  };

  // remove current conditional branch from stack
  // process top parent branch on exit
  assert(!cond_branches_.empty());
  auto curr_branch = cond_branches_.top();
  cond_branches_.pop();
  if (cond_branches_.empty()) {
    // emit conditional statements
    for (auto& var : cond_vars_) {      
      auto dst = var.first;
      assert(type_proxy == dst->type());
      for (auto& slice : var.second) {
        auto value  = this->emit_conditionals(dst, slice.first, slice.second, curr_branch);
        reinterpret_cast<proxyimpl*>(dst)->add_source(slice.first.offset, value, 0, slice.first.length);
      }
    }
    // cleanup
    delete_branch(curr_branch);
    cond_vars_.clear();
    cond_inits_.clear();
    block_idx_ = 0;
  }
}

void context::begin_block(lnodeimpl* pred) {
  // insert new conditional block  
  auto curr_branch = cond_branches_.top();
  auto new_block = new cond_block_t(++block_idx_, pred, curr_branch);
  curr_branch->blocks.push_back(new_block);
}

void context::end_block() {
  //--
}

void context::remove_local_variable(lnodeimpl* src, lnodeimpl* dst) {
  // remove from source init block
  auto it = cond_inits_.find(src->id());
  if (it != cond_inits_.end()) {
    cond_inits_.erase(it);
    if (dst) {
      // add to destination's init block
      auto it = cond_inits_.find(dst->id());
      if (it != cond_inits_.end()) {
        cond_inits_[src->id()] = it->second;
      }
    }
  }
}

bool context::conditional_enabled(lnodeimpl* node) const {
  // a node is conditionally assigned if it is not local to the current block
  if (cond_branches_.empty())
    return false;
  if (node) {
    auto it = cond_inits_.find(node->id());
    if (it != cond_inits_.end()) {
      auto curr_branch = cond_branches_.top();
      auto curr_block  = curr_branch->blocks.back();
      if (it->second == curr_block)
        return false;
    }
  }
  return true;
}

void context::conditional_assign(
    lnodeimpl* dst,
    uint32_t offset,
    uint32_t length,
    lnodeimpl* src,
    const source_location& sloc) {
  assert(this->conditional_enabled(dst));
  auto& slices     = cond_vars_[dst];
  auto curr_branch = cond_branches_.top();
  auto curr_block  = curr_branch->blocks.back();

  //--
  auto add_definition = [&](cond_slices_t& slices,
                            const cond_range_t& range,
                            uint32_t loc,
                            lnodeimpl* src,
                            uint32_t src_offset) {
    if (src->size() != range.length) {
      src = this->create_node<proxyimpl>(src, src_offset, range.length, sloc);
    }
    slices[range][loc] = src;
  };

  {
    //--
    cond_slices_t split_list;

    //--
    auto create_split_definitions = [&](const cond_range_t& range,
                                     uint32_t offset,
                                     uint32_t length) {
      cond_range_t new_range{offset, length};
      for (auto def : slices[range]) {
        add_definition(split_list, new_range, def.first, def.second, offset - range.offset);
      }
    };

    // create split list for partially overlapping definitions
    for (auto it = slices.begin(), end = slices.end(); it != end;) {
      const auto& curr  = it->first;
      uint32_t curr_end = curr.offset + curr.length;
      uint32_t var_end  = offset + length;

      // does variable overlap existing definition?
      if (offset < curr_end && var_end > curr.offset) {
        if (offset <= curr.offset && var_end >= curr_end) {
          // variable fully overlaps current
          ++it;
          continue;
        } else if (offset < curr.offset) {
          // variable overlaps on the left
          create_split_definitions(curr, curr.offset, var_end - curr.offset);
          create_split_definitions(curr, var_end, curr_end - var_end);
        } else if (var_end > curr_end) {
          // variable overlaps on the right
          create_split_definitions(curr, curr.offset, offset - curr.offset);
          create_split_definitions(curr, offset, curr_end - offset);
        } else {
          // variable fully included
          if (offset != curr.offset) {
            create_split_definitions(curr, curr.offset, offset - curr.offset);
          }
          create_split_definitions(curr, offset, var_end - offset);
          if (var_end != curr_end) {
            create_split_definitions(curr, var_end, curr_end - var_end);
          }
        }
        // remove entry
        it = slices.erase(it);
      } else {
        ++it;
      }
    }

    // insert split entries
    if (!split_list.empty()) {
      for (auto& item : split_list) {
        slices[item.first] = item.second;
      }
    }
  }

  //--
  std::function<void (cond_defs_t&, const cond_block_t*)>
      delete_definitions = [&](cond_defs_t& defs, const cond_block_t* block) {
    // delete definition in current block
    auto it = defs.find(block->id);
    if (it != defs.end()) {
      defs.erase(it);
    }
    if(defs.empty())
      return;
    // recurse delete definitions in nested blocks
    for (auto branch : block->branches) {
      for (auto block : branch->blocks) {
        delete_definitions(defs, block);
      }
    }
  };

  //--
  uint32_t var_offset = offset;

  //--
  auto add_new_definition = [&](uint32_t offset, uint32_t length) {
    cond_range_t new_range{offset, length};

    // delete all existing definitions within current block
    delete_definitions(slices[new_range], curr_block);

    // add new definition
    add_definition(slices, new_range, curr_block->id, src, offset - var_offset);
  };

  // add new definitions to current block
  for (auto it = slices.begin(), end = slices.end();
       length != 0 && it != end; ++it) {
    const auto& curr = it->first;
    uint32_t curr_end = curr.offset + curr.length;
    uint32_t var_end  = offset + length;

    // does variable overlap existing definition?
    if (offset < curr_end && var_end > curr.offset) {
      if (offset == curr.offset
       && var_end == curr_end) {
        // variable strictly overlaps current definition
        add_new_definition(offset, length);
        length = 0; // no need to continue
      } else if (offset < curr.offset) {
        // variable overlaps current definition on the left
        add_new_definition(offset, curr.offset - offset);
        add_new_definition(curr.offset, curr.length);
        uint32_t delta = (curr.offset - offset) + curr.length;
        offset += delta;
        length -= delta;
      } else if (var_end > curr_end) {
        // variable overlaps current definition on the right
        add_new_definition(curr.offset, curr.length);
        offset += curr.length;
        length -= curr.length;
      } else {
        // splitting should have eliminated this case
        assert(false);
      }
    } else if (std::next(it) == end
            || var_end <= curr.offset) {
      // no overlap with current and next
      add_new_definition(offset, length);
      length = 0; // no need to continue
    } else {
      // definitions no not overlap
      continue;
    }
  }

  if (length != 0) {
    add_new_definition(offset, length);
  }
}

lnodeimpl*
context::emit_conditionals(lnodeimpl* dst,
                           const cond_range_t& range,
                           const cond_defs_t& defs,
                           const cond_br_t* branch) {
  //--
  std::function<lnodeimpl*(const cond_br_t* branch,
                           lnodeimpl* current)> emit_conditional_branch;

  //--
  typedef std::list<std::pair<lnodeimpl*, lnodeimpl*>> branch_info_t;

  //--
  auto branch_optimizer = [&](lnodeimpl* key,
                              branch_info_t& values,
                              lnodeimpl* current)->lnodeimpl* {
    // ensure default argument
    if (values.back().first) {
       values.emplace_back(nullptr, current);
    }

    {
      // skip paths with value equal to default value
      auto df_value = values.back().second;
      for (auto it = values.begin(), end = values.end(); it != end;) {
        if (it->first && it->second == df_value) {
          it = values.erase(it);
        } else {
          ++it;
        }
      }
    }

    // return single assignment as default value
    if (1 ==  values.size()) {
      assert(nullptr == values.front().first);
      return values.front().second;
    }

    // coallesce cascading ternary branches sharing the same default value
    // p2 ? (p1 ? t1 : f1) : f1 => (p1 & p2) ? t1 : f1;
    if (2 == values.size()) {
      auto _true = dynamic_cast<selectimpl*>(values.front().second);
      if (_true) {
        uint32_t num_srcs = _true->has_key() ? 4 : 3;
        if (_true->num_srcs() == num_srcs
         && _true->src(num_srcs-1) == values.back().second) {
          // combine predicates
          auto pred0 = values.front().first;
          if (key) {
            pred0 = this->create_node<aluimpl>(ch_op::eq, 1, false, key, pred0, branch->sloc);
          }
          auto pred1 = _true->src(0);
          if (_true->has_key()) {
            // create predicate
            pred1 = this->create_node<aluimpl>(ch_op::eq, 1, false, pred1, _true->src(1), branch->sloc);
            // remove key from src list
            _true->remove_key();
          }
          auto pred = this->create_node<aluimpl>(ch_op::andl, 1, false, pred0, pred1, branch->sloc);
          _true->src(0) = pred;
          return _true;
        }
      }
    }

    return nullptr;
  };

  //--
  auto emit_conditional_block = [&](const cond_block_t* block,
                                    lnodeimpl* current)->lnodeimpl* {
    // get existing definition in current block
    auto it = defs.find(block->id);
    if (it != defs.end()) {
      current = it->second;
    }
    // recurse emit conditionals from nested blocks
    for (auto br : block->branches) {
      current = emit_conditional_branch(br, current);
    }
    return current;
  };

  //--
  emit_conditional_branch = [&](const cond_br_t* branch,
                                lnodeimpl* current)->lnodeimpl* {
    uint32_t changed = 0;
    branch_info_t values;

    for (auto block : branch->blocks) {
      // get definition
      auto value = emit_conditional_block(block, current);
      values.emplace_back(block->pred, value);
      changed |= (value != current);
    }

    if (changed) {
      // optimize the branch
      auto ret = branch_optimizer(branch->key, values, current);
      if (ret)
        return ret;

      // create select node
      auto sel = this->create_node<selectimpl>(
                          current->size(), branch->key, branch->sloc);
      for (auto& value : values) {
        auto pred = value.first;
        if (pred) {
          // the case predicate should be a scalar value
          assert(!branch->key || type_lit == pred->type());
          sel->srcs().push_back(pred);
        }
        sel->srcs().push_back(value.second);
      }

      return sel;
    }

    return current;
  };

  // get current variable value
  auto current = dst->slice(range.offset, range.length, branch->sloc);

  // emit conditional variable
  auto it = cond_inits_.find(dst->id());
  if (it != cond_inits_.end()) {
    current = emit_conditional_block(it->second, current);
  } else {    
    current = emit_conditional_branch(branch, current);
  }

  return current;
}

lnodeimpl* context::create_predicate(const source_location& sloc) {
  auto zero = this->literal(bitvector(1, false));
  auto one = this->literal(bitvector(1, true));

  // create predicate variable
  auto predicate = this->create_node<proxyimpl>(zero, 0, 1, sloc);
  this->remove_local_variable(predicate, nullptr);

  // assign predicate
  this->conditional_assign(predicate, 0, 1, one, sloc);

  return predicate;
}

lnodeimpl* context::literal(const bitvector& value) {
  // first lookup literals cache
  for (auto lit : literals_) {
    if (lit->data() == value)
      return lit;
  }
  // create new literal
  return this->create_node<litimpl>(value);
}

bindimpl* context::find_binding(context* module, const source_location& sloc) {
  for (auto binding : bindings_) {
    if (binding->module() == module)
      return binding;
  }
  return this->create_node<bindimpl>(module, sloc);
}

void context::register_tap(const lnode& node,
                           const std::string& name,
                           const source_location& sloc) {
  this->create_node<tapimpl>(node, unique_tap_names_.get(name).c_str(), sloc);
}

void context::build_run_list(std::vector<lnodeimpl*>& runlist) {
  std::unordered_set<uint32_t> visited, cycles;
  std::unordered_set<lnodeimpl*> update_list;
  std::vector<lnodeimpl*> snodes;
  std::vector<lnodeimpl*> taps;
  std::unordered_set<lnodeimpl*> uninitialized_regs;

  std::function<void (context*)> gather_snodes = [&](context* ctx) {
    for (auto node : ctx->bindings()) {
      gather_snodes(node->module());
    }
    auto& list = ctx->snodes();
    snodes.reserve(snodes.size() + list.size());
    snodes.insert(snodes.end(), list.begin(), list.end());
  };

  std::function<void (context*)> gather_taps = [&](context* ctx) {
    for (auto node : ctx->bindings()) {
      gather_taps(node->module());
    }
    for (auto node : ctx->taps()) {
      taps.push_back(node);
    }
    for (auto node : ctx->gtaps()) {
      taps.push_back(node);
    }
  };

  std::function<bool (lnodeimpl*)> dfs_visit = [&](lnodeimpl* node)->bool {
    if (visited.count(node->id())) {
      // if a node depends on an update node, it also needs to be updated.
      return (update_list.count(node) != 0);
    }

    // check for cycles
    if (cycles.count(node->id())) {
      // handling register cycles
      switch (node->type()) {
      case type_reg:
        // Detect uninitialized registers
        if (!reinterpret_cast<regimpl*>(node)->has_init())
          uninitialized_regs.insert(node);
        [[fallthrough]];
      case type_mem:
      case type_udfs:
        // we found a cycle, return 'true'
        return true;
      default:
        break;
      }
#define LCOV_EXCL_START
      if (platform::self().cflags() & cflags::dump_ast) {
        for (auto n : runlist) {
          std::cerr << n->ctx()->id() << ": ";
          n->print(std::cerr, platform::self().dbg_level());
          std::cerr << std::endl;
        }
      }
      fprintf(stderr, "error: found cycle on variable %s\n", node->debug_info().c_str());
      std::abort();      
      return false;
#define LCOV_EXCL_END
    }
    cycles.insert(node->id());

    bool update = false;

    // handling for special nodes
    switch (node->type()) {
    case type_bind:
      visited.insert(node->id());
      return false; // no following
    case type_input: {
      auto input = reinterpret_cast<inputimpl*>(node);
      if (!input->input().empty()) {
        update |= dfs_visit(input->input().impl());
      }
    } break;
    case type_bindout: {
      auto port = reinterpret_cast<bindportimpl*>(node);
      update |= dfs_visit(port->ioport().impl());
    } break;
    default:
      break;
    }

    // visit source nodes
    for (auto& src : node->srcs()) {
      update |= dfs_visit(src.impl());
    }

    if (update) {
      // a cycle exists in dependent path, this node should be updated
      update_list.insert(node);
    }

    runlist.push_back(node);
    visited.insert(node->id());

    return update;
  };

  DBG(2, "run list evaluation for %s (#%d) ...\n", this->name().c_str(), this->id());

  // gather sequential nodes from all contexts
  gather_snodes(this);

  // enable cycle detection for sequential nodes
  for (auto node : snodes) {
    cycles.insert(node->id());
  }

  // insert sequential nodes dependencies
  for (auto node : snodes) {
    for (auto& src : node->srcs()) {
      dfs_visit(src.impl());
    }
  }

  // make a copy of the update list and empty it
  std::unordered_set<lnodeimpl*> update_list2;
  std::swap(update_list2, update_list);

  // disable cycle detection for sequential nodes
  for (auto node : snodes) {
    cycles.erase(node->id());
  }

  // insert sequential nodes
  auto old_size = runlist.size();
  for (auto node : snodes) {
    dfs_visit(node);
  }
  snodes.clear();

  // sort recently inserted sequential node in reverse dependency order
  std::reverse(runlist.begin() + old_size, runlist.begin() + runlist.size());

  // invalidate all update nodes to force re-insertion
  for (auto node : update_list2) {
    cycles.erase(node->id());
    visited.erase(node->id());
  }
  update_list2.clear();

  // insert output nodes
  for (auto node : outputs_) {
    dfs_visit(node);
  }

  // gather tap nodes accross all contexts
  gather_taps(this);

  // insert tap nodes
  for (auto node : taps) {
    dfs_visit(node);
  }
  taps.clear();

  if (platform::self().cflags() & cflags::dump_ast) {
    for (auto node : runlist) {
      std::cerr << node->ctx()->id() << ": ";
      node->print(std::cerr, platform::self().dbg_level());
      std::cerr << std::endl;
    }
  }

  if (!uninitialized_regs.empty()
   && (platform::self().cflags() & cflags::check_reg_init)) {
    for (auto node : uninitialized_regs) {
      fprintf(stderr, "warning: uninitialized register %s\n",
              node->debug_info().c_str());
    }
  }
}

lnodeimpl* context::create_udf_node(udf_iface* udf,
                                    const std::initializer_list<lnode>& inputs,
                                    const source_location& sloc) {
  if (udf->delta() != 0) {
    return this->create_node<udfsimpl>(udf, inputs, sloc);
  } else {
    return this->create_node<udfcimpl>(udf, inputs, sloc);
  }
}

void context::register_enum_string(const lnode& node, enum_string_cb callback) {
  enum_strings_.emplace(node.var_id(), callback);
}

const char* context::enum_to_string(const lnode& node) {
  auto iter = enum_strings_.find(node.var_id());
  if (iter != enum_strings_.end()) {
    return iter->second(node.data().word(0));
  }
  return "undefined";
}

void context::dump_ast(std::ostream& out, uint32_t level) {
  for (auto node : nodes_) {
    node->print(out, level);
    out << std::endl;
  }
}

void context::dump_cfg(lnodeimpl* node, std::ostream& out, uint32_t level) {
  std::unordered_set<uint32_t> visits;
  std::unordered_map<uint32_t, tapimpl*> taps;

  std::function<void(lnodeimpl*)> dump_cfg_impl = [&](lnodeimpl* node) {
    visits.insert(node->id());
    node->print(out, level);

    auto iter = taps.find(node->id());
    if (iter != taps.end()) {
      out << " // " << iter->second->name();
      out << std::endl;
      return;
    }
    out << std::endl;

    for (auto& src : node->srcs()) {
      if (!visits.count(src.id())) {
        dump_cfg_impl(src.impl());
      }
    }
  };
  
  for (tapimpl* tap : taps_) {
    taps[tap->target().id()] = tap;
  }
  
  visits.insert(node->id());
  
  if (type_tap == node->type()) {
    auto tap = reinterpret_cast<tapimpl*>(node);
    node = tap->target().impl();
    node->print(out, level);
    out << " // " << tap->name();
  } else {  
    node->print(out, level);
  }
  out << std::endl;    
  for (auto& src : node->srcs()) {
    if (!visits.count(src.id())) {
      dump_cfg_impl(src.impl());
    }
  }
}

void context::dump_stats(std::ostream& out) {
  uint64_t num_memories = 0;
  uint64_t num_registers = 0;
  uint64_t num_muxes = 0;
  uint64_t num_alus = 0;
  uint64_t num_lits = 0;

  uint64_t memory_bits = 0;
  uint64_t register_bits = 0;

  std::function<void(context*)> calc_stats = [&](context* ctx) {
    for (lnodeimpl* node : ctx->nodes()) {
      switch (node->type()) {
      case type_mem:
        memory_bits += reinterpret_cast<memimpl*>(node)->total_size();
        ++num_memories;
        break;
      case type_reg:
        register_bits += node->size();
        ++num_registers;
        break;
      case type_sel:
        ++num_muxes;
        break;
      case type_alu:
        ++num_alus;
        break;
      case type_lit:
        ++num_lits;
        break;
      case type_bind:
        calc_stats(reinterpret_cast<bindimpl*>(node)->module());
        break;
      default:
        break;
      }
    }
  };

  calc_stats(this);

  out << "ch-stats: total memory bits = " << memory_bits << std::endl;
  out << "ch-stats: total register bits = " << register_bits << std::endl;
  out << "ch-stats: total memories = " << num_memories << std::endl;
  out << "ch-stats: total registers = " << num_registers << std::endl;
  out << "ch-stats: total muxes = " << num_muxes << std::endl;
  out << "ch-stats: total alus = " << num_alus << std::endl;
  out << "ch-stats: total literals = " << num_lits << std::endl;
}

///////////////////////////////////////////////////////////////////////////////

void ch::internal::registerTap(const lnode& node,
                               const std::string& name,
                               const source_location& sloc) {
  node.impl()->ctx()->register_tap(node, name, sloc);
}

void ch::internal::registerEnumString(const lnode& node, void* callback) {
  node.impl()->ctx()->register_enum_string(node, (enum_string_cb)callback);
}

#include "proxyimpl.h"
#include "context.h"
#include "logic.h"

using namespace ch::internal;

proxyimpl::proxyimpl(context* ctx,
                     uint32_t size,
                     const std::string& name,
                     const source_location& sloc)
  : lnodeimpl(ctx->node_id(), type_proxy, size, ctx, name, sloc)
{}

proxyimpl::proxyimpl(context* ctx,
                     lnodeimpl* src,
                     const std::string& name,
                     const source_location& sloc)
  : lnodeimpl(ctx->node_id(), type_proxy, src->size(), ctx, name, sloc) {
  this->add_source(0, src, 0, src->size());
}

proxyimpl::proxyimpl(context* ctx,
                     lnodeimpl* src,
                     uint32_t offset,
                     uint32_t length,
                     const std::string& name,
                     const source_location& sloc)
  : lnodeimpl(ctx->node_id(), type_proxy, length, ctx, name, sloc) {
  this->add_source(0, src, offset, length);
}

lnodeimpl* proxyimpl::clone(context* ctx, const clone_map& cloned_nodes) const {
  auto node = ctx->create_node<proxyimpl>(this->size(), name_, sloc_);
  for (auto& src : this->srcs()) {
    node->add_src(cloned_nodes.at(src.id()));
  }
  for (auto& range : ranges_) {
    node->ranges().emplace_back(range);
  }
  return node;
}

bool proxyimpl::has_sparse_range() const {
  uint32_t size = 0;
  for (auto& r : ranges_) {
    size += r.length;
  }
  return (size != this->size());
}

void proxyimpl::add_source(uint32_t dst_offset,
                           lnodeimpl* src,
                           uint32_t src_offset,
                           uint32_t length) {
  assert(length != 0);
  assert(dst_offset + length <= this->size());
  assert(src_offset + length <= src->size());

  // update source location
  if (sloc_.empty()) {
    sloc_ = src->sloc();
  }

  // add new source
  auto size = num_srcs();
  uint32_t new_srcidx = size;
  for (uint32_t i = 0, n = size; i < n; ++i) {
    if (this->src(i).id() == src->id()) {
      new_srcidx = i;
      break;
    }
  }

  if (new_srcidx == size) {
    new_srcidx = this->add_src(src);
  }

  uint32_t n = ranges_.size();
  if (0 == n) {
    // insert the first entry
    ranges_.push_back({new_srcidx, dst_offset, src_offset, length});
  } else {
    // find the slot to insert source node,
    // split existing nodes if needed
    std::set<uint32_t> deleted; // use ordered set for index ordering
    uint32_t i = 0;
    for (; length != 0 && i < n; ++i) {
      auto& curr = ranges_[i];
      uint32_t curr_end = curr.dst_offset + curr.length;
      uint32_t src_end  = dst_offset + length;
      range_t new_range{new_srcidx, dst_offset, src_offset, length};
      
      // do ranges overlap?
      if (dst_offset < curr_end && src_end > curr.dst_offset) {
        if (dst_offset <= curr.dst_offset && src_end >= curr_end) {
          // source fully overlaps, replace current node
          uint32_t delta = curr_end - dst_offset;
          new_range.length = delta;
          if (new_range != curr) {
            deleted.emplace(curr.src_idx);
            curr = new_range;
          }
          dst_offset += delta;
          src_offset += delta;
          length -= delta;
        } else if (dst_offset < curr.dst_offset) {
          // source overlaps on the left,
          // replace left size of the current node
          uint32_t delta = src_end - curr.dst_offset;
          curr.dst_offset += delta;
          curr.src_offset += delta;
          curr.length -= delta;

          ranges_.insert(ranges_.begin() + i, new_range);
          ++n;

          length = 0;
        } else if (src_end > curr_end) {
          // source overlaps on the right,
          // replace right size of the current node
          uint32_t delta = curr_end - dst_offset;
          curr.length -= delta;
          
          new_range.length = delta;
          
          ++i; 
          ranges_.insert(ranges_.begin() + i, new_range);
          ++n;       
          
          dst_offset += delta;
          src_offset += delta;
          length -= delta;
        } else {
          // source fully included,
          // we need to split current node into two chunks
          uint32_t delta = src_end - curr.dst_offset;
          range_t curr_after(curr);          
          curr_after.length -= delta;
          curr_after.dst_offset += delta;
          curr_after.src_offset += delta;
          curr.length -= (curr_end - dst_offset);
          assert(curr.length || curr_after.length);

          if (0 == curr.length) {
            // replace first chunk
            curr = new_range;
          } else {
            // insert source after first chunk
            ++i;
            ranges_.insert(ranges_.begin() + i, 1, new_range);
            ++n;
          }
          
          if (curr_after.length) {
            // insert second chunk after source
            ranges_.insert(ranges_.begin() + (i+1), 1, curr_after); 
            ++n;
          }         
          length = 0;
        }
      } else if (i+1 == n || src_end <= ranges_[i+1].dst_offset) {
        // no overlap with current and next
        i += (curr_end <= dst_offset) ? 1 : 0;
        ranges_.insert(ranges_.begin() + i, new_range);       
        ++n;
        
        length = 0; // no need to continue
      } else {
        // no overlap with current,
        // skip merge if no update took place
        continue;
      } 
      
      if (i > 0) {
        // try merging with previous range
        auto d = this->merge_adjacent_ranges(i);
        if (d) {
          i -= (d < 0) ? 1 : 0;
          --n;
        }
      }       
    }

    if (length != 0) {
      // add remaining range
      ranges_.push_back({new_srcidx, dst_offset, src_offset, length});
      ++n;
    }

    if (i < n) {
      // try merging with previous range
      this->merge_adjacent_ranges(i);
    }
    
    // cleanup deleted nodes
    // tranverse the set in reverse order to process higher indices first
    for (auto it = deleted.rbegin(), end = deleted.rend(); it != end; ++it) {
      uint32_t idx = *it;
      bool in_use = false;
      // ensure that it is not in use
      for (auto& r : ranges_) {
        if (r.src_idx == idx) {
          in_use = true;
          break;
        }
      }
      if (!in_use) {
        this->erase_source(idx, false);
      }
    }    
  }
}

int proxyimpl::merge_adjacent_ranges(uint32_t index) {
  if (index > 0
   && ranges_[index-1].src_idx == ranges_[index].src_idx
   && (ranges_[index-1].dst_offset + ranges_[index-1].length) == ranges_[index].dst_offset
   && ranges_[index].src_offset == ranges_[index-1].src_offset + ranges_[index-1].length) {
    ranges_[index-1].length += ranges_[index].length;
    ranges_.erase(ranges_.begin() + index);
    return -1;
  }
  if (index + 1 < ranges_.size()
   && ranges_[index].src_idx == ranges_[index+1].src_idx
   && (ranges_[index].dst_offset + ranges_[index].length) == ranges_[index+1].dst_offset
   && ranges_[index+1].src_offset == ranges_[index].src_offset + ranges_[index].length) {
    ranges_[index].length += ranges_[index+1].length;
    ranges_.erase(ranges_.begin() + index + 1);
    return 1;
  }
  return 0;
}

void proxyimpl::erase_source(uint32_t index, bool resize) {
  // update ranges
  auto size = this->size();
  uint32_t d = 0;
  for (uint32_t i = 0, n = ranges_.size(); i < n; ++i) {
    auto& range = ranges_[i];
    if (range.src_idx >= index) {
      if (range.src_idx > index) {
        if (resize) {
          range.dst_offset = d;
        }
        range.src_idx -= 1;
      } else {
        size -= range.length;
        ranges_.erase(ranges_.begin() + i);
        --i;
        --n;
        continue;
      }
    }
    d += range.length;
  }
  // remove src
  this->remove_src(index);
  if (resize) {
    this->resize(size);
  }
}

void proxyimpl::write(uint32_t dst_offset,
                      lnodeimpl* src,
                      uint32_t src_offset,
                      uint32_t length,
                      const source_location& sloc) {
  assert(size() > dst_offset);
  assert(size() >= dst_offset + length);
  if (ctx_->conditional_enabled(this)) {
    if (src_offset != 0 || src->size() != length) {
      src = ctx_->create_node<proxyimpl>(src, src_offset, length, src->name(), sloc);
    }
    ctx_->conditional_write(this, dst_offset, length, src, sloc);
  } else {
    this->add_source(dst_offset, src, src_offset, length);
  }
}

bool proxyimpl::equals(const lnodeimpl& other) const {
  if (lnodeimpl::equals(other)) {
    auto& _other = reinterpret_cast<const proxyimpl&>(other);
    return (this->ranges_ == _other.ranges_);
  }
  return false;
}

lnodeimpl* proxyimpl::source(uint32_t offset, 
                             uint32_t length, 
                             const std::string& name,
                             const source_location& sloc) const {
  assert(length <= this->size());
  if (0 == this->num_srcs())
    return nullptr;

  // return the nested node if the offset/size match
  for (auto& range : ranges_) {
    if (range.length == length
     && range.dst_offset == offset
     && range.src_offset == 0
     && this->src(range.src_idx).size() == length) {
      return this->src(range.src_idx).impl();
    }
  }

  // return new slice
  auto proxy = ctx_->create_node<proxyimpl>(length, name, sloc);
  for (auto& range : ranges_) {
    uint32_t r_end = range.dst_offset + range.length;
    uint32_t src_end = offset + length;
    if (offset >= r_end || src_end <= range.dst_offset)
      continue;
    uint32_t sub_start = std::max(offset, range.dst_offset);
    uint32_t sub_end = std::min(src_end, r_end);
    uint32_t dst_offset = sub_start - offset;
    uint32_t src_offset = range.src_offset + (sub_start - range.dst_offset);
    proxy->add_source(dst_offset, this->src(range.src_idx).impl(), src_offset, sub_end - sub_start);
  }

  return proxy;
}

void proxyimpl::print(std::ostream& out) const {
  out << "#" << id_ << " <- " << this->type() << this->size();
  out << "(";
  uint32_t d(0);
  auto_separator sep(", ");
  for (auto& range : ranges_) {
    out << sep;        
    if (d != range.dst_offset) {
      out << range.dst_offset << "-";
    }
    d += range.length;
    auto& src = this->src(range.src_idx);
    out << "#" << src.id();
    if (range.length < src.size()) {
      out << "[" << range.src_offset;
      if (range.length > 1) {
        out << "-" << range.src_offset + (range.length - 1);
      }
      out << "]";
    }
  }
  out << ")";
}

///////////////////////////////////////////////////////////////////////////////

refimpl::refimpl(
    context* ctx,
    lnodeimpl* src,
    uint32_t offset,
    uint32_t length,
    const std::string& name,
    const source_location& sloc)
  : proxyimpl(ctx, src, offset, length, name, sloc) 
{}

void refimpl::write(
    uint32_t dst_offset,
    lnodeimpl* src,
    uint32_t src_offset,
    uint32_t length,
    const source_location& sloc) {
  assert(1 == ranges_.size());
  assert(length <= ranges_[0].length);
  assert(type_proxy == this->src(0).impl()->type());
  auto dst = reinterpret_cast<proxyimpl*>(this->src(0).impl());  
  dst->write(
    ranges_[0].src_offset + dst_offset,
    src,
    src_offset,
    length,
    sloc);
}
#pragma once

#include "lnodeimpl.h"

namespace ch {
namespace internal {

class proxyimpl : public lnodeimpl {
public:

  struct range_t {
    uint32_t src_idx;
    uint32_t dst_offset;
    uint32_t src_offset;
    uint32_t length;

    bool operator==(const range_t& other) const {
      return this->src_idx    == other.src_idx
          && this->dst_offset == other.dst_offset
          && this->src_offset == other.src_offset
          && this->length     == other.length;
    }

    bool operator!=(const range_t& other) const {
      return !(*this == other);
    }

    struct hash_t {
      std::size_t operator()(const range_t& key) const {
        uint32_t h0 = (key.length << 16) | key.src_offset;
        uint32_t h1 = (key.src_idx << 16) | key.dst_offset;
        return hash_combine(h0, h1);
      }
    };
  };

  bool is_identity() const {
    return (1 == ranges_.size())
        && (this->src(0).size() == this->size());
  }

  virtual bool check_fully_initialized() const;

  const range_t& range(uint32_t index) const {
    return ranges_[index];
  }

  const std::vector<range_t>& ranges() const {
    return ranges_;
  }

  std::vector<range_t>& ranges() {
    return ranges_;
  }

  void add_source(uint32_t dst_offset,
                  const lnode& src,
                  uint32_t src_offset,
                  uint32_t length);

  void erase_source(uint32_t src_idx, bool resize = false);

  int merge_adjacent_ranges(uint32_t index);

  lnodeimpl* clone(context* ctx, const clone_map& cloned_nodes) const override;

  bool equals(const lnodeimpl& other) const override;

  void write(uint32_t dst_offset,
             const lnode& src,
             uint32_t src_offset,
             uint32_t length) override;

  lnodeimpl* slice(uint32_t offset, uint32_t length, const source_location& sloc) const override;

  void print(std::ostream& out) const override;
  
protected:

  proxyimpl(context* ctx,
            uint32_t size,
            const source_location& sloc,
            const std::string& name);

  proxyimpl(context* ctx,
            const lnode& src,
            const source_location& sloc,
            const std::string& name);

  proxyimpl(context* ctx,
            const lnode& src,
            uint32_t offset,
            uint32_t length,
            const source_location& sloc,
            const std::string& name);

  std::vector<range_t> ranges_;

  friend class context;
};

///////////////////////////////////////////////////////////////////////////////

class refimpl : public proxyimpl {
public:

  void write(uint32_t dst_offset,
             const lnode& src,
             uint32_t src_offset,
             uint32_t length) override;

  bool check_fully_initialized() const override;

protected:

  refimpl(context* ctx,
          const lnode& src,
          uint32_t offset,
          uint32_t length,
          const source_location& sloc,
          const std::string& name);

  friend class context;
};

}
}

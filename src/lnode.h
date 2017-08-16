#pragma once

#include "typebase.h"

namespace cash {
namespace detail {

class lnodeimpl;
class context;

using ch_cycle = uint64_t;

class lnode {
public:
  
  using data_type = nodebuf<lnode>;

  lnode();

  explicit lnode(uint32_t size);

  explicit lnode(lnodeimpl* impl);

  explicit lnode(const bitvector& value);

  explicit lnode(const data_type& data);

  lnode(const lnode& rhs);

  lnode(const lnode& rhs, uint32_t size);

  lnode(lnode&& rhs);

  ~lnode();

  lnode& operator=(const lnode& rhs);

  lnode& operator=(lnode&& rhs);

  lnodeimpl* get_impl() const;

  void set_impl(lnodeimpl* impl);
  
  uint32_t get_id() const;
  
  uint32_t get_size() const;
  
  context* get_ctx() const;
  
  bool ready() const;
  
  bool valid() const;
 
  const bitvector& eval(ch_cycle t);  

  void assign(const lnode& rhs, uint32_t size);

  void assign(const bitvector& value);

  void read_data(data_type& inout,
                 uint32_t offset,
                 uint32_t length,
                 uint32_t size) const;
  
  void write_data(uint32_t dst_offset,
                  const data_type& in,
                  uint32_t src_offset,
                  uint32_t src_length,
                  uint32_t size);

protected:

  void ensureInitialized(uint32_t size) const;

  void clear();
  
  void move(lnode& rhs);
  
  void init(uint32_t dst_offset,
            const lnode& src,
            uint32_t src_offset,
            uint32_t src_length,
            uint32_t size);

  void assign(uint32_t dst_offset,
              const lnode& src,
              uint32_t src_offset,
              uint32_t src_length,
              uint32_t size);
  
  mutable lnodeimpl* impl_;
  
  friend class lnodeimpl;
  friend class context; 
};

inline std::ostream& operator<<(std::ostream& out, const lnode& rhs) {
  out << rhs.get_id();
  return out;
}

}
}

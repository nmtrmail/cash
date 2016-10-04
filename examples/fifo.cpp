#include <iostream>
#include <fstream>
#include <chdl.h>

using namespace chdl::core;
using namespace chdl::sim;

template <unsigned ADDR, unsigned WIDTH>
__ch_out(ch_bitv<WIDTH>, ch_logic, ch_logic) FiFo(
  const ch_bitv<WIDTH>& din,
  const ch_logic& push,
  const ch_logic& pop) {
  
  ch_bitv<WIDTH> dout;
  ch_logic empty;
  ch_logic full;
  
  ch_mem<WIDTH, ADDR> mem;
  ch_bitv<ADDR+1> rd_ptr, wr_ptr;
  ch_bitv<ADDR> rd_addr(ch_slice<ADDR>(rd_ptr));
  ch_bitv<ADDR> wr_addr(ch_slice<ADDR>(wr_ptr));

  ch_logic reading(pop && !empty);
  ch_logic writing(push && (!full || pop));  
  
  rd_ptr = ch_reg(ch_select(reading, rd_ptr + 1, rd_ptr));
  wr_ptr = ch_reg(ch_select(writing, wr_ptr + 1, wr_ptr));
  
  empty = (wr_ptr == rd_ptr);
  full  = (wr_addr == rd_addr) && (wr_ptr[ADDR] != rd_ptr[ADDR]);
  dout  = mem.read(rd_addr);
  mem.write(wr_addr, din, writing);
  
  return __ch_ret(dout, empty, full);
};

int main(int argc, char **argv) {
  ch_bus<2> din, dout;
  ch_signal push, pop, empty, full;

  ch_device myDevice(FiFo<1, 2>, din, push, pop, dout, empty, full);

  /*std::ofstream v_file("fifo.v");
  myDevice.toVerilog("fifo", v_file);
  v_file.close();*/

  ch_vcdtracer tracer("fifo.vcd");
  __ch_trace(tracer, din, push, pop, dout, empty, full);
  tracer.run([&](ch_cycle time)->bool {
    switch (time) {
    case 0:
      din  = 0x1;
      push = 1_b;
      break;
    case 1:
      din  = 0x2;
      push = 1_b;
      break;
    case 2:
      din  = 0x0;
      push = 0_b;
      break;
    case 3:
      pop = 1_b;
      break;
    case 4:
      pop = 1_b;
      break;
    case 5:
      pop = 0_b;
      break;
    }
    return (time < 10);
  });
  tracer.close();

  return 0;
}

#include <iostream>
#include <fstream>
#include <cash.h>

using namespace cash::core;
using namespace cash::core_literals;
using namespace cash::sim;

template <unsigned A, unsigned W>
__out(ch_bit<W>, ch_bit1, ch_bit1) FiFo(
      const ch_bit<W>& din,
      const ch_bit1& push,
      const ch_bit1& pop) {
  ch_ram<W, A> mem;
  ch_seq<ch_bit<A+1>> rd_ptr, wr_ptr;
  ch_bit1 empty, full;

  ch_bit<A> rd_A(ch_slice<A>(rd_ptr));
  ch_bit<A> wr_A(ch_slice<A>(wr_ptr));

  ch_bit1 reading(pop && !empty);
  ch_bit1 writing(push && !full);
  
  rd_ptr.next = ch_select(reading, rd_ptr + 1, rd_ptr);
  wr_ptr.next = ch_select(writing, wr_ptr + 1, wr_ptr);
  
  empty = (wr_ptr == rd_ptr);
  full  = (wr_A == rd_A) && (wr_ptr[A] != rd_ptr[A]);
  
  auto dout = mem[rd_A];
  __if (writing) (
    mem[wr_A] = din;
  );
  
  __ret(dout, empty, full);
}

int main(int argc, char **argv) {
  std::ofstream vcd_file("fifo.vcd");
  ch_bus2 din, dout;
  ch_bus1 push, pop, empty, full;

  ch_device myDevice(FiFo<1, 2>, din, push, pop, dout, empty, full);

  /*std::ofstream v_file("fifo.v");
  myDevice.to_verilog("fifo", v_file);
  v_file.close();*/

  ch_vcdtracer tracer(vcd_file, myDevice);
  __trace(tracer, din, push, pop, dout, empty, full);
  tracer.run([&](ch_cycle time)->bool {
    switch (time) {
    case 0:
      assert(empty);
      assert(!full);
      din  = 1;
      push = 1;      
      break;      
    case 1:
      assert(!empty);
      assert(!full);
      assert(dout == 1);      
      din  = 2;
      push = 1;
      break;
    case 2:
      assert(!empty);
      assert(full);
      assert(dout == 1);
      din  = 0;
      push = 0;
      break;
    case 3:
      assert(!empty);
      assert(full);
      assert(dout == 1);
      pop = 1;
      break;
    case 4:
      assert(!empty);
      assert(!full);
      assert(dout == 2);
      pop = 1;
      break;
    case 5:
      assert(empty);
      assert(!full);
      assert(dout == 1);
      pop = 0;
      break;
    }
    std::cout << "t" << time << ": din=" << din << ", push=" << push << ", pop=" << pop << ", dout=" << dout << ", empty=" << empty << ", full=" << full << std::endl;
    return (time < 6);
  });

  return 0;
}

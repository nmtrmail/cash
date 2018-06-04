module ch_queue(
  input wire clk,
  input wire reset,
  input wire io_enq_valid,
  input wire[3:0] io_enq_data,
  input wire io_deq_ready,
  output wire io_enq_ready,
  output wire io_deq_valid,
  output wire[3:0] io_deq_data,
  output wire[1:0] io_size
);
  reg[1:0] reg26;
  reg[1:0] reg32;
  wire proxy35; // v2309 - /home/blaise/dev/cash/src/htl/queue.h(27)
  wire proxy37; // v2310 - /home/blaise/dev/cash/src/htl/queue.h(28)
  wire and38;
  wire and40;
  wire[1:0] add44;
  wire[1:0] sel46;
  wire[1:0] add49;
  wire[1:0] sel51;
  reg[3:0] mem53[0:1];
  wire ne57;
  wire eq61;
  wire ne63;
  wire or65;
  wire[1:0] sub67;

  always @ (posedge clk) begin
    reg26 <= sel46;
  end
  always @ (posedge clk) begin
    reg32 <= sel51;
  end
  assign proxy35 = reg26[0];
  assign proxy37 = reg32[0];
  assign and38 = io_deq_ready & ne57;
  assign and40 = io_enq_valid & or65;
  assign add44 = reg26 + 2'h1;
  assign sel46 = and38 ? add44 : reg26;
  assign add49 = reg32 + 2'h1;
  assign sel51 = and40 ? add49 : reg32;
  always @(posedge clk) begin
    if (and40) begin
      mem53[proxy37] = io_enq_data;
    end
  end
  assign ne57 = reg32 != reg26;
  assign eq61 = reg32[1] == reg26[1];
  assign ne63 = proxy37 != proxy35;
  assign or65 = ne63 | eq61;
  assign sub67 = reg32 - reg26;

  assign io_enq_ready = or65;
  assign io_deq_valid = ne57;
  assign io_deq_data = mem53[proxy35];
  assign io_size = sub67;

endmodule

module QueueWrapper(
  input wire clk,
  input wire reset,
  input wire io_enq_valid,
  input wire[3:0] io_enq_data,
  input wire io_deq_ready,
  output wire io_enq_ready,
  output wire io_deq_valid,
  output wire[3:0] io_deq_data
);
  wire bindin28;
  wire bindin29;
  wire bindin30;
  wire[3:0] bindin33;
  wire bindout38;
  wire bindout43;
  wire[3:0] bindout48;
  wire bindin51;

  assign bindin28 = clk;
  assign bindin29 = reset;
  ch_queue __module224__(.clk(bindin28), .reset(bindin29), .io_enq_valid(bindin30), .io_enq_data(bindin33), .io_deq_ready(bindin51), .io_enq_ready(bindout38), .io_deq_valid(bindout43), .io_deq_data(bindout48));
  assign bindin30 = io_enq_valid;
  assign bindin33 = io_enq_data;
  assign bindin51 = io_deq_ready;

  assign io_enq_ready = bindout38;
  assign io_deq_valid = bindout43;
  assign io_deq_data = bindout48;

endmodule
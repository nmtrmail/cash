`timescale 1ns/1ns
`include "vending.v"

`define assert(condition) if (!(condition)) begin $display("assertion FAILED!"); $finish_and_return(1); end

module testbench();

    reg clk    = 0;
    reg reset  = 0;
    reg nickel = 1;
    reg dime   = 0;
    wire valid;

    VendingMachine device(clk, reset, nickel, dime, valid);

    always begin
        #1 clk = !clk;
    end

    initial begin
        $dumpfile("testbench.vcd");
        $dumpvars(0, testbench);

        $display ("time\tclk\treset\tnickel\tdime\tvalid");
        $monitor("%3d\t%b\t%b\t%b\t%b\t%b", $time, clk, reset, nickel, dime, valid);

        #0 reset  = 1;
        #1 reset  = 1;
        #1 reset  = 0;

        #8 `assert(valid == 1);
            $finish;
    end

endmodule
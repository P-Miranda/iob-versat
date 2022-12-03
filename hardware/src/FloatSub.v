`timescale 1ns / 1ps
`include "xversat.vh"

module FloatSub #(
         parameter DELAY_W = 32,
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,

    //input / output data
    input [DATA_W-1:0]            in0,
    input [DATA_W-1:0]            in1,

    (* versat_latency = 5 *) output [DATA_W-1:0]       out0
    );

wire [DATA_W-1:0] negative = {~in1[31],in1[30:0]};

fp_add adder(
    .clk(clk),
    .rst(rst),

    .start(1'b1),
    .done(),

    .op_a(in0),
    .op_b(negative),

    .overflow(),
    .underflow(),
    .exception(),

    .res(out0)
     );

endmodule

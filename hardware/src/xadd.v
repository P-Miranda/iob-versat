`timescale 1ns / 1ps
`include "xversat.vh"

module xadd #(
         parameter DELAY_W = 10,
         parameter DATA_W = 32
              )
    (
    //control
    input                         clk,
    input                         rst,
    
    input                         run,
    output                        done,

    //input / output data
    input [DATA_W-1:0]            in0,
    input [DATA_W-1:0]            in1,

    output reg [DATA_W-1:0]       out0
    );

assign done = 1'b1;

always @(posedge clk,posedge rst)
begin
   if(rst)
      out0 <= 0;
   else
      out0 <= in0 + in1;
end

endmodule
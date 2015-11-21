// adaFruit 32x32 LED display driver
/////////////////////////////////////////////
// ledDriver.sv
// HMC E155 19 November 2015
// ascott@hmc.edu, emuller@hmc.edu
/////////////////////////////////////////////


/////////////////////////////////////////////
// ledDriver
//      Top level module for the led driver with spi interface and the 
//      driver core.
/////////////////////////////////////////////
module ledDriver(input logic clk, reset, sclk, mosi,
                output logic r0, g0, b0, r1, g1, b1, a, b, c, d, latch, outclk);


endmodule

/////////////////////////////////////////////
// led_spi
//      SPI interface. Reads in 288 (32 x 3 x 3 bits per row) bits then outputs
//      the bits on a bus as well as the address for where to put those bits.
/////////////////////////////////////////////
module led_spi(input logic sclk, mosi,
                output logic [287:0] data,
                output logic [3:0] addr);

endmodule


/////////////////////////////////////////////
// led_core
//      Core of the led drive. Has two RAMs to which it writes the data it
//      receives from the spi module. 
/////////////////////////////////////////////
module led_core(input logic clk, reset,
                input logic [287:0] data,
                input logic [3:0] addr,
                output logic r0, g0, b0, r1, g1, b1, a, b, c, d, latch);

endmodule

/////////////////////////////////////////////
// shift_reg
//      Shift register that reads in parallel data and outputs serial data.
/////////////////////////////////////////////
module shift_reg #(parameter OUTWIDTH = 9)
                (input logic clk, reset,
                input logic [OUTWIDTH*32-1:0] parallelin,
                output logic [OUTWIDTH-1:0] serialout);
    logic[32] internalData [OUTWIDTH - 1];
    always_ff @(posedge clk, posedge reset)
        if (reset)
            internalData <= 0; // probably needs to be done a different way
        else
            internalData <= {internalData[0], internalData[31:1]}

    assign serialout = internalData[0];
endmodule

module easy_shift_reg (input logic clk, reset, load,
                        input logic[3:0] parallelin,
                        output logic serialout);
    logic in0, in1, in2, in3, out0, out1, out2, out3;
    flopr #(1) reg1(clk, reset, in0, out0);
    flopr #(1) reg2(clk, reset, in1, out1);
    flopr #(1) reg3(clk, reset, in2, out2);
    flopr #(1) reg4(clk, reset, in3, out3);
    assign in0 = load ? parallelin[3] : 1'b1;
    assign in1 = load ? parallelin[2] : out0;
    assign in2 = load ? parallelin[1] : out1;
    assign in3 = load ? parallelin[0] : out2;
    assign serialout = out3;
endmodule

/////////////////////////////////////////////
// flopr
//   Parameterized width flop that writes every clock cycle
/////////////////////////////////////////////

module flopr #(parameter WIDTH = 8)
              (input  logic             clk, reset,
               input  logic [WIDTH-1:0] d, 
               output logic [WIDTH-1:0] q);

    always_ff @(posedge clk, posedge reset)
        if (reset)
            q <= 0;
        else
            q <= d;
endmodule

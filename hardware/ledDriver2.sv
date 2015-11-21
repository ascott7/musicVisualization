/**
 * \file ledDriver2.sv
 *
 * \author Eric Mueller -- emueller@hmc.edu
 *
 * \brief adaFruit 32x32 LED display driver.
 */

/**
 * \brief Top level module. Just glue.
 *
 * \param clk     40MHz board clock
 * \param reset   synchronous system reset signal
 * \param sck     SPI clock
 * \param sdi     SPI slave data in
 * \param rgb1    'R1', 'G1', and 'B1' inputs for the LED matrix.
 * \param rgb2    same for 'R2', 'G2', 'B2'
 * \param rsel    'A', 'B', 'C', and 'D' inputs for the LED matrix, i.e. the
 *                row select pins.
 * \param mclk    'clk' inout to the LED matrix
 * \param latch   'latch' input to the LED matrix
 * \param oe      'output enable' input to the LED matrix
 *
 * \note For mult-bit busses, the component signals listed above are given in
 * lsb to msb order, e.g. rgb1[0] is the R1 signal.
 */
module driver(input  logic clk,
              input  logic reset,
              input  logic sck, 
              input  logic sdi,
              output logic [2:0] rgb1, rgb2,
              output logic [3:0] rsel,
              output logic mclk,
              output logic latch,
              output logic oe);

    logic done, fend, fstart, we;
    logic [3*CDEPTH-1:0] rpix, wpix;
    output logic [9:0] addr;

    assign oe = 1'b1;

    frame_reader fr(clk, reset, sck, sdi, addr, rpix, done);
    frame_writer fw(clk, reset, we, wpix, addr, fstart, fend,
                    rb1, rgb2, rsel, mclk, latch);
    controler ctl(clk, reset, done, fend, rpix, wpix, addr, fstart, we);

endmodule

/**
 * \brief Read in frames from SPI and store them in a RAM buffer. Allow read
 * access via an address bus.
 * 
 * \param CDEPTH   The color depth in bits. Default to using 4 bits to represent
 *                 each color. **NOTE** this dictates the number of bits the
 *                 SPI protocal reads for each pixel.
 *
 * \param raddr    The read address, i.e. which pixel to read.
 * \param pix      The pixel at that raddr.
 * \param done     Raised for a single cycle when we finish reading a frame
 */
module frame_reader
                  #(parameter CDEPTH=4)
                   (input  logic clk,
                    input  logic reset,
                    input  logic __sck, 
                    input  logic __sdi,
                    input  logic [9:0] raddr,
                    output logic [3*CDEPTH-1:0] pix,
                    output logic done);

    logic [3*CDEPTH-1:0] frame[0:32*32];
    logic [9:0] faddr;
    logic [7:0] bits;
    logic saw_sck;
    logic _sck, _sdi, sck, sdi;

    assign pix = frame[raddr];

    /*
     * SPI protocol. faddr keeps track of where in the frame buffer we are
     * currently writing to. Bits keeps track of the number of bits we have
     * written to the current pixel. saw_sck ensures that we only act on each
     * positive sck edge once, without using it in a sensativity list
     */
    always_ff @(posedge clk) begin
        /* synchronize sck and sdi */
        _sck <= __sck;
        _sdi <= __sdi;
        sck <= _sck;
        sdi <= _sdi;

        /* XXX: reset should zero the frame, but that needs an FSM. */
        if (reset) begin
            faddr <= '0;
            bits <= '0;
            saw_sck <= '0;
        end else if (sck && ~saw_sck) begin
            saw_sck <= '1;
            frame[faddr] = bits == 0 ? sdi : frame[faddr] | sdi << bits;
            if (bits == 3*CDEPTH-1) begin
                bits <= '0;
                faddr <= faddr + 1'b1;
            end else
                bits <= bits + 1'b1;
        end else if (~sck)
            saw_sck <= '0;

        done <= faddr == '1;
    end
endmodule

/**
 * \brief Frame writer. Maintains an internal frame buffer written to by the
 * user of the module and writes out the current frame when the fstart signal
 * is raised. The fend signal is raised when the current frame has been fully
 * written. 
 *
 * *** module params ***
 * \param CDEPTH   Color depth of frame buffer.
 * \param MCLK_DIV_BITS  number of bits to use for the matrix clock divider.
 *                       e.g. MCLK_DIV_BITS=3 implies mclk has one rising edge
 *                       for every 8 rising edges of clk.
 *
 * *** control signals ***
 * \param clk      40MHz board clock
 * \param reset    synchronous system reset
 * \param fstart   raised by controler when module should start writing out
 *                 the next frame
 * \param fend     raised by this module when it finishes writing a frame.
 *                 this is not lowered until fstart is again raised. the module
 *                 does nothing once this signal is raised.
 *
 * *** ram interface ***
 * \param we       write enable for internal frame buffer
 * \param wpix     write line for internal frame buffer
 * \param waddr    write adddress for internal frame buffer
 *
 * *** LED matrix outputs ***
 * \param lo_rgb   first set of RGB outputs. should be the physically higher
 *                 set of rows, but corresponds to the lower addresses in the 
 *                 buffer.
 * \param hi_rgb   second set of RGB outputs. physicially lower set of rows.
 * \param mclk     matrix clock input
 * \param latch    matrix latch input
 *
 * \detail This module performs the following functions:
 *
 *     1. Maintain an internal frame buffer. This, combined with the buffer
 *        stored by the frame_reader implemented double-buffering and
 *        with the help of the controler prevents screen tearing. 
 *
 *     2. Clock out rows of data on the cadance of mclk, a clock slow enough
 *        to avoid analog problems with the LED Matrix. This include driving
 *        the latch signal appropriately
 *
 *     3. Implement PWM. At the advice of other users of the LED Matrix, PWM
 *        is implemented by repeatedly refreshing a single row to get
 *        the desired color, then moving on, as opposed to repeatedly
 *        refreshing the whole frame.
 */
module frame_writer
                  #(parameter CDEPTH=4,
                    parameter MCLK_DIV_BITS=3)
                   (input  logic clk,
                    input  logic reset,
                    input  logic we,
                    input  logic [3*CDEPTH-1:0] wpix,
                    input  logic [9:0] waddr,
                    input  logic fstart,
                    output logic fend,
                    output logic [2:0] lo_rgb,
                    output logic [2:0] hi_rgb
                    output logic [3:0] row,
                    output logic mclk,
                    output logic latch);

    logic [3*CDEPTH-1:0] frame[0:32*32];
    logic [4:0] col;
    input logic [3*CDEPTH-1:0] lo_pix, hi_pix;
    logic [CDEPTH-1:0] pwm_cnt;
    logic [MCLK_DIV_BITS-1:0] mclk_div;

    always_ff @(posedge clk) begin
        if (reset || fstart) begin
            row <= '0;
            col <= '0;
            fend <= '0;
            lo_rgb <= '0;
            hi_rgb <= '0;
            pwm_cnt <= '0;
            mclk_div <= '0;
        end else if (~fend) begin
            mclk_div <= mclk_div + 1'b1;
            col <= mclk_div == {MCLK_DIV_BITS-1'b1, 1'b0} ? col + 1'b1 : col;
            pwm_cnt <= col == '1 ? pwm_cnt + 1'b1 : pwm_cnt;
            row <= pwm_cnt == '1 ? row + 1'b1 : row;
            fend <= {col,row,mclk_div} == '1;
        end
    end

    assign frame[waddr] = we ? wpix : frame[waddr];

    assign lo_pix = frame[{col, row, 1'b0}];
    assign hi_pix = frame[{col, row, 1'b1}];
    assign mclk = mclk_div[MCLK_DIV_BITS-1];
    assign latch = col == '1;

    assign lo_rgb[0] = lo_pix[CDEPTH-1:0]  > pwm_cnt;
    assign lo_rgb[1] = lo_pix[2*CDEPTH-1:CDEPTH] > pwm_cnt;
    assign lo_rgb[2] = lo_pix[3*CDEPTH-1:2*CDEPTH] > pwm_cnt;

    assign hi_rgb[0] = hi_pix[CDEPTH-1:0] > pwm_cnt;
    assign hi_rgb[1] = hi_pix[2*CDEPTH-1:CDEPTH] > pwm_cnt;
    assign hi_rgb[2] = hi_pix[3*CDEPTH-1:2*CDEPTH] > pwm_cnt;
endmodule

/**
 * \brief State machine to control the whole system. The main reason this exists
 * is to implement double buffering/prevent tearing.
 *
 * \param clk      40MHz board clock
 * \param reset    synchronous system reset
 * \param done     input from frame_reader -- raised when we get a new frame
 *                 from the Pi.
 * \param fend     input from frame_writer -- raised when we finish writing a
 *                 frame
 * \param rpix     pixel to read from the frame_reader's internal buffer
 * \param wpix     pixel to write to the frame_writer's internal buffer
 * \param addr     address to read from frame_reader and write to frame_writer
 * \param fstart   raised for a single cycle when the frame_writer should start
 *                 writing out the next frame.
 * \param we       write enable for frame_writer's ram
 */
module controler
               #(parameter CDEPTH=4)
                (input  logic clk,
                 input  logic reset,
                 input  logic done,
                 input  logic fend,
                 input  logic [3*CDEPTH-1:0] rpix,
                 output logic [3*CDEPTH-1:0] wpix,
                 output logic [9:0] addr,
                 output logic fstart,
                 output logic we);

    typedef enum logic [2:0]
        {WRITING, NEW_FRAME, COPYING, WAITING, RESET} state_t;

    state_t state, next_state;
    logic saw_done;

    always_ff @(posedge clk) begin
        if (reset) begin
            addr <= '0;
            state <= RESET;
            addr <= '0;
        end else begin
            state <= next_state;
            addr <= state == COPYING || state == RESET ? addr + 1'b1 : addr;
            saw_done <= state == COPYING ? '0 : done ? '1 : saw_done;
        end
    end

    always_comb
        case (state)
        WRITING: next_state = fend ? saw_done ? COPYING : NEW_FRAME : WRITING;
        NEW_FRAME: next_state = WRITING;
        COPYING: next_state = addr == '1 ? WAITING : COPYING;
        WAITING: next_state = saw_done ? COPYING : WAITING;
        RESET: next_state = addr == '1 ? WAITING : RESET;
        endcase

    assign wpix = state == RESET ? '0 : rpix;
    assign fstart = state == NEW_FRAME;
    assign we = state == COPYING || state == RESET;

endmodule
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
 * \param mclk    'clk' input to the LED matrix
 * \param latch   'latch' input to the LED matrix
 * \param oe      'output enable' input to the LED matrix
 *
 * \note For mult-bit buses, the component signals listed above are given in
 * lsb to msb order, e.g. rgb1[0] is the R1 signal.
 */
module ledDriver2
                #(parameter CDEPTH=4)
                 (input  logic clk,
                  input  logic reset,
                  input  logic sck, 
                  input  logic sdi,
                  output logic [2:0] rgb1, rgb2,
                  output logic [3:0] rsel,
                  output logic mclk,
                  output logic latch,
                  output logic oe);

    logic cdone, rdone, fend, fstart, we;
    logic [3*CDEPTH-1:0] rpix, wpix;
    logic [9:0] raddr, waddr;

    assign oe = 1'b0;

    frame_reader fr(clk, reset, sck, sdi, cdone, raddr, rpix, rdone);
    frame_writer fw(clk, reset, we, wpix, waddr, fstart, fend,
                    rgb1, rgb2, rsel, mclk, latch);
    controller ctl(clk, reset, rdone, fend, rpix, wpix, 
                   raddr, waddr, fstart, we, cdone);

endmodule

/**
 * \brief Read in frames from SPI and store them in a RAM buffer. Allow read
 * access via an address bus.
 * 
 * \param CDEPTH   The color depth in bits. Default to using 4 bits to represent
 *                 each color. **NOTE** this dictates the number of bits the
 *                 SPI protocol reads for each pixel.
 * \param FRAME_ORDER   The frame has 2**FRAME_ORDER pixels.
 *
 * \param clk      40MHz board clock
 * \param reset    synchronous system reset
 * \param __sck    unsynchronized SPI clock
 * \param __sdi    unsynchronized SPI data in
 * \param cdone    copy done signal. Raised by controller when it finih
 * \param raddr    The read address, i.e. which pixel to read.
 * \param pix      The pixel at that raddr.
 * \param rdone    Raised for one cycle when we finish a reading a frame.
 */
module frame_reader
                  #(parameter CDEPTH=4,
                    parameter FRAME_ORDER=10)
                   (input  logic clk,
                    input  logic reset,
                    input  logic __sck, 
                    input  logic __sdi,
                    input  logic cdone,
                    input  logic [FRAME_ORDER-1:0] raddr,
                    output logic [3*CDEPTH-1:0] pix_out,
                    output logic rdone);

    typedef enum logic [2:0] {WAIT, NEW_SDI, FULL_PIX, DONE, COPY, SCK_HI} state_t;

    state_t state, next_state;
    logic [3*CDEPTH-1:0] pix_in;
    logic [FRAME_ORDER-1:0] waddr, addr;
    logic [7:0] bits;
    logic _sck, _sdi, sck, sdi, we;

    ram #(3*CDEPTH, FRAME_ORDER) frame(clk, we, addr, pix_in, pix_out);

    /*
     * SPI protocol. waddr keeps track of where in the frame buffer we are
     * currently writing to. Bits keeps track of the number of bits we have
     * written to the current pixel. saw_sck ensures that we only act on each
     * positive sck edge once, without using it in a sensitivity list
     */
    always_ff @(posedge clk) begin
        /* synchronize sck and sdi */
        _sck <= __sck;
        _sdi <= __sdi;
        sck <= _sck;
        sdi <= _sdi;

        if (reset) begin
            waddr <= '0;
            bits <= '0;
            state <= WAIT;
            pix_in <= '0;
        end else begin
            state <= next_state;
            if (state == NEW_SDI) begin
                bits <= bits + 1'b1;
                pix_in <= {sdi, pix_in[3*CDEPTH-1:1]};
            end else if (state == FULL_PIX) begin
                bits <= '0;
                waddr <= waddr + 1'b1;
                pix_in <= '0;
            end
        end
    end

    always_comb
        case (state)
        WAIT: next_state = sck ? NEW_SDI : WAIT;
        NEW_SDI: next_state = bits == 3*CDEPTH-1 ? FULL_PIX : SCK_HI;
        FULL_PIX: next_state = waddr == '1 ? DONE : sck ? SCK_HI : WAIT;
        DONE: next_state = COPY;
        COPY: next_state = cdone ? WAIT : COPY;
        SCK_HI: next_state = sck ? SCK_HI : WAIT;
        default: next_state = WAIT;
        endcase

    assign addr = state == COPY ? raddr : waddr;
    assign we = state == FULL_PIX; 
    assign rdone = state == DONE;

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
 * \param fstart   raised by controller when module should start writing out
 *                 the next frame
 * \param fend     raised by this module when it finishes writing a frame.
 *                 this is not lowered until fstart is again raised. the module
 *                 does nothing once this signal is raised.
 *
 * *** ram interface ***
 * \param we       write enable for internal frame buffer
 * \param wpix     write line for internal frame buffer
 * \param waddr    write address for internal frame buffer
 *
 * *** LED matrix outputs ***
 * \param lo_rgb   first set of RGB outputs. should be the physically higher
 *                 set of rows, but corresponds to the lower addresses in the 
 *                 buffer.
 * \param hi_rgb   second set of RGB outputs. physically lower set of rows.
 * \param mclk     matrix clock input
 * \param latch    matrix latch input
 *
 * \detail This module performs the following functions:
 *
 *     1. Maintain an internal frame buffer. This, combined with the buffer
 *        stored by the frame_reader implemented double-buffering and
 *        with the help of the controller prevents screen tearing. 
 *
 *     2. Clock out rows of data on the cadence of mclk, a clock slow enough
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
                    parameter FRAME_ORDER=10,
                    parameter MCLK_DIV_BITS=7)
                   (input  logic clk,
                    input  logic reset,
                    input  logic we,
                    input  logic [3*CDEPTH-1:0] wpix,
                    input  logic [FRAME_ORDER-1:0] waddr,
                    input  logic fstart,
                    output logic fend,
                    output logic [2:0] lo_rgb,
                    output logic [2:0] hi_rgb,
                    output logic [3:0] row,
                    output logic mclk,
                    output logic latch);

    typedef enum logic [2:0] {WAIT, WRITE_ROW, LATCH, ROW_END, DONE} state_t;

    state_t state, next_state;
    logic [4:0] col;
    logic [3*CDEPTH-1:0] lo_pix, hi_pix;
    logic [MCLK_DIV_BITS-1:0] mclk_div;
    logic [FRAME_ORDER-2:0] addr;
    logic lo_we, hi_we;
    logic [5:0] latch_count;

    /* 
     * we use seperate rams for the top and bottom halves of the frame so we
     * we can read them both at the same time
     */
    ram #(3*CDEPTH, FRAME_ORDER-1) lo_ram(clk, lo_we, addr, wpix, lo_pix);
    ram #(3*CDEPTH, FRAME_ORDER-1) hi_ram(clk, hi_we, addr, wpix, hi_pix);

    assign addr = state == WAIT ? waddr[FRAME_ORDER-2:0] : {row, col};
    assign lo_we = we && ~waddr[FRAME_ORDER-1];
    assign hi_we = we && waddr[FRAME_ORDER-1];

    always_ff @(posedge clk) begin
        if (reset) begin
            row <= '0;
            col <= '0;
            mclk_div <= '0;
            state <= WAIT;
            latch_count <= '0;
        end else begin
            state <= next_state;

            if (state == WRITE_ROW) begin
                mclk_div <= mclk_div + 1'b1;

                /* increment col on falling edge of mclk */
                if (mclk_div == '1)
                    col <= col + 1'b1;

            end else if (state == LATCH)
                latch_count <= latch_count + 1'b1;
            else if (state == ROW_END)
                row <= row + 1'b1;
        end
    end

    always_comb
        case (state)
        WAIT: next_state = fstart ? WRITE_ROW : WAIT;
        WRITE_ROW: next_state = col == '1 && mclk_div == '1 ? LATCH : WRITE_ROW;
        LATCH: next_state = latch_count == '1 ? ROW_END : LATCH;
        ROW_END: next_state = row == '1 ? DONE : WRITE_ROW;
        DONE: next_state = WAIT;
        endcase

    assign mclk = ~mclk_div[MCLK_DIV_BITS-1];
    assign latch = state == LATCH;
    assign fend = state == DONE;

    /*
     * XXX: still need to implement PWM
     */
    assign lo_rgb[0] = lo_pix[CDEPTH-1:0] > '0;
    assign lo_rgb[1] = lo_pix[2*CDEPTH-1:CDEPTH] > '0;
    assign lo_rgb[2] = lo_pix[3*CDEPTH-1:2*CDEPTH] > '0;
    assign hi_rgb[0] = hi_pix[CDEPTH-1:0] > '0;
    assign hi_rgb[1] = hi_pix[2*CDEPTH-1:CDEPTH] > '0;
    assign hi_rgb[2] = hi_pix[3*CDEPTH-1:2*CDEPTH] > '0;
endmodule

/**
 * \brief State machine to control the whole system. The main reason this exists
 * is to implement double buffering/prevent tearing.
 *
 * \param clk      40MHz board clock
 * \param reset    synchronous system reset
 * \param rdone    input from frame_reader -- raised when we finish reading
 *                 a new frame from the Pi
 * \param fend     input from frame_writer -- raised when we finish writing a
 *                 frame
 * \param rpix     pixel to read from the frame_reader's internal buffer
 * \param wpix     pixel to write to the frame_writer's internal buffer
 * \param addr     address to read from frame_reader and write to frame_writer
 * \param fstart   raised for a single cycle when the frame_writer should start
 *                 writing out the next frame.
 * \param we       write enable for frame_writer's ram
 * \param cdone    raised when the controler is finished copying data 
 */
module controller
               #(parameter CDEPTH=4)
                (input  logic clk,
                 input  logic reset,
                 input  logic rdone,
                 input  logic fend,
                 input  logic [3*CDEPTH-1:0] rpix, 
                 output logic [3*CDEPTH-1:0] wpix,
                 output logic [9:0] raddr,
                 output logic [9:0] waddr,
                 output logic fstart,
                 output logic we,
                 output logic cdone);

    typedef enum logic [2:0] {RESET, WRITE, NEW_FRAME, COPY, WAIT} state_t;

    state_t state, next_state;
    logic saw_done;

    always_ff @(posedge clk) begin
        if (reset) begin
            raddr <= '0;
            state <= RESET;
            saw_done <= '0;
            waddr <= '0;
        end else begin
            state <= next_state;

            if (state == COPY || state == RESET)
                raddr <= raddr + 1'b1;

            if (state == COPY)
                saw_done <= '0;
            else if (rdone)
                saw_done <= '1;
        end

        waddr <= raddr;
    end

    always_comb
        case (state)
        WRITE: next_state = fend ? saw_done ? COPY : NEW_FRAME : WRITE;
        NEW_FRAME: next_state = WRITE;
        COPY: next_state = cdone ? NEW_FRAME : COPY;
        WAIT: next_state = saw_done ? COPY : WAIT;
        RESET: next_state = cdone ? WAIT : RESET;
        default: next_state = RESET;
        endcase

    assign wpix = state == RESET ? '0 : rpix;
    assign fstart = state == NEW_FRAME;
    assign cdone = waddr == '1;
    assign we = state == COPY || state == RESET;

endmodule

/**
 * \brief simple RAM module.
 *
 * \param WSIZE   size of each word in the RAM
 * \param ORDER   the ram has 2^ORDER bytes
 *
 * \param clk     system clock
 * \param we      write enable
 * \param addr    address line
 * \param in      input data. ignored unless write enable is high
 * \param out     output data, i.e. RAM[addr]
 */
module ram
         #(parameter WSIZE,
           parameter ORDER)
          (input  logic clk,
           input  logic we,
           input  logic [ORDER-1:0] addr,
           input  logic [WSIZE-1:0] in,
           output logic [WSIZE-1:0] out);

    logic [WSIZE-1:0] buffer[0:2**ORDER-1];

    always_ff @(posedge clk) begin
        if (we)
            buffer[addr] <= in;
        out <= buffer[addr];
    end
endmodule

module testbench();
    logic [11:0] frame[0:32*32-1];
    logic [9:0] addr;
    logic [3:0] bits;
    logic [2:0] rgb1, rgb2;
    logic [3:0] rsel;
    logic clk, reset, sck, sck_out, finished_send, sdi, mclk, latch, oe, did_reset;

    ledDriver2 dut(clk, reset, sck_out, sdi, rgb1, rgb2, rsel, mclk, latch, oe);

    initial begin
        bits <= '0;
        addr <= '0;
        reset <= '1;
        did_reset <= '0;
        finished_send <= '0;

        $readmemh("frame.txt", frame);
    end

    initial forever begin
        clk = 1'b0; #5;
        clk = 1'b1; #5;
    end

    /* we don't want sclk to like up with clk */
    initial forever begin
        sck = 1'b0; #21;
        sck = 1'b1; #21;
    end

    always_ff @(posedge clk) begin
        did_reset <= reset;
        if (did_reset)
            reset <= '0;
    end

    always_ff @(posedge sck) begin
        if (~reset && ~finished_send) begin
            sdi <= frame[addr][bits];
            bits <= bits == 11 ? '0 : bits + 1;
            if (bits == 11)
                addr <= addr + 1;
        end

        if (addr == '1 && bits == 11)
            finished_send <= '1;
    end

    assign sck_out = finished_send ? '0 : sck;
endmodule
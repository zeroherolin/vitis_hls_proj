`timescale 1ns / 100ps

`define X_LEN 2048

module tb_single_filter;

reg ap_clk;
reg ap_rst;
reg ap_start;

// DUT interface
wire        ap_done;
wire        ap_idle;
wire        ap_ready;
wire [63:0] x_dout;
wire        x_empty_n;
wire        x_read;
reg  [31:0] jx;
reg  [31:0] gamma;
wire [31:0] x_judgment_din;
wire        x_judgment_full_n;
wire        x_judgment_write;
wire [31:0] max_out;
wire        max_out_ap_vld;

always #5 ap_clk = ~ap_clk; // 100MHz

filter dut0 (
    .ap_clk(ap_clk),
    .ap_rst(ap_rst),
    .ap_start(ap_start),
    .ap_done(ap_done),
    .ap_idle(ap_idle),
    .ap_ready(ap_ready),
    .x_dout(x_dout),
    .x_empty_n(x_empty_n),
    .x_read(x_read),
    .jx(jx),
    .gamma(gamma),
    .x_judgment_din(x_judgment_din),
    .x_judgment_full_n(x_judgment_full_n),
    .x_judgment_write(x_judgment_write),
    .max_out(max_out),
    .max_out_ap_vld(max_out_ap_vld)
);

reg [63:0] input_data [0:`X_LEN-1];
reg [15:0] data_ptr;
assign x_dout = input_data[data_ptr];
assign x_empty_n = (data_ptr < `X_LEN);
assign x_judgment_full_n = 1'b1;

initial begin
    initialize_system();
    run_test();
    $finish;
end

task initialize_system;
begin
    ap_clk = 0;
    ap_rst = 1;
    ap_start = 0;
    data_ptr = 0;

    initialize_param();
    initialize_input_data();

    #100;
    ap_rst = 0;
    #100;
end
endtask

task initialize_param;
begin
    jx = 1;
    gamma = $shortrealtobits(200000000.0/3.0e-6);
end
endtask

task initialize_input_data;
    integer real_file, imag_file;
    integer status_r, status_i;
    shortreal real_val, imag_val;
begin
    real_file = $fopen("/home/xilinx/Desktop/filter_proj/vitis_hls/input_x_real.txt", "r");
    imag_file = $fopen("/home/xilinx/Desktop/filter_proj/vitis_hls/input_x_imag.txt", "r");

    if (!real_file || !imag_file) begin
        $display("Error opening input files!");
        $finish;
    end

    for(int i=0; i<`X_LEN; i++) begin
        status_r = $fscanf(real_file, "%f", real_val);
        status_i = $fscanf(imag_file, "%f", imag_val);

        if (status_r != 1 || status_i != 1) begin
            $display("Error reading data at line %0d", i);
            $finish;
        end

        input_data[i] = {
            $shortrealtobits(imag_val),
            $shortrealtobits(real_val)
        };
    end

    $fclose(real_file);
    $fclose(imag_file);
end
endtask

task run_test;
begin
    $display("[%0t] Starting test...", $time);
    ap_start = 1;

    while(data_ptr < `X_LEN) begin
        @(posedge ap_clk);
        if(x_read && x_empty_n) begin
            data_ptr <= data_ptr + 1;
        end
    end
    $display("[%0t] Input data transfer completed!", $time);

    wait(ap_ready);
    @(posedge ap_clk);
    ap_start = 0;

    wait(ap_done);
    #100;
    $display("Test Done!", $time);
end
endtask

endmodule

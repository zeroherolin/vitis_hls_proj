`timescale 1ns / 100ps

`define X_LEN 2048

module tb_full_filter;

reg ap_clk;
reg ap_rst;
reg ap_start;

// DUT interface
wire [5:0]        ap_done;
wire [5:0]        ap_idle;
wire [5:0]        ap_ready;
wire [63:0]       x_dout [0:5];
wire [5:0]        x_empty_n;
wire [5:0]        x_read;
reg  [31:0]       jx_values [0:5];
reg  [31:0]       gamma_values [0:5];
wire [31:0]       x_judgment_din [0:5];
wire [5:0]        x_judgment_full_n;
wire [5:0]        x_judgment_write;
wire [31:0]       max_out [0:5];
wire [5:0]        max_out_ap_vld;

always #5 ap_clk = ~ap_clk; // 100MHz

generate
genvar i;
for (i=0; i<6; i=i+1) begin : dut_gen
    filter dut (
        .ap_clk(ap_clk),
        .ap_rst(ap_rst),
        .ap_start(ap_start),
        .ap_done(ap_done[i]),
        .ap_idle(ap_idle[i]),
        .ap_ready(ap_ready[i]),
        .x_dout(x_dout[i]),
        .x_empty_n(x_empty_n[i]),
        .x_read(x_read[i]),
        .jx(jx_values[i]),
        .gamma(gamma_values[i]),
        .x_judgment_din(x_judgment_din[i]),
        .x_judgment_full_n(x_judgment_full_n[i]),
        .x_judgment_write(x_judgment_write[i]),
        .max_out(max_out[i]),
        .max_out_ap_vld(max_out_ap_vld[i])
    );
end
endgenerate

reg [63:0] input_data [0:`X_LEN-1];
reg [15:0] data_ptr [0:5];

generate
for (i=0; i<6; i=i+1) begin : data_assign
    assign x_dout[i] = input_data[data_ptr[i]];
    assign x_empty_n[i] = (data_ptr[i] < `X_LEN);
    assign x_judgment_full_n[i] = 1'b1;
end
endgenerate

always @(posedge ap_clk) begin
    if (ap_rst) begin
        foreach (data_ptr[i]) data_ptr[i] <= 0;
    end else begin
        for (int i=0; i<6; i++) begin
            if (x_read[i] && x_empty_n[i]) begin
                data_ptr[i] <= data_ptr[i] + 1;
            end
        end
    end
end

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

    initialize_param();
    initialize_input_data();

    #100;
    ap_rst = 0;
    #100;
end
endtask

task initialize_param;
begin
    jx_values[0] = 1;
    jx_values[1] = 1;
    jx_values[2] = -1;
    jx_values[3] = -1;
    jx_values[4] = -1;
    jx_values[5] = 1;
    gamma_values[0] = $shortrealtobits(200000000.0/3.0e-6);
    gamma_values[1] = $shortrealtobits(200000000.0/4.0e-6);
    gamma_values[2] = $shortrealtobits(200000000.0/3.0e-6);
    gamma_values[3] = $shortrealtobits(200000000.0/5.0e-6);
    gamma_values[4] = $shortrealtobits(200000000.0/4.0e-6);
    gamma_values[5] = $shortrealtobits(200000000.0/5.0e-6);
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

reg [31:0] zhzq;
reg zhzq_vld;
initial zhzq_vld = 1'b0;

task run_test;
begin
    $display("[%0t] Starting test...", $time);
    ap_start = 1;

    wait(ap_ready);
    @(posedge ap_clk);
    ap_start = 0;

    wait(ap_done);
    #100;

    begin : zhzq_calculation
        for (int j = 0; j < 5; j++) begin
            if (max_out_ap_vld[j] && 
                ($bitstoshortreal(max_out[j]) >= 1000.0)) 
            begin
                zhzq = 5 - j;
                zhzq_vld = 1'b1;
                $display("[%0t] Found valid zhzq = %0d at DUT%0d", $time, zhzq, j);
                disable zhzq_calculation;
            end
        end
        $display("[%0t] No valid zhzq found", $time);
    end

    $display("[%0t] All DUTs Test Completed! zhzq = %0d", $time, zhzq);
end
endtask

endmodule

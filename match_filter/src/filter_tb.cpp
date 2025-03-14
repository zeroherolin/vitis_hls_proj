#include <hls_stream.h>
#include <hls_math.h>
#include <fstream>
#include <iostream>
#include "filter.h"

void read_input_from_file(
    const char* real_file,
    const char* imag_file,
    hls::stream<cplx> &out_stream,
    int length
){
    std::ifstream fr(real_file);
    std::ifstream fi(imag_file);

    if(!fr.is_open() || !fi.is_open()){
        std::cerr << "Error opening input files!" << std::endl;
        return;
    }

    for(int i = 0; i < length; i++){
        data_t real_part, imag_part;
        fr >> real_part;
        fi >> imag_part;
        out_stream.write(cplx(real_part, imag_part));
    }

    fr.close();
    fi.close();
}

void write_output_to_file(
    const char* filename,
    hls::stream<data_t> &in_stream,
    int length
){
    std::ofstream fout(filename);

    if(!fout.is_open()){
        std::cerr << "Error opening output file!" << std::endl;
        return;
    }

    for(int i = 0; i < length; i++){
        data_t val = in_stream.read();
        fout << val << std::endl;
    }

    fout.close();
}

int main() {
    hls::stream<cplx> x_stream_in;
    hls::stream<cplx> x_cp1;
    hls::stream<data_t> x_judgment1;

    const char* real_file = "../../../../input_x_real.txt";
    const char* imag_file = "../../../../input_x_imag.txt";
    const int data_length = 2048;

    read_input_from_file(real_file, imag_file, x_stream_in, data_length);

    int jx[6] = {1, 1, -1, -1, -1, 1};
    data_t gamma[6] = {BD/3.0e-6, BD/4.0e-6, BD/3.0e-6, BD/5.0e-6, BD/4.0e-6, BD/5.0e-6};
    data_t max_out[6];

    for(int j = 0; j < data_length; j++) {
        cplx x = x_stream_in.read();
        x_cp1.write(x);
    }

    filter_top(x_cp1, jx[0], gamma[0], x_judgment1, max_out[0]);

    const char* out_j1 = "../../../../out_j1.txt";

    write_output_to_file(out_j1, x_judgment1, data_length);

    return 0;
}

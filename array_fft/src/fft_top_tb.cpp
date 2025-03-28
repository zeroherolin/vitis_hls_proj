#include <hls_stream.h>
#include <hls_math.h>
#include <fstream>
#include <iostream>
#include "fft_top.h"

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

int main() {
    hls::stream<cplx> x_stream_in;

    const char* real_file = "../../../../input_x_real.txt";
    const char* imag_file = "../../../../input_x_imag.txt";
    const int data_length = 4096;

    read_input_from_file(real_file, imag_file, x_stream_in, data_length/2);
    read_input_from_file(real_file, imag_file, x_stream_in, data_length/2);

    cplx x_in[data_length];
    cplx x_out[data_length];

    for(int i = 0; i < data_length; i++){
        x_in[i] = x_stream_in.read();
    }

    bool ovflo;

    fft_top(1, x_in, x_out, &ovflo);

    for(int i = 0; i < data_length; i++){
        std::cout << x_out[i] << std::endl;
    }

    return 0;
}


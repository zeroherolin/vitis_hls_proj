#include "fft_top.h"

void dummy_proc_fe(
    bool direction,
    config_t* config,
    cplx in[FFT_LENGTH],
    cplx out[FFT_LENGTH]
) {
    config->setDir(direction);
    config->setSch(0x0);
    config->setNfft(FFT_NFFT_MAX);
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
        out[i] = in[i];
    }
}

void dummy_proc_be(
    status_t* status_in,
    bool* ovflo,
    cplx in[FFT_LENGTH],
    cplx out[FFT_LENGTH]
) {
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
        out[i] = in[i];
    }
    *ovflo = status_in->getOvflo() & 0x1;
}

void fft_top(
    bool direction,
    cplx in[FFT_LENGTH],
    cplx out[FFT_LENGTH],
    bool* ovflo
) {
#pragma HLS INTERFACE mode=ap_none port=direction
#pragma HLS INTERFACE mode=ap_fifo depth=FFT_LENGTH port=in
#pragma HLS INTERFACE mode=ap_fifo depth=FFT_LENGTH port=out
#pragma HLS INTERFACE mode=ap_fifo depth=1 port=ovflo

#pragma HLS DATAFLOW

    cplx xn[FFT_LENGTH] __attribute__((no_ctor));
    cplx xk[FFT_LENGTH] __attribute__((no_ctor));
#pragma HLS STREAM variable=xn
#pragma HLS STREAM variable=xk

    config_t fft_config;
    status_t fft_status;

    dummy_proc_fe(direction, &fft_config, in, xn);

    hls::fft<config1>(xn, xk, &fft_status, &fft_config);

    dummy_proc_be(&fft_status, ovflo, xk, out);
}


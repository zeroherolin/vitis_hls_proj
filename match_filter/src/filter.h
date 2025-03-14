#ifndef __FILTER_H__
#define __FILTER_H__

#include <ap_fixed.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <hls_fft.h>
#include <hls_math.h>

#define BD     200000000.0f
#define FS     512000000.0f
#define TP     4.0e-6f
#define TP_NUM ((int)(TP * FS)) // 2048
#define X_LEN  2048
#define X1_LEN (X_LEN + TP_NUM) // 4096

const char FFT_INPUT_WIDTH  = 32;
const char FFT_OUTPUT_WIDTH = FFT_INPUT_WIDTH;
const char FFT_STATUS_WIDTH = 8;
const bool FFT_HAS_NFFT     = false;
const char FFT_CONFIG_WIDTH = FFT_HAS_NFFT ? 24 : 16;
const char FFT_NFFT_MAX     = 12;
const int  FFT_LENGTH       = 1 << FFT_NFFT_MAX; // 4096

struct config1 : hls::ip_fft::params_t {
    static const bool has_nfft = FFT_HAS_NFFT;
    static const bool ovflo = true;
    static const unsigned input_data_width   = FFT_INPUT_WIDTH;
    static const unsigned output_data_width  = FFT_OUTPUT_WIDTH;
    static const unsigned status_width       = FFT_STATUS_WIDTH;
    static const unsigned config_width       = FFT_CONFIG_WIDTH;
    static const unsigned max_nfft           = FFT_NFFT_MAX;
    static const unsigned phase_factor_width = 24;
    static const unsigned stages_block_ram   = (max_nfft < 10) ? 0 : (max_nfft - 9);
    static const unsigned arch_opt = hls::ip_fft::pipelined_streaming_io;
    static const unsigned ordering_opt = hls::ip_fft::natural_order;
    static const unsigned scaling_opt = hls::ip_fft::scaled;
};

typedef hls::ip_fft::config_t<config1> config_t;
typedef hls::ip_fft::status_t<config1> status_t;

typedef float data_t;
typedef std::complex<data_t> cplx;

void filter_top(
    hls::stream<cplx>& x,
    int jx,
    data_t gamma,
    hls::stream<data_t>& x_judgment,
    data_t &max_out
);

#endif

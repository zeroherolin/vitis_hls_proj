#include "filter.h"

void dummy_proc_fe(
    bool direction,
    config_t* config,
    hls::stream<cplx>& in,
    cplx out[FFT_LENGTH]
) {
    config->setDir(direction);
    config->setSch(0x0);

L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
        out[i] = in.read();
    }
}

void dummy_proc_be(
    status_t* status_in,
    bool* ovflo,
    cplx in[FFT_LENGTH],
    hls::stream<cplx>& out
) {
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
        out.write(in[i]);
    }
    *ovflo = status_in->getOvflo() & 0x1;
}

void dummy_proc_be_ifft(
    status_t* status_in,
    bool* ovflo,
    cplx in[FFT_LENGTH],
    hls::stream<cplx>& out
) {
L0:
    for (int i = 0; i < FFT_LENGTH; i++) {
        cplx temp_in = in[i];
        out.write(cplx(temp_in.real()/FFT_LENGTH, temp_in.imag()/FFT_LENGTH));
    }
    *ovflo = status_in->getOvflo() & 0x1;
}

void fft_top(
    hls::stream<cplx>& in,
    hls::stream<cplx>& out
) {
#pragma HLS DATAFLOW

    cplx xn[FFT_LENGTH] __attribute__((no_ctor));
    cplx xk[FFT_LENGTH] __attribute__((no_ctor));
#pragma HLS STREAM variable=xn
#pragma HLS STREAM variable=xk

    config_t fft_config;
    status_t fft_status;

    bool ovflo;

    dummy_proc_fe(1, &fft_config, in, xn);

    hls::fft<config1>(xn, xk, &fft_status, &fft_config);

    dummy_proc_be(&fft_status, &ovflo, xk, out);
}

void restore_ifft(
    hls::stream<cplx>& input,
    hls::stream<cplx>& output
) {
    cplx buffer[FFT_LENGTH] __attribute__((no_ctor));

L0:
    for (int j = 0; j < FFT_LENGTH; j++) {
#pragma HLS PIPELINE II=1
        buffer[j] = input.read();
    }

#ifdef __SYNTHESIS__

    output.write(buffer[0]);
L1_1:
    for (int i = 0; i < FFT_LENGTH-1; i++) {
#pragma HLS PIPELINE II=1
        output.write(buffer[FFT_LENGTH-i-1]);
    }

#else

L1:
    for (int i = 0; i < FFT_LENGTH; i++) {
#pragma HLS PIPELINE II=1
        output.write(buffer[i]);
    }

#endif
}

void ifft_top(
    hls::stream<cplx>& in,
    hls::stream<cplx>& out
) {
#pragma HLS DATAFLOW

    cplx xn[FFT_LENGTH] __attribute__((no_ctor));
    cplx xk[FFT_LENGTH] __attribute__((no_ctor));
#pragma HLS STREAM variable=xn
#pragma HLS STREAM variable=xk

    hls::stream<cplx> str_ifft;
#pragma HLS STREAM variable=str_ifft depth=FFT_LENGTH

    config_t fft_config;
    status_t fft_status;

    bool ovflo;

    dummy_proc_fe(0, &fft_config, in, xk);

    hls::fft<config1>(xk, xn, &fft_status, &fft_config);

    dummy_proc_be_ifft(&fft_status, &ovflo, xn, str_ifft);

    restore_ifft(str_ifft, out);
}

void fftshift(
    hls::stream<cplx>& input,
    hls::stream<cplx>& output
) {
    const int half = FFT_LENGTH/2;
    cplx buffer[FFT_LENGTH] __attribute__((no_ctor));

SHIFT_LOOP1:
    for (int j = 0; j < FFT_LENGTH; j++) {
#pragma HLS PIPELINE II=1
        buffer[j] = input.read();
    }

SHIFT_LOOP2:
    for (int j = 0; j < FFT_LENGTH; j++) {
#pragma HLS PIPELINE II=1
        int idx = (j < half) ? (j + half) : (j - half);
        output.write(buffer[idx]);
    }
}

void gen_filter(
    int jx,
    data_t gamma,
    hls::stream<cplx>& dout
) {
GEN_FILTER_COEFF:
    for (int j = 0; j < X1_LEN; j++) {
#pragma HLS PIPELINE II=1
        if (j >= X1_LEN/2 - TP_NUM/2 && j < X1_LEN/2 + TP_NUM/2) {
            int idx = j - (X1_LEN/2 - TP_NUM/2);
            int j_actual = idx - TP_NUM/2;
            data_t phase = M_PI * jx * gamma * j_actual/FS * j_actual/FS;
            dout.write(cplx(hls::cos(phase), hls::sin(phase)));
        } else
            dout.write(cplx(0, 0));
    }
}

void conjugate(
    hls::stream<cplx>& din,
    hls::stream<cplx>& dout
) {
CONJUGATE_PROCESS:
    for (int j = 0; j < X1_LEN; j++) {
#pragma HLS PIPELINE II=1
        cplx str_out = din.read();
        data_t real_part = str_out.real();
        data_t imag_part = str_out.imag();
        dout.write(cplx(real_part, -imag_part));
    }
}

void cplx_mult(
    hls::stream<cplx>& din1,
    hls::stream<cplx>& din2,
    hls::stream<cplx>& dout
) {
COMPLEX_MULT:
    for (int j = 0; j < X1_LEN; j++) {
#pragma HLS PIPELINE II=1
        dout.write(din1.read() * din2.read());
    }
}

void out_select(
    hls::stream<cplx>& din,
    hls::stream<data_t>& dout
) {
OUTPUT_SELECTION:
    for (int j = 0; j < X1_LEN; j++) {
#pragma HLS PIPELINE II=1
        cplx out_scale = din.read();
        if (j >= TP_NUM/2 && j < X1_LEN - TP_NUM/2) {
            data_t out_real = out_scale.real();
            data_t out_imag = out_scale.imag();
            data_t out_sq = out_real * out_real + out_imag * out_imag;
            dout.write(hls::sqrt(out_sq));
        }
    }
}

void match_filter(
    int jx,
    data_t gamma,
    hls::stream<cplx>& x_in, // x_1
    hls::stream<data_t>& x_out
) {
    hls::stream<cplx> self_1;
    hls::stream<cplx> str_shift1;
    hls::stream<cplx> str_fft1;
    hls::stream<cplx> str_shift2;
    hls::stream<cplx> str_conj1;

    hls::stream<cplx> str_fft2;
    hls::stream<cplx> str_shift3;
    hls::stream<cplx> str_mult1;
    hls::stream<cplx> str_shift4;
    hls::stream<cplx> str_ifft1;

#pragma HLS STREAM variable=self_1 depth=X1_LEN
#pragma HLS STREAM variable=str_shift1 depth=X1_LEN
#pragma HLS STREAM variable=str_fft1 depth=X1_LEN
#pragma HLS STREAM variable=str_shift2 depth=X1_LEN
#pragma HLS STREAM variable=str_conj1 depth=X1_LEN

#pragma HLS STREAM variable=str_fft2 depth=X1_LEN
#pragma HLS STREAM variable=str_shift3 depth=X1_LEN
#pragma HLS STREAM variable=str_mult1 depth=X1_LEN
#pragma HLS STREAM variable=str_shift4 depth=X1_LEN
#pragma HLS STREAM variable=str_ifft1 depth=X1_LEN

#pragma HLS DATAFLOW

    bool ovflo;

    gen_filter(jx, gamma, self_1);

    fftshift(self_1, str_shift1);

    fft_top(str_shift1, str_fft1);

    fftshift(str_fft1, str_shift2);

    conjugate(str_shift2, str_conj1);

    fft_top(x_in, str_fft2);

    fftshift(str_fft2, str_shift3);

    cplx_mult(str_shift3, str_conj1, str_mult1);

    fftshift(str_mult1, str_shift4);

    ifft_top(str_shift4, str_ifft1);

    out_select(str_ifft1, x_out);
}

void find_max(
    hls::stream<data_t> &din,
    data_t &max_out
){
    max_out = 0;
FIND_MAX:
    for(int j = 0; j < X_LEN; j++) {
#pragma HLS PIPELINE II=1
        data_t read_temp = din.read();
        if (read_temp > max_out) max_out = read_temp;
    }
}

void gen_x1(
    hls::stream<cplx>& x,
    hls::stream<cplx>& x1
) {
GEN_X1:
    for(int j = 0; j < X1_LEN; j++) {
#pragma HLS PIPELINE II=1
        cplx x_1;
        if(j >= TP_NUM/2 && j < X1_LEN - TP_NUM/2)
            x_1 = x.read();
        else
            x_1 = cplx(0, 0);
        x1.write(x_1);
    }
}

void shunt_judgment(
    hls::stream<data_t>& din,
    hls::stream<data_t>& dout1,
    hls::stream<data_t>& dout2
) {
SHUNT_JUDGMENT:
    for(int j = 0; j < X_LEN; j++) {
#pragma HLS PIPELINE II=1
        data_t tmp1 = din.read();
        dout1.write(tmp1);
        dout2.write(tmp1);
    }
}

void filter_top(
    hls::stream<cplx>& x,
    int jx,
    data_t gamma,
    hls::stream<data_t>& x_judgment,
    data_t &max_out
) {
#pragma HLS INTERFACE mode=ap_fifo port=x
#pragma HLS INTERFACE mode=ap_none port=jx
#pragma HLS INTERFACE mode=ap_none port=gamma
#pragma HLS INTERFACE mode=ap_fifo port=x_judgment
#pragma HLS INTERFACE mode=ap_vld port=max_out

#pragma HLS DATAFLOW

    hls::stream<cplx> x_1;
    hls::stream<data_t> x_j;
    hls::stream<data_t> x_jc;

#pragma HLS STREAM variable=x_1 depth=X1_LEN
#pragma HLS STREAM variable=x_j depth=X_LEN
#pragma HLS STREAM variable=x_jc depth=X_LEN

    gen_x1(x, x_1);

    match_filter(jx, gamma, x_1, x_j);

    shunt_judgment(x_j, x_jc, x_judgment);

    find_max(x_jc, max_out);
}

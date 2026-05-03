#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <fftw3.h>
#include <stdexcept>
#include <complex>

struct FFTWPlan
{
    std::vector<float> window;
    fftwf_complex *in = nullptr;
    fftwf_complex *out = nullptr;
    fftwf_plan plan = nullptr;

    FFTWPlan(int size, bool direction = true) : window(size)
    {
        for (int i = 0; i < size; ++i)
            window[i] = 0.5f - 0.5f * std::cos(2.0f * float(M_PIf) * float(i) / float(size - 1));

        in = reinterpret_cast<fftwf_complex *>(fftwf_malloc(sizeof(fftwf_complex) * size));
        out = reinterpret_cast<fftwf_complex *>(fftwf_malloc(sizeof(fftwf_complex) * size));
        if (!in || !out)
            throw std::bad_alloc{};

        plan = fftwf_plan_dft_1d(size, in, out, direction ? FFTW_FORWARD : FFTW_BACKWARD, FFTW_MEASURE);
        if (!plan)
            throw std::runtime_error("fftwf_plan_dft_1d failed");
    }

    ~FFTWPlan()
    {
        if (plan)
            fftwf_destroy_plan(plan);
        if (in)
            fftwf_free(in);
        if (out)
            fftwf_free(out);
    }

    FFTWPlan(FFTWPlan &&other) noexcept
        : window(std::move(other.window)),
          in(other.in),
          out(other.out),
          plan(other.plan)
    {
        other.in = nullptr;
        other.out = nullptr;
        other.plan = nullptr;
    }

    FFTWPlan &operator=(FFTWPlan &&other) noexcept
    {
        if (this != &other)
        {
            if (plan)
                fftwf_destroy_plan(plan);
            if (in)
                fftwf_free(in);
            if (out)
                fftwf_free(out);

            window = std::move(other.window);
            in = other.in;
            out = other.out;
            plan = other.plan;

            other.in = nullptr;
            other.out = nullptr;
            other.plan = nullptr;
        }
        return *this;
    }
    FFTWPlan(const FFTWPlan &) = delete;
    FFTWPlan &operator=(const FFTWPlan &) = delete;
};

struct OFDMConfig
{
    size_t n_subcarriers = 128;
    size_t n_cp = 32;
    size_t pilots_spacing = 19;
    std::complex<float> pilot = {1.0f, 0.0f};
};

struct beam
{
    float base_distance;
    float distance;
    float coefficient;
    size_t absolute_offset;
    float mult;
};

struct FEC
{
    int block_size = 32;
    int overhead = 6;
    int bytes_size = block_size / 8;
    int block_bits = block_size + overhead;
    size_t rows = 5;
    size_t cols = block_size + overhead;
    size_t super_bits = rows * cols;
};

class MultipathChannel
{
public:
    void pass_through(const std::vector<std::complex<float>> &signal_in, std::vector<std::complex<float>> &signal_out);
    void set_paths(size_t beams = 4, float sample_rate = 1.92e6, float carrier = 2.2e9);
    void set_noise(float db);
    void update_paths(float coefficient);

private:
    size_t beams_count = 4;
    std::vector<beam> beams;
    float noise_power;
    float sigma;
    float sample_rate;
    float carrier;
};

enum class Modulation
{
    BPSK,
    QPSK,
    QAM16,
    QAM64,
};

void encoder(const char *letters, std::vector<std::uint8_t> &bits);
void decoder(const std::vector<std::uint8_t> &bits, char *letters);
std::vector<uint8_t> hamming_encode(const std::vector<uint8_t> &data);
std::vector<uint8_t> hamming_decode(const std::vector<uint8_t> &data);
std::vector<uint8_t> interleave(const std::vector<uint8_t> &input, size_t rows, size_t cols);
std::vector<uint8_t> deinterleave(const std::vector<uint8_t> &input, size_t rows, size_t cols);

void fec_encoding(std::vector<uint8_t> &bits, std::vector<uint8_t> &output, FEC &config);
void fec_decoding(std::vector<uint8_t> &bits, std::vector<uint8_t> &output, FEC &config);
float get_ber(const std::vector<uint8_t> &tx, const std::vector<uint8_t> &rx);

std::vector<uint8_t> to_bits(const std::vector<uint8_t> &data);
std::vector<uint8_t> from_bits(const std::vector<uint8_t> &bits);

void generate_bits(std::vector<uint8_t> &bits, size_t L = 1000);
void ofdm(const std::vector<std::complex<float>> &symbols, std::vector<std::complex<float>> &buffer, OFDMConfig &OFDMConfig);
void qpsk_mapper_3gpp(const std::vector<uint8_t> &bits, std::vector<std::complex<float>> &symbols);
void qpsk_demapper_3gpp(const std::vector<std::complex<float>> &symbols, std::vector<uint8_t> &bits);
void demodulate_ofdm(const std::vector<std::complex<float>> &input, std::vector<std::complex<float>> &output, OFDMConfig &config, int start);
void ofdm_equalize(std::vector<std::complex<float>> &input, std::vector<std::complex<float>> &output, OFDMConfig &config);
void compute_spectrum(const std::vector<std::complex<float>> &input, std::vector<float> &output, std::vector<float> &freqs, float sample_rate);

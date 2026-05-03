#include <funcs.h>

#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <complex>
#include <random>

static bool is_pow2(int x)
{
    return x && !(x & (x - 1));
}

void encoder(const char *letters, std::vector<std::uint8_t> &bits)
{
    for (int i = 0; i < bits.size(); ++i)
        bits[i] = (std::uint8_t)letters[i];
}

void decoder(const std::vector<std::uint8_t> &bits, char *letters)
{
    for (int i = 0; i < bits.size(); ++i)
        letters[i] = (char)bits[i];
}

std::vector<uint8_t> hamming_encode(const std::vector<uint8_t> &bits)
{
    std::size_t m = bits.size();
    std::size_t r = 0;
    while ((1ULL << r) < m + r + 1)
        r++;

    std::size_t n = m + r;
    std::vector<int> out(n + 1, 0);

    for (std::size_t i = 1, j = 0; i <= n; ++i)
        if (!is_pow2(i))
            out[i] = bits[j++];

    for (std::size_t i = 0; i < r; ++i)
    {
        std::size_t p = 1ULL << i;
        int x = 0;
        for (std::size_t j = 1; j <= n; ++j)
            if (j & p)
                x ^= out[j];
        out[p] = x;
    }

    return std::vector<uint8_t>(out.begin() + 1, out.end());
}

std::vector<uint8_t> hamming_decode(const std::vector<uint8_t> &bits)
{
    std::size_t n = bits.size();
    std::size_t r = 0;
    while ((1ULL << r) < n - r + 1)
        r++;

    std::vector<int> in(n + 1);
    for (std::size_t i = 1; i <= n; ++i)
        in[i] = bits[i - 1];

    std::size_t syndrome = 0;
    for (std::size_t i = 0; i < r; ++i)
    {
        std::size_t p = 1ULL << i;
        int x = 0;
        for (std::size_t j = 1; j <= n; ++j)
            if (j & p)
                x ^= in[j];
        if (x)
            syndrome |= p;
    }

    if (syndrome && syndrome <= n)
        in[syndrome] ^= 1;

    std::vector<uint8_t> out;
    for (std::size_t i = 1; i <= n; ++i)
        if (!is_pow2(i))
            out.push_back(in[i]);

    return out;
}

inline uint8_t get_bit(const std::vector<uint8_t> &data, size_t idx)
{
    return (data[idx / 8] >> (7 - (idx % 8))) & 1;
}

inline void set_bit(std::vector<uint8_t> &data, size_t idx, uint8_t val)
{
    if (val)
        data[idx / 8] |= (1 << (7 - (idx % 8)));
    else
        data[idx / 8] &= ~(1 << (7 - (idx % 8)));
}

std::vector<uint8_t> to_bits(const std::vector<uint8_t> &data)
{
    std::vector<uint8_t> bits;
    for (uint8_t b : data)
        for (int i = 7; i >= 0; --i)
            bits.push_back((b >> i) & 1);
    return bits;
}

std::vector<uint8_t> from_bits(const std::vector<uint8_t> &bits)
{
    std::vector<uint8_t> result;
    uint8_t cur = 0;
    int cnt = 0;
    for (int b : bits)
    {
        cur = (cur << 1) | b;
        if (++cnt == 8)
        {
            result.push_back(cur);
            cur = 0;
            cnt = 0;
        }
    }
    if (cnt)
        result.push_back(cur << (8 - cnt));
    return result;
}

std::vector<uint8_t> interleave(const std::vector<uint8_t> &bits, size_t rows, size_t cols)
{
    if (bits.size() != rows * cols)
        throw std::runtime_error("bits for interleave != rows x cols");
    std::vector<uint8_t> out(rows * cols, 0);
    for (std::size_t r = 0; r < rows; ++r)
        for (std::size_t c = 0; c < cols; ++c)
            out[c * rows + r] = bits[r * cols + c];
    return out;
}

std::vector<uint8_t> deinterleave(const std::vector<uint8_t> &bits, size_t rows, size_t cols)
{
    if (bits.size() != rows * cols)
        throw std::runtime_error("bits for deinterleave != rows x cols");
    std::vector<uint8_t> out(rows * cols, 0);
    for (std::size_t r = 0; r < rows; ++r)
        for (std::size_t c = 0; c < cols; ++c)
            out[r * cols + c] = bits[c * rows + r];
    return out;
}

void fec_encoding(std::vector<uint8_t> &bits, std::vector<uint8_t> &output, FEC &config)
{
    output.clear();

    size_t block_group = config.block_size * config.rows;
    while (bits.size() % block_group != 0)
        bits.push_back(0);

    size_t num_data_blocks = bits.size() / config.block_size;
    size_t num_fec_frames = num_data_blocks / config.rows;

    for (size_t s = 0; s < num_fec_frames; ++s)
    {
        std::vector<uint8_t> interleave_matrix;

        for (size_t r = 0; r < config.rows; ++r)
        {
            size_t i = s * config.rows + r;

            auto start = bits.begin() + i * config.block_size;
            auto end = start + config.block_size;

            std::vector<uint8_t> block(start, end);

            auto hamming_block = hamming_encode(block);
            interleave_matrix.insert(interleave_matrix.end(), hamming_block.begin(), hamming_block.end());
        }

        auto interleaved = interleave(interleave_matrix, config.rows, config.cols);
        output.insert(output.end(), interleaved.begin(), interleaved.end());
    }
};

void fec_decoding(std::vector<uint8_t> &bits, std::vector<uint8_t> &output, FEC &config)
{
    output.clear();

    size_t frame_size = config.super_bits;
    size_t num_frames = bits.size() / frame_size;

    for (size_t s = 0; s < num_frames; ++s)
    {
        auto start = bits.begin() + s * frame_size;
        auto end = start + frame_size;

        std::vector<uint8_t> frame(start, end);

        auto deinterleaved = deinterleave(frame, config.rows, config.cols);

        for (size_t r = 0; r < config.rows; ++r)
        {
            auto row_start = deinterleaved.begin() + r * config.cols;
            auto row_end = row_start + config.cols;

            std::vector<uint8_t> row(row_start, row_end);

            auto decoded = hamming_decode(row);
            output.insert(output.end(), decoded.begin(), decoded.end());
        }
    }
}

float get_ber(const std::vector<uint8_t> &tx, const std::vector<uint8_t> &rx)
{
    size_t min_size = std::min(tx.size(), rx.size());
    size_t bit_errors = 0;
    for (size_t i = 0; i < min_size; ++i)
    {
        if (tx[i] != rx[i])
            bit_errors++;
    }

    double ber = (double)bit_errors / min_size;
    return ber;
}

static std::pair<uint8_t, uint8_t> demap_component_3gpp(float val)
{
    uint8_t b_sign = (val < 0.0f) ? 1 : 0;
    uint8_t b_amp = (std::abs(val) < 2.0f) ? 0 : 1;
    return {b_sign, b_amp};
}

void qpsk_mapper_3gpp(const std::vector<uint8_t> &bits, std::vector<std::complex<float>> &symbols)
{
    size_t symbols_count = bits.size() / 2;
    symbols.resize(symbols_count);
    for (size_t i = 0; i < symbols_count; ++i)
        symbols[i] = std::complex<float>(
                         bits[2 * i + 0] * -2.0 + 1.0,
                         bits[2 * i + 1] * -2.0 + 1.0) /
                     sqrtf(2.0);
}

void qpsk_demapper_3gpp(const std::vector<std::complex<float>> &symbols, std::vector<uint8_t> &bits)
{
    bits.resize(symbols.size() * 2);

    for (size_t i = 0; i < symbols.size(); ++i)
    {
        bits[2 * i + 0] = demap_component_3gpp(symbols[i].real()).first;
        bits[2 * i + 1] = demap_component_3gpp(symbols[i].imag()).first;
    }
}

void calculate_symbol(OFDMConfig &OFDMConfig, std::vector<int> &pilots, std::vector<int> &data, std::vector<bool> &is_pilot, std::vector<bool> &is_guard)
{
    size_t N = static_cast<size_t>(OFDMConfig.n_subcarriers);
    int PS = OFDMConfig.pilots_spacing;

    data.clear();
    pilots.clear();
    is_pilot.resize(N, false);
    is_guard.resize(N, false);

    int counter = 0;
    for (size_t k = 0; k < N; ++k)
    {
        if (k == 0 || (k >= 37 && k <= 91))
        {
            is_guard[k] = true;
            continue;
        }
        if ((counter % PS == 0) || (k == N / 2 - 28) || (k == N / 2 + 28) || (k == N - 1))
        {
            pilots.push_back(k);
            is_pilot[k] = true;
        }
        else
            data.push_back(k);
        counter++;
    }
};

void ofdm(const std::vector<std::complex<float>> &symbols, std::vector<std::complex<float>> &buffer, OFDMConfig &OFDMConfig)
{
    int Ncp = OFDMConfig.n_cp;
    int N = OFDMConfig.n_subcarriers;
    int PS = OFDMConfig.pilots_spacing;
    static auto pilot = OFDMConfig.pilot;

    if (N < 4 or PS < 2)
        return;

    static FFTWPlan ifft(N, false);

    int total_symbols = (int)symbols.size();
    std::vector<int> data;
    std::vector<int> pilots;
    std::vector<bool> is_guard;
    std::vector<bool> is_pilot;
    calculate_symbol(OFDMConfig, pilots, data, is_pilot, is_guard);

    int symbols_per_ofdm = static_cast<int>(data.size());
    int num_ofdm_symbols = (total_symbols + symbols_per_ofdm - 1) / symbols_per_ofdm;

    buffer.reserve((num_ofdm_symbols + Ncp) * (N + 2));
    buffer.clear();

    for (int sym = 0; sym < num_ofdm_symbols; ++sym)
    {
        for (int i = 0; i < N; ++i)
        {
            ifft.in[i][0] = 0.0f;
            ifft.in[i][1] = 0.0f;
        }

        for (int k : pilots)
        {
            ifft.in[k][0] = pilot.real();
            ifft.in[k][1] = pilot.imag();
        }

        for (int i = 0; i < data.size(); ++i)
        {
            int idx = sym * symbols_per_ofdm + i;
            int k = data[i];

            if (idx < total_symbols)
            {
                ifft.in[k][0] = std::real(symbols[idx]);
                ifft.in[k][1] = std::imag(symbols[idx]);
            }
            else
            {
                ifft.in[k][0] = 0.0f;
                ifft.in[k][1] = 0.0f;
            }
        }

        fftwf_execute(ifft.plan);

        for (int n = 0; n < N; ++n)
        {
            ifft.out[n][0] /= static_cast<float>(N) / 10;
            ifft.out[n][1] /= static_cast<float>(N) / 10;
        }

        // Cyclic Prefix
        for (int n = N - Ncp; n < N; ++n)
            buffer.push_back({ifft.out[n][0], ifft.out[n][1]});

        // Data
        for (int n = 0; n < N; ++n)
            buffer.push_back({ifft.out[n][0], ifft.out[n][1]});
    }
}

void ofdm_equalize(std::vector<std::complex<float>> &input, std::vector<std::complex<float>> &output, OFDMConfig &config)
{
    static size_t N = config.n_subcarriers;
    static auto known_pilot = config.pilot;

    float accumulated_phase = 0;
    std::vector<std::complex<float>> temp = input;
    output.clear();

    std::vector<int> pilots;
    std::vector<int> data;
    std::vector<bool> is_pilot(N, false);
    std::vector<bool> is_guard(N, false);

    calculate_symbol(config, pilots, data, is_pilot, is_guard);

    std::vector<std::complex<float>> H_prev(N, {1, 0});

    for (size_t i = 0; i + N <= temp.size(); i += N)
    {
        std::vector<std::complex<float>> sym(temp.begin() + i, temp.begin() + i + N);

        std::vector<std::complex<float>> H(N, {0, 0});
        std::vector<std::complex<float>> equalized(N);

        for (auto k : pilots)
            H[k] = sym[k] / known_pilot;

        for (size_t p = 0; p < pilots.size() - 1; ++p)
        {
            int k1 = pilots[p];
            int k2 = pilots[p + 1];

            auto H1 = H[k1];
            auto H2 = H[k2];

            float a1 = std::arg(H1);
            float a2 = std::arg(H2);

            float da = a2 - a1;
            if (da > M_PIf)
                da -= 2 * M_PIf;
            if (da < -M_PIf)
                da += 2 * M_PIf;

            float m1 = std::abs(H1);
            float m2 = std::abs(H2);

            for (int k = k1 + 1; k < k2; ++k)
            {
                if (is_guard[k])
                    continue;

                float alpha = float(k - k1) / float(k2 - k1);

                float a = a1 + alpha * da;
                float m = m1 + alpha * (m2 - m1);

                H[k] = std::polar(m, a);
            }
        }

        for (int k = 0; k < pilots.front(); ++k)
            if (!is_guard[k])
                H[k] = H[pilots.front()];

        for (int k = pilots.back() + 1; k < N; ++k)
            if (!is_guard[k])
                H[k] = H[pilots.back()];

        for (int k = 1; k < N; ++k)
            if (std::abs(H[k]) > 1e-12f)
                equalized[k] = sym[k] / H[k];
            else
                equalized[k] = sym[k];

        float cpe = 0;
        for (auto k : pilots)
            cpe += std::arg(equalized[k] / known_pilot);
        cpe /= pilots.size();

        accumulated_phase += cpe;

        float mean_amp_pilots = 0;
        for (auto k : pilots)
            mean_amp_pilots += std::abs(equalized[k]);
        mean_amp_pilots /= pilots.size();

        for (int k = 0; k < N; ++k)
            if (!is_guard[k])
                equalized[k] /= mean_amp_pilots;

        std::complex<float> rot = std::exp(std::complex<float>(0, -accumulated_phase));
        for (int k = 0; k < N; ++k)
            if (!is_guard[k])
                equalized[k] *= rot;

        for (int k = 0; k < N; ++k)
            if (!is_pilot[k] and !is_guard[k])
                output.push_back(equalized[k]);
    }
}

void MultipathChannel::pass_through(const std::vector<std::complex<float>> &signal_in, std::vector<std::complex<float>> &signal_out)
{
    if (signal_in.empty() || beams_count == 0)
        return;

    size_t max_offset = beams[beams_count - 1].absolute_offset;
    signal_out.assign(signal_in.size() + max_offset, {0.0f, 0.0f});

    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> dis(0.0f, sigma);

    for (size_t i = 0; i < beams_count; ++i)
    {
        size_t offset = beams[i].absolute_offset;
        float gain = beams[i].coefficient;

        for (size_t k = 0; k < signal_in.size(); ++k)
            signal_out[k + offset] += signal_in[k] * gain + std::complex<float>(dis(gen), dis(gen));
    }
    signal_out.resize(signal_in.size(), {0.0f, 0.0f});
}

void MultipathChannel::set_paths(size_t beams_count_, float sample_rate, float carrier)
{
    // Настройка генератора случайных чисел
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(10.0f, 500.0f);

    this->beams_count = beams_count_;
    this->sample_rate = sample_rate;
    this->carrier = carrier;
    beams.resize(beams_count);

    float c = 3e8f;
    float Ts = 1.0f / sample_rate;

    for (int i = 0; i < beams_count; ++i)
        beams[i].base_distance = dis(gen);

    std::sort(beams.begin(), beams.begin() + beams_count, [](const beam &a, const beam &b)
              { return a.base_distance < b.base_distance; });

    // Beams configure
    for (int i = 0; i < beams_count; ++i)
    {
        beams[i].absolute_offset = static_cast<size_t>(std::round((beams[i].distance - beams[0].base_distance) / (c * Ts)));
        beams[i].coefficient = c / (4 * M_PIf * beams[i].base_distance * carrier);
    }
};

void demodulate_ofdm(const std::vector<std::complex<float>> &input, std::vector<std::complex<float>> &output, OFDMConfig &config, int start)
{
    size_t CP = config.n_cp;
    size_t N = config.n_subcarriers;
    size_t symbol_len = N + CP;

    static FFTWPlan fft(N, true);
    output.clear();

    size_t offset = start;

    size_t num_symbols = (input.size() > offset) ? (input.size() - offset) / symbol_len : 0;

    for (size_t k = 0; k < num_symbols; ++k)
    {
        size_t idx = offset + k * symbol_len + CP;

        if (idx + N > input.size())
            break;

        for (size_t i = 0; i < N; ++i)
        {
            fft.in[i][0] = std::real(input[idx + i]);
            fft.in[i][1] = std::imag(input[idx + i]);
        }
        fftwf_execute(fft.plan);

        for (size_t n = 0; n < N; ++n)
            output.push_back({fft.out[n][0], fft.out[n][1]});
    }
}

void MultipathChannel::set_noise(float db)
{
    noise_power = std::pow(10.0f, db / 10.0f);
    sigma = std::sqrt(noise_power / 2.0f);
};

void MultipathChannel::update_paths(float coefficient)
{
    float c = 3e8f;
    float Ts = 1.0f / sample_rate;

    for (int i = 0; i < beams_count; ++i)
    {
        float d = beams[i].base_distance * (1.0f + (coefficient - 1.0f) * (i + 1));
        beams[i].absolute_offset = static_cast<size_t>(
            std::round((d - beams[0].base_distance) / (c * Ts)));
        beams[i].coefficient = c / (4 * M_PIf * d * carrier);
    }
}

void generate_bits(std::vector<uint8_t> &bits, size_t L)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);

    bits.resize(L);

    for (size_t i = 0; i < L; ++i)
    {
        bits[i] = dis(gen);
    }
};

void compute_spectrum(const std::vector<std::complex<float>> &input, std::vector<float> &output, std::vector<float> &freqs, float sample_rate)
{
    FFTWPlan plan(input.size());
    size_t N = input.size();
    output.resize(N);
    freqs.resize(N);

    for (size_t i = 0; i < N; ++i)
    {
        plan.in[i][0] = input[i].real();
        plan.in[i][1] = input[i].imag();
    }
    fftwf_execute(plan.plan);

    for (size_t i = 0; i < N; ++i)
    {
        float mag = std::sqrt(plan.out[i][0] * plan.out[i][0] +
                              plan.out[i][1] * plan.out[i][1]);
        output[i] = 20.0f * std::log10(mag + 1e-12f);

        int k = (int)i - (int)(N / 2);
        freqs[i] = (float)k * sample_rate / (float)N;
    }

    std::rotate(output.begin(), output.begin() + N / 2, output.end());
}

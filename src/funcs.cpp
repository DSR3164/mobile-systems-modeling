#include <funcs.h>

#include <vector>
#include <cstdint>
#include <iostream>
#include <stdexcept>

static bool is_pow2(int x)
{
    return x && !(x & (x - 1));
}

void encoder(char *letters, std::vector<std::uint8_t> &bits)
{
    for (int i = 0; i < bits.size(); ++i)
        bits[i] = (std::uint8_t)letters[i];
}

void decoder(std::vector<std::uint8_t> bits, char *letters)
{
    for (int i = 0; i < bits.size(); ++i)
        letters[i] = (char)bits[i];
}

std::vector<uint8_t> hamming_encode(const std::vector<uint8_t> &data)
{
    std::vector<int> bits;

    for (uint8_t b : data)
        for (int i = 7; i >= 0; --i)
            bits.push_back((b >> i) & 1);

    size_t m = bits.size();
    size_t r = 8;
    while ((1ULL << r) < m + r + 1) r++;

    size_t n = m + r;
    std::vector<int> out(n + 1, 0);

    for (size_t i = 1, j = 0; i <= n; ++i)
        if (!is_pow2(i))
            out[i] = bits[j++];

    for (size_t i = 0; i < r; ++i)
    {
        size_t p = 1ULL << i;
        int x = 0;

        for (size_t j = 1; j <= n; ++j)
            if (j & p)
                x ^= out[j];

        out[p] = x;
    }

    std::vector<uint8_t> result;
    uint8_t cur = 0;
    int cnt = 0;

    for (size_t i = 1; i <= n; ++i)
    {
        cur = (cur << 1) | out[i];
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

std::vector<uint8_t> hamming_decode(const std::vector<uint8_t> &data)
{
    std::vector<int> bits;

    for (uint8_t b : data)
        for (int i = 7; i >= 0; --i)
            bits.push_back((b >> i) & 1);

    size_t n = bits.size();

    size_t r = 0;
    while ((1ULL << r) < n + 1) r++;

    std::vector<int> in(n + 1);
    for (size_t i = 1; i <= n; ++i)
        in[i] = bits[i - 1];

    size_t syndrome = 0;

    for (size_t i = 0; i < r; ++i)
    {
        size_t p = 1ULL << i;
        int x = 0;

        for (size_t j = 1; j <= n; ++j)
            if (j & p)
                x ^= in[j];

        if (x)
            syndrome |= p;
    }

    if (syndrome && syndrome <= n)
    {
        in[syndrome] ^= 1;
        std::cout << "\n\e[31mОшибка\e[39m на месте " << syndrome << "\n";
    }

    std::vector<int> outbits;

    for (size_t i = 1; i <= n; ++i)
        if (!is_pow2(i))
            outbits.push_back(in[i]);

    std::vector<uint8_t> result;
    uint8_t cur = 0;
    int cnt = 0;

    for (int b : outbits)
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

std::vector<uint8_t> interleave(const std::vector<uint8_t> &input, size_t rows, size_t cols)
{
    size_t total_bits = rows * cols;
    if (input.size() * 8 < total_bits)
        throw std::runtime_error("not enough bits");

    std::vector<uint8_t> output((total_bits + 7) / 8, 0);

    for (size_t r = 0; r < rows; ++r)
    {
        for (size_t c = 0; c < cols; ++c)
        {
            size_t in_idx = r * cols + c;   // запись по строкам
            size_t out_idx = c * rows + r;   // чтение по столбцам

            uint8_t bit = get_bit(input, in_idx);
            set_bit(output, out_idx, bit);
        }
    }

    return output;
}

// deinterleave (обратная операция)
std::vector<uint8_t> deinterleave(const std::vector<uint8_t> &input, size_t rows, size_t cols)
{
    size_t total_bits = rows * cols;
    if (input.size() * 8 < total_bits)
        throw std::runtime_error("not enough bits");

    std::vector<uint8_t> output((total_bits + 7) / 8, 0);

    for (size_t r = 0; r < rows; ++r)
    {
        for (size_t c = 0; c < cols; ++c)
        {
            size_t out_idx = r * cols + c;
            size_t in_idx = c * rows + r;

            uint8_t bit = get_bit(input, in_idx);
            set_bit(output, out_idx, bit);
        }
    }

    return output;
}

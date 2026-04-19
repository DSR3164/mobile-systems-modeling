#pragma once
#include <vector>
#include <cstdint>
#include <cstring>

void encoder(char *letters, std::vector<std::uint8_t> &bits);
void decoder(std::vector<std::uint8_t> bits, char *letters);
std::vector<uint8_t> hamming_encode(const std::vector<uint8_t>& data);
std::vector<uint8_t> hamming_decode(const std::vector<uint8_t>& data);
std::vector<uint8_t> interleave(const std::vector<uint8_t> &input, size_t rows, size_t cols);
std::vector<uint8_t> deinterleave(const std::vector<uint8_t> &input, size_t rows, size_t cols);

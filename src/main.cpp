#include "funcs.h"

#include <iostream>
#include <cmath>

using namespace std;

int main()
{
    srand(time(0)); // 1234569
    size_t rd = rand() % 70 + 30;
    int block_size = 32;
    auto bytes_size = block_size / 8;
    int block_bits = block_size + 8;
    size_t rows = 5;                 // сколько блоков Хэмминга в матрице
    size_t cols = block_size + 6;    // длина одного блока
    size_t super_bits = rows * cols; // 190
    char letters_in[rd];
    char letters_out[rd];
    vector<uint8_t> bytes(rd);
    vector<uint8_t> result;
    vector<uint8_t> receive;
    for (size_t i = 0; i < rd; ++i)
        letters_in[i] = 'a' + rand() % 26;

    encoder(letters_in, bytes);

    std::cerr << "letters_in: ";
    for (size_t i = 0; i < rd; ++i)
        std::cerr << letters_in[i];
    std::cerr << "\n";

    std::cerr << "bytes before pad: ";
    for (size_t i = 0; i < bytes.size(); ++i)
        std::cerr << (char)bytes[i];
    std::cerr << "\n";

    while (bytes.size() % bytes_size != 0)
        bytes.push_back(0);

    while ((bytes.size() / bytes_size) % rows != 0)
        bytes.push_back(0);

    std::cerr << "bytes after pad: ";
    for (size_t i = 0; i < bytes.size(); ++i)
        std::cerr << (char)bytes[i];
    std::cerr << "\n";

    auto all_bits = to_bits(bytes);
    auto num_blocks = bytes.size() / bytes_size;

    size_t num_super = num_blocks / rows;

    for (size_t s = 0; s < num_super; ++s)
    {
        std::vector<uint8_t> matrix;
        for (size_t r = 0; r < rows; ++r)
        {
            size_t i = s * rows + r;
            std::vector<uint8_t> block(all_bits.begin() + i * 32, all_bits.begin() + i * 32 + 32);
            auto hammed = hamming_encode(block); // 38 бит
            matrix.insert(matrix.end(), hammed.begin(), hammed.end());
        }

        auto interleaved = interleave(matrix, rows, cols);
        result.insert(result.end(), interleaved.begin(), interleaved.end());
    }

    size_t burst_start = (result.size() / 3 / super_bits) * super_bits;
    size_t burst_len = 5;
    if (burst_start + burst_len > result.size())
    {
        std::cerr << "burst выходит за границы result\n";
        return 1;
    }

    std::cerr << "Rows: " << rows << "\n"
              << "Cols: " << cols << "\n";
    std::cerr << "Вносим ошибки с позиции " << burst_start
              << " длиной " << burst_len << "\n";

    for (size_t i = burst_start; i < burst_start + burst_len; ++i)
    {
        size_t block_idx = i / block_bits;
        size_t pos_in_block = i % block_bits;
        size_t row = pos_in_block % rows;
        size_t col = pos_in_block / rows;
        std::cerr << "ошибка бит " << i
                  << " блок=" << block_idx
                  << " pos_in_block=" << pos_in_block
                  << " row=" << row << " col=" << col << "\n";
        result[i] ^= 1;
    }

    // Декодирование
    for (size_t s = 0; s < num_super; ++s)
    {
        std::vector<uint8_t> block(result.begin() + s * super_bits, result.begin() + s * super_bits + super_bits);

        auto deinterleaved = deinterleave(block, rows, cols);

        for (size_t r = 0; r < rows; ++r)
        {
            std::vector<uint8_t> row_bits(deinterleaved.begin() + r * cols, deinterleaved.begin() + r * cols + cols);
            auto dehammed = hamming_decode(row_bits); // 32 бита
            receive.insert(receive.end(), dehammed.begin(), dehammed.end());
        }
    }

    auto all_bytes = from_bits(receive);

    all_bytes.resize(rd);

    std::cerr << "rd=" << rd << " all_bytes.size()=" << all_bytes.size() << "\n";
    for (size_t i = 0; i < min(all_bytes.size(), (size_t)8); ++i)
        std::cerr << "all_bytes[" << i << "]=" << (char)all_bytes[i] << "\n";
    std::cerr << "letters_in[0]=" << letters_in[0] << " bytes[0]=" << (char)bytes[0] << "\n";
    decoder(all_bytes, letters_out);

    cout << "\e[39mОригинальное сообщение\e[32m" << endl;
    for (size_t i = 0; i < rd; ++i)
        cout << letters_in[i];
    cout << endl;

    cout << "\e[39mПолученное сообщение\e[32m" << endl;
    for (size_t i = 0; i < rd; ++i)
        cout << letters_out[i];
    cout << endl
         << endl
         << (!memcmp(letters_in, letters_out, rd) ? "Ошибок нет\e[39m" : "\e[31mОшибки есть\e[39m") << endl;
    cout << "\e[39m\e[49m";

    return 0;
}

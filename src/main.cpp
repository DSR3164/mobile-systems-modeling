#include "funcs.h"

#include <iostream>
#include <cmath>

using namespace std;

int main()
{
    srand(time(0));
    size_t rd = rand() % 70 + 30;
    int block_size = 32;
    char letters_in[rd];
    char letters_out[rd];
    vector<uint8_t> bytes(rd);
    vector<uint8_t> result;
    vector<uint8_t> receive;
    for (size_t i = 0; i < rd; ++i)
        letters_in[i] = 'a' + rand() % 26;

    encoder(letters_in, bytes);

    for (size_t i = 0; i < (bytes.size() / block_size); ++i)
    {
        vector<uint8_t> block;
        auto shift = i * block_size;
        block.insert(block.begin(), bytes.begin() + shift, bytes.begin() + block_size + shift);
        auto hammen = hamming_encode(block);
        // auto inter = interleave(hammen, 8, 5);
        result.insert(result.end(), hammen.begin(), hammen.end());
    }

    for (size_t i = 0; i < (result.size() / block_size); ++i)
    {
        vector<uint8_t> block;
        auto shift = i * block_size;
        block.insert(block.begin(), result.begin() + shift, result.begin() + block_size + shift);
        // auto inter = deinterleave(block, 8, 5);
        auto hammen = hamming_decode(block);
        receive.insert(receive.end(), hammen.begin(), hammen.end());
    }
    decoder(receive, letters_out);

    cout << "\e[39mОригинальное сообщение\e[32m" << endl;
    for (size_t i = 0; i < bytes.size(); ++i)
        cout << letters_in[i];
    cout << endl;

    cout << "\e[39mПолученное сообщение\e[32m" << endl;
    for (size_t i = 0; i < bytes.size(); ++i)
        cout << letters_out[i];
    cout << endl
         << endl
         << (!memcmp(letters_in, letters_out, rd) ? "Ошибок нет" : "Ошибки есть") << endl;
    cout << "\e[39m\e[49m";

    return 0;
}

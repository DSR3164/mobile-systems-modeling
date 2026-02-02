#include<funcs.h>

void coder(char *letters, std::vector<std::bitset<8>> &bits)
{
    for (int i = 0; i < bits.size(); ++i)
        bits[i] = std::bitset<8>(letters[i]);
}

void decoder(std::vector<std::bitset<8>> bits, char *letters)
{

    for (int i = 0; i < bits.size(); ++i)
    {
        unsigned long x = bits[i].to_ulong();
        letters[i] = static_cast<unsigned char>(x);
    }
}
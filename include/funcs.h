#pragma once
#include <vector>
#include <bitset>
#include <iostream>
#include <cstdint>
#include <cstring>

void coder(char *letters, std::vector<std::bitset<8>> &bits);
void decoder(std::vector<std::bitset<8>> bits, char *letters);

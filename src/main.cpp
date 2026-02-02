#include "funcs.h"

using namespace std;

int main()
{
    srand(time(0));
    int rd = rand() % 70 + 30;
    char letters_in[rd];
    char letters_out[rd];
    vector<bitset<8>> bits(rd);
    for (int i = 0; i < rd; ++i)
        letters_in[i] = 'a' + rand() % 26;

    coder(letters_in, bits);
    decoder(bits, letters_out);

    for (size_t i = 0; i < bits.size(); ++i)
        cout << letters_in[i];
    cout << endl;
    for (size_t i = 0; i < bits.size(); ++i)
        cout << letters_out[i];
    cout << endl;

    return 0;
}
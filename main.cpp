#include "main.h"

//--------------------------------
// K2, the chess engine
// Author: Sergey Meus (serg_meus@mail.ru)
// Krasnoyarsk Krai, Russia
// 2012-2017
//--------------------------------




#define UNUSED(x) (void)(x)
//--------------------------------
int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    k2chess *chess = new k2chess();
    auto att_squares_w = chess->test_count_attacked_squares(k2chess::white);
    auto att_squares_b = chess->test_count_attacked_squares(k2chess::black);
    auto all_attacks_w = chess->test_count_all_attacks(k2chess::white);
    auto all_attacks_b = chess->test_count_all_attacks(k2chess::white);

    std::cout << att_squares_w << ' ' << att_squares_b << ' ' <<
                 all_attacks_w << ' ' << all_attacks_b << std::endl;

    delete chess;
}

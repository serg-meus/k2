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

    size_t att_squares_w, att_squares_b, all_attacks_w, all_attacks_b;
    k2chess *chess = new k2chess();

    att_squares_w = chess->test_count_attacked_squares(k2chess::white);
    att_squares_b = chess->test_count_attacked_squares(k2chess::black);
    all_attacks_w = chess->test_count_all_attacks(k2chess::white);
    all_attacks_b = chess->test_count_all_attacks(k2chess::black);
    assert(att_squares_w == 18);
    assert(att_squares_b == 18);
    assert(all_attacks_w == 24);
    assert(all_attacks_b == 24);

    assert(chess->FenToBoard((char *)"4k3/8/5n2/5n2/8/8/8/3RK3 w - - 0 1"));
    att_squares_w = chess->test_count_attacked_squares(k2chess::white);
    att_squares_b = chess->test_count_attacked_squares(k2chess::black);
    all_attacks_w = chess->test_count_all_attacks(k2chess::white);
    all_attacks_b = chess->test_count_all_attacks(k2chess::black);
    assert(att_squares_w == 15);
    assert(att_squares_b == 19);
    assert(all_attacks_w == 16);
    assert(all_attacks_b == 21);

    delete chess;
}

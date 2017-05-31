#include "main.h"

//--------------------------------
// K2, the chess engine
// Author: Sergey Meus (serg_meus@mail.ru)
// Krasnoyarsk Krai, Russia
// 2012-2017
//--------------------------------





//--------------------------------
void test_attack_tables(k2chess *chess, size_t att_w, size_t att_b,
                        size_t all_w, size_t all_b)
{
    size_t att_squares_w, att_squares_b, all_attacks_w, all_attacks_b;
    att_squares_w = chess->test_count_attacked_squares(k2chess::white);
    att_squares_b = chess->test_count_attacked_squares(k2chess::black);
    all_attacks_w = chess->test_count_all_attacks(k2chess::white);
    all_attacks_b = chess->test_count_all_attacks(k2chess::black);
    assert(att_squares_w == att_w);
    assert(att_squares_b == att_b);
    assert(all_attacks_w == all_w);
    assert(all_attacks_b == all_b);
}




#define UNUSED(x) (void)(x)
//--------------------------------
int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    k2chess *chess = new k2chess();

    test_attack_tables(chess, 18, 18, 24, 24);

    assert(chess->FenToBoard((char *)"4k3/8/5n2/5n2/8/8/8/3RK3 w - - 0 1"));
    test_attack_tables(chess, 15, 19, 16, 21);

    assert(chess->FenToBoard((char *)
        "4rrk1/p4q2/1p2b3/1n6/1N6/1P2B3/P4Q2/4RRK1 w - - 0 1"));
    test_attack_tables(chess, 33, 33, 48, 48);


    delete chess;
}

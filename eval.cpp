#include "eval.h"

using eval_t = eval::eval_t;
using nn_t = eval::nn_t;


eval_t eval::Eval() {
    eval_t W = eval_t(10000*calc_nn_out(white));
    eval_t B = eval_t(10000*calc_nn_out(black));
    return side == white ? eval_t(W - B) : eval_t(B - W);
}


nn_t eval::calc_nn_out(const bool color) {
    in2 = calc_first_nn_layer(color);
    auto in3 = A2*in2 + B2;
    in3.transform_each(fptr(tanh));
    auto in4 = A3*in3 + B3;
    return in4.at(0, 0);
}


matrix<nn_t, eval::first_layer_size, 1> &
eval::calc_first_nn_layer(bool color) {
    sparce_multiply(color);
    in2 = in2 + B1;
    in2.transform_each(fptr(tanh));
    return in2;
}


void eval::sparce_multiply(const bool color) {
    in2.transform_each([](nn_t) {return 0;}); // clear array
    for (unsigned pc_ix = 0; pc_ix < occupancy_ix; ++pc_ix) {
        u64 cur = bb[color][pc_ix];
        while (cur != 0) {
            const u64 lowbit = lower_bit(cur);
            const u8 coord = trail_zeros(lowbit) ^ u8(color == white ? 0 : 56);
            const int index = int(64*pc_ix + coord);
            for (int i = 0; i < int(first_layer_size); ++i)
                in2.at(i, 0) += A1.at(i, index);
            cur ^= lowbit;
        }
    }
}

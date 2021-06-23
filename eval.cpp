#include "eval.h"


using eval_t = k2eval::eval_t;





//-----------------------------
eval_t k2eval::Eval(const bool stm)
{
    in2 = CalcFirstNNLayer(stm);
    auto in3 = a2*in2 + b2;
    in3.transform_each(tanh);
    auto in4 = a3*in3 + b3;
    auto out = in4.at(0, 0);
    out = out > (1 - out_lim) ? (1 - out_lim) : (out < out_lim ? out_lim : out);
    auto ans = static_cast<eval_t>(-400/1.3*log10(1./out - 1));
    return ans;
}





//-----------------------------
k2eval::matrix<k2eval::nn_t, k2eval::first_layer_size, 1> &
k2eval::CalcFirstNNLayer(bool stm)
{
    in2.transform_each([](nn_t) {return 0;});
    SparceMultiply(stm, true);
    SparceMultiply(!stm, false);
    in2 = in2 + b1;
    in2.transform_each(tanh);
    return in2;
}





//-----------------------------
void k2eval::SparceMultiply(const bool stm, const bool side)
{
    const unsigned sz = board_height*board_width;
    unsigned shft[] = {0, 0, sz, 2*sz, 3*sz, 4*sz, 5*sz - board_width};
    auto mask = exist_mask[stm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        const auto coord = coords[stm][piece_id];
        const auto type = get_type(b[coord]);
        const auto new_coord = stm ? coord :
            get_coord(get_col(coord), 7 - get_row(coord));
        const auto index = (!side)*input_vector_size/2 + shft[type] + new_coord;
        for(unsigned i = 0; i < first_layer_size; ++i)
            in2.at(i, 0) += a1.at(i, index);
    }
}





//-----------------------------
void k2eval::EvalDebug()
{
    std::cout << Eval(wtm) << std::endl;
}





#ifndef NDEBUG
//-----------------------------
void k2eval::RunUnitTests()
{
    k2chess::RunUnitTests();

    matrix<i8, 2, 2> m1({4, 3, 2, -1});
    assert(m1.get_rows() == 2 && m1.get_cols() == 2);
    assert(m1.at(0, 0) == 4 && m1.at(0, 1)== 3 && m1.at(1, 0) == 2 && m1.at(1, 1) == -1);
    auto m2 = m1;
    assert(m2 == m1);
    m2.at(1, 1) = 3;
    assert(m2.at(1, 1) == 3);
    assert(m1.at(1, 1)== -1);
    assert(m2 != m1);
    auto m3 = matrix<i8, 3, 2>({-1, -1, -1, -1, 3, 4});
    assert(m3.get_rows() == 3 && m3.get_cols() == 2);
//    assert(m3 != m1);  // checked at compile time
    assert(m3.at(2, 0) == 3);
    matrix<i8, 3, 2> m4({1, 2, 3, 4, 5, 6});
    auto m5 = m3 + m4;
    assert(m5 == (matrix<i8, 3, 2>({0, 1, 2, 3, 8, 10})));
    auto m6 = m4 - m3;
    assert(m6 == (matrix<i8, 3, 2>({2, 3, 4, 5, 2, 2})));
    matrix<i8, 2, 3> m7({1, -2, 3, -4, 5, -6});
    auto m8 = m7*m6;
    assert(m8.get_cols() == 2 && m8.get_rows() == 2);
    assert(m8 == (matrix<i8, 2, 2>({0, -1, 0, 1})));
}
#endif // NDEBUG

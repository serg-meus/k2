#include "chess.h"
#include "matrix.h"

class eval : public chess {

    public:

    typedef double nn_t;
    typedef i16 eval_t;

    eval_t Eval();

    protected:

    using fptr = nn_t(*)(nn_t);
    static const unsigned
    input_vector_size = 384,
    first_layer_size = 4,
    second_layer_size = 4;
    matrix<nn_t, first_layer_size, input_vector_size> A1;
    matrix<nn_t, first_layer_size, 1> B1;
    matrix<nn_t, second_layer_size, first_layer_size> A2;
    matrix<nn_t, second_layer_size, 1> B2;
    matrix<nn_t, 1, second_layer_size> A3;
    matrix<nn_t, 1, 1> B3;
    matrix<nn_t, first_layer_size, 1> in2;

    std::array<eval_t, king_ix + 1> material;

    nn_t calc_nn_out(const bool color);
    void sparce_multiply(const bool color);
    matrix<nn_t, first_layer_size, 1> & calc_first_nn_layer(bool color);

    public:

    eval() :
    #include "nn_data.h"
    in2({0}),
    material({{100, 390, 410, 600, 800, 16000}})
    {
    }
};

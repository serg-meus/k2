#include "chess.h"
#include "matrix.h"

class eval : public chess {

    typedef u64 (*eval_fptr)(u64 bb, u64 *args);

    public:

    typedef double nn_t;
    typedef i16 eval_t;

    template <class T>
    class vec2
    {
    public:

        T mid, end;

        vec2() : mid(0), end(0) {}
        vec2(const T m, const T e) : mid(m), end(e) {}
        vec2(const vec2& v) : mid(v.mid), end(v.end) {}
        vec2& operator =(const vec2& v) {mid=v.mid; end=v.end; return *this;}
        bool operator ==(const vec2& v) const {return mid==v.mid && end==v.end;}
        bool operator !=(const vec2& v) const {return mid!=v.mid || end!=v.end;}
        vec2 operator +(const vec2& v) const {return vec2(T(mid+v.mid),T(end+v.end));}
        vec2 operator -(const vec2& v) const {return vec2(T(mid-v.mid),T(end-v.end));}
        vec2 operator -() const {return vec2(T(-mid), T(-end));}
        vec2 operator *(const vec2& v) const {return vec2(T(mid*v.mid),T(end*v.end));}
        vec2 operator /(const vec2& v) const {return vec2(T(mid/v.mid),T(end/v.end));}
        vec2 operator *(const T x) const {return vec2(T(mid*x), T(end*x));}
        vec2 operator /(const T x) const {return vec2(T(mid / x), T(end / x));}
        friend vec2 operator *(const T x, const vec2& v)
        {
            return vec2(T(v.mid*x), T(v.end*x));
        }
        void operator +=(const vec2 v) {mid = T(mid + v.mid); end = T(end + v.end);}
        void operator -=(const vec2 v) {mid = T(mid - v.mid); end = T(end - v.end);}
    };

    eval_t Eval_NN();
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

    std::array<eval_t, king_ix + 1> material_values;
    std::array<vec2<eval_t>, king_ix + 1> piece_values;
    vec2<eval_t> pst[king_ix + 1][64];
    vec2<eval_t> pawn_doubled, pawn_isolated, pawn_dbl_iso, pawn_hole,
        pawn_gap, pawn_king_tropism1, pawn_king_tropism2,
        pawn_king_tropism3, pawn_pass0, pawn_pass1, pawn_pass2, pawn_blk_pass0,
        pawn_blk_pass1, pawn_blk_pass2, pawn_pass_connected, pawn_unstoppable;
    std::array<eval_t, 16> mobility_curve;
    std::array<int, 2> material_sum, num_pieces;

    nn_t calc_nn_out(const bool color);
    void sparce_multiply(const bool color);
    matrix<nn_t, first_layer_size, 1> & calc_first_nn_layer(bool color);
    vec2<eval_t> eval_material(bool color);
    vec2<eval_t> eval_pst(bool color);
    u64 double_pawns(bool color);
    u64 isolated_pawns(bool color);
    vec2<eval_t> eval_double_and_isolated(bool color);
    u64 passed_pawns(bool color);
    u64 for_each_set_bit(u64 bitboard, const eval_fptr &foo ,u64 *args);
    static u64 passed_pawn(u64 pawn_bb, u64 *args);
    static u8 distance(u8 coord1, u8 coord2);
    u64 unstoppable_pawns(u64 passers, bool color, bool side_to_move);
    static u64 unstoppable_pawn(u64 passer_bb , u64 *args);
    static u64 unstop_fill(u64 passer_bb, bool color, u64 *kings_bb);
    u64 connected_passers(u64 passers);
    static u64 adjacent_pawn(u64 pawn_bb, u64 *args);
    eval_t pawn_gaps(u64 pawns);
    static u64 one_pawn_gap(u64 pawn_bb, u64 *args);
    u64 pawn_holes(u64 passers, bool color);
    static u64 one_pawn_hole(u64 pawn_bb, u64 *args);
    u64 king_pawn_tropism(u64 passers, bool color, u64 king_bb, u8 dist);
    static u64 tropism(u64 pawn_bb, u64 *args);
    eval_t mobility_piece_type(bool color, u8 piece_index);
    vec2<eval_t> eval_pawns(bool color);
    vec2<eval_t> eval_king_tropism(u64 passers, bool color);
    u64 sum_for_each_set_bit(u64 bitboard, const eval_fptr &foo , u64 *args);
    vec2<eval_t> eval_passers(u64 passers, bool color);
    eval_t interpolate_eval(vec2<eval_t> val);
    vec2<eval_t> eval_king_safety(bool color);
    vec2<eval_t> eval_mobility(bool color);


eval_t row_from_bb(u64 lowbit, bool color) {
    return get_row(trail_zeros(lowbit) ^ u8(color == white ? 0 : 56));
}

static u64 fill_up(u64 given)  {
    u64 tmp = given << 8 | given << 16;
    tmp |= tmp | tmp << 16;
    return tmp | tmp << 32;
}

static u64 fill_down(u64 given) {
    u64 tmp = given >> 8 | given >> 16;
    tmp |= tmp | tmp >> 16;
    return tmp | tmp >> 32;
}

static int shifts(bool color) {
	return color ? -8 : 8;
}

    public:

    eval() :
    #include "nn_data.h"
    in2({0}),
    material_values({{100, 390, 410, 600, 1000, 32000}}),
    piece_values({{{100, 128}, {450, 370}, {470, 390}, {560, 680}, {1170, 1290}}}),
    #include "pst.h"
    #include "eval_features.h"
    material_sum(), num_pieces()
    {
    }
};

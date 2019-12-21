#include "chess.h"
#include <complex>


//--------------------------------
class k2eval : public k2chess
{


public:

    void RunUnitTests();

    k2eval() : material_values {{0, 0}, king_val, queen_val, rook_val,
                bishop_val, knight_val, pawn_val},
    pst
{
{ //king
{{-121,-83},{62,-44},{89,-44},{25,-32}, {-7,-9},  {73,3},   {119,-34},{68,-52}},
{{113,-35}, {126,-7},{91,6},  {123,16}, {69,15},  {124,28}, {117,-7}, {81,-24}},
{{121,-17}, {82,8},  {9,26},  {-79,36}, {-51,32}, {115,43}, {127,27}, {45,-13}},
{{20,-23},  {72,6},  {-63,35},{-103,47},{-96,43}, {-73,38}, {115,7},  {8,-17}},
{{-62,-25}, {2,-3},  {-79,34},{-123,42},{-124,45},{-66,34}, {16,-1}, {-26,-23}},
{{26,-29},  {40,-5}, {-37,23},{-52,31}, {-49,34}, {-41,26}, {65,1},   {36,-24}},
{{27,-45},  {59,-18},{29,-1}, {-37,17}, {5,15},   {11,8},   {66,-14}, {63,-40}},
{{-61,-64}, {41,-46},{17,-25},{-43,-7}, {56,-35}, {-15,-35},{71,-47}, {22,-66}},
},

{ // queen
{{-86,-21},{-26,7},  {-15,-6},{-40,2}, {17,-8}, {-17,-10},{-22,-23},{12,-21}},
{{-69,-15},{-65,-1}, {-27,13},{34,11}, {-35,21},{-2,2},   {-17,-6}, {30,-42}},
{{-38,-16},{-47,-7}, {-7,-1}, {-12,22},{43,21}, {11,1},   {-15,-3}, {-10,-24}},
{{-31,-1}, {-27,8},  {-26,12},{-16,33},{12,23}, {6,12},   {-12,16}, {-17,-6}},
{{6,-13},  {-24,24}, {2,13},  {-13,36},{20,14}, {4,13},   {7,8},    {-14,-10}},
{{-7,-1},  {20,-22}, {8,7},   {4,1},   {8,1},   {1,9},    {9,1},    {8,-11}},
{{-21,-7}, {-8,-1},  {32,-12},{11,-1}, {22,-9}, {11,-15}, {-28,-19},{6,-26}},
{{11,-21}, {-15,-12},{2,-3},  {34,-8}, {2,-7},  {-34,-7}, {-47,-19},{-72,-34}},
},

{ // rook
{{45,14}, {30,11}, {6,18}, {41,14},{65,14}, {16,12}, {9,13},  {-15,17}},
{{23,13}, {11,12}, {45,8}, {45,9}, {24,5},  {34,5},  {-14,13},{2,11}},
{{13,4},  {18,7},  {13,6}, {25,7}, {-12,4}, {-12,1}, {16,-2}, {-19,-5}},
{{-12,3}, {-22,7}, {14,11},{11,1}, {3,1},   {-6,1},  {-60,-3},{-15,-4}},
{{-26,6}, {-15,10},{-3,10},{-10,9},{-9,1},  {-44,6}, {-20,-4},{-21,-5}},
{{-29,4}, {-10,4}, {-17,1},{-20,5},{-12,-3},{-29,-1},{-19,3}, {-39,1}},
{{-25,-1},{-2,-1}, {-22,1},{-5,1}, {-12,-5},{-12,-3},{-38,-3},{-63,-2}},
{{2,-2},  {3,7},   {17,4}, {7,7},  {12,2},  {1,2},   {-42,7}, {10,-24}},
},

{ // bishop
{{-99,-9}, {-50,-15},{-126,1},{-101,1},{-29,-5},{-96,-2},{-35,-12},{-32,-29}},
{{-66,-7}, {-17,-5}, {-36,7}, {-67,-4},{13,-8}, {34,-7}, {7,-13},  {-101,-12}},
{{-42,4},  {-1,-5},  {22,-5}, {1,-1},  {-5,4},  {25,-3}, {-1,1},   {-23,-3}},
{{-8,-6},  {-21,11}, {-2,8},  {43,7},  {22,6},  {30,-4}, {-15,-1}, {-10,-8}},
{{-1,-5},  {6,-2},   {2,11},  {25,13}, {26,5},  {-12,8}, {-1,-4},  {8,-13}},
{{10,-7},  {10,4},   {13,8},  {11,8},  {-2,13}, {22,5},  {6,4},    {11,-4}},
{{2,-16},  {25,-10}, {18,-7}, {-3,-2}, {3,8},   {7,-5},  {41,-5},  {-5,-28}},
{{-29,-24},{8,-15},  {-8,-13},{-2,-4}, {2,-12}, {-6,-12},{-37,-7}, {-35,-7}},
},

{ // knight
{{-126,-89},{-126,-24},{-15,7},{-58,-5},{68,-15},{-126,-16},{-121,-28},{-126,-100}},
{{-126,-5}, {-51,9},   {82,-1},{50,20}, {8,13},  {59,-6},   {16,-15},{-81,-38}},
{{-89,-3},  {33,12},   {62,36},{82,34}, {87,22}, {124,11},  {50,1},  {8,-26}},
{{-1,-3},   {21,27},   {41,47},{82,45}, {36,53}, {101,31},  {9,32},  {37,-8}},
{{-21,3},   {16,16},   {39,42},{32,52}, {47,43}, {42,42},   {47,22}, {9,-4}},
{{-22,-11}, {2,26},    {16,32},{40,38}, {55,32}, {39,26},   {28,13}, {-20,-4}},
{{-32,-25}, {-38,-1},  {9,7},  {27,14}, {8,27},  {16,6},    {-1,1},  {-1,-32}},
{{-61,-44}, {-10,-30}, {-54,-3},{-29,1},{10,-7}, {-4,-7},   {-12,-9},{-34,-59}},
},

{ // pawn
{{0,0},    {0,0},    {0,0},    {0,0},   {0,0},   {0,0},    {0,0},   {0,0},},
{{127,92}, {127,61}, {127,33}, {127,20},{127,40},{127,29}, {108,55},{124,76},},
{{85,25},  {50,22},  {38,8},   {4,-14}, {36,-19},{113,-12},{60,7},  {53,20},},
{{12,7},   {13,-4},  {-2,-8},  {19,-21},{26,-13},{14,-8},  {16,1},  {-12,3},},
{{-19,1},  {-23,-2}, {-18,-7}, {7,-16}, {9,-11}, {-8,-9},  {-20,-6},{-32,-5},},
{{-15,-16},{-30,-10},{-24,-10},{-18,-5},{-9,-3}, {-17,-2}, {-7,-13},{-14,-19},},
{{-14,-10},{-27,-9}, {-48,12}, {-35,-3},{-23,3}, {16,3},   {-7,-3}, {-18,-17},},
{{0,0},    {0,0},    {0,0},    {0,0},   {0,0},   {0,0},    {0,0},   {0,0},},
},
}

    {
        InitPawnStruct();
    }


protected:


    typedef u8 rank_t;
    typedef u8 dist_t;

    // type for storing mid- (real) and endgame (imag) part of eval terms
    typedef std::complex<eval_t> eval_vect;

    const eval_t
    centipawn = 100,
    infinite_score = 32760,
    king_value = 32000,
    material_total = 80,
    rook_max_pawns_for_open_file = 2;

    eval_vect
    pawn_val = {100, 128},
    knight_val = {395, 369},
    bishop_val = {405, 405},
    rook_val = {600, 640},
    queen_val = {1200, 1300},
    king_val = {0, 0},

    pawn_dbl_iso = {56, 49},
    pawn_iso = {15, 25},
    pawn_dbl = {5, 19},
    pawn_hole = {47, 22},
    pawn_gap = {-2, 24},
    pawn_king_tropism1 = {0, 26},
    pawn_king_tropism2 = {0, 15},
    pawn_king_tropism3 = {0, 34},
    pawn_pass0 = {-1, -6},
    pawn_pass1 = {-1, -10},
    pawn_pass2 = {1, 9},
    pawn_blk_pass0 = {-10, -100},
    pawn_blk_pass1 = {6, 56},
    pawn_blk_pass2 = {0, -4},
    pawn_pass_connected = {0, 16},
    pawn_unstoppable = {12, 242},
    king_saf_no_shelter = {22, 0},
    king_saf_no_queen = {20, 20},
    king_saf_attack1 = {20, 0},
    king_saf_attack2 = {88, 0},
    king_saf_central_files = {63, 0},
    rook_last_rank = {21, 0},
    rook_semi_open_file = {22, 0},
    rook_open_file = {58, 0},
    bishop_pair = {47, 47},
    mob_queen = {6, 6},
    mob_rook = {13, 13},
    mob_bishop = {10, 10},
    mob_knight = {0, 0},
    imbalance_king_in_corner = {0, 200},
    imbalance_draw_divider = {32, 32},
    imbalance_multicolor = {30, 57},
    imbalance_no_pawns = {24, 24},
    side_to_move_bonus = {8, 8};

    eval_t initial_score;
    rank_t p_max[board_width + 2][sides], p_min[board_width + 2][sides];
    rank_t (*pawn_max)[sides], (*pawn_min)[sides];
    eval_vect material_values[piece_types + 1];

    eval_vect pst[piece_types][board_height][board_width];


public:


    void EvalDebug(const bool stm);

    eval_vect SetupPosition(const char *p)
    {
        k2chess::SetupPosition(p);
        InitPawnStruct();
        eval_vect cur_eval = InitEvalOfMaterial() + InitEvalOfPST();
        return cur_eval;
    }


protected:


    eval_vect FastEval(const bool stm, const move_c m) const;
    eval_t Eval(const bool stm, eval_vect cur_eval);
    eval_vect InitEvalOfMaterial();
    eval_vect InitEvalOfPST();
    void InitPawnStruct();
    void SetPawnStruct(const coord_t col);
    bool IsPasser(const coord_t col, const bool stm) const;
    bool MakeMove(const move_c move);
    void TakebackMove(const move_c move);

    eval_t GetEvalScore(const bool stm, const eval_vect cur_eval) const
    {
        i32 X, Y;
        X = material[0]/centipawn + 1 + material[1]/centipawn +
                1 - pieces[0] - pieces[1];
        Y = ((cur_eval.real() - cur_eval.imag())*X +
             material_total*cur_eval.imag())/material_total;
        eval_t ans = Y;
        return stm ? ans : -ans;
    }

    dist_t king_dist(const coord_t from_coord, const coord_t to_coord) const
    {
        return std::max(std::abs(get_col(from_coord) - get_col(to_coord)),
                        std::abs(get_row(from_coord) - get_row(to_coord)));
    }

    bool is_same_color(const coord_t coord1, const coord_t coord2) const
    {
        const auto sum_coord1 = get_col(coord1) + get_row(coord1);
        const auto sum_coord2 = get_col(coord2) + get_row(coord2);
        return (sum_coord1 & 1) == (sum_coord2 & 1);
    }

    bool is_king_near_corner(const bool stm) const
    {
        const auto k = king_coord(stm);
        const auto min_dist1 = std::min(king_dist(k, get_coord(0, 0)),
                                        king_dist(k, get_coord(0, max_row)));
        const auto min_dist2 = std::min(king_dist(k, get_coord(max_col, 0)),
                                        king_dist(k, get_coord(max_col,
                                                               max_row)));
        return std::min(min_dist1, min_dist2) <= 1 &&
                (get_col(k) == 0 || get_row(k) == 0 ||
                 get_col(k) == max_col || get_row(k) == max_row);
    }

    eval_vect vect_mul(eval_vect a, eval_vect b)
    {
        eval_vect ans;
        ans.real(a.real()*b.real());
        ans.imag(a.imag()*b.imag());
        return ans;
    }

    eval_vect vect_div(eval_vect a, eval_vect b)
    {
        eval_vect ans;
        ans.real(a.real()/b.real());
        ans.imag(a.imag()/b.imag());
        return ans;
    }

    eval_vect EvalSideToMove(const bool stm)
    {
        return stm ? side_to_move_bonus : -side_to_move_bonus;
    }


private:


    eval_vect EvalPawns(const bool side, const bool stm);
    bool IsUnstoppablePawn(const coord_t col, const bool side,
                           const bool stm) const;
    eval_vect EvalImbalances(const bool stm, eval_vect val);
    void MovePawnStruct(const piece_t movedPiece, const move_c move);
    eval_vect EvalMobility(bool stm);
    eval_vect EvalKingSafety(const bool king_color);
    bool KingHasNoShelter(coord_t k_col, coord_t k_row, const bool stm) const;
    bool Sheltered(const coord_t k_col, coord_t k_row, const bool stm) const;
    attack_t KingSafetyBatteries(const coord_t targ_coord, const attack_t att,
                                 const bool stm) const;
    eval_vect EvalRooks(const bool stm);
    size_t CountAttacksOnKing(const bool stm, const coord_t k_col,
                              const coord_t k_row) const;
    void DbgOut(const char *str, eval_vect val, eval_vect &sum) const;
};

#include "chess.h"
#include <complex>


//--------------------------------
class k2eval : public k2chess
{


public:


    k2eval();
    void RunUnitTests();


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

    pawn_dbl_iso = {46, 49},
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

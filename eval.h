#include "chess.h"

#include <iostream>





//--------------------------------
class k2eval : public k2chess
{

public:
    k2eval();
    void RunUnitTests();

protected:

    typedef i8 pst_t;
    typedef u8 rank_t;
    typedef u8 dist_t;

    const eval_t
    king_value = 32000,
    infinite_score = 32760,

    pawn_val_opn = 100,
    kinght_val_opn = 395,
    bishop_val_opn = 405,
    rook_val_opn = 600,
    queen_val_opn = 1200,
    pawn_val_end = 128,
    kinght_val_end = 369,
    bishop_val_end = 405,
    rook_val_end = 640,
    queen_val_end = 1200,

    pawn_dbl_iso_opn = -15,
    pawn_dbl_iso_end = -55,
    pawn_iso_opn = -15,
    pawn_iso_end = -25,
    pawn_dbl_opn = -5,
    pawn_dbl_end = -15,
    pawn_hole_opn = -30,
    pawn_hole_end = 0,
    pawn_gap_opn = 0,
    pawn_gap_end = -30,
    pawn_king_tropism1 = 30,
    pawn_king_tropism2 = 10,
    pawn_king_tropism3 = 15,
    pawn_pass_1 = 0,
    pawn_pass_2 = 21,
    pawn_pass_3 = 40,
    pawn_pass_4 = 85,
    pawn_pass_5 = 150,
    pawn_pass_6 = 200,
    pawn_blk_pass_1 = 0,
    pawn_blk_pass_2 = 10,
    pawn_blk_pass_3 = 30,
    pawn_blk_pass_4 = 65,
    pawn_blk_pass_5 = 80,
    pawn_blk_pass_6 = 120,
    pawn_pass_opn_divider = 3,
    pawn_pass_connected = 28,
    pawn_unstoppable_1 = 120,
    pawn_unstoppable_2 = 350;

    eval_t val_opn, val_end;
    eval_t initial_score;
    rank_t p_max[board_width + 2][sides], p_min[board_width + 2][sides];
    rank_t (*pawn_max)[sides], (*pawn_min)[sides];
    eval_t material_values_opn[piece_types + 1];
    eval_t material_values_end[piece_types + 1];
    dist_t king_tropism[sides];
    dist_t tropism_factor[2][7];

    const pst_t pst[piece_types][sides][board_height][board_width];
//    std::vector<float> tuning_factors;


public:


    eval_t EvalDebug();

    bool SetupPosition(const char *p)
    {
        bool ans = k2chess::SetupPosition(p);
        InitPawnStruct();
        InitEvalOfMaterialAndPst();
        return ans;
    }


protected:


    struct state_s
    {
        eval_t val_opn;
        eval_t val_end;
        dist_t tropism[sides];
    };

    state_s e_state[prev_states + max_ply]; // eval state for each ply depth
    state_s *state;  // pointer to eval state

    void FastEval(const move_c m);
    eval_t Eval();
    void InitEvalOfMaterialAndPst();
    void InitPawnStruct();
    void SetPawnStruct(const coord_t col);
    bool IsPasser(const coord_t col, const bool stm) const;
    bool MakeMove(const move_c m);
    void TakebackMove(const move_c m);

    void MkEvalAfterFastMove(const move_c m)
    {
        state[ply - 1].val_opn = val_opn;
        state[ply - 1].val_end = val_end;

        auto from_coord = k2chess::state[ply].from_coord;

        MoveKingTropism(from_coord, m, wtm);

        MovePawnStruct(b[m.to_coord], from_coord, m);
    }

    eval_t ReturnEval(const bool stm) const
    {
        i32 X, Y;
        X = material[0]/100 + 1 + material[1]/100 + 1 - pieces[0] - pieces[1];

        Y = ((val_opn - val_end)*X + 80*val_end)/80;
        return stm ? (eval_t)(Y) : (eval_t)(-Y);
    }

    bool is_light(const piece_t piece, const bool stm) const
    {
        return piece != empty_square && (piece & white) == stm;
    }

    bool is_dark(const piece_t piece, const bool stm) const
    {
        return piece != empty_square && (piece & white) != stm;
    }

    dist_t king_dist(const coord_t from_coord, const coord_t to_coord) const
    {
        return std::max(std::abs(get_col(from_coord) - get_col(to_coord)),
                        std::abs(get_row(from_coord) - get_row(to_coord)));
    }


private:

    const size_t opening = 0, endgame = 1;

    void EvalPawns(const bool stm);
    bool IsUnstoppablePawn(const coord_t col, const bool side_of_pawn,
                           const bool stm) const;
    void KingSafety(const bool king_color);
    void MaterialImbalances();
    eval_t KingWeakness(const bool king_color);
    eval_t CountKingTropism(const bool king_color);
    void MoveKingTropism(const coord_t from_coord, const move_c move,
                         const bool king_color);
    eval_t KingOpenFiles(const bool king_color);
    void MovePawnStruct(const piece_t movedPiece, const coord_t from_coord,
                        const move_c m);
    void MobilityEval(bool stm);
};

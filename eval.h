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

    king_value = 32000,
    infinite_score = 32760;

    eval_t val_opn, val_end;
    eval_t initial_score;
    rank_t p_max[board_width + 2][sides], p_min[board_width + 2][sides];
    rank_t (*pawn_max)[sides], (*pawn_min)[sides];
    eval_t material_values_opn[piece_types + 1];
    eval_t material_values_end[piece_types + 1];
    dist_t king_tropism[sides];
    dist_t tropism_factor[2][7];

    const pst_t pst[piece_types][sides][board_height][board_width];
    piece_t pawn, knight, bishop, rook, queen, king;
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

    void FastEval(move_c m);
    eval_t Eval();
    void InitEvalOfMaterialAndPst();
    void InitPawnStruct();
    void SetPawnStruct(coord_t col);
    bool IsPasser(coord_t col, bool stm);
    bool MakeMove(move_c m);
    void TakebackMove(move_c m);

    void MkEvalAfterFastMove(move_c m)
    {
        state[ply - 1].val_opn = val_opn;
        state[ply - 1].val_end = val_end;

        auto from_coord = k2chess::state[ply].from_coord;

        MoveKingTropism(from_coord, m, wtm);

        MovePawnStruct(b[m.to_coord], from_coord, m);
    }

    eval_t ReturnEval(const bool stm)
    {
        i32 X, Y;
        X = material[0] + 1 + material[1] + 1 - pieces[0] - pieces[1];

        Y = ((val_opn - val_end)*X + 80*val_end)/80;
        return stm ? (eval_t)(Y) : (eval_t)(-Y);
    }

    bool is_light(const piece_t piece, const bool stm)
    {
        return piece != empty_square && (piece & white) == stm;
    }

    bool is_dark(const piece_t piece, const bool stm)
    {
        return piece != empty_square && (piece & white) != stm;
    }

    dist_t king_dist(const coord_t from_coord, const coord_t to_coord)
    {
        return std::max(std::abs(get_col(from_coord) - get_col(to_coord)),
                        std::abs(get_row(from_coord) - get_row(to_coord)));
    }


private:

    const size_t opening = 0, endgame = 1;

    void EvalPawns(bool stm);
    bool IsUnstoppablePawn(coord_t x, coord_t y, bool stm);
    void KingSafety(bool king_color);
    void MaterialImbalances();
    eval_t KingWeakness(bool king_color);
    eval_t CountKingTropism(bool king_color);
    void MoveKingTropism(coord_t from_coord, move_c m,
                          bool king_color);
    eval_t KingOpenFiles(bool king_color);
    void MovePawnStruct(piece_t movedPiece, coord_t from_coord, move_c m);
};

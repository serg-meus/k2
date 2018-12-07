#include "eval.h"
#include <limits>





class k2movegen : public k2eval
{


public:

    void RunUnitTests();

    bool MakeMove(const char *in)
    {
        return k2chess::MakeMove(in);
    }

    void TakebackMove()
    {
        k2chess::TakebackMove();
    }


protected:


    typedef u8 movcr_t;
    typedef u8 apprice_t;
    typedef u32 history_t;

    const static size_t move_array_size = 256;

    const static priority_t
    second_killer = 198,
    first_killer = 199,
    move_from_hash = std::numeric_limits<priority_t>::max(),
    king_capture = std::numeric_limits<priority_t>::max(),
    bad_captures = 63;

    const static apprice_t
    not_apprice = 0,
    apprice_only_captures = 1,
    apprice_all = 2;

    struct principal_variation_s
    {
        size_t length;
        move_c moves[max_ply + 1];
    };

    principal_variation_s pv[max_ply];
    move_c killers[max_ply][sides];
    history_t history[sides][piece_types][board_height*board_width];
    history_t min_history, max_history;

    movcr_t GenMoves(move_c * const move_array,
                     const bool need_capture_or_promotion);
    eval_t StaticExchangeEval(const move_c m);
    void AppriceMoves(move_c * const move_array, const movcr_t moveCr,
                      const move_c best_move);


private:


    void GenPawnSilent(move_c * const move_array, movcr_t * const movCr,
                       const piece_id_t piece_id);
    void GenPawnCapturesAndPromotions(move_c * const move_array,
                                      movcr_t * const movCr,
                                      const piece_id_t piece_id);
    void GenCastles(move_c * const move_array, movcr_t * const movCr);
    eval_t SEE(const coord_t to_coord, const eval_t frStreng,
               eval_t val, const bool stm);
    size_t SeeMinAttacker(const coord_t to_coord) const;

    void PushMove(move_c * const move_array, movcr_t * const movCr,
                  const coord_t from_coord, const coord_t to_coord,
                  const move_flag_t flag)
    {
        move_c move;
        move.from_coord = from_coord;
        move.to_coord = to_coord;
        move.flag = flag;
        move.priority = 0;

        assert(IsPseudoLegal(move));
        if(IsLegal(move))
            move_array[(*movCr)++] = move;
    }

    void ProcessSeeBatteries(const coord_t to_coord,
                             const coord_t attacker_coord);

    size_t test_gen_pawn(const char* str_coord, bool captures_and_promotions);
    size_t test_gen_castles();
    size_t test_gen_moves(bool need_captures_or_promotions);
};

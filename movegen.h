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


    typedef u8 apprice_t;
    typedef u32 history_t;

    const static size_t move_array_size = 256;

    const static priority_t
    second_killer = 198,
    first_killer = 199,
    move_from_hash = std::numeric_limits<priority_t>::max(),
    king_capture = std::numeric_limits<priority_t>::max(),
    bad_captures = 63;
    
    struct principal_variation_s
    {
        size_t length;
        move_c moves[max_ply + 1];
    };

    principal_variation_s pv[max_ply];
    move_c killers[max_ply][sides];
    history_t history[sides][piece_types][board_size];

    size_t GenMoves(move_c * const move_array, const bool only_captures) const;
    eval_t StaticExchangeEval(const move_c m) const;
    void AppriceMoves(move_c * const move_array, const size_t moveCr,
                      const move_c best_move) const;
    size_t KeepLegalMoves(move_c * const move_array,
                          const size_t move_cr) const;


private:


    void GenPawnSilent(const piece_id_t piece_id, move_c * const move_array,
                       size_t * const movCr) const;
    void GenPawnNonSilent(const piece_id_t piece_id,
                          move_c * const move_array,
                          size_t * const movCr) const;
    void GenCastles(move_c * const move_array, size_t * const movCr) const;
    eval_t SEE(const coord_t to_coord, const eval_t frStreng,
               eval_t val, bool stm, attack_t *att) const;
    size_t SeeMinAttacker(const attack_t att) const;
    void ProcessSeeBatteries(const coord_t to_coord,
                             const coord_t attacker_coord,
                             const bool stm, attack_t *att) const;

    size_t test_gen_pawn(const char* str_coord, bool only_captures) const;
    size_t test_gen_castles() const;
    size_t test_gen_moves(bool only_captures) const;
    void GenSliderMoves(const bool only_captures, const piece_id_t piece_id,
                        const coord_t from_coord, const piece_type_t type,
                        move_c * const move_array,
                        size_t * const move_cr) const;
    void GenNonSliderMoves(const bool only_captures, const piece_id_t piece_id,
                           const coord_t from_coord, const piece_type_t type,
                           move_c * const move_array,
                           size_t * const move_cr) const;
    size_t AppriceCaptures(const move_c move) const;
    size_t AppriceSilentMoves(const piece_type_t type, const coord_t fr_coord,
                              const coord_t to_coord) const;
    void AppriceHistory(move_c * const move_array, size_t move_cr,
                        history_t min_history, history_t max_history) const;

    void PushMove(move_c * const move_array, size_t * const move_cr,
                  const coord_t from_coord, const coord_t to_coord,
                  const move_flag_t flag) const
    {
        move_c move;
        move.from_coord = from_coord;
        move.to_coord = to_coord;
        move.flag = flag;
        move.priority = 0;

        assert(IsPseudoLegal(move));
        move_array[(*move_cr)++] = move;
    }
};

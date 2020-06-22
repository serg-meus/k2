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

    const static size_t move_capacity = 256;

    const static priority_t
    second_killer = 198,
    first_killer = 199,
    move_from_hash = std::numeric_limits<priority_t>::max(),
    bad_captures = 63;
    
    struct principal_variation_s
    {
        size_t length;
        move_c moves[max_ply + 1];
    };

    principal_variation_s pv[max_ply];
    move_c killers[max_ply][sides];
    history_t history[sides][piece_types][board_size];

    size_t GenMoves(move_c (&move_array)[move_capacity],
                    const bool only_captures) const;
    piece_val_t StaticExchangeEval(const move_c m) const;
    void AppriceMoves(move_c (&move_array)[move_capacity],
                      const size_t moveCr,
                      const move_c best_move) const;
    size_t KeepLegalMoves(move_c (&move_array)[move_capacity],
                          const size_t move_cr) const;
    move_c GetNextMove(move_c (&move_array)[move_capacity],
                       size_t &max_moves,
                       const size_t cur_move_cr,
                       const move_c hash_best_move,
                       const bool hash_one_reply,
                       const bool captures_only,
                       const move_c prev_move) const;
    bool GetFirstMove(move_c (&move_array)[move_capacity],
                      size_t  &max_moves,
                      move_c &first_move,
                      const move_c hash_best_move,
                      const bool hash_one_reply,
                      const bool only_captures) const;
    bool GetSecondMove(move_c (&move_array)[move_capacity],
                       size_t &max_moves, move_c &second_move,
                       move_c prev_move) const;
    size_t FindMaxMoveIndex(move_c (&move_array)[move_capacity],
                            const size_t max_moves,
                            const size_t cur_move) const;

private:


    void GenPawnSilent(const piece_id_t piece_id,
                       move_c (&move_array)[move_capacity],
                       size_t &movCr) const;
    void GenPawnNonSilent(const piece_id_t piece_id,
                          move_c (&move_array)[move_capacity],
                          size_t &movCr) const;
    void GenCastles(move_c (&move_array)[move_capacity],
                    size_t &move_cr) const;
    piece_val_t SEE(const coord_t to_coord, const piece_val_t fr_value,
               piece_val_t val, bool stm, attack_t (&att)[sides]) const;
    size_t SeeMinAttacker(const attack_t att) const;
    void ProcessSeeBatteries(const coord_t to_coord,
                             const coord_t attacker_coord,
                             const bool stm, attack_t (&att)[sides]) const;

    size_t test_gen_pawn(const char* str_coord, bool only_captures) const;
    size_t test_gen_castles() const;
    size_t test_gen_moves(bool only_captures) const;
    void GenSliderMoves(move_c (&move_array)[move_capacity],
                        size_t &move_cr, const bool only_captures,
                        const piece_id_t piece_id,
                        const coord_t from_coord,
                        const piece_type_t type) const;
    void GenNonSliderMoves(move_c (&move_array)[move_capacity],
                           size_t &move_cr, const bool only_captures,
                           const piece_id_t piece_id, const coord_t from_coord,
                           const piece_type_t type) const;
    size_t AppriceCaptures(const move_c move) const;
    size_t AppriceSilentMoves(const piece_type_t type, const coord_t fr_coord,
                              const coord_t to_coord) const;
    void AppriceHistory(move_c (&move_array)[move_capacity],
                        const size_t move_cr,
                        const history_t min_history,
                        const history_t max_history) const;

    void PushMove(move_c (&move_array)[move_capacity],
                  size_t &move_cr,
                  const coord_t from_coord, const coord_t to_coord,
                  const move_flag_t flag) const
    {
        move_c move;
        move.from_coord = from_coord;
        move.to_coord = to_coord;
        move.flag = flag;
        move.priority = 0;

        assert(IsPseudoLegal(move));
        move_array[move_cr++] = move;
    }
};

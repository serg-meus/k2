#include "eval.h"
#include <limits>
#include <array>




class k2movegen : public k2eval
{


public:

    k2movegen()
    {
        for(auto &v: pseudo_legal_pool)
            v.reserve(move_capacity);
    }

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
    move_from_hash = 255,
    move_one_reply = 254,
    bad_captures = 63;

    const static size_t one_reply = 10000;

    struct principal_variation_s
    {
        size_t length;
        move_c moves[max_ply + 1];
    };

    principal_variation_s pv[max_ply];
    move_c killers[max_ply][sides];
    history_t history[sides][piece_types][board_size];

    std::array<std::vector<move_c>, max_ply> pseudo_legal_pool;
    std::array<unsigned, max_ply> pseudo_legal_cr;

    bool GetNextMove(std::vector<move_c> &moves,
                     move_c tt_best_move,
                     const bool captures_only);
    void GenLegalMoves(std::vector<move_c> &moves, 
                       std::vector<move_c> &pseudo_legal_moves,
                       const bool only_captures) const;


private:


    void GenPawnSilent(std::vector<move_c> &moves,
                       const piece_id_t piece_id) const;
    void GenPawnNonSilent(std::vector<move_c> &moves,
                          const piece_id_t piece_id) const;
    void GenCastles(std::vector<move_c> &moves) const;
    piece_val_t SEE(const coord_t to_coord, const piece_val_t fr_value,
               piece_val_t val, bool stm, attack_t (&att)[sides]) const;
    size_t SeeMinAttacker(const attack_t att) const;
    void ProcessSeeBatteries(const coord_t to_coord,
                             const coord_t attacker_coord,
                             const bool stm, attack_t (&att)[sides]) const;

    size_t test_gen_pawn(const char* str_coord, bool only_captures) const;
    size_t test_gen_castles() const;
    size_t test_gen_moves(bool only_captures) const;
    void GenSliderMoves(std::vector<move_c> &moves,
                        const bool only_captures,
                        const piece_id_t piece_id,
                        const coord_t from_coord,
                        const piece_type_t type) const;
    void GenNonSliderMoves(std::vector<move_c> &moves,
                           const bool only_captures,
                           const piece_id_t piece_id,
                           const coord_t from_coord,
                           const piece_type_t type) const;
    size_t AppriceCapture(const move_c move) const;
    size_t AppriceSilentMove(const piece_type_t type, const coord_t fr_coord,
                              const coord_t to_coord) const;
    void AppriceHistory(std::vector<move_c> &moves,
                        const history_t min_history,
                        const history_t max_history) const;
    void GenPseudoLegalMoves(std::vector<move_c> &moves,
                             const bool only_captures) const;
    bool GenHashMove(std::vector<move_c> &moves,
                     move_c &tt_move) const;
    piece_val_t StaticExchangeEval(const move_c m) const;
    void AppriceMoves(std::vector<move_c> &moves,
                      const bool only_captures) const;
    bool GetNextLegalMove();
    void GenerateAndAppriceAllMoves(const move_c tt_best_move,
                                    const bool only_captures);
};

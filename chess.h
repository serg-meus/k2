#include "board.h"
#include <cassert>


class chess : public board {

    public:

    using move_s = board::move_s;

    chess();
    chess(const std::string &fen);

    static constexpr const char* start_pos =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";

    enum class gen_mode {all_moves, only_captures, only_silent};
    std::vector<move_s> gen_moves() { return gen_moves(size_t(-1)); }
    void gen_pseudo_legal_moves(std::vector<move_s> &moves,
                                const gen_mode mode) const;
    bool setup_position(const std::string &fen);
    bool enter_move(const std::string &str);
    bool is_mate() const;
    bool is_draw();
    bool is_stalemate();
    bool is_in_check(const bool king_side) const;
    bool is_N_fold_repetition(const unsigned N) const;

    bool game_over() {
        return is_mate() || is_draw();
    }

    bool is_repetition() const {
        return is_N_fold_repetition(1);
    }

    bool is_draw_by_N_move_rule(const int N) const {
        return reversible_halfmoves == 2*N && !is_mate();
    }

    void gen_pseudo_legal_moves(std::vector<move_s> &moves) {
        gen_pseudo_legal_moves(moves, gen_mode::all_moves);
    }

    bool was_legal(const move_s &move) const {
        if(move.index == king_ix && std::abs(i8(move.from_coord) -
                i8(move.to_coord)) == 2)
           return castling_was_legal(side, move.from_coord, move.to_coord);
        return !is_in_check(!side);
    }

    protected:

    std::array<std::array<u64, 2>, 2> castle_masks;

    std::vector<move_s> gen_moves(size_t max_moves);
    void gen_pawns(std::vector<move_s> &moves, const u64 occupancy,
                   const u64 opp_occupancy, const gen_mode mode) const;
    void gen_pawns_silent(std::vector<move_s> &moves, const u64 pawn_bb,
                          const u64 occupancy) const;
    void gen_pawns_captures_and_promotions(std::vector<move_s> &moves,
                                           const u64 pawn_bb,
                                           const u64 occupancy,
                                           u64 opp_occupancy) const;
    void push_pawn_moves(std::vector<move_s> &moves, u64 bitboard,
                         const int shift) const;
    void gen_non_pawns(std::vector<move_s> &moves, const u8 piece_index,
                       const u64 occupancy, const u64 opp_occupancy,
                       const gen_mode mode) const;
    void gen_castles(std::vector<move_s> &moves, const u64 occupancy) const;
    bool is_attacked_by_slider(const u8 coord, const bool att_side) const;
    bool is_attacked_by_non_slider(const u8 coord, const bool att_side) const;
    bool castling_was_legal(const bool att_side, const u8 fr_crd,
                            const u8 to_crd) const;
    bool is_pseudo_legal(const move_s move) const;
    bool is_pseudo_legal_pawn(const u8 from_coord, const u64 to_bb) const;
    bool is_pseudo_legal_king(const u8 from_coord, const u8 to_coord) const;
    bool slider_maybe_attacks(const u8 coord, const bool att_side) const;
    bool king_cant_move(const bool color) const;

    u64 all_non_pawn_attacks(const u8 index, const u8 from_coord,
                             const u64 occupancy) const {
    if(index == knight_ix)
        return knight_attacks(from_coord);
    else if(index == bishop_ix)
        return bishop_attacks(from_coord, occupancy);
    else if(index == rook_ix)
        return rook_attacks(from_coord, occupancy);
    else if(index == queen_ix)
        return queen_attacks(from_coord, occupancy);
    else if(index == king_ix)
        return king_attacks(from_coord);
    assert(false);
    return(false);
    }

    void gen_pawns(std::vector<move_s> &moves, const u64 occupancy,
                   const u64 opp_occupancy) {
        gen_pawns(moves, occupancy, opp_occupancy, gen_mode::all_moves);
    }
};

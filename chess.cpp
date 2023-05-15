#include "chess.h"


chess::chess() :
    castle_masks{{{0x6000000000000000, 0x0e00000000000000}, {0x60, 0x0e}}} {
    setup_position(start_pos);
}


chess::chess(const std::string &fen) :
    castle_masks{{{0x6000000000000000, 0x0e00000000000000}, {0x60, 0x0e}}} {
    setup_position(fen);
}


bool chess::setup_position(const std::string &fen) {
    return board::setup_position(fen);
}


bool chess::enter_move(const std::string &str) {
    const auto move = move_from_str(str);
    if (!is_pseudo_legal(move))
        return false;
    make_move(move);
    if (was_legal(move))
        return true;
    unmake_move();
    return false;
}


std::vector<board::move_s> chess::gen_moves() {
    std::vector<move_s> moves, tmp_moves;
    tmp_moves.reserve(64);
    gen_pseudo_legal_moves(tmp_moves);
    for (auto m : tmp_moves) {
        make_move(m);
        if (was_legal(m))
            moves.push_back(m);
        unmake_move();
    }
    return moves;
}


void chess::gen_pseudo_legal_moves(std::vector<move_s> &moves,
                                   const gen_mode mode) const {
    const u64 occupancy = bb[0][occupancy_ix] | bb[1][occupancy_ix];
    const u64 opp_occupancy = bb[!side][occupancy_ix];
    gen_pawns(moves, occupancy, opp_occupancy, mode);
    gen_non_pawns(moves, knight_ix, occupancy, opp_occupancy, mode);
    gen_non_pawns(moves, bishop_ix, occupancy, opp_occupancy, mode);
    gen_non_pawns(moves, rook_ix, occupancy, opp_occupancy, mode);
    gen_non_pawns(moves, queen_ix, occupancy, opp_occupancy, mode);
    gen_non_pawns(moves, king_ix, occupancy, opp_occupancy, mode);
    if (mode != gen_mode::only_captures)
        gen_castles(moves, occupancy);
}


bool chess::is_in_check(const bool king_side) const {
    const u8 king_coord = trail_zeros(bb[king_side][king_ix]);
    if (is_attacked_by_non_slider(king_coord, !king_side))
        return true;
    if (slider_maybe_attacks(king_coord, !king_side)) {
        if (is_attacked_by_slider(king_coord, !king_side))
            return true;
    }
    return false;
}


bool chess::is_mate() {
    if (!is_in_check(side))
        return false;
    return gen_moves().size() == 0;
}


bool chess::is_draw() {
    return is_N_fold_repetition(2) || is_draw_by_N_move_rule(75) ||
        is_stalemate() || is_draw_by_material();
}


bool chess::is_stalemate() {
    if (is_in_check(side))
        return false;
    return gen_moves().size() == 0;
}


bool chess::is_N_fold_repetition(const unsigned N) const {
    int cur_ply = int(state_storage.size());
    int cur_reverse_cr = reversible_halfmoves;
    unsigned repeat_cr = 0;
    for (int ply = cur_ply - 2; ply >= 0; ply -= 2) {
        if (state_storage[size_t(ply)].reversible_halfmoves >= cur_reverse_cr)
            return false;
        if (state_storage[size_t(ply)].hash_key == hash_key) {
            repeat_cr++;
            if (repeat_cr >= N)
                return true;
        }
    }
    return false;
}


bool chess::slider_maybe_attacks(const u8 coord,
                                 const bool attacker_side) const {
    const bool can_diag = (diag_mask[coord] | antidiag_mask[coord]) &
        (bb[attacker_side][bishop_ix] | bb[attacker_side][queen_ix]);
    const bool can_rook = (file_mask(coord) | rank_mask(coord)) &
        (bb[attacker_side][rook_ix] | bb[attacker_side][queen_ix]);
    return can_diag | can_rook;
}


void chess::gen_pawns(std::vector<move_s> &moves, const u64 occupancy,
                      const u64 opp_occupancy, const gen_mode mode) const {
    const u64 pawn_bb = bb[side][pawn_ix];
    if (mode != gen_mode::only_captures)
        gen_pawns_silent(moves, pawn_bb, occupancy);
    if (mode != gen_mode::only_silent)
        gen_pawns_captures_and_promotions(moves, pawn_bb, occupancy,
                                          opp_occupancy);
}


void chess::gen_pawns_silent(std::vector<move_s> &moves, const u64 pawn_bb,
                             const u64 occupancy) const {
    const int shift = side == white ? 8 : -8;
    const u64 mask = rank_mask(side == white ? '7' : '2');
    const u64 pawn_pushes = all_pawn_pushes(pawn_bb & ~mask, side, occupancy);
    push_pawn_moves(moves, pawn_pushes, shift, false);
    const u64 dbl_pushes = all_pawn_double_pushes(pawn_pushes, side,
                                                  occupancy);
    push_pawn_moves(moves, dbl_pushes, 2*shift, false);
}


void chess::gen_pawns_captures_and_promotions(std::vector<move_s> &moves,
                                              const u64 pawn_bb,
                                              const u64 occupancy,
                                              u64 opp_occupancy) const {
    const int shift = side == white ? 8 : -8;
    const u64 mask = rank_mask(side == white ? '7' : '2');
    const u64 pawn_pushes = all_pawn_pushes(pawn_bb & mask, side, occupancy);
    push_pawn_moves(moves, pawn_pushes, shift, false);
    opp_occupancy |= en_passant_bitboard;
    const u64 capt_queenside =
        all_pawn_attacks_queenside(pawn_bb, side, opp_occupancy);
    const auto &shifts_q = pawn_attacks_queenside_shifts;
    push_pawn_moves(moves, capt_queenside, -shifts_q[side], true);
    const u64 capt_kingside =
        all_pawn_attacks_kingside(pawn_bb, side, opp_occupancy);
    const auto &shifts_k = pawn_attacks_kingside_shifts;
    push_pawn_moves(moves, capt_kingside, -shifts_k[side], true);
}


void chess::push_pawn_moves(std::vector<move_s> &moves, u64 bitboard,
                            const int shift, const bool is_capt) const {
    while (bitboard) {
        const u64 lowbit = lower_bit(bitboard);
        const u8 to_coord = trail_zeros(lowbit);
        const u8 promos[] = {0, queen_ix, rook_ix, knight_ix, bishop_ix};
        const u8 row = get_row(to_coord);
        const u8 beg = (row == 0 || row == 7) ? 1 : 0;
        const u8 end = (row == 0 || row == 7) ? sizeof(promos) : 1;
        for (u8 promo = beg; promo < end; ++promo)
            moves.push_back({pawn_ix, u8(to_coord - shift),
                               to_coord, promo, is_capt});
        bitboard ^= lowbit;
    }
}


void chess::gen_non_pawns(std::vector<move_s> &moves, const u8 piece_index,
                          const u64 occupancy, const u64 opp_occupancy,
                          const gen_mode mode) const {
    const u64 own_occupancy = occupancy ^ opp_occupancy;
    u64 from_bitboard = bb[side][piece_index];
    while (from_bitboard) {
        const u64 from_lowbit = lower_bit(from_bitboard);
        const u8 from_coord = trail_zeros(from_lowbit);
        u64 to_bitboard = ~own_occupancy &
            all_non_pawn_attacks(piece_index, from_coord, occupancy);
        if (mode == gen_mode::only_captures)
            to_bitboard &= opp_occupancy;
        else if (mode == gen_mode::only_silent)
            to_bitboard &= ~opp_occupancy;
        while (to_bitboard) {
            const u64 to_lowbit = lower_bit(to_bitboard);
            moves.push_back({piece_index, from_coord, trail_zeros(to_lowbit),
                            0, bool(to_lowbit & opp_occupancy)});
            to_bitboard ^= to_lowbit;
        }
        from_bitboard ^= from_lowbit;
    }
}


void chess::gen_castles(std::vector<move_s> &moves, const u64 occupancy) const{
    const u8 fr_coord[] = {str_to_coord("e8"), str_to_coord("e1")};
    const auto &cstl = castling_rights[side];
    if (cstl & castle_kingside &&
            !(occupancy & castle_masks[side][0]))
        moves.push_back({king_ix, fr_coord[side],
                           u8(fr_coord[side] + 2), 0});
    if (cstl & castle_queenside &&
            !(occupancy & castle_masks[side][1]))
        moves.push_back({king_ix, fr_coord[side],
                           u8(fr_coord[side] - 2), 0});
}


bool chess::is_attacked_by_slider(const u8 coord,
                                  const bool attacker_side) const {
    const u64 occ = bb[0][occupancy_ix] | bb[1][occupancy_ix];
    if (rook_attacks(coord, occ) & (bb[attacker_side][rook_ix] |
                                    bb[attacker_side][queen_ix]))
        return true;
    if (bishop_attacks(coord, occ) & (bb[attacker_side][bishop_ix] |
                                      bb[attacker_side][queen_ix]))
        return true;
    return false;
}


bool chess::is_attacked_by_non_slider(const u8 coord,
                                      const bool attacker_side) const {
    const u64 pawn_bb = bb[attacker_side][pawn_ix];
    if (all_pawn_attacks(pawn_bb, attacker_side, u64(-1)) & one_nth_bit(coord))
        return true;
    if (knight_attacks(coord) & bb[attacker_side][knight_ix])
        return true;
    if (king_attacks(coord) & bb[attacker_side][king_ix])
        return true;
    return false;
}


bool chess::castling_was_legal(const bool attacker_side,
                               const u8 fr_crd, const u8 to_crd) const {
    const auto crd1 = fr_crd < to_crd ? fr_crd : to_crd;
    const auto crd2 = fr_crd < to_crd ? to_crd : fr_crd;
    for (auto coord = crd1; coord <= crd2; ++coord)
        if (is_attacked_by_slider(coord, attacker_side) ||
                is_attacked_by_non_slider(coord, attacker_side))
            return false;
    return true;
}


bool chess::is_pseudo_legal(const move_s move) const {
    const u8 index = find_index(side, one_nth_bit(move.from_coord));
    if (index == u8(-1))
        return false;
    if (find_index(side, one_nth_bit(move.to_coord)) != u8(-1))
        return false;
    const u64 to_bb = one_nth_bit(move.to_coord);
    const u64 occ = bb[0][occupancy_ix] | bb[1][occupancy_ix];
    if (index == pawn_ix)
        return is_pseudo_legal_pawn(move.from_coord, to_bb);
    else if (index == king_ix)
        return is_pseudo_legal_king(move.from_coord, move.to_coord);
    else
        return bool(to_bb & all_non_pawn_attacks(move.index,
                                                 move.from_coord, occ));
}


bool chess::is_pseudo_legal_pawn(const u8 from_coord, const u64 to_bb) const {
    const u64 pawn_bb = one_nth_bit(from_coord);
    const u64 opp_occ = bb[!side][occupancy_ix];
    const u64 occ = opp_occ | bb[side][occupancy_ix];
    const u64 att = all_pawn_attacks(pawn_bb, side,
                                     opp_occ | en_passant_bitboard);
    const u64 psh = all_pawn_pushes(pawn_bb, side, occ);
    const u64 dbl = all_pawn_double_pushes(psh, side, occ);
    return bool(to_bb & (att | psh | dbl));
}


bool chess::is_pseudo_legal_king(const u8 from_coord, const u8 to_coord) const {
    const u64 to_bb = one_nth_bit(to_coord);
    if (abs(i8(from_coord) - i8(to_coord)) != 2)
        return bool(to_bb & king_attacks(from_coord));
    const u64 opp_occ = bb[!side][occupancy_ix];
    const u64 occ = opp_occ | bb[side][occupancy_ix];
    const u64 mask = from_coord > to_coord ?
        castle_queenside : castle_kingside;
    if (!(mask & castling_rights[side]))
        return false;
    return !(occ & castle_masks[side][size_t(mask - 1)]);
}


bool chess::is_draw_by_material() const {
    for (auto ix : {pawn_ix, rook_ix, queen_ix})
        if (bb[0][ix] || bb[1][ix])
            return false;
    auto white_bishops = __builtin_popcountll(bb[white][bishop_ix]);
    auto black_bishops = __builtin_popcountll(bb[black][bishop_ix]);
    auto white_knights = __builtin_popcountll(bb[white][knight_ix]);
    auto black_knights = __builtin_popcountll(bb[black][knight_ix]);
    return white_bishops + white_knights <= 1 && black_bishops + black_knights <= 1;
}


std::string chess::game_text_result() {
    std::string ans;
    if (is_mate())
        ans = !side ? "1-0 {White mates}" : "0-1 {Black mates";
    else if (is_stalemate())
        ans = "1/2 - 1/2 {Stalemate}";
    else {
        ans = "1/2 - 1/2 {Draw by ";
        if (is_draw_by_material())
        ans += "insufficient mating material}";
        else if (is_N_fold_repetition(3 - 1))
            ans += "3-fold repetition}";
        else
            ans += "fifty moves rule";
    }
    return ans;
}

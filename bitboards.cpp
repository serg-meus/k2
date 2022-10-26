#include "bitboards.h"


bitboards::bitboards() : first_rank_attacks{{{0}}}
{
    fill_first_rank_attacks();
    fill_diag_masks();
    fill_antidiag_masks();
    fill_non_slider_attacks(knight_shifts, knight_attacks_arr);
    fill_non_slider_attacks(king_shifts, king_attacks_arr);
}


void bitboards::fill_first_rank_attacks() {
    for(u8 col = 0; col < first_rank_attacks.size(); ++col)
        for(u8 occ = 0; occ < first_rank_attacks.at(0).size(); ++occ)
            first_rank_attacks.at(col).at(occ) =
                calc_rank_attack(col, u8(2*occ));
}


u8 bitboards::calc_rank_attack(const u8 rook_col,
                               const u8 occupancy) const {
    u8 ans = 0;
    for(int left_ray = rook_col - 1; left_ray >= 0; --left_ray) {
        ans |= u8(1 << left_ray);
        if(get_nth_bit(occupancy, u8(left_ray)))
            break;
    }
    for(int right_ray = rook_col + 1; right_ray < 8; ++right_ray) {
        ans |= u8(1 << right_ray);
        if(get_nth_bit(occupancy, u8(right_ray)))
            break;
    }
    return ans;
}


void bitboards::fill_diag_masks() {
    for(u8 coord = 0; coord < diag_mask.size(); ++coord)
        diag_mask.at(coord) = calc_diag_mask(coord);
}


void bitboards::fill_antidiag_masks() {
    for(u8 coord = 0; coord < antidiag_mask.size(); ++coord)
        antidiag_mask.at(coord) = calc_antidiag_mask(coord);
}


u64 bitboards::mask_loop(u64 ans, u8 coord, const char sign,
                         const unsigned val, const int col,
                         const int inc) const {
    while(get_col(coord) != u8(col) &&
           (sign == '<' ? coord <= val : coord >= val)) {
        coord = u8(int(coord) + inc);
        ans |= one_nth_bit(coord);
    }
    return ans;
}


u64 bitboards::calc_diag_mask(const u8 coord) const {
    u64 ans = mask_loop(0, coord, '<', 63 - 9, 7, 9);
    ans |= mask_loop(ans, coord, '>', 9, 0, -9);
    return ans;
}


u64 bitboards::calc_antidiag_mask(const u8 coord) const {
    u64 ans = mask_loop(0, coord, '<', 63 - 7, 0, 7);
    ans |= mask_loop(ans, coord, '>', 7, 7, -7);
    return ans;
}


u64 bitboards::diag_attacks(const u8 coord, const u64 occupancy) const {
    u64 masked_occ = diag_mask[coord] & occupancy;
    u64 rank_occ = masked_occ*file_mask('b') >> u64(57);
    u8 rank_att = get_one_rank_attack(get_col(coord), u8(rank_occ));
    u64 filled_att = file_mask('a')*rank_att;
    return diag_mask[coord] & filled_att;
}


u64 bitboards::antidiag_attacks(const u8 coord, const u64 occupancy) const {
    u64 masked_occ = antidiag_mask[coord] & occupancy;
    u64 rank_occ = masked_occ*file_mask('b') >> u64(57);
    u8 rank_att = get_one_rank_attack(get_col(coord), u8(rank_occ));
    u64 filled_att = file_mask('a')*rank_att;
    return antidiag_mask[coord] & filled_att;
}


u64 bitboards::file_attacks(const u8 coord, const u64 occupancy) const {
    u64 diag_a1h8 = 0x8040201008040201;
    u64 diag_c2h7 = 0x0080402010080400;
    u64 col = get_col(coord);
    u64 masked_occ = file_mask('a') & (occupancy >> col);
    const auto rank_occ = size_t(diag_c2h7*masked_occ >> u64(58));
    const auto col2 = size_t(7 - get_row(coord));
    u8 rank_att = first_rank_attacks[col2][rank_occ];
    u64 filled_att = diag_a1h8*rank_att;
    return (file_mask('h') & filled_att) >> (u64(7) - col);
}


u64 bitboards::rank_attacks(const u8 coord, const u64 occupancy) const {
    u64 first_rank = 0xff;
    u64 row = get_row(coord);
    u8 rank_occ = u8(occupancy >> u64(8)*row & first_rank);
    u64 rank_att = get_one_rank_attack(get_col(coord), rank_occ);
    return rank_att << u64(8)*row;
}


u64 bitboards::calc_non_slider_attack(const u8 coord,
                                      const std::array<i8, 8> shifts) const {
    u64 ans = 0;
    int col0 = get_col(coord);
    int row0 = get_row(coord);
    int e4 = str_to_coord("e4");
    for(auto shift: shifts) {
        int d_row = ((e4 + shift) >> 3) - (e4 >> 3);
        int d_col = ((e4 + shift) & 7) - (e4 & 7);
        int col = col0 + d_col;
        int row = row0 + d_row;
        if(col < 0 || col > 7 || row < 0 || row > 7)
            continue;
        ans |= one_nth_bit(get_coord(u8(col), u8(row)));
    }
    return ans;
}


void bitboards::fill_non_slider_attacks(const std::array<i8, 8> shifts,
                                        std::array<u64, 64> &attacks_arr) {
    for(u8 coord = 0; coord < attacks_arr.size(); ++coord)
        attacks_arr.at(coord) = calc_non_slider_attack(coord, shifts);
}


u64 bitboards::all_pawn_attacks(const u64 pawns_bitboard,
                                const bool is_white,
                                const u64 opp_occupancy) const {
        const auto K = all_pawn_attacks_kingside(pawns_bitboard, is_white,
                                                 opp_occupancy);
        const auto Q = all_pawn_attacks_queenside(pawns_bitboard, is_white,
                                                  opp_occupancy);
        return K | Q;
    }


u64 bitboards::all_pawn_attacks_kingside(const u64 pawns_bitboard,
                                         const bool is_white,
                                         const u64 opp_occupancy) const {
    return opp_occupancy & \
        signed_shift(pawns_bitboard & ~file_mask('h'),
                      pawn_attacks_kingside_shifts[is_white]);
}


u64 bitboards::all_pawn_attacks_queenside(const u64 pawns_bitboard,
                                          const bool is_white,
                                          const u64 opp_occupancy) const {
    return opp_occupancy & \
        signed_shift(pawns_bitboard & ~file_mask('a'),
                      pawn_attacks_queenside_shifts[is_white]);
}


u64 bitboards::all_pawn_pushes(const u64 pawns_bitboard,
                               const bool is_white,
                               const u64 occupancy) const {
    return ~occupancy & signed_shift(pawns_bitboard,
                                      pawn_push_shifts[is_white]);
}


u64 bitboards::all_pawn_double_pushes(const u64 pawn_pushes,
                                      const bool is_white,
                                      const u64 occupancy) const {
    u64 beg_rank[] = {0xff00000000, 0xff000000};
    return ~occupancy & beg_rank[is_white] & \
        signed_shift(pawn_pushes, pawn_push_shifts[is_white]);
}

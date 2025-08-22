#include "utils.h"
#include <array>


class bitboards {

    public:

    bitboards();

    u64 bishop_attacks(const u8 bishop_coord, const u64 occupancy) const {
        return diag_attacks(bishop_coord, occupancy) |
            antidiag_attacks(bishop_coord, occupancy);
    }
    u64 rook_attacks(const u8 rook_coord, const u64 occupancy) const {
        return rank_attacks(rook_coord, occupancy) |
            file_attacks(rook_coord, occupancy);
    }
    u64 queen_attacks(const u8 queen_coord, const u64 occupancy) const {
        return bishop_attacks(queen_coord, occupancy) |
            rook_attacks(queen_coord, occupancy);
    }
    u64 knight_attacks(const u8 knight_coord) const {
        return knight_attacks_arr[knight_coord];
    }
    u64 king_attacks(const u8 king_coord) const {
        return king_attacks_arr[king_coord];
    }
    u64 all_pawn_attacks(const u64 pawns_bitboard,
                         const bool is_white,
                         const u64 opp_occupancy) const;
    u64 all_pawn_attacks_kingside(const u64 pawns_bitboard,
                                  const bool is_white,
                                  const u64 opp_occupancy) const;
    u64 all_pawn_attacks_queenside(const u64 pawns_bitboard,
                                   const bool is_white,
                                   const u64 opp_occupancy) const;
    u64 all_pawn_pushes(const u64 pawns_bitboard,
                        const bool is_white,
                        const u64 opp_occupancy) const;
    u64 all_pawn_double_pushes(const u64 pawns_pushes,
                               const bool is_white,
                               const u64 opp_occupancy) const;
    u64 diag_attacks(const u8 coord, const u64 occupancy) const;
    u64 antidiag_attacks(const u8 coord, const u64 occupancy) const;
    u64 file_attacks(const u8 coord, const u64 occupancy) const;
    u64 rank_attacks(const u8 coord, const u64 occupancy) const;

    std::array<std::array<u8, 64>, 8> first_rank_attacks;
    std::array<u64, 64> diag_mask {0}, antidiag_mask {0},
        knight_attacks_arr {0}, king_attacks_arr{0};
    std::array<i8, 8> knight_shifts = {17, 10, -6, -15, -17, -10, 6, 15};
    std::array<i8, 8> king_shifts = {8, 9, 1, -7, -8, -9, -1, 7};
    std::array<i8, 2> pawn_attacks_kingside_shifts = {7, -9},
        pawn_attacks_queenside_shifts = {9, -7}, pawn_push_shifts = {8, -8};

    const u64 dark_squares = 0x5555555555555555,
        light_squares = 0xaaaaaaaaaaaaaaaa;

    protected:

    u8 get_one_rank_attack(const u8 col, const u8 occupancy) const {
        return first_rank_attacks[col][occupancy >> 1 & 63];
    }

    void fill_first_rank_attacks();
    void fill_diag_masks();
    void fill_antidiag_masks();
    u64 calc_diag_mask(const u8 coord) const;
    u64 calc_antidiag_mask(const u8 coord) const;
    u64 mask_loop(u64 ans, u8 coord, const char sign,
                  const unsigned val, const int col, const int inc) const;
    u8 calc_rank_attack(const u8 rook_col, const u8 occupancy) const;
    u64 calc_non_slider_attack(const u8 coords,
                               const std::array<i8, 8> shifts) const;
    void fill_non_slider_attacks(const std::array<i8, 8> shifts,
                                 std::array<u64, 64> &attacks_arr);
    void fill_diag_mask();
    void fill_antidiag_mask();
};

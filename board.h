#include "bitboards.h"
#include <array>
#include <limits>
#include <random>
#include <iostream>


struct board_state {

    struct move_s {
        u8 priority;
        u8 from_coord : 6;
        u8 to_coord : 6;
        u8 index : 3;
        u8 promo : 3;
        u8 is_capture : 1;

        move_s() : priority(0), from_coord(0),
            to_coord(0), index(0), promo(0), is_capture(0) {}

        move_s(u8 ix, u8 fr, u8 to, u8 pr) : priority(0), from_coord(0),
            to_coord(0), index(0), promo(0), is_capture(0) {
            index = ix & 7;
            from_coord = fr & 63;
            to_coord = to & 63;
            promo = pr & 7;
        }

        move_s(u8 ix, u8 fr, u8 to, u8 pr, bool ic) : priority(0),
            from_coord(0), to_coord(0), index(0), promo(0), is_capture(0) {
            index = ix & 7;
            from_coord = fr & 63;
            to_coord = to & 63;
            promo = pr & 7;
            is_capture = ic;
        }

        bool operator == (move_s m) const {
            return index == m.index && from_coord == m.from_coord &&
                to_coord == m.to_coord && promo == m.promo;
        }

        bool operator != (move_s m) const {
            return index != m.index || from_coord != m.from_coord ||
                to_coord != m.to_coord || promo != m.promo;
        }

        bool operator < (move_s m) const {
            return m.priority < priority;
        }

        friend std::ostream& operator << (std::ostream &os, const move_s m) {
            os << "index: " << int(m.index) <<
                "\nfrom: " << char('a' + get_col(u8(m.from_coord))) <<
                char('1' + get_row(u8(m.from_coord))) <<
                "\nto: " << char('a' + get_col(u8(m.to_coord))) <<
                char('1' + get_row(u8(m.to_coord))) <<
                "\npromo: " << int(m.promo) <<
                "\npriority: " << int(m.priority) <<
                "\nis_capture:" << bool(m.is_capture) << "\n";
            return os;
        }
    };

    static_assert(sizeof(move_s) == 4, "Wrong size of move struct");

    static const bool black = false, white = true;
    static const u8 pawn_ix = 0, knight_ix = 1, bishop_ix = 2, rook_ix = 3,
        queen_ix = 4, king_ix = 5, occupancy_ix = 6;

    board_state() :
        en_passant_bitboard(0),
        hash_key(0),
        bb({{{0}}}),
        castling_rights({0}),
        reversible_halfmoves(0) {}

    u64 en_passant_bitboard;
    u64 hash_key;
    std::array<std::array<u64, occupancy_ix + 1>, 2> bb;
    std::array<u8, 2> castling_rights = {0};
    int reversible_halfmoves;
};


class board : public board_state, protected bitboards {

    public:

    board();

    static const u8 castle_kingside = 1, castle_queenside = 2;
    bool side;

    bool setup_position(const std::string &fen);
    void enter_move(const std::string &str);
    void make_move(const move_s &move);
    bool unmake_move();
    std::string board_to_fen() const;
    std::string board_to_ascii() const;
    move_s move_from_str(const std::string &str) const;
    std::string move_to_str(const move_s &move) const;

    protected:

    std::vector<board_state> state_storage;
    bool is_reversible;
    std::array<std::array<std::array<u64, 64>, 6>,  2> rnd;
    std::array<u64, 16> rnd_castling;
    std::array<u64, 9> rnd_en_passant;

    void init();
    u8 find_index(const bool color, const u64 bitboard) const;
    void save_state(const move_s &move, const u8 captured_index);
    void make_common(const move_s &move);
    void make_capture(const move_s &move);
    void make_en_passant(const move_s &move);
    void update_en_passant_bb(const move_s &move);
    u8 make_promotion(const move_s &move);
    void make_castling(const move_s &move);
    void update_castling_rights(const move_s &move);
    void update_castling_rights_on_rook_capture(const u8 to_coord);
    bool setup_row(u8 row, const std::string &str);
    bool setup_rest(const std::string &str);
    void setup_occupancy();
    void setup_side_occupancy(const bool color);
    void setup_castling_rights(const std::string &str);
    void setup_en_passant(const std::string &str);
    u8 char_to_bb_index(char symbol, bool &color) const;
    u8 char_to_bb_index(char symbol) const;
    char bb_index_to_char(const bool color, const u8 index) const;
    std::string bitboards_to_fen() const;
    char square_to_fen(const u8 col, const u8 row) const;
    std::string castling_to_fen() const;
    std::string en_passant_to_fen() const;
    std::string compress_fen(std::string in) const;
    u64 setup_hash_key();
    u8 min_attacker(const u8 to_coord, const u64 occ, const bool color,
                    u64 &attacker_bb) const;
    bool is_passer(const bool color, const u8 pawn_coord,
                   const u64 opp_pawns) const;

    u8 str_to_coord(const std::string &str_c) const {
        return get_coord(u8(str_c[0] - 'a'), u8(str_c[1] - '1'));
    }

    size_t castling_index() const {
        return size_t(castling_rights[0] + 4*castling_rights[1]);
    }

    u8 enps_ix(const u8 coord) const {
        return coord == u8(-1) ? 0 : u8(1 + get_col(coord));
    }

    bool is_silent(const move_s move) const {
        u64 to_bb = one_nth_bit(move.to_coord);
        if (bb[!side][occupancy_ix] & to_bb)
            return true;
        if (move.index != pawn_ix)
            return false;
        auto row = get_row(move.to_coord);
        if (row == 0 || row == 7)
            return true;
        if (to_bb & en_passant_bitboard)
            return true;
        return false;
    }

};

#include "engine.h"
#include <memory>

using move_s = board::move_s;


class bitboards_tst : public bitboards {
    public:
    using bitboards::diag_mask;
    using bitboards::antidiag_mask;
    using bitboards::knight_attacks_arr;
    using bitboards::king_attacks_arr;
    using bitboards::get_one_rank_attack;
};


class board_tst : public board {
    public:
    using board::castling_rights;
    using board::en_passant_bitboard;
    using board::reversible_halfmoves;
    using board::bb;
};


class chess_tst: public chess {
    public:
    using chess::chess;
    using chess::gen_pawns;
    using chess::gen_castles;
    using chess::is_attacked_by_slider;
    using chess::is_attacked_by_non_slider;
    using chess::setup_hash_key;
    using chess::bb;
    using board_state::hash_key;
    using chess::state_storage;
    using board_state::reversible_halfmoves;
};

class eval_tst: public eval {
    public:
};

class engine_tst: public engine {
    public:
    using engine::engine;
    using engine::static_exchange_eval;
    using engine::min_attacker;
    using engine::bb;
    using engine::material;
};

class test_all : public bitboards {

public :

    void test_main();
    void test_bitboards();
    void test_one_rank_attack(const bitboards_tst& B);
    void test_str_to_coord();
    void test_fill_diag_masks(const bitboards_tst& B);
    void test_fill_antidiag_masks(const bitboards_tst& B);
    void test_diag_attacks(const bitboards_tst& B);
    void test_antidiag_attacks(const bitboards_tst& B);
    void test_file_attacks(const bitboards_tst& B);
    void test_rank_attacks(const bitboards_tst& B);
    void test_fill_non_slider_attacks(const bitboards_tst& B);
    void test_attacks(const bitboards_tst& B);
    void test_board();
    void test_setup_position();
    void test_board_to_fen();
    void test_make_move();
    void test_bitboards_integrity(const board_tst& B);
    void test_unmake_move();
    void test_chess();
    void test_gen_pseudo_legal_moves();
    void test_is_attacked_by();
    bool test_sequence_of_moves(const std::string &moves, chess_tst &C);
    void test_legality();
    void test_utils();
    void test_game_over();
    void test_perft();
    void test_engine();
    void test_eval();
    void test_see();
};

#include "k2.h"

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
    using board::is_passer;
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
    using eval::material_values;
    using eval::piece_values;
    using eval::eval_material;
    using eval::eval_pst;
    using eval::pst;
    using eval::double_pawns;
    using eval::isolated_pawns;
    using eval::pawn_doubled;
    using eval::pawn_isolated;
    using eval::pawn_dbl_iso;
    using eval::eval_double_and_isolated;
    using eval::passed_pawns;
    using eval::unstoppable_pawns;
    using eval::connected_passers;
    using eval::pawn_gaps;
    using eval::pawn_holes;
    using eval::king_pawn_tropism;
    using eval::mobility_piece_type;
    using eval::eval_pawns;
    using eval::mobility_curve;
};


class engine_tst: public engine {
    public:
    using engine::engine;
    using engine::static_exchange_eval;
    using engine::min_attacker;
    using engine::bb;
    using engine::material_values;
    using engine::not_a_move;
    using engine::next_move;
};


class k2_tst: public k2 {
    public:
    using k2::timer_start;
    using k2::update_clock;
    using k2::current_clock;
    using k2::move_cr;
    using k2::moves_per_time_control;
    using k2::time_per_time_control;
    using k2::time_inc;
    using k2::time_for_move;
    using k2::set_time_for_move;
    using k2::time_margin;
};


class test_all : public eval {

    u64 bit(const char *str_c) {
        return one_nth_bit(str_to_coord(str_c));
    }

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
    void test_eval_material();
    void test_eval_pst();
    void test_double_pawns();
    void test_isolated_pawns();
    void test_double_and_isolated();
    void test_passed_pawns();
    void test_distance();
    void test_unstoppable();
    void test_connected_passers();
    void test_pawn_gaps();
    void test_pawn_holes();
    void test_eval_pawns();
    void test_king_pawn_tropism();
    void test_mobility_piece_type();
    void test_see();
    void test_next_move();
    void test_set_time_for_move();
    void test_update_clock();
    void test_k2();
    void test_is_passer();
};

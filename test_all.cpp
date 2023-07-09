#include "test_all.h"


int main() {
    auto T = test_all();
    T.test_main();
    std::cout << "All tests passed" << std::endl;
    return 0;
}


void test_all::test_main() {
    test_utils();
    test_bitboards();
    test_board();
    test_chess();
    test_eval();
    test_engine();
    test_k2();
}


void test_all::test_utils() {
    test_str_to_coord();

    const std::string str = "  1  AB   A;A  ";
    auto ans = split(str, ' ', 0);
    assert(ans.size() == 3);
    assert(ans[0] == "1");
    assert(ans[1] == "AB");
    assert(ans[2] == "A;A");
    ans = split(str, ' ', 1);
    assert(ans.size() == 2);
    assert(ans[0] == "1");
    assert(ans[1] == "AB   A;A  ");
    ans = split(str, 'A', 2);
    assert(ans.size() == 3);
    assert(ans[0] == "  1  ");
    assert(ans[1] == "B   ");
    assert(ans[2] == ";A  ");
}


void test_all::test_bitboards() {
    auto B = bitboards_tst();
    test_one_rank_attack(B);
    test_fill_diag_masks(B);
    test_fill_antidiag_masks(B);
    test_diag_attacks(B);
    test_antidiag_attacks(B);
    test_file_attacks(B);
    test_rank_attacks(B);
    test_attacks(B);
}


void test_all::test_board() {
    test_setup_position();
    test_board_to_fen();
    test_make_move();
    test_unmake_move();
    test_is_passer();
}


void test_all::test_chess() {
    test_gen_pseudo_legal_moves();
    test_is_attacked_by();
    test_legality();
    test_game_over();
}


void test_all::test_eval() {
}


void test_all::test_engine() {
    test_perft();
    test_see();
    test_next_move();
}


void test_all::test_k2() {
    test_set_time_for_move();
    test_update_clock();
}


void test_all::test_one_rank_attack(const bitboards_tst& B) {
    assert(B.get_one_rank_attack(3, 0x4a) == 0x76);
    assert(B.get_one_rank_attack(4, 0x91) == 0xef);
    assert(B.get_one_rank_attack(5, 0x20) == 0xdf);
    assert(B.get_one_rank_attack(0, 0x60) == 0x3e);
    assert(B.get_one_rank_attack(0, 0x01) == 0xfe);
    assert(B.get_one_rank_attack(0, 0x00) == 0xfe);
    assert(B.get_one_rank_attack(7, 0x80) == 0x7f);
}


void test_all::test_str_to_coord() {
    assert(str_to_coord("a1") == 0);
    assert(str_to_coord("b1") == 1);
    assert(str_to_coord("g3") == 22);
}


void test_all::test_fill_diag_masks(const bitboards_tst& B) {
    assert(B.diag_mask[str_to_coord("a1")] == 0x8040201008040200);
    assert(B.diag_mask[str_to_coord("h8")] == 0x0040201008040201);
    assert(B.diag_mask[str_to_coord("d4")] == 0x8040201000040201);
    assert(B.diag_mask[str_to_coord("g3")] == 0x0000000080002010);
    assert(B.diag_mask[str_to_coord("b7")] == 0x0400010000000000);
    assert(B.diag_mask[str_to_coord("h1")] == 0);
    assert(B.diag_mask[str_to_coord("a8")] == 0);
}


void test_all::test_fill_antidiag_masks(const bitboards_tst& B) {
    assert(B.antidiag_mask[str_to_coord("h1")] == 0x0102040810204000);
    assert(B.antidiag_mask[str_to_coord("a8")] == 0x0002040810204080);
    assert(B.antidiag_mask[str_to_coord("d4")] == 0x0001020400102040);
    assert(B.antidiag_mask[str_to_coord("b3")] == 0x0000000001000408);
    assert(B.antidiag_mask[str_to_coord("g7")] == 0x2000800000000000);
    assert(B.antidiag_mask[str_to_coord("a1")] == 0);
    assert(B.antidiag_mask[str_to_coord("h8")] == 0);
}


void test_all::test_diag_attacks(const bitboards_tst& B) {
    assert(B.diag_attacks(str_to_coord("d4"), 0) == 0x8040201000040201);
    assert(B.diag_attacks(str_to_coord("e4"), 0) == 0x0080402000080402);
    assert(B.diag_attacks(str_to_coord("d4"),
                          0x040000) == 0x8040201000040000);
    assert(B.diag_attacks(str_to_coord("e4"),
                          0x0000400000080000) == 0x0000402000080000);
    assert(B.diag_attacks(str_to_coord("b6"),
                          0x0804000100000000) == 0x0004000100000000);
    assert(B.diag_attacks(str_to_coord("d4"),
                          ~0x8040201000040201) == 0x8040201000040201);
}


void test_all::test_antidiag_attacks(const bitboards_tst& B) {
    assert(B.antidiag_attacks(str_to_coord("d4"), 0) == 0x0001020400102040);
    assert(B.antidiag_attacks(str_to_coord("e4"), 0) == 0x0102040800204080);
    assert(B.antidiag_attacks(str_to_coord("d4"),
                              0x100000) == 0x0001020400100000);
    assert(B.antidiag_attacks(str_to_coord("e4"),
                              0x0000040000200000) == 0x0000040800200000);
    assert(B.antidiag_attacks(str_to_coord("g6"),
                              0x1020008000000000) == 0x0020008000000000);
    assert(B.antidiag_attacks(str_to_coord("d4"),
                              ~0x0001020400102040ULL) == 0x0001020400102040);
}


void test_all::test_file_attacks(const bitboards_tst& B) {
    assert(B.file_attacks(str_to_coord("d4"), 0) == 0x0808080800080808);
    assert(B.file_attacks(str_to_coord("e4"), 0) == 0x1010101000101010);
    assert(B.file_attacks(str_to_coord("d4"),
                          0x080000) == 0x0808080800080000);
    assert(B.file_attacks(str_to_coord("e4"),
                          0x0000001000100000) == 0x0000001000100000);
    assert(B.file_attacks(str_to_coord("a1"),
                          0x0001000000000000) == 0x0001010101010100);
    assert(B.file_attacks(str_to_coord("h8"), 0x80) == 0x0080808080808080);
    assert(B.file_attacks(str_to_coord("d4"),
                          ~0x0808080800080808ULL) == 0x0808080800080808);
}


void test_all::test_rank_attacks(const bitboards_tst& B) {
    assert(B.rank_attacks(str_to_coord("d4"), 0) == 0xf7000000);
    assert(B.rank_attacks(str_to_coord("e5"), 0) == 0xef00000000);
    assert(B.rank_attacks(str_to_coord("c8"),
                          0x1200000000000000) == 0x1a00000000000000);
    assert(B.rank_attacks(str_to_coord("h2"), 0x0200) == 0x7e00);
    assert(B.rank_attacks(str_to_coord("a3"), 0x800000) == 0xfe0000);
    assert(B.rank_attacks(str_to_coord("g1"), 0x20) == 0xa0);
    assert(B.rank_attacks(str_to_coord("c4"), ~0xff000000ULL) == 0xfb000000);
}


void test_all::test_fill_non_slider_attacks(const bitboards_tst& B) {
    assert(B.knight_attacks_arr[str_to_coord("e4")] == 0x0000284400442800);
    assert(B.knight_attacks_arr[str_to_coord("b3")] == 0x0000000508000805);
    assert(B.knight_attacks_arr[str_to_coord("h8")] == 0x0020400000000000);
    assert(B.knight_attacks_arr[str_to_coord("a7")] == 0x0400040200000000);
    assert(B.king_attacks_arr[str_to_coord("e4")] == 0x0000003828380000);
    assert(B.king_attacks_arr[str_to_coord("a3")] == 0x0000000003020300);
    assert(B.king_attacks_arr[str_to_coord("h8")] == 0x40c0000000000000);
    assert(B.king_attacks_arr[str_to_coord("e1")] == 0x3828);
}


void test_all::test_attacks(const bitboards_tst& B) {
    assert(B.queen_attacks(str_to_coord("d5"),
                           0xbde2051a3204e1f1) == 0x08281c161c0a0908);
    assert(B.knight_attacks(str_to_coord("f2")) == 0x50880088);
    assert(B.king_attacks(str_to_coord("a1")) == 0x0302);
    u64 w_pawns = 0x801fa00;  // 8/p2p3p/7p/3p3p/3Pp2k/PK3p2/1P1PPPPP/8 w - -
    u64 b_pawns = 0x89808810200000;
    assert(B.all_pawn_attacks_kingside(w_pawns, 1, b_pawns) ==
           one_nth_bit(str_to_coord("f3")));
    assert(B.all_pawn_attacks_queenside(w_pawns, 1, b_pawns) ==
           one_nth_bit(str_to_coord("f3")));
    assert(B.all_pawn_attacks_kingside(b_pawns, 0, w_pawns) ==
           one_nth_bit(str_to_coord("g2")));
    assert(B.all_pawn_attacks_queenside(b_pawns, 0, w_pawns) ==
           one_nth_bit(str_to_coord("e2")));
    u64 w_king = one_nth_bit(str_to_coord("b3"));
    u64 b_king = one_nth_bit(str_to_coord("h4"));
    u64 occupancy = w_pawns | b_pawns | w_king | b_king;
    u64 w_pushes = B.all_pawn_pushes(w_pawns, 1, occupancy);
    assert(w_pushes == 0x1d80000);
    u64 b_pushes = B.all_pawn_pushes(b_pawns, 0, occupancy);
    assert(b_pushes == 0x90000100000);
    assert(B.all_pawn_double_pushes(w_pushes, 1, occupancy) == 0x40000000);
    assert(B.all_pawn_double_pushes(b_pushes, 0, occupancy) == 0x100000000);
}


void test_all::test_setup_position() {
    auto brd = board_tst();
    brd.setup_position(
        "rq2k1nr/1bppb1pp/np2p3/pB2Pp2/3P4/1P3N2/PBP1QPPP/RN2K2R"
        " w Qk f6 0 11 ");
    const auto white = brd.white;
    const auto black = brd.black;
    assert(brd.side == white);
    assert(brd.castling_rights[white] == brd.castle_queenside);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.en_passant_bitboard == one_nth_bit(str_to_coord("f6")));
    assert(brd.reversible_halfmoves == 0);
    assert(brd.bb[white][brd.pawn_ix] == 0x100802e500);
    assert(brd.bb[black][brd.pawn_ix] == 0xcc122100000000);
    assert(brd.bb[white][brd.knight_ix] == 0x200002);
    assert(brd.bb[black][brd.knight_ix] == 0x4000010000000000);
    assert(brd.bb[white][brd.bishop_ix] == 0x0200000200);
    assert(brd.bb[black][brd.bishop_ix] == 0x12000000000000);
    assert(brd.bb[white][brd.rook_ix] == 0x81);
    assert(brd.bb[black][brd.rook_ix] == 0x8100000000000000);
    assert(brd.bb[white][brd.queen_ix] == 0x1000);
    assert(brd.bb[black][brd.queen_ix] == 0x0200000000000000);
    assert(brd.bb[white][brd.king_ix] == 0x10);
    assert(brd.bb[black][brd.king_ix] == 0x1000000000000000);
    assert(brd.bb[white][brd.occupancy_ix] == 0x120822f793);
    assert(brd.bb[black][brd.occupancy_ix] == 0xd3de132100000000);
}


void test_all::test_board_to_fen() {
    auto brd = board();
    std::string fen =
        "rq2k1nr/1bppb1pp/np2p3/pB2Pp2/3P4/1P3N2/PBP1QPPP/RN2K2R w Qk f6 3";
    brd.setup_position(fen);
    assert(brd.board_to_fen() == fen);
    fen = "r3k2r/8/3b4/8/8/8/4B3/3QK3 b kq - 11";
    brd.setup_position(fen);
    assert(brd.board_to_fen() == fen);
    fen = "4k3/8/8/3Pp3/8/8/8/4K3 w - e6 0";
    brd.setup_position(fen);
    assert(brd.board_to_fen() == fen);
}


void test_all::test_make_move() {
    auto brd = board_tst();
    const auto white = brd.white;
    const auto black = brd.black;
    std::string fen =
        "rq2k1nr/1bppb1pp/np2p3/pB2Pp2/3P4/1P3N2/PBP1QPPP/RN2K2R w Qk f6 3";
    brd.setup_position(fen);
    brd.enter_move("a2a4");
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("a2"))));
    assert(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("a4")));
    assert(brd.side == black);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == brd.castle_queenside);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.reversible_halfmoves == 0);

    brd.setup_position(fen);
    brd.enter_move("a2a3");
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("a2"))));
    assert(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("a3")));
    assert(brd.side == black);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == brd.castle_queenside);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.reversible_halfmoves == 0);

    brd.setup_position(fen);
    brd.enter_move("e2d1");
    assert(!(brd.bb[white][brd.queen_ix] & one_nth_bit(str_to_coord("e2"))));
    assert(brd.bb[white][brd.queen_ix] & one_nth_bit(str_to_coord("d1")));
    assert(brd.side == black);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == brd.castle_queenside);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.reversible_halfmoves == 4);

    brd.setup_position(fen);
    brd.enter_move("e1d2");
    assert(!(brd.bb[white][brd.king_ix] & one_nth_bit(str_to_coord("e1"))));
    assert(brd.bb[white][brd.king_ix] & one_nth_bit(str_to_coord("d2")));
    assert(brd.side == black);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == 0);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.reversible_halfmoves == 4);

    brd.setup_position(fen);
    brd.enter_move("e1g1");
    assert(!(brd.bb[white][brd.king_ix] & one_nth_bit(str_to_coord("e1"))));
    assert(brd.bb[white][brd.king_ix] & one_nth_bit(str_to_coord("g1")));
    assert(!(brd.bb[white][brd.rook_ix] & one_nth_bit(str_to_coord("h1"))));
    assert(brd.bb[white][brd.rook_ix] & one_nth_bit(str_to_coord("f1")));
    assert(brd.side == black);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == 0);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.reversible_halfmoves == 4);

    brd.enter_move("g8f6");
    assert(!(brd.bb[black][brd.knight_ix] & one_nth_bit(str_to_coord("g8"))));
    assert(brd.bb[black][brd.knight_ix] & one_nth_bit(str_to_coord("f6")));
    assert(brd.side == white);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == 0);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.reversible_halfmoves == 5);

    brd.enter_move("e5f6");
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("e5"))));
    assert(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("f6")));
    assert(!(brd.bb[black][brd.knight_ix] & one_nth_bit(str_to_coord("f6"))));
    assert(brd.side == black);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == 0);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.reversible_halfmoves == 0);

    brd.enter_move("h8f8");
    assert(brd.castling_rights[black] == 0);
    assert(brd.reversible_halfmoves == 1);

    brd.setup_position(fen);
    assert(brd.bb[white][brd.knight_ix] & one_nth_bit(str_to_coord("f3")));
    brd.enter_move("b1c3");
    assert(brd.bb[white][brd.occupancy_ix] & one_nth_bit(str_to_coord("f3")));
    brd.enter_move("b7f3");
    assert(!(brd.bb[black][brd.bishop_ix] & one_nth_bit(str_to_coord("b7"))));
    assert(brd.bb[black][brd.bishop_ix] & one_nth_bit(str_to_coord("f3")));
    assert(!(brd.bb[white][brd.knight_ix] & one_nth_bit(str_to_coord("f3"))));
    assert(brd.side == white);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == brd.castle_queenside);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.reversible_halfmoves == 0);

    brd.enter_move("e1c1");
    brd.enter_move("g8f6");
    brd.enter_move("e5f6");
    brd.enter_move("e8g8");
    assert(!(brd.bb[black][brd.king_ix] & one_nth_bit(str_to_coord("e8"))));
    assert(brd.bb[black][brd.king_ix] & one_nth_bit(str_to_coord("g8")));
    assert(!(brd.bb[black][brd.rook_ix] & one_nth_bit(str_to_coord("h8"))));
    assert(brd.bb[black][brd.rook_ix] & one_nth_bit(str_to_coord("f8")));
    assert(brd.side == white);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == 0);
    assert(brd.castling_rights[black] == 0);
    assert(brd.reversible_halfmoves == 1);

    brd.enter_move("f6e7");
    brd.enter_move("f3e2");
    assert(brd.bb[white][brd.occupancy_ix] == 0x001000020806e78c);
    assert(brd.bb[black][brd.occupancy_ix] == 0x63cc132100001000);
    brd.enter_move("e7f8");
    assert(!(brd.bb[white][brd.queen_ix] & one_nth_bit(str_to_coord("e7"))));
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("e7"))));
    assert(brd.bb[white][brd.queen_ix] & one_nth_bit(str_to_coord("f8")));
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("f8"))));
    assert(brd.side == black);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == 0);
    assert(brd.castling_rights[black] == 0);
    assert(brd.reversible_halfmoves == 0);
    assert(brd.bb[white][brd.occupancy_ix] == 0x200000020806e78c);
    assert(brd.bb[black][brd.occupancy_ix] == 0x43CC132100001000);
    test_bitboards_integrity(brd);

    brd.setup_position(fen);
    brd.enter_move("e5f6");
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("e5"))));
    assert(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("f6")));
    assert(!(brd.bb[black][brd.pawn_ix] & one_nth_bit(str_to_coord("f5"))));
    assert(brd.side == black);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.castling_rights[white] == brd.castle_queenside);
    assert(brd.castling_rights[black] == brd.castle_kingside);
    assert(brd.reversible_halfmoves == 0);
    assert(brd.bb[white][brd.occupancy_ix] == 0x20020822f793);
    assert(brd.bb[black][brd.occupancy_ix] == 0xd3de130100000000);
    test_bitboards_integrity(brd);

    fen = "4b3/3P2k1/8/8/8/8/1p4K1/N1N5 w - -";
    brd.setup_position(fen);
    brd.enter_move("d7e8n");
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("d7"))));
    assert(brd.bb[white][brd.knight_ix] & one_nth_bit(str_to_coord("e8")));
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("e8"))));
    brd.unmake_move();
    brd.enter_move("d7e8b");
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("d7"))));
    assert(brd.bb[white][brd.bishop_ix] & one_nth_bit(str_to_coord("e8")));
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("e8"))));
    brd.unmake_move();
    brd.enter_move("d7e8r");
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("d7"))));
    assert(brd.bb[white][brd.rook_ix] & one_nth_bit(str_to_coord("e8")));
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("e8"))));
    brd.unmake_move();
    brd.enter_move("d7e8q");
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("d7"))));
    assert(brd.bb[white][brd.queen_ix] & one_nth_bit(str_to_coord("e8")));
    assert(!(brd.bb[white][brd.pawn_ix] & one_nth_bit(str_to_coord("e8"))));
}


void test_all::test_unmake_move() {
    auto brd = board_tst();
    const auto white = brd.white;
    const auto black = brd.black;
    const auto fen =
        "rq2k1nr/1bppb1pp/np2p3/pB2Pp2/3P4/1P3N2/PBP1QPPP/RN2K2R w Qk f6 3";
    brd.setup_position(fen);
    brd.enter_move("e5f6");
    brd.enter_move("g7f6");
    brd.enter_move("e1g1");
    brd.enter_move("e8d8");
    brd.unmake_move();
    assert(!(brd.bb[black][brd.king_ix] & one_nth_bit(str_to_coord("d8"))));
    assert(brd.bb[black][brd.king_ix] & one_nth_bit(str_to_coord("e8")));
    assert(brd.castling_rights[white] == 0);
    assert(brd.castling_rights[black] & brd.castle_kingside);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.reversible_halfmoves == 1);
    brd.unmake_move();
    assert(!(brd.bb[white][brd.king_ix] & one_nth_bit(str_to_coord("g1"))));
    assert(brd.bb[white][brd.king_ix] & one_nth_bit(str_to_coord("e1")));
    assert(!(brd.bb[white][brd.rook_ix] & one_nth_bit(str_to_coord("f1"))));
    assert(brd.bb[white][brd.rook_ix] & one_nth_bit(str_to_coord("h1")));
    assert(brd.castling_rights[white] & brd.castle_queenside);
    assert(brd.castling_rights[black] & brd.castle_kingside);
    assert(brd.en_passant_bitboard == 0);
    assert(brd.reversible_halfmoves == 0);
    brd.unmake_move();
    brd.unmake_move();
    assert(brd.castling_rights[white] & brd.castle_queenside);
    assert(brd.castling_rights[black] & brd.castle_kingside);
    assert(brd.en_passant_bitboard == one_nth_bit(str_to_coord("f6")));
    assert(brd.reversible_halfmoves == 3);
    assert(brd.bb[white][brd.occupancy_ix] == 0x120822f793);
    assert(brd.bb[black][brd.occupancy_ix] == 0xd3de132100000000);
    test_bitboards_integrity(brd);
}


void test_all::test_is_passer() {
    auto brd = board_tst();
    const auto white = brd.white;
    const auto black = brd.black;
    const auto pawn_ix = brd.pawn_ix;
    brd.setup_position("4k3/6p1/2p3P1/Pp2P3/p7/8/3P3P/4K3 w - -");
    assert(brd.is_passer(white, str_to_coord("a5"), brd.bb[black][pawn_ix]));
    assert(brd.is_passer(black, str_to_coord("a4"), brd.bb[white][pawn_ix]));
    assert(brd.is_passer(black, str_to_coord("b5"), brd.bb[white][pawn_ix]));
    assert(!brd.is_passer(black, str_to_coord("c6"), brd.bb[white][pawn_ix]));
    assert(!brd.is_passer(white, str_to_coord("d2"), brd.bb[black][pawn_ix]));
    assert(brd.is_passer(white, str_to_coord("e5"), brd.bb[black][pawn_ix]));
    assert(!brd.is_passer(black, str_to_coord("g7"), brd.bb[white][pawn_ix]));
    assert(!brd.is_passer(white, str_to_coord("g6"), brd.bb[black][pawn_ix]));
    assert(!brd.is_passer(white, str_to_coord("h2"), brd.bb[black][pawn_ix]));
}


void test_all::test_bitboards_integrity(const board_tst &B) {
    for(auto color : {B.black, B.white}) {
        auto ans = u64(0);
        for(auto i : {B.pawn_ix, B.knight_ix, B.bishop_ix, B.rook_ix,
                      B.queen_ix, B.king_ix})
            ans ^= B.bb[color][i];
        assert(ans == B.bb[color][B.occupancy_ix]);
    }
}


void test_all::test_gen_pseudo_legal_moves() {
    auto C = chess_tst("8/3pp1p1/6p1/1p2P3/1kP2p1p/p3P2K/PP1P2PP/8 w - -");
    u64 occupancy = C.bb[0][C.occupancy_ix] | C.bb[1][C.occupancy_ix];
    u64 opp_occupancy = C.bb[!C.side][C.occupancy_ix];
    std::vector<move_s> moves;
    C.gen_pawns(moves, occupancy, opp_occupancy);
    assert(moves.size() == 11);
    moves.clear();
    assert(C.enter_move("g2g4"));
    occupancy = C.bb[0][C.occupancy_ix] | C.bb[1][C.occupancy_ix];
    opp_occupancy = C.bb[!C.side][C.occupancy_ix];
    C.gen_pawns(moves, occupancy, opp_occupancy);
    assert(moves.size() == 10);
    moves.clear();
    assert(C.enter_move("d7d5"));
    occupancy = C.bb[0][C.occupancy_ix] | C.bb[1][C.occupancy_ix];
    opp_occupancy = C.bb[!C.side][C.occupancy_ix];
    C.gen_pawns(moves, occupancy, opp_occupancy);
    assert(moves.size() == 12);
    moves.clear();
    C.setup_position("r3k2r/p5pp/8/8/8/8/P4P1P/R3K2R w KQkq -");
    occupancy = C.bb[0][C.occupancy_ix] | C.bb[1][C.occupancy_ix];
    C.gen_castles(moves, occupancy);
    assert(moves.size() == 2);
    moves.clear();
    assert(C.enter_move("e1g1"));
    occupancy = C.bb[0][C.occupancy_ix] | C.bb[1][C.occupancy_ix];
    C.gen_castles(moves, occupancy);
    assert(moves.size() == 2);
    moves.clear();
    C.setup_position("r3k2r/p5pp/7N/4B3/4b3/8/5P1P/RN2K2R w KQkq -");
    occupancy = C.bb[0][C.occupancy_ix] | C.bb[1][C.occupancy_ix];
    C.gen_castles(moves, occupancy);
    assert(moves.size() == 1);
    moves.clear();
    assert(C.enter_move("e1g1"));
    occupancy = C.bb[0][C.occupancy_ix] | C.bb[1][C.occupancy_ix];
    C.gen_castles(moves, occupancy);
    assert(moves.size() == 2);
    moves.clear();
    C.setup_position("rn2k2r/p5pp/8/8/8/6P1/P6P/R3K1NR w KQkq -");
    occupancy = C.bb[0][C.occupancy_ix] | C.bb[1][C.occupancy_ix];
    C.gen_castles(moves, occupancy);
    assert(moves.size() == 1);
    moves.clear();
    assert(C.enter_move("e1e2"));
    occupancy = C.bb[0][C.occupancy_ix] | C.bb[1][C.occupancy_ix];
    C.gen_castles(moves, occupancy);
    assert(moves.size() == 1);
    moves.clear();
    C.setup_position("3k4/3b4/8/1Q5P/6B1/1r4N1/4p1nR/4K3 w - -");
    C.gen_pseudo_legal_moves(moves);
    assert(moves.size() == 39);
    moves.clear();
    C.setup_position("3k2n1/3b3P/8/1Q6/6B1/1r4B1/1p2p1NR/n1N1K3 w - -");
    C.gen_pseudo_legal_moves(moves);
    assert(moves.size() == 58);
    moves.clear();
    assert(C.enter_move("g4h3"));
    C.gen_pseudo_legal_moves(moves);
    assert(moves.size() == 32);
    moves.clear();
    C.setup_position("k7/8/8/7p/4p1pP/1p2P1PQ/PP5B/K4N1R w - -");
    C.gen_pseudo_legal_moves(moves);
    assert(moves.size() == 9);
    std::vector<move_s> estimate =
        {{0, 8, 16, 0}, {0, 8, 17, 0}, {0, 8, 24, 0},
        {1, 5, 11, 0}, {2, 15, 6, 0}, {3, 7, 6, 0},
        {4, 23, 14, 0}, {4, 23, 30, 0}, {5, 0, 1, 0}};
    assert(moves.size() == estimate.size());
    for(auto est : estimate)
        assert(std::find(moves.begin(), moves.end(), est) != moves.end());
    moves.clear();
    C.setup_position("4r2k/5P2/4N3/6pP/B7/8/8/1K6 w - g6");
    C.gen_pseudo_legal_moves(moves, gen_mode::only_captures);
    assert(moves.size() == 11);
    moves.clear();
    C.gen_pseudo_legal_moves(moves, gen_mode::only_silent);
    assert(moves.size() == 19);
    moves.clear();
    assert(C.enter_move("f7f8r"));
    C.gen_pseudo_legal_moves(moves, gen_mode::only_captures);
    assert(moves.size() == 2);
    moves.clear();
    C.gen_pseudo_legal_moves(moves, gen_mode::only_silent);
    assert(moves.size() == 9);
}


void test_all::test_is_attacked_by() {
    auto C = chess_tst("4KR2/3P4/3nQ3/B6q/3b4/2N5/4p3/r3k3 w - -");
    const u8 black = C.black;
    const u8 white = C.white;
    assert(C.is_attacked_by_slider(str_to_coord("e8"), black));
    assert(C.is_attacked_by_slider(str_to_coord("e8"), white));
    assert(C.is_attacked_by_non_slider(str_to_coord("e8"), black));
    assert(C.is_attacked_by_non_slider(str_to_coord("e8"), white));
    assert(C.is_attacked_by_non_slider(str_to_coord("e8"), black));
    assert(!C.is_attacked_by_slider(str_to_coord("e1"), white));
    assert(!C.is_attacked_by_non_slider(str_to_coord("e1"), white));
    assert(C.is_attacked_by_slider(str_to_coord("e1"), black));
    assert(!C.is_attacked_by_non_slider(str_to_coord("e1"), black));
    assert(C.is_attacked_by_slider(str_to_coord("a5"), black));
}


bool test_all::test_sequence_of_moves(const std::string &str_moves,
                                      chess_tst &chess_obj) {
    for(auto S : split(str_moves)) {
        if(startswith(to_lower(S), "unmov")) {
            if(!chess_obj.unmake_move())
                return false;
            if(chess_obj.hash_key != chess_obj.setup_hash_key())
                return false;
        }
        else if(S[0] == '~') {
            S.erase(S.begin());
            if(chess_obj.enter_move(S))
                return false;
        }
        else {
            if(!chess_obj.enter_move(S))
                return false;
            else if(chess_obj.hash_key != chess_obj.setup_hash_key())
                return false;
        }
    }
    return true;
}


void test_all::test_legality() {
    auto C = chess_tst();
    auto moves = "~c1b2 ~c1a3 ~c1d3 ~b1d2 ~a1a2 ~a1a5 ~a1a7 "
        "~d1d8 ~d1e3 ~e1g1 ~e1e2 ~e2e5 ~e2e8 ~e2d3";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("8/8/1P3k1N/P1b3p1/8/8/3K4/8 w - -");
    moves = "~b6c5 ~b6a5 ~b6b5 ~b6b8 ~h6a8 ~h6a4 ~h6b5 ~h6b7 ~d2g1 "
        "b6b7 ~g5h6 ~g5f6 ~g5g6 ~c5h8 ~c5h7 ~c5h2 ~c5h1 ~f6h6 ~b6b8 ~f6g8 g5g4";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("r3k2r/p5pp/7N/4B3/4b3/6P1/7P/RN2K2R w KQkq -");
    moves = "e1g1 e8c8 UNMOVE UNMOVE ~e1c1 ~e1c8 ~e1g8 h2h3 ~e8g8 ~e8g1 "
        "~e8c1 UNMOVE e1e2 e8e7 e2e1 e7e8 ~e1g1 h2h3 ~e8c8 UNMOVE UNMOVE "
        "UNMOVE UNMOVE UNMOVE h1g1 a8b8 g1h1 b8a8 ~e1g1 h2h3 ~e8g8";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("rn2k2r/p5pp/8/8/8/6P1/P6P/R3K1NR w KQkq -");
    moves = "a1b1 h8g8 b1a1 g8h8 ~e1c1 h2h3 ~e8g8";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("r3k2r/p5pp/7N/8/2B1b3/6P1/7P/RN2K2R w KQkq -");
    assert(C.enter_move("e1g1"));

    C.setup_position("r3k2r/1p5p/1N6/4B3/4b3/1n6/1P5P/R3K2R w KQkq -");
    moves = "e5h8 e4h1 ~e1g1 ~e8g8 b6a8 b3a1 ~e1c1 ~e8c8";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("3Q4/8/7B/8/q2N4/2p2Pk1/8/2n1K2R b K -");
    moves = "g3g2 UNMOVE ~g3h4 ~g3g4 ~g3f4 ~g3f3 ~g3f2 ~g3h2 ~g3h3 a4b3 "
        "~e1d1 ~e1d2 ~e1e2 ~e1f2 ~e1d1 e1f1 UNMOVE e1g1";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("4k3/4r3/8/b7/1N5q/4B3/5P2/2R1K3 w - - ");
    moves = "~b4a2 ~e3g5 ~f2f3 e1d1 e8d8 ~d2c4 UNMOVE ~e2d3 UNMOVE ~f2f3 "
        "c1a1 a5b4 ~e3d2";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position(
        "rnbq1k1r/1p1Pbppp/2p5/p7/1PB5/8/P1P1NnPP/RNBQK2R w KQ a6");
    assert(C.enter_move("b4b5"));

    C.setup_position("4k3/3ppp2/4b3/1n5Q/B7/8/4R3/4K3 b - -");
    moves = "b5a7 UNMOVE e6g4 ~f7f6 UNMOVE e8d8 e2d2";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("8/8/8/8/k1p4R/8/3P4/2K5 w - -");
    moves = "d2d4 ~c4d3";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("3k2q1/2p5/8/3P4/8/8/K7/8 b - -");
    moves = "c7c5 ~d5c6";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("8/8/8/8/k1p4B/8/3P4/2K5 w - -");
    moves = "d2d4 c4d3 UNMOVE c4c3 d4d5";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("rnb2k1r/pp1Pbppp/2p5/q7/1PB5/8/P1P1NnPP/RNBQK2R w KQ -");
    moves = "b4a5 b7b5 a5b6";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("8/2p5/3p4/KP5r/1R3pPk/8/4P3/8 b - g3");
    assert(!C.enter_move("f4g3"));

    C.setup_position("8/8/3p4/KPp4r/1R3p1k/4P3/6P1/8 w - c6");
    assert(!C.enter_move("b5c6"));

    C.setup_position("8/2p5/8/KP1p3r/1R2PpPk/8/8/8 b - g3");
    assert(C.enter_move("f4g3"));

    C.setup_position("8/2p5/8/KP1p3r/1R2PpPk/8/8/8 b - e3");
    assert(C.enter_move("f4e3"));

    C.setup_position("8/8/3p4/1Pp3r1/1K2Rp1k/8/4P1P1/8 w - c6");
    assert(C.enter_move("b5c6"));

    C.setup_position("8/5k2/8/3pP3/2B5/8/8/4K3 w - d6");
    moves = "e5d6 ~f7e6";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("8/3r4/5k2/8/4pP2/2K5/8/5R2 b - f3");
    assert(C.enter_move("e4f3"));

    C.setup_position("8/8/K2p4/1Pp4r/1R3p1k/8/4P1P1/8 w - c6");
    moves = "b5c6 h4g3 ~a6a5 ~a6b5";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("3k2r1/2p5/8/3P4/8/8/K7/8 b - -");
    moves = "c7c5 d5c6 UNMOVE d5d6 c5c4";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("8/3n3R/2k1p1p1/1r1pP1P1/p2P3P/1NK5/8/8 w - -");
    moves = "b3a1 b5b1";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("3k4/3r4/8/3b4/8/8/5P2/3K4 b - -");
    moves = "d5b3 ~f2f4 d1e1";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("3k4/8/8/8/8/2n5/3P4/3K4 w - -");
    moves = "~d2d3 d2c3 UNMOVE d1c2";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("3k4/8/8/8/8/1b2N3/8/3K4 w - -");
    moves = "~e3d5 e3c2 UNMOVE d1c1";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("1n2k3/1pp5/8/8/8/1P6/2PPB3/4K3 w - -");
    moves = "e2b5 b8c6 b5a4 e8d7";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("3q1rk1/3p4/8/8/8/8/3P4/3Q1RK1 b - -");
    moves = "f8f1 d1f1 d8f8 f1f8";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("8/5k2/8/8/2n5/8/5P2/1N3K2 b - -");
    moves = "c4d2 b1d2 f7f6 f2f4";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("6k1/8/8/8/8/8/8/4K2R w K -");
    moves = "e1g1 g8h8";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("r3k3/8/8/8/8/8/8/1K6 b q -");
    moves = "e8c8 b1a1";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("8/2pPk3/8/8/8/8/8/2K5 w - -");
    moves = "d7d8n e7d8 c1c2 d8c8";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("4K3/2P5/2B5/8/b7/1p6/7P/3k4 w - -");
    moves = "c6a4 UNMOVE ~c6h1 c7c8 d1c1 c6a4 ~b3a4 UNMOVE UNMOVE UNMOVE c7c8r";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("4k3/8/8/b7/1r6/8/1P1K1N2/8 b - - 0 1");
    moves = "b4d4 ~d2c3 ~d2e1 ~d2d3 ~d2d1 ~b2b4 ~f2d3 d2c1 UNMOVE d2c2 "
        "UNMOVE d2e3 UNMOVE d2e2";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position(
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -");
    moves = "d2d4 b2a1q h6f7";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("rnb2k1r/pp1PbBpp/1qp5/8/8/8/PPP1NnPP/RNBQK2R w KQ -");
    moves = "d7d8q e7d8 a2a4 b6b4 ~a4a5";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -");
    moves = "e1g1 f2d1 d7c8q";
    assert(test_sequence_of_moves(moves, C));

    C.setup_position("r3k3/5P2/8/8/8/8/8/4K3 b q -");
    assert(!C.enter_move("e8c8"));

    C.setup_position("4k2r/3P4/8/8/8/8/8/4K3 b k -");
    assert(!C.enter_move("e8g8"));

    C.setup_position("r3k2r/8/4N3/8/8/8/4p3/R3K2R w KQkq -");
    moves = "~e1g1 ~e1c1 e1e2 ~e8g8 ~e8c8 UNMOVE e6c7 ~e8g8 ~e8c8";
    assert(test_sequence_of_moves(moves, C));
}


void test_all::test_game_over() {
    auto C = chess_tst("7k/8/6RK/8/8/8/8/8 b - -");
    assert(!C.is_mate());
    assert(C.is_stalemate());
    assert(C.is_draw());
    assert(C.game_over());
    C.setup_position("4R2k/8/7K/8/8/8/8/8 b - -");
    assert(C.is_mate());
    assert(!C.is_stalemate());
    assert(!C.is_draw());
    assert(C.game_over());
    C.setup_position("7k/8/8/8/1p1p4/pPpPp3/PRP1P3/KRN5 w - -");
    assert(!C.is_mate());
    assert(C.is_stalemate());
    assert(C.is_draw());
    assert(C.game_over());
    C.setup_position("7k/8/8/8/1p1p4/pPpP4/PRP1P3/KRN5 w - -");
    assert(!C.is_mate());
    assert(!C.is_stalemate());
    assert(!C.is_draw());
    assert(!C.game_over());
    C.setup_position("7k/8/8/8/8/p7/Np6/KN6 w - -");
    assert(C.is_mate());
    assert(!C.is_stalemate());
    assert(!C.is_draw());
    assert(C.game_over());
    C.setup_position("r6k/8/8/8/3b4/8/8/KN6 w - -");
    assert(C.is_mate());
    assert(!C.is_stalemate());
    assert(!C.is_draw());
    assert(C.game_over());
    C.setup_position("8/6k1/8/6pn/pQ6/Pp3P1K/1P3P1P/6r1 w - -");
    assert(test_sequence_of_moves("b4e7 g7h6 e7b4 h6g7", C));
    assert(C.is_repetition());
    assert(!C.is_draw());
    assert(!C.game_over());
    C.setup_position("4k3/8/8/8/8/3r4/8/4K2R b K -");
    assert(test_sequence_of_moves("d3e3 e1d1 e3d3 d1e1", C));
    assert(!C.is_repetition());
    assert(!C.is_draw());
    assert(!C.game_over());
    C.setup_position("4k3/2p5/5R2/1P6/8/8/8/4K3 b - -");
    assert(test_sequence_of_moves("c7c5 f6e6 e8d8 e6d6 d8e8 d6f6", C));
    assert(!C.is_repetition());
    assert(!C.is_draw());
    assert(!C.game_over());
    C.setup_position("4k3/2p5/5R2/8/8/8/8/4K3 b - -");
    assert(test_sequence_of_moves("c7c5 f6e6 e8d8 e6d6 d8e8 d6e6", C));
    assert(C.is_repetition());
    assert(!C.is_N_fold_repetition(2));
    assert(!C.is_draw());
    assert(!C.game_over());
    C.setup_position("4k3/2p5/5R2/8/8/8/8/4K3 b - -");
    assert(test_sequence_of_moves("c7c5 f6e6 e8d8 e6d6 d8e8 d6e6 e8f8 "
        "e6f6 f8e8 f6e6", C));
    assert(C.is_repetition());
    assert(C.is_N_fold_repetition(2));
    assert(C.is_draw());
    assert(C.game_over());
    C.setup_position("4k3/8/5R2/2p5/8/8/8/4K3 w - -");
    assert(test_sequence_of_moves("f6e6 e8d8 e6d6 d8e8 d6e6 e8f8 e6f6 f8e8 "
        "f6e6 e8d8 e6d6 d8e8 d6e6", C));
    assert(C.is_repetition());
    assert(C.is_N_fold_repetition(3));
    assert(C.is_draw());
    assert(C.game_over());
    C.setup_position("4k3/8/8/8/8/8/8/3K4 w - -");
    assert(C.is_draw());
    assert(C.game_over());
    C.setup_position("44k3/8/6N1/8/8/8/1b6/3K4 w - -");
    assert(C.is_draw());
    assert(C.game_over());
    C.setup_position("4k3/8/6NN/8/8/8/8/3K4 w - -");
    assert(!C.is_draw());
    assert(!C.game_over());
    C.setup_position("4k3/8/4p2B/8/n7/8/8/3K4 w - -");
    assert(!C.is_draw());
    assert(!C.game_over());
    C.setup_position("4k3/8/7B/8/n7/8/1R6/3K4 w - -");
    assert(!C.is_draw());
    assert(!C.game_over());
}


void test_all::test_perft() {
    const std::string fen =
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -";
    u8 depth = 3;
    bool verbose = false;
    std::vector<u64> estimate = {0, 48, 2039, 97862, 4085603, 193690690,
                                 8031647685, 374190009323};

    auto E = engine();
    E.setup_position(fen.size() ? fen : E.start_pos);
    auto t0 = std::chrono::high_resolution_clock::now();

    u64 nodes = E.perft(depth, verbose);

    auto t1 = std::chrono::high_resolution_clock::now();
    if (!verbose && nodes == estimate.at(depth))
        return;
    std::chrono::duration<double> elapsed = t1 - t0;
    std::cout << "\nNodes searched:  " << nodes << std::endl;
    std::cout << "Nodes estimated: " << estimate.at(depth) << std::endl;
    std::cout << "Time spent: " << elapsed.count() << std::endl;
    std::cout << "Nodes per second = " <<
        double(nodes)/(elapsed.count() + 1e-6) << std::endl;
    assert(nodes == estimate.at(depth));
}


void test_all::test_see() {
    auto E = engine_tst();
    E.setup_position("3k4/3b4/2P5/1Q5p/6B1/1r4N1/4p1nR/4K3 w - -");
    auto occ = E.bb[0][E.occupancy_ix] | E.bb[1][E.occupancy_ix];
    auto to_coord = str_to_coord("d7");
    u64 ans_bb;
    E.min_attacker(to_coord, occ, E.white, ans_bb);
    assert (ans_bb == one_nth_bit(str_to_coord("c6")));
    to_coord = str_to_coord("e2");
    E.min_attacker(to_coord, occ, E.white, ans_bb);
    assert (ans_bb == one_nth_bit(str_to_coord("g3")));
    to_coord = str_to_coord("h5");
    E.min_attacker(to_coord, occ, E.white, ans_bb);
    assert (ans_bb == one_nth_bit(str_to_coord("g3")));
    to_coord = str_to_coord("b3");
    E.min_attacker(to_coord, occ, E.white, ans_bb);
    assert (ans_bb == one_nth_bit(str_to_coord("b5")));
    to_coord = str_to_coord("g2");
    E.min_attacker(to_coord, occ, E.white, ans_bb);
    assert (ans_bb == one_nth_bit(str_to_coord("h2")));
    to_coord = str_to_coord("b5");
    E.min_attacker(to_coord, occ, E.black, ans_bb);
    assert (ans_bb == one_nth_bit(str_to_coord("b3")));
    to_coord = str_to_coord("e1");
    E.min_attacker(to_coord, occ, E.black, ans_bb);
    assert (ans_bb == one_nth_bit(str_to_coord("g2")));
    to_coord = str_to_coord("g4");
    E.min_attacker(to_coord, occ, E.black, ans_bb);
    assert (ans_bb == one_nth_bit(str_to_coord("h5")));
    E.setup_position("1b3rk1/4n2p/6p1/5p2/6P1/3B2N1/6PP/5RK1 w - -");
    auto move = E.move_from_str("g4f5");
    assert(E.static_exchange_eval(move) == E.material.at(E.pawn_ix));
    E.setup_position("2b2rk1/4n2p/6p1/5p2/6P1/3B2N1/6PP/5RK1 w - -");
    move = E.move_from_str("g4f5");
    assert(E.static_exchange_eval(move) == 0);
    E.setup_position("B2r2k1/4nb2/1N3n2/3R4/8/2N5/8/3R2K1 b - -");
    move = E.move_from_str("e7d5");
    assert(E.static_exchange_eval(move) ==
        E.material.at(E.rook_ix) - E.material.at(E.knight_ix));
    move = E.move_from_str("f6d5");
    assert(E.static_exchange_eval(move) ==
        E.material.at(E.rook_ix) - E.material.at(E.knight_ix));
    move = E.move_from_str("f7d5");
    assert(E.static_exchange_eval(move) ==
        E.material.at(E.rook_ix) - E.material.at(E.bishop_ix));
    move = E.move_from_str("d8d5");
    assert(E.static_exchange_eval(move) == 0);
    E.setup_position("3rk3/8/8/8/8/8/8/3QK3 w - -");
    move = E.move_from_str("d1d8");
    assert(E.static_exchange_eval(move) ==
        E.material.at(E.rook_ix) - E.material.at(E.queen_ix));
    E.setup_position("q2b1rk1/5pp1/n6p/8/2Q5/7P/3P2P1/2R2BK1 w - -");
    move = E.move_from_str("c4a6");
    assert(E.static_exchange_eval(move) == E.material.at(E.knight_ix));
    E.enter_move("d2d3");
    E.enter_move("h6h5");
    assert(E.static_exchange_eval(move) ==
        E.material.at(E.knight_ix) - E.material.at(E.queen_ix));
    E.setup_position("r1b3k1/4nr1p/5qp1/5p2/6P1/3B1RN1/5RPP/5QK1 w - -");
    move = E.move_from_str("g4f5");
    assert(E.static_exchange_eval(move) == E.material.at(E.pawn_ix));
    E.setup_position("r1b3k1/4nr1p/5qp1/5p2/6P1/3R1RN1/5BPP/5QK1 w - -");
    move = E.move_from_str("g4f5");
    assert(E.static_exchange_eval(move) == 0);
    E.setup_position("5rk1/5q2/5r2/8/8/5R2/5R2/5QK1 w - -");
    move = E.move_from_str("f3f6");
    assert(E.static_exchange_eval(move) == E.material.at(E.rook_ix));
    E.setup_position("5rk1/5q2/5r2/8/8/5R2/5B2/5QK1 w - -");
    move = E.move_from_str("f3f6");
    assert(E.static_exchange_eval(move) == 0);
    E.setup_position("3bn1k1/3np1p1/rpqr1p2/4P1PN/6NB/5R2/5RPP/5QK1 w - -");
    move = E.move_from_str("e5f6");
    assert(E.static_exchange_eval(move) == E.material.at(E.pawn_ix));
    move = E.move_from_str("g5f6");
    assert(E.static_exchange_eval(move) == E.material.at(E.pawn_ix));
    move = E.move_from_str("h5f6");
    assert(E.static_exchange_eval(move) ==
        2*E.material.at(E.pawn_ix) - E.material.at(E.knight_ix));
    E.setup_position("5b2/8/8/2k5/1p6/2P5/3B4/4K3 w - -");
    move = E.move_from_str("c3b4");
    assert(E.static_exchange_eval(move) == E.material.at(E.pawn_ix));
    E.setup_position("5b2/8/8/k7/1p6/2P5/3B4/4K3 w - -");
    move = E.move_from_str("c3b4");
    assert(E.static_exchange_eval(move) == 0);
    E.setup_position("3r2k1/6p1/2q2r1p/pp1p1p2/3P4/1QN1PBP1/PP3P1P/6K1 w - -");
    move = E.move_from_str("c3d5");
    assert(E.static_exchange_eval(move) == E.material.at(E.pawn_ix));
}


void test_all::test_next_move() {
    auto E = engine_tst();
    move_s ans;

    E.setup_position("4k3/3p2p1/1b5b/n7/8/8/p2Q4/3K4 w - -");
    std::vector<move_s> moves;
    move_s tt_move = E.not_a_move;
    unsigned stage = 0, move_num = unsigned(-1);
    ans = E.next_move(moves, tt_move, move_num, stage, 1);
    assert(ans == E.move_from_str("d2a2"));
    ans = E.next_move(moves, tt_move, move_num, stage, 1);
    assert(ans == E.move_from_str("d2h6"));
    ans = E.next_move(moves, tt_move, move_num, stage, 1);
    assert(ans == E.move_from_str("d2a5"));

    E.setup_position("K3n2k/3P4/8/8/1N6/8/b7/8 w - -");
    moves.clear();
    stage = 0;
    move_num = unsigned(-1);
    ans = E.next_move(moves, tt_move, move_num, stage, 1);
    assert(ans == E.move_from_str("d7e8q"));
    ans = E.next_move(moves, tt_move, move_num, stage, 1);
    assert(ans == E.move_from_str("d7e8r"));
    ans = E.next_move(moves, tt_move, move_num, stage, 1);
    assert(ans == E.move_from_str("d7d8q"));
    ans = E.next_move(moves, tt_move, move_num, stage, 1);
    assert(ans == E.move_from_str("d7e8b"));

    E.setup_position("3rrqk1/1P3pp1/7p/6P1/8/PR6/1Q3P1P/1R4K1 w - -");
    moves.clear();
    stage = 0;
    move_num = unsigned(-1);
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("b7b8q"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("b7b8r"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("b7b8b"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("b7b8n"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("g5h6"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.not_a_move);

    E.setup_position("k7/pp2q1p1/7p/8/1b5R/N1P5/1P3PP1/K3Q2R w - -");
    moves.clear();
    stage = 0;
    move_num = unsigned(-1);
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("c3b4"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("h4b4"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("e1e7"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.not_a_move);

    E.setup_position("k7/pp2q1p1/7p/8/1b5R/N1P5/1P3PP1/K3Q2R b - -");
    moves.clear();
    stage = 0;
    move_num = unsigned(-1);
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("b4a3"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.move_from_str("e7e1"));
    ans = E.next_move(moves, tt_move, move_num, stage, 0);
    assert(ans == E.not_a_move);
}


void test_all::test_set_time_for_move() {
    auto K = k2_tst();

    K.move_cr = 0;
    K.current_clock = 1;
    K.time_inc = 0;
    K.moves_per_time_control = 0;
    K.set_time_for_move();
    assert(is_close(K.time_for_move,  1./60));
    assert(is_close(K.max_time_for_move, 3./60 - K.time_margin));

    K.moves_per_time_control = 10;
    K.set_time_for_move();
    assert(is_close(K.time_for_move,  1./20));
    assert(is_close(K.max_time_for_move, 3./20 - K.time_margin));

    K.moves_per_time_control = 2;
    K.set_time_for_move();
    assert(is_close(K.time_for_move,  1./2));
    assert(is_close(K.max_time_for_move, 1./2 - K.time_margin));

    K.moves_per_time_control = 0;
    K.time_inc = .1;
    K.set_time_for_move();
    assert(is_close(K.time_for_move,  1./60 + .1));
    assert(is_close(K.max_time_for_move, 3./60 + .3 - K.time_margin));

    K.move_cr = 6;
    K.time_inc = 0;
    K.set_time_for_move();
    assert(is_close(K.time_for_move,  1./60));
    assert(is_close(K.max_time_for_move, 3./60 - K.time_margin));

    K.moves_per_time_control = 10;
    K.set_time_for_move();
    assert(is_close(K.time_for_move,  1./4));
    assert(is_close(K.max_time_for_move, 1./4 - K.time_margin));

    K.moves_per_time_control = 0;
    K.time_inc = .1;
    K.set_time_for_move();
    assert(is_close(K.time_for_move,  1./60 + .1));
    assert(is_close(K.max_time_for_move, 3./60 + .3 - K.time_margin));
}


void test_all::test_update_clock() {
    auto K = k2_tst();

    K.move_cr = 0;
    K.moves_per_time_control = 1;
    K.time_per_time_control = 2;
    K.current_clock = 1;
    K.timer_start();
    K.update_clock();
    assert(K.move_cr == 0);
    assert(K.current_clock >= 2.8);
}

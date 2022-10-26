#include "board.h"


void board::init() {
    bb[0] = {0};
    bb[1] = {0};
    side = white;
    en_passant_bitboard = 0;
    castling_rights = {0, 0};
    reversible_halfmoves = 0;
    hash_key = 0;
    is_reversible = true;
    state_storage.clear();
    state_storage.reserve(64);
}


board::board() : side(true),
                 state_storage(0),
                 is_reversible(false),
                 rnd{{{{0}}}},
                 rnd_castling{{0}},
                 rnd_en_passant{{0}} {
    std::uniform_int_distribution<u64> rnd_distr(0, u64(-1));
    std::mt19937 rnd_gen;
    for(auto color : {black, white})
        for(auto index: {pawn_ix, knight_ix, bishop_ix, rook_ix,
                         queen_ix, king_ix})
            for(size_t sq = 0; sq < 64; ++sq)
                rnd[color][index][sq] = rnd_distr(rnd_gen);
    for(auto &it : rnd_castling)
        it = rnd_distr(rnd_gen);
    for(auto &it : rnd_en_passant)
        it = rnd_distr(rnd_gen);
}


bool board::setup_position(const std::string &fen) {
    init();
    const auto fen_split = split(fen, ' ', 1);
    const auto rows = split(fen_split[0], '/');
    for(u8 i = 0; i < rows.size(); ++i)
        if(i >= 8 || !setup_row(u8(7 - i), rows[i]))
            return false;
    setup_occupancy();
    if(!setup_rest(fen_split[1]))
        return false;
    hash_key = setup_hash_key();
    return true;
}


void board::enter_move(const std::string &str) {
    make_move(move_from_str(str));
}


void board::make_move(const move_s &move) {
    cur_move = move;
    state_storage.push_back(static_cast<board_state &>(*this));
    is_reversible = true;
    make_castling(move);
    make_capture(move);
    const u8 new_index = make_promotion(move);
    make_common({new_index, move.from_coord, move.to_coord, move.promo});
    side = !side;
    hash_key ^= u64(-1);
}


bool board::unmake_move() {
    if(!state_storage.size())
        return false;
    side = !side;
    static_cast<board_state &>(*this) = state_storage.back();
    state_storage.pop_back();
    return true;
}


board::move_s board::move_from_str(const std::string &str) const {
    move_s ans;
    const u8 from_coord = str_to_coord(str.substr(0, 2));
    ans.index = find_index(side, one_nth_bit(from_coord));
    ans.promo = queen_ix;
    if(str.size() == 5)
        ans.promo = char_to_bb_index(str.at(4));
    ans.from_coord = from_coord;
    ans.to_coord = str_to_coord(str.substr(2, 2));
    return ans;
}


std::string board::move_to_str(const move_s &move) const {
    std::string ans;
    ans.append(coord_to_str(move.from_coord));
    ans.append(coord_to_str(move.to_coord));
    if(move.index == pawn_ix && get_row(move.to_coord) ==
            (side == white ? 7 : 0))
        ans.push_back(bb_index_to_char(black, move.index));
    return ans;
}


std::string board::board_to_fen() const {
    std::string ans = bitboards_to_fen();
    ans += side == white ? " w " : " b ";
    ans += castling_to_fen();
    ans += en_passant_to_fen();
    ans += std::to_string(reversible_halfmoves);
    return ans;
}


std::string board::board_to_ascii() const {
    std::string ans = "      --";
    for(u8 row = 0; row < 8; ++row) {
        ans += std::string(31, '-');
        if(row)
            ans += "|";
        ans += "\n    " + std::to_string(8 - row) + " ";
        for(u8 col = 0; col < 8; ++col) {
            ans += "| ";
            const char sq = square_to_fen(col, u8(7 - row));
            if(sq == '0')
                ans += "  ";
            else if(isupper(sq))
                ans += " " + std::string(1, sq);
            else
                ans += "*" + std::string(1, char(toupper(sq)));
        }
        ans += "|\n";
        if(row < 7)
            ans += "      |";
    }
    ans += "      " + std::string(32, '-') + "\n     ";
    ans += "   a   b   c   d   e   f   g   h\n";
    return ans;
}


bool board::setup_row(u8 row_num, const std::string &row_str) {
    u8 col = 0;
    for(auto sym : row_str) {
        if(sym >= '0' && sym <= '8') {
            col = u8(col + sym - '0');
            continue;
        }
        bool color;
        const auto index = char_to_bb_index(sym, color);
        if(index == u8(-1))
            return false;
        bb[color][index] |= one_nth_bit(get_coord(col, row_num));
        col++;
    }
    return true;
}


bool board::setup_rest(const std::string &rest_str) {
    const auto rest = split(rest_str, ' ', 0);
    if(rest[0].size() != 1)
        return false;
    switch(rest[0][0]) {
        case 'w' : side = white; break;
        case 'b' : side = black; break;
        default: return false;
    }
    setup_castling_rights(rest[1]);
    setup_en_passant(rest[2]);
    if(rest.size() == 3)
        return true;
    reversible_halfmoves = std::stoi(rest[3]);
    return true;
}


void board::setup_occupancy() {
    setup_side_occupancy(black);
    setup_side_occupancy(white);
}


void board::setup_side_occupancy(const bool color) {
    bb[color][occupancy_ix] = bb[color][pawn_ix] |
        bb[color][knight_ix] | bb[color][bishop_ix] |
        bb[color][rook_ix] | bb[color][queen_ix] | bb[color][king_ix];
}


u8 board::char_to_bb_index(char symbol, bool &color) const {
    color = symbol < 'a';
    if(color)
        symbol = char(symbol + 'a' - 'A');
    switch(symbol) {
        case 'p': return pawn_ix;
        case 'n': return knight_ix;
        case 'b': return bishop_ix;
        case 'r': return rook_ix;
        case 'q': return queen_ix;
        case 'k': return king_ix;
        default: return u8(-1);
    }
}


u8 board::char_to_bb_index(char symbol) const {
    const bool color = symbol < 'a';
    if(color)
        symbol = char(symbol + 'a' - 'A');
    switch(symbol) {
        case 'p': return pawn_ix;
        case 'n': return knight_ix;
        case 'b': return bishop_ix;
        case 'r': return rook_ix;
        case 'q': return queen_ix;
        case 'k': return king_ix;
        default: return u8(-1);
    }
}


void board::setup_en_passant(const std::string &coord_str) {
    if(coord_str == "-")
        return;
    const u64 from_fen = one_nth_bit(str_to_coord(coord_str));
    const u64 p_att1 = all_pawn_attacks_queenside(bb[side][pawn_ix],
                                                  side, u64(-1));
    const u64 p_att2 = all_pawn_attacks_kingside(bb[side][pawn_ix],
                                                  side, u64(-1));
    en_passant_bitboard = from_fen & (p_att1 | p_att2);
}


void board::setup_castling_rights(const std::string &cstl_str) {
    castling_rights[black] = 0;
    castling_rights[white] = 0;
    if(cstl_str == "-")
        return;
    for(auto ch : cstl_str) {
        const u8 color = ch < 'a' ? white : black;
        const u8 tmp = ch < 'a' ? u8(ch + 'a' - 'A') : u8(ch);
        const u8 val = (tmp == 'k' ? castle_kingside : castle_queenside);
        castling_rights[color] |= val;
    }
}


void board::make_common(const move_s &move) {
    bb[side][move.index] &= ~one_nth_bit(move.from_coord);
    bb[side][move.index] |= one_nth_bit(move.to_coord);
    bb[side][occupancy_ix] &= ~one_nth_bit(move.from_coord);
    bb[side][occupancy_ix] |= one_nth_bit(move.to_coord);
    hash_key ^= rnd[side][move.index][move.from_coord] ^
        rnd[side][move.index][move.to_coord];
    if(is_reversible)
        reversible_halfmoves++;
    else
        reversible_halfmoves = 0;
}


void board::make_castling(const move_s &move) {

    const size_t store_index = castling_index();
    update_castling_rights(move);
    hash_key ^= rnd_castling[store_index] ^
        rnd_castling[castling_index()];

    if(move.index != king_ix)
        return;
    const int d_col = int(get_col(move.to_coord)) -
        int(get_col(move.from_coord));
    if(std::abs(d_col) != 2)
        return;
    const u8 row = side ? 0 : 7;
    const u8 from_col = d_col > 0 ? 7 : 0;
    const u8 to_col = d_col > 0 ? 5 : 3;
    const u8 fr_coord = get_coord(from_col, row);
    const u8 to_coord = get_coord(to_col, row);
    bb[side][rook_ix] &= ~one_nth_bit(fr_coord);
    bb[side][rook_ix] |= one_nth_bit(to_coord);
    bb[side][occupancy_ix] &= ~one_nth_bit(fr_coord);
    bb[side][occupancy_ix] |= one_nth_bit(to_coord);
    hash_key ^= rnd[side][rook_ix][fr_coord] ^
        rnd[side][rook_ix][to_coord];
}


void board::update_castling_rights(const move_s &move) {
    if(move.index == king_ix) {
        castling_rights[side] = 0;
        return;
    }
    if(bb[!side][rook_ix] & one_nth_bit(move.to_coord))
        update_castling_rights_on_rook_capture(move.to_coord);
    if(!castling_rights[side] || move.index != rook_ix)
        return;
    const u8 from_col = get_col(move.from_coord);
    if(from_col == 0)
        castling_rights[side] &= u8(~castle_queenside);
    else if(from_col == 7)
        castling_rights[side] &= u8(~castle_kingside);
}


void board::update_castling_rights_on_rook_capture(const u8 to_coord) {
    if(side == black) {
        if(to_coord == str_to_coord("a1"))
            castling_rights[white] &= u8(~castle_queenside);
        else if(to_coord == str_to_coord("h1"))
            castling_rights[white] &= u8(~castle_kingside);
        return;
    }
    if(to_coord == str_to_coord("a8"))
        castling_rights[black] &= u8(~castle_queenside);
    else if(to_coord == str_to_coord("h8"))
        castling_rights[black] &= u8(~castle_kingside);
}


void board::make_capture(const move_s &move) {
    make_en_passant(move);
    update_en_passant_bb(move);
    const u64 bit = one_nth_bit(move.to_coord);
    if(!(bb[!side][occupancy_ix] & bit))
        return;
    const u8 captured_index = find_index(!side, bit);
    bb[!side][captured_index] &= ~bit;
    bb[!side][occupancy_ix] &= ~bit;
    hash_key ^= rnd[!side][captured_index][move.to_coord];
    is_reversible = false;
}


void board::make_en_passant(const move_s &m) {
    if(m.index != pawn_ix || one_nth_bit(m.to_coord) !=
            en_passant_bitboard)
        return;
    if(get_nth_bit(bb[!side][occupancy_ix], m.to_coord))
        return;
    const u8 opp_pawn_coord = u8(m.to_coord + (side == black ? 8 : -8));
    const u64 opp_pawn_bb = one_nth_bit(opp_pawn_coord);
    bb[!side][pawn_ix] &= ~opp_pawn_bb;
    bb[!side][occupancy_ix] &= ~opp_pawn_bb;
    hash_key ^= rnd[!side][pawn_ix][opp_pawn_coord];
    is_reversible = false;
}


void board::update_en_passant_bb(const move_s &m) {
    const u8 store_enps = trail_zeros(en_passant_bitboard);
    if(m.index != pawn_ix ||
            std::abs(i8(m.from_coord) - i8(m.to_coord)) != 16) {
        en_passant_bitboard = 0;
        hash_key ^= rnd_en_passant[enps_ix(store_enps)] ^
            rnd_en_passant[0];
        return;
    }
    const int shift = side == white ? 8 : -8;
    u8 enps_coord = u8(m.from_coord + shift);
    en_passant_bitboard = one_nth_bit(enps_coord) &
        (all_pawn_attacks_queenside(bb[!side][pawn_ix], !side, u64(-1)) |
        all_pawn_attacks_kingside(bb[!side][pawn_ix], !side, u64(-1)));
    if(!en_passant_bitboard)
        enps_coord = u8(-1);
    hash_key ^= rnd_en_passant[enps_ix(store_enps)] ^
        rnd_en_passant[enps_ix(enps_coord)];
}


u8 board::make_promotion(const move_s &move) {
    if(move.index != pawn_ix)
        return move.index;
    is_reversible = false;
    if(get_row(move.to_coord) != (side == white ? 7 : 0))
        return move.index;
    bb[side][pawn_ix] &= ~one_nth_bit(move.from_coord);
    hash_key ^= rnd[side][pawn_ix][move.from_coord] ^
        rnd[side][move.promo][move.from_coord];
    return move.promo;
}


u8 board::find_index(const bool color, const u64 given_bb) const {
    for(auto i : {pawn_ix, knight_ix, bishop_ix, rook_ix, queen_ix, king_ix})
        if(bb[color][i] & given_bb)
            return i;
    return u8(-1);
}


std::string board::bitboards_to_fen() const {
    std::string ans;
    for(int row = 0; row < 8; ++row) {
        for(int col = 0; col < 8; ++col)
            ans += square_to_fen(u8(col), u8(7 - row));
        ans += "/";
    }
    return compress_fen(ans.substr(0, ans.size() - 1));
}


char board::square_to_fen(const u8 col, const u8 row) const {
    for(bool color : {black, white})
        for(u8 index : {pawn_ix, knight_ix, bishop_ix, rook_ix,
                queen_ix, king_ix})
            if(get_nth_bit(bb[color][index], get_coord(col, row)))
                return bb_index_to_char(color, index);
    return '0';
}


std::string board::en_passant_to_fen() const {
    if(!en_passant_bitboard)
        return "- ";
    const u8 en_pass_coord = trail_zeros(en_passant_bitboard);
    return coord_to_str(en_pass_coord) + " ";
}


std::string board::compress_fen(std::string in) const {
    std::string ans;
    size_t start = 0, end = 0, pos;
    while(true) {
        if((pos = in.find_first_of('0', start)) == std::string::npos)
            break;
        ans.append(in.substr(end, pos - end));
        if((end = in.find_first_not_of('0', pos)) == std::string::npos)
            end = in.size();
        ans.append(std::string(1, char('0' + end - pos)));
        start = end + 1;
    }
    ans.append(in.substr(start - 1));
    return ans;
}


char board::bb_index_to_char(const bool color, const u8 index) const {
    std::string pcs[2] = {"pnbrqk", "PNBRQK"};
    return pcs[color].at(index);
}


std::string board::castling_to_fen() const {
    if(!castling_rights[black] && !castling_rights[white])
        return "- ";
    std::string ans;
    if(castling_rights[white] & castle_kingside)
        ans += 'K';
    if(castling_rights[white] & castle_queenside)
        ans += 'Q';
    if(castling_rights[black] & castle_kingside)
        ans += 'k';
    if(castling_rights[black] & castle_queenside)
        ans += 'q';
    return ans + ' ';
}


u64 board::setup_hash_key() {
    u64 ans = 0;
    for(auto color : {black, white})
        for(u8 coord = 0; coord < 64; ++coord) {
            const u8 index = find_index(color, one_nth_bit(coord));
            if(index == u8(-1))
                continue;
            ans ^= rnd[color][index][coord];
        }
    ans ^= rnd_castling[castling_index()];
    ans ^= rnd_en_passant[enps_ix(trail_zeros(en_passant_bitboard))];
    if(side == black)
        ans ^= u64(-1);
    return ans;
}

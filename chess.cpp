#include "chess.h"





//--------------------------------
k2chess::k2chess() :
    rays{0, 8, 8, 4, 4, 8, 0},
    delta_col
        {
            { 0,  0,  0,  0,  0,  0,  0,  0},
            { 1,  0, -1,  1, -1, -1,  0,  1},  // king
            { 1,  0, -1,  1, -1, -1,  0,  1},  // queen
            { 1,  0, -1,  0,  0,  0,  0,  0},  // rook
            { 1, -1,  1, -1,  0,  0,  0,  0},  // bishop
            { 1,  2,  2,  1, -1, -2, -2, -1},  // knight
        },
    delta_row
        {
            { 0,  0,  0,  0,  0,  0,  0,  0},
            { 1,  1,  1,  0,  0, -1, -1, -1},  // king
            { 1,  1,  1,  0,  0, -1, -1, -1},  // queen
            { 0,  1,  0, -1,  0,  0,  0,  0},  // rook
            { 1,  1, -1, -1,  0,  0,  0,  0},  // bishop
            { 2,  1, -1, -2, -2, -1,  1,  2},  // knight
        },
       is_slider{false, false, true, true, true, false, false},
       pc_streng{0, 0, 12, 6, 4, 4, 1},
       streng{0, 15000, 120, 60, 40, 40, 10},
       sort_streng{0, 15000, 120, 60, 41, 39, 10}
{
    cv = cur_moves;
    memset(b_state, 0, sizeof(b_state));
    state = &b_state[prev_states];
    coords[black].board = b;
    coords[black].streng = sort_streng;
    coords[white].board = b;
    coords[white].streng = sort_streng;

    SetupPosition(start_position);
}





//--------------------------------
void k2chess::InitAttacks()
{
    memset(attacks, 0, sizeof(attacks));
    memset(xattacks, 0, sizeof(xattacks));
    bool sides[] = {black, white};
    for(auto stm : sides)
    {
        for(auto it = coords[stm].rbegin(); it != coords[stm].rend(); ++it)
            InitAttacksOnePiece(*it, false);

        auto k_coord = *king_coord[stm];
        b[k_coord] = set_piece_color(black_queen, stm);
        InitAttacksOnePiece(k_coord, true);
        b[k_coord] = set_piece_color(black_king, stm);
    }
}





//--------------------------------
void k2chess::InitAttacksOnePiece(coord_t coord, bool use_extended_attacks)
{
    const auto type = get_piece_type(b[coord]);
    if(type == pawn)
    {
        return;
    }
    const auto max_len = is_slider[type] ?
               std::max((size_t) board_height, (size_t) board_width) : 1;
    const auto color = get_piece_color(b[coord]);
    auto it = find_piece(color, coord);
    auto index = it.get_array_index();

    for(auto ray = 0; ray < rays[type]; ray++)
    {
        int col_ = get_col(coord);
        int row_ = get_row(coord);
        for(size_t i = 0; i < max_len; ++i)
        {
            col_ += delta_col[type][ray];
            row_ += delta_row[type][ray];
            if(!col_within(col_) || !row_within(row_))
                break;
            if(!use_extended_attacks)
                attacks[color][get_coord(col_, row_)] |= (1 << index);
            else
                xattacks[color][get_coord(col_, row_)] |= (1 << index);
            if(b[get_coord(col_, row_)] != empty_square)
                break;
        }
    }
}





//--------------------------------
void k2chess::UpdateAttacks()
{

}





//--------------------------------
void k2chess::UpdateAttacksOnePiece()
{

}





//--------------------------------
bool k2chess::SetupPosition(const char *fen)
{
    char *ptr = const_cast<char *>(fen);
    material[black] = 0;
    material[white] = 0;
    pieces[black] = 0;
    pieces[white] = 0;
    reversible_moves = 0;
    memset(quantity, 0, sizeof(quantity));
    ply = 0;

    if((ptr = ParseMainPartOfFen(ptr)) == nullptr)
        return false;
    if((ptr = ParseSideToMoveInFen(ptr)) == nullptr)
        return false;
    if((ptr = ParseCastlingRightsInFen(ptr)) == nullptr)
        return false;
    if((ptr = ParseEnPassantInFen(ptr)) == nullptr)
        return false;
    if(*(++ptr) && *(++ptr))  // set up number of reversible moves
        while(*ptr >= '0' && *ptr <= '9')
        {
            reversible_moves *= 10;
            reversible_moves += (*ptr++ - '0');
        }
    InitPieceLists();
    king_coord[white] = --coords[white].end();
    king_coord[black] = --coords[black].end();

    InitAttacks();
    done_moves.clear();
    return true;
}




//--------------------------------
bool k2chess::InitPieceLists()
{
    coords[black].clear();
    coords[white].clear();
    for(size_t i = 0; i < sizeof(b)/sizeof(*b); i++)
    {
        if(b[i] == empty_square)
            continue;
        coords[get_piece_color(b[i])].push_back(i);

        const auto color = get_piece_color(b[i]);
        const auto type = get_piece_type(to_black(b[i]));
        quantity[color][type]++;
    }

    coords[black].sort();
    coords[white].sort();

    return true;
}





//--------------------------------
char* k2chess::ParseMainPartOfFen(char *ptr)
{
    char chars[] = "kqrbnpKQRBNP";
    piece_t pcs[] =
    {
        black_king, black_queen, black_rook, black_bishop,
        black_knight, black_pawn, white_king, white_queen,
        white_rook, white_bishop, white_knight, white_pawn
    };

    for(int row = max_row; row >= 0; row--)
    {
        for(int col = 0; col <= max_col; col++)
        {
            auto chr = *ptr - '0';
            if(chr >= 1 && chr <= board_width)
            {
                for(auto i = 0; i < chr; ++i)
                    b[get_coord(col++, row)] = empty_square;
                col--;
            }
            else if(*ptr == '/')
                col--;
            else
            {
                size_t index = 0;
                const size_t max_index = sizeof(chars)/sizeof(*chars);
                for(; index < max_index; ++index)
                    if(*ptr == chars[index])
                        break;
                if(index >= max_index)
                    return nullptr;
                if(to_black(pcs[index]) == black_pawn
                        && (row < pawn_default_row
                            || row > max_col - pawn_default_row))
                    return nullptr;
                auto piece = pcs[index];
                b[get_coord(col, row)] = piece;
                auto color = get_piece_color(piece);
                material[color] += pc_streng[get_piece_type(piece)];
                pieces[color]++;
            }
            ptr++;
        }
    }
    return ptr;
}





//--------------------------------
char* k2chess::ParseSideToMoveInFen(char *ptr)
{
    ptr++;
    if(*ptr == 'b')
        wtm = black;
    else if(*ptr == 'w')
        wtm = white;
    else
        return nullptr;

    return ptr;
}





//--------------------------------
char *k2chess::ParseCastlingRightsInFen(char *ptr)
{
    castle_t castling_rights = 0;
    ptr += 2;
    char cstl_chr[] = "KQkq-";
    castle_t cstl_code[] = {white_can_castle_right, white_can_castle_left,
                            black_can_castle_right, black_can_castle_left, 0};
    coord_t k_col = default_king_col;
    coord_t k_row[] = {0, 0, max_row, max_row};
    coord_t r_col[] = {max_row, 0, max_row, 0};
    coord_t r_row[] = {0, 0, max_row, max_row};
    piece_t k_piece[] = {white_king, white_king, black_king, black_king};
    piece_t r_piece[] = {white_rook, white_rook, black_rook, black_rook};
    const size_t max_index = sizeof(cstl_chr)/sizeof(*cstl_chr);
    while(*ptr != ' ')
    {
        size_t index = 0;
        for(; index < max_index; ++index)
            if(*ptr == cstl_chr[index])
                break;
        if(index >= max_index)
            return nullptr;
        if(b[get_coord(k_col, k_row[index])] == k_piece[index] &&
                b[get_coord(r_col[index], r_row[index])] == r_piece[index])
            castling_rights |= cstl_code[index];
        ptr++;
    }
    state[0].cstl = castling_rights;
    return ptr;
}




//--------------------------------
char* k2chess::ParseEnPassantInFen(char *ptr)
{
    state[0].ep = 0;
    ptr++;
    if(*ptr != '-')
    {
        int col = *(ptr++) - 'a';
        int row = *(ptr++) - '1';
        int s = wtm ? -1 : 1;
        piece_t pawn = wtm ? white_pawn : black_pawn;
        if(b[get_coord(col - 1, row + s)] == pawn
                || b[get_coord(col + 1, row + s)] == pawn)
            state[0].ep = col + 1;
    }
    return ptr;
}





//--------------------------------
void k2chess::ShowMove(const coord_t from_coord, const coord_t to_coord)
{
    char *cur = cur_moves + move_max_display_length*(ply - 1);
    *(cur++) = get_col(from_coord) + 'a';
    *(cur++) = get_row(from_coord) + '1';
    *(cur++) = get_col(to_coord) + 'a';
    *(cur++) = get_row(to_coord) + '1';
    *(cur++) = ' ';
    *cur = '\0';
}





//--------------------------------
bool k2chess::MakeCastleOrUpdateFlags(const move_c move,
                                      const coord_t from_coord)
{
    const auto cs = state[ply].cstl;
    if(move.piece_iterator == king_coord[wtm])  // king moves
        state[ply].cstl &= wtm ?
                    ~(white_can_castle_right | white_can_castle_left) :
                    ~(black_can_castle_right | black_can_castle_left);
    else if(b[from_coord] == set_piece_color(black_rook, wtm))  // rook moves
    {
        const auto col = get_col(from_coord);
        const auto row = get_row(from_coord);
        if((wtm && row == 0) || (!wtm && row == max_col))
        {
            if(col == max_col)
                state[ply].cstl &= wtm ? ~white_can_castle_right :
                                         ~black_can_castle_right;
            else if(col == 0)
                state[ply].cstl &= wtm ? ~white_can_castle_left :
                                         ~black_can_castle_left;
        }
    }
    if(b[move.to_coord] == set_piece_color(black_rook, !wtm))  // rook is taken
    {
        const auto col = get_col(move.to_coord);
        const auto row = get_row(from_coord);
        if((wtm && row == max_col) || (!wtm && row == 0))
        {
            if(col == max_col)
                state[ply].cstl &= wtm ? ~black_can_castle_right :
                                         ~white_can_castle_right;
            else if(col == 0)
                state[ply].cstl &= wtm ? ~black_can_castle_left :
                                         ~white_can_castle_left;
        }
    }
    bool castling_rights_changed = (cs != state[ply].cstl);
    if(castling_rights_changed)
        reversible_moves = 0;

    if(!(move.flag & is_castle))
        return castling_rights_changed;

    coord_t rook_from_coord, rook_to_coord;
    const auto row = wtm ? 0 : max_row;
    if(move.flag == is_right_castle)
    {
        rook_from_coord = get_coord(max_col, row);
        const auto col = default_king_col + cstl_move_length - 1;
        rook_to_coord = get_coord(col, row);
    }
    else
    {
        rook_from_coord = get_coord(0, row);
        const auto col = default_king_col - cstl_move_length + 1;
        rook_to_coord = get_coord(col, row);
    }
    auto it = find_piece(wtm, rook_from_coord);
    assert(it != coords[wtm].end());
    state[ply].castled_rook_it = it;
    b[rook_to_coord] = b[rook_from_coord];
    b[rook_from_coord] = empty_square;
    *it = rook_to_coord;

    reversible_moves = 0;

    return false;
}





//--------------------------------
void k2chess::UnMakeCastle(const move_c move)
{
    auto rook_iterator = coords[wtm].begin();
    rook_iterator = state[ply].castled_rook_it;
    const auto from_coord =*rook_iterator;
    const auto to_coord = move.flag == is_right_castle ? max_col : 0;
    b[to_coord] = b[from_coord];
    b[from_coord] = empty_square;
    *rook_iterator = to_coord;
}





//--------------------------------
bool k2chess::MakeEnPassantOrUpdateFlags(const move_c move,
                                         const coord_t from_coord)
{
    const shifts_t delta = get_row(move.to_coord) - get_row(from_coord);
    if(std::abs(delta) == pawn_long_move_length)
    {
        bool opp_pawn_is_near = get_col(move.to_coord) < max_col &&
                b[move.to_coord + 1] == set_piece_color(black_pawn, !wtm);
        opp_pawn_is_near |= get_col(move.to_coord) > 0 &&
                b[move.to_coord - 1] == set_piece_color(black_pawn, !wtm);
        if(opp_pawn_is_near)
        {
            state[ply].ep = get_col(move.to_coord) + 1;
            return true;
        }
    }
    else if(move.flag & is_en_passant)
        b[move.to_coord + (wtm ? -board_width : board_width)] = empty_square;
    return false;
}





//--------------------------------
void k2chess::StoreCurrentBoardState(const move_c move,
                                     const coord_t from_coord)
{
    state[ply].cstl = state[ply - 1].cstl;
    state[ply].captured_piece = b[move.to_coord];

    state[ply].from_coord = from_coord;
    state[ply].reversible_moves = reversible_moves;
}





//--------------------------------
void k2chess::MakeCapture(const move_c move)
{
    auto to_coord = move.to_coord;
    if (move.flag & is_en_passant)
    {
        to_coord += wtm ? -board_width : board_width;
        material[!wtm]--;
        quantity[!wtm][pawn]--;
    }
    else
    {
        const auto piece_index = get_piece_type(state[ply].captured_piece);
        material[!wtm] -= pc_streng[piece_index];
        quantity[!wtm][piece_index]--;
    }
    auto it_cap = find_piece(!wtm, to_coord);
    assert(it_cap != coords[!wtm].end());
    state[ply].captured_it = it_cap;
    coords[!wtm].erase(it_cap);
    pieces[!wtm]--;
    reversible_moves = 0;
}





//--------------------------------
void k2chess::MakePromotion(const move_c move, iterator it)
{
    piece_t pcs[] = {0, black_queen, black_knight, black_rook, black_bishop};
    const auto piece_num = move.flag & is_promotion;
    if(piece_num == 0)
        return;

    const auto piece = pcs[piece_num];
    b[move.to_coord] = set_piece_color(piece, wtm);
    material[wtm] += pc_streng[get_piece_type(piece)] - pc_streng[pawn];
    quantity[wtm][pawn]--;
    quantity[wtm][get_piece_type(piece)]++;
    state[ply].nprom = ++it;
    --it;
    coords[wtm].move_element(king_coord[wtm], it);
    reversible_moves = 0;
}





//--------------------------------
bool k2chess::MakeMove(const move_c move)
{
    bool is_special_move = false;
    ply++;
    auto it = coords[wtm].begin();
    it = move.piece_iterator;
    const auto from_coord = *it;
    StoreCurrentBoardState(move, from_coord);
    reversible_moves++;

    if(move.flag & is_capture)
        MakeCapture(move);

    // trick: fast exit if not K|Q|R moves, and no captures
    if((b[from_coord] <= white_rook) || (move.flag & is_capture))
        is_special_move |= MakeCastleOrUpdateFlags(move, from_coord);

    state[ply].ep = 0;
    if(to_black(b[from_coord]) == black_pawn)
    {
        is_special_move |= MakeEnPassantOrUpdateFlags(move, from_coord);
        reversible_moves = 0;
    }
#ifndef NDEBUG
    ShowMove(from_coord, move.to_coord);
#endif // NDEBUG

    b[move.to_coord] = b[from_coord];
    b[from_coord] = empty_square;

    if(move.flag & is_promotion)
        MakePromotion(move, it);

    *it = move.to_coord;
    wtm = !wtm;

    return is_special_move;
}





//--------------------------------
void k2chess::UnmakeCapture(const move_c move)
{
    auto it_cap = coords[!wtm].begin();
    it_cap = state[ply].captured_it;

    if(move.flag & is_en_passant)
    {
        material[!wtm]++;
        quantity[!wtm][pawn]++;
        if(wtm)
        {
            b[move.to_coord - board_width] = black_pawn;
            *it_cap = move.to_coord - board_width;
        }
        else
        {
            b[move.to_coord + board_width] = white_pawn;
            *it_cap = move.to_coord + board_width;
        }
    }
    else
    {
        const auto capt_index = get_piece_type(state[ply].captured_piece);
        material[!wtm] += pc_streng[capt_index];
        quantity[!wtm][capt_index]++;
    }

    coords[!wtm].restore(it_cap);
    pieces[!wtm]++;
}





//--------------------------------
void k2chess::UnmakePromotion(const move_c move)
{
    piece_t pcs[] = {0, black_queen, black_knight, black_rook, black_bishop};
    const move_flag_t piece_num = move.flag & is_promotion;

    auto it_prom = coords[wtm].begin();
    it_prom = state[ply].nprom;
    auto before_king = king_coord[wtm];
    --before_king;
    coords[wtm].move_element(it_prom, before_king);
    const auto piece_index = get_piece_type(pcs[piece_num]);
    material[wtm] -= pc_streng[piece_index] - pc_streng[pawn];
    quantity[wtm][pawn]++;
    quantity[wtm][piece_num]--;
}





//--------------------------------
void k2chess::TakebackMove(const move_c move)
{
    const auto from_coord = state[ply].from_coord;
    auto it = coords[!wtm].begin();
    it = move.piece_iterator;
    *it = from_coord;

    if(move.flag & is_promotion)
        b[from_coord] = set_piece_color(white_pawn, !wtm);
    else
        b[from_coord] = b[move.to_coord];

    b[move.to_coord] = state[ply].captured_piece;
    reversible_moves = state[ply].reversible_moves;
    wtm = !wtm;

    if(move.flag & is_capture)
        UnmakeCapture(move);

    if(move.flag & is_promotion)
        UnmakePromotion(move);
    else if(move.flag & is_castle)
        UnMakeCastle(move);

    ply--;

#ifndef NDEBUG
    cur_moves[move_max_display_length*ply] = '\0';
#endif // NDEBUG
}





//--------------------------------
k2chess::iterator k2chess::find_piece(const bool stm,
                                      const coord_t coord)
{
    auto it = coords[stm].begin();
    const auto it_end = coords[stm].end();
    for(; it != it_end; ++it)
        if(*it == coord)
            break;
    return it;
}






//--------------------------------
bool k2chess::MakeMove(const char* str)
{
    const auto len = strlen(str);
    if(len < 4 || len > 5)  // only notation like 'g1f3', 'e7e8q' supported
        return false;

    const auto it = find_piece(wtm, get_coord(str));
    if(it == coords[wtm].end())
        return false;

    move_c move;
    move.to_coord = get_coord(&str[2]);
    move.piece_iterator = it;
    move.flag = InitMoveFlag(move, str[4]);
    if(move.flag == is_bad_move_flag)
        return false;

    MakeMove(move);
    done_moves.push_back(move);

    return true;
}





//--------------------------------
bool k2chess::TakebackMove()
{
    if(done_moves.size() == 0)
        return false;

    auto move = done_moves.back();
    done_moves.pop_back();
    TakebackMove(move);

    return true;
}





//--------------------------------
k2chess::move_flag_t k2chess::InitMoveFlag(const move_c move, char promo_to)
{
    move_flag_t ans = 0;
    auto it = coords[wtm].begin();
    it = move.piece_iterator;
    const auto from_coord = *it;
    const auto piece = get_piece_type(b[from_coord]);
    if(b[move.to_coord] != empty_square
            && get_piece_color(b[from_coord])
            != get_piece_color(b[move.to_coord]))
        ans |= is_capture;
    if(piece == pawn
            && get_row(from_coord) == (wtm ? board_height - 2 : 1))
    {
        if(promo_to == 'q' or promo_to == '\0')
            ans |= is_promotion_to_queen;
        else if(promo_to == 'r')
            ans |= is_promotion_to_rook;
        else if(promo_to == 'b')
            ans |= is_promotion_to_bishop;
        else if(promo_to == 'n')
            ans |= is_promotion_to_knight;
        else
            return is_bad_move_flag;
    }
    if(piece == pawn)
    {
        const auto en_passant_state = state[ply].ep;
        const auto delta = (en_passant_state - 1) - get_col(from_coord);
        auto required_row = pawn_default_row + pawn_long_move_length;
        if(wtm)
            required_row = max_row - required_row;
        if(en_passant_state && std::abs(delta) == 1
                && get_row(from_coord) == required_row)
            ans |= is_en_passant | is_capture;
    }
    if(piece == king && get_col(*king_coord[wtm]) == default_king_col)
    {
        if(get_col(move.to_coord) == default_king_col + cstl_move_length)
            ans |= is_right_castle;
        else if(get_col(move.to_coord) == default_king_col - cstl_move_length)
            ans |= is_left_castle;
    }

    return ans;
}





//--------------------------------
size_t k2chess::test_count_attacked_squares(bool stm, bool use_extended_attacks)
{
    size_t ans = 0;
    for(auto it : use_extended_attacks ? xattacks[stm] : attacks[stm])
        if(it != 0)
            ans++;
    return ans;
}





//--------------------------------
size_t k2chess::test_count_all_attacks(bool stm, bool use_extended_attacks)
{
    size_t ans = 0;
    for(auto it : use_extended_attacks ? xattacks[stm] : attacks[stm])
        ans += std::bitset<sizeof(attack_t)*CHAR_BIT>(it).count();
    return ans;
}





//--------------------------------
void k2chess::test_attack_tables(size_t att_w, size_t att_b,
                        size_t all_w, size_t all_b)
{
    size_t att_squares_w, att_squares_b, all_attacks_w, all_attacks_b;
    att_squares_w = test_count_attacked_squares(white, false);
    att_squares_b = test_count_attacked_squares(black, false);
    all_attacks_w = test_count_all_attacks(white, false);
    all_attacks_b = test_count_all_attacks(black, false);
    assert(att_squares_w == att_w);
    assert(att_squares_b == att_b);
    assert(all_attacks_w == all_w);
    assert(all_attacks_b == all_b);
}





//--------------------------------
void k2chess::RunUnitTests()
{
    assert(SetupPosition(start_position));
    assert(b[*coords[white].begin()] == white_pawn);
    assert(b[*(--coords[white].end())] == white_king);
    assert(b[*coords[white].rbegin()] == white_king);
    assert(b[*(--coords[white].rend())] == white_pawn);
    assert(b[*coords[black].begin()] == black_pawn);
    assert(b[*(--coords[black].end())] == black_king);
    assert(b[*coords[black].rbegin()] == black_king);
    assert(b[*(--coords[black].rend())] == black_pawn);
    assert(*king_coord[white] == get_coord(default_king_col, 0));
    assert(*king_coord[black] ==
           get_coord(default_king_col, max_row));
    assert(ply == 0);
    assert(wtm == white);

    assert(MakeMove("e2e4"));
    assert(b[get_coord("e2")] == empty_square);
    assert(b[get_coord("e4")] == white_pawn);
    assert(ply == 1);
    assert(wtm == black);
    assert(reversible_moves == 0);

    assert(MakeMove("b8c6"));
    assert(b[get_coord("b8")] == empty_square);
    assert(b[get_coord("c6")] == black_knight);
    assert(ply == 2);
    assert(wtm == white);
    assert(reversible_moves == 1);

    assert(MakeMove("f1c4"));
    assert(b[get_coord("f1")] == empty_square);
    assert(b[get_coord("e2")] == empty_square);
    assert(b[get_coord("d3")] == empty_square);
    assert(b[get_coord("c4")] == white_bishop);
    assert(reversible_moves == 2);

    assert(MakeMove("e7e6"));
    assert(b[get_coord("e7")] == empty_square);
    assert(b[get_coord("e6")] == black_pawn);
    assert(reversible_moves == 0);

    assert(MakeMove("d2d4"));
    assert(MakeMove("f8e7"));
    assert(MakeMove("e4e5"));
    assert(MakeMove("d7d5"));
    assert(MakeMove("e5d6"));
    assert(b[get_coord("e5")] == empty_square);
    assert(b[get_coord("d6")] == white_pawn);
    assert(b[get_coord("d5")] == empty_square);
    assert(ply == 9);
    assert(reversible_moves == 0);
    assert(quantity[white][pawn] == 8);
    assert(quantity[black][pawn] == 7);
    assert(material[white] == 48);
    assert(material[black] == 47);
    assert(pieces[white] == 16);
    assert(pieces[black] == 15);
    assert(find_piece(black, get_coord("d5")) == coords[black].end());

    assert(SetupPosition("r3k2r/ppp2ppp/8/8/8/8/PPP2PPP/R3K2R w KQkq - 0 1"));
    assert(MakeMove("e1g1"));
    assert(b[get_coord("e1")] == empty_square);
    assert(b[get_coord("g1")] == white_king);
    assert(b[get_coord("h1")] == empty_square);
    assert(b[get_coord("f1")] == white_rook);
    assert(reversible_moves == 0);

    assert(MakeMove("e8c8"));
    assert(b[get_coord("e8")] == empty_square);
    assert(b[get_coord("c8")] == black_king);
    assert(b[get_coord("a8")] == empty_square);
    assert(b[get_coord("d8")] == black_rook);
    assert(reversible_moves == 0);

    assert(SetupPosition("r3k2r/ppp2ppp/8/8/8/8/PPP2PPP/R3K2R w KQkq - 0 1"));
    assert(MakeMove("e1c1"));
    assert(b[get_coord("e1")] == empty_square);
    assert(b[get_coord("c1")] == white_king);
    assert(b[get_coord("a1")] == empty_square);
    assert(b[get_coord("d1")] == white_rook);
    assert(reversible_moves == 0);

    assert(MakeMove("e8g8"));
    assert(b[get_coord("e8")] == empty_square);
    assert(b[get_coord("g8")] == black_king);
    assert(b[get_coord("h8")] == empty_square);
    assert(b[get_coord("f8")] == black_rook);
    assert(reversible_moves == 0);

    assert(SetupPosition("2n1r3/3P4/7k/8/8/8/8/K7 w - - 4 1"));
    assert(MakeMove("d7d8q"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("d8")] == white_queen);
    assert(reversible_moves == 0);

    assert(TakebackMove());
    assert(b[get_coord("d7")] == white_pawn);
    assert(b[get_coord("d8")] == empty_square);
    assert(reversible_moves == 4);

    assert(!TakebackMove());

    assert(MakeMove("d7d8"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("d8")] == white_queen);
    assert(reversible_moves == 0);

    assert(TakebackMove());
    assert(MakeMove("d7e8b"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("e8")] == white_bishop);
    assert(reversible_moves == 0);

    assert(SetupPosition("k7/8/8/8/8/7K/3p4/2N1R3 b - - 0 1"));
    assert(MakeMove("d2c1r"));
    assert(b[get_coord("d2")] == empty_square);
    assert(b[get_coord("c1")] == black_rook);
    assert(reversible_moves == 0);

    assert(TakebackMove());
    assert(MakeMove("d2c1n"));
    assert(b[get_coord("d2")] == empty_square);
    assert(b[get_coord("c1")] == black_knight);
    assert(reversible_moves == 0);

    assert(SetupPosition(start_position));
    test_attack_tables(18, 18, 24, 24);

    assert(SetupPosition("4k3/8/5n2/5n2/8/8/8/3RK3 w - - 0 1"));
    test_attack_tables(15, 19, 16, 21);

    assert(SetupPosition(
               "2r2rk1/p4q2/1p2b3/1n6/1N6/1P2B3/P4Q2/2R2RK1 w - - 0 1"));
    test_attack_tables(37, 37, 55, 55);

    assert(SetupPosition(
               "2k1r2r/1pp3pp/p2b4/2p1n2q/6b1/1NQ1B3/PPP2PPP/R3RNK1"
               " b - - 0 1"));
    test_attack_tables(28, 31, 48, 51);
}

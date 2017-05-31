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
       slider{false, false, true, true, true, false, false},
       pc_streng{0, 0, 12, 6, 4, 4, 1},
       streng{0, 15000, 120, 60, 40, 40, 10},
       sort_streng{0, 15000, 120, 60, 41, 39, 10}
{
    InitChess();
}




//--------------------------------
void k2chess::InitChess()
{
    cv = cur_moves;
    memset(b_state, 0, sizeof(b_state));
    state = &b_state[prev_states];

    InitBrd();
}





//--------------------------------
void k2chess::InitBrd()
{
    piece_t pcs[] =
    {
        white_knight, white_knight, white_bishop, white_bishop,
        white_rook, white_rook, white_queen, white_king
    };
    coord_t crd[] = {6, 1, 5, 2, 7, 0, 3, 4};

    memset(b, 0, sizeof(b));

    coords[white].clear();
    coords[black].clear();

    for(auto i = 0; i < 8; i++)
    {
        b[get_coord(i, 1)] = white_pawn;
        coords[white].push_front(get_coord(7 - i, 1));
        b[get_coord(i, 6)] = black_pawn;
        coords[black].push_front(get_coord(7 - i, 6));
        b[get_coord(crd[i], 0)] = pcs[i];
        coords[white].push_back(get_coord(crd[i], 0));
        b[get_coord(crd[i], 7)]= pcs[i] ^ white;
        coords[black].push_back(get_coord(crd[i], 7));
    }

    InitAttacks();

    state[0].captured_piece = 0;
    state[0].cstl = 0x0F;
    state[0].ep = 0;
    wtm = white;
    ply = 0;
    cv[0] = '\0';
    pieces[black] = 16;
    pieces[white] = 16;
    material[black] = 48;
    material[white] = 48;

    reversible_moves = 0;

    king_coord[white] = --coords[white].end();
    king_coord[black] = --coords[black].end();

    for(auto clr = black; clr <= white; clr++)
    {
        quantity[clr][get_index(black_king)] = 1;
        quantity[clr][get_index(black_queen)] = 1;
        quantity[clr][get_index(black_rook)] = 2;
        quantity[clr][get_index(black_bishop)] = 2;
        quantity[clr][get_index(black_knight)] = 2;
        quantity[clr][get_index(black_pawn)] = 8;
    }

}





//--------------------------------
void k2chess::InitAttacks()
{
    memset(attacks, 0, sizeof(attacks));
    for(auto stm = black; stm <= white; ++stm)
        for(auto it = coords[stm].rbegin(); it != coords[stm].rend(); ++it)
            InitAttacksOnePiece(get_col(*it), get_row(*it));
}





//--------------------------------
void k2chess::InitAttacksOnePiece(const shifts_t col, const shifts_t row)
{
    const auto coord = get_coord(col, row);
    const auto type = get_index(b[coord]);
    if(type == get_index(black_pawn))
    {
        return;
    }
    const auto max_len = slider[type] ?
               std::max((size_t) board_height, (size_t) board_width) : 1;
    const auto color = get_piece_color(b[coord]);
    for(auto ray = 0; ray < rays[type]; ray++)
    {
        auto col_ = col;
        auto row_ = row;
        for(size_t i = 0; i < max_len; ++i)
        {
            col_ += delta_col[type][ray];
            row_ += delta_row[type][ray];
            if(!col_within(col_) || !row_within(row_))
                break;
            attacks[color][get_coord(col_, row_)] |= (1 << type);
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
size_t k2chess::test_count_attacked_squares(side_to_move_t stm)
{
    size_t ans = 0;
    for(auto it : attacks[stm])
        if(it != 0)
            ans++;
    return ans;
}





//--------------------------------
size_t k2chess::test_count_all_attacks(side_to_move_t stm)
{
    size_t ans = 0;
    for(auto it : attacks[stm])
        ans += std::bitset<32>(it).count();
    return ans;
}





//--------------------------------
bool k2chess::BoardToMen()
{
    coords[black].clear();
    coords[white].clear();
    for(size_t i = 0; i < sizeof(b)/sizeof(*b); i++)
    {
        if(b[i] == empty_square)
            continue;
        coords[b[i] & white].push_back(i);
        quantity[b[i] & white][get_index(to_black(b[i]))]++;
    }

//    coords[black].sort(PieceListCompare);
//    coords[white].sort(PieceListCompare);
    for(auto color = black; color <= white; ++color)
    {
        bool replaced = true;
        while(replaced)
        {
            replaced = false;
            for(auto cur_iter = coords[color].begin();
                    cur_iter != --(coords[color].end());
                    ++cur_iter)
            {
                auto nxt_iter = cur_iter;
                ++nxt_iter;
                auto cur_coord = *cur_iter;
                auto nxt_coord = *nxt_iter;
                auto cur_piece = b[cur_coord];
                auto nxt_piece = b[nxt_coord];
                auto cur_streng = sort_streng[get_index(cur_piece)];
                auto nxt_streng = sort_streng[get_index(nxt_piece)];
                if(cur_streng > nxt_streng)
                {
                    coords[color].move_element(cur_iter, nxt_iter);
                    cur_iter = nxt_iter;
                    replaced = true;
                }
            }
        }
#ifndef NDEBUG
        for(auto it = coords[color].begin();
                it != --(coords[color].end());
                ++it)
        {
            auto nx = it;
            ++nx;
            assert(sort_streng[get_index(b[*nx])] >=
                   sort_streng[get_index(b[*it])]);
        }
#endif //NDEBUG
    }
    return true;
}





//--------------------------------
bool k2chess::FenToBoard(char *ptr)
{
    char chars[] = "kqrbnpKQRBNP";
    piece_t pcs[] =
    {
        black_king, black_queen, black_rook, black_bishop,
        black_knight, black_pawn, white_king, white_queen, white_rook,
        white_bishop, white_knight, white_pawn
    };
    material[black] = 0;
    material[white] = 0;
    pieces[black] = 0;
    pieces[white] = 0;
    reversible_moves = 0;
    memset(quantity, 0, sizeof(quantity));

    for(auto row = board_height - 1; row >= 0; row--)
    {
        for(auto col = 0; col < board_width; col++)
        {
            auto chr = *ptr - '0';
            if(chr >= 1 && chr <= 8)
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
                    return false;
                if(to_black(pcs[index]) == black_pawn
                        && (row == 0 || row == 7))
                    return false;
                auto piece = pcs[index];
                b[get_coord(col, row)] = piece;
                auto color = get_piece_color(piece);
                material[color] += pc_streng[get_index(piece)];
                pieces[color]++;
            }
            ptr++;
        }
    }
    BoardToMen();

    state[0].ep = 0;
    ptr++;
    if(*ptr == 'b')
        wtm = black;
    else if(*ptr == 'w')
        wtm = white;
    else
        return false;

    castle_t cstl = 0;
    ptr += 2;
    while(*ptr != ' ')
        switch(*ptr)
        {
        case 'K' :
            cstl |= 0x01;
            ptr++;
            break;
        case 'Q' :
            cstl |= 0x02;
            ptr++;
            break;
        case 'k' :
            cstl |= 0x04;
            ptr++;
            break;
        case 'q' :
            cstl |= 0x08;
            ptr++;
            break;
        case '-' :
            ptr++;
            break;
        case ' ' :
            break;
        default :
            return false;
        }

    if((cstl & 0x01)
            && (b[get_coord(4, 0)] != white_king
                || b[get_coord(7, 0 )] != white_rook))
        cstl &= ~0x01;
    if((cstl & 0x02)
            && (b[get_coord(4, 0)] != white_king
                || b[get_coord(0, 0 )] != white_rook))
        cstl &= ~0x02;
    if((cstl & 0x04)
            && (b[get_coord(4, 7)] != black_king
                || b[get_coord(7, 7 )] != black_rook))
        cstl &= ~0x04;
    if((cstl & 0x08)
            && (b[get_coord(4, 7)] != black_king
                || b[get_coord(0, 7 )] != black_rook))
        cstl &= ~0x08;

    state[0].cstl = cstl;

    ptr++;
    if(*ptr != '-')
    {
        int col = *(ptr++) - 'a';
        int row = *(ptr++) - '1';
        int s = wtm ? -1 : 1;
        piece_t pawn = wtm ? white_pawn : black_pawn;
        if(b[get_coord(col-1, row+s)] == pawn
                || b[get_coord(col+1, row+s)] == pawn)
            state[0].ep = col + 1;
    }
    if(*(++ptr) && *(++ptr))
        while(*ptr >= '0' && *ptr <= '9')
        {
            reversible_moves *= 10;
            reversible_moves += (*ptr++ - '0');
        }

    king_coord[white] = --coords[white].end();
    king_coord[black] = --coords[black].end();

    InitAttacks();
    return true;
}





//--------------------------------
void k2chess::ShowMove(coord_t from_coord, coord_t to_coord)
{
    char *cur = cur_moves + 5*(ply - 1);
    *(cur++) = get_col(from_coord) + 'a';
    *(cur++) = get_row(from_coord) + '1';
    *(cur++) = get_col(to_coord) + 'a';
    *(cur++) = get_row(to_coord) + '1';
    *(cur++) = ' ';
    *cur = '\0';
}





//--------------------------------
bool k2chess::MakeCastle(move_c m, coord_t from_coord)
{
    auto cs = state[ply].cstl;
    if(m.piece_iterator == king_coord[wtm])  // king moves
        state[ply].cstl &= wtm ? 0xFC : 0xF3;
    if(b[from_coord] == (black_rook ^ wtm))  // rook moves
    {
        if((wtm && from_coord == 0x07) || (!wtm && from_coord == 0x77))
            state[ply].cstl &= wtm ? 0xFE : 0xFB;
        else if((wtm && from_coord == 0x00) || (!wtm && from_coord == 0x70))
            state[ply].cstl &= wtm ? 0xFD : 0xF7;
    }
    if(b[m.to_coord] == (white_rook ^ wtm))  // rook is taken
    {
        if((wtm && m.to_coord == 0x77) || (!wtm && m.to_coord == 0x07))
            state[ply].cstl &= wtm ? 0xFB : 0xFE;
        else if((wtm && m.to_coord == 0x70) || (!wtm && m.to_coord == 0x00))
            state[ply].cstl &= wtm ? 0xF7 : 0xFD;
    }
    bool castleRightsChanged = (cs != state[ply].cstl);
    if(castleRightsChanged)
        reversible_moves = 0;

    if(!(m.flag & is_castle))
        return castleRightsChanged;

    coord_t rFr, rTo;
    if(m.flag == is_castle_kingside)
    {
        rFr = 0x07;
        rTo = 0x05;
    }
    else
    {
        rFr = 0x00;
        rTo = 0x03;
    }
    if(!wtm)
    {
        rFr += 0x70;
        rTo += 0x70;
    }
    auto it = coords[wtm].begin();
    for(; it != coords[wtm].end(); ++it)
        if(*it == rFr)
            break;

    state[ply].castled_rook_it = it;
    b[rTo] = b[rFr];
    b[rFr] = empty_square;
    *it = rTo;

    reversible_moves = 0;

    return false;
}





//--------------------------------
void k2chess::UnMakeCastle(move_c m)
{
    auto rMen = coords[wtm].begin();
    rMen = state[ply].castled_rook_it;
    auto rFr =*rMen;
    auto rTo = rFr + (m.flag == is_castle_kingside ? 2 : -3);
    b[rTo] = b[rFr];
    b[rFr] = empty_square;
    *rMen = rTo;
}





//--------------------------------
bool k2chess::MakeEP(move_c m, coord_t from_coord)
{
    int delta = m.to_coord - from_coord;
    int to_coord = m.to_coord;
    if(std::abs(delta) == 0x20
            && (b[m.to_coord + 1] == (white_pawn ^ wtm)
                || b[to_coord - 1] == (white_pawn ^ wtm)))
    {
        state[ply].ep = (to_coord & 0x07) + 1;
        return true;
    }
    if(m.flag & is_en_passant)
        b[to_coord + (wtm ? -16 : 16)] = empty_square;
    return false;
}





//-----------------------------
bool k2chess::PieceListCompare(coord_t men1, coord_t men2)
{
    return sort_streng[get_index(b[men1])] > sort_streng[get_index(b[men2])];
}





//--------------------------------
void k2chess::StoreCurrentBoardState(move_c m, coord_t from_coord,
                                     coord_t targ)
{
    state[ply].cstl = state[ply - 1].cstl;
    state[ply].captured_piece = b[m.to_coord];

    state[ply].from_coord = from_coord;
    state[ply].to_coord = targ;
    state[ply].reversibleCr = reversible_moves;
    state[ply].priority = m.priority;
}





//--------------------------------
void k2chess::MakeCapture(move_c m, coord_t targ)
{
    if (m.flag & is_en_passant)
    {
        targ += wtm ? -16 : 16;
        material[!wtm]--;
        quantity[!wtm][get_index(black_pawn)]--;
    }
    else
    {
        material[!wtm] -=
            pc_streng[get_index(state[ply].captured_piece)];
        quantity[!wtm][get_index(state[ply].captured_piece)]--;
    }

    auto it_cap = coords[!wtm].begin();
    auto it_end = coords[!wtm].end();
    for(; it_cap != it_end; ++it_cap)
        if(*it_cap == targ)
            break;
    assert(it_cap != it_end);
    state[ply].captured_it = it_cap;
    coords[!wtm].erase(it_cap);
    pieces[!wtm]--;
    reversible_moves = 0;
}





//--------------------------------
void k2chess::MakePromotion(move_c m, iterator it)
{
    auto prIx = m.flag & is_promotion;
    piece_t prPc[] = {0, black_queen, black_knight, black_rook, black_bishop};
    if(prIx)
    {
        b[m.to_coord] = prPc[prIx] ^ wtm;
        material[wtm] += pc_streng[get_index(prPc[prIx])]
                         - pc_streng[get_index(black_pawn)];
        quantity[wtm][get_index(black_pawn)]--;
        quantity[wtm][get_index(prPc[prIx])]++;
        state[ply].nprom = ++it;
        --it;
        coords[wtm].move_element(king_coord[wtm], it);
        reversible_moves = 0;
    }
}





//--------------------------------
bool k2chess::MkMoveFast(move_c m)
{
    bool is_special_move = false;
    ply++;
    auto it = coords[wtm].begin();
    it = m.piece_iterator;
    auto from_coord = *it;
    auto targ = m.to_coord;
    StoreCurrentBoardState(m, from_coord, targ);
    reversible_moves++;

    if(m.flag & is_capture)
        MakeCapture(m, targ);

    // trick: fast exit if not K|Q|R moves, and no captures
    if((b[from_coord] <= white_rook) || (m.flag & is_capture))
        is_special_move |= MakeCastle(m, from_coord);

    state[ply].ep = 0;
    if((b[from_coord] ^ wtm) == black_pawn)
    {
        is_special_move |= MakeEP(m, from_coord);
        reversible_moves = 0;
    }
#ifndef NDEBUG
    ShowMove(from_coord, m.to_coord);
#endif // NDEBUG

    b[m.to_coord] = b[from_coord];
    b[from_coord] = empty_square;

    if(m.flag & is_promotion)
        MakePromotion(m, it);

    *it = m.to_coord;
    wtm ^= white;

    return is_special_move;
}





//--------------------------------
void k2chess::UnmakeCapture(move_c m)
{
    auto it_cap = coords[!wtm].begin();
    it_cap = state[ply].captured_it;

    if(m.flag & is_en_passant)
    {
        material[!wtm]++;
        quantity[!wtm][get_index(black_pawn)]++;
        if(wtm)
        {
            b[m.to_coord - 16] = black_pawn;
            *it_cap = m.to_coord - 16;
        }
        else
        {
            b[m.to_coord + 16] = white_pawn;
            *it_cap = m.to_coord + 16;
        }
    }
    else
    {
        material[!wtm]
        += pc_streng[get_index(state[ply].captured_piece)];
        quantity[!wtm][get_index(state[ply].captured_piece)]++;
    }

    coords[!wtm].restore(it_cap);
    pieces[!wtm]++;
}





//--------------------------------
void k2chess::UnmakePromotion(move_c m)
{
    piece_t prPc[] = {0, black_queen, black_knight, black_rook, black_bishop};

    move_flag_t prIx = m.flag & is_promotion;

    auto it_prom = coords[wtm].begin();
    it_prom = state[ply].nprom;
    auto before_king = king_coord[wtm];
    --before_king;
    coords[wtm].move_element(it_prom, before_king);
    material[wtm] -= pc_streng[get_index(prPc[prIx])]
                     - pc_streng[get_index(black_pawn)];
    quantity[wtm][get_index(black_pawn)]++;
    quantity[wtm][get_index(prPc[prIx])]--;
}





//--------------------------------
void k2chess::UnMoveFast(move_c m)
{
    auto from_coord = state[ply].from_coord;
    auto it = coords[!wtm].begin();
    it = m.piece_iterator;
    *it = from_coord;
    b[from_coord] = (m.flag & is_promotion) ? (white_pawn ^ wtm)
                    : b[m.to_coord];
    b[m.to_coord] = state[ply].captured_piece;

    reversible_moves = state[ply].reversibleCr;

    wtm ^= white;

    if(m.flag & is_capture)
        UnmakeCapture(m);

    if(m.flag & is_promotion)
        UnmakePromotion(m);
    else if(m.flag & is_castle)
        UnMakeCastle(m);

    ply--;

#ifndef NDEBUG
    cur_moves[5*ply] = '\0';
#endif // NDEBUG
}

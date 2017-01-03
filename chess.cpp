#include "chess.h"





//--------------------------------
k2chess::k2chess() :    rays{0, 8, 8, 4, 4, 8, 0},
                        shifts{
                                { 0,  0,  0,  0,  0,  0,  0,  0},
                                { 1, 17, 16, 15, -1,-17,-16,-15},
                                { 1, 17, 16, 15, -1,-17,-16,-15},
                                { 1, 16, -1,-16, 0,  0,  0,  0},
                                {17, 15,-17,-15, 0,  0,  0,  0},
                                {18, 33, 31, 14,-18,-33,-31,-14}},
                        pc_streng{0, 0, 12, 6, 4, 4, 1},
                        streng{0, 15000, 120, 60, 40, 40, 10},
                        sort_streng{0, 15000, 120, 60, 41, 39, 10},
                        slider{false, false, true, true, true, false, false}

{
    InitChess();
}




//--------------------------------
void k2chess::InitChess()
{
    cv = cur_moves;
    InitBrd();
}





//--------------------------------
void k2chess::InitBrd()
{
    piece_t pcs[] = {white_knight, white_knight, white_bishop, white_bishop, white_rook, white_rook, white_queen, white_king};
    coord_t crd[] = {6, 1, 5, 2, 7, 0, 3, 4};

    memset(b, 0, sizeof(b));

    coords[white].clear();
    coords[black].clear();

    InitAttacks();

    for(auto i = 0; i < 8; i++)
    {
        b[get_coord(i, 1)]      = white_pawn;
        coords[white].push_front(get_coord(7 - i, 1));
        b[get_coord(i, 6)]     = black_pawn;
        coords[black].push_front(get_coord(7 - i, 6));
        b[get_coord(crd[i], 0)]      = pcs[i];
        coords[white].push_back(get_coord(crd[i], 0));
        b[get_coord(crd[i], 7)]      = pcs[i] ^ white;
        coords[black].push_back(get_coord(crd[i], 7));
    }

    b_state[prev_states + 0].capt    = 0;
    b_state[prev_states + 0].cstl    = 0x0F;
    b_state[prev_states + 0].ep      = 0;
    wtm             = white;
    ply             = 0;
    cv[0]           = '\0';
    pieces[black]   = 16;
    pieces[white]   = 16;
    material[black] = 48;
    material[white] = 48;

    reversible_moves     = 0;

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
    for(auto i = 1; i < 6; i++)
    {
        if(!slider[i])
            for(auto j = 0; j < rays[i]; j++)
                attacks[120 + shifts[i][j]] |= (1 << i);
        else
        {
            for(auto j = 0; j < rays[i]; j++)
                for(auto k = 1; k < 8; k++)
                {
                    auto to = 120 + k*shifts[i][j];
                    if(to >= 0 && to < 240)
                    {
                        attacks[to] |= (1 << i);
                        get_shift[to] = shifts[i][j];
                        get_delta[to] = k;
                    }
                }
        }
    }
}





//--------------------------------
bool k2chess::BoardToMen()
{
    coords[black].clear();
    coords[white].clear();
    for(size_t i = 0; i < sizeof(b)/sizeof(*b); i++)
    {
        if(!within_board(i) || b[i] == empty_square)
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
bool k2chess::FenToBoard(char *p)
{
    char chars[] = "kqrbnpKQRBNP";
    piece_t pcs[]  = { black_king, black_queen, black_rook, black_bishop, black_knight, black_pawn, white_king, white_queen, white_rook, white_bishop, white_knight, white_pawn};
    streng_t mtr[]   = {0, 12, 6, 4, 4, 1, 0, 12, 6, 4, 4, 1};
    material[black] = 0;
    material[white] = 0;
    pieces[black]   = 0;
    pieces[white]   = 0;
    reversible_moves = 0;
    memset(quantity, 0, sizeof(quantity));

    for(auto row = 7; row >= 0; row--)
        for(auto col = 0; col <= 7; col++)
        {
            int to = get_coord(col, row);
            int ip = *p - '0';
            if(ip >= 1 && ip <= 8)
            {
                for(auto j = 0; j < ip; ++j)
                {
                    assert(within_board(to + j));
                    b[to + j] = 0;
                }
                col += *p++ - '1';
            }
            else if(*p == '/')
            {
                p++;
                col = -1;						// break ?
                continue;						//
            }
            else
            {
                size_t i = 0;
                for(; i < 12; ++i)
                    if(*p == chars[i])
                        break;
                if(i >= 12)
                    return false;
                if(to_black(pcs[i]) == black_pawn && (row == 0 || row == 7))
                    return false;
                b[get_coord(col, row)] = pcs[i];
                material[i >= 6]  += mtr[i];
                pieces[i >= 6]++;
                p++;
            }
        }// for( col


    BoardToMen();

    b_state[prev_states + 0].ep   = 0;
    wtm = (*(++p) == 'b') ? black : white;

    castle_t cstl = 0;
    p += 2;
    while(*p != ' ')
        switch(*p)
        {
            case 'K' : cstl |= 0x01; p++; break;
            case 'Q' : cstl |= 0x02; p++; break;
            case 'k' : cstl |= 0x04; p++; break;
            case 'q' : cstl |= 0x08; p++; break;
            case '-' : p++; break;
            case ' ' : break;
            default : return false;
        }

    if((cstl & 0x01)
    && (b[get_coord(4, 0)] != white_king || b[get_coord(7, 0 )] != white_rook))
        cstl &= ~0x01;
    if((cstl & 0x02)
    && (b[get_coord(4, 0)] != white_king || b[get_coord(0, 0 )] != white_rook))
        cstl &= ~0x02;
    if((cstl & 0x04)
    && (b[get_coord(4, 7)] != black_king || b[get_coord(7, 7 )] != black_rook))
        cstl &= ~0x04;
    if((cstl & 0x08)
    && (b[get_coord(4, 7)] != black_king || b[get_coord(0, 7 )] != black_rook))
        cstl &= ~0x08;

    b_state[prev_states + 0].cstl = cstl;

    p++;
    if(*p != '-')
    {
        int col = *(p++) - 'a';
        int row = *(p++) - '1';
        int s = wtm ? -1 : 1;
        piece_t pawn = wtm ? white_pawn : black_pawn;
        if(b[get_coord(col-1, row+s)] == pawn || b[get_coord(col+1, row+s)] == pawn)
            b_state[prev_states + 0].ep = col + 1;
    }
    if(*(++p) && *(++p))
        while(*p >= '0' && *p <= '9')
        {
            reversible_moves *= 10;
            reversible_moves += (*p++ - '0');
        }

    king_coord[white] = --coords[white].end();
    king_coord[black] = --coords[black].end();

    return true;
}





//--------------------------------
void k2chess::ShowMove(coord_t fr, coord_t to)
{
    char *cur = cur_moves + 5*(ply - 1);
    *(cur++) = get_col(fr) + 'a';
    *(cur++) = get_row(fr) + '1';
    *(cur++) = get_col(to) + 'a';
    *(cur++) = get_row(to) + '1';
    *(cur++) = ' ';
    *cur     = '\0';
}





//--------------------------------
bool k2chess::MakeCastle(move_c m, coord_t fr)
{
    auto cs = b_state[prev_states + ply].cstl;
    if(m.pc == king_coord[wtm])                     // K moves
        b_state[prev_states + ply].cstl &= wtm ? 0xFC : 0xF3;
    if(king_coord[wtm] != --coords[wtm].end())
        ply = ply;
    else if(b[fr] == (black_rook ^ wtm))        // R moves
    {
        if((wtm && fr == 0x07) || (!wtm && fr == 0x77))
            b_state[prev_states + ply].cstl &= wtm ? 0xFE : 0xFB;
        else if((wtm && fr == 0x00) || (!wtm && fr == 0x70))
            b_state[prev_states + ply].cstl &= wtm ? 0xFD : 0xF7;
    }
    if(b[m.to] == (white_rook ^ wtm))           // R is taken
    {
        if((wtm && m.to == 0x77) || (!wtm && m.to == 0x07))
            b_state[prev_states + ply].cstl &= wtm ? 0xFB : 0xFE;
        else if((wtm && m.to == 0x70) || (!wtm && m.to == 0x00))
            b_state[prev_states + ply].cstl &= wtm ? 0xF7 : 0xFD;
    }
    bool castleRightsChanged = (cs != b_state[prev_states + ply].cstl);
    if(castleRightsChanged)
        reversible_moves = 0;

    if(!(m.flg & is_castle))
        return castleRightsChanged;

    coord_t rFr, rTo;
    if(m.flg == is_castle_kingside)
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

    b_state[prev_states + ply].castled_rook_it = it;
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
    rMen = b_state[prev_states + ply].castled_rook_it;
    auto rFr =*rMen;
    auto rTo = rFr + (m.flg == is_castle_kingside ? 2 : -3);
    b[rTo] = b[rFr];
    b[rFr] = empty_square;
    *rMen = rTo;
}





//--------------------------------
bool k2chess::MakeEP(move_c m, coord_t fr)
{
    int delta = m.to - fr;
    int to = m.to;
    if(std::abs(delta) == 0x20
    && (b[m.to + 1] == (white_pawn ^ wtm) || b[to - 1] == (white_pawn ^ wtm)))
    {
        b_state[prev_states + ply].ep = (to & 0x07) + 1;
        return true;
    }
    if(m.flg & is_en_passant)
        b[to + (wtm ? -16 : 16)] = empty_square;
    return false;
}





//--------------------------------
bool k2chess::SliderAttack(coord_t fr, coord_t to)
{
    auto shift = get_shift[120 + to - fr];
    auto delta = get_delta[120 + to - fr];
    for(auto i = 0; i < delta - 1; i++)
    {
        fr += shift;
        if(b[fr])
            return false;
    }
    return true;
}





//--------------------------------
bool k2chess::Attack(coord_t to, side_to_move_t xtm)
{
    if((!xtm && (b[to+15] == black_pawn || b[to+17] == black_pawn))
    ||  (xtm && ((to >= 15 && b[to-15] == white_pawn)
    || (to >= 17 && b[to-17] == white_pawn))))

        return true;

    auto it = king_coord[xtm];
    do
    {
        auto fr = *it;
        auto pt = get_index(b[fr]);
        if(pt == get_index(black_pawn))
            break;

        auto att = attacks[120 + to - fr] & (1 << pt);
        if(!att)
            continue;
        if(!slider[pt])
            return true;
        if(SliderAttack(to, fr))
             return true;
    } while(it-- != coords[xtm].begin());

    return false;
}





//--------------------------------
bool k2chess::LegalSlider(coord_t fr, coord_t to, piece_t pt)
{
    assert(120 + to - fr >= 0);
    assert(120 + to - fr < 240);
    auto shift = get_shift[120 + to - fr];
    for(auto i = 0; i < 7; i++)
    {
        to -= shift;
        if(!within_board(to))
            return true;
        if(b[to])
            break;
    }
    if(b[to] == (black_queen ^ wtm)
    || (pt == black_bishop && b[to] == (black_bishop ^ wtm))
    || (pt == black_rook && b[to] == (black_rook ^ wtm)))
        return false;

    return true;
}





//--------------------------------
bool k2chess::Legal(move_c m, bool ic)
{
    if(ic || to_black(b[m.to]) == black_king)
        return !Attack(*king_coord[!wtm], wtm);
    auto fr = b_state[prev_states + ply].fr;
    auto to = *king_coord[!wtm];
    assert(120 + to - fr >= 0);
    assert(120 + to - fr < 240);
    if(attacks[120 + to - fr] & (1 << get_index(black_rook)))
        return LegalSlider(fr, to, black_rook);
    if(attacks[120 + to - fr] & (1 << get_index(black_bishop)))
        return LegalSlider(fr, to, black_bishop);

    return true;
}





//-----------------------------
bool k2chess::PieceListCompare(coord_t men1, coord_t men2)
{
    return sort_streng[get_index(b[men1])] > sort_streng[get_index(b[men2])];
}





//--------------------------------
void k2chess::StoreCurrentBoardState(move_c m, coord_t fr, coord_t targ)
{
    b_state[prev_states + ply].cstl  = b_state[prev_states + ply - 1].cstl;
    b_state[prev_states + ply].capt  = b[m.to];

    b_state[prev_states + ply].fr = fr;
    b_state[prev_states + ply].to = targ;
    b_state[prev_states + ply].reversibleCr = reversible_moves;
    b_state[prev_states + ply].scr = m.scr;
    reversible_moves++;
}





//--------------------------------
void k2chess::MakeCapture(move_c m, coord_t targ)
{
    if (m.flg & is_en_passant)
    {
        targ += wtm ? -16 : 16;
        material[!wtm]--;
        quantity[!wtm][get_index(black_pawn)]--;
    }
    else
    {
        material[!wtm] -=
                pc_streng[get_index(b_state[prev_states + ply].capt)];
        quantity[!wtm][get_index(b_state[prev_states + ply].capt)]--;
    }

    auto it_cap = coords[!wtm].begin();
    auto it_end = coords[!wtm].end();
    for(; it_cap != it_end; ++it_cap)
        if(*it_cap == targ)
            break;
    assert(it_cap != it_end);
    b_state[prev_states + ply].captured_it = it_cap;
    coords[!wtm].erase(it_cap);
    pieces[!wtm]--;
    reversible_moves = 0;
}





//--------------------------------
void k2chess::MakePromotion(move_c m, iterator it)
{
    auto prIx = m.flg & is_promotion;
    piece_t prPc[] = {0, black_queen, black_knight, black_rook, black_bishop};
    if(prIx)
    {
        b[m.to] = prPc[prIx] ^ wtm;
        material[wtm] += pc_streng[get_index(prPc[prIx])]
                - pc_streng[get_index(black_pawn)];
        quantity[wtm][get_index(black_pawn)]--;
        quantity[wtm][get_index(prPc[prIx])]++;
        b_state[prev_states + ply].nprom = ++it;
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
    it      = m.pc;
    auto fr   = *it;
    auto targ = m.to;
    StoreCurrentBoardState(m, fr, targ);

    if(m.flg & is_capture)
        MakeCapture(m, targ);

    if((b[fr] <= white_rook) || (m.flg & is_capture))        // trick: fast exit if not K|Q|R moves, and no captures
        is_special_move |= MakeCastle(m, fr);

    b_state[prev_states + ply].ep = 0;
    if((b[fr] ^ wtm) == black_pawn)
    {
        is_special_move     |= MakeEP(m, fr);
        reversible_moves = 0;
    }
#ifndef NDEBUG
    ShowMove(fr, m.to);
#endif // NDEBUG

    b[m.to]     = b[fr];
    b[fr]       = empty_square;

    if(m.flg & is_promotion)
        MakePromotion(m, it);

    *it   = m.to;
    wtm ^= white;

    return is_special_move;
}





//--------------------------------
void k2chess::UnmakeCapture(move_c m)
{
    auto it_cap = coords[!wtm].begin();
    it_cap = b_state[prev_states + ply].captured_it;

    if(m.flg & is_en_passant)
    {
        material[!wtm]++;
        quantity[!wtm][get_index(black_pawn)]++;
        if(wtm)
        {
            b[m.to - 16] = black_pawn;
            *it_cap = m.to - 16;
        }
        else
        {
            b[m.to + 16] = white_pawn;
            *it_cap = m.to + 16;
        }
    }// if en_pass
    else
    {
        material[!wtm]
            += pc_streng[get_index(b_state[prev_states + ply].capt)];
        quantity[!wtm][get_index(b_state[prev_states + ply].capt)]++;
    }

    coords[!wtm].restore(it_cap);
    pieces[!wtm]++;
}





//--------------------------------
void k2chess::UnmakePromotion(move_c m)
{
    piece_t prPc[] = {0, black_queen, black_knight, black_rook, black_bishop};

    move_flag_t prIx = m.flg & is_promotion;

    auto it_prom = coords[wtm].begin();
    it_prom = b_state[prev_states + ply].nprom;
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
    auto fr = b_state[prev_states + ply].fr;
    auto it = coords[!wtm].begin();
    it = m.pc;
    *it = fr;
    b[fr] = (m.flg & is_promotion) ? white_pawn ^ wtm : b[m.to];
    b[m.to] = b_state[prev_states + ply].capt;

    reversible_moves = b_state[prev_states + ply].reversibleCr;

    wtm ^= white;

    if(m.flg & is_capture)
        UnmakeCapture(m);

    if(m.flg & is_promotion)
        UnmakePromotion(m);
    else if(m.flg & is_castle)
        UnMakeCastle(m);

    ply--;

#ifndef NDEBUG
    cur_moves[5*ply] = '\0';
#endif // NDEBUG
}

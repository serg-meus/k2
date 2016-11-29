#include "chess.h"





//--------------------------------
u8  b[137];                                                             // board array in "0x88" style
short_list<u8, lst_sz> coords[2];

u8  attacks[240],                                                       // table for quick detect possible attacks
    get_delta[240];                                                     // I'm already forget what's this
i8  get_shift[240];                                                     // ... and this
u8 rays[7]   = {0, 8, 8, 4, 4, 8, 0};
i8 shifts[6][8] = {{ 0,  0,  0,  0,  0,  0,  0,  0},
                    { 1, 17, 16, 15, -1,-17,-16,-15},
                    { 1, 17, 16, 15, -1,-17,-16,-15},
                    { 1, 16, -1,-16, 0,  0,  0,  0},
                    {17, 15,-17,-15, 0,  0,  0,  0},
                    {18, 33, 31, 14,-18,-33,-31,-14}};
u8  pc_streng[7]    =  {0, 0, 12, 6, 4, 4, 1};
short streng[7]     = {0, 15000, 120, 60, 40, 40, 10};
short sort_streng[7] = {0, 15000, 120, 60, 41, 39, 10};
u8  slider[7]       = {0, 0, 1, 1, 1, 0, 0};
int material[2], pieces[2];
BrdState  b_state[prev_states + max_ply];
unsigned wtm;
depth_t ply;
u64 nodes, tmpCr;
char *cv;
u16 reversible_moves;

char cur_moves[5*max_ply];

iterator king_coord[2];
u8 quantity[2][6 + 1];







//--------------------------------
void InitChess()
{
    cv = cur_moves;
    InitBrd();
}





//--------------------------------
void InitBrd()
{
    u8 pcs[] = {_N, _N, _B, _B, _R, _R, _Q, _K};
    u8 crd[] = {6, 1, 5, 2, 7, 0, 3, 4};

    memset(b, 0, sizeof(b));

    coords[white].clear();
    coords[black].clear();

    InitAttacks();

    int i;
    for(i = 0; i < 8; i++)
    {
        b[XY2SQ(i, 1)]      = _P;
        coords[white].push_front(XY2SQ(7 - i, 1));
        b[XY2SQ(i, 6)]     = _p;
        coords[black].push_front(XY2SQ(7 - i, 6));
        b[XY2SQ(crd[i], 0)]      = pcs[i];
        coords[white].push_back(XY2SQ(crd[i], 0));
        b[XY2SQ(crd[i], 7)]      = pcs[i] ^ white;
        coords[black].push_back(XY2SQ(crd[i], 7));
    }

    b_state[prev_states + 0].capt    = 0;
    b_state[prev_states + 0].cstl    = 0x0F;
    b_state[prev_states + 0].ep      = 0;
    wtm             = white;
    ply             = 0;
    nodes           = 0;
    cv[0]           = '\0';
    pieces[black]   = 16;
    pieces[white]   = 16;
    material[black] = 48;
    material[white] = 48;

    reversible_moves     = 0;

    king_coord[white] = --coords[white].end();
    king_coord[black] = --coords[black].end();

    for(u8 clr = 0; clr <= 1; clr++)
    {
        quantity[clr][GET_INDEX(_k)] = 1;
        quantity[clr][GET_INDEX(_q)] = 1;
        quantity[clr][GET_INDEX(_r)] = 2;
        quantity[clr][GET_INDEX(_b)] = 2;
        quantity[clr][GET_INDEX(_n)] = 2;
        quantity[clr][GET_INDEX(_p)] = 8;
    }

}





//--------------------------------
void InitAttacks()
{
    int i, j, k;
    for(i = 1; i < 6; i++)
    {
        if(!slider[i])
            for(j = 0; j < rays[i]; j++)
                attacks[120 + shifts[i][j]] |= (1 << i);
        else
        {
            for(j = 0; j < rays[i]; j++)
                for(k = 1; k < 8; k++)
                {
                    int to = 120 + k*shifts[i][j];
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
bool BoardToMen()
{
    unsigned i;

    coords[black].clear();
    coords[white].clear();
    for(i = 0; i < sizeof(b)/sizeof(*b); i++)
    {
        if(!ONBRD(i) || b[i] == __)
            continue;
        coords[b[i] & white].push_back(i);
        quantity[b[i] & white][GET_INDEX(TO_BLACK(b[i]))]++;
    }

    coords[black].sort(PieceListCompare);
    coords[white].sort(PieceListCompare);

    return true;
}





//--------------------------------
bool FenToBoard(char *p)
{
    char chars[] = "kqrbnpKQRBNP";
    char pcs[]  = { _k, _q, _r, _b, _n, _p, _K, _Q, _R, _B, _N, _P};
    int mtr[]   = {0, 12, 6, 4, 4, 1, 0, 12, 6, 4, 4, 1};
    int i;
    material[black] = 0;
    material[white] = 0;
    pieces[black]   = 0;
    pieces[white]   = 0;
    reversible_moves = 0;
    memset(quantity, 0, sizeof(quantity));

    for(int row = 7; row >= 0; row--)
        for(int col = 0; col <= 7; col++)
        {
            int to = XY2SQ(col, row);
            int ip = *p - '0';
            if(ip >= 1 && ip <= 8)
            {
                for(int j = 0; j < ip; ++j)
                {
                    assert(ONBRD(to + j));
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
                i = 0;
                for(; i < 12; ++i)
                    if(*p == chars[i])
                        break;
                if(i >= 12)
                    return false;
                if(TO_BLACK(pcs[i]) == _p && (row == 0 || row == 7))
                    return false;
                b[XY2SQ(col, row)] = pcs[i];
                material[i >= 6]  += mtr[i];
                pieces[i >= 6]++;
                p++;
            }
        }// for( col


    BoardToMen();

    b_state[prev_states + 0].ep   = 0;
    wtm = (*(++p) == 'b') ? black : white;

    u8 cstl = 0;
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
    && (b[XY2SQ(4, 0)] != _K || b[XY2SQ(7, 0 )] != _R))
        cstl &= ~0x01;
    if((cstl & 0x02)
    && (b[XY2SQ(4, 0)] != _K || b[XY2SQ(0, 0 )] != _R))
        cstl &= ~0x02;
    if((cstl & 0x04)
    && (b[XY2SQ(4, 7)] != _k || b[XY2SQ(7, 7 )] != _r))
        cstl &= ~0x04;
    if((cstl & 0x08)
    && (b[XY2SQ(4, 7)] != _k || b[XY2SQ(0, 7 )] != _r))
        cstl &= ~0x08;

    b_state[prev_states + 0].cstl = cstl;

    p++;
    if(*p != '-')
    {
        int col = *(p++) - 'a';
        int row = *(p++) - '1';
        int s = wtm ? -1 : 1;
        u8 pawn = wtm ? _P : _p;
        if(b[XY2SQ(col-1, row+s)] == pawn || b[XY2SQ(col+1, row+s)] == pawn)
            b_state[prev_states + 0].ep = col + 1;
    }
    if(*(++p) && *(++p))
        while(*p >= '0' && *p <= '9')
        {
            reversible_moves *= 10;
            reversible_moves += (*p++ - '0');
        }


    InitPawnStruct();

    king_coord[white] = --coords[white].end();
    king_coord[black] = --coords[black].end();

    return true;
}





//--------------------------------
void ShowMove(coord_t fr, coord_t to)
{
    char *cur = cur_moves + 5*(ply - 1);
    *(cur++) = COL(fr) + 'a';
    *(cur++) = ROW(fr) + '1';
    *(cur++) = COL(to) + 'a';
    *(cur++) = ROW(to) + '1';
    *(cur++) = ' ';
    *cur     = '\0';
}





//--------------------------------
bool MakeCastle(Move m, coord_t fr)
{
    u8 cs = b_state[prev_states + ply].cstl;
    if(m.pc == king_coord[wtm])                     // K moves
        b_state[prev_states + ply].cstl &= wtm ? 0xFC : 0xF3;
    if(king_coord[wtm] != --coords[wtm].end())
        ply = ply;
    else if(b[fr] == (_r ^ wtm))        // R moves
    {
        if((wtm && fr == 0x07) || (!wtm && fr == 0x77))
            b_state[prev_states + ply].cstl &= wtm ? 0xFE : 0xFB;
        else if((wtm && fr == 0x00) || (!wtm && fr == 0x70))
            b_state[prev_states + ply].cstl &= wtm ? 0xFD : 0xF7;
    }
    if(b[m.to] == (_R ^ wtm))           // R is taken
    {
        if((wtm && m.to == 0x77) || (!wtm && m.to == 0x07))
            b_state[prev_states + ply].cstl &= wtm ? 0xFB : 0xFE;
        else if((wtm && m.to == 0x70) || (!wtm && m.to == 0x00))
            b_state[prev_states + ply].cstl &= wtm ? 0xF7 : 0xFD;
    }
    bool castleRightsChanged = (cs != b_state[prev_states + ply].cstl);
    if(castleRightsChanged)
        reversible_moves = 0;

    if(!(m.flg & mCSTL))
        return castleRightsChanged;

    u8 rFr, rTo;
    if(m.flg == mCS_K)
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
    b[rFr] = __;
    *it = rTo;

    reversible_moves = 0;

    return false;
}





//--------------------------------
void UnMakeCastle(Move m)
{
    auto rMen = coords[wtm].begin();
    rMen = b_state[prev_states + ply].castled_rook_it;
    u8 rFr =*rMen;
    u8 rTo = rFr + (m.flg == mCS_K ? 2 : -3);
    b[rTo] = b[rFr];
    b[rFr] = __;
    *rMen = rTo;
}





//--------------------------------
bool MakeEP(Move m, coord_t fr)
{
    int delta = m.to - fr;
    u8 to = m.to;
    if(ABSI(delta) == 0x20
    && (b[m.to + 1] == (_P ^ wtm) || b[to - 1] == (_P ^ wtm)))
    {
        b_state[prev_states + ply].ep = (to & 0x07) + 1;
        return true;
    }
    if(m.flg & mENPS)
        b[to + (wtm ? -16 : 16)] = __;
    return false;
}





//--------------------------------
bool SliderAttack(coord_t fr, coord_t to)
{
    int shift = get_shift[120 + to - fr];
    int delta = get_delta[120 + to - fr];
    for(int i = 0; i < delta - 1; i++)
    {
        fr += shift;
        if(b[fr])
            return false;
    }
    return true;
}





//--------------------------------
bool Attack(coord_t to, u32 xtm)
{
    if((!xtm && (b[to+15] == _p || b[to+17] == _p))
    ||  (xtm && ((to >= 15 && b[to-15] == _P)
    || (to >= 17 && b[to-17] == _P))))

        return true;

    auto it = king_coord[xtm];
    do
    {
        u8 fr = *it;
        int pt  = GET_INDEX(b[fr]);
        if(pt   == GET_INDEX(_p))
            break;

        u8 att = attacks[120 + to - fr] & (1 << pt);
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
bool LegalSlider(coord_t fr, coord_t to, u8 pt)
{
    assert(120 + to - fr >= 0);
    assert(120 + to - fr < 240);
    int shift = get_shift[120 + to - fr];
    for(int i = 0; i < 7; i++)
    {
        to -= shift;
        if(!ONBRD(to))
            return true;
        if(b[to])
            break;
    }
    if(b[to] == (_q ^ wtm)
    || (pt == _b && b[to] == (_b ^ wtm))
    || (pt == _r && b[to] == (_r ^ wtm)))
        return false;

    return true;
}





//--------------------------------
bool Legal(Move m, bool ic)
{
    if(ic || TO_BLACK(b[m.to]) == _k)
        return !Attack(*king_coord[!wtm], wtm);
    u8 fr = b_state[prev_states + ply].fr;
    u8 to = *king_coord[!wtm];
    assert(120 + to - fr >= 0);
    assert(120 + to - fr < 240);
    if(attacks[120 + to - fr] & (1 << GET_INDEX(_r)))
        return LegalSlider(fr, to, _r);
    if(attacks[120 + to - fr] & (1 << GET_INDEX(_b)))
        return LegalSlider(fr, to, _b);

    return true;
}





//-----------------------------
bool PieceListCompare(coord_t men1, coord_t men2)
{
    return sort_streng[GET_INDEX(b[men1])] > sort_streng[GET_INDEX(b[men2])];
}





//--------------------------------
void StoreCurrentBoardState(Move m, coord_t fr, coord_t targ)
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
void MakeCapture(Move m, coord_t targ)
{
    if (m.flg & mENPS)
    {
        targ += wtm ? -16 : 16;
        material[!wtm]--;
        quantity[!wtm][GET_INDEX(_p)]--;
    }
    else
    {
        material[!wtm] -=
                pc_streng[GET_INDEX(b_state[prev_states + ply].capt)];
        quantity[!wtm][GET_INDEX(b_state[prev_states + ply].capt)]--;
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
void MakePromotion(Move m, iterator it)
{
    int prIx = m.flg & mPROM;
    u8 prPc[] = {0, _q, _n, _r, _b};
    if(prIx)
    {
        b[m.to] = prPc[prIx] ^ wtm;
        material[wtm] += pc_streng[GET_INDEX(prPc[prIx])]
                - pc_streng[GET_INDEX(_p)];
        quantity[wtm][GET_INDEX(_p)]--;
        quantity[wtm][GET_INDEX(prPc[prIx])]++;
        b_state[prev_states + ply].nprom = ++it;
        --it;
        coords[wtm].move_element(king_coord[wtm], it);
        reversible_moves = 0;
    }
}





//--------------------------------
bool MkMoveFast(Move m)
{
    bool is_special_move = false;
    ply++;
    auto it = coords[wtm].begin();
    it      = m.pc;
    u8 fr   = *it;
    u8 targ = m.to;
    StoreCurrentBoardState(m, fr, targ);

    if(m.flg & mCAPT)
        MakeCapture(m, targ);

    if((b[fr] <= _R) || (m.flg & mCAPT))        // trick: fast exit if not K|Q|R moves, and no captures
        is_special_move |= MakeCastle(m, fr);

    b_state[prev_states + ply].ep = 0;
    if((b[fr] ^ wtm) == _p)
    {
        is_special_move     |= MakeEP(m, fr);
        reversible_moves = 0;
    }
#ifndef NDEBUG
    ShowMove(fr, m.to);
#endif // NDEBUG

    b[m.to]     = b[fr];
    b[fr]       = __;

    if(m.flg & mPROM)
        MakePromotion(m, it);

    *it   = m.to;
    wtm ^= white;

    return is_special_move;
}





//--------------------------------
void UnmakeCapture(Move m)
{
    auto it_cap = coords[!wtm].begin();
    it_cap = b_state[prev_states + ply].captured_it;

    if(m.flg & mENPS)
    {
        material[!wtm]++;
        quantity[!wtm][GET_INDEX(_p)]++;
        if(wtm)
        {
            b[m.to - 16] = _p;
            *it_cap = m.to - 16;
        }
        else
        {
            b[m.to + 16] = _P;
            *it_cap = m.to + 16;
        }
    }// if en_pass
    else
    {
        material[!wtm]
            += pc_streng[GET_INDEX(b_state[prev_states + ply].capt)];
        quantity[!wtm][GET_INDEX(b_state[prev_states + ply].capt)]++;
    }

    coords[!wtm].restore(it_cap);
    pieces[!wtm]++;
}





//--------------------------------
void UnmakePromotion(Move m)
{
    u8 prPc[] = {0, _q, _n, _r, _b};

    int prIx = m.flg & mPROM;

    auto it_prom = coords[wtm].begin();
    it_prom = b_state[prev_states + ply].nprom;
    auto before_king = king_coord[wtm];
    --before_king;
    coords[wtm].move_element(it_prom, before_king);
    material[wtm] -= pc_streng[GET_INDEX(prPc[prIx])]
            - pc_streng[GET_INDEX(_p)];
    quantity[wtm][GET_INDEX(_p)]++;
    quantity[wtm][GET_INDEX(prPc[prIx])]--;
}





//--------------------------------
void UnMoveFast(Move m)
{
    u8 fr = b_state[prev_states + ply].fr;
    auto it = coords[!wtm].begin();
    it = m.pc;
    *it = fr;
    b[fr] = (m.flg & mPROM) ? _P ^ wtm : b[m.to];
    b[m.to] = b_state[prev_states + ply].capt;

    reversible_moves = b_state[prev_states + ply].reversibleCr;

    wtm ^= white;

    if(m.flg & mCAPT)
        UnmakeCapture(m);

    if(m.flg & mPROM)
        UnmakePromotion(m);
    else if(m.flg & mCSTL)
        UnMakeCastle(m);

    ply--;

#ifndef NDEBUG
    cur_moves[5*ply] = '\0';
#endif // NDEBUG
}

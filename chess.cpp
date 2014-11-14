#include "chess.h"

//--------------------------------
UC  b[137];                                                             // board array in "0x88" style
short_list<UC, lst_sz> coords[2];

UC  attacks[240],                                                       // table for quick detect possible attacks
    get_delta[240];                                                     // I'm already forget what's this
SC  get_shift[240];                                                     // ... and this
UC rays[7]   = {0, 8, 8, 4, 4, 8, 0};
SC shifts[6][8] = {{ 0,  0,  0,  0,  0,  0,  0,  0},
                    { 1, 17, 16, 15, -1,-17,-16,-15},
                    { 1, 17, 16, 15, -1,-17,-16,-15},
                    { 1, 16, -1,-16, 0,  0,  0,  0},
                    {17, 15,-17,-15, 0,  0,  0,  0},
                    {18, 33, 31, 14,-18,-33,-31,-14}};
UC  pc_streng[7]    =  {0, 12, 6, 4, 4, 1};
short streng[7]     = {0, 15000, 120, 60, 40, 40, 10};
short sort_streng[7] = {0, 15000, 120, 60, 41, 39, 10};
UC  slider[7]       = {0, 0, 1, 1, 1, 0, 0};
int material[2], pieces[2];
BrdState  boardState[prev_states + max_ply];
unsigned wtm, ply;
UQ nodes, tmpCr;
char *cv;
US reversibleMoves;

#ifndef DONT_USE_PAWN_STRUCT
    int pmax[10][2], pmin[10][2];
#endif
UQ hash_key;
char curVar[5*max_ply];

short_list<UC, lst_sz>::iterator king_coord[2];
//--------------------------------
void InitChess()
{
    cv = curVar;
    InitBrd();

    pmax[1 - 1][0] = 0;
    pmax[1 - 1][1] = 0;
    pmin[1 - 1][0] = 7;
    pmin[1 - 1][1] = 7;
    pmax[8 + 1][0]  = 0;
    pmax[8 + 1][1]  = 0;
    pmin[8 + 1][0]  = 7;
    pmin[8 + 1][1]  = 7;
}

//--------------------------------
void InitBrd()
{
    UC pcs[] = {_N, _N, _B, _B, _R, _R, _Q, _K};
    UC crd[] = {6, 1, 5, 2, 7, 0, 3, 4};

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

    boardState[prev_states + 0].capt    = 0;
    boardState[prev_states + 0].cstl    = 0x0F;
    boardState[prev_states + 0].ep      = 0;
    wtm             = white;
    ply             = 0;
    nodes           = 0;
    cv[0]           = '\0';
    pieces[black]   = 16;
    pieces[white]   = 16;
    material[black] = 48;
    material[white] = 48;

    reversibleMoves     = 0;
#ifndef DONT_USE_PAWN_STRUCT
    InitPawnStruct();
#endif // DONT_USE_PAWN_STRUCT

    king_coord[white] = --coords[white].end();
    king_coord[black] = --coords[black].end();

    auto it = coords[white].begin();
    for(; it != coords[black].end(); ++it)
        if(b[*it] != _P)
            break;
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
    reversibleMoves = 0;

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
                if((pcs[i] & ~white) == _p && (row == 0 || row == 7))
                    return false;
                b[XY2SQ(col, row)] = pcs[i];
                material[i >= 6]  += mtr[i];
                pieces[i >= 6]++;
                p++;
            }// else
        }// for( col


    BoardToMen();

    boardState[prev_states + 0].ep   = 0;
    wtm = (*(++p) == 'b') ? black : white;

    UC cstl = 0;
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

    boardState[prev_states + 0].cstl = cstl;

    p++;
    if(*p != '-')
    {
        int col = *(p++) - 'a';
        int row = *(p++) - '1';
        int s = wtm ? -1 : 1;
        UC pawn = wtm ? _P : _p;
        if(b[XY2SQ(col-1, row+s)] == pawn || b[XY2SQ(col+1, row+s)] == pawn)
            boardState[prev_states + 0].ep = col + 1;
    }
    if(*(++p) && *(++p))
        while(*p >= '0' && *p <= '9')
        {
            reversibleMoves *= 10;
            reversibleMoves += (*p++ - '0');
        }

#ifndef DONT_USE_PAWN_STRUCT
    InitPawnStruct();
#endif // DONT_USE_PAWN_STRUCT

    king_coord[white] = --coords[white].end();
    king_coord[black] = --coords[black].end();

    return true;
}

//--------------------------------
void ShowMove(UC fr, UC to)
{
    char *cv = curVar + 5*(ply - 1);
    *(cv++) = COL(fr) + 'a';
    *(cv++) = ROW(fr) + '1';
    *(cv++) = COL(to) + 'a';
    *(cv++) = ROW(to) + '1';
    *(cv++) = ' ';
    *cv     = '\0';
}

//--------------------------------
bool MakeCastle(Move m, UC fr)
{
    UC cs = boardState[prev_states + ply].cstl;
    if(m.pc == king_coord[wtm])                     // K moves
        boardState[prev_states + ply].cstl &= wtm ? 0xFC : 0xF3;
    if(king_coord[wtm] != --coords[wtm].end())
        ply = ply;
    else if(b[fr] == (_r ^ wtm))        // R moves
    {
        if((wtm && fr == 0x07) || (!wtm && fr == 0x77))
            boardState[prev_states + ply].cstl &= wtm ? 0xFE : 0xFB;
        else if((wtm && fr == 0x00) || (!wtm && fr == 0x70))
            boardState[prev_states + ply].cstl &= wtm ? 0xFD : 0xF7;
    }
    if(b[m.to] == (_R ^ wtm))           // R is taken
    {
        if((wtm && m.to == 0x77) || (!wtm && m.to == 0x07))
            boardState[prev_states + ply].cstl &= wtm ? 0xFB : 0xFE;
        else if((wtm && m.to == 0x70) || (!wtm && m.to == 0x00))
            boardState[prev_states + ply].cstl &= wtm ? 0xF7 : 0xFD;
    }
    bool castleRightsChanged = (cs != boardState[prev_states + ply].cstl);
    if(castleRightsChanged)
        reversibleMoves = 0;

    if(!(m.flg & mCSTL))
        return castleRightsChanged;

    UC rFr, rTo;
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

    boardState[prev_states + ply].castled_rook_it = it;
    b[rTo] = b[rFr];
    b[rFr] = __;
    *it = rTo;

    reversibleMoves = 0;

    return false;
}

//--------------------------------
void UnMakeCastle(Move m)
{
    auto rMen = coords[wtm].begin();
    rMen = boardState[prev_states + ply].castled_rook_it;
    UC rFr =*rMen;
    UC rTo = rFr + (m.flg == mCS_K ? 2 : -3);
    b[rTo] = b[rFr];
    b[rFr] = __;
    *rMen = rTo;
}

//--------------------------------
bool MakeEP(Move m, UC fr)
{
    int delta = m.to - fr;
    UC to = m.to;
    if(ABSI(delta) == 0x20
    && (b[m.to + 1] == (_P ^ wtm) || b[to - 1] == (_P ^ wtm)))
    {
        boardState[prev_states + ply].ep = (to & 0x07) + 1;
        return true;
    }
    if(m.flg & mENPS)
        b[to + (wtm ? -16 : 16)] = __;
    return false;
}

//--------------------------------
bool SliderAttack(UC fr, UC to)
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
bool Attack(UC to, int xtm)
{
    if((!xtm && (b[to+15] == _p || b[to+17] == _p))
    ||  (xtm && ((to >= 15 && b[to-15] == _P)
        || (to >= 17 && b[to-17] == _P))))

        return true;

    auto it = king_coord[xtm];
    do
    {
        UC fr = *it;
        int pt  = b[fr]/2;
        if(pt   == _p/2)
            break;

        UC att = attacks[120 + to - fr] & (1 << pt);
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
bool LegalSlider(UC fr, UC to, UC pt)
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
    if(ic || (b[m.to] & ~white) == _k)
        return !Attack(*king_coord[!wtm], wtm);
    UC fr = boardState[prev_states + ply].fr;
    UC to = *king_coord[!wtm];
    assert(120 + to - fr >= 0);
    assert(120 + to - fr < 240);
    if(attacks[120 + to - fr] & (1 << _r/2))
        return LegalSlider(fr, to, _r);
    if(attacks[120 + to - fr] & (1 << _b/2))
        return LegalSlider(fr, to, _b);

    return true;
}

//-----------------------------
void SetPawnStruct(int x)
{
    assert(x >= 0 && x <= 7);
    int y;
    if(!wtm)
    {
        y = 1;
        while(b[XY2SQ(x, 7 - y)] != _p && y < 7)
            y++;
        pmin[x + 1][0] = y;

        y = 6;
        while(b[XY2SQ(x, 7 - y)] != _p && y > 0)
            y--;
        pmax[x + 1][0] = y;
    }
    else
    {
        y = 1;
        while(b[XY2SQ(x, y)] != _P && y < 7)
            y++;
        pmin[x + 1][1] = y;

        y = 6;
        while(b[XY2SQ(x, y)] != _P && y > 0)
            y--;
        pmax[x + 1][1] = y;
    }
}

//-----------------------------
void MovePawnStruct(UC movedPiece, UC fr, Move m)
{
    if((movedPiece/2) == _p/2 || (m.flg & mPROM))
    {
        SetPawnStruct(COL(m.to));
        if(m.flg)
            SetPawnStruct(COL(fr));
    }
    if(boardState[prev_states + ply].capt/2 == _p/2
    || (m.flg & mENPS))                                    // mENPS not needed
    {
        wtm ^= white;
        SetPawnStruct(COL(m.to));
        wtm ^= white;
    }
}

//-----------------------------
void InitPawnStruct()
{
    int x, y;
    for(x = 0; x < 8; x++)
    {
        pmax[x + 1][0] = 0;
        pmax[x + 1][1] = 0;
        pmin[x + 1][0] = 7;
        pmin[x + 1][1] = 7;
        for(y = 1; y < 7; y++)
            if(b[XY2SQ(x, y)] == _P)
            {
                pmin[x + 1][1] = y;
                break;
            }
        for(y = 6; y >= 1; y--)
            if(b[XY2SQ(x, y)] == _P)
            {
                pmax[x + 1][1] = y;
                break;
            }
        for(y = 6; y >= 1; y--)
            if(b[XY2SQ(x, y)] == _p)
            {
                pmin[x + 1][0] = 7 - y;
                break;
            }
         for(y = 1; y < 7; y++)
            if(b[XY2SQ(x, y)] == _p)
            {
                pmax[x + 1][0] = 7 - y;
                break;
            }
    }
}

//-----------------------------
bool PieceListCompare(UC men1, UC men2)
{
    return sort_streng[b[men1]/2] > sort_streng[b[men2]/2];
}

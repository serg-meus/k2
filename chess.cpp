#include "chess.h"

//--------------------------------
UC  unused1[24],                                                        // unused space for avoiding bounds check
    b[128],                                                             // board array in "0x88" style
    unused2[16],                                                        // unused space for avoiding bounds check
    men[64],                                                            // piece list
    menNxt[64],                                                         // pointer to next piece
    attacks[240],                                                       // table for quick deteck possible attacks
    get_delta[240];                                                     // I'm already forget what's this
signed char get_shift[240];                                             // ... and this
UC rays[]   = {0, 8, 8, 4, 4, 8, 0};
SC shifts[6][8] = {{ 0,  0,  0,  0,  0,  0,  0,  0},
                    { 1, 17, 16, 15, -1,-17,-16,-15},
                    { 1, 17, 16, 15, -1,-17,-16,-15},
                    { 1, 16, -1,-16, 0,  0,  0,  0},
                    {17, 15,-17,-15, 0,  0,  0,  0},
                    {18, 33, 31, 14,-18,-33,-31,-14}};
UC  pc_streng[]   =  {0, 0, 12, 6, 4, 4, 1};
UC  slider[] = {0, 0, 1, 1, 1, 0, 0};
int material[2], pieces[2];
BrdState  boardState[PREV_STATES + MAX_PLY];
unsigned wtm, ply;
UQ nodes, tmpCr;
char *cv;
US reversibleMoves;

#ifdef USE_PAWN_STRUCT
    int pmax_tmp[2], pmax[9][2], pmin_tmp[2], pmin[9][2];
#endif

char curVar[5*MAX_PLY];

//--------------------------------
void InitChess()
{
    cv = curVar;
    InitBrd();

    pmax[rays[0] - 1][0] = 0;       //<< NB: dirty hack used here to avoid compiler warnings
    pmax[rays[0] - 1][1] = 0;
    pmin[rays[0] - 1][0] = 7;
    pmin[rays[0] - 1][1] = 7;
    pmax[8][0]  = 0;
    pmax[8][1]  = 0;
    pmin[8][0]  = 7;
    pmin[8][1]  = 7;
}

//--------------------------------
void InitBrd()
{
    UC pcs[] = {_R, _N, _B, _Q, _K, _B, _N, _R};
    UC nms[] = { 2,  6,  4,  1,  0,  5,  7,  3};
    memset(b, 0, sizeof(b));
    memset(men, 0, sizeof(men));
    memset(menNxt, 0, sizeof(menNxt));
    InitAttacks();

    int i;
    for(i = 0; i < 8; i++)
    {
        b[XY2SQ(i, 1)]      = _P;
        men[i + 0x29 ]      = XY2SQ(i, 1);
        b[XY2SQ(i, 6)]      = _p;
        men[i + 0x09 ]      = XY2SQ(i, 6);
        b[XY2SQ(i, 0)]      = pcs[i];
        men[nms[i] + 0x21]  = XY2SQ(i, 0);
        b[XY2SQ(i, 7)]      = pcs[i] ^ WHT;
        men[nms[i] + 0x01]  = XY2SQ(i, 7);
    }
    for(i = 0; i < 0x10; i++)
    {
        menNxt[i]           = i + 1;
        menNxt[i + 0x20]    = i + 0x21;
    }
    boardState[PREV_STATES + 0].capt    = 0;
    boardState[PREV_STATES + 0].cstl    = 0x0F;
    boardState[PREV_STATES + 0].ep      = 0;
    wtm             = WHT;
    ply             = 0;
    nodes           = 0;
    cv[0]           = '\0';
    pieces[0]       = 16;
    pieces[1]       = 16;
    material[0]     = 48;
    material[1]     = 48;
/*    ttKey           = TTSetKey();
    InitPawnStruct();
*/
    reversibleMoves     = 0;
#ifdef USE_PAWN_STRUCT
    InitPawnStruct();
#endif // USE_PAWN_STRUCT
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
    char strng[]  = {0, 32, 12, 6, 4, 4, 1};
    unsigned wPieceCr = 0, bPieceCr = 0;
    unsigned i, j;
    for(i = 0; i < sizeof(men); i++)
    {
        men[i] = 0;
        menNxt[i] = 0;
    }
    for(i = 0; i < sizeof(b); i++)
    {
        if(!ONBRD(i) || b[i] == __)
            continue;
        if(b[i] < WHT)
            men[wPieceCr++] = i;
        else
            men[WHT + bPieceCr++] = i;
    }

    UC tmp;
    for(j = 0; j < wPieceCr; j++)
        for(i = 0; i < wPieceCr - 1; i++)
            if(strng[b[men[i]]] < strng[b[men[i + 1]]])
            {
                tmp         = men[i];
                men[i]      = men[i + 1];
                men[i + 1]  = tmp;
            }
    for(j = WHT; j < WHT + bPieceCr; j++)
        for(i = WHT; i < WHT + bPieceCr - 1; i++)
            if(strng[(b[men[i]] & ~WHT)] < strng[(b[men[i + 1]] & ~WHT)])
            {
                tmp         = men[i];
                men[i]      = men[i + 1];
                men[i + 1]  = tmp;
            }

    memmove(&men[1], &men[0], sizeof(men) - 1);
    men[0] = 0;

    for(i = 0; i < wPieceCr; i++)
        menNxt[i] = i + 1;
    for(i = WHT; i < WHT + bPieceCr; i++)
        menNxt[i] = i + 1;

    return true;
}

//--------------------------------
bool FenToBoard(char *p)
{
    char chars[] = "kqrbnpKQRBNP";
    char pcs[]  = { _k, _q, _r, _b, _n, _p, _K, _Q, _R, _B, _N, _P};
    int mtr[]   = {0, 12, 6, 4, 4, 1, 0, 12, 6, 4, 4, 1};
    int i;
    material[0] = 0;
    material[1] = 0;
    pieces[0]   = 0;
    pieces[1]   = 0;
    reversibleMoves = 0;

    for(int row = 7; row >= 0; row--)
        for(int col = 0; col <= 7; col++)
        {
            int to = XY2SQ(col, row);
            int ip = *p - '0';
            if(ip >= 1 && ip <= 8)
            {
                memset(&b[to], 0, ip*sizeof(*b));
                col += *p++ - '1';
            }
            else if(*p == '/')
            {
                p++;
                col = -1;
                continue;
            }
            else
            {
                i = 0;
                for(; i < 12 && *p != chars[i]; i++)
                    ;
                if(i >= 12)
                    return false;
                if((pcs[i] & 0x0F) == _p && (row == 0 || row == 7))
                    return false;
                b[XY2SQ(col, row)] = pcs[i];
                material[i >= 6]  += mtr[i];
                pieces[i >= 6]++;
                p++;
            }// else
        }// for( col
    BoardToMen();

    boardState[PREV_STATES + 0].ep   = 0;
    wtm = (*(++p) == 'b') ? BLK : WHT;

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
    boardState[PREV_STATES + 0].cstl = cstl;

    p++;
    if(*p != '-')
    {
        int col = *(p++) - 'a';
        int row = *(p++) - '1';
        int s = wtm ? -1 : 1;
        UC pawn = wtm ? _P : _p;
        if(b[XY2SQ(col-1, row+s)] == pawn || b[XY2SQ(col+1, row+s)] == pawn)
            boardState[PREV_STATES + 0].ep = col + 1;
    }
    if(*(++p) && *(++p))
        while(*p >= '0' && *p <= '9')
        {
            reversibleMoves *= 10;
            reversibleMoves += (*p++ - '0');
        }
/*    ttKey = TTSetKey();*/
#ifdef USE_PAWN_STRUCT
    InitPawnStruct();
#endif // USE_PAWN_STRUCT
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
    UC cs = boardState[PREV_STATES + ply].cstl;
    if(MOVEPC(m) == wtm + 1)                     // K moves
        boardState[PREV_STATES + ply].cstl &= wtm ? 0xFC : 0xF3;
    else if(b[fr] == (_r ^ wtm))        // R moves
    {
        if((wtm && fr == 0x07) || (!wtm && fr == 0x77))
            boardState[PREV_STATES + ply].cstl &= wtm ? 0xFE : 0xFB;
        else if((wtm && fr == 0x00) || (!wtm && fr == 0x70))
            boardState[PREV_STATES + ply].cstl &= wtm ? 0xFD : 0xF7;
    }
    if(b[MOVETO(m)] == (_R ^ wtm))           // R is taken
    {
        if((wtm && MOVETO(m) == 0x77) || (!wtm && MOVETO(m) == 0x07))
            boardState[PREV_STATES + ply].cstl &= wtm ? 0xFB : 0xFE;
        else if((wtm && MOVETO(m) == 0x70) || (!wtm && MOVETO(m) == 0x00))
            boardState[PREV_STATES + ply].cstl &= wtm ? 0xF7 : 0xFD;
    }
    bool castleRightsChanged = (cs != boardState[PREV_STATES + ply].cstl);
    if(castleRightsChanged)
        reversibleMoves = 0;

    if(!(MOVEFLG(m) & (mCSTL << 16)))
        return castleRightsChanged;

    UC rFr, rTo;
    if(MOVEFLG(m) == (mCS_K << 16))
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
    unsigned i;
    for(i = wtm + 1; i < wtm + 0x20; i++)   //<< NB: wrong
        if(men[i] == rFr)
            break;
    assert(men[i] == rFr);

    boardState[PREV_STATES + ply].ncstl_r = i;
    b[rTo] = b[rFr];
    b[rFr] = __;
    men[i] = rTo;

    reversibleMoves = 0;

    return false;
}

//--------------------------------
void UnMakeCastle(Move m)
{
    UC rMen = boardState[PREV_STATES + ply].ncstl_r;
    UC rFr = men[rMen];
    UC rTo = rFr + (MOVEFLG(m) == (mCS_K << 16) ? 2 : -3);
    b[rTo] = b[rFr];
    b[rFr] = __;
    men[rMen] =rTo;
}

//--------------------------------
bool MakeEP(Move m, UC fr)
{
    int delta = MOVETO(m) - fr;
    UC to = MOVETO(m);
    if(ABSI(delta) == 0x20
    && (b[MOVETO(m) + 1] == (_P ^ wtm) || b[to - 1] == (_P ^ wtm)))
    {
        boardState[PREV_STATES + ply].ep = (to & 7) + 1;
        return true;
    }
    if(MOVEFLG(m) & (mENPS << 16))
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
    UC fr;

    if((!xtm && (b[to+15] == _p || b[to+17] == _p))
    ||  (xtm && (b[to-15] == _P || b[to-17] == _P)))
        return true;

    UC menNum = xtm;
    for(int menCr = 0; menCr < pieces[xtm >> 5]; menCr++)
    {
        menNum  = menNxt[menNum];
        fr      = men[menNum];

        assert(fr < sizeof(b)/sizeof(*b));
        int pt  = b[fr] & 7;
        if(pt   == _p)
            break;

        assert((unsigned)(120 + to - fr) < sizeof(attacks)/sizeof(*attacks));
        UC att = attacks[120 + to - fr] & (1 << pt);
        if(!att)
            continue;
        assert((unsigned)pt < sizeof(slider)/sizeof(*slider));
        if(!slider[pt])
            return true;
        if(SliderAttack(to, fr))
             return true;
    }// for (menCr

    return false;
}

//--------------------------------
bool LegalSlider(UC fr, UC to, UC pt)
{
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
    if(ic || (b[MOVETO(m)] & ~WHT) == _k)
        return !Attack(men[(wtm ^ WHT) + 1], wtm);
    UC fr = boardState[PREV_STATES + ply].fr;
    UC to = men[(wtm ^ WHT) + 1];
    if(attacks[120 + to - fr] & (1 << _r))
        return LegalSlider(fr, to, _r);
    if(attacks[120 + to - fr] & (1 << _b))
        return LegalSlider(fr, to, _b);

    return true;
}

//-----------------------------
void SetPawnStruct(int x)
{
    int y;
    if(!wtm)
    {
        y = 1;
        while(b[XY2SQ(x, 7 - y)] != _p && y < 7)
            y++;
        pmin[x][0] = y;

        y = 6;
        while(b[XY2SQ(x, 7 - y)] != _p && y > 0)
            y--;
        pmax[x][0] = y;
    }
    else
    {
        y = 1;
        while(b[XY2SQ(x, y)] != _P && y < 7)
            y++;
        pmin[x][1] = y;

        y = 6;
        while(b[XY2SQ(x, y)] != _P && y > 0)
            y--;
        pmax[x][1] = y;
    }
}

//-----------------------------
void MovePawnStruct(UC movedPiece, UC fr, Move m)
{
    if((movedPiece & 0x0F) == _p
    || (MOVEFLG(m) & (mPROM << 16)))
    {
        SetPawnStruct(COL(MOVETO(m)));
        if(MOVEFLG(m))
            SetPawnStruct(COL(fr));
    }
    if((boardState[PREV_STATES + ply].capt & 0x0F) == _p
    || (MOVEFLG(m) & (mENPS << 16)))                                    // mENPS нахрена например?
    {
        wtm ^= WHT;
        SetPawnStruct(COL(MOVETO(m)));
        wtm ^= WHT;
    }
}

//-----------------------------
void InitPawnStruct()
{
    int x, y;
    for(x = 0; x < 8; x++)
    {
        pmax[x][0] = 0;
        pmax[x][1] = 0;
        pmin[x][0] = 7;
        pmin[x][1] = 7;
        for(y = 1; y < 7; y++)
            if(b[XY2SQ(x, y)] == _P)
            {
                pmin[x][1] = y;
                break;
            }
        for(y = 6; y >= 1; y--)
            if(b[XY2SQ(x, y)] == _P)
            {
                pmax[x][1] = y;
                break;
            }
        for(y = 6; y >= 1; y--)
            if(b[XY2SQ(x, y)] == _p)
            {
                pmin[x][0] = 7 - y;
                break;
            }
         for(y = 1; y < 7; y++)
            if(b[XY2SQ(x, y)] == _p)
            {
                pmax[x][0] = 7 - y;
                break;
            }
    }
}

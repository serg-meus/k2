#include "hash.h"

//--------------------------------
UQ  zorb[12][8][8],
    zorb_en_passant[9],
    zorb_castling[16];


/*int MEN_TO_ZORB(UC x)
{
    return men2zorb[x];
}
*/
//--------------------------------
bool InitHashTable()
{
    InitChess();

    std::uniform_int_distribution<UQ> zorb_distr(0, (UQ)-1);
    std::mt19937 rnd_gen;
//    rnd_gen.seed(31);

    for(unsigned i = 0; i < 12; ++i)
        for(unsigned j = 0; j < 8; ++j)
            for(unsigned k = 0; k < 8; ++k)
                zorb[i][j][k] = zorb_distr(rnd_gen);

    for(unsigned i = 1;
    i < sizeof(zorb_en_passant)/sizeof(*zorb_en_passant);
    ++i)
        zorb_en_passant[i] = zorb_distr(rnd_gen);

    for(unsigned i = 1;
    i < sizeof(zorb_castling)/sizeof(*zorb_castling);
    ++i)
        zorb_castling[i] = zorb_distr(rnd_gen);

    zorb_en_passant[0] = 0;
    zorb_castling[0] = 0;

    return true;
}

//--------------------------------
UQ InitHashKey()
{
    UQ ans = 0;

    UC menNum   = wtm;
    menNum      = menNxt[menNum];
    unsigned maxPc = pieces[wtm >> 5];

    for(unsigned pcCr = 0; pcCr < maxPc; ++pcCr)
    {
        UC pc = men[menNum];
        ans ^= zorb[MEN_TO_ZORB(b[pc])][COL(pc)][ROW(pc)];
        menNum      = menNxt[menNum];
    }
    menNum  = wtm ^ WHT;
    menNum  = menNxt[menNum];
    maxPc   = pieces[(wtm ^ WHT) >> 5];

    for(unsigned pcCr = 0; pcCr < maxPc; ++pcCr)
    {
        UC pc = men[menNum];
        ans ^= zorb[MEN_TO_ZORB(b[pc])][COL(pc)][ROW(pc)];
        menNum      = menNxt[menNum];
    }

    if(!wtm)
        ans ^= (UQ)-1;
    BrdState &f = boardState[PREV_STATES + ply];
    ans ^= zorb_en_passant[f.ep] ^ zorb_castling[f.cstl];

    return ans;
}

//--------------------------------
void MoveHashKey(Move m, UC fr, int special)
{
    UC pt   = b[MOVETO(m)];
    BrdState &f = boardState[PREV_STATES + ply],
            &_f = boardState[PREV_STATES + ply - 1];

    hash_key ^= zorb[MEN_TO_ZORB(pt)][COL(fr)][ROW(fr)]
           ^ zorb[MEN_TO_ZORB(pt)][COL(MOVETO(m))][ROW(MOVETO(m))];

    if(f.capt)
        hash_key ^= zorb[MEN_TO_ZORB(f.capt)][COL(MOVETO(m))][ROW(MOVETO(m))];
    if(_f.ep)
        hash_key ^= zorb_en_passant[_f.ep];

    if(MOVEFLG(m) & (mPROM << 16))
        hash_key ^= zorb[MEN_TO_ZORB(_p ^ wtm)][COL(fr)][ROW(fr)]
               ^ zorb[MEN_TO_ZORB(pt)][COL(fr)][ROW(fr)];
    else if(MOVEFLG(m) & (mENPS << 16))
        hash_key ^= zorb[MEN_TO_ZORB(_P ^ wtm)][COL(MOVETO(m))]
            [ROW(MOVETO(m)) + (wtm ? -1 : 1)];
    else if(MOVEFLG(m) & (mCSTL << 16))
    {
        if(wtm)
        {
            if(MOVEFLG(m) & (mCS_K << 16))
                hash_key ^= zorb[MEN_TO_ZORB(_R)][7][0]
                         ^  zorb[MEN_TO_ZORB(_R)][5][0];
            else
                hash_key ^= zorb[MEN_TO_ZORB(_R)][0][0]
                         ^  zorb[MEN_TO_ZORB(_R)][3][0];
        }
        else
        {
            if(MOVEFLG(m) & (mCS_K << 16))
                hash_key ^= zorb[MEN_TO_ZORB(_r)][7][7]
                         ^  zorb[MEN_TO_ZORB(_r)][5][7];
            else
                hash_key ^= zorb[MEN_TO_ZORB(_r)][0][7]
                         ^  zorb[MEN_TO_ZORB(_r)][3][7];
        }
        hash_key ^= zorb_castling[_f.cstl]
                 ^  zorb_castling[f.cstl];
    }
    hash_key ^= -1L;
    if(!special)
        return;
    if((pt & ~WHT) == _p && !f.capt)
        hash_key ^= zorb_en_passant[f.ep];
    else
        hash_key ^= zorb_castling[_f.cstl] ^ zorb_castling[f.cstl];
}

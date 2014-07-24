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

    for(auto fr : pc_list[wtm])
        ans ^= zorb[MEN_TO_ZORB(b[fr])][COL(fr)][ROW(fr)];

    for(auto fr : pc_list[!wtm])
        ans ^= zorb[MEN_TO_ZORB(b[fr])][COL(fr)][ROW(fr)];

    if(!wtm)
        ans ^= (UQ)-1;
    BrdState &f = boardState[prev_states + ply];
    ans ^= zorb_en_passant[f.ep] ^ zorb_castling[f.cstl];

    return ans;
}

//--------------------------------
void MoveHashKey(Move m, UC fr, int special)
{
    UC pt   = b[m.to];
    BrdState &f = boardState[prev_states + ply],
            &_f = boardState[prev_states + ply - 1];

    hash_key ^= zorb[MEN_TO_ZORB(pt)][COL(fr)][ROW(fr)]
           ^ zorb[MEN_TO_ZORB(pt)][COL(m.to)][ROW(m.to)];

    if(f.capt)
        hash_key ^= zorb[MEN_TO_ZORB(f.capt)][COL(m.to)][ROW(m.to)];
    if(_f.ep)
        hash_key ^= zorb_en_passant[_f.ep];

    if(m.flg & mPROM)
        hash_key ^= zorb[MEN_TO_ZORB(_p ^ wtm)][COL(fr)][ROW(fr)]
               ^ zorb[MEN_TO_ZORB(pt)][COL(fr)][ROW(fr)];
    else if(m.flg & mENPS)
        hash_key ^= zorb[MEN_TO_ZORB(_P ^ wtm)][COL(m.to)]
            [ROW(m.to) + (wtm ? -1 : 1)];
    else if(m.flg & mCSTL)
    {
        if(wtm)
        {
            if(m.flg & mCS_K)
                hash_key ^= zorb[MEN_TO_ZORB(_R)][7][0]
                         ^  zorb[MEN_TO_ZORB(_R)][5][0];
            else
                hash_key ^= zorb[MEN_TO_ZORB(_R)][0][0]
                         ^  zorb[MEN_TO_ZORB(_R)][3][0];
        }
        else
        {
            if(m.flg & mCS_K)
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
    if((pt & ~white) == _p && !f.capt)
        hash_key ^= zorb_en_passant[f.ep];
    else
        hash_key ^= zorb_castling[_f.cstl] ^ zorb_castling[f.cstl];
}

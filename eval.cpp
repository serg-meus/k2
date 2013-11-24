#include "eval.h"

short valOpn, valEnd;

short MatArrOpn[] = {  0, 0, Q_VAL_OPN, R_VAL_OPN, B_VAL_OPN, N_VAL_OPN, P_VAL_OPN};
short MatArrEnd[] = {  0, 0, Q_VAL_END, R_VAL_END, B_VAL_END, N_VAL_END, P_VAL_END};

char  kingDist[120];
short tropism[]   = {  0, 0, -50, -30, -20, -20, -10};

//-----------------------------
void InitEval()
{
    valOpn = 0;
    valEnd = 0;
    InitMoveGen();
    for(int i = 0; i < 120; i++)
        if(i & 8)
            kingDist[i] = MAXI(8 - COL(i), ROW(i) + 1);
        else
            kingDist[i] = MAXI(COL(i), ROW(i));
}

//-----------------------------
short Eval(/*short alpha, short beta*/)
{
/*    if(wtm && (valOpn < alpha - 550 && valEnd < alpha - 550)pieces
    || wtm && (valOpn > beta + 550 && valEnd > beta + 550)
    || !wtm && (-valOpn < alpha - 550 && -valEnd < alpha - 550)
    || !wtm && (-valOpn < alpha - 550 && -valEnd < alpha - 550))
        return valOpn;*/

    boardState[PREV_STATES + ply].valOpn = valOpn;
    boardState[PREV_STATES + ply].valEnd = valEnd;

#ifdef USE_PAWN_STRUCT
    EvalPawns((bool)WHT);
    EvalPawns((bool)BLK);
#endif // USE_PAWN_STRUCT


    if(!(boardState[PREV_STATES + ply].cstl & 0x03))
        SimpleKingShield(WHT);
    if(!(boardState[PREV_STATES + ply].cstl & 0x0C))
        SimpleKingShield(BLK);

//    SimpleKingDist(WHT);
//    SimpleKingDist(BLK);


#ifdef USE_PAWN_STRUCT
    ClampedRook(WHT);
    ClampedRook(BLK);
#endif // USE_PAWN_STRUCT

#ifndef CHECK_PREDICTED_VALUE
    if(reversibleMoves > ply)
    {
        valOpn = (int)valOpn * (USELESS_HALFMOVES_TO_DRAW - reversibleMoves) / USELESS_HALFMOVES_TO_DRAW;
        valEnd = (int)valEnd * (USELESS_HALFMOVES_TO_DRAW - reversibleMoves) / USELESS_HALFMOVES_TO_DRAW;
    }
#endif


    int X, Y;
    X = material[0] + 1 + material[1] + 1 - pieces[0] - pieces[1];
    Y = ((valOpn - valEnd)*X + 80*valEnd)/80;

    valOpn = boardState[PREV_STATES + ply].valOpn;
    valEnd = boardState[PREV_STATES + ply].valEnd;

    return wtm ? (short)(-Y) : (short)Y;
}


//-----------------------------
void FastEval(Move m)
{
    short deltaScoreOpn = 0, deltaScoreEnd = 0;

    int x  = COL(MOVETO(m));
    int y  = ROW(MOVETO(m));
    int x0 = COL(boardState[PREV_STATES + ply].fr);
    int y0 = ROW(boardState[PREV_STATES + ply].fr);
    UC pt  = b[MOVETO(m)] & ~WHT;

    if(!wtm)
    {
        y  = 7 - y;
        y0 = 7 - y0;
    }

    if(MOVEFLG(m) & (mPROM << 16))
    {
        deltaScoreOpn -= MatArrOpn[_p] + pst[_p - 1][0][y0][x0];
        deltaScoreEnd -= MatArrEnd[_p] + pst[_p - 1][1][y0][x0];
        switch((MOVEFLG(m) >> 16) & mPROM)
        {
            case mPR_Q :    deltaScoreOpn += MatArrOpn[_q] + pst[_q - 1][0][y0][x0];
                            deltaScoreEnd += MatArrEnd[_q] + pst[_q - 1][1][y0][x0];
                            break;
            case mPR_R :    deltaScoreOpn += MatArrOpn[_r] + pst[_r - 1][0][y0][x0];
                            deltaScoreEnd += MatArrEnd[_r] + pst[_r - 1][1][y0][x0];
                            break;
            case mPR_B :    deltaScoreOpn += MatArrOpn[_b] + pst[_b - 1][0][y0][x0];
                            deltaScoreEnd += MatArrOpn[_b] + pst[_b - 1][1][y0][x0];
                            break;
            case mPR_N :    deltaScoreOpn += MatArrOpn[_n] + pst[_n - 1][0][y0][x0];
                            deltaScoreEnd += MatArrOpn[_n] + pst[_n - 1][1][y0][x0];
                            break;
        }
    }

    if(MOVEFLG(m) & (mCAPT << 16))
    {
        UC capt = boardState[PREV_STATES + ply].capt & ~WHT;
        if(MOVEFLG(m) & (mENPS << 16))
        {
            deltaScoreOpn +=  MatArrOpn[_p] + pst[_p - 1][0][7 - y0][x];
            deltaScoreEnd +=  MatArrEnd[_p] + pst[_p - 1][1][7 - y0][x];
        }
        else
        {
            deltaScoreOpn +=  MatArrOpn[capt] + pst[capt - 1][0][7 - y][x];
            deltaScoreEnd +=  MatArrEnd[capt] + pst[capt - 1][1][7 - y][x];
        }
    }
    else if(MOVEFLG(m) & (mCS_K << 16))
    {
        deltaScoreOpn += pst[_r - 1][0][7][5] - pst[_r - 1][0][7][7];
        deltaScoreEnd += pst[_r - 1][1][7][5] - pst[_r - 1][1][7][7];
    }
    else if(MOVEFLG(m) & (mCS_Q << 16))
    {
        deltaScoreOpn += pst[_r - 1][0][7][3] - pst[_r - 1][0][7][0];
        deltaScoreEnd += pst[_r - 1][1][7][3] - pst[_r - 1][1][7][0];
    }

    deltaScoreOpn += pst[pt - 1][0][y][x] - pst[pt - 1][0][y0][x0];
    deltaScoreEnd += pst[pt - 1][1][y][x] - pst[pt - 1][1][y0][x0];

#ifdef EVAL_KING_TROPISM

    UC k        = men[wtm + 1];
    UC fr       = boardState[PREV_STATES + ply].fr;
    UC to       = MOVETO(m);
    UC kdist_fr = kingDist[ABSI(k - fr)];
    UC kdist_to = kingDist[ABSI(k - to)];

    if(pt == _k)
    {
        deltaScoreOpn -= EvalAllKingDist(wtm, fr);
        deltaScoreOpn += EvalAllKingDist(wtm, to);
    }
    else if(kdist_fr >  2 && kdist_to <= 2)
        deltaScoreOpn -= tropism[pt];
    else if(kdist_fr <= 2 && kdist_to >  2)
        deltaScoreOpn += (MOVEFLG(m) & (mPROM << 16)) ? tropism[_p] : tropism[pt];
    else
        ply = ply;

    if(kdist_to <= 2 && (MOVEFLG(m) & (mPROM << 16)))
    {
        deltaScoreOpn += tropism[_p];
        if(kdist_fr <= 2)
            deltaScoreOpn -= tropism[pt];
    }

    if(MOVEFLG(m)  & (mCAPT << 16))
    {
        k        = men[(wtm ^ WHT) + 1];
        const bool en_ps = MOVEFLG(m) & (mENPS << 16);
        if(en_ps)
            to += wtm ? 16 : -16;
        kdist_to = kingDist[ABSI(k - to)];
        if(kdist_to <= 2)
        {
            UC capt;
            if(en_ps)
                capt = _p;
            else
                capt = boardState[PREV_STATES + ply].capt & ~WHT;
            deltaScoreOpn -= tropism[capt];
        }// if(kdist_to
    }// if(MOVEGLG(m) ...
#endif // EVAL_KING_TROPISM

    if(wtm)
    {
        valOpn -= deltaScoreOpn;
        valEnd -= deltaScoreEnd;
    }
    else
    {
        valOpn += deltaScoreOpn;
        valEnd += deltaScoreEnd;
    }
}

//-----------------------------
void EvalAllMaterialAndPST()
{
    valOpn = 0;
    valEnd = 0;
    for(unsigned i = 0; i < sizeof(b); ++i)
    {
        UC pt = b[i];
        if(pt == __)
            continue;
        UC x0 = COL(i);
        UC y0 = ROW(i);
        if(pt & WHT)
            y0 = 7 - y0;

        short tmpOpn = MatArrOpn[pt & ~WHT] + pst[(pt & ~WHT) - 1][0][y0][x0];
        short tmpEnd = MatArrEnd[pt & ~WHT] + pst[(pt & ~WHT) - 1][1][y0][x0];

        if(pt & WHT)
        {
            valOpn += tmpOpn;
            valEnd += tmpEnd;
        }
        else
        {
            valOpn -= tmpOpn;
            valEnd -= tmpEnd;
        }
    }
#ifdef EVAL_KING_TROPISM
    valOpn -= EvalAllKingDist(WHT, men[BLK + 1]);
    valOpn += EvalAllKingDist(BLK, men[WHT + 1]);
#endif // EVAL_KING_TROPISM
}


//#ifdef USE_PAWN_STRUCT
//--------------------------------
void EvalPawns(bool stm)
{
    short ansO = 0, ansE = 0;
	bool promo, prevPromo = false;
    bool oppHasOnlyPawns = material[!stm] == pieces[!stm] - 1;
    for(int i = 0; i < 8; i++)
    {
        bool doubled = false, isolany = false;

        int mx = pmax[i][stm];
        if(mx == 0)
            continue;

        if(pmin[i][stm] != mx)
            doubled = true;
        if(pmax[i - 1][stm] == 0 && pmax[i + 1][stm] == 0)
            isolany = true;
        if(doubled && isolany)
        {
            ansE -= 55;
            ansO -= 15;
        }
        else if(isolany)
        {
            ansE -= 10;
            ansO -= 10;
        }
        else if(doubled)
        {
            ansE -= 4;
            ansO -= 1;
        }

        promo = false;
        if(mx >= 7 - pmin[  i  ][!stm]
        && mx >= 7 - pmin[i - 1][!stm]
        && mx >= 7 - pmin[i + 1][!stm])
            promo = true;

        if(promo)
        {
            if(i > 0 && prevPromo && mx >= 5
            && pmax[i - 1][stm] >= 5)
                ansE += DBL_PROMO_P*MAXI(mx, pmax[i - 1][stm]);

            prevPromo = true;
            short delta = 16*mx - 16;
            ansE += delta;
            ansO += -delta/4;

            if(oppHasOnlyPawns)
            {
                UC k = men[stm << 4];
                if(!stm)
                    k ^= 0x70;
                if(TestUnstoppable(i, 7 - pmax[i][stm], stm << 5))
                {
                    ansO += 120*mx + 350;
                    ansE += 120*mx + 350;
                }
                else if(pmax[i][stm] == 4 && ABSI(COL(k) - i) <= 1
                && i != 0 && i != 7 && ROW(k) > 4 && ROW(k) < 7)
                {
                    ansO += 120*mx + 350;
                    ansE += 120*mx + 350;
                }
            }

        }// if promo
        else
            prevPromo = false;
    }// for i


/*    int connectedCr = 0;
    for(int i = 1; i < 8; i++)
    {
        int mx  = pmax[i][stm];
        int pmx = pmax[i - 1][stm];
        if(mx == 0 || ABSI(mx - pmx) > 1)
        {
            ansE += connectedCr*3/5;
//            ansO += connectedCr*3/5;
            connectedCr = 0;
        }
        else
            connectedCr++;
    }
*/
    valOpn += stm ? ansO : -ansO;
    valEnd += stm ? ansE : -ansE;
}

//-----------------------------
void ClampedRook(UC stm)
{
    UC k = men[stm + 1];

    if(stm)
    {
        if(k == 0x06 && b[0x07] == _R && pmax[7][1])
            valOpn -= CLAMPED_R;
        else if(k == 0x05)
        {
            if(pmax[7][1] && b[0x07] == _R)
                valOpn -= CLAMPED_R;
            else
            if(pmax[6][1] && b[0x06] == _R)
                valOpn -= CLAMPED_R;
        }
        else if(k == 0x01 && b[0x00] == _R && pmax[0][1])
            valOpn -= CLAMPED_R;
        else if(k == 0x02)
        {
            if(pmax[0][1] && b[0x00] == _R)
                valOpn -= CLAMPED_R;
            else
            if(pmax[1][1] && b[0x01] == _R)
                valOpn -= CLAMPED_R;
        }
     }
     else
     {
        if(k == 0x76 && b[0x77] == _r && pmax[7][0])
            valOpn += CLAMPED_R;
        else if(k == 0x75)
        {
            if(pmax[7][0] && b[0x77] == _r)
                valOpn += CLAMPED_R;
            else
            if(pmax[6][0] && b[0x76] == _r)
                valOpn += CLAMPED_R;
        }
        else if(k == 0x71 && b[0x70] == _r && pmax[0][0])
            valOpn += CLAMPED_R;
        else if(k == 0x72)
        {
            if(pmax[0][0] && b[0x70] == _r)
                valOpn += CLAMPED_R;
            else
            if(pmax[1][0] && b[0x71] == _r)
                valOpn += CLAMPED_R;
        }
     }
}
//#endif // USE_PAWN_STRUCT

//-----------------------------
void SimpleKingShield(UC stm)
{
    short ans = 0;
    int k = men[stm + 1];
    int shft = stm ? 16 : -16;

    if((stm && (k == 0x00 || k == 0x05))
    || (!stm && (k == 0x70 || k == 0x75)))
        k++;
    if((stm && (k == 0x07 || k == 0x02))
    || (!stm && (k == 0x77 || k == 0x72)))
        k--;

    int pwn1 = 0, pwn2 = 0;
    for(int i = k + shft - 1; i <= k + shft + 1; i++)
    {
        if(b[i] == (_p ^ stm))
            pwn1++;
        if(!ONBRD(i + shft))
            continue;
        if(b[i + shft] == (_p ^ stm))
            pwn2++;
    }
    if(pwn1 == 3 || (pwn1 == 2 && pwn2 >= 1))
        ans = SHIELD_K;

    short y = ans * material[!stm] / 48;

    valOpn += stm ? y : -y;
}

//-----------------------------
void SimpleKingDist(UC stm)
{
    UC menNum   = stm ^ WHT;
    menNum      = menNxt[menNum];
    UC maxPc    = pieces[!stm];
    UC k        = men[stm + 1];
    unsigned i  = 0;
    short ans   = 0, ans2 = 0;

    for(i = 0; i < maxPc; ++i)
    {
        UC fr = men[menNum];
        menNum = menNxt[menNum];
        int dst = kingDist[ABSI(k - fr)];
        if(dst > 3)
            continue;

        short X;
        switch(b[fr] & ~WHT)
        {
        case _p :
            X = material[!stm] + 1 - pieces[!stm];
            if(dst <= 1)
                ans += (40 - X) * 5 / 4;
            else if(dst <= 2)
                ans += (40 - X) * 5 / 8;
            break;
        case _q :
            ans2 += dst <= 2 ? -104 : -104 / 4;
            break;
        case _r :
            ans2 += dst <= 2 ? -114 : -114 / 4;
            break;
        case _n :
            ans2 += dst <= 2 ? -63 : -28;
            break;
        }// switch
    }// for i

    menNum   = stm;
    menNum   = menNxt[menNum];
    maxPc    = pieces[stm >> 5];

    for(i = 0; i < maxPc; ++i)
    {
        UC fr = men[menNum];
        menNum = menNxt[menNum];
        if((b[fr] & ~WHT) == _p)
        {
            int dst = kingDist[ABSI(k - fr)];
            short X = material[!stm] + 1 - pieces[!stm];
            if(dst <= 1)
            {
                ans     += (40 - X);
                ans2    += (40 - X) * 5 / 4;
            }
            else if(dst <= 2)
            {
                ans     += (40 - X) / 2;
                ans     += (40 - X) * 5 / 8;
            }
        }// if
    }// for i

    valEnd += stm ? ans  : -ans;
    valOpn += stm ? ans2 : -ans2;
}

//-----------------------------
bool TestUnstoppable(int x, int y, UC stm)
{
    UC k    = men[(stm ^ WHT) + 1];
    if(y > 5)
        y = 5;
    int psq = XY2SQ(x, stm ? 7 : 0);
    int d = kingDist[ABSI(k - psq)];
    if(COL(men[stm + 1]) == x && kingDist[ABSI(men[stm + 1] - psq)] <= y)
        y++;
    return d - (stm != wtm) > y;
}

//-----------------------------
short EvalAllKingDist(UC stm, UC king_coord)
{
    UC menNum   = stm;
    menNum      = menNxt[menNum];
    UC maxPc    = pieces[stm >> 5];
    unsigned i  = 0;
    short ans   = 0;

    for(; i < maxPc; ++i)
    {
        UC fr = men[menNum];
        UC pt = b[fr] & ~WHT;

        int x = kingDist[ABSI(king_coord - fr)];
        menNum = menNxt[menNum];
        if(x > 2)
            continue;

        ans += tropism[pt];
    }

    return ans;
}

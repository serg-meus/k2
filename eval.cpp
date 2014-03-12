#include "eval.h"

short valOpn, valEnd;

short MatArrOpn[] = {  0, 0, Q_VAL_OPN, R_VAL_OPN, B_VAL_OPN, N_VAL_OPN, P_VAL_OPN};
short MatArrEnd[] = {  0, 0, Q_VAL_END, R_VAL_END, B_VAL_END, N_VAL_END, P_VAL_END};

char  kingDist[120];
short tropism[]   = {  0, 0, -50, -30, -20, -20, -10};

#ifdef TUNE_PARAMETERS
    std::vector <float> param;
#endif // TUNE_PARAMETERS

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

    KingSafety(WHT);
    KingSafety(BLK);

#ifdef USE_PAWN_STRUCT
    ClampedRook(WHT);
    ClampedRook(BLK);
#endif // USE_PAWN_STRUCT


#ifndef CHECK_PREDICTED_VALUE
   if(reversibleMoves > ply)
    {
        valOpn = (int)valOpn * (FIFTY_MOVES - reversibleMoves) / FIFTY_MOVES;
        valEnd = (int)valEnd * (FIFTY_MOVES - reversibleMoves) / FIFTY_MOVES;
    }
#endif


    int X, Y;
    X = material[0] + 1 + material[1] + 1 - pieces[0] - pieces[1];

    if(X == 3
    && (material[0] == 4 || material[1] == 4)
    && (pieces[0] + pieces[1] == 3))                // KNk, KBk, Kkn, Kkb
        return 0;

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
        if(!ONBRD(i))
            continue;
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
    valOpn -= EvalAllKingDist(WHT, men[BLK + 1));
    valOpn += EvalAllKingDist(BLK, men[WHT + 1));
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

        int mx = pmax[i + 1][stm];
        if(mx == 0)
            continue;

        if(pmin[i + 1][stm] != mx)
            doubled = true;
        if(pmax[i - 0][stm] == 0 && pmax[i + 2][stm] == 0)
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
        if(mx >= 7 - pmin[i + 1][!stm]
        && mx >= 7 - pmin[i - 0][!stm]
        && mx >= 7 - pmin[i + 2][!stm])
            promo = true;

        if(promo)
        {
            if(i > 0 && prevPromo && mx >= 5
            && pmax[i - 0][stm] >= 5)
                ansE += DBL_PROMO_P*MAXI(mx, pmax[i - 0][stm]);

            prevPromo = true;
            short delta = 16*mx - 16;
            ansE += delta;
            ansO += -delta/4;

/*            UC k = men[(stm << 5) + 1];
            UC col_k = COL(k);
            if(ABSI(i - col_k) == 1)
            {
                UC row_k = stm ? ROW(k) : 7 - ROW(k);
                if(mx == row_k || mx == row_k - 1)
                    ansE += 60;
            }
*/

            if(oppHasOnlyPawns)
            {
                if(TestUnstoppable(i, 7 - pmax[i + 1][stm], stm << 5))
                {
                    ansO += 120*mx + 350;
                    ansE += 120*mx + 350;
                }
            }

        }// if promo
        else
            prevPromo = false;
    }// for i

    valOpn += stm ? ansO : -ansO;
    valEnd += stm ? ansE : -ansE;
}

//-----------------------------
void ClampedRook(UC stm)
{
    UC k = men[stm + 1];

    if(stm)
    {
        if(k == 0x06 && b[0x07] == _R && pmax[7 + 1][1])
            valOpn -= CLAMPED_R;
        else if(k == 0x05)
        {
            if(pmax[7 + 1][1] && b[0x07] == _R)
                valOpn -= CLAMPED_R;
            else
            if(pmax[6 + 1][1] && b[0x06] == _R)
                valOpn -= CLAMPED_R;
        }
        else if(k == 0x01 && b[0x00] == _R && pmax[0 + 1][1])
            valOpn -= CLAMPED_R;
        else if(k == 0x02)
        {
            if(pmax[0 + 1][1] && b[0x00] == _R)
                valOpn -= CLAMPED_R;
            else
            if(pmax[1 + 1][1] && b[0x01] == _R)
                valOpn -= CLAMPED_R;
        }
     }
     else
     {
        if(k == 0x76 && b[0x77] == _r && pmax[7 + 1][0])
            valOpn += CLAMPED_R;
        else if(k == 0x75)
        {
            if(pmax[7 + 1][0] && b[0x77] == _r)
                valOpn += CLAMPED_R;
            else
            if(pmax[6 + 1][0] && b[0x76] == _r)
                valOpn += CLAMPED_R;
        }
        else if(k == 0x71 && b[0x70] == _r && pmax[0 + 1][0])
            valOpn += CLAMPED_R;
        else if(k == 0x72)
        {
            if(pmax[0 + 1][0] && b[0x70] == _r)
                valOpn += CLAMPED_R;
            else
            if(pmax[1 + 1][0] && b[0x71] == _r)
                valOpn += CLAMPED_R;
        }
     }
}
//#endif // USE_PAWN_STRUCT

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
/*short EvalAllKingDist(UC stm, UC king_coord)
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

//-----------------------------
short KingShieldFactor(UC stm)
{
    int k = men[stm + 1];

    int row_k = stm ? ROW(k) : 7 - ROW(k);
    if(row_k > 2)
        return 7;

    int col_k = COL(k);
    if(col_k == 0)
        col_k++;
    else if(col_k == 7)
        col_k--;

    int danger = -4  + 2*pmin[col_k][stm >> 5]
                + pmin[col_k - 1][stm >> 5]
                + pmin[col_k + 1][stm >> 5];

    if(danger >= 5)
    {
        int shft = stm ? 16 : -16;
        if(LIGHT(b[k + shft], stm))
        {
            if(LIGHT(b[k + shft - 1], stm)
            || LIGHT(b[k + shft + 1], stm))
                danger = 1;
            else if(LIGHT(b[k + 2*shft + 1], stm)
            || LIGHT(b[k + 2*shft - 1], stm))
                danger = 3;
        }
        else if(LIGHT(b[k + 2*shft + 1], stm)
        && (LIGHT(b[k + shft + 1], stm)
            || LIGHT(b[k + shft - 1], stm)))
                danger = 4;
    }
    if(danger > 7)
        danger = 7;

    return danger;
}

//-----------------------------
short SimpleKingDist(UC stm)
{
    UC menNum   = stm ^ WHT;
    menNum      = menNxt[menNum];
    UC maxPc    = pieces[!stm];
    UC k        = men[stm + 1];
    unsigned i  = 0;
    short ans   = 0;

    for(i = 0; i < maxPc; ++i)
    {
        UC fr = men[menNum];
        menNum = menNxt[menNum];
        int dst = kingDist[ABSI(k - fr)];
        if(dst > 3)
            continue;

        switch(b[fr] & ~WHT)
        {
        case _q :
            ans += dst <= 2 ? 212 : 212 / 4;
            break;

        case _b :
            ans += dst <= 2 ? 84 : 84/4;
            break;
        case _n :
            ans += dst <= 2 ? 24 : 24/4;
            break;
        }// switch
    }// for i

    return ans;
}

//-----------------------------
int KingAttacks(UC stm)
{
    int ans = 0;
    SC shifts[] = {15, 16, 17};//, 1, -1};//, -15, -16, -17};
    UC k = men[stm + 1];
    for(unsigned i = 0; i < sizeof(shifts); ++i)
    {
        if(!ONBRD(k + shifts[i]))
            continue;
        if(Attack(men[stm + 1], stm ^ WHT))
            ans++;
    }
    return ans;
}
*/

//-----------------------------
short KingShieldFactor(UC stm)
{
    int k = men[stm + 1];
    int shft = stm ? 16 : -16;

    if(!ONBRD(k + shft + shft))
        return 7;

    if(COL(k) == 0)
        k++;
    else if(COL(k) == 7)
        k--;

    int shieldPieces1 = 0, shieldPieces2 = 0;
    int shieldPieces1or2 = 0;
    for(int i = k + shft - 1; i <= k + shft + 1; ++i)
    {
        bool tmp1 = LIGHT(b[i], stm);
        bool tmp2 = LIGHT(b[i + shft], stm);
        if(tmp1)
            shieldPieces1++;
        if(tmp2)
            shieldPieces2++;
        if(tmp1 || tmp2)
            shieldPieces1or2++;
    }

    if(shieldPieces1 == 3)
        return 0;
    else if(shieldPieces1 == 2)
    {
        bool centralPawn1 = LIGHT(b[k + shft], stm);
        if(!centralPawn1)
        {
            bool centralPawn2 = LIGHT(b[k + shft + shft], stm);
            return !centralPawn2 ? 6 : 2;
        }
        else
            return shieldPieces1or2 == 3 ? 1 : 3;
    }
    else if(shieldPieces1 == 1)
    {
        bool centralPawn1 = LIGHT(b[k + shft], stm);
        if(centralPawn1)
            return shieldPieces1or2 >= 2 ? 3 : 5;
        else
        {
            bool centralPawn2 = LIGHT(b[k + shft + shft], stm);
            return !centralPawn2 ? 7 : 3;
        }
    }
    else
    {
        if(shieldPieces1or2 == 3)
            return 3;
        else if(shieldPieces1or2 == 2)
            return 5;
        else
            return 7;
    }
}

//-----------------------------
void KingSafety(UC stm)
{
    if(material[!stm] - pieces[!stm] < 16)
        return;

    short ans = 0;

//    UC k = men[stm + 1];                                                //
//    if(COL(k) == 3 || COL(k) == 4)
//        ans -= 100;

    if(boardState[PREV_STATES + ply].cstl & (0x0C >> (stm >> 4)))       // able to castle
    {
        valOpn += stm ? ans : -ans;
        return;
    }

    int sh  = KingShieldFactor(stm);
    ans +=  (1 - sh)*33;

    valOpn += stm ? ans : -ans;
}

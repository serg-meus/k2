#include "eval.h"

short valOpn, valEnd;

short MatArrOpn[] = {  0, 0, Q_VAL_OPN, R_VAL_OPN, B_VAL_OPN, N_VAL_OPN, P_VAL_OPN};
short MatArrEnd[] = {  0, 0, Q_VAL_END, R_VAL_END, B_VAL_END, N_VAL_END, P_VAL_END};

char  kingDist[120];

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

    boardState[prev_states + ply].valOpn = valOpn;
    boardState[prev_states + ply].valEnd = valEnd;

#ifndef DONT_USE_PAWN_STRUCT
    EvalPawns((bool)white);
    EvalPawns((bool)black);
#endif // DONT_USE_PAWN_STRUCT

    KingSafety(white);
    KingSafety(black);

#ifndef DONT_USE_PAWN_STRUCT
    ClampedRook(white);
    ClampedRook(black);
#endif // DONT_USE_PAWN_STRUCT


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

    valOpn = boardState[prev_states + ply].valOpn;
    valEnd = boardState[prev_states + ply].valEnd;

    return wtm ? (short)(-Y) : (short)Y;
}


//-----------------------------
void FastEval(Move m)
{
    short deltaScoreOpn = 0, deltaScoreEnd = 0;

    int x  = COL(m.to);
    int y  = ROW(m.to);
    int x0 = COL(boardState[prev_states + ply].fr);
    int y0 = ROW(boardState[prev_states + ply].fr);
    UC pt  = b[m.to]/2;

    if(!wtm)
    {
        y  = 7 - y;
        y0 = 7 - y0;
    }

    if(m.flg & mPROM)
    {
        deltaScoreOpn -= MatArrOpn[_p/2] + pst[_p/2 - 1][0][y0][x0];
        deltaScoreEnd -= MatArrEnd[_p/2] + pst[_p/2 - 1][1][y0][x0];
        switch(m.flg & mPROM)
        {
            case mPR_Q :    deltaScoreOpn += MatArrOpn[_q/2] + pst[_q/2 - 1][0][y0][x0];
                            deltaScoreEnd += MatArrEnd[_q/2] + pst[_q/2 - 1][1][y0][x0];
                            break;
            case mPR_R :    deltaScoreOpn += MatArrOpn[_r/2] + pst[_r/2 - 1][0][y0][x0];
                            deltaScoreEnd += MatArrEnd[_r/2] + pst[_r/2 - 1][1][y0][x0];
                            break;
            case mPR_B :    deltaScoreOpn += MatArrOpn[_b/2] + pst[_b/2 - 1][0][y0][x0];
                            deltaScoreEnd += MatArrOpn[_b/2] + pst[_b/2 - 1][1][y0][x0];
                            break;
            case mPR_N :    deltaScoreOpn += MatArrOpn[_n/2] + pst[_n/2 - 1][0][y0][x0];
                            deltaScoreEnd += MatArrOpn[_n/2] + pst[_n/2 - 1][1][y0][x0];
                            break;
        }
    }

    if(m.flg & mCAPT)
    {
        UC capt = boardState[prev_states + ply].capt & ~white;
        if(m.flg & mENPS)
        {
            deltaScoreOpn +=  MatArrOpn[_p/2] + pst[_p/2 - 1][0][7 - y0][x];
            deltaScoreEnd +=  MatArrEnd[_p/2] + pst[_p/2 - 1][1][7 - y0][x];
        }
        else
        {
            deltaScoreOpn +=  MatArrOpn[capt/2] + pst[capt/2 - 1][0][7 - y][x];
            deltaScoreEnd +=  MatArrEnd[capt/2] + pst[capt/2 - 1][1][7 - y][x];
        }
    }
    else if(m.flg & mCS_K)
    {
        deltaScoreOpn += pst[_r/2 - 1][0][7][5] - pst[_r/2 - 1][0][7][7];
        deltaScoreEnd += pst[_r/2 - 1][1][7][5] - pst[_r/2 - 1][1][7][7];
    }
    else if(m.flg & mCS_Q)
    {
        deltaScoreOpn += pst[_r/2 - 1][0][7][3] - pst[_r/2 - 1][0][7][0];
        deltaScoreEnd += pst[_r/2 - 1][1][7][3] - pst[_r/2 - 1][1][7][0];
    }

    deltaScoreOpn += pst[pt - 1][0][y][x] - pst[pt - 1][0][y0][x0];
    deltaScoreEnd += pst[pt - 1][1][y][x] - pst[pt - 1][1][y0][x0];

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
        if(pt & white)
            y0 = 7 - y0;

        short tmpOpn = MatArrOpn[pt/2] + pst[(pt/2) - 1][0][y0][x0];
        short tmpEnd = MatArrEnd[pt/2] + pst[(pt/2) - 1][1][y0][x0];

        if(pt & white)
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
}

//--------------------------------
bool TestPromo(int col, UC stm)
{
    int mx = pmax[col + 1][stm];

    if(mx >= 7 - pmin[col + 1][!stm]
    && mx >= 7 - pmin[col - 0][!stm]
    && mx >= 7 - pmin[col + 2][!stm])
        return true;
    else
        return false;

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

            if(oppHasOnlyPawns)
            {
                if(TestUnstoppable(i, 7 - pmax[i + 1][stm], stm))
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
    UC k = *king_coord[stm];

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
    UC k    = *king_coord[!stm];

    if(y > 5)
        y = 5;
    int psq = XY2SQ(x, stm ? 7 : 0);
    int d = kingDist[ABSI(k - psq)];
    if(COL(*king_coord[stm]) == x
    && kingDist[ABSI(*king_coord[stm] - psq)] <= y)
        y++;
    return d - (stm != wtm) > y;
}

//-----------------------------
short KingShieldFactor(UC stm)
{
    int k = *king_coord[stm];
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

    UC k = *king_coord[stm];                                              //
    if(COL(k) == 3 || COL(k) == 4)
        ans -= 100;


    if(boardState[prev_states + ply].cstl & (0x0C >> 2*stm))       // able to castle
    {
        valOpn += stm ? ans : -ans;
        return;
    }

    int sh  = KingShieldFactor(stm);
    ans +=  (1 - sh)*33;

    valOpn += stm ? ans : -ans;
}

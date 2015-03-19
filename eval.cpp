#include "eval.h"

short val_opn, val_end;

short material_values_opn[] = {  0, 0, Q_VAL_OPN, R_VAL_OPN, B_VAL_OPN, N_VAL_OPN, P_VAL_OPN};
short material_values_end[] = {  0, 0, Q_VAL_END, R_VAL_END, B_VAL_END, N_VAL_END, P_VAL_END};

char  king_dist[120];

#ifdef TUNE_PARAMETERS
    std::vector <float> param;
#endif // TUNE_PARAMETERS

//-----------------------------
void InitEval()
{

    if(king_dist[59] != 5)      // run for the first time
    {
//        float pst_gain[6][2] = {{1.5, 0}, {1, 1}, {1, 1}, {4.63, 0.54}, {2.06, 0.44}, {1.25, 0.35}};
//        float pst_gain[6][2] = {{1.5, 1}, {1, 1}, {1, 1}, {1.5, 1}, {1.5, 1}, {1, 1}};
        float pst_gain[6][2] = {{1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}};
        for(int pc = 0; pc < 6; ++pc)
            for(int clr = 0; clr < 1; ++clr)
                for(int i = 0; i < 8; ++i)
                    for(int j = 0; j < 8; ++j)
                        pst[pc][clr][i][j] *= pst_gain[pc][clr];
    }


    val_opn = 0;
    val_end = 0;
    InitMoveGen();
    for(int i = 0; i < 120; i++)
        if(i & 8)
            king_dist[i] = MAXI(8 - COL(i), ROW(i) + 1);
        else
            king_dist[i] = MAXI(COL(i), ROW(i));

/*    if(param.at(0) != 0 && param.at(1) == 0)
        for(int i = 0; i < 8; ++i)
            for(int j = 0; j < 8; ++j)
            {
                int tmp;
                tmp = (float)pst[_p/2 - 1][ 0][i][j] * param.at(0);
                if(tmp < -128)
                    tmp = -128;
                else if(tmp > 127)
                    tmp = 127;
                pst[_p/2 - 1][0][i][j] = tmp;
            }

    if(param.at(1) != 0 && param.at(0) == 0)
        for(int i = 0; i < 8; ++i)
            for(int j = 0; j < 8; ++j)
            {
                int tmp = (float)pst[_p/2 - 1][1][i][j] * param.at(1);
                if(tmp < -128)
                    tmp = -128;
                else if(tmp > 127)
                    tmp = 127;
                pst[_p/2 - 1][1][i][j] = tmp;

            }
*/
}

//-----------------------------
short Eval(/*short alpha, short beta*/)
{

    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

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


/*#ifndef CHECK_PREDICTED_VALUE
   if(reversible_moves > ply)
    {
        val_opn = (int)val_opn * (FIFTY_MOVES - reversible_moves) / FIFTY_MOVES;
        val_end = (int)val_end * (FIFTY_MOVES - reversible_moves) / FIFTY_MOVES;
    }
#endif
*/

    int X, Y;
    X = material[0] + 1 + material[1] + 1 - pieces[0] - pieces[1];

    if(X == 3 && (material[0] == 4 || material[1] == 4))
    {
        if(pieces[0] + pieces[1] == 3)                                  // KNk, KBk, Kkn, Kkb
        {
            val_opn = 0;
            val_end = 0;
        }
        if(material[1] == 1 && material[0] == 4)                        // KPkn, KPkb
            val_end += B_VAL_END + P_VAL_END/4;
        if(material[0] == 1 && material[1] == 4)                        // KNkp, KBkp
            val_end -= B_VAL_END + P_VAL_END/4;
    }
    if(X == 6)                                                          // KNNk, Kknn
    {
        bool two_knights_white = false, two_knights_black = false;
        if(material[0] == 8 && pieces[0] == 3
        && material[1] == 0)
            two_knights_black = true;
        if(material[1] == 8 && pieces[1] == 3
        && material[0] == 0)
            two_knights_white = true;

        if(two_knights_black || two_knights_white)
        {
            auto rit = coords[two_knights_white ? white : black].rbegin();
            ++rit;
            if((b[*rit] & ~white) == _n
            && (b[*(++rit)] & ~white) == _n)
            {
                val_opn = 0;
                val_end = 0;
            }
        }
    }

    if(HowManyPieces(_B) == 2)
    {
        val_opn += 50;
        val_end += 50;
    }
    if(HowManyPieces(_b) == 2)
    {
        val_opn -= 50;
        val_end -= 50;
    }

    Y = ((val_opn - val_end)*X + 80*val_end)/80;

    val_opn = b_state[prev_states + ply].val_opn;
    val_end = b_state[prev_states + ply].val_end;

    return wtm ? (short)(-Y) : (short)Y;
}

//-----------------------------
void FastEval(Move m)
{
    short delta_opn = 0, delta_end = 0;

    int x  = COL(m.to);
    int y  = ROW(m.to);
    int x0 = COL(b_state[prev_states + ply].fr);
    int y0 = ROW(b_state[prev_states + ply].fr);
    UC pt  = b[m.to]/2;

    if(!wtm)
    {
        y  = 7 - y;
        y0 = 7 - y0;
    }

    if(m.flg & mPROM)
    {
        delta_opn -= material_values_opn[_p/2] + pst[_p/2 - 1][0][y0][x0];
        delta_end -= material_values_end[_p/2] + pst[_p/2 - 1][1][y0][x0];
        switch(m.flg & mPROM)
        {
            case mPR_Q :    delta_opn += material_values_opn[_q/2] + pst[_q/2 - 1][0][y0][x0];
                            delta_end += material_values_end[_q/2] + pst[_q/2 - 1][1][y0][x0];
                            break;
            case mPR_R :    delta_opn += material_values_opn[_r/2] + pst[_r/2 - 1][0][y0][x0];
                            delta_end += material_values_end[_r/2] + pst[_r/2 - 1][1][y0][x0];
                            break;
            case mPR_B :    delta_opn += material_values_opn[_b/2] + pst[_b/2 - 1][0][y0][x0];
                            delta_end += material_values_end[_b/2] + pst[_b/2 - 1][1][y0][x0];
                            break;
            case mPR_N :    delta_opn += material_values_opn[_n/2] + pst[_n/2 - 1][0][y0][x0];
                            delta_end += material_values_end[_n/2] + pst[_n/2 - 1][1][y0][x0];
                            break;
        }
    }

    if(m.flg & mCAPT)
    {
        UC capt = b_state[prev_states + ply].capt & ~white;
        if(m.flg & mENPS)
        {
            delta_opn +=  material_values_opn[_p/2] + pst[_p/2 - 1][0][7 - y0][x];
            delta_end +=  material_values_end[_p/2] + pst[_p/2 - 1][1][7 - y0][x];
        }
        else
        {
            delta_opn +=  material_values_opn[capt/2] + pst[capt/2 - 1][0][7 - y][x];
            delta_end +=  material_values_end[capt/2] + pst[capt/2 - 1][1][7 - y][x];
        }
    }
    else if(m.flg & mCS_K)
    {
        delta_opn += pst[_r/2 - 1][0][7][5] - pst[_r/2 - 1][0][7][7];
        delta_end += pst[_r/2 - 1][1][7][5] - pst[_r/2 - 1][1][7][7];
    }
    else if(m.flg & mCS_Q)
    {
        delta_opn += pst[_r/2 - 1][0][7][3] - pst[_r/2 - 1][0][7][0];
        delta_end += pst[_r/2 - 1][1][7][3] - pst[_r/2 - 1][1][7][0];
    }

    delta_opn += pst[pt - 1][0][y][x] - pst[pt - 1][0][y0][x0];
    delta_end += pst[pt - 1][1][y][x] - pst[pt - 1][1][y0][x0];

    if(wtm)
    {
        val_opn -= delta_opn;
        val_end -= delta_end;
    }
    else
    {
        val_opn += delta_opn;
        val_end += delta_end;
    }
}

//-----------------------------
void EvalAllMaterialAndPST()
{
    val_opn = 0;
    val_end = 0;
    for(unsigned i = 0; i < sizeof(b)/sizeof(*b); ++i)
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

        short tmpOpn = material_values_opn[pt/2] + pst[(pt/2) - 1][0][y0][x0];
        short tmpEnd = material_values_end[pt/2] + pst[(pt/2) - 1][1][y0][x0];

        if(pt & white)
        {
            val_opn += tmpOpn;
            val_end += tmpEnd;
        }
        else
        {
            val_opn -= tmpOpn;
            val_end -= tmpEnd;
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
    bool promo, prev_promo = false;
    bool opp_only_pawns = material[!stm] == pieces[!stm] - 1;

    for(int i = 0; i < 8; i++)
    {
        bool doubled = false, isolany = false/*, backward*/;

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

        if(i > 0 && i < 7
        && mx < pmin[i + 0][stm] && mx < pmin[i + 2][stm])
        {   // pawn holes occupied by enemy pieces
            int y_coord = stm ? mx + 1 : 7 - mx - 1;
            UC op_piece = b[XY2SQ(i, y_coord)];
            bool occupied = DARK(op_piece, stm)
                    && (op_piece & ~white) != _p;
            if(occupied)
                ansO -= 30;
        }

        // test for passer
        promo = false;
        if(mx >= 7 - pmin[i + 1][!stm]
        && mx >= 7 - pmin[i - 0][!stm]
        && mx >= 7 - pmin[i + 2][!stm])
            promo = true;

/*
        backward = false;
        if(mx < 5 && !isolany
        && mx < pmin[i - 0][stm] && mx < pmin[i + 2][stm])
        {
            if(pmax[i + 1][!stm] == 6 - mx)
                backward = true;
            else if(pmax[i - 0][!stm] == 5 - mx)
                backward = true;
            else if(pmax[i + 2][!stm] == 5 - mx)
                backward = true;
        }
        if(backward)
            ansE -= 20;
*/
        short k = *king_coord[stm];
        short opp_k = *king_coord[!stm];
        short pawn_coord = XY2SQ(i, stm ? mx + 1 : 7 - mx - 1);
        short k_dist = king_dist[ABSI(k - pawn_coord)];
        short opp_k_dist = king_dist[ABSI(opp_k - pawn_coord)];

        // king pawn tropism
        if(promo)
        {
            if(k_dist <= 1)
                ansE += 75;
            else if(k_dist == 2)
                ansE += 15;
            if(opp_k_dist <= 1)
                ansE -= 75;
            else if(opp_k_dist == 2)
                ansE -= 15;
        }
/*        else if(backward)
        {
            if(k_dist <= 1)
                ansE += 15;
            else if(k_dist == 2)
                ansE += 3;
            if(opp_k_dist <= 1)
                ansE -= 15;
            else if(opp_k_dist == 2)
                ansE -= 3;
        }
*/
        if(!promo)
        {
            prev_promo = false;
            continue;
        }

        // passer pawn evaluation
        int A, B;
        if(mx < 4)
        {
            A = 21;
            B = -21;
        }
        else
        {
            A = 61;
            B = -158;
        }
        int delta = A*mx + B;
        if(mx == 6 && b[XY2SQ(i, stm ? 7 : 0)] != __)                   // if passer blocked before promotion
            delta = 2*delta/7;

        ansE += delta;
        ansO += delta/3;


        if(promo && prev_promo && ABSI(mx - pmax[i + 0][stm]) <= 1)      // two connected passers
        {
            int mmx = std::max(pmax[i + 0][stm], mx);
            if(mmx > 4)
                ansE += 28*mmx;
        }
        prev_promo = true;

        if(opp_only_pawns)
        {
            if(TestUnstoppable(i, 7 - pmax[i + 1][stm], stm))
            {
                ansO += 120*mx + 350;
                ansE += 120*mx + 350;
            }
        }

    }// for i

    val_opn += stm ? ansO : -ansO;
    val_end += stm ? ansE : -ansE;
}

//-----------------------------
void ClampedRook(UC stm)
{
    UC k = *king_coord[stm];

    if(stm)
    {
        if(k == 0x06 && b[0x07] == _R && pmax[7 + 1][1])
            val_opn -= CLAMPED_R;
        else if(k == 0x05)
        {
            if(pmax[7 + 1][1] && b[0x07] == _R)
                val_opn -= CLAMPED_R;
            else
            if(pmax[6 + 1][1] && b[0x06] == _R)
                val_opn -= CLAMPED_R;
        }
        else if(k == 0x01 && b[0x00] == _R && pmax[0 + 1][1])
            val_opn -= CLAMPED_R;
        else if(k == 0x02)
        {
            if(pmax[0 + 1][1] && b[0x00] == _R)
                val_opn -= CLAMPED_R;
            else
            if(pmax[1 + 1][1] && b[0x01] == _R)
                val_opn -= CLAMPED_R;
        }
     }
     else
     {
        if(k == 0x76 && b[0x77] == _r && pmax[7 + 1][0])
            val_opn += CLAMPED_R;
        else if(k == 0x75)
        {
            if(pmax[7 + 1][0] && b[0x77] == _r)
                val_opn += CLAMPED_R;
            else
            if(pmax[6 + 1][0] && b[0x76] == _r)
                val_opn += CLAMPED_R;
        }
        else if(k == 0x71 && b[0x70] == _r && pmax[0 + 1][0])
            val_opn += CLAMPED_R;
        else if(k == 0x72)
        {
            if(pmax[0 + 1][0] && b[0x70] == _r)
                val_opn += CLAMPED_R;
            else
            if(pmax[1 + 1][0] && b[0x71] == _r)
                val_opn += CLAMPED_R;
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
    int d = king_dist[ABSI(k - psq)];
    if(COL(*king_coord[stm]) == x
    && king_dist[ABSI(*king_coord[stm] - psq)] <= y)
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
    if(material[!stm] - pieces[!stm] < 8)
        return;

    short ans = 0;

    UC k = *king_coord[stm];
    if(COL(k) == 3 || COL(k) == 4)
        ans -= 75;

    int sh  = KingShieldFactor(stm);
    ans +=  material[!stm]*(1 - sh)/3;

    int occ_cr = 0/*, occ_cr_qr = 0*/;
    int pieces_near = 0;
    auto rit = coords[!stm].rbegin();
    ++rit;
    for(; rit != coords[!stm].rend(); ++rit)
    {
        UC pt = b[*rit] & ~white;
        if(pt == _p)
            break;
        int dist = king_dist[ABSI(k - *rit)];
        if(dist >= 4)
            continue;
        pieces_near++;
        if(dist < 3 && pt != _b && pt != _n)
        {
            occ_cr += 2;
        }
        else occ_cr++;
    }
    short tropism = 40*occ_cr*occ_cr;
    if(pieces_near == 1)
        tropism /= 2;
//    if(occ_cr > 1 && occ_cr_qr == 0)
//        tropism /= 2;

    if(b_state[prev_states + ply].cstl & (0x0C >> 2*stm))       // able to castle
     {
        if(pieces_near == 1)
            tropism = 0;
        else
            tropism /= 2;
     }

    ans -= tropism;

    val_opn += stm ? ans : -ans;
}

//-----------------------------
int HowManyPieces(UC pc)
{
    UC stm = pc & white;
    auto rit = coords[stm].rbegin();
    rit++;
    for(; rit != coords[stm].rend(); ++rit)
        if(b[*rit] == pc)
            break;
    if(rit == coords[stm].rend())
        return 0;
    ++rit;
    if(rit == coords[stm].rend())
        return 1;
    if(b[*rit] == pc)
        return 2;
    return 1;
}

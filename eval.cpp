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
    val_opn = 0;
    val_end = 0;
    InitMoveGen();
    for(int i = 0; i < 120; i++)
        if(i & 8)
            king_dist[i] = MAXI(8 - COL(i), ROW(i) + 1);
        else
            king_dist[i] = MAXI(COL(i), ROW(i));
}

//-----------------------------
short ReturnEval(UC stm)
{
    int X, Y;
    X = material[0] + 1 + material[1] + 1 - pieces[0] - pieces[1];

    Y = ((val_opn - val_end)*X + 80*val_end)/80;
    return stm ? (short)(Y) : (short)(-Y);
}

//-----------------------------
short Eval()
{

    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    EvalPawns((bool)white);
    EvalPawns((bool)black);

    KingSafety(white);
    KingSafety(black);

    ClampedRook(white);
    ClampedRook(black);

    ClampedBishop(white);
    ClampedBishop(black);

    MaterialImbalances();

    short ans = -ReturnEval(wtm);
    ans -= 8;                                                           // bonus for side to move

    val_opn = b_state[prev_states + ply].val_opn;
    val_end = b_state[prev_states + ply].val_end;

    return ans;
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
    int mx = pawn_max[col + 1][stm];

    if(mx >= 7 - pawn_min[col + 1][!stm]
    && mx >= 7 - pawn_min[col - 0][!stm]
    && mx >= 7 - pawn_min[col + 2][!stm])
        return true;
    else
        return false;

}

//--------------------------------
void EvalPawns(bool stm)
{
    short ansO = 0, ansE = 0;
    bool promo, prev_promo = false;
    bool opp_only_pawns = material[!stm] == pieces[!stm] - 1;

    for(int i = 0; i < 8; i++)
    {
        bool doubled = false, isolany = false/*, backward*/;

        int mx = pawn_max[i + 1][stm];
        if(mx == 0)
            continue;

        if(pawn_min[i + 1][stm] != mx)
            doubled = true;
        if(pawn_max[i - 0][stm] == 0 && pawn_max[i + 2][stm] == 0)
            isolany = true;
        if(doubled && isolany)
        {
            ansE -= 55;
            ansO -= 15;
        }
        else if(isolany)
        {
            ansE -= 25;
            ansO -= 15;
        }
        else if(doubled)
        {
            ansE -= 4;
            ansO -= 1;
        }

        if(i > 0 && i < 7
        && mx < pawn_min[i + 0][stm] && mx < pawn_min[i + 2][stm])
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
        if(mx >= 7 - pawn_min[i + 1][!stm]
        && mx >= 7 - pawn_min[i - 0][!stm]
        && mx >= 7 - pawn_min[i + 2][!stm])
            promo = true;

/*
        backward = false;
        if(mx < 5 && !isolany
        && mx < pawn_min[i - 0][stm] && mx < pawn_min[i + 2][stm])
        {
            if(pawn_max[i + 1][!stm] == 6 - mx)
                backward = true;
            else if(pawn_max[i - 0][!stm] == 5 - mx)
                backward = true;
            else if(pawn_max[i + 2][!stm] == 5 - mx)
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


        if(promo && prev_promo && ABSI(mx - pawn_max[i + 0][stm]) <= 1)      // two connected passers
        {
            int mmx = std::max(pawn_max[i + 0][stm], mx);
            if(mmx > 4)
                ansE += 28*mmx;
        }
        prev_promo = true;

        if(opp_only_pawns)
        {
            if(TestUnstoppable(i, 7 - pawn_max[i + 1][stm], stm))
            {
                ansO += 120*mx + 350;
                ansE += 120*mx + 350;
            }
        }

    }// for i

    val_opn += stm ? ansO : -ansO;
    val_end += stm ? ansE : -ansE;
}

//------------------------------
void ClampedBishop(UC stm)
{
    if(material[stm] - pieces[stm] < 20)
        return;

    short ans = 0;
    int left_shift = stm ? 15 : -15;
    int right_shift = stm ? 17 : -17;
    bool clamped_left, clamped_right;

    auto rit = coords[stm].rbegin();

    while(rit != coords[stm].rend()
    && (b[*rit] & ~white) != _b)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    UC col = COL(*rit);
    UC lft = b[*rit + left_shift];
    UC rgt = b[*rit + right_shift];

    clamped_left  = (col == 0 || LIGHT(lft, stm));
    clamped_right = (col == 7 || LIGHT(rgt, stm));

    if(clamped_left && clamped_right)
        ans -= 40;
    else if(clamped_left || clamped_right)
    {
        int clmp_crd = *rit + 2*(clamped_left ? right_shift : left_shift);
        if(!ONBRD(clmp_crd))
            ans -= 15;
        else if(b[clmp_crd] == (_p ^ !stm))
            ans -= 40;
        else if(b[clmp_crd] != __)
            ans -= 15;
    }

    ++rit;
    if(rit == coords[stm].rend())
        return;
    if((b[*rit] & ~white) != _b)
        return;

    col = COL(*rit);
    lft = b[*rit + left_shift];
    rgt = b[*rit + right_shift];

    clamped_left  = (col == 0 || LIGHT(lft, stm));
    clamped_right = (col == 7 || LIGHT(rgt, stm));

    if(clamped_left && clamped_right)
        ans -= 40;
    else if(clamped_left || clamped_right)
    {
        int clmp_crd = *rit + 2*(clamped_left ? right_shift : left_shift);
        if(!ONBRD(clmp_crd))
            ans -= 15;
        else if(b[clmp_crd] == (_p ^ !stm))
            ans -= 40;
        else if(b[clmp_crd] != __)
            ans -= 15;
    }

    val_opn += stm ? ans : -ans;
    val_end += stm ? ans : -ans;
}

//-----------------------------
void ClampedRook(UC stm)
{
    UC k = *king_coord[stm];

    if(stm)
    {
        if(k == 0x06 && b[0x07] == _R && pawn_max[7 + 1][1])
            val_opn -= CLAMPED_R;
        else if(k == 0x05)
        {
            if(pawn_max[7 + 1][1] && b[0x07] == _R)
                val_opn -= CLAMPED_R;
            else
            if(pawn_max[6 + 1][1] && b[0x06] == _R)
                val_opn -= CLAMPED_R;
        }
        else if(k == 0x01 && b[0x00] == _R && pawn_max[0 + 1][1])
            val_opn -= CLAMPED_R;
        else if(k == 0x02)
        {
            if(pawn_max[0 + 1][1] && b[0x00] == _R)
                val_opn -= CLAMPED_R;
            else
            if(pawn_max[1 + 1][1] && b[0x01] == _R)
                val_opn -= CLAMPED_R;
        }
     }
     else
     {
        if(k == 0x76 && b[0x77] == _r && pawn_max[7 + 1][0])
            val_opn += CLAMPED_R;
        else if(k == 0x75)
        {
            if(pawn_max[7 + 1][0] && b[0x77] == _r)
                val_opn += CLAMPED_R;
            else
            if(pawn_max[6 + 1][0] && b[0x76] == _r)
                val_opn += CLAMPED_R;
        }
        else if(k == 0x71 && b[0x70] == _r && pawn_max[0 + 1][0])
            val_opn += CLAMPED_R;
        else if(k == 0x72)
        {
            if(pawn_max[0 + 1][0] && b[0x70] == _r)
                val_opn += CLAMPED_R;
            else
            if(pawn_max[1 + 1][0] && b[0x71] == _r)
                val_opn += CLAMPED_R;
        }
     }
}

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
        ans -= 42;

    int sh  = KingShieldFactor(stm);
    ans +=  material[!stm]*(1 - sh)/3;

    int occ_cr = 0;
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
void MaterialImbalances()
{
    int X = material[black] + 1 + material[white] + 1
            - pieces[black] - pieces[white];

    if(X == 3 && (material[black] == 4 || material[white] == 4))
    {
        if(pieces[black] + pieces[white] == 3)                          // KNk, KBk, Kkn, Kkb
        {
            val_opn = 0;
            val_end = 0;
        }
        if(material[white] == 1 && material[black] == 4)                // KPkn, KPkb
            val_end += B_VAL_END + P_VAL_END/4;
        if(material[black] == 1 && material[white] == 4)                // KNkp, KBkp
            val_end -= B_VAL_END + P_VAL_END/4;
    }
    else if(X == 6)                                                     // KNNk, Kknn
    {
        if(quantity[white][_n/2] == 2
        || quantity[black][_n/2] == 2)
        {
            val_opn = 0;
            val_end = 0;
        }
        // many code for mating with only bishop and knight
        else if((quantity[white][_n/2] == 1 && quantity[white][_b/2] == 1)
             || (quantity[black][_n/2] == 1 && quantity[black][_b/2] == 1))
        {
            UC stm = quantity[white][_n/2] == 1 ? 1 : 0;
            auto rit = coords[stm].begin();
            ++rit;
            assert(b[*rit] == (_b & ~white));

            short ans = 0;
            auto ok = *king_coord[!stm];
            if(ok == 0x06 || ok == 0x07 || ok == 0x17
            || ok == 0x70 || ok == 0x71 || ok == 0x60)
                ans = 200;
            if(ok == 0x00 || ok == 0x01 || ok == 0x10
            || ok == 0x77 || ok == 0x76 || ok == 0x67)
                ans = -200;

            bool bishop_on_light_square = ((COL(*rit)) + ROW(*rit)) & 1;
            if(!bishop_on_light_square)
                ans = -ans;
            if(!stm)
                ans = -ans;
            val_end += ans;
        }
    }

    // two bishops
    if(quantity[white][_b/2] == 2)
    {
        val_opn += 50;
        val_end += 50;
    }
    if(quantity[black][_b/2] == 2)
    {
        val_opn -= 50;
        val_end -= 50;
    }

    // pawn absense for both sides
    if(quantity[white][_p/2] == 0 && quantity[black][_p/2] == 0
    && material[white] != 0 && material[black] != 0)
        val_end /= 3;

    // multicoloured bishops
    if(quantity[white][_b/2] == 1 && quantity[black][_b/2] == 1)
    {
        auto w_it = coords[white].rbegin();
        while(w_it != coords[white].rend()
        && b[*w_it] != _B)
            ++w_it;
        assert(w_it != coords[white].rend());

        auto b_it = coords[black].rbegin();
        while(b_it != coords[black].rend()
        && b[*b_it] != _b)
            ++b_it;
        assert(b_it != coords[white].rend());

        int sum_coord_w = COL(*w_it) + ROW(*w_it);
        int sum_coord_b = COL(*b_it) + ROW(*b_it);

        if((sum_coord_w & 1) != (sum_coord_b & 1))
        {
            if(material[white] - pieces[white] == 4 - 2
            && material[black] - pieces[black] == 4 - 2)
                val_end /= 2;
            else
                val_end = val_end*4/5;

        }
    }

}

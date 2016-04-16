#include "eval.h"





short material_values_opn[] = {  0, 0, Q_VAL_OPN, R_VAL_OPN, B_VAL_OPN, N_VAL_OPN, P_VAL_OPN};
short material_values_end[] = {  0, 0, Q_VAL_END, R_VAL_END, B_VAL_END, N_VAL_END, P_VAL_END};

char  king_dist[120];
UC    attack_near_king[240];

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

    KingSafety2(white);
    KingSafety2(black);

    ClampedRook(white);
    ClampedRook(black);

    BishopMobility(white);
    BishopMobility(black);

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

            default :       assert(false);
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
        {
            prev_promo = false;
            continue;
        }

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
//            UC blocker = b[XY2SQ(i, stm ? mx + 1: 7 - mx - 1)];
//            if(DARK(blocker, stm) && (blocker & ~white) != _p)
//                ansE -= 20;
        }
        else if(doubled)
        {
            ansE -= 15;
            ansO -= 5;
        }

        // test for passer
        promo = false;
        if(mx >= 7 - pawn_min[i + 1][!stm]
        && mx >= 7 - pawn_min[i - 0][!stm]
        && mx >= 7 - pawn_min[i + 2][!stm])
            promo = true;


        if(!promo && i > 0 && i < 7
        && mx < pawn_min[i + 0][stm] && mx < pawn_min[i + 2][stm])
        {   // pawn holes occupied by enemy pieces
            int y_coord = stm ? mx + 1 : 7 - mx - 1;
            UC op_piece = b[XY2SQ(i, y_coord)];
            bool occupied = DARK(op_piece, stm)
            && (op_piece & ~white) != _p;
            if(occupied)
                ansO -= 30;
        }


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

        // king pawn tropism
        if(promo)
        {
            short k = *king_coord[stm];
            short opp_k = *king_coord[!stm];
            short pawn_coord = XY2SQ(i, stm ? mx + 1 : 7 - mx - 1);
            short k_dist = king_dist[ABSI(k - pawn_coord)];
            short opp_k_dist = king_dist[ABSI(opp_k - pawn_coord)];

            if(k_dist <= 1)
                ansE += 65;
            else if(k_dist == 2)
                ansE += 15;
            if(opp_k_dist <= 1)
                ansE -= 65;
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
        int pass[] =         {0, 0, 21, 40, 85, 150, 200};
        int blocked_pass[] = {0, 0, 10, 20, 40,  50,  60};
        bool blocked = b[XY2SQ(i, stm ? mx + 1 : 7 - mx - 1)] != __;
        int delta = blocked ? blocked_pass[mx] : pass[mx];

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
void BishopMobility(UC stm)
{
    short nonlin[] = {0, 8, 12, 15, 18, 20, 23, 25, 27, 28, 30, 32, 34, 35, 37, 38};
//    if(material[stm] - pieces[stm] < 20)
//        return;

    short ans = 0, cr;

    auto rit = coords[stm].rbegin();

    while(rit != coords[stm].rend()
    && (b[*rit] & ~white) != _b)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    UC at;
    cr = 0;
    for(UC ray = 0; ray < 4; ++ray)
    {
        at = *rit;
        for(UC i = 0; i < 8; ++i)
        {
            at += shifts[_b/2][ray];
            if(!ONBRD(at) || b[at]/2 == _p/2)
                break;
            cr++;
        }
    }
    ans += 2*nonlin[cr] - 20;

    ++rit;
    if(rit == coords[stm].rend() || (b[*rit] & ~white) != _b)
{
        val_opn += stm ? ans : -ans;
        val_end += stm ? ans : -ans;
        return;
}

    cr = 0;
    for(UC ray = 0; ray < 4; ++ray)
    {
        at = *rit;
        for(UC i = 0; i < 8; ++i)
        {
            at += shifts[_b/2][ray];
            if(!ONBRD(at) || b[at]/2 == _p/2)
                break;
            cr++;
        }
    }
    ans += 2*nonlin[cr] - 20;


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

    short ans = 0;
    auto rit = coords[stm].rbegin();

    while(rit != coords[stm].rend()
    && (b[*rit] & ~white) != _r)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    UC rook_on_7th_cr = 0;
    if((stm && ROW(*rit) >= 6) || (!stm && ROW(*rit) <= 1))
        rook_on_7th_cr++;
    if(quantity[stm][_p/2] >= 4
    && pawn_max[COL(*rit) + 1][!stm] == 0)
        ans += (pawn_max[COL(*rit) + 1][stm] == 0 ? 54 : 10);
    ++rit;
    if(rit != coords[stm].rend() && (b[*rit] & ~white) == _r)
    {
        if((stm && ROW(*rit) >= 6) || (!stm && ROW(*rit) <= 1))
            rook_on_7th_cr++;
        if(quantity[stm][_p/2] >= 4
        && pawn_max[COL(*rit) + 1][!stm] == 0)
            ans += (pawn_max[COL(*rit) + 1][stm] == 0 ? 54 : 10);
    }
    ans += rook_on_7th_cr*47;
    val_opn += (stm ? ans : -ans);

}





//-----------------------------
bool TestUnstoppable(int x, int y, UC stm)
{
    UC k = *king_coord[!stm];

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
    short ans = 0;

    if(!ONBRD(k + shft + shft))
        ans = 7;

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
        ans =  0;
    else if(shieldPieces1 == 2)
    {
        bool centralPawn1 = LIGHT(b[k + shft], stm);
        if(!centralPawn1)
        {
            bool centralPawn2 = LIGHT(b[k + shft + shft], stm);
            ans =  !centralPawn2 ? 6 : 2;
        }
        else
            ans =  shieldPieces1or2 == 3 ? 1 : 3;
    }
    else if(shieldPieces1 == 1)
    {
        bool centralPawn1 = LIGHT(b[k + shft], stm);
        if(centralPawn1)
            ans = shieldPieces1or2 >= 2 ? 3 : 5;
        else
        {
            bool centralPawn2 = LIGHT(b[k + shft + shft], stm);
            ans = !centralPawn2 ? 7 : 3;
        }
    }
    else
    {
        if(shieldPieces1or2 == 3)
            ans = 3;
        else if(shieldPieces1or2 == 2)
            ans = 5;
        else
            ans = 7;
    }

    return ans;
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
void KingSafety2(UC king_color)
{
    if(material[!king_color] - pieces[!king_color] < 8)
        return;

    short ans = 0;

    UC k = *king_coord[king_color];
    if(COL(k) == 3 || COL(k) == 4)
        ans -= 42;

    int shield_badness  = KingShieldFactor(king_color);
    ans +=  material[!king_color]*(1 - shield_badness)/3;

    static UC influence_factor[2][7] =
    {
        {0, 0, 10, 10, 10, 10, 0},
        {0, 0, 20, 20, 10, 10, 0}
    };


    int occ_cr = 0;
    int pieces_near = 0;
    auto rit = coords[!king_color].rbegin();
    ++rit;
    for(; rit != coords[!king_color].rend(); ++rit)
    {
        UC pt = b[*rit] & ~white;
        if(pt == _p)
            break;
        int dist = king_dist[ABSI(k - *rit)];
        if(dist >= 4)
            continue;
        pieces_near++;
        occ_cr += influence_factor[dist < 3][pt/2];
    }
    int tropism = occ_cr*occ_cr*(4 + shield_badness)/50;

    if(pieces_near == 1)
        tropism /= 2;

    if(b_state[prev_states + ply].cstl & (0x0C >> 2*king_color))       // able to castle
     {
        if(pieces_near == 1)
            tropism = 0;
        else
            tropism /= 2;
     }

    ans -= tropism;

    val_opn += king_color ? ans : -ans;
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
    else if(X == 6 && (material[0] == 0 || material[1] == 0))           // KNNk, KBNk, KBBk, etc
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
            for(; rit != coords[stm].end(); ++rit)
                if((b[*rit] & ~white) == _b)
                    break;
            assert((b[*rit] & ~white) == _b);

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
        val_opn += 30;
        val_end += 30;
    }
    if(quantity[black][_b/2] == 2)
    {
        val_opn -= 30;
        val_end -= 30;
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





//-----------------------------
short EvalDebug()
{
    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    short store_vo, store_ve, store_sum;

    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);
    std::cout << "\t\t\tMidgame\tEndgame\tTotal" << std::endl;
    std::cout << "Material + PST\t\t";
    std::cout << val_opn << '\t' << val_end << '\t'
              << store_sum << std::endl;

    EvalPawns((bool)white);
    std::cout << "White pawns\t\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);
    EvalPawns((bool)black);
    std::cout << "Black pawns\t\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);

    KingSafety2(white);
    std::cout << "King safety white\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);
    KingSafety2(black);
    std::cout << "King safety black\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);

    ClampedRook(white);
    std::cout << "White rooks\t\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);
    ClampedRook(black);
    std::cout << "Black rooks\t\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);

    BishopMobility(white);
    std::cout << "White bishops\t\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);
    BishopMobility(black);
    std::cout << "Black bishops\t\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);

    MaterialImbalances();
    std::cout << "Imbalances summary\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);

    short ans = -ReturnEval(wtm);
    ans -= 8;                                                           // bonus for side to move
    std::cout << "Bonus for side to move\t\t\t";
    std::cout <<  (wtm ? 8 : -8) << std::endl << std::endl;

    val_opn = b_state[prev_states + ply].val_opn;
    val_end = b_state[prev_states + ply].val_end;


    int attacks, weight, attackers;

    std::cout << "White king safety data: " << std::endl;
    std::cout << "King weakness: " << KingWeakness(white);
    weight = 0;
    attackers = 0;
    attacks = CountAttacksOnKingShelter(white, weight, attackers);
    std::cout << ", attacks: " << attacks << ", weight: "
              << weight << ", attackers: " << attackers << std::endl;

    std::cout << "Black king safety data: " << std::endl;
    std::cout << "King weakness: " << KingWeakness(black);
    weight = 0;
    attackers = 0;
    attacks = CountAttacksOnKingShelter(black, weight, attackers);
    std::cout << ", attacks: " << attacks << ", weight: "
              << weight << ", attackers: " << attackers << std::endl;

    std::cout << std::endl << std::endl;

    std::cout << "Eval summary: " << (wtm ? -ans : ans) << std::endl;
    std::cout << "(positive values means advantage for white)" << std::endl;

    return ans;
}





//-----------------------------
short KingWeakness(UC king_color)
{
    short ans = 0;
    int k = *king_coord[king_color];
    int shft = king_color ? 16 : -16;

    if(COL(k) == 0)
        k++;
    else if(COL(k) == 7)
        k--;

    if(COL(k) == 2 || COL(k) == 5)
        ans += 30;
    if(COL(k) == 3 || COL(k) == 4)
    {
        if(b_state[prev_states + ply].cstl & (0x0C >> 2*king_color))
            ans += 30;
        else
            ans += 90;
    }
    if((king_color == white && ROW(k) > 1)
    || (king_color == black && ROW(k) < 6))
    {
        ans += 60;
//        return ans;
    }


    int index = 0;
    for(int i = 0; i < 3; ++i)
    {
        UC pt1 = b[i + k + shft - 1];
        UC pt2 = b[i + k + 2*shft - 1];
        if(pt1 == (_p | king_color) || pt2 == (_p | king_color))
            continue;
        if(pt1 != __ && (pt1 & white) == king_color
        && pt2 != __ && (pt2 & white) == king_color)  // LIGHT() macros needed
            continue;

            index += (1 << i);
    }
    index = 7 - index;
    // cases: ___, __p, _p_, _pp, p__, p_p, pp_, ppp
    int cases[]  = {0, 1, 1, 3, 1, 2, 3, 4};
    short scores[] = {140, 75, 75, 10, 0};

    if(cases[index] == 2)
    {
        if(LIGHT(b[k + shft], king_color))
            ans -= 45;
        else if(b[k + 2*shft] == (_p | king_color))
            ans -= 45;
    }
    ans += scores[cases[index]];

    return ans;
}





//-----------------------------
int CountAttacksOnOneSquare(UC king_color, UC attacked_coord, int &weight,
                            UC *attacker_coords, UC &attacker_cr)
{
    int ans = 0;
    UC shifts[] = {15, 16, 17};

    for(size_t i = 0; i < sizeof(shifts)/sizeof(*shifts); ++i)
    {
        UC to = attacked_coord;
        for(int j = 0; j < 7; ++j)
        {
            to += (king_color ? shifts[i] : -shifts[i]);
            if(!ONBRD(to))
                break;
            else if(b[to]/2 == _p/2)
                break;
            else if(b[to] == __)
                continue;
            else if(LIGHT(b[to], king_color))
                break;



            if(b[to]/2 == _q/2)
            {
                ans++;
                weight += 60;
                attacker_coords[attacker_cr++] = to;
            }
            else if(b[to]/2 == _r/2)
            {
                if(shifts[i] == 16)
                {
                    ans++;
                    weight += 60;
                    attacker_coords[attacker_cr++] = to;
                }
                else
                    break;
            }
            else if(b[to]/2 == _b/2)
            {
                if(shifts[i] != 16)
                {
                    ans++;
                    weight += 30;
                    attacker_coords[attacker_cr++] = to;
                }
                else
                    break;
            }
            else if(b[to]/2 == _n/2)
                break;
            else if(b[to]/2 == _k/2)
                break;

        }// for j
    }// for i

    return ans;
}





//-----------------------------
int CountAttacksOnKingShelter(UC king_color, int &weight, int &attackers)
{
    int ans = 0;
    attackers = 0;
    int k_shifts[] = {15, 16, 17, 31, 32, 33};
    UC attacker_coords[32];
    UC attacker_cr = 0;

    UC k = *king_coord[king_color];
    if(COL(k) == 0)
        k++;
    else if(COL(k) == 7)
        k--;


    for(size_t i = 0; i < sizeof(k_shifts)/sizeof(*k_shifts); ++i)
    {
        UC pt = k + (king_color ? k_shifts[i] : -k_shifts[i]);
        if(!ONBRD(pt))
            continue;
        if(k_shifts[i] > 17 && !LIGHT(b[pt], king_color))
            continue;
        if(DARK(b[pt], king_color))
            continue;

        ans += CountAttacksOnOneSquare(king_color, pt, weight,
                                       attacker_coords, attacker_cr);
    }

    for(UC i = 0; i < attacker_cr; ++i)
        for(UC j = 0; j < attacker_cr - i - 1; ++j)
            if(attacker_coords[j + 1] > attacker_coords[j])
                std::swap(attacker_coords[j + 1], attacker_coords[j]);
    if(attacker_cr > 0)
        attackers = 1;
    for(UC i = 0; i < attacker_cr - 1; ++i)
        if(attacker_coords[i + 1] != attacker_coords[i])
            attackers++;


    return ans;
}


//-----------------------------
void KingSafety3(UC king_color)
{
    if(quantity[!king_color][_q/2] == 0)
        return;

    short ans = 0;

    int king_weakness = KingWeakness(king_color);
    if(king_weakness <= 70)
    {
        ans -= king_weakness;
        val_opn += king_color ? ans : -ans;
        return;
    }
//    king_weakness = king_weakness*(material[!king_color] + 24)*ans/72;

    int weight = 0, attackers = 0;
    int attacks = CountAttacksOnKingShelter(king_color,
                                            weight, attackers);

    int attack_val = 3*attackers*weight/2;

    if(attackers == 1)
        attack_val = 0;
    else if(attackers == 2)
        attack_val /= 2;
    if(attack_val > 250)
        attack_val = 250;

    ans -= king_weakness/2 + attack_val;

//    ans = (material[!king_color] + 24)*ans/72;

    val_opn += king_color ? ans : -ans;
}

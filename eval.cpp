#include "eval.h"




short val_opn, val_end;

UC pawn_max[10][2], pawn_min[10][2];

short material_values_opn[] = {  0, 0, Q_VAL_OPN, R_VAL_OPN, B_VAL_OPN, N_VAL_OPN, P_VAL_OPN};
short material_values_end[] = {  0, 0, Q_VAL_END, R_VAL_END, B_VAL_END, N_VAL_END, P_VAL_END};

char  king_dist[120];
short king_tropism[2];

UC tropism_factor[2][7] =
{   //  k  Q   R   B   N   P
    {0, 0, 10, 10, 10,  4, 4},  // 4 >= dist > 3
    {0, 0, 21, 21, 10,  0, 10}  // dist < 3
};

#ifdef TUNE_PARAMETERS
    std::vector <float> param;
#endif // TUNE_PARAMETERS





//-----------------------------
void InitEval()
{
    InitChess();

    val_opn = 0;
    val_end = 0;

    for(int i = 0; i < 120; i++)
        if(i & 8)
            king_dist[i] = MAXI(8 - COL(i), ROW(i) + 1);
        else
            king_dist[i] = MAXI(COL(i), ROW(i));

    InitPawnStruct();

    b_state[prev_states + ply].val_opn = 0;
    b_state[prev_states + ply].val_end = 0;

    pawn_max[1 - 1][0] = 0;
    pawn_max[1 - 1][1] = 0;
    pawn_min[1 - 1][0] = 7;
    pawn_min[1 - 1][1] = 7;
    pawn_max[8 + 1][0]  = 0;
    pawn_max[8 + 1][1]  = 0;
    pawn_min[8 + 1][0]  = 7;
    pawn_min[8 + 1][1]  = 7;
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

    KingSafety3(white);
    KingSafety3(black);

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
    UC pt  = GET_INDEX(b[m.to]);

    if(!wtm)
    {
        y  = 7 - y;
        y0 = 7 - y0;
    }

    if(m.flg & mPROM)
    {
        delta_opn -= material_values_opn[GET_INDEX(_p)]
                    + pst[GET_INDEX(_p) - 1][0][y0][x0];
        delta_end -= material_values_end[GET_INDEX(_p)]
                    + pst[GET_INDEX(_p) - 1][1][y0][x0];
        switch(m.flg & mPROM)
        {
            case mPR_Q :    delta_opn += material_values_opn[GET_INDEX(_q)]
                                        + pst[GET_INDEX(_q) - 1][0][y0][x0];
                            delta_end += material_values_end[GET_INDEX(_q)]
                                        + pst[GET_INDEX(_q) - 1][1][y0][x0];
                            break;
            case mPR_R :    delta_opn += material_values_opn[GET_INDEX(_r)]
                                        + pst[GET_INDEX(_r) - 1][0][y0][x0];
                            delta_end += material_values_end[GET_INDEX(_r)]
                                        + pst[GET_INDEX(_r) - 1][1][y0][x0];
                            break;
            case mPR_B :    delta_opn += material_values_opn[GET_INDEX(_b)]
                                        + pst[GET_INDEX(_b) - 1][0][y0][x0];
                            delta_end += material_values_end[GET_INDEX(_b)]
                                        + pst[GET_INDEX(_b) - 1][1][y0][x0];
                            break;
            case mPR_N :    delta_opn += material_values_opn[GET_INDEX(_n)]
                                        + pst[GET_INDEX(_n) - 1][0][y0][x0];
                            delta_end += material_values_end[GET_INDEX(_n)]
                                        + pst[GET_INDEX(_n) - 1][1][y0][x0];
                            break;

            default :       assert(false);
                            break;
        }
    }

    if(m.flg & mCAPT)
    {
        UC capt = TO_BLACK(b_state[prev_states + ply].capt);
        if(m.flg & mENPS)
        {
            delta_opn +=  material_values_opn[GET_INDEX(_p)]
                        + pst[GET_INDEX(_p) - 1][0][7 - y0][x];
            delta_end +=  material_values_end[GET_INDEX(_p)]
                        + pst[GET_INDEX(_p) - 1][1][7 - y0][x];
        }
        else
        {
            delta_opn +=  material_values_opn[GET_INDEX(capt)]
                        + pst[GET_INDEX(capt) - 1][0][7 - y][x];
            delta_end +=  material_values_end[GET_INDEX(capt)]
                        + pst[GET_INDEX(capt) - 1][1][7 - y][x];
        }
    }
    else if(m.flg & mCS_K)
    {
        delta_opn += pst[GET_INDEX(_r) - 1][0][7][5]
                    - pst[GET_INDEX(_r) - 1][0][7][7];
        delta_end += pst[GET_INDEX(_r) - 1][1][7][5]
                    - pst[GET_INDEX(_r) - 1][1][7][7];
    }
    else if(m.flg & mCS_Q)
    {
        delta_opn += pst[GET_INDEX(_r) - 1][0][7][3]
                    - pst[GET_INDEX(_r) - 1][0][7][0];
        delta_end += pst[GET_INDEX(_r) - 1][1][7][3]
                    - pst[GET_INDEX(_r) - 1][1][7][0];
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

        short tmpOpn = material_values_opn[GET_INDEX(pt)]
                        + pst[(GET_INDEX(pt)) - 1][0][y0][x0];
        short tmpEnd = material_values_end[GET_INDEX(pt)]
                        + pst[(GET_INDEX(pt)) - 1][1][y0][x0];

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
    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    king_tropism[white] = CountKingTropism(white);
    king_tropism[black] = CountKingTropism(black);
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
//            if(DARK(blocker, stm) && TO_BLACK(blocker) != _p)
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
            && TO_BLACK(op_piece) != _p;
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
            int mmx = std::max((int)pawn_max[i + 0][stm], mx);
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
    && TO_BLACK(b[*rit]) != _b)
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
            at += shifts[GET_INDEX(_b)][ray];
            if(!ONBRD(at) || TO_BLACK(b[at]) == _p)
                break;
            cr++;
        }
    }
    ans += 2*nonlin[cr] - 20;

    ++rit;
    if(rit == coords[stm].rend() || TO_BLACK(b[*rit]) != _b)
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
            at += shifts[GET_INDEX(_b)][ray];
            if(!ONBRD(at) || TO_BLACK(b[at]) == _p)
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
    && TO_BLACK(b[*rit]) != _r)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    UC rook_on_7th_cr = 0;
    if((stm && ROW(*rit) >= 6) || (!stm && ROW(*rit) <= 1))
        rook_on_7th_cr++;
    if(quantity[stm][GET_INDEX(_p)] >= 4
    && pawn_max[COL(*rit) + 1][stm] == 0)
        ans += (pawn_max[COL(*rit) + 1][!stm] == 0 ? 54 : 10);
    ++rit;
    if(rit != coords[stm].rend() && TO_BLACK(b[*rit]) == _r)
    {
        if((stm && ROW(*rit) >= 6) || (!stm && ROW(*rit) <= 1))
            rook_on_7th_cr++;
        if(quantity[stm][GET_INDEX(_p)] >= 4
        && pawn_max[COL(*rit) + 1][stm] == 0)
            ans += (pawn_max[COL(*rit) + 1][!stm] == 0 ? 54 : 10);
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
        if(quantity[white][GET_INDEX(_n)] == 2
        || quantity[black][GET_INDEX(_n)] == 2)
        {
            val_opn = 0;
            val_end = 0;
        }
        // many code for mating with only bishop and knight
        else if((quantity[white][GET_INDEX(_n)] == 1
                 && quantity[white][GET_INDEX(_b)] == 1)
             || (quantity[black][GET_INDEX(_n)] == 1
                 && quantity[black][GET_INDEX(_b)] == 1))
        {
            UC stm = quantity[white][GET_INDEX(_n)] == 1 ? 1 : 0;
            auto rit = coords[stm].begin();
            for(; rit != coords[stm].end(); ++rit)
                if(TO_BLACK(b[*rit]) == _b)
                    break;
            assert(TO_BLACK(b[*rit]) == _b);

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
    else if(X == 3)
    {
        // KBPk or KNPk with pawn at a(h) file
        if(material[white] == 0)
        {
            UC k = *king_coord[white];
            if((pawn_max[1 + 0][black] != 0 && king_dist[ABSI(k - 0x00)] <= 1)
            || (pawn_max[1 + 7][black] != 0 && king_dist[ABSI(k - 0x07)] <= 1))
                val_end += 750;
        }
        else if(material[black] == 0)
        {
            UC k = *king_coord[black];
            if((pawn_max[1 + 0][white] != 0 && king_dist[ABSI(k - 0x70)] <= 1)
            || (pawn_max[1 + 7][white] != 0 && king_dist[ABSI(k - 0x77)] <= 1))
                val_end -= 750;
        }
    }
    else if(X == 0)
    {
        // KPk
        if(material[white] + material[black] == 1)
        {
            UC stm = material[white] == 1;
            auto it = coords[stm].rbegin();
            ++it;
            UC colp = COL(*it);
            bool unstop = TestUnstoppable(colp, 7 - pawn_max[colp + 1][stm], stm);
            int dist_k = king_dist[ABSI(*king_coord[stm] - *it)];
            int dist_opp_k = king_dist[ABSI(*king_coord[!stm] - *it)];

            if(!unstop && dist_k > dist_opp_k + (wtm == stm))
            {
                val_opn = 0;
                val_end = 0;
            }
            else if((colp == 0 || colp == 7)
            && king_dist[ABSI(*king_coord[!stm] -
                              (colp + (stm ? 0x70 : 0)))] <= 1)
            {
                val_opn = 0;
                val_end = 0;
            }
        }
    }

    // two bishops
    if(quantity[white][GET_INDEX(_b)] == 2)
    {
        val_opn += 30;
        val_end += 30;
    }
    if(quantity[black][GET_INDEX(_b)] == 2)
    {
        val_opn -= 30;
        val_end -= 30;
    }

    // pawn absense for both sides
    if(quantity[white][GET_INDEX(_p)] == 0
       && quantity[black][GET_INDEX(_p)] == 0
    && material[white] != 0 && material[black] != 0)
        val_end /= 3;

    // multicoloured bishops
    if(quantity[white][GET_INDEX(_b)] == 1
    && quantity[black][GET_INDEX(_b)] == 1)
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

    KingSafety3(white);
    std::cout << "King safety white\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);
    KingSafety3(black);
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

    std::cout << std::endl << std::endl;

    std::cout << "Eval summary: " << (wtm ? -ans : ans) << std::endl;
    std::cout << "(positive values means advantage for white)" << std::endl;

    val_opn = b_state[prev_states + ply].val_opn;
    val_end = b_state[prev_states + ply].val_end;

    return ans;
}





//-----------------------------
void SetPawnStruct(int col)
{
    assert(col >= 0 && col <= 7);
    int y;
    if(wtm)
    {
        y = 1;
        while(b[XY2SQ(col, 7 - y)] != _p && y < 7)
            y++;
        pawn_min[col + 1][black] = y;

        y = 6;
        while(b[XY2SQ(col, 7 - y)] != _p && y > 0)
            y--;
        pawn_max[col + 1][black] = y;
    }
    else
    {
        y = 1;
        while(b[XY2SQ(col, y)] != _P && y < 7)
            y++;
        pawn_min[col + 1][white] = y;

        y = 6;
        while(b[XY2SQ(col, y)] != _P && y > 0)
            y--;
        pawn_max[col + 1][white] = y;
    }
}





//-----------------------------
void MovePawnStruct(UC movedPiece, UC fr, Move m)
{
    if(TO_BLACK(movedPiece) == _p || (m.flg & mPROM))
    {
        SetPawnStruct(COL(m.to));
        if(m.flg)
            SetPawnStruct(COL(fr));
    }
    if(TO_BLACK(b_state[prev_states + ply].capt) == _p
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
        pawn_max[x + 1][0] = 0;
        pawn_max[x + 1][1] = 0;
        pawn_min[x + 1][0] = 7;
        pawn_min[x + 1][1] = 7;
        for(y = 1; y < 7; y++)
            if(b[XY2SQ(x, y)] == _P)
            {
                pawn_min[x + 1][1] = y;
                break;
            }
        for(y = 6; y >= 1; y--)
            if(b[XY2SQ(x, y)] == _P)
            {
                pawn_max[x + 1][1] = y;
                break;
            }
        for(y = 6; y >= 1; y--)
            if(b[XY2SQ(x, y)] == _p)
            {
                pawn_min[x + 1][0] = 7 - y;
                break;
            }
         for(y = 1; y < 7; y++)
            if(b[XY2SQ(x, y)] == _p)
            {
                pawn_max[x + 1][0] = 7 - y;
                break;
            }
    }
}




//-----------------------------
short CountKingTropism(UC king_color)
{
    short occ_cr = 0;
    auto rit = coords[!king_color].rbegin();
    ++rit;
    for(; rit != coords[!king_color].rend(); ++rit)
    {
        UC pt = TO_BLACK(b[*rit]);
        if(pt == _p)
            break;
        int dist = king_dist[ABSI(*king_coord[king_color] - *rit)];
        if(dist >= 4)
            continue;
        occ_cr += tropism_factor[dist < 3][GET_INDEX(pt)];
    }
    return occ_cr;
}





//-----------------------------
void MoveKingTropism(UC fr, Move m, UC king_color)
{
    b_state[prev_states + ply].tropism[black] = king_tropism[black];
    b_state[prev_states + ply].tropism[white] = king_tropism[white];

    UC pt = b[m.to];

    if(TO_BLACK(pt) == _k)
    {
        king_tropism[king_color]  = CountKingTropism(king_color);
        king_tropism[!king_color] = CountKingTropism(!king_color);

        return;
    }

    int dist_to = king_dist[ABSI(*king_coord[king_color] - m.to)];
    int dist_fr = king_dist[ABSI(*king_coord[king_color] - fr)];


    if(dist_fr < 4 && !(m.flg & mPROM))
        king_tropism[king_color] -= tropism_factor[dist_fr < 3]
                                                  [GET_INDEX(pt)];
    if(dist_to < 4)
        king_tropism[king_color] += tropism_factor[dist_to < 3]
                                                  [GET_INDEX(pt)];

    UC cap = b_state[prev_states + ply].capt;
    if(m.flg & mCAPT)
    {
        dist_to = king_dist[ABSI(*king_coord[!king_color] - m.to)];
        if(dist_to < 4)
            king_tropism[!king_color] -= tropism_factor[dist_to < 3]
                                                       [GET_INDEX(cap)];
    }

#ifndef NDEBUG
    short tmp = CountKingTropism(king_color);
    if(king_tropism[king_color] != tmp && TO_BLACK(cap) != _k)
        ply = ply;
    tmp = CountKingTropism(!king_color);
    if(king_tropism[!king_color] != tmp && TO_BLACK(cap) != _k)
        ply = ply;
#endif // NDEBUG
}





//-----------------------------
bool MkMoveAndEval(Move m)
{
    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    bool is_special_move = MkMoveFast(m);

    UC fr = b_state[prev_states + ply].fr;

    MoveKingTropism(fr, m, wtm);

    MovePawnStruct(b[m.to], fr, m);

    return is_special_move;
}





//-----------------------------
void UnMoveAndEval(Move m)
{
    king_tropism[black] = b_state[prev_states + ply].tropism[black];
    king_tropism[white] = b_state[prev_states + ply].tropism[white];

    UC fr = b_state[prev_states + ply].fr;

    UnMoveFast(m);

    ply++;
    wtm ^= white;
    MovePawnStruct(b[fr], fr, m);
    wtm ^= white;
    ply--;

    val_opn = b_state[prev_states + ply].val_opn;
    val_end = b_state[prev_states + ply].val_end;
}





//-----------------------------
void MkEvalAfterFastMove(Move m)
{
    b_state[prev_states + ply - 1].val_opn = val_opn;
    b_state[prev_states + ply - 1].val_end = val_end;

    UC fr = b_state[prev_states + ply].fr;

    MoveKingTropism(fr, m, wtm);

    MovePawnStruct(b[m.to], fr, m);
}





//-----------------------------
short KingOpenFiles(UC king_color)
{
    short ans = 0;
    int k = *king_coord[king_color];

    if(COL(k) == 0)
        k++;
    else if(COL(k) == 7)
        k--;

    int open_files_near_king = 0, open_files[3] = {0};
    for(int i = 0; i < 3; ++i)
    {
        int col = COL(k) + i - 1;
        if(col < 0 || col > 7)
            continue;
        if(/*pawn_max[col + 1][king_color] == 0
        && */pawn_max[col + 1][!king_color] == 0)
        {
            open_files_near_king++;
            open_files[i]++;
        }
    }

    if(open_files_near_king == 0)
        return 0;

    UC rooks_queens_on_file[8] = {0};
    auto rit = coords[!king_color].rbegin();
    ++rit;
    for(; rit != coords[!king_color].rend(); ++rit)
    {
        UC pt = b[*rit];
        if(TO_BLACK(pt) != _q && TO_BLACK(pt) != _r)
            break;
        int k = pawn_max[COL(*rit) + 1][king_color] ? 1 : 2;
        rooks_queens_on_file[COL(*rit)] +=
                k*(TO_BLACK(pt) == _r ? 2 : 1);
    }

    for(int i = 0; i < 3; ++i)
    {
        if(open_files[i])
            ans += rooks_queens_on_file[COL(k) + i - 1];
    }
    if(ans <= 2)
        ans = ans/2;

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
        if(b_state[prev_states + ply].cstl & (0x0C >> 2*king_color)) // able to castle
            ans += 30;
        else
            ans += 60;
    }
    if((king_color == white && ROW(k) > 1)
    || (king_color == black && ROW(k) < 6))
    {
        ans += 60;
    }


    int index = 0;
    for(int i = 0; i < 3; ++i)
    {
        if(i + k + 2*shft - 1 < 0
        || i + k + 2*shft - 1 >= (int)(sizeof(b)/sizeof(*b)))
            continue;
        UC pt1 = b[i + k + shft - 1];
        UC pt2 = b[i + k + 2*shft - 1];
        const UC pwn = _p | king_color;
        if(pt1 == pwn || pt2 == pwn)
            continue;

        if(LIGHT(pt1, king_color)
        && LIGHT(pt2, king_color))
            continue;

        if(i + k + shft - 2 < 0)
            continue;

        if(LIGHT(pt2, king_color)
        && (b[i + k + shft - 2] == pwn || b[i + k + shft + 0] == pwn))
            continue;

        index += (1 << i);
    }
    index = 7 - index;
    // cases: ___, __p, _p_, _pp, p__, p_p, pp_, ppp
    int cases[]  = {0, 1, 1, 3, 1, 2, 3, 4};
    short scores[] = {140, 75, 75, 10, 0};

    ans += scores[cases[index]];

    if(cases[index] != 4)
        ans += 25*KingOpenFiles(king_color);

    return ans;
}





//-----------------------------
void KingSafety3(UC king_color)
{
    int trp = king_tropism[king_color];

    if(quantity[!king_color][GET_INDEX(_q)] == 0)
    {
        trp += trp*trp/15;
        val_opn += king_color ? -trp : trp;

        return;
    }

    if(trp == 21 || trp == 10)
        trp /= 4;
    else if(trp >= 60)
        trp *= 4;
    trp += trp*trp/5;

    short kw = KingWeakness(king_color);
    if(trp <= 6)
        kw /= 2;
    else if(kw >= 40)
        trp *= 2;

    if(trp > 500)
        trp = 500;

    if(b_state[prev_states + ply].cstl & (0x0C >> 2*king_color)) // able to castle
        kw /= 4;
    else
        kw = (material[!king_color] + 24)*kw/72;

    int ans = -3*(kw + trp)/2;

    val_opn += king_color ? ans : -ans;

}


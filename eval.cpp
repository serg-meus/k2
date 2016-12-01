#include "eval.h"




score_t val_opn, val_end;

rank_t pawn_max[10][2], pawn_min[10][2];

score_t material_values_opn[] = {  0, 0, Q_VAL_OPN, R_VAL_OPN, B_VAL_OPN, N_VAL_OPN, P_VAL_OPN};
score_t material_values_end[] = {  0, 0, Q_VAL_END, R_VAL_END, B_VAL_END, N_VAL_END, P_VAL_END};

dist_t king_dist[120];
tropism_t king_tropism[2];

tropism_t tropism_factor[2][7] =
{   //  k  Q   R   B   N   P
    {0, 0, 10, 10, 10,  4, 4},  // 4 >= dist > 3
    {0, 0, 21, 21, 10,  0, 10}  // dist < 3
};

#ifdef TUNE_PARAMETERS
    std::vector <float> param;
#endif // TUNE_PARAMETERS





//-----------------------------
score_t Eval()
{
    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    EvalPawns(white);
    EvalPawns(black);

    KingSafety(white);
    KingSafety(black);

    ClampedRook(white);
    ClampedRook(black);

    BishopMobility(white);
    BishopMobility(black);

    MaterialImbalances();

    score_t ans = -ReturnEval(wtm);
    ans -= 8;                                                           // bonus for side to move

    val_opn = b_state[prev_states + ply].val_opn;
    val_end = b_state[prev_states + ply].val_end;

    return ans;
}





//-----------------------------
void InitEval()
{
    InitChess();

    val_opn = 0;
    val_end = 0;

    for(size_t i = 0; i < 120; i++)
        if(i & 8)
            king_dist[i] = std::max(8 - COL(i), ROW(i) + 1);
        else
            king_dist[i] = std::max(COL(i), ROW(i));

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
score_t ReturnEval(side_to_move_t stm)
{
    i32 X, Y;
    X = material[0] + 1 + material[1] + 1 - pieces[0] - pieces[1];

    Y = ((val_opn - val_end)*X + 80*val_end)/80;
    return stm ? (score_t)(Y) : (score_t)(-Y);
}





//-----------------------------
void FastEval(move_c m)
{
    score_t ansO = 0, ansE = 0;

    coord_t x  = COL(m.to);
    coord_t y  = ROW(m.to);
    coord_t x0 = COL(b_state[prev_states + ply].fr);
    coord_t y0 = ROW(b_state[prev_states + ply].fr);
    piece_index_t pt  = GET_INDEX(b[m.to]);

    if(!wtm)
    {
        y  = 7 - y;
        y0 = 7 - y0;
    }

    piece_index_t idx;
    move_flag_t flag = m.flg & mPROM;
    if(flag)
    {
        idx = GET_INDEX(_p);
        ansO -= material_values_opn[idx] + pst[idx - 1][0][y0][x0];
        ansE -= material_values_end[idx] + pst[idx - 1][1][y0][x0];

        piece_t promo_pieces[] = {_q, _n, _r, _b};
        idx = GET_INDEX(promo_pieces[flag - 1]);
        ansO += material_values_opn[idx] + pst[idx - 1][0][y0][x0];
        ansE += material_values_end[idx] + pst[idx - 1][1][y0][x0];

    }

    if(m.flg & mCAPT)
    {
        piece_t capt = TO_BLACK(b_state[prev_states + ply].capt);
        if(m.flg & mENPS)
        {
            idx = GET_INDEX(_p);
            ansO += material_values_opn[idx] + pst[idx - 1][0][7 - y0][x];
            ansE += material_values_end[idx] + pst[idx - 1][1][7 - y0][x];
        }
        else
        {
            idx = GET_INDEX(capt);
            ansO += material_values_opn[idx] + pst[idx - 1][0][7 - y][x];
            ansE += material_values_end[idx] + pst[idx - 1][1][7 - y][x];
        }
    }
    else if(m.flg & mCS_K)
    {
        idx = GET_INDEX(_r);
        ansO += pst[idx - 1][0][7][5] - pst[idx - 1][0][7][7];
        ansE += pst[idx - 1][1][7][5] - pst[idx - 1][1][7][7];
    }
    else if(m.flg & mCS_Q)
    {
        idx = GET_INDEX(_r);
        ansO += pst[idx - 1][0][7][3] - pst[idx - 1][0][7][0];
        ansE += pst[idx - 1][1][7][3] - pst[idx - 1][1][7][0];
    }

    ansO += pst[pt - 1][0][y][x] - pst[pt - 1][0][y0][x0];
    ansE += pst[pt - 1][1][y][x] - pst[pt - 1][1][y0][x0];

    val_opn += wtm ? -ansO : ansO;
    val_end += wtm ? -ansE : ansE;
}





//-----------------------------
void InitEvaOfMaterialAndPst()
{
    val_opn = 0;
    val_end = 0;
    for(size_t col = 0; col < sizeof(b)/sizeof(*b); ++col)
    {
        if(!ONBRD(col))
            continue;
        piece_t pt = b[col];
        if(pt == __)
            continue;
        coord_t x0 = COL(col);
        coord_t y0 = ROW(col);
        if(pt & white)
            y0 = 7 - y0;

        piece_index_t idx = GET_INDEX(pt);
        score_t tmpOpn = material_values_opn[idx] + pst[idx - 1][0][y0][x0];
        score_t tmpEnd = material_values_end[idx] + pst[idx - 1][1][y0][x0];

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
bool IsPasser(coord_t col, side_to_move_t stm)
{
    rank_t mx = pawn_max[col + 1][stm];

    if(mx >= 7 - pawn_min[col + 1][!stm]
    && mx >= 7 - pawn_min[col - 0][!stm]
    && mx >= 7 - pawn_min[col + 2][!stm])
        return true;
    else
        return false;

}





//--------------------------------
void EvalPawns(side_to_move_t stm)
{
    score_t ansO = 0, ansE = 0;
    bool passer, prev_passer = false;
    bool opp_only_pawns = material[!stm] == pieces[!stm] - 1;

    for(coord_t col = 0; col < 8; col++)
    {
        bool doubled = false, isolany = false;

        rank_t mx = pawn_max[col + 1][stm];
        if(mx == 0)
        {
            prev_passer = false;
            continue;
        }

        if(pawn_min[col + 1][stm] != mx)
            doubled = true;
        if(pawn_max[col - 0][stm] == 0 && pawn_max[col + 2][stm] == 0)
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
            ansE -= 15;
            ansO -= 5;
        }

        passer = IsPasser(col, stm);
        if(!passer)
        {
            // pawn holes occupied by enemy pieces
            if(col > 0 && col < 7 && mx < pawn_min[col + 0][stm]
            && mx < pawn_min[col + 2][stm])
            {
                coord_t y_coord = stm ? mx + 1 : 7 - mx - 1;
                piece_t op_piece = b[XY2SQ(col, y_coord)];
                bool occupied = DARK(op_piece, stm)
                && TO_BLACK(op_piece) != _p;
                if(occupied)
                    ansO -= 30;
            }

            // gaps in pawn structure
            if(pawn_max[col + 0][stm]
            && std::abs(mx - pawn_max[col + 0][stm]) > 1)
                ansE -= 33;

            prev_passer = false;
            continue;
        }
        // following code executed only for passers

        // king pawn tropism
        coord_t k = *king_coord[stm];
        coord_t opp_k = *king_coord[!stm];
        coord_t pawn_coord = XY2SQ(col, stm ? mx + 1 : 7 - mx - 1);
        dist_t k_dist = king_dist[std::abs(k - pawn_coord)];
        dist_t opp_k_dist = king_dist[std::abs(opp_k - pawn_coord)];

        if(k_dist <= 1)
            ansE += 30 + 10*mx;
        else if(k_dist == 2)
            ansE += 15;
        if(opp_k_dist <= 1)
            ansE -= 30 + 10*mx;
        else if(opp_k_dist == 2)
            ansE -= 15;

        // passed pawn evaluation
        score_t pass[] =         {0, 0, 21, 40, 85, 150, 200};
        score_t blocked_pass[] = {0, 0, 10, 20, 40,  50,  60};
        bool blocked = b[XY2SQ(col, stm ? mx + 1 : 7 - mx - 1)] != __;
        score_t delta = blocked ? blocked_pass[mx] : pass[mx];

        ansE += delta;
        ansO += delta/3;

        // connected passers
        if(passer && prev_passer && std::abs(mx - pawn_max[col + 0][stm]) <= 1)
        {
            rank_t mmx = std::max(pawn_max[col + 0][stm], mx);
            if(mmx > 4)
                ansE += 28*mmx;
        }
        prev_passer = true;

        // unstoppable
        if(opp_only_pawns
        && IsUnstoppablePawn(col, 7 - pawn_max[col + 1][stm], stm))
        {
            ansO += 120*mx + 350;
            ansE += 120*mx + 350;
        }

    }// for col

    val_opn += stm ? ansO : -ansO;
    val_end += stm ? ansE : -ansE;
}





//------------------------------
score_t OneBishopMobility(coord_t b_coord)
{
    score_t ans;
    score_t empty_squares = 0;
    for(size_t ray = 0; ray < 4; ++ray)
    {
        coord_t coord = b_coord;
        for(size_t col = 0; col < 8; ++col)
        {
            coord += shifts[GET_INDEX(_b)][ray];
            if(!ONBRD(coord) || TO_BLACK(b[coord]) == _p)
                break;
            empty_squares++;
        }
    }
    ans = -empty_squares*empty_squares/6 + 6*empty_squares - 16;
    return ans;
}





//------------------------------
void BishopMobility(side_to_move_t stm)
{
    score_t ans = 0;

    auto rit = coords[stm].rbegin();

    while(rit != coords[stm].rend()
    && TO_BLACK(b[*rit]) != _b)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    ans += OneBishopMobility(*rit);

    ++rit;
    if(rit != coords[stm].rend() && TO_BLACK(b[*rit]) == _b)
        ans += OneBishopMobility(*rit);

    val_opn += stm ? ans : -ans;
    val_end += stm ? ans : -ans;
}





//-----------------------------
void ClampedRook(side_to_move_t stm)
{
    coord_t k = *king_coord[stm];

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

    score_t ans = 0;
    auto rit = coords[stm].rbegin();

    while(rit != coords[stm].rend()
    && TO_BLACK(b[*rit]) != _r)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    size_t rook_on_7th_cr = 0;
    if((stm && ROW(*rit) >= 6) || (!stm && ROW(*rit) <= 1))
        rook_on_7th_cr++;
    if(quantity[stm][GET_INDEX(_p)] >= 4
    && pawn_max[COL(*rit) + 1][stm] == 0)
        ans += (pawn_max[COL(*rit) + 1][!stm] == 0 ? 54 : 22);

    size_t empty_sq = 0;
    for(coord_t i = 1; i <= 2; ++i)
    {
        if(COL(*rit) + i < 7 && b[*rit + i] == __)
            empty_sq++;
        if(COL(*rit) - i > 0 && b[*rit - i] == __)
            empty_sq++;
    }
    if(empty_sq <= 1)
        ans -= 15;

    ++rit;
    if(rit != coords[stm].rend() && TO_BLACK(b[*rit]) == _r)
    {
        if((stm && ROW(*rit) >= 6) || (!stm && ROW(*rit) <= 1))
            rook_on_7th_cr++;
        if(quantity[stm][GET_INDEX(_p)] >= 4
        && pawn_max[COL(*rit) + 1][stm] == 0)
            ans += (pawn_max[COL(*rit) + 1][!stm] == 0 ? 54 : 22);

        empty_sq = 0;
        for(coord_t i = 1; i <= 2; ++i)
        {
            if(COL(*rit) + i < 7 && b[*rit + i] == __)
                empty_sq++;
            if(COL(*rit) - i > 0 && b[*rit - i] == __)
                empty_sq++;
        }
        if(empty_sq <= 1)
            ans -= 15;
    }
    ans += rook_on_7th_cr*47;
    val_opn += (stm ? ans : -ans);

}





//-----------------------------
bool IsUnstoppablePawn(coord_t col, coord_t row, side_to_move_t stm)
{
    coord_t k = *king_coord[!stm];

    if(row > 5)
        row = 5;
    coord_t psq = XY2SQ(col, stm ? 7 : 0);
    dist_t d = king_dist[std::abs(k - psq)];
    if(COL(*king_coord[stm]) == col
    && king_dist[std::abs(*king_coord[stm] - psq)] <= row)
        row++;
    return (score_t)d - (stm != wtm) > row;
}





//-----------------------------
void MaterialImbalances()
{
    score_t X = material[black] + 1 + material[white] + 1
            - pieces[black] - pieces[white];

    if(X == 3 && (material[black] == 4 || material[white] == 4))
    {
        // KNk, KBk, Kkn, Kkb
        if(pieces[black] + pieces[white] == 3)
        {
            val_opn = 0;
            val_end = 0;
            return;
        }
        // KPkn, KPkb
        if(material[white] == 1 && material[black] == 4)
            val_end += B_VAL_END + P_VAL_END/4;
        // KNkp, KBkp
        if(material[black] == 1 && material[white] == 4)
            val_end -= B_VAL_END + P_VAL_END/4;
    }
    // KNNk, KBNk, KBBk, etc
    else if(X == 6 && (material[0] == 0 || material[1] == 0))
    {
        if(quantity[white][GET_INDEX(_n)] == 2
        || quantity[black][GET_INDEX(_n)] == 2)
        {
            val_opn = 0;
            val_end = 0;
            return;
        }
        // many code for mating with only bishop and knight
        else if((quantity[white][GET_INDEX(_n)] == 1
                 && quantity[white][GET_INDEX(_b)] == 1)
             || (quantity[black][GET_INDEX(_n)] == 1
                 && quantity[black][GET_INDEX(_b)] == 1))
        {
            side_to_move_t stm = quantity[white][GET_INDEX(_n)] == 1 ? 1 : 0;
            auto rit = coords[stm].begin();
            for(; rit != coords[stm].end(); ++rit)
                if(TO_BLACK(b[*rit]) == _b)
                    break;
            assert(TO_BLACK(b[*rit]) == _b);

            score_t ans = 0;
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
            coord_t k = *king_coord[white];
            if((pawn_max[1 + 0][black] != 0 && king_dist[std::abs(k - 0x00)] <= 1)
            || (pawn_max[1 + 7][black] != 0 && king_dist[std::abs(k - 0x07)] <= 1))
                val_end += 750;
        }
        else if(material[black] == 0)
        {
            coord_t k = *king_coord[black];
            if((pawn_max[1 + 0][white] != 0 && king_dist[std::abs(k - 0x70)] <= 1)
            || (pawn_max[1 + 7][white] != 0 && king_dist[std::abs(k - 0x77)] <= 1))
                val_end -= 750;
        }
    }
    else if(X == 0)
    {
        // KPk
        if(material[white] + material[black] == 1)
        {
            side_to_move_t stm = material[white] == 1;
            auto it = coords[stm].rbegin();
            ++it;
            coord_t colp = COL(*it);
            bool unstop = IsUnstoppablePawn(colp, 7 - pawn_max[colp + 1][stm], stm);
            dist_t dist_k = king_dist[std::abs(*king_coord[stm] - *it)];
            dist_t dist_opp_k = king_dist[std::abs(*king_coord[!stm] - *it)];

            if(!unstop && dist_k > dist_opp_k + (wtm == stm))
            {
                val_opn = 0;
                val_end = 0;
            }
            else if((colp == 0 || colp == 7)
            && king_dist[std::abs(*king_coord[!stm] -
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

    // pawn absence for both sides
    if(quantity[white][GET_INDEX(_p)] == 0
       && quantity[black][GET_INDEX(_p)] == 0
    && material[white] != 0 && material[black] != 0)
        val_end /= 3;

    // multicolored bishops
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

        coord_t sum_coord_w = COL(*w_it) + ROW(*w_it);
        coord_t sum_coord_b = COL(*b_it) + ROW(*b_it);

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
score_t EvalDebug()
{
    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    score_t store_vo, store_ve, store_sum;

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

    KingSafety(white);
    std::cout << "King safety white\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);
    KingSafety(black);
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

    score_t ans = -ReturnEval(wtm);
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
void SetPawnStruct(coord_t col)
{
    assert(col <= 7);
    coord_t y;
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
void MovePawnStruct(piece_t moved_piece, coord_t from_coord, move_c move)
{
    if(TO_BLACK(moved_piece) == _p || (move.flg & mPROM))
    {
        SetPawnStruct(COL(move.to));
        if(move.flg)
            SetPawnStruct(COL(from_coord));
    }
    if(TO_BLACK(b_state[prev_states + ply].capt) == _p
    || (move.flg & mENPS))                                    // mENPS not needed
    {
        wtm ^= white;
        SetPawnStruct(COL(move.to));
        wtm ^= white;
    }
}





//-----------------------------
void InitPawnStruct()
{
    coord_t x, y;
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
score_t CountKingTropism(side_to_move_t king_color)
{
    score_t occ_cr = 0;
    auto rit = coords[!king_color].rbegin();
    ++rit;
    for(; rit != coords[!king_color].rend(); ++rit)
    {
        piece_t cur_piece = TO_BLACK(b[*rit]);
        if(cur_piece == _p)
            break;
        dist_t dist = king_dist[std::abs(*king_coord[king_color] - *rit)];
        if(dist >= 4)
            continue;
        occ_cr += tropism_factor[dist < 3][GET_INDEX(cur_piece)];
    }
    return occ_cr;
}





//-----------------------------
void MoveKingTropism(coord_t from_coord, move_c move, side_to_move_t king_color)
{
    b_state[prev_states + ply].tropism[black] = king_tropism[black];
    b_state[prev_states + ply].tropism[white] = king_tropism[white];

    piece_t cur_piece = b[move.to];

    if(TO_BLACK(cur_piece) == _k)
    {
        king_tropism[king_color]  = CountKingTropism(king_color);
        king_tropism[!king_color] = CountKingTropism(!king_color);

        return;
    }

    dist_t dist_to = king_dist[std::abs(*king_coord[king_color] - move.to)];
    dist_t dist_fr = king_dist[std::abs(*king_coord[king_color] - from_coord)];


    if(dist_fr < 4 && !(move.flg & mPROM))
        king_tropism[king_color] -= tropism_factor[dist_fr < 3]
                                                  [GET_INDEX(cur_piece)];
    if(dist_to < 4)
        king_tropism[king_color] += tropism_factor[dist_to < 3]
                                                  [GET_INDEX(cur_piece)];

    piece_t cap = b_state[prev_states + ply].capt;
    if(move.flg & mCAPT)
    {
        dist_to = king_dist[std::abs(*king_coord[!king_color] - move.to)];
        if(dist_to < 4)
            king_tropism[!king_color] -= tropism_factor[dist_to < 3]
                                                       [GET_INDEX(cap)];
    }

#ifndef NDEBUG
    score_t tmp = CountKingTropism(king_color);
    if(king_tropism[king_color] != tmp && TO_BLACK(cap) != _k)
        ply = ply;
    tmp = CountKingTropism(!king_color);
    if(king_tropism[!king_color] != tmp && TO_BLACK(cap) != _k)
        ply = ply;
#endif // NDEBUG
}





//-----------------------------
bool MkMoveAndEval(move_c m)
{
    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    bool is_special_move = MkMoveFast(m);

    coord_t fr = b_state[prev_states + ply].fr;

    MoveKingTropism(fr, m, wtm);

    MovePawnStruct(b[m.to], fr, m);

    return is_special_move;
}





//-----------------------------
void UnMoveAndEval(move_c m)
{
    king_tropism[black] = b_state[prev_states + ply].tropism[black];
    king_tropism[white] = b_state[prev_states + ply].tropism[white];

    coord_t fr = b_state[prev_states + ply].fr;

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
void MkEvalAfterFastMove(move_c m)
{
    b_state[prev_states + ply - 1].val_opn = val_opn;
    b_state[prev_states + ply - 1].val_end = val_end;

    coord_t fr = b_state[prev_states + ply].fr;

    MoveKingTropism(fr, m, wtm);

    MovePawnStruct(b[m.to], fr, m);
}





//-----------------------------
score_t KingOpenFiles(side_to_move_t king_color)
{
    score_t ans = 0;
    coord_t k = *king_coord[king_color];

    if(COL(k) == 0)
        k++;
    else if(COL(k) == 7)
        k--;

    size_t open_files_near_king = 0, open_files[3] = {0};
    for(coord_t i = 0; i < 3; ++i)
    {
        shifts_t col = COL(k) + i - 1;
        if(col < 0 || col > 7)
            continue;
        if(pawn_max[col + 1][!king_color] == 0)
        {
            open_files_near_king++;
            open_files[i]++;
        }
    }

    if(open_files_near_king == 0)
        return 0;

    size_t rooks_queens_on_file[8] = {0};
    auto rit = coords[!king_color].rbegin();
    ++rit;
    for(; rit != coords[!king_color].rend(); ++rit)
    {
        piece_t pt = b[*rit];
        if(TO_BLACK(pt) != _q && TO_BLACK(pt) != _r)
            break;
        rank_t k = pawn_max[COL(*rit) + 1][king_color] ? 1 : 2;
        rooks_queens_on_file[COL(*rit)] +=
                k*(TO_BLACK(pt) == _r ? 2 : 1);
    }

    for(size_t i = 0; i < 3; ++i)
    {
        if(open_files[i])
            ans += rooks_queens_on_file[COL(k) + i - 1];
    }
    if(ans <= 2)
        ans = ans/2;

    return ans;
}





//-----------------------------
score_t KingWeakness(side_to_move_t king_color)
{
    score_t ans = 0;
    coord_t k = *king_coord[king_color];
    shifts_t shft = king_color ? 16 : -16;

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


    score_t idx = 0;
    for(score_t col = 0; col < 3; ++col)
    {
        if(col + k + 2*shft - 1 < 0
        || col + k + 2*shft - 1 >= (score_t)(sizeof(b)/sizeof(*b)))
            continue;
        piece_t pt1 = b[col + k + shft - 1];
        piece_t pt2 = b[col + k + 2*shft - 1];
        const piece_t pwn = _p | king_color;
        if(pt1 == pwn || pt2 == pwn)
            continue;

        if(LIGHT(pt1, king_color)
        && LIGHT(pt2, king_color))
            continue;

        if(col + k + shft - 2 < 0)
            continue;

        if(LIGHT(pt2, king_color)
        && (b[col + k + shft - 2] == pwn || b[col + k + shft + 0] == pwn))
            continue;

        idx += (1 << col);
    }
    idx = 7 - idx;
    // cases: ___, __p, _p_, _pp, p__, p_p, pp_, ppp
    size_t cases[]  = {0, 1, 1, 3, 1, 2, 3, 4};
    score_t scores[] = {140, 75, 75, 10, 0};

    ans += scores[cases[idx]];

    if(cases[idx] != 4)
        ans += 25*KingOpenFiles(king_color);

    return ans;
}





//-----------------------------
void KingSafety(side_to_move_t king_color)
{
    tropism_t trp = king_tropism[king_color];

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

    if(material[!king_color] - pieces[!king_color] < 24)
        trp /= 2;

    score_t kw = KingWeakness(king_color);
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

    score_t ans = -3*(kw + trp)/2;

    val_opn += king_color ? ans : -ans;

}


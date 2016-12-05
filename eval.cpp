#include "eval.h"




score_t val_opn, val_end;

rank_t pawn_max[10][2], pawn_min[10][2];

score_t material_values_opn[] = {  0, 0, queen_val_opn, rook_val_opn, bishop_val_opn, kinght_val_opn, pawn_val_opn};
score_t material_values_end[] = {  0, 0, queen_val_end, rook_val_end, bishop_val_end, kinght_val_end, pawn_val_end};

dist_t king_dist[120];
tropism_t king_tropism[2];

tropism_t tropism_factor[2][7] =
{   //  k  Q   R   B   N   P
    {0, 0, 10, 10, 10,  4, 4},  // 4 >= dist > 3
    {0, 0, 21, 21, 10,  0, 10}  // dist < 3
};


//std::vector <float> param;





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

    auto ans = -ReturnEval(wtm);
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

    for(auto i = 0; i < 120; i++)
        if(i & 8)
            king_dist[i] = std::max(8 - get_col(i), get_row(i) + 1);
        else
            king_dist[i] = std::max(get_col(i), get_row(i));

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

    auto x  = get_col(m.to);
    auto y  = get_row(m.to);
    auto x0 = get_col(b_state[prev_states + ply].fr);
    auto y0 = get_row(b_state[prev_states + ply].fr);
    auto pt = get_index(b[m.to]);

    if(!wtm)
    {
        y  = 7 - y;
        y0 = 7 - y0;
    }

    piece_index_t idx;
    auto flag = m.flg & mPROM;
    if(flag)
    {
        idx = get_index(_p);
        ansO -= material_values_opn[idx] + pst[idx - 1][0][y0][x0];
        ansE -= material_values_end[idx] + pst[idx - 1][1][y0][x0];

        piece_t promo_pieces[] = {_q, _n, _r, _b};
        idx = get_index(promo_pieces[flag - 1]);
        ansO += material_values_opn[idx] + pst[idx - 1][0][y0][x0];
        ansE += material_values_end[idx] + pst[idx - 1][1][y0][x0];

    }

    if(m.flg & mCAPT)
    {
        auto capt = to_black(b_state[prev_states + ply].capt);
        if(m.flg & mENPS)
        {
            idx = get_index(_p);
            ansO += material_values_opn[idx] + pst[idx - 1][0][7 - y0][x];
            ansE += material_values_end[idx] + pst[idx - 1][1][7 - y0][x];
        }
        else
        {
            idx = get_index(capt);
            ansO += material_values_opn[idx] + pst[idx - 1][0][7 - y][x];
            ansE += material_values_end[idx] + pst[idx - 1][1][7 - y][x];
        }
    }
    else if(m.flg & mCS_K)
    {
        idx = get_index(_r);
        ansO += pst[idx - 1][0][7][5] - pst[idx - 1][0][7][7];
        ansE += pst[idx - 1][1][7][5] - pst[idx - 1][1][7][7];
    }
    else if(m.flg & mCS_Q)
    {
        idx = get_index(_r);
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
        if(!within_board(col))
            continue;
        auto pt = b[col];
        if(pt == __)
            continue;
        auto x0 = get_col(col);
        auto y0 = get_row(col);
        if(pt & white)
            y0 = 7 - y0;

        auto idx = get_index(pt);
        auto tmpOpn = material_values_opn[idx] + pst[idx - 1][0][y0][x0];
        auto tmpEnd = material_values_end[idx] + pst[idx - 1][1][y0][x0];

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
    auto mx = pawn_max[col + 1][stm];

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

    for(auto col = 0; col < 8; col++)
    {
        bool doubled = false, isolany = false;

        auto mx = pawn_max[col + 1][stm];
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
                auto y_coord = stm ? mx + 1 : 7 - mx - 1;
                auto op_piece = b[get_coord(col, y_coord)];
                bool occupied = is_dark(op_piece, stm)
                && to_black(op_piece) != _p;
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
        auto k = *king_coord[stm];
        auto opp_k = *king_coord[!stm];
        auto pawn_coord = get_coord(col, stm ? mx + 1 : 7 - mx - 1);
        auto k_dist = king_dist[std::abs(k - pawn_coord)];
        auto opp_k_dist = king_dist[std::abs(opp_k - pawn_coord)];

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
        bool blocked = b[get_coord(col, stm ? mx + 1 : 7 - mx - 1)] != __;
        auto delta = blocked ? blocked_pass[mx] : pass[mx];

        ansE += delta;
        ansO += delta/3;

        // connected passers
        if(passer && prev_passer && std::abs(mx - pawn_max[col + 0][stm]) <= 1)
        {
            auto mmx = std::max(pawn_max[col + 0][stm], mx);
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
    auto ans = 0;
    auto empty_squares = 0;
    for(auto ray = 0; ray < 4; ++ray)
    {
        auto coord = b_coord;
        for(auto col = 0; col < 8; ++col)
        {
            coord += shifts[get_index(_b)][ray];
            if(!within_board(coord) || to_black(b[coord]) == _p)
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
    && to_black(b[*rit]) != _b)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    ans += OneBishopMobility(*rit);

    ++rit;
    if(rit != coords[stm].rend() && to_black(b[*rit]) == _b)
        ans += OneBishopMobility(*rit);

    val_opn += stm ? ans : -ans;
    val_end += stm ? ans : -ans;
}





//-----------------------------
void ClampedRook(side_to_move_t stm)
{
    const score_t clamped_rook  = 82;
    auto k = *king_coord[stm];

    if(stm)
    {
        if(k == 0x06 && b[0x07] == _R && pawn_max[7 + 1][1])
            val_opn -= clamped_rook;
        else if(k == 0x05)
        {
            if(pawn_max[7 + 1][1] && b[0x07] == _R)
                val_opn -= clamped_rook;
            else
            if(pawn_max[6 + 1][1] && b[0x06] == _R)
                val_opn -= clamped_rook;
        }
        else if(k == 0x01 && b[0x00] == _R && pawn_max[0 + 1][1])
            val_opn -= clamped_rook;
        else if(k == 0x02)
        {
            if(pawn_max[0 + 1][1] && b[0x00] == _R)
                val_opn -= clamped_rook;
            else
            if(pawn_max[1 + 1][1] && b[0x01] == _R)
                val_opn -= clamped_rook;
        }
     }
     else
     {
        if(k == 0x76 && b[0x77] == _r && pawn_max[7 + 1][0])
            val_opn += clamped_rook;
        else if(k == 0x75)
        {
            if(pawn_max[7 + 1][0] && b[0x77] == _r)
                val_opn += clamped_rook;
            else
            if(pawn_max[6 + 1][0] && b[0x76] == _r)
                val_opn += clamped_rook;
        }
        else if(k == 0x71 && b[0x70] == _r && pawn_max[0 + 1][0])
            val_opn += clamped_rook;
        else if(k == 0x72)
        {
            if(pawn_max[0 + 1][0] && b[0x70] == _r)
                val_opn += clamped_rook;
            else
            if(pawn_max[1 + 1][0] && b[0x71] == _r)
                val_opn += clamped_rook;
        }
    }

    score_t ans = 0;
    auto rit = coords[stm].rbegin();

    while(rit != coords[stm].rend()
    && to_black(b[*rit]) != _r)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    auto rook_on_7th_cr = 0;
    if((stm && get_row(*rit) >= 6) || (!stm && get_row(*rit) <= 1))
        rook_on_7th_cr++;
    if(quantity[stm][get_index(_p)] >= 4
    && pawn_max[get_col(*rit) + 1][stm] == 0)
        ans += (pawn_max[get_col(*rit) + 1][!stm] == 0 ? 54 : 22);

    auto empty_sq = 0;
    for(auto i = 1; i <= 2; ++i)
    {
        if(get_col(*rit) + i < 7 && b[*rit + i] == __)
            empty_sq++;
        if(get_col(*rit) - i > 0 && b[*rit - i] == __)
            empty_sq++;
    }
    if(empty_sq <= 1)
        ans -= 15;

    ++rit;
    if(rit != coords[stm].rend() && to_black(b[*rit]) == _r)
    {
        if((stm && get_row(*rit) >= 6) || (!stm && get_row(*rit) <= 1))
            rook_on_7th_cr++;
        if(quantity[stm][get_index(_p)] >= 4
        && pawn_max[get_col(*rit) + 1][stm] == 0)
            ans += (pawn_max[get_col(*rit) + 1][!stm] == 0 ? 54 : 22);

        empty_sq = 0;
        for(coord_t i = 1; i <= 2; ++i)
        {
            if(get_col(*rit) + i < 7 && b[*rit + i] == __)
                empty_sq++;
            if(get_col(*rit) - i > 0 && b[*rit - i] == __)
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
    auto k = *king_coord[!stm];

    if(row > 5)
        row = 5;
    auto psq = get_coord(col, stm ? 7 : 0);
    auto d = king_dist[std::abs(k - psq)];
    if(get_col(*king_coord[stm]) == col
    && king_dist[std::abs(*king_coord[stm] - psq)] <= row)
        row++;
    return (score_t)d - (stm != wtm) > row;
}





//-----------------------------
void MaterialImbalances()
{
    auto X = material[black] + 1 + material[white] + 1
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
            val_end += bishop_val_end + pawn_val_end/4;
        // KNkp, KBkp
        if(material[black] == 1 && material[white] == 4)
            val_end -= bishop_val_end + pawn_val_end/4;
    }
    // KNNk, KBNk, KBBk, etc
    else if(X == 6 && (material[0] == 0 || material[1] == 0))
    {
        if(quantity[white][get_index(_n)] == 2
        || quantity[black][get_index(_n)] == 2)
        {
            val_opn = 0;
            val_end = 0;
            return;
        }
        // many code for mating with only bishop and knight
        else if((quantity[white][get_index(_n)] == 1
                 && quantity[white][get_index(_b)] == 1)
             || (quantity[black][get_index(_n)] == 1
                 && quantity[black][get_index(_b)] == 1))
        {
            auto stm = quantity[white][get_index(_n)] == 1 ? white : black;
            auto rit = coords[stm].begin();
            for(; rit != coords[stm].end(); ++rit)
                if(to_black(b[*rit]) == _b)
                    break;
            assert(to_black(b[*rit]) == _b);

            score_t ans = 0;
            auto ok = *king_coord[!stm];
            if(ok == 0x06 || ok == 0x07 || ok == 0x17
            || ok == 0x70 || ok == 0x71 || ok == 0x60)
                ans = 200;
            if(ok == 0x00 || ok == 0x01 || ok == 0x10
            || ok == 0x77 || ok == 0x76 || ok == 0x67)
                ans = -200;

            bool bishop_on_light_square = ((get_col(*rit)) + get_row(*rit)) & 1;
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
            auto k = *king_coord[white];
            if((pawn_max[1 + 0][black] != 0 && king_dist[std::abs(k - 0x00)] <= 1)
            || (pawn_max[1 + 7][black] != 0 && king_dist[std::abs(k - 0x07)] <= 1))
                val_end += 750;
        }
        else if(material[black] == 0)
        {
            auto k = *king_coord[black];
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
            auto colp = get_col(*it);
            bool unstop = IsUnstoppablePawn(colp, 7 - pawn_max[colp + 1][stm], stm);
            auto dist_k = king_dist[std::abs(*king_coord[stm] - *it)];
            auto dist_opp_k = king_dist[std::abs(*king_coord[!stm] - *it)];

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
    if(quantity[white][get_index(_b)] == 2)
    {
        val_opn += 30;
        val_end += 30;
    }
    if(quantity[black][get_index(_b)] == 2)
    {
        val_opn -= 30;
        val_end -= 30;
    }

    // pawn absence for both sides
    if(quantity[white][get_index(_p)] == 0
       && quantity[black][get_index(_p)] == 0
    && material[white] != 0 && material[black] != 0)
        val_end /= 3;

    // multicolored bishops
    if(quantity[white][get_index(_b)] == 1
    && quantity[black][get_index(_b)] == 1)
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

        auto sum_coord_w = get_col(*w_it) + get_row(*w_it);
        auto sum_coord_b = get_col(*b_it) + get_row(*b_it);

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

    auto store_vo = val_opn;
    auto store_ve = val_end;
    auto store_sum = ReturnEval(white);
    std::cout << "\t\t\tMidgame\tEndgame\tTotal" << std::endl;
    std::cout << "Material + PST\t\t";
    std::cout << val_opn << '\t' << val_end << '\t'
              << store_sum << std::endl;

    EvalPawns(white);
    std::cout << "White pawns\t\t";
    std::cout << val_opn - store_vo << '\t' << val_end - store_ve << '\t'
              << ReturnEval(white) - store_sum << std::endl;
    store_vo = val_opn;
    store_ve = val_end;
    store_sum = ReturnEval(white);
    EvalPawns(black);
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

    auto ans = -ReturnEval(wtm);
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
        while(b[get_coord(col, 7 - y)] != _p && y < 7)
            y++;
        pawn_min[col + 1][black] = y;

        y = 6;
        while(b[get_coord(col, 7 - y)] != _p && y > 0)
            y--;
        pawn_max[col + 1][black] = y;
    }
    else
    {
        y = 1;
        while(b[get_coord(col, y)] != _P && y < 7)
            y++;
        pawn_min[col + 1][white] = y;

        y = 6;
        while(b[get_coord(col, y)] != _P && y > 0)
            y--;
        pawn_max[col + 1][white] = y;
    }
}





//-----------------------------
void MovePawnStruct(piece_t moved_piece, coord_t from_coord, move_c move)
{
    if(to_black(moved_piece) == _p || (move.flg & mPROM))
    {
        SetPawnStruct(get_col(move.to));
        if(move.flg)
            SetPawnStruct(get_col(from_coord));
    }
    if(to_black(b_state[prev_states + ply].capt) == _p
    || (move.flg & mENPS))                                    // mENPS not needed
    {
        wtm ^= white;
        SetPawnStruct(get_col(move.to));
        wtm ^= white;
    }
}





//-----------------------------
void InitPawnStruct()
{
    for(auto x = 0; x < 8; x++)
    {
        pawn_max[x + 1][0] = 0;
        pawn_max[x + 1][1] = 0;
        pawn_min[x + 1][0] = 7;
        pawn_min[x + 1][1] = 7;
        for(auto y = 1; y < 7; y++)
            if(b[get_coord(x, y)] == _P)
            {
                pawn_min[x + 1][1] = y;
                break;
            }
        for(auto y = 6; y >= 1; y--)
            if(b[get_coord(x, y)] == _P)
            {
                pawn_max[x + 1][1] = y;
                break;
            }
        for(auto y = 6; y >= 1; y--)
            if(b[get_coord(x, y)] == _p)
            {
                pawn_min[x + 1][0] = 7 - y;
                break;
            }
         for(auto y = 1; y < 7; y++)
            if(b[get_coord(x, y)] == _p)
            {
                pawn_max[x + 1][0] = 7 - y;
                break;
            }
    }
}




//-----------------------------
score_t CountKingTropism(side_to_move_t king_color)
{
    auto occ_cr = 0;
    auto rit = coords[!king_color].rbegin();
    ++rit;
    for(; rit != coords[!king_color].rend(); ++rit)
    {
        auto cur_piece = to_black(b[*rit]);
        if(cur_piece == _p)
            break;
        auto dist = king_dist[std::abs(*king_coord[king_color] - *rit)];
        if(dist >= 4)
            continue;
        occ_cr += tropism_factor[dist < 3][get_index(cur_piece)];
    }
    return occ_cr;
}





//-----------------------------
void MoveKingTropism(coord_t from_coord, move_c move, side_to_move_t king_color)
{
    b_state[prev_states + ply].tropism[black] = king_tropism[black];
    b_state[prev_states + ply].tropism[white] = king_tropism[white];

    auto cur_piece = b[move.to];

    if(to_black(cur_piece) == _k)
    {
        king_tropism[king_color]  = CountKingTropism(king_color);
        king_tropism[!king_color] = CountKingTropism(!king_color);

        return;
    }

    auto dist_to = king_dist[std::abs(*king_coord[king_color] - move.to)];
    auto dist_fr = king_dist[std::abs(*king_coord[king_color] - from_coord)];


    if(dist_fr < 4 && !(move.flg & mPROM))
        king_tropism[king_color] -= tropism_factor[dist_fr < 3]
                                                  [get_index(cur_piece)];
    if(dist_to < 4)
        king_tropism[king_color] += tropism_factor[dist_to < 3]
                                                  [get_index(cur_piece)];

    auto cap = b_state[prev_states + ply].capt;
    if(move.flg & mCAPT)
    {
        dist_to = king_dist[std::abs(*king_coord[!king_color] - move.to)];
        if(dist_to < 4)
            king_tropism[!king_color] -= tropism_factor[dist_to < 3]
                                                       [get_index(cap)];
    }

#ifndef NDEBUG
    auto tmp = CountKingTropism(king_color);
    if(king_tropism[king_color] != tmp && to_black(cap) != _k)
        ply = ply;
    tmp = CountKingTropism(!king_color);
    if(king_tropism[!king_color] != tmp && to_black(cap) != _k)
        ply = ply;
#endif // NDEBUG
}





//-----------------------------
bool MkMoveAndEval(move_c m)
{
    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    bool is_special_move = MkMoveFast(m);

    auto fr = b_state[prev_states + ply].fr;

    MoveKingTropism(fr, m, wtm);

    MovePawnStruct(b[m.to], fr, m);

    return is_special_move;
}





//-----------------------------
void UnMoveAndEval(move_c m)
{
    king_tropism[black] = b_state[prev_states + ply].tropism[black];
    king_tropism[white] = b_state[prev_states + ply].tropism[white];

    auto fr = b_state[prev_states + ply].fr;

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

    auto fr = b_state[prev_states + ply].fr;

    MoveKingTropism(fr, m, wtm);

    MovePawnStruct(b[m.to], fr, m);
}





//-----------------------------
score_t KingOpenFiles(side_to_move_t king_color)
{
    auto ans = 0;
    auto k = *king_coord[king_color];

    if(get_col(k) == 0)
        k++;
    else if(get_col(k) == 7)
        k--;

    int open_files_near_king = 0, open_files[3] = {0};
    for(auto i = 0; i < 3; ++i)
    {
        auto col = get_col(k) + i - 1;
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

    int rooks_queens_on_file[8] = {0};
    auto rit = coords[!king_color].rbegin();
    ++rit;
    for(; rit != coords[!king_color].rend(); ++rit)
    {
        auto pt = b[*rit];
        if(to_black(pt) != _q && to_black(pt) != _r)
            break;
        auto k = pawn_max[get_col(*rit) + 1][king_color] ? 1 : 2;
        rooks_queens_on_file[get_col(*rit)] +=
                k*(to_black(pt) == _r ? 2 : 1);
    }

    for(auto i = 0; i < 3; ++i)
    {
        if(open_files[i])
            ans += rooks_queens_on_file[get_col(k) + i - 1];
    }
    if(ans <= 2)
        ans = ans/2;

    return ans;
}





//-----------------------------
score_t KingWeakness(side_to_move_t king_color)
{
    auto ans = 0;
    auto k = *king_coord[king_color];
    auto shft = king_color ? 16 : -16;

    if(get_col(k) == 0)
        k++;
    else if(get_col(k) == 7)
        k--;

    if(get_col(k) == 2 || get_col(k) == 5)
        ans += 30;
    if(get_col(k) == 3 || get_col(k) == 4)
    {
        if(b_state[prev_states + ply].cstl & (0x0C >> 2*king_color)) // able to castle
            ans += 30;
        else
            ans += 60;
    }
    if((king_color == white && get_row(k) > 1)
    || (king_color == black && get_row(k) < 6))
    {
        ans += 60;
    }


    auto idx = 0;
    for(auto col = 0; col < 3; ++col)
    {
        if(col + k + 2*shft - 1 < 0
        || col + k + 2*shft - 1 >= (score_t)(sizeof(b)/sizeof(*b)))
            continue;
        auto pt1 = b[col + k + shft - 1];
        auto pt2 = b[col + k + 2*shft - 1];
        const piece_t pwn = _p | king_color;
        if(pt1 == pwn || pt2 == pwn)
            continue;

        if(is_light(pt1, king_color)
        && is_light(pt2, king_color))
            continue;

        if(col + k + shft - 2 < 0)
            continue;

        if(is_light(pt2, king_color)
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
    auto trp = king_tropism[king_color];

    if(quantity[!king_color][get_index(_q)] == 0)
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

    auto kw = KingWeakness(king_color);
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

    auto ans = -3*(kw + trp)/2;

    val_opn += king_color ? ans : -ans;

}





//--------------------------------
bool is_light(piece_t piece, side_to_move_t stm)
{
    return piece != __ && (piece & white) == stm;
}





//--------------------------------
bool is_dark(piece_t piece, side_to_move_t stm)
{
    return piece != __ && (piece & white) != stm;
}

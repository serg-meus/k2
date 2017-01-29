#include "eval.h"





//-----------------------------
k2chess::score_t k2eval::Eval()
{
    state[ply].val_opn = val_opn;
    state[ply].val_end = val_end;

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
    ans -= 8;  // bonus for side to move

    val_opn = state[ply].val_opn;
    val_end = state[ply].val_end;

    return ans;
}





//-----------------------------
void k2eval::InitEval()
{
    val_opn = 0;
    val_end = 0;

    for(auto i = 0; i < 120; i++)
        if(i & 8)
            king_dist[i] = std::max(8 - get_col(i), get_row(i) + 1);
        else
            king_dist[i] = std::max(get_col(i), get_row(i));

    InitPawnStruct();

    state[ply].val_opn = 0;
    state[ply].val_end = 0;

    p_max[0][black] = 0;
    p_max[0][white] = 0;
    p_min[0][black] = 7;
    p_min[0][white] = 7;
    p_max[9][black] = 0;
    p_max[9][white] = 0;
    p_min[9][black] = 7;
    p_min[9][white] = 7;

    king_tropism[white] = 0;
    king_tropism[black] = 0;
}





//-----------------------------
void k2eval::FastEval(move_c m)
{
    score_t ansO = 0, ansE = 0;

    auto x = get_col(m.to_coord);
    auto y = get_row(m.to_coord);
    auto x0 = get_col(state[ply].from_coord);
    auto y0 = get_row(state[ply].from_coord);
    auto pt = get_index(b[m.to_coord]);

    if(!wtm)
    {
        y = 7 - y;
        y0 = 7 - y0;
    }

    piece_index_t idx;
    auto flag = m.flag & is_promotion;
    if(flag)
    {
        idx = pawn;
        ansO -= material_values_opn[idx] + pst[idx - 1][0][y0][x0];
        ansE -= material_values_end[idx] + pst[idx - 1][1][y0][x0];

        piece_t promo_pieces[] =
        {
            black_queen, black_knight, black_rook, black_bishop
        };
        idx = get_index(promo_pieces[flag - 1]);
        ansO += material_values_opn[idx] + pst[idx - 1][0][y0][x0];
        ansE += material_values_end[idx] + pst[idx - 1][1][y0][x0];

    }

    if(m.flag & is_capture)
    {
        auto capt = to_black(state[ply].capt);
        if(m.flag & is_en_passant)
        {
            idx = pawn;
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
    else if(m.flag & is_castle_kingside)
    {
        idx = rook;
        ansO += pst[idx - 1][0][7][5] - pst[idx - 1][0][7][7];
        ansE += pst[idx - 1][1][7][5] - pst[idx - 1][1][7][7];
    }
    else if(m.flag & is_castle_queenside)
    {
        idx = rook;
        ansO += pst[idx - 1][0][7][3] - pst[idx - 1][0][7][0];
        ansE += pst[idx - 1][1][7][3] - pst[idx - 1][1][7][0];
    }

    ansO += pst[pt - 1][0][y][x] - pst[pt - 1][0][y0][x0];
    ansE += pst[pt - 1][1][y][x] - pst[pt - 1][1][y0][x0];

    val_opn += wtm ? -ansO : ansO;
    val_end += wtm ? -ansE : ansE;
}





//-----------------------------
void k2eval::InitEvaOfMaterialAndPst()
{
    val_opn = 0;
    val_end = 0;
    for(size_t col = 0; col < sizeof(b)/sizeof(*b); ++col)
    {
        if(!within_board(col))
            continue;
        auto pt = b[col];
        if(pt == empty_square)
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
    state[ply].val_opn = val_opn;
    state[ply].val_end = val_end;

    king_tropism[white] = CountKingTropism(white);
    king_tropism[black] = CountKingTropism(black);
}





//--------------------------------
bool k2eval::IsPasser(coord_t col, side_to_move_t stm)
{
    auto mx = pawn_max[col][stm];

    if(mx >= 7 - pawn_min[col][!stm]
            && mx >= 7 - pawn_min[col - 1][!stm]
            && mx >= 7 - pawn_min[col + 1][!stm])
        return true;
    else
        return false;

}





//--------------------------------
void k2eval::EvalPawns(side_to_move_t stm)
{
    score_t ansO = 0, ansE = 0;
    bool passer, prev_passer = false;
    bool opp_only_pawns = material[!stm] == pieces[!stm] - 1;

    for(auto col = 0; col < 8; col++)
    {
        bool doubled = false, isolany = false;

        auto mx = pawn_max[col][stm];
        if(mx == 0)
        {
            prev_passer = false;
            continue;
        }

        if(pawn_min[col][stm] != mx)
            doubled = true;
        if(pawn_max[col - 1][stm] == 0 && pawn_max[col + 1][stm] == 0)
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
            if(col > 0 && col < 7 && mx < pawn_min[col - 1][stm]
                    && mx < pawn_min[col + 1][stm])
            {
                auto y_coord = stm ? mx + 1 : 7 - mx - 1;
                auto op_piece = b[get_coord(col, y_coord)];
                bool occupied = is_dark(op_piece, stm)
                                && to_black(op_piece) != black_pawn;
                if(occupied)
                    ansO -= 30;
            }

            // gaps in pawn structure
            if(pawn_max[col - 1][stm]
                    && std::abs(mx - pawn_max[col - 1][stm]) > 1)
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
        auto next_square = get_coord(col, stm ? mx + 1 : 7 - mx - 1);
        bool blocked = b[next_square] != empty_square;
        auto delta = blocked ? blocked_pass[mx] : pass[mx];

        ansE += delta;
        ansO += delta/3;

        // connected passers
        if(passer && prev_passer
                && std::abs(mx - pawn_max[col - 1][stm]) <= 1)
        {
            auto mmx = std::max(pawn_max[col - 1][stm], mx);
            if(mmx > 4)
                ansE += 28*mmx;
        }
        prev_passer = true;

        // unstoppable
        if(opp_only_pawns
                && IsUnstoppablePawn(col, 7 - pawn_max[col][stm], stm))
        {
            ansO += 120*mx + 350;
            ansE += 120*mx + 350;
        }
    }

    val_opn += stm ? ansO : -ansO;
    val_end += stm ? ansE : -ansE;
}





//------------------------------
k2chess::score_t k2eval::OneBishopMobility(coord_t b_coord)
{
    auto ans = 0;
    auto empty_squares = 0;
    for(auto ray = 0; ray < 4; ++ray)
    {
        auto coord = b_coord;
        for(auto col = 0; col < 8; ++col)
        {
            coord += shifts[bishop][ray];
            if(!within_board(coord) || to_black(b[coord]) == black_pawn)
                break;
            empty_squares++;
        }
    }
    ans = -empty_squares*empty_squares/6 + 6*empty_squares - 16;
    return ans;
}





//------------------------------
void k2eval::BishopMobility(side_to_move_t stm)
{
    score_t ans = 0;

    auto rit = coords[stm].rbegin();

    while(rit != coords[stm].rend()
            && to_black(b[*rit]) != black_bishop)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    ans += OneBishopMobility(*rit);

    ++rit;
    if(rit != coords[stm].rend() && to_black(b[*rit]) == black_bishop)
        ans += OneBishopMobility(*rit);

    val_opn += stm ? ans : -ans;
    val_end += stm ? ans : -ans;
}





//-----------------------------
inline void k2eval::ClampedRook(side_to_move_t stm)
{
    const score_t clamped_rook = 82;
    auto k = *king_coord[stm];

    if(stm)
    {
        if(k == 0x06 && b[0x07] == white_rook && pawn_max[7][1])
            val_opn -= clamped_rook;
        else if(k == 0x05)
        {
            if(pawn_max[7][1] && b[0x07] == white_rook)
                val_opn -= clamped_rook;
            else if(pawn_max[6][1] && b[0x06] == white_rook)
                val_opn -= clamped_rook;
        }
        else if(k == 0x01 && b[0x00] == white_rook && pawn_max[0][1])
            val_opn -= clamped_rook;
        else if(k == 0x02)
        {
            if(pawn_max[0][1] && b[0x00] == white_rook)
                val_opn -= clamped_rook;
            else if(pawn_max[1][1] && b[0x01] == white_rook)
                val_opn -= clamped_rook;
        }
    }
    else
    {
        if(k == 0x76 && b[0x77] == black_rook && pawn_max[7][0])
            val_opn += clamped_rook;
        else if(k == 0x75)
        {
            if(pawn_max[7][0] && b[0x77] == black_rook)
                val_opn += clamped_rook;
            else if(pawn_max[6][0] && b[0x76] == black_rook)
                val_opn += clamped_rook;
        }
        else if(k == 0x71 && b[0x70] == black_rook && pawn_max[0][0])
            val_opn += clamped_rook;
        else if(k == 0x72)
        {
            if(pawn_max[0][0] && b[0x70] == black_rook)
                val_opn += clamped_rook;
            else if(pawn_max[1][0] && b[0x71] == black_rook)
                val_opn += clamped_rook;
        }
    }

    score_t ans = 0;
    auto rit = coords[stm].rbegin();

    while(rit != coords[stm].rend()
            && to_black(b[*rit]) != black_rook)
        ++rit;
    if(rit == coords[stm].rend())
        return;

    auto rook_on_7th_cr = 0;
    if((stm && get_row(*rit) >= 6) || (!stm && get_row(*rit) <= 1))
        rook_on_7th_cr++;
    if(quantity[stm][pawn] >= 4
            && pawn_max[get_col(*rit)][stm] == 0)
        ans += (pawn_max[get_col(*rit)][!stm] == 0 ? 54 : 22);

    auto empty_sq = 0;
    for(auto i = 1; i <= 2; ++i)
    {
        if(get_col(*rit) + i < 7 && b[*rit + i] == empty_square)
            empty_sq++;
        if(get_col(*rit) - i > 0 && b[*rit - i] == empty_square)
            empty_sq++;
    }
    if(empty_sq <= 1)
        ans -= 15;

    ++rit;
    if(rit != coords[stm].rend() && to_black(b[*rit]) == black_rook)
    {
        if((stm && get_row(*rit) >= 6) || (!stm && get_row(*rit) <= 1))
            rook_on_7th_cr++;
        if(quantity[stm][pawn] >= 4
                && pawn_max[get_col(*rit)][stm] == 0)
            ans += (pawn_max[get_col(*rit)][!stm] == 0 ? 54 : 22);

        empty_sq = 0;
        for(coord_t i = 1; i <= 2; ++i)
        {
            if(get_col(*rit) + i < 7 && b[*rit + i] == empty_square)
                empty_sq++;
            if(get_col(*rit) - i > 0 && b[*rit - i] == empty_square)
                empty_sq++;
        }
        if(empty_sq <= 1)
            ans -= 15;
    }
    ans += rook_on_7th_cr*47;
    val_opn += (stm ? ans : -ans);

}





//-----------------------------
bool k2eval::IsUnstoppablePawn(coord_t col, coord_t row, side_to_move_t stm)
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
void k2eval::MaterialImbalances()
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
        if(quantity[white][knight] == 2
                || quantity[black][knight] == 2)
        {
            val_opn = 0;
            val_end = 0;
            return;
        }
        // many code for mating with only bishop and knight
        else if((quantity[white][knight] == 1
                 && quantity[white][bishop] == 1)
                || (quantity[black][knight] == 1
                    && quantity[black][bishop] == 1))
        {
            auto stm = quantity[white][knight] == 1
                       ? white : black;
            auto rit = coords[stm].begin();
            for(; rit != coords[stm].end(); ++rit)
                if(to_black(b[*rit]) == black_bishop)
                    break;
            assert(to_black(b[*rit]) == black_bishop);

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
            if((pawn_max[0][black] != 0
                    && king_dist[std::abs(k - 0x00)] <= 1)
                    || (pawn_max[7][black] != 0
                        && king_dist[std::abs(k - 0x07)] <= 1))
                val_end += 750;
        }
        else if(material[black] == 0)
        {
            auto k = *king_coord[black];
            if((pawn_max[0][white] != 0
                    && king_dist[std::abs(k - 0x70)] <= 1)
                    || (pawn_max[7][white] != 0
                        && king_dist[std::abs(k - 0x77)] <= 1))
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
            bool unstop = IsUnstoppablePawn(colp,
                                            7 - pawn_max[colp][stm],
                                            stm);
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
    if(quantity[white][bishop] == 2)
    {
        val_opn += 30;
        val_end += 30;
    }
    if(quantity[black][bishop] == 2)
    {
        val_opn -= 30;
        val_end -= 30;
    }

    // pawn absence for both sides
    if(quantity[white][pawn] == 0
            && quantity[black][pawn] == 0
            && material[white] != 0 && material[black] != 0)
        val_end /= 3;

    // multicolored bishops
    if(quantity[white][bishop] == 1
            && quantity[black][bishop] == 1)
    {
        auto w_it = coords[white].rbegin();
        while(w_it != coords[white].rend()
                && b[*w_it] != white_bishop)
            ++w_it;
        assert(w_it != coords[white].rend());

        auto b_it = coords[black].rbegin();
        while(b_it != coords[black].rend()
                && b[*b_it] != black_bishop)
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
k2chess::score_t k2eval::EvalDebug()
{
    state[ply].val_opn = val_opn;
    state[ply].val_end = val_end;

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
    ans -= 8;  // bonus for side to move
    std::cout << "Bonus for side to move\t\t\t";
    std::cout << (wtm ? 8 : -8) << std::endl << std::endl;

    std::cout << std::endl << std::endl;

    std::cout << "Eval summary: " << (wtm ? -ans : ans) << std::endl;
    std::cout << "(positive values means advantage for white)" << std::endl;

    val_opn = state[ply].val_opn;
    val_end = state[ply].val_end;

    return ans;
}





//-----------------------------
void k2eval::SetPawnStruct(k2chess::coord_t col)
{
    assert(col <= 7);
    coord_t y;
    if(wtm)
    {
        y = 1;
        while(b[get_coord(col, 7 - y)] != black_pawn && y < 7)
            y++;
        pawn_min[col][black] = y;

        y = 6;
        while(b[get_coord(col, 7 - y)] != black_pawn && y > 0)
            y--;
        pawn_max[col][black] = y;
    }
    else
    {
        y = 1;
        while(b[get_coord(col, y)] != white_pawn && y < 7)
            y++;
        pawn_min[col][white] = y;

        y = 6;
        while(b[get_coord(col, y)] != white_pawn && y > 0)
            y--;
        pawn_max[col][white] = y;
    }
}





//-----------------------------
void k2eval::MovePawnStruct(piece_t moved_piece, coord_t from_coord,
                            move_c move)
{
    if(to_black(moved_piece) == black_pawn || (move.flag & is_promotion))
    {
        SetPawnStruct(get_col(move.to_coord));
        if(move.flag)
            SetPawnStruct(get_col(from_coord));
    }
    if(to_black(state[ply].capt) == black_pawn
            || (move.flag & is_en_passant))  // is_en_passant not needed
    {
        wtm ^= white;
        SetPawnStruct(get_col(move.to_coord));
        wtm ^= white;
    }
#ifndef NDEBUG
    rank_t copy_max[8][2], copy_min[8][2];
    memcpy(copy_max, pawn_max, sizeof(copy_max));
    memcpy(copy_min, pawn_min, sizeof(copy_min));
    InitPawnStruct();
    assert(!memcmp(copy_max, pawn_max, sizeof(copy_max)));
    assert(!memcmp(copy_min, pawn_min, sizeof(copy_min)));
#endif
}





//-----------------------------
void k2eval::InitPawnStruct()
{
    for(auto x = 0; x < 8; x++)
    {
        pawn_max[x][0] = 0;
        pawn_max[x][1] = 0;
        pawn_min[x][0] = 7;
        pawn_min[x][1] = 7;
        for(auto y = 1; y < 7; y++)
            if(b[get_coord(x, y)] == white_pawn)
            {
                pawn_min[x][1] = y;
                break;
            }
        for(auto y = 6; y >= 1; y--)
            if(b[get_coord(x, y)] == white_pawn)
            {
                pawn_max[x][1] = y;
                break;
            }
        for(auto y = 6; y >= 1; y--)
            if(b[get_coord(x, y)] == black_pawn)
            {
                pawn_min[x][0] = 7 - y;
                break;
            }
        for(auto y = 1; y < 7; y++)
            if(b[get_coord(x, y)] == black_pawn)
            {
                pawn_max[x][0] = 7 - y;
                break;
            }
    }
}




//-----------------------------
k2chess::score_t k2eval::CountKingTropism(side_to_move_t king_color)
{
    auto occ_cr = 0;
    auto rit = coords[!king_color].rbegin();
    ++rit;
    for(; rit != coords[!king_color].rend(); ++rit)
    {
        auto cur_piece = to_black(b[*rit]);
        if(cur_piece == black_pawn)
            break;
        auto dist = king_dist[std::abs(*king_coord[king_color] - *rit)];
        if(dist >= 4)
            continue;
        occ_cr += tropism_factor[dist < 3][get_index(cur_piece)];
    }
    return occ_cr;
}





//-----------------------------
void k2eval::MoveKingTropism(coord_t from_coord, move_c move,
                             side_to_move_t king_color)
{
    state[ply].tropism[black] = king_tropism[black];
    state[ply].tropism[white] = king_tropism[white];

    auto cur_piece = b[move.to_coord];

    if(to_black(cur_piece) == black_king)
    {
        king_tropism[king_color] = CountKingTropism(king_color);
        king_tropism[!king_color] = CountKingTropism(!king_color);

        return;
    }
    auto k_coord = *king_coord[king_color];
    auto dist_to = king_dist[std::abs(k_coord - move.to_coord)];
    auto dist_fr = king_dist[std::abs(k_coord - from_coord)];


    if(dist_fr < 4 && !(move.flag & is_promotion))
        king_tropism[king_color] -= tropism_factor[dist_fr < 3]
                                    [get_index(cur_piece)];
    if(dist_to < 4)
        king_tropism[king_color] += tropism_factor[dist_to < 3]
                                    [get_index(cur_piece)];

    auto cap = state[ply].capt;
    if(move.flag & is_capture)
    {
        dist_to = king_dist[std::abs(*king_coord[!king_color]
                                     - move.to_coord)];
        if(dist_to < 4)
            king_tropism[!king_color] -= tropism_factor[dist_to < 3]
                                         [get_index(cap)];
    }

#ifndef NDEBUG
    auto tmp = CountKingTropism(king_color);
    if(king_tropism[king_color] != tmp && to_black(cap) != black_king)
        ply = ply;
    tmp = CountKingTropism(!king_color);
    if(king_tropism[!king_color] != tmp && to_black(cap) != black_king)
        ply = ply;
#endif // NDEBUG
}





//-----------------------------
bool k2eval::MkMoveAndEval(move_c m)
{
    state[ply].val_opn = val_opn;
    state[ply].val_end = val_end;

    bool is_special_move = MkMoveFast(m);

    auto from_coord = state[ply].from_coord;

    MoveKingTropism(from_coord, m, wtm);

    MovePawnStruct(b[m.to_coord], from_coord, m);

    return is_special_move;
}





//-----------------------------
void k2eval::UnMoveAndEval(move_c m)
{
    king_tropism[black] = state[ply].tropism[black];
    king_tropism[white] = state[ply].tropism[white];

    auto from_coord = state[ply].from_coord;

    UnMoveFast(m);

    ply++;
    wtm ^= white;
    MovePawnStruct(b[from_coord], from_coord, m);
    wtm ^= white;
    ply--;

    val_opn = state[ply].val_opn;
    val_end = state[ply].val_end;
}





//-----------------------------
k2chess::score_t k2eval::KingOpenFiles(side_to_move_t king_color)
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
        if(pawn_max[col][!king_color] == 0)
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
        if(to_black(pt) != black_queen && to_black(pt) != black_rook)
            break;
        auto k = pawn_max[get_col(*rit)][king_color] ? 1 : 2;
        rooks_queens_on_file[get_col(*rit)] +=
            k*(to_black(pt) == black_rook ? 2 : 1);
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
k2chess::score_t k2eval::KingWeakness(side_to_move_t king_color)
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
        // if able to castle
        if(state[ply].cstl & (0x0C >> 2*king_color))
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
        const piece_t pwn = black_pawn | king_color;
        if(pt1 == pwn || pt2 == pwn)
            continue;

        if(is_light(pt1, king_color)
                && is_light(pt2, king_color))
            continue;

        if(col + k + shft - 2 < 0)
            continue;

        if(is_light(pt2, king_color)
                && (b[col + k + shft - 2] == pwn
                    || b[col + k + shft + 0] == pwn))
            continue;

        idx += (1 << col);
    }
    idx = 7 - idx;
    // cases: ___, __p, _p_, _pp, p__, p_p, pp_, ppp
    size_t cases[] = {0, 1, 1, 3, 1, 2, 3, 4};
    score_t scores[] = {140, 75, 75, 10, 0};

    ans += scores[cases[idx]];

    if(cases[idx] != 4)
        ans += 25*KingOpenFiles(king_color);

    return ans;
}





//-----------------------------
void k2eval::KingSafety(side_to_move_t king_color)
{
    auto trp = king_tropism[king_color];

    if(quantity[!king_color][queen] == 0)
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
    // if able to castle
    if(state[ply].cstl & (0x0C >> 2*king_color))
        kw /= 4;
    else
        kw = (material[!king_color] + 24)*kw/72;

    auto ans = -3*(kw + trp)/2;

    val_opn += king_color ? ans : -ans;
}





//-----------------------------
k2eval::k2eval() : material_values_opn {0, 0, queen_val_opn, rook_val_opn,
            bishop_val_opn, kinght_val_opn, pawn_val_opn},
    material_values_end {0, 0, queen_val_end, rook_val_end,
                           bishop_val_end, kinght_val_end, pawn_val_end},
    tropism_factor  //  k  Q   R   B   N   P
{
    {0, 0, 10, 10, 10,  4, 4},  // 4 >= dist > 3
    {0, 0, 21, 21, 10,  0, 10}  // dist < 3
},
pst
{
    {
        // KING
        {   {-8,   -18,   -18,   -28,   -28,   -18,   -18,    -8},
            {-8,   -18,   -18,   -28,   -28,   -18,   -18,    -8},
            {-8,   -18,   -18,   -28,   -28,   -18,   -18,    -8},
            {-8,   -18,   -18,   -28,   -28,   -18,   -18,    -8},
            { 2,    -8,    -8,   -18,   -18,    -8,    -8,     2},
            {12,     2,     2,     2,     2,     2,     2,    12},
            {42,    42,    22,    22,    22,    22,    42,    42},
            {42,    62,    32,    22,    22,    32,    62,    42}
        },

        {   {-60,   -45,   -31,   -16,   -16,   -31,   -45,   -60},
            {-40,   -15,    -1,    14,    14,    -1,   -15,   -40},
            {-30,     0,    34,    40,    40,    34,     0,   -30},
            {-30,     0,    40,    50,    50,    40,     0,   -30},
            {-30,     0,    40,    50,    50,    40,     0,   -30},
            {-30,     0,    34,    40,    40,    34,     0,   -30},
            {-40,   -15,    -1,    14,    14,    -1,   -15,   -40},
            {-60,   -45,   -31,   -16,   -16,   -31,   -45,   -60}
        }
    },

    {
        // QUEEN
        {   {-16,    -6,    -6,    -1,    -1,    -6,    -6,   -16},
            { -6,     4,     4,     4,     4,     4,     4,    -6},
            { -6,     4,     4,     4,     4,     4,     4,    -6},
            { -1,     4,     4,     4,     4,     4,     4,    -1},
            { -1,     4,     4,     4,     4,     4,     4,    -1},
            { -6,     4,     4,     4,     4,     4,     4,    -6},
            { -6,     4,     4,     4,     4,     4,     4,    -6},
            {-16,    -6,    -6,    -1,    -1,    -6,    -6,   -16}
        },

        {   {-17,    -7,    -7,    -2,    -2,    -7,    -7,   -17},
            { -7,     3,     3,     3,     3,     3,     3,    -7},
            { -7,     3,     8,     8,     8,     8,     3,    -7},
            { -2,     3,     8,     8,     8,     8,     3,    -2},
            { -2,     3,     8,     8,     8,     8,     3,    -2},
            { -7,     3,     8,     8,     8,     8,     3,    -7},
            { -7,     3,     3,     3,     3,     3,     3,    -7},
            {-17,    -7,    -7,    -7,    -7,    -7,    -7,   -17},
        }
    },

    {
        // ROOK
        {   {  0,     0,     0,     0,     0,     0,     0,     0},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            {  0,     0,     0,     0,     0,     0,     0,     0}
        },

        {   {  0,     0,     0,     0,     0,     0,     0,     0},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            { -5,     0,     0,     0,     0,     0,     0,    -5},
            {  0,     0,     0,     0,     0,     0,     0,     0}
        }
    },

    {
        //BISHOP
        {   {-20,   -10,   -10,   -10,   -10,   -10,   -10,   -20},
            {-10,     0,     0,     0,     0,     0,     0,   -10},
            {-10,     0,     5,    10,    10,     5,     0,   -10},
            {-10,     5,     5,    10,    10,     5,     5,   -10},
            {-10,     0,    10,    10,    10,    10,     0,   -10},
            {-10,    10,    10,    10,    10,    10,    10,   -10},
            {-10,     5,     0,     0,     0,     0,     5,   -10},
            {-20,   -10,   -10,   -10,   -10,   -10,   -10,   -20}
        },

        {   {-18,    -8,    -8,    -8,    -8,    -8,    -8,   -18},
            { -8,     2,     2,     2,     2,     2,     2,    -8},
            { -8,     2,     7,    12,    12,     7,     2,    -8},
            { -8,     7,     7,    12,    12,     7,     7,    -8},
            { -8,     2,    12,    12,    12,    12,     2,    -8},
            { -8,    12,    12,    12,    12,    12,    12,    -8},
            { -8,     7,     2,     2,     2,     2,     7,    -8},
            {-18,    -8,    -8,    -8,    -8,    -8,    -8,   -18}
        }
    },

    {
        //KNIGHT
        {   {-38,   -28,   -18,   -18,   -18,   -18,   -28,   -38},
            {-28,    -8,    12,    12,    12,    12,    -8,   -28},
            {  0,    25,    30,    40,    40,    30,    25,     0},
            {-10,    17,    42,    50,    50,    42,    17,   -10},
            {-18,    12,    27,    32,    32,    27,    12,   -18},
            {-18,    17,    22,    27,    27,    22,    17,   -18},
            {-28,    -8,    12,    17,    17,    12,    -8,   -28},
            {-38,   -28,   -18,   -18,   -18,   -18,   -28,   -38}
        },

        {   {-75,   -25,   -15,   -15,   -15,   -15,   -25,   -75},
            {-25,    -5,    15,    15,    15,    15,    -5,   -25},
            {-15,    15,    25,    30,    30,    25,    15,   -15},
            {-15,    20,    30,    35,    35,    30,    20,   -15},
            {-15,    15,    30,    35,    35,    30,    15,   -15},
            {-15,    20,    25,    30,    30,    25,    20,   -15},
            {-25,    -5,    15,    20,    20,    15,    -5,   -25},
            {-75,   -25,   -15,   -15,   -15,   -15,   -25,   -75}
        }
    },

    {
        //PAWN
        {   {-11,   -11,   -11,   -11,   -11,   -11,   -11,   -11},
            { 39,    39,    39,    39,    39,    39,    39,    39},
            { -1,    -1,     9,    19,    19,     9,    -1,    -1},
            { -6,    -6,    -1,    14,    14,    -1,    -6,    -6},
            {-11,   -11,   -11,     9,     9,   -11,   -11,   -11},
            { -6,   -16,   -21,   -11,   -11,   -21,   -16,    -6},
            { -6,    -1,    -1,   -31,   -31,    -1,    -1,    -6},
            {-11,   -11,   -11,   -11,   -11,   -11,   -11,   -11},
        },

        {   { 0,      0,     0,     0,     0,     0,     0,     0},
            { 0,      0,     0,     0,     0,     0,     0,     0},
            { 0,      0,     0,     0,     0,     0,     0,     0},
            { 0,      0,     0,     0,     0,     0,     0,     0},
            { 0,      0,     0,     0,     0,     0,     0,     0},
            { 0,      0,     0,     0,     0,     0,     0,     0},
            { 0,      0,     0,     0,     0,     0,     0,     0},
            { 0,      0,     0,     0,     0,     0,     0,     0},
        }
    }
}

{
    pawn_max = &p_max[1];
    pawn_min = &p_min[1];

    pawn = get_index(black_pawn);
    knight = get_index(black_knight);
    bishop = get_index(black_bishop);
    rook = get_index(black_rook);
    queen = get_index(black_queen);
    king = get_index(black_king);
    InitEval();
}

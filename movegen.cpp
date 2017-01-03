#include "movegen.h"





//--------------------------------
void k2movegen::InitMoveGen()
{
    InitEval();
}





//--------------------------------
k2movegen::movcr_t k2movegen::GenMoves(move_c *move_array,
                                       move_c *best_move,
                                       priority_t apprice)
{
    movcr_t moveCr = 0;

    GenCastles(move_array, &moveCr);

    auto it = coords[wtm].begin();
    for(;it != coords[wtm].end(); ++it)
    {
        auto fr = *it;
        auto at = get_index(b[fr]);
        if(at == get_index(black_pawn))
        {
            GenPawn(move_array, &moveCr, it);
            continue;
        }

        if(!slider[at])
        {
            for(auto ray = 0; ray < rays[at]; ray++)
            {
                auto to = fr + shifts[at][ray];
                if(within_board(to) && !is_light(b[to], wtm))
                    PushMove(move_array, &moveCr, it, to, b[to] ? is_capture : 0);
            }
            continue;
        }

        for(auto ray = 0; ray < rays[at]; ray++)
        {
            auto to = fr;
            for(auto i = 0; i < 8; i++)
            {
                to += shifts[at][ray];
                if(!within_board(to))
                    break;
                auto tt = b[to];
                if(!tt)
                {
                    PushMove(move_array, &moveCr, it, to, 0);
                    continue;
                }
                if(is_dark(b[to], wtm))
                    PushMove(move_array, &moveCr, it, to, is_capture);
                break;
            }// for(i
        }// for(ray
    }// for(it
    if(apprice == apprice_all)
        AppriceMoves(move_array, moveCr, best_move);
    else if(apprice == apprice_only_captures)
        AppriceQuiesceMoves(move_array, moveCr);

    return moveCr;
}





//--------------------------------
void k2movegen::GenPawn(move_c *move_array, movcr_t *moveCr, iterator it)
{
    move_flag_t pBeg, pEnd;
    auto fr = *it;
    if((wtm && get_row(fr) == 6) || (!wtm && get_row(fr) == 1))
    {
        pBeg = is_promotion_to_queen;
        pEnd = is_promotion_to_bishop;
    }
    else
    {
        pBeg = 0;
        pEnd = 0;
    }
    for(auto i = pBeg; i <= pEnd; ++i)
        if(wtm)
        {
            auto to = fr + 17;
            if(within_board(to) && is_dark(b[to], wtm))
                PushMove(move_array, moveCr, it, to, is_capture | i);
            to = fr + 15;
            if(within_board(to) && is_dark(b[to], wtm))
                PushMove(move_array, moveCr, it, to, is_capture | i);
            to = fr + 16;
            if(within_board(to) && !b[to])                 // within_board not needed
                PushMove(move_array, moveCr, it, to, i);
            if(get_row(fr) == 1 && !b[fr + 16] && !b[fr + 32])
                PushMove(move_array, moveCr, it, fr + 32, 0);
            auto ep = b_state[prev_states + ply].ep;
            auto delta = ep - 1 - get_col(fr);
            if(ep && std::abs(delta) == 1 && get_row(fr) == 4)
                PushMove(move_array, moveCr, it, fr + 16 + delta, is_capture | is_en_passant);
        }
        else
        {
            auto to = fr - 17;
            if(within_board(to) && is_dark(b[to], wtm))
                PushMove(move_array, moveCr, it, to, is_capture | i);
            to = fr - 15;
            if(within_board(to) && is_dark(b[to], wtm))
                PushMove(move_array, moveCr, it, to, is_capture | i);
            to = fr - 16;
            if(within_board(to) && !b[to])                  // within_board not needed
                PushMove(move_array, moveCr, it, to, i);
            if(get_row(fr) == 6 && !b[fr - 16] && !b[fr - 32])
                PushMove(move_array, moveCr, it, fr - 32, 0);
            auto ep = b_state[prev_states + ply].ep;
            auto delta = ep - 1 - get_col(fr);
            if(ep && std::abs(delta) == 1 && get_row(fr) == 3)
                PushMove(move_array, moveCr, it, fr - 16 + delta, is_capture | is_en_passant);
        }
}





//--------------------------------
void k2movegen::GenPawnCap(move_c *move_array, movcr_t *moveCr, iterator it)
{
    size_t pBeg, pEnd;
    auto fr = *it;
    if((wtm && get_row(fr) == 6) || (!wtm && get_row(fr) == 1))
    {
        pBeg = is_promotion_to_queen;
        pEnd = is_promotion_to_queen;
    }
    else
    {
        pBeg = 0;
        pEnd = 0;
    }
    for(auto i = pBeg; i <= pEnd; ++i)
        if(wtm)
        {
            auto to = fr + 17;
            if(within_board(to) && is_dark(b[to], wtm))
                PushMove(move_array, moveCr, it, to, is_capture | i);
            to = fr + 15;
            if(within_board(to) && is_dark(b[to], wtm))
                PushMove(move_array, moveCr, it, to, is_capture | i);
            to = fr + 16;
            if(pBeg && !b[to])
                PushMove(move_array, moveCr, it, to, i);
            auto ep = b_state[prev_states + ply].ep;
            auto delta = ep - 1 - get_col(fr);
            if(ep && std::abs(delta) == 1 && get_row(fr) == 4)
                PushMove(move_array, moveCr, it, fr + 16 + delta, is_capture | is_en_passant);
        }
        else
        {
            auto to = fr - 17;
            if(within_board(to) && is_dark(b[to], wtm))
                PushMove(move_array, moveCr, it, to, is_capture | i);
            to = fr - 15;
            if(within_board(to) && is_dark(b[to], wtm))
                PushMove(move_array, moveCr, it, to, is_capture | i);
            to = fr - 16;
            if(pBeg && !b[to])
                PushMove(move_array, moveCr, it, to, i);
            auto ep = b_state[prev_states + ply].ep;
            auto delta = ep - 1 - get_col(fr);
            if(ep && std::abs(delta) == 1 && get_row(fr) == 3)
                PushMove(move_array, moveCr, it, fr - 16 + delta, is_capture | is_en_passant);
        }
}





//--------------------------------
void k2movegen::GenCastles(move_c *move_array, movcr_t *moveCr)
{
    castle_t msk = wtm ? 0x03 : 0x0C;
    auto cst = b_state[prev_states + ply].cstl & msk;
    bool check = false, not_seen = true;
    if(!cst)
        return;
    if(wtm)
    {
        if((cst & 1) && !b[0x05] && !b[0x06])
        {
            check = Attack(0x04, black);
            not_seen = false;
            if(!check && !Attack(0x05, black) && !Attack(0x06, black))
                PushMove(move_array, moveCr,
                         king_coord[white], 0x06, is_castle_kingside);
        }
        if((cst & 2) && !b[0x03] && !b[0x02] && !b[0x01])
        {
            if(not_seen)
                check = Attack(0x04, black);
            if(!check && !Attack(0x03, black) && !Attack(0x02, black))
                PushMove(move_array, moveCr,
                         king_coord[white], 0x02, is_castle_queenside);
        }
    }
    else
    {
        if((cst & 4) && !b[0x75] && !b[0x76])
        {
            check = Attack(0x74, white);
            not_seen = false;
            if(!check && !Attack(0x75, white) && !Attack(0x76, white))
                PushMove(move_array, moveCr,
                         king_coord[black], 0x76, is_castle_kingside);
        }
        if((cst & 8) && !b[0x73] && !b[0x72] && !b[0x71])
        {
            if(not_seen)
                check = Attack(0x74, white);
            if(!check && !Attack(0x73, white) && !Attack(0x72, white))
                PushMove(move_array, moveCr,
                         king_coord[black], 0x72, is_castle_queenside);
        }
    }
}





//--------------------------------
k2movegen::movcr_t k2movegen::GenCaptures(move_c *move_array)
{
    movcr_t moveCr = 0;
    auto it = coords[wtm].begin();
    for(; it != coords[wtm].end(); ++it)
    {
        auto fr = *it;
        auto at = get_index(b[fr]);
        if(at == get_index(black_pawn))
        {
            GenPawnCap(move_array, &moveCr, it);
            continue;
        }
        if(!slider[at])
        {
            for(auto  ray = 0; ray < rays[at]; ray++)
            {
                auto to = fr + shifts[at][ray];
                if(within_board(to) && is_dark(b[to], wtm))
                    PushMove(move_array, &moveCr, it, to, is_capture);
            }
            continue;
        }

        for(auto ray = 0; ray < rays[at]; ray++)
        {
            auto to = fr;
            for(auto i = 0; i < 8; i++)
            {
                to += shifts[at][ray];
                if(!within_board(to))
                    break;
                auto tt = b[to];
                if(!tt)
                    continue;
                if(is_dark(tt, wtm))
                    PushMove(move_array, &moveCr, it, to, is_capture);
                break;
            }// for(i
        }// for(ray
    }// for(pc
    AppriceQuiesceMoves(move_array, moveCr);
    SortQuiesceMoves(move_array, moveCr);
    return moveCr;
}





//--------------------------------
void k2movegen::AppriceMoves(move_c *move_array, movcr_t moveCr, move_c *bestMove)
{
    min_history = std::numeric_limits<history_t>::max();
    max_history = 0;

    auto it = coords[wtm].begin();
    auto bm = *move_array;
    if(bestMove == nullptr)
        bm.flg = 0xFF;
    else
        bm = *bestMove;
    for(auto i = 0; i < moveCr; i++)
    {
        auto m = move_array[i];

        it = m.pc;
        auto fr_pc = b[*it];
        auto to_pc = b[m.to];

        if(m == bm)
            move_array[i].scr = move_from_hash;
        else
        if(to_pc == empty_square && !(m.flg & is_promotion))
        {
            if(m == killers[ply][0])
                move_array[i].scr = first_killer;
            else if(m == killers[ply][1])
                move_array[i].scr = second_killer;
            else
            {
                auto fr = *it;
                auto h = history[wtm][get_index(b[fr]) - 1][m.to];
                if(h > max_history)
                    max_history = h;
                if(h < min_history)
                    min_history = h;

                auto y   = get_row(m.to);
                auto x   = get_col(m.to);
                auto y0  = get_row(*it);
                auto x0  = get_col(*it);
                if(wtm)
                {
                    y   = 7 - y;
                    y0  = 7 - y0;
                }
                auto pstVal   = pst[get_index(fr_pc) - 1][0][y][x]
                            - pst[get_index(fr_pc) - 1][0][y0][x0];
                pstVal      = 96 + pstVal/2;
                move_array[i].scr = pstVal;
            } // else (ordinary move)
        }// if(to_pc == empty_square &&
        else
        {
            auto src = streng[get_index(fr_pc)];
            auto dst = (m.flg & is_capture) ? streng[get_index(to_pc)] : 0;

            if(dst && dst - src <= 0)
            {
                auto tmp = SEE_main(m);
                dst = tmp;
                src = 0;
            }

            if(dst > 120)
            {
                move_array[i].scr = 0;
                continue;
            }

            streng_t prms[] = {0, 120, 40, 60, 40};
            if(dst <= 120 && (m.flg & is_promotion))
                dst += prms[m.flg & is_promotion];

            auto ans = dst >= src ? dst - src/16 : dst - src;

            if(dst - src >= 0)
            {
                assert(200 + ans/8 > first_killer);
                assert(200 + ans/8 <= 250);
                move_array[i].scr = (200 + ans/8);
            }
            else
            {
                if(to_black(b[*it]) != black_king)
                {
                    assert(-ans/2 >= 0);
                    assert(-ans/2 <= bad_captures);
                    move_array[i].scr = -ans/2;
                }
                else
                {
                    assert(dst/10 >= 0);
                    assert(dst/10 <= bad_captures);
                    move_array[i].scr = dst/10;
                }
            }
       }// else on captures
    }// for( i

    for(auto i = 0; i < moveCr; i++)
    {
        auto m = move_array[i];
        if(m.scr >= second_killer || (m.flg & is_capture))
            continue;
        it      = m.pc;
        auto fr   = *it;
        auto h = history[wtm][get_index(b[fr]) - 1][m.to];
        if(h > 3)
        {
            h -= min_history;
            h = 64*h / (max_history - min_history + 1);
            h += 128;
            move_array[i].scr = h;
            continue;
        }
    }// for( i
}





//--------------------------------
void k2movegen::AppriceQuiesceMoves(move_c *move_array, movcr_t moveCr)
{
    auto it = coords[wtm].begin();
    for(auto i = 0; i < moveCr; i++)
    {
        auto m = move_array[i];

        it = m.pc;
        auto fr = b[*it];
        auto pt = b[m.to];

        auto src = sort_streng[get_index(fr)];
        auto dst = (m.flg & is_capture) ? sort_streng[get_index(pt)] : 0;

        if(dst > 120)
        {
            move_array[i].scr = king_capture;
            return;
        }

        streng_t prms[] = {0, 120, 40, 60, 40};
        if(dst <= 120 && (m.flg & is_promotion))
            dst += prms[m.flg & is_promotion];

        streng_t ans;
        if(dst > src)
            ans = dst - src/16;
        else
        {
            dst = SEE_main(move_array[i]);
            src = 0;
            ans = dst;

        }
        if(dst > 120)
        {
            move_array[i].scr = 0;
            continue;
        }

        if(dst >= src)
        {
            assert(200 + ans/8 > first_killer);
            assert(200 + ans/8 <= 250);
            move_array[i].scr = (200 + ans/8);
        }
        else
        {
            if(to_black(fr) != black_king)
            {
                assert(-ans/2 >= 0);
                assert(-ans/2 <= bad_captures);
                move_array[i].scr = -ans/2;
            }
            else
            {
                assert(dst/10 >= 0);
                assert(dst/10 <= bad_captures);
                move_array[i].scr = dst/10;
            }
        }
    }
}





//-----------------------------
k2chess::streng_t k2movegen::SEE(coord_t to, streng_t frStreng, score_t val, bool stm)
{
    auto it = SeeMinAttacker(to);
    if(it == coords[!wtm].end())
        return -val;

    val -= frStreng;
    auto tmp1 = -val;
    if(wtm != stm && tmp1 < -2)
        return tmp1;

    auto storeMen = it;
    auto storeBrd = b[*storeMen];
    coords[!wtm].erase(it);
    b[*storeMen] = empty_square;
    wtm = !wtm;

    auto tmp2 = -SEE(to, streng[get_index(storeBrd)], -val, stm);

    wtm = !wtm;
    val = std::min(tmp1, tmp2);

    it = storeMen;
    coords[!wtm].restore(it);
    b[*storeMen] = storeBrd;
    return val;
}





//-----------------------------
k2chess::iterator k2movegen::SeeMinAttacker(coord_t to)
{
    shifts_t shft_l[] = {15, -17};
    shifts_t shft_r[] = {17, -15};
    piece_t  pw[] = {black_pawn, white_pawn};

    if(to + shft_l[!wtm] > 0 && b[to + shft_l[!wtm]] == pw[!wtm])
        for(auto it = coords[!wtm].begin();
            it != coords[!wtm].end();
            ++it)
            if(*it == to + shft_l[!wtm])
                return it;

    if(to + shft_r[!wtm] > 0 && b[to + shft_r[!wtm]] == pw[!wtm])
        for(auto it = coords[!wtm].begin();
            it != coords[!wtm].end();
            ++it)
            if(*it == to + shft_r[!wtm])
                return it;

    auto it = coords[!wtm].begin();
    for(; it != coords[!wtm].end(); ++it)
    {
        auto fr = *it;
        auto pt  = get_index(b[fr]);
        if(pt == get_index(black_pawn))
            continue;
        auto att = attacks[120 + to - fr] & (1 << pt);
        if(!att)
            continue;
        if(!slider[pt])
            return it;
        if(SliderAttack(to, fr))
             return it;
    }// for (menCr

    return it;
}





//-----------------------------
k2chess::streng_t k2movegen::SEE_main(move_c m)
{
    auto it = coords[wtm].begin();
    it = m.pc;
    auto fr_pc = b[*it];
    auto to_pc = b[m.to];
    auto src = streng[get_index(fr_pc)];
    auto dst = (m.flg & is_capture) ? streng[get_index(to_pc)] : 0;
    auto storeMen = coords[wtm].begin();
    storeMen = m.pc;
    auto storeBrd = b[*storeMen];
    coords[wtm].erase(storeMen);
    b[*storeMen] = empty_square;

    auto see_score = -SEE(m.to, src, dst, wtm);
    see_score = std::min(dst, see_score);

    coords[wtm].restore(storeMen);
    b[*storeMen] = storeBrd;
    return see_score;
}





//--------------------------------
void k2movegen::SortQuiesceMoves(move_c *move_array, movcr_t moveCr)
{
    if(moveCr <= 1)
        return;
    for(auto i = 0; i < moveCr; ++i)
    {
        bool swoped_around = false;
        for(auto j = 0; j < moveCr - i - 1; ++j)
        {
            if(move_array[j + 1].scr > move_array[j].scr)
            {
                std::swap(move_array[j], move_array[j + 1]);
                swoped_around = true;
            }
        }
        if(!swoped_around)
            break;
    }
}

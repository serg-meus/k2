#include "movegen.h"





//--------------------------------
Move pv[max_ply][max_ply + 1];          // the 'flg' property of first element in a row is length of PV at that depth
Move killers[max_ply][2];
unsigned history[2][6][128];
unsigned min_history, max_history;





//--------------------------------
void InitMoveGen()
{
    InitEval();
}





//--------------------------------
void PushMove(Move *move_array, movcr_t *movCr, iterator it, coord_t to,
              move_flag_t flg)
{
    Move    m;
    m.pc    = it;
    m.to    = to;
    m.flg   = flg;

    move_array[(*movCr)++] = m;
}





//--------------------------------
movcr_t GenMoves(Move *move_array, Move *best_move, u8 apprice)
{
    coord_t i, to, ray;
    movcr_t moveCr = 0;

    GenCastles(move_array, &moveCr);

    auto it = coords[wtm].begin();
    for(;it != coords[wtm].end(); ++it)
    {
        coord_t fr = *it;
        piece_index_t at = GET_INDEX(b[fr]);
        if(at == GET_INDEX(_p))
        {
            GenPawn(move_array, &moveCr, it);
            continue;
        }

        if(!slider[at])
        {
            for(ray = 0; ray < rays[at]; ray++)
            {
                to = fr + shifts[at][ray];
                if(ONBRD(to) && !LIGHT(b[to], wtm))
                    PushMove(move_array, &moveCr, it, to, b[to] ? mCAPT : 0);
            }
            continue;
        }

        for(ray = 0; ray < rays[at]; ray++)
        {
            to = fr;
            for(i = 0; i < 8; i++)
            {
                to += shifts[at][ray];
                if(!ONBRD(to))
                    break;
                piece_t tt = b[to];
                if(!tt)
                {
                    PushMove(move_array, &moveCr, it, to, 0);
                    continue;
                }
                if(DARK(b[to], wtm))
                    PushMove(move_array, &moveCr, it, to, mCAPT);
                break;
            }// for(i
        }// for(ray
    }// for(it
    if(apprice == APPRICE_ALL)
        AppriceMoves(move_array, moveCr, best_move);
    else if(apprice == APPRICE_CAPT)
        AppriceQuiesceMoves(move_array, moveCr);

    return moveCr;
}





//--------------------------------
void GenPawn(Move *move_array, movcr_t *moveCr, iterator it)
{
    coord_t to;
    move_flag_t pBeg, pEnd;
    coord_t fr = *it;
    if((wtm && ROW(fr) == 6) || (!wtm && ROW(fr) == 1))
    {
        pBeg = mPR_Q;
        pEnd = mPR_B;
    }
    else
    {
        pBeg = 0;
        pEnd = 0;
    }
    for(size_t i = pBeg; i <= pEnd; ++i)
        if(wtm)
        {
            to = fr + 17;
            if(ONBRD(to) && DARK(b[to], wtm))
                PushMove(move_array, moveCr, it, to, mCAPT | i);
            to = fr + 15;
            if(ONBRD(to) && DARK(b[to], wtm))
                PushMove(move_array, moveCr, it, to, mCAPT | i);
            to = fr + 16;
            if(ONBRD(to) && !b[to])                 // ONBRD not needed
                PushMove(move_array, moveCr, it, to, i);
            if(ROW(fr) == 1 && !b[fr + 16] && !b[fr + 32])
                PushMove(move_array, moveCr, it, fr + 32, 0);
            u8 ep = b_state[prev_states + ply].ep;
            i8 delta = ep - 1 - COL(fr);
            if(ep && ABSI(delta) == 1 && ROW(fr) == 4)
                PushMove(move_array, moveCr, it, fr + 16 + delta, mCAPT | mENPS);
        }
        else
        {
            to = fr - 17;
            if(ONBRD(to) && DARK(b[to], wtm))
                PushMove(move_array, moveCr, it, to, mCAPT | i);
            to = fr - 15;
            if(ONBRD(to) && DARK(b[to], wtm))
                PushMove(move_array, moveCr, it, to, mCAPT | i);
            to = fr - 16;
            if(ONBRD(to) && !b[to])                  // ONBRD not needed
                PushMove(move_array, moveCr, it, to, i);
            if(ROW(fr) == 6 && !b[fr - 16] && !b[fr - 32])
                PushMove(move_array, moveCr, it, fr - 32, 0);
            u8 ep = b_state[prev_states + ply].ep;
            i8 delta = ep - 1 - COL(fr);
            if(ep && ABSI(delta) == 1 && ROW(fr) == 3)
                PushMove(move_array, moveCr, it, fr - 16 + delta, mCAPT | mENPS);
        }
}





//--------------------------------
void GenPawnCap(Move *move_array, movcr_t *moveCr, iterator it)
{
    coord_t to;
    size_t pBeg, pEnd;
    coord_t fr = *it;
    if((wtm && ROW(fr) == 6) || (!wtm && ROW(fr) == 1))
    {
        pBeg = mPR_Q;
        pEnd = mPR_Q;
    }
    else
    {
        pBeg = 0;
        pEnd = 0;
    }
    for(size_t i = pBeg; i <= pEnd; ++i)
        if(wtm)
        {
            to = fr + 17;
            if(ONBRD(to) && DARK(b[to], wtm))
                PushMove(move_array, moveCr, it, to, mCAPT | i);
            to = fr + 15;
            if(ONBRD(to) && DARK(b[to], wtm))
                PushMove(move_array, moveCr, it, to, mCAPT | i);
            to = fr + 16;
            if(pBeg && !b[to])
                PushMove(move_array, moveCr, it, to, i);
            u8 ep = b_state[prev_states + ply].ep;
            i8 delta = ep - 1 - COL(fr);
            if(ep && ABSI(delta) == 1 && ROW(fr) == 4)
                PushMove(move_array, moveCr, it, fr + 16 + delta, mCAPT | mENPS);
        }
        else
        {
            to = fr - 17;
            if(ONBRD(to) && DARK(b[to], wtm))
                PushMove(move_array, moveCr, it, to, mCAPT | i);
            to = fr - 15;
            if(ONBRD(to) && DARK(b[to], wtm))
                PushMove(move_array, moveCr, it, to, mCAPT | i);
            to = fr - 16;
            if(pBeg && !b[to])
                PushMove(move_array, moveCr, it, to, i);
            u8 ep = b_state[prev_states + ply].ep;
            i8 delta = ep - 1 - COL(fr);
            if(ep && ABSI(delta) == 1 && ROW(fr) == 3)
                PushMove(move_array, moveCr, it, fr - 16 + delta, mCAPT | mENPS);
        }
}





//--------------------------------
void GenCastles(Move *move_array, movcr_t *moveCr)
{
    u8 msk = wtm ? 0x03 : 0x0C;
    u8 cst = b_state[prev_states + ply].cstl & msk;
    i8 check = -1;
    if(!cst)
        return;
    if(wtm)
    {
        if((cst & 1) && !b[0x05] && !b[0x06])
        {
            check = Attack(0x04, black);
            if(!check && !Attack(0x05, black) && !Attack(0x06, black))
                PushMove(move_array, moveCr,
                         king_coord[white], 0x06, mCS_K);
        }
        if((cst & 2) && !b[0x03] && !b[0x02] && !b[0x01])
        {
            if(check == -1)
                check = Attack(0x04, black);
            if(!check && !Attack(0x03, black) && !Attack(0x02, black))
                PushMove(move_array, moveCr,
                         king_coord[white], 0x02, mCS_Q);
        }
    }
    else
    {
        if((cst & 4) && !b[0x75] && !b[0x76])
        {
            check = Attack(0x74, white);
            if(!check && !Attack(0x75, white) && !Attack(0x76, white))
                PushMove(move_array, moveCr,
                         king_coord[black], 0x76, mCS_K);
        }
        if((cst & 8) && !b[0x73] && !b[0x72] && !b[0x71])
        {
            if(check == -1)
                check = Attack(0x74, white);
            if(!check && !Attack(0x73, white) && !Attack(0x72, white))
                PushMove(move_array, moveCr,
                         king_coord[black], 0x72, mCS_Q);
        }
    }
}





//--------------------------------
movcr_t GenCaptures(Move *move_array)
{
    coord_t i, to, ray;
    movcr_t moveCr = 0;
    auto it = coords[wtm].begin();
    for(; it != coords[wtm].end(); ++it)
    {
        coord_t fr = *it;
        piece_index_t at = GET_INDEX(b[fr]);
        if(at == GET_INDEX(_p))
        {
            GenPawnCap(move_array, &moveCr, it);
            continue;
        }
        if(!slider[at])
        {
            for(ray = 0; ray < rays[at]; ray++)
            {
                to = fr + shifts[at][ray];
                if(ONBRD(to) && DARK(b[to], wtm))
                    PushMove(move_array, &moveCr, it, to, mCAPT);
            }
            continue;
        }

        for(ray = 0; ray < rays[at]; ray++)
        {
            to = fr;
            for(i = 0; i < 8; i++)
            {
                to += shifts[at][ray];
                if(!ONBRD(to))
                    break;
                piece_t tt = b[to];
                if(!tt)
                    continue;
                if(DARK(tt, wtm))
                    PushMove(move_array, &moveCr, it, to, mCAPT);
                break;
            }// for(i
        }// for(ray
    }// for(pc
    AppriceQuiesceMoves(move_array, moveCr);
    SortQuiesceMoves(move_array, moveCr);
    return moveCr;
}





//--------------------------------
void AppriceMoves(Move *move_array, movcr_t moveCr, Move *bestMove)
{
#ifndef DONT_USE_HISTORY
    min_history = UINT_MAX;
    max_history = 0;
#endif

    auto it = coords[wtm].begin();
    Move bm = *move_array;
    if(bestMove == nullptr)
        bm.flg = 0xFF;
    else
        bm = *bestMove;
    for(movcr_t i = 0; i < moveCr; i++)
    {
        Move m = move_array[i];

        it = m.pc;
        piece_t fr_pc = b[*it];
        piece_t to_pc = b[m.to];

        if(m == bm)
            move_array[i].scr = PV_FOLLOW;
        else
        if(to_pc == __ && !(m.flg & mPROM))
        {
            if(m == killers[ply][0])
                move_array[i].scr = FIRST_KILLER;
            else if(m == killers[ply][1])
                move_array[i].scr = SECOND_KILLER;
            else
            {
#ifndef DONT_USE_HISTORY
                coord_t fr = *it;
                unsigned h = history[wtm][GET_INDEX(b[fr]) - 1][m.to];
                if(h > max_history)
                    max_history = h;
                if(h < min_history)
                    min_history = h;
#endif // DONT_USE_HISTORY
                coord_t y   = ROW(m.to);
                coord_t x   = COL(m.to);
                coord_t y0  = ROW(*it);
                coord_t x0  = COL(*it);
                if(wtm)
                {
                    y   = 7 - y;
                    y0  = 7 - y0;
                }
                i8 pstVal   = pst[GET_INDEX(fr_pc) - 1][0][y][x]
                            - pst[GET_INDEX(fr_pc) - 1][0][y0][x0];
                pstVal      = 96 + pstVal/2;
                move_array[i].scr = pstVal;
            } // else (ordinary move)
        }// if(to_pc == __ &&
        else
        {
            streng_t ans;
            streng_t src = streng[GET_INDEX(fr_pc)];
            streng_t dst = (m.flg & mCAPT) ? streng[GET_INDEX(to_pc)] : 0;

#ifndef DONT_USE_SEE_SORTING
            if(dst && dst - src <= 0)
            {
                streng_t tmp = SEE_main(m);
                dst = tmp;
                src = 0;
            }
#else
            if(src > 120)
            {
                move_array[i].scr = EQUAL_CAPTURE;
                continue;
            }
            else
#endif // DONT_USE_SEE_SORTING
            if(dst > 120)
            {
                move_array[i].scr = 0;
                continue;
            }

            streng_t prms[] = {0, 120, 40, 60, 40};
            if(dst <= 120 && (m.flg & mPROM))
                dst += prms[m.flg & mPROM];

            if(dst >= src)
                ans = dst - src/16;
            else
                ans = dst - src;

            if(dst - src >= 0)
            {
                assert(200 + ans/8 > FIRST_KILLER);
                assert(200 + ans/8 <= 250);
                move_array[i].scr = (200 + ans/8);
            }
            else
            {
                if(TO_BLACK(b[*it]) != _k)
                {
                    assert(-ans/2 >= 0);
                    assert(-ans/2 <= BAD_CAPTURES);
                    move_array[i].scr = -ans/2;
                }
                else
                {
                    assert(dst/10 >= 0);
                    assert(dst/10 <= BAD_CAPTURES);
                    move_array[i].scr = dst/10;
                }
            }
       }// else on captures
    }// for( i

#ifndef DONT_USE_HISTORY
    for(movcr_t i = 0; i < moveCr; i++)
    {
        Move m = move_array[i];
        if(m.scr >= std::min(MOVE_FROM_PV, SECOND_KILLER)
        || (m.flg & mCAPT))
            continue;
        it      = m.pc;
        coord_t fr   = *it;
        unsigned h = history[wtm][GET_INDEX(b[fr]) - 1][m.to];
        if(h > 3)
        {
            h -= min_history;
            h = 64*h / (max_history - min_history + 1);
            h += 128;
            move_array[i].scr = h;
            continue;
        }
    }// for( i
#endif
}





//--------------------------------
void AppriceQuiesceMoves(Move *move_array, movcr_t moveCr)
{
    auto it = coords[wtm].begin();
    for(movcr_t i = 0; i < moveCr; i++)
    {
        Move m = move_array[i];

        it = m.pc;
        coord_t fr = b[*it];
        piece_t pt = b[m.to];

        streng_t src = sort_streng[GET_INDEX(fr)];
        streng_t dst = (m.flg & mCAPT) ? sort_streng[GET_INDEX(pt)] : 0;

        if(dst > 120)
        {
            move_array[i].scr = KING_CAPTURE;
            return;
        }

        streng_t prms[] = {0, 120, 40, 60, 40};
        if(dst <= 120 && (m.flg & mPROM))
            dst += prms[m.flg & mPROM];

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
            assert(200 + ans/8 > FIRST_KILLER);
            assert(200 + ans/8 <= 250);
            move_array[i].scr = (200 + ans/8);
        }
        else
        {
            if(TO_BLACK(fr) != _k)
            {
                assert(-ans/2 >= 0);
                assert(-ans/2 <= BAD_CAPTURES);
                move_array[i].scr = -ans/2;
            }
            else
            {
                assert(dst/10 >= 0);
                assert(dst/10 <= BAD_CAPTURES);
                move_array[i].scr = dst/10;
            }
        }
    }
}





//-----------------------------
streng_t SEE(coord_t to, streng_t frStreng, score_t val, bool stm)
{
    auto it = SeeMinAttacker(to);
    if(it == coords[!wtm].end())
        return -val;

    val -= frStreng;
    score_t tmp1 = -val;
    if(wtm != stm && tmp1 < -2)
        return tmp1;

    auto storeMen = it;
    piece_t storeBrd = b[*storeMen];
    coords[!wtm].erase(it);
    b[*storeMen] = __;
    wtm = !wtm;

    streng_t tmp2 = -SEE(to, streng[GET_INDEX(storeBrd)], -val, stm);

    wtm = !wtm;
    val = std::min(tmp1, tmp2);

    it = storeMen;
    coords[!wtm].restore(it);
    b[*storeMen] = storeBrd;
    return val;
}





//-----------------------------
iterator SeeMinAttacker(coord_t to)
{
    i8 shft_l[] = {15, -17};
    i8 shft_r[] = {17, -15};
    piece_t  pw[] = {_p, _P};

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
        coord_t fr = *it;
        piece_index_t pt  = GET_INDEX(b[fr]);
        if(pt == GET_INDEX(_p))
            continue;
        u8 att = attacks[120 + to - fr] & (1 << pt);
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
streng_t SEE_main(Move m)
{
    auto it = coords[wtm].begin();
    it = m.pc;
    piece_t fr_pc = b[*it];
    piece_t to_pc = b[m.to];
    streng_t src = streng[GET_INDEX(fr_pc)];
    streng_t dst = (m.flg & mCAPT) ? streng[GET_INDEX(to_pc)] : 0;
    auto storeMen = coords[wtm].begin();
    storeMen = m.pc;
    piece_t storeBrd = b[*storeMen];
    coords[wtm].erase(storeMen);
    b[*storeMen] = __;

    streng_t see_score = -SEE(m.to, src, dst, wtm);
    see_score = std::min(dst, see_score);

    coords[wtm].restore(storeMen);
    b[*storeMen] = storeBrd;
    return see_score;
}





//--------------------------------
void SortQuiesceMoves(Move *move_array, movcr_t moveCr)
{
    if(moveCr <= 1)
        return;
    for(movcr_t i = 0; i < moveCr; ++i)
    {
        bool swoped_around = false;
        for(movcr_t j = 0; j < moveCr - i - 1; ++j)
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



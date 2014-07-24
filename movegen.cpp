#include "movegen.h"

//--------------------------------
Move *ml;
int moveCr;
Move pv[MAX_PLY][MAX_PLY + 1];          // first element in a row is length of PV at that depth
Move kil[MAX_PLY][2];
//unsigned history [2][6][128];
bool genBestMove;
Move bestMoveToGen;

//--------------------------------
void InitMoveGen()
{
    InitHashTable();
}

//--------------------------------
int GenMoves(Move *list, int apprice)
{
    int i, fr, to, ray;

    moveCr = 0;
    ml = list;
    GenCastles();
    UC menNum   = wtm;
    menNum      = menNxt[menNum];
    unsigned pcCr = 0;
    unsigned maxPc = pieces[wtm >> 5];
    for(; pcCr < maxPc; pcCr++)
    {
        fr = men[menNum];


        UC at = b[fr] & 7;
        if(at == _p)
        {
            GenPawn(menNum);
            menNum = menNxt[menNum];
            continue;
        }

        if(!slider[at])
        {
            for(ray = 0; ray < rays[at]; ray++)
            {
                to = fr + shifts[at][ray];
                if(ONBRD(to) && !LIGHT(b[to], wtm))
                    PUSH_MOVE(menNum, to, b[to] ? mCAPT : 0);
            }
            menNum = menNxt[menNum];

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
                UC tt = b[to];
                if(!tt)
                {
                    PUSH_MOVE(menNum, to, 0);
                    continue;
                }
                if(DARK(b[to], wtm))
                    PUSH_MOVE(menNum, to, mCAPT);
                break;
            }// for(i
        }// for(ray
        menNum = menNxt[menNum];
    }// for(pc
    if(apprice == APPRICE_ALL)
        AppriceMoves(list);
    else if(apprice == APPRICE_CAPT)
        AppriceQuiesceMoves(list);
//   AppriceHistory(list);
    return moveCr;
}

//--------------------------------
void GenPawn(UC pc)
{
    unsigned to, pBeg, pEnd;
    UC fr = men[pc];
    if((wtm && ROW(fr) == 6) || (!wtm && ROW(fr) == 1))
    {
        pBeg = mPR_Q;
        pEnd = mPR_B;   //>> ply <= 2 ? mPR_B : mPR_Q;
    }
    else
    {
        pBeg = 0;
        pEnd = 0;
    }
    for(unsigned i = pBeg; i <= pEnd; i += 0x010000)
        if(wtm)
        {
            to = fr + 17;
            if(ONBRD(to) && DARK(b[to], wtm))
                PUSH_MOVE(pc, to, mCAPT | i);
            to = fr + 15;
            if(ONBRD(to) && DARK(b[to], wtm))
                PUSH_MOVE(pc, to, mCAPT | i);
            to = fr + 16;
            if(ONBRD(to) && !b[to])                 // ONBRD íàõðåíà íàïðèìåð?
                PUSH_MOVE(pc, to, i);
            if(ROW(fr) == 1 && !b[fr + 16] && !b[fr + 32])
                PUSH_MOVE(pc, fr + 32, 0);
            int ep = boardState[PREV_STATES + ply].ep;
            int delta = ep - 1 - COL(fr);
            if(ep && ABSI(delta) == 1 && ROW(fr) == 4)
                PUSH_MOVE(pc, fr + 16 + delta, mCAPT | mENPS);
        }
        else
        {
            to = fr - 17;
            if(ONBRD(to) && DARK(b[to], wtm))
                PUSH_MOVE(pc, to, mCAPT | i);
            to = fr - 15;
            if(ONBRD(to) && DARK(b[to], wtm))
                PUSH_MOVE(pc, to, mCAPT | i);
            to = fr - 16;
            if(ONBRD(to) && !b[to])                  // ONBRD íàõðåíà íàïðèìåð?
                PUSH_MOVE(pc, to, i);
            if(ROW(fr) == 6 && !b[fr - 16] && !b[fr - 32])
                PUSH_MOVE(pc, fr - 32, 0);
            int ep = boardState[PREV_STATES + ply].ep;
            int delta = ep - 1 - COL(fr);
            if(ep && ABSI(delta) == 1 && ROW(fr) == 3)
                PUSH_MOVE(pc, fr - 16 + delta, mCAPT | mENPS);
        }
}

//--------------------------------
void GenPawnCap(UC pc)
{
    unsigned to, pBeg, pEnd;
    UC fr = men[pc];
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
    for(unsigned i = pBeg; i <= pEnd; i += 0x010000)
        if(wtm)
        {
            to = fr + 17;
            if(ONBRD(to) && DARK(b[to], wtm))
                PUSH_MOVE(pc, to, mCAPT | i);
            to = fr + 15;
            if(ONBRD(to) && DARK(b[to], wtm))
                PUSH_MOVE(pc, to, mCAPT | i);
            to = fr + 16;
            if(pBeg && !b[to])
                PUSH_MOVE(pc, to, i);
            int ep = boardState[PREV_STATES + ply].ep;
            int delta = ep - 1 - COL(fr);
            if(ep && ABSI(delta) == 1 && ROW(fr) == 4)
                PUSH_MOVE(pc, fr + 16 + delta, mCAPT | mENPS);
        }
        else
        {
            to = fr - 17;
            if(ONBRD(to) && DARK(b[to], wtm))
                PUSH_MOVE(pc, to, mCAPT | i);
            to = fr - 15;
            if(ONBRD(to) && DARK(b[to], wtm))
                PUSH_MOVE(pc, to, mCAPT | i);
            to = fr - 16;
            if(pBeg && !b[to])
                PUSH_MOVE(pc, to, i);
            int ep = boardState[PREV_STATES + ply].ep;
            int delta = ep - 1 - COL(fr);
            if(ep && ABSI(delta) == 1 && ROW(fr) == 3)
                PUSH_MOVE(pc, fr - 16 + delta, mCAPT | mENPS);
        }
}

//--------------------------------
void GenCastles()
{
    UC msk = wtm ? 0x03 : 0x0C;
    UC cst = boardState[PREV_STATES + ply].cstl & msk;
    int check = -1;
    if(!cst)
        return;
    if(wtm)
    {
        if((cst & 1) && !b[0x05] && !b[0x06])
        {
            check = Attack(0x04, BLK);
            if(!check && !Attack(0x05, BLK) && !Attack(0x06, BLK))
                PUSH_MOVE(WHT + 1, 0x06, mCS_K);
        }
        if((cst & 2) && !b[0x03] && !b[0x02] && !b[0x01])
        {
            if(check == -1)
                check = Attack(0x04, BLK);
            if(!check && !Attack(0x03, BLK) && !Attack(0x02, BLK))
                PUSH_MOVE(WHT + 1, 0x02, mCS_Q);
        }
    }
    else
    {
        if((cst & 4) && !b[0x75] && !b[0x76])
        {
            check = Attack(0x74, WHT);
            if(!check && !Attack(0x75, WHT) && !Attack(0x76, WHT))
                PUSH_MOVE(BLK + 1, 0x76, mCS_K);
        }
        if((cst & 8) && !b[0x73] && !b[0x72] && !b[0x71])
        {
            if(check == -1)
                check = Attack(0x74, WHT);
            if(!check && !Attack(0x73, WHT) && !Attack(0x72, WHT))
                PUSH_MOVE(BLK + 1, 0x72, mCS_Q);
        }
    }
}

//--------------------------------
int GenCaptures(Move *list)
{
    int i, fr, to, ray;
    moveCr = 0;
    ml = list;

    UC menNum   = wtm;

    unsigned pcCr = 0;
    unsigned maxPc = pieces[wtm >> 5];

    for(; pcCr < maxPc; pcCr++)
    {
        menNum  = menNxt[menNum];
        fr      = men[menNum];

        UC at = b[fr] & 7;
        if(at == _p)
        {
            GenPawnCap(menNum);
            continue;
        }
        if(!slider[at])
        {
            for(ray = 0; ray < rays[at]; ray++)
            {
                to = fr + shifts[at][ray];
                if(ONBRD(to) && DARK(b[to], wtm))
                    PUSH_MOVE(menNum, to, mCAPT);
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
                UC tt = b[to];
                if(!tt)
                    continue;
                if(DARK(tt, wtm))
                    PUSH_MOVE(menNum, to, mCAPT);
                break;
            }// for(i
        }// for(ray
    }// for(pc
    AppriceQuiesceMoves(list);
    return moveCr;
}

//--------------------------------
void AppriceMoves(Move *list)
{
    for(int i = 0; i < moveCr; i++)
    {
        Move m = list[i];
        UC fr = b[men[MOVEPC(m)]] & (WHT - 1);
        UC pt = b[MOVETO(m)] & (WHT - 1);

        if(genBestMove && m == (bestMoveToGen & EXCEPT_SCORE))
            list[i] |= (PV_FOLLOW << MOVE_SCORE_SHIFT);
        else if(pt == __ && !(MOVEFLG(m) & mPROM))
        {
            if(m == (kil[ply][0] & EXCEPT_SCORE))
                list[i] |= (FIRST_KILLER << MOVE_SCORE_SHIFT);
            else if(m == (kil[ply][1] & EXCEPT_SCORE))
                list[i] |= (SECOND_KILLER << MOVE_SCORE_SHIFT);
            else if(m == (pv[ply][1] & EXCEPT_SCORE))
                list[i] |= (MOVE_FROM_PV << MOVE_SCORE_SHIFT);
            else
            {
                int y   = ROW(MOVETO(m));
                int x   = COL(MOVETO(m));
                int y0  = ROW(men[MOVEPC(m)]);
                int x0  = COL(men[MOVEPC(m)]);
                if(wtm)
                {
                    y   = 7 - y;
                    y0  = 7 - y0;
                }
                int pstVal  = pst[fr - 1][0][y][x] - pst[fr - 1][0][y0][x0];
                pstVal      = (pstVal >> 1) + 32 + 16;
                list[i] |= (pstVal << MOVE_SCORE_SHIFT);
            }
/*            else
            {
                movScr[ply][i] = 0;
                UC fr = men[m.pc);
                unsigned h = history[!wtm][(b[fr] & 0x0F) - 1][m.to];
                if(h > maxHist)
                    maxHist = h;
            }*/
        }
        else
        {
            int ans;
            int src = streng[fr];
            int dst = (MOVEFLG(m) & mCAPT) ? streng[pt] : 0;

/*           if(dst && dst < src)
            {
                UC storeMen = men[m.pc];
                UC storeBrd = b[storeMen];
                men[m.pc]   = 0xFF;
                b[storeMen] = __;
                short tmp = -SEE(m.to, src, dst);
                if(tmp > 0)
                    tmp >>= 1;
                dst = tmp;
                src = 0;
                men[m.pc]   = storeMen;
                b[storeMen] = storeBrd;
            }
*/
            if(src > 120)
            {
                list[i] |= (EQUAL_CAPTURE << MOVE_SCORE_SHIFT);
                continue;
            }
            else if(dst > 120)
            {
                list[i] |= (KING_CAPTURE << MOVE_SCORE_SHIFT);
                return;
            }

            ans = dst - src/16;
            short prms[] = {0, 1200, 400, 600, 400};
            if(MOVEFLG(m) & mPROM)
                ans += prms[(MOVEFLG(m) & mPROM) >> 16];

            if(ans > 0)
                list[i] |= ((0x80 + ans/10) << MOVE_SCORE_SHIFT);
            else if(ans < 0)
            {
                if(fr != _k)
                    list[i] |= ((11 + ans/10) << MOVE_SCORE_SHIFT);
            }
            else
                list[i] |= (EQUAL_CAPTURE << MOVE_SCORE_SHIFT);
        }
    }
}

//--------------------------------
void AppriceQuiesceMoves(Move *list)
{
    for(int i = 0; i < moveCr; i++)
    {
        Move m = list[i];
        UC fr = b[men[MOVEPC(m)]] & (WHT - 1);
        UC pt = b[MOVETO(m)] & (WHT - 1);

        int ans;
        int src = streng[fr];
        int dst = (MOVEFLG(m) & mCAPT) ? streng[pt] : 0;
        if(src > 120)
            continue;
        else if(dst > 120)
        {
            list[i] |= (KING_CAPTURE << MOVE_SCORE_SHIFT);
            return;
        }

        ans = dst - src;
        short prms[] = {0, 1200, 400, 600, 400};
        if(dst <= 1200 && (MOVEFLG(m) & mPROM))
            ans += prms[(MOVEFLG(m) & mPROM) >> 16];

        if(ans > 0)
            list[i] |= ((0x80 + ans/10) << MOVE_SCORE_SHIFT);
        else if(ans < 0)
        {
            if(fr != _k)
                list[i] |= ((11 + ans/10) << MOVE_SCORE_SHIFT);
        }
        else
            list[i] |= (EQUAL_CAPTURE << MOVE_SCORE_SHIFT);

    }

}

//--------------------------------
void Next(Move *list, int cur, int top, Move *ans)
{

    int max = -32000, imx = cur;

    for(int i = cur; i < top; i++)
    {
        UC sc = MOVESCR(list[i]);
        if(sc > max)
        {
            max = sc;
            imx = i;
        }
    }
    *ans = list[imx];
    if(imx != cur)
    {
        list[imx] = list[cur];
        list[cur] = *ans;
    }
}

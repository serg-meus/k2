 #include "engine.h"

//--------------------------------
char        stop_str[] = "";
unsigned    stopPly   = 4;

Timer       timer;
double      time0;
double      timeSpent, timeToThink;
unsigned    timeMaxNodes, timeMaxPly;
unsigned    rootPly, rootTop, rootMoveCr;
bool        _abort_, stop, analyze, busy;
Move        rootMoveList[MAX_MOVES];
UQ          qnodes, cutCr, cutNumCr[8], qCutCr, qCutNumCr[8], futCr;
UQ          totalNodes;
double      timeBase, timeInc, timeRemains, totalTimeSpent;
unsigned    movesPerSession;
int         halfMovCr, movsRemain;
Move        doneMoves[USELESS_HALFMOVES_TO_DRAW];
bool        spentExactTime;
unsigned    resignCr;

#ifdef CHECK_PREDICTED_VALUE
    PredictedInfo prediction;
#endif

#ifdef TUNE_PARAMETERS

std::vector <float> tuning_vec;

#endif // TUNE_PARAMETERS

//--------------------------------
short Search(int depth, short alpha, short beta)
{
    if(ply >= MAX_PLY - 1 || DrawDetect())
    {
        pv[ply][0] = 0;
        return 0;
    }
    bool ic = Attack(men[wtm + 1], wtm ^ WHT);
    if(ic)
        depth++;
#ifdef USE_FTL
    if(depth <= 1 && depth >= 0 && !ic
    && boardState[PREV_STATES + ply].capt == 0
    && boardState[PREV_STATES + ply - 1].to   != 0xFF
    && beta < K_VAL - MAX_PLY)
    {
        short margin = depth == 0 ? 350 : 550;
        margin += beta;
        if((wtm  &&  valOpn > margin &&  valEnd > margin)
        || (!wtm && -valOpn > margin && -valEnd > margin))
        {
            if(depth == 1)
                ply = ply;
            futCr++;
            return beta;
        }
    }
#endif
    if(depth <= 0)
    {
        return Quiesce(alpha, beta);
    }
    nodes++;
    if(nodes % 500 == 0)
        CheckForInterrupt();

#ifdef USE_NMR
    if(NullMove(depth, beta, ic))
        return beta;
#endif

//    short _alpha = alpha;
    Move moveList[MAX_MOVES];

    int i = 0, legals = 0, top = GenMoves(moveList, APPRICE_ALL);

    Move m;
    short x;

    boardState[PREV_STATES + ply].valOpn = valOpn;
    boardState[PREV_STATES + ply].valEnd = valEnd;

    for(; i < top && !stop && alpha < K_VAL - MAX_PLY + 1; i++)
    {
/*#ifndef NDEBUG
        UC c[24 + 128 + 16 + 64 + 64 + 240];                //<< NB: wrong
        memcpy(c, unused1, 24 + 128 + 16 + 64 + 64 + 240);  //<< NB: wrong
#endif*/
        Next(moveList, i, top, &m);
        MkMove(m);
#ifndef NDEBUG
        if((!stopPly || rootPly == stopPly) && strcmp(stop_str, cv) == 0)
            ply = ply;
#endif
        if(!Legal(m, ic))
        {
            UnMove(m);
            continue;
        }

        FastEval(m);

#ifdef USE_PVS
        if(legals && depth > 1 && beta != alpha + 1 && ABSI(alpha) < K_VAL - MAX_PLY + 1)
        {
            x = -Search(depth - 1 /*- lmr*/, -alpha - 1, -alpha);
            if(x > alpha)
                x = -Search(depth - 1, -beta, -alpha);
        }
        else
            x = -Search(depth - 1 /*- lmr*/, -beta, -alpha);
#else
        x = -Search(depth - 1, -beta, -alpha);
#endif
        followPV = false;

        legals++;

        if(x >= beta)
            i += 999;
        else if(x > alpha)
        {
            alpha = x;
            StorePV(m);
        }
        UnMove(m);
/*#ifndef NDEBUG
        if(memcmp(unused1, c, 24 + 128 + 16 + 64 + 64 + 240) != 0)
            ply = ply;
#endif*/
    }// for i

    if(!legals && alpha < K_VAL - MAX_PLY + 1)
    {
        pv[ply][0] = 0;
        return ic ? -K_VAL + ply : 0;
    }
    if(i > top + 1)
    {
        UpdateStatistics(m, depth, i);
        return beta;
    }
    return alpha;
}

//-----------------------------
short Quiesce(short alpha, short beta)
{
    if(ply >= MAX_PLY - 1)
        return 0;
    nodes++;
    qnodes++;
    if(nodes % 500 == 0)
        CheckForInterrupt();

    pv[ply][0] = 0;

    boardState[PREV_STATES + ply].valOpn = valOpn;
    boardState[PREV_STATES + ply].valEnd = valEnd;

    short x = Eval(/*alpha, beta*/);

    if(-x >= beta)
        return beta;
    else if(-x > alpha)
        alpha = -x;

    Move moveList[MAX_MOVES];
    unsigned i = 0, top = GenCaptures(moveList);

    for(; i < top && !stop; i++)
    {
/*#ifndef NDEBUG
        UC c[24 + 128 + 16 + 64 + 64 + 240];
        memcpy(c, unused1, 24 + 128 + 16 + 64 + 64 + 240);
#endif*/
        Move m;
        Next(moveList, i, top, &m);

        MkMove(m);
#ifndef NDEBUG
        if((!stopPly || rootPly == stopPly) && strcmp(stop_str, cv) == 0)
            ply = ply;
#endif
        if((boardState[PREV_STATES + ply].capt & ~WHT) == _k)
        {
            UnMove(m);
            return K_VAL;
        }
        FastEval(m);


        x = -Quiesce(-beta, -alpha);

        if(x >= beta)
            i += 999;
        else if(x > alpha)
        {
            alpha   = x;
            StorePV(m);
        }
        UnMove(m);
/*#ifndef NDEBUG
        if(memcmp(unused1, c, 24 + 128 + 16 + 64 + 64 + 240) != 0)
            ply = ply;
#endif*/
    }// for(i

    if(i > top + 1)
    {
        qCutCr++;
        if(i < 1000 + sizeof(qCutNumCr)/sizeof(*qCutNumCr))
            qCutNumCr[i - 1000]++;
        return beta;
    }
    return alpha;
}

//--------------------------------
void Perft(int depth)
{
    Move moveList[MAX_MOVES];
    bool ic = Attack(men[wtm + 1], wtm ^ WHT);
    GenMoves(moveList, APPRICE_NONE);
    int top = moveCr;
    for(int i = 0; i < top; i++)
    {
/*#ifndef NDEBUG
        UC c[24 + 128 + 16 + 64 + 64 + 240];
        memcpy(c, unused1, 24 + 128 + 16 + 64 + 64 + 240);
#endif*/
/*#ifndef NDEBUG
        UQ tmpCr;
        if(depth == PERFT_DEPTH)
            tmpCr = nodes;
#endif*/
        Move m = moveList[i];
        MkMove(m);
#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0)
            ply = ply;
#endif
//        bool legal = !Attack(men[(wtm ^ WHT) + 1], wtm);
        bool legal = Legal(m, ic);
        if(depth > 1 && legal)
            Perft(depth - 1);
        if(depth == 1 && legal)
            nodes++;
/*#ifndef NDEBUG
        if(depth == PERFT_DEPTH && legal)
           std::cout << cv << nodes - tmpCr << std::endl;
#endif*/
        UnMove(m);
/*#ifndef NDEBUG
        if(memcmp(unused1, c, 24 + 128 + 16 + 64 + 64 + 240) != 0)
            ply = ply;
#endif*/
    }
}

//-----------------------------
void StorePV(Move m)
{
    int nextLen = pv[ply][0];
    pv[ply - 1][0] = nextLen + 1;
    pv[ply - 1][1] = m;
    memcpy(&pv[ply - 1][2], &pv[ply][1], nextLen << 2);
}

//-----------------------------
void UpdateStatistics(Move m, int dpt, unsigned i)
{
    cutCr++;
    if(i < 1000 + sizeof(cutNumCr)/sizeof(*cutNumCr))
        cutNumCr[i - 1000]++;
    if(MOVEFLG(m))
        return;
    if(m != kil[ply][0] && m != kil[ply][1])
    {
        kil[ply][1] = kil[ply][0];
        kil[ply][0] = m;
    }

/*    UC fr = men[m.pc];
    unsigned &h = history[!wtm][(b[fr] & 0x0F) - 1][m.to];
    h += dpt*dpt + 1;*/
}

//--------------------------------
void MainSearch()
{
    busy = true;
//    std::cout << "( busy = true ) " << std::endl;
    InitSearch();

    short sc = 0, _sc_;
    rootPly = /*prevRootPly > 2 ? prevRootPly - 1 :*/ 1;
//    if(rootPly > timeMaxPly)
//        rootPly = 1;
    rootTop = 0;
    for(; rootPly <= MAX_PLY && !stop; ++rootPly)
    {
        if(rootPly == 6)
            ply = ply;

        followPV = true;
        _sc_     = sc;

        sc = RootSearch(rootPly, -INF, INF);

#ifdef CHECK_PREDICTED_VALUE

        if(!stop && !_abort_ && ABSI(sc) < K_VAL - MAX_PLY
        && (prediction.state == 4 || prediction.state == 1)
        && rootPly == prediction.depth - 2)
        {
            if((prediction.state == 4 && sc != prediction.score)
            || (prediction.state == 1 && sc < prediction.score))
            {
                std::cout << "tellusererror err03: wrong predicted score" << std::endl;
                std::cout << "resign" << std::endl;
                stop = true;
            }
            prediction.state = 1;
        }
#endif // CHECK_PREDICTED_VALUE

        if(stop && rootMoveCr <= 2)
        {
            rootPly--;
            sc = _sc_;
        }
        double time1 = timer.getElapsedTimeInMicroSec();
        timeSpent = time1 - time0;

        if(!analyze)
        {
            if(timeSpent > timeToThink && !timeMaxNodes)
                break;
            if(ABSI(sc) > K_VAL - MAX_PLY && !stop)
                break;
            if(rootTop == 1 && rootPly >= 5 )
                break;
        }
        if(rootPly >= timeMaxPly)
            break;

    }// for(rootPly
//    if(!analyze)
//        prevRootPly = rootPly;
    if(sc < -RESIGN_VALUE)
        resignCr++;
    else
        resignCr = 0;

    if((!stop && !analyze && sc < -K_VAL + MAX_PLY)
    || (!analyze && sc < -RESIGN_VALUE && resignCr > RESIGN_MOVES))
    {
  //      if(!pipe)
  //          FlushConsoleInputBuffer(hnd);
        std::cout << "resign" << std::endl;// << "(mate found)" << std::endl;
        busy = false;
        timer.stop();
        return;
    }

    totalNodes      += nodes;
    totalTimeSpent  += timeSpent;

    timer.stop();

    if(!_abort_)
    {
#ifdef CHECK_PREDICTED_VALUE

        if(rootPly > 1 && pv[0][0] > 1)
        {
            MkMove(pv[0][1]);
            bool ic = Attack(men[wtm + 1], wtm ^ WHT);
            UnMove(pv[0][1]);
            prediction.depth = ic ? rootPly + 1 : ic;
            if(stop)
                prediction.depth--;
            prediction.oppMove = (pv[0][2] & EXCEPT_SCORE);
            prediction.score = sc;
            prediction.state = 2;
        }
#endif // CHECK_PREDICTED_VALUE
        SearchResultOutput();
    }
//    else
//        prevRootPly = 0;
    if(_abort_)
        analyze = false;

    busy = false;
//    std::cout << "( busy = false ) " << std::endl;
}

//--------------------------------
short RootSearch(int depth, short alpha, short beta)
{
    bool ic = Attack(men[wtm + 1], wtm ^ WHT);
    boardState[PREV_STATES + ply].valOpn = valOpn;
    boardState[PREV_STATES + ply].valEnd = valEnd;
    if(!rootTop)
        RootMoveGen(ic);
//	short _alpha = alpha;
    rootMoveCr = 0;

    Move m = 0x000100FF, probed = 0x000100FF;

    UQ prevDeltaNodes = 0;

    short x;

    for(; rootMoveCr < rootTop && alpha < K_VAL - 99 && !stop; rootMoveCr++)
    {
        m = rootMoveList[rootMoveCr];
//        plyMoves[ply + 1] = m;
        if(m == probed)
            continue;
        MkMove(m);
#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0 && (!stopPly || rootPly == stopPly))
            ply = ply;
#endif

        FastEval(m);
        UQ _nodes = nodes;

        if(!DrawByRepetitionInRoot(m))
            x = -Search(depth - 1, -beta, -alpha);
        else
        {
            x = 0;
            pv[1][0] = 0;
        }

        followPV = false;

        UQ dn = nodes - _nodes;
        if(x <= alpha && rootMoveCr > 2 && dn > prevDeltaNodes && depth > 2)
        {
            int tmp = *(int *)&rootMoveList[rootMoveCr];
            *(int *)&rootMoveList[rootMoveCr] =
                *(int *)&rootMoveList[rootMoveCr - 1];
            *(int *)&rootMoveList[rootMoveCr - 1] = tmp;
        }
        prevDeltaNodes = dn;

        if(stop)
            x   = -INF;

        if(x >= beta)
        {
            rootMoveCr += 999;
            UnMove(m);
        }
        else if(x > alpha)
        {
            alpha = x;
            StorePV(m);
            UnMove(m);

            if(depth > 3 && x != -INF
            && !(stop && rootMoveCr < 2))
                PlyOutput(x);
            if(rootMoveCr != 0)
            {
                int tmp = *(int *)&rootMoveList[rootMoveCr];
                memmove(&rootMoveList[1], &rootMoveList[0],
                        sizeof(Move)*rootMoveCr);
                *(int *)&rootMoveList[0] = tmp;
            }
        }
        else
        {
            UnMove(m);
            if(MOVEPC(probed) == 0xFF)
                probed = m;
        }
    }

    if(depth <= 3 && rootTop)
        PlyOutput(alpha);
    if(!rootTop)
    {
        pv[0][0] = 0;
        return ic ? -K_VAL + ply : 0;
    }
/*    else if(rootMoveCr > rootTop + 1)
        StoreInTT(depth, tCUT, beta, m);
    else if(alpha > _alpha)
        StoreInTT(depth, tPV, alpha, pv[ply][1]);
    else
        StoreInTT(depth, tALL, _alpha, probed);*/
    if(rootMoveCr > rootTop + 1)
    {
        UpdateStatistics(m, depth, rootMoveCr);
        return beta;
    }
    return alpha;
}

//--------------------------------
void RootMoveGen(bool ic)
{
    Move moveList[MAX_MOVES];
    rootTop = GenMoves(moveList, APPRICE_ALL);
    boardState[PREV_STATES + ply].valOpn = valOpn;
    boardState[PREV_STATES + ply].valEnd = valEnd;
    for(unsigned i = 0; i < rootTop; i++)
    {
        Move m;
        Next(moveList, i, rootTop, &m);
        MkMove(m);
        if(!Legal(m, ic))
        {
            memmove(&moveList[i], &moveList[i + 1],
                sizeof(Move)*(rootTop - 1));
            rootTop--;
            i--;
        }
        else
            rootMoveList[i] = m;
        UnMove(m);
    }
/*    if(TTSearch())
    {
        Move bm;
        int i = -1;
        if(UnPackMove(TTcur->bMov, &bm))
            for(i = 0; i < rootTop; i++)
                if(rootMoveList[i] == bm)
                    break;
        if(i > 0 && i < rootTop)
        {
            Move tmp = rootMoveList[i];
            rootMoveList[i] = rootMoveList[0];
            rootMoveList[0] = tmp;
        }

    }*/
    pv[0][1] = rootMoveList[0];

/*    randomize();
    for(int i = 0; i < rootTop; i++)
        rootMoveList[i].scr = (rand() >> 13) + (rand() >> 13) + (rand() >> 13);*/
}

//--------------------------------
void InitSearch()
{
    InitTime();
    nodes   = 0;
    qnodes  = 0;
    cutCr   = 0;
    qCutCr  = 0;

    unsigned i, j;
    for(i = 0; i < sizeof(cutNumCr)/sizeof(*cutNumCr); i++)
        cutNumCr[i] = 0;
    for(i = 0; i < sizeof(qCutNumCr)/sizeof(*qCutNumCr); i++)
        qCutNumCr[i] = 0;
    for(i = 0; i < MAX_PLY; i ++)
        for(j = 0; j < MAX_PLY + 1; j++)
            pv[i][j] = (Move) 0;
    for(i = 0; i < MAX_PLY; i++)
    {
        kil[i][0] = (Move) 0;
        kil[i][1] = (Move) 0;
    }
    stop = false;
    _abort_ = false;
//    if(analyze)
//        prevRootPly = 0;

    std::cout   << "( tRemain=" << timeRemains/1e6
                << ", t2thnk=" << timeToThink/1e6
                << ", tExact = " << (spentExactTime ? "true " : "false ")
                << " )" << std::endl;
    std::cout   << "Ply Value  Time    Nodes        Principal Variation" << std::endl;

    timer.start();
    time0 = timer.getElapsedTimeInMicroSec();
}

//--------------------------------
void SearchResultOutput()
{
    if(analyze)
        return;
    char mov[6];
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
//    UQ rd   = TTReduce();
    Move  p = pv[0][1];
    int  f  = men[MOVEPC(p)];
    mov[0]  = COL(f) + 'a';
    mov[1]  = ROW(f) + '1';
    mov[2]  = COL(MOVETO(p)) + 'a';
    mov[3]  = ROW(MOVETO(p)) + '1';
    mov[4]  = (MOVEFLG(p) & (mPROM << 16)) ? proms[(MOVEFLG(p) >> 16) & mPROM] : '\0';
    mov[5]  = '\0';

    if(!MakeLegalMove(mov))
    {
        std::cout << "tellusererror err01" << std::endl << "resign" << std::endl;
//        if(!pipe)
//            FlushConsoleInputBuffer(hnd);
    }
//    sumNodes += nodes;
    std::cout << "( q/n = " << (int)(qnodes/(nodes/100 + 1)) << "%, ";

    std::cout << "cut = [";
    for(unsigned i = 0; i < sizeof(cutNumCr)/sizeof(*cutNumCr); i++)
        std::cout  << (int)(cutNumCr[i]/(cutCr/100 + 1)) << " ";
    std::cout << "]% )" << std::endl;

    std::cout << "( qCut = [";
    for(unsigned i = 0; i < sizeof(qCutNumCr)/sizeof(*qCutNumCr); i++)
        std::cout  << (int)(qCutNumCr[i]/(qCutCr/100 + 1)) << " ";
    std::cout << "]% )" << std::endl;

    std::cout   << "( tSpent=" << timeSpent/1e6
                << ", nodes=" << nodes
                << ", futCr = " << futCr
                << " )" << std::endl;

    std::cout << "move " << mov << std::endl;
}

//--------------------------------
void PlyOutput(short sc)
{
        using namespace std;

        double time1 = timer.getElapsedTimeInMicroSec();
        int tsp = (int)((time1 - time0)/10000.);

        cout << setfill(' ') << setw(4)  << left << rootPly;
        cout << setfill(' ') << setw(7)  << left << sc;
        cout << setfill(' ') << setw(8)  << left << tsp;
        cout << setfill(' ') << setw(12) << left << nodes << ' ';
        ShowPV(0);
        cout << endl;
}

//-----------------------------
void InitTime()
{
    timeRemains -= timeSpent;
    timeRemains = ABSI(timeRemains);
    int movsAfterControl;
    if(movesPerSession == 0)
        movsAfterControl = halfMovCr/2;
    else
        movsAfterControl = (halfMovCr/2) % movesPerSession;

    if(movesPerSession != 0)
        movsRemain = movesPerSession - movsAfterControl;
    else
        movsRemain = 32;
    if(timeBase == 0)
        movsRemain = 1;

    if(!spentExactTime && (movsRemain <= 5
    || (timeInc == 0 && movesPerSession == 0
    && timeRemains < timeBase / 4)))
    {
//        std::cout << "telluser Switch to exact time spent mode OK" << std::endl;
        spentExactTime = true;
    }

    if(movsAfterControl == 0 && halfMovCr/2 != 0)
    {
        timeRemains += timeBase;
//        std::cout << "telluser Switch to non-exact time spent mode OK" << std::endl;
        spentExactTime = false;
    }
    timeRemains += timeInc;
//    if(movsRemain == 0)
//        ply = ply;
    timeToThink = timeRemains / movsRemain;
    if(movsRemain != 1 && !spentExactTime)
        timeToThink /= 3;                           // average branching factor
    else if(timeInc != 0 && timeBase != 0)
        timeToThink = timeInc;
}

//-----------------------------
bool ShowPV(int _ply)
{
    char pc2chr[] = "?KQRBNP??????????????????????????KQRBNP";
    bool ans = true;
    int i = 0, stp = pv[ply][0];
    for(; i < stp; i++)
    {

        Move m = pv[_ply][i + 1];
        char pc = pc2chr[b[men[MOVEPC(m)]]];
        if(pc == 'K' && COL(men[MOVEPC(m)]) == 4 && COL(MOVETO(m)) == 6)
            std::cout << "OO";
        else if(pc == 'K' && COL(men[MOVEPC(m)]) == 4 && COL(MOVETO(m)) == 2)
            std::cout << "OOO";
        else if(pc != 'P')
        {
            std::cout << pc;
            Ambiguous(m);
            if(MOVEFLG(m) & (mCAPT << 16))
                std::cout << 'x';
            std::cout << (char)(COL(MOVETO(m)) + 'a');
            std::cout << (char)(ROW(MOVETO(m)) + '1');
        }
        else if(MOVEFLG(m) & (mCAPT << 16))
            std::cout << (char)(COL(men[MOVEPC(m)]) + 'a') << "x"
                 << (char)(COL(MOVETO(m)) + 'a') << (char)(ROW(MOVETO(m)) + '1');
        else
            std::cout << (char)(COL(MOVETO(m)) + 'a') << (char)(ROW(MOVETO(m)) + '1');
        char proms[] = "?QNRB";
        if(pc == 'P' && (MOVEFLG(m) & (mPROM << 16)))
            std::cout << proms[(MOVEFLG(m) >> 16) & mPROM];
        std::cout << ' ';
        MkMove(m);
        bool ic = Attack(men[wtm + 1], wtm ^ WHT);
        if(!Legal(m, ic))
            ans = false;
    }
    for(; i > 0; i--)
        UnMove(*(Move *) &pv[_ply][i]);
    return ans;
}

//-----------------------------
void Ambiguous(Move m)
{
    Move marr[8];
    unsigned ambCr = 0;
    UC fr0 = men[MOVEPC(m)];
    UC pt0 = b[fr0];

    UC menNum   = wtm;
    unsigned pcCr = 0;
    unsigned maxPc = pieces[wtm >> 5];
    for(; pcCr < maxPc; pcCr++)
    {
        menNum = menNxt[menNum];
        if(menNum == MOVEPC(m))
            continue;
        UC fr = men[menNum];

        UC pt = b[fr];
        if(pt != pt0)
            continue;
        pt &= 0x07;
        if(!(attacks[120 + fr - MOVETO(m)] & (1 << pt)))
            continue;
        if(slider[pt] && !SliderAttack(fr, MOVETO(m)))
            continue;
        Move x = m;
        x |= (fr << MOVE_SCORE_SHIFT);
        marr[ambCr++]   = x;
    }
    if(!ambCr)
        return;
    bool sameCols = false, sameRows = false;
    for(unsigned i = 0; i < ambCr; i++)
    {
        if(COL(MOVESCR(marr[i])) == COL(fr0))
            sameCols = true;
        if(ROW(MOVESCR(marr[i])) == (unsigned)ROW(fr0))
            sameRows = true;
    }
    if(sameCols && sameRows)
        std::cout << (char)(COL(fr0) + 'a') << (char)(ROW(fr0) + '1');
    else if(sameCols)
        std::cout << (char)(ROW(fr0) + '1');
    else
        std::cout << (char)(COL(fr0) + 'a');

}

//-----------------------------
bool MakeLegalMove(char *mov)
{
    int ln = strlen(mov);
    if(ln < 4 || ln > 5)
        return false;
    bool ic = Attack(men[wtm + 1], wtm ^ WHT);
    RootMoveGen(ic);

    char rMov[6];
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    for(unsigned i = 0; i < rootTop; ++i)
    {
        Move m  = rootMoveList[i];
        rMov[0] = COL(men[MOVEPC(m)]) + 'a';
        rMov[1] = ROW(men[MOVEPC(m)]) + '1';
        rMov[2] = COL(MOVETO(m)) + 'a';
        rMov[3] = ROW(MOVETO(m)) + '1';
        rMov[4] = (MOVEFLG(m) & (mPROM << 16)) ? proms[(MOVEFLG(m) >> 16) & mPROM] : '\0';
        rMov[5] = '\0';

        if(strcmp(mov, rMov) == 0)
        {
            UC fr = men[MOVEPC(m)];
            MkMove(m);

            FastEval(m);

            short _valOpn_ = valOpn;
            short _valEnd_ = valEnd;

            memmove(&boardState[0], &boardState[1],
                    (PREV_STATES + 2)*sizeof(BrdState));
            ply--;
            EvalAllMaterialAndPST();
            if(valOpn != _valOpn_ || valEnd != _valEnd_)
            {
                std::cout << "telluser err02: wrong score. Fast: "
                        << _valOpn_ << '/' << _valEnd_
                        << ", all: " << valOpn << '/' << valEnd
                        << std::endl << "resign"
                        << std::endl;
            }

            memmove(&doneMoves[1], &doneMoves[0], sizeof(doneMoves) - sizeof(Move));
            Move m1 = (fr << MOVE_SCORE_SHIFT) | (m & EXCEPT_SCORE);
            doneMoves[0] = m1;
            halfMovCr++;

#ifdef CHECK_PREDICTED_VALUE
            if(prediction.state == 2)
                prediction.state = 3;
            else if(prediction.state == 3 && prediction.oppMove == (m & EXCEPT_SCORE))
                prediction.state = 4;
            else
                prediction.state = 1;
#endif // CHECK_PREDICTED_VALUE

            return true;
        }
    }
    return false;
}

//--------------------------------
void InitEngine()
{
    InitEval();

    timeMaxPly      = MAX_PLY;
    timeRemains     = 300000000;
    timeBase        = 300000000;
    timeInc         = 0;
    movesPerSession = 0;
    timeMaxNodes    = 0;

    std::cin.rdbuf()->pubsetbuf(NULL, 0);
    std::cout.rdbuf()->pubsetbuf(NULL, 0);

    _abort_         = false;
    stop            = false;
    totalNodes      = 0;
    totalTimeSpent  = 0;

#ifdef CHECK_PREDICTED_VALUE
    prediction.state = 0;
#endif // CHECK_PREDICTED_VALUE

    doneMoves[0]    = -1;
    spentExactTime  = false;
    halfMovCr       = 0;
    resignCr        = 0;
    timeSpent       = 0;
}

//--------------------------------
bool FenToBoardAndVal(char *fen)
{
    bool ans = FenToBoard(fen);

    doneMoves[0] = -1;

    if(ans)
        EvalAllMaterialAndPST();

    return ans;
}

//--------------------------------
bool DrawDetect()
{
    if(material[0] + material[1] == 0)
        return true;
    if(pieces[0] + pieces[1] == 3
    && (material[0] == 4 || material[1] == 4))
        return true;
    if(pieces[0] == 2 && pieces[1] == 2
    && material[0] == 4 && material[1] == 4)
        return true;

    if(reversibleMoves >= USELESS_HALFMOVES_TO_DRAW)
        return true;

    return SimpleDrawByRepetition();
}

//--------------------------------
bool SimpleDrawByRepetition()
{
    if(reversibleMoves < 4)
        return false;
    if(boardState[PREV_STATES + ply].to == boardState[PREV_STATES + ply - 2].fr
    && boardState[PREV_STATES + ply].fr == boardState[PREV_STATES + ply - 2].to
    && boardState[PREV_STATES + ply - 1].fr == boardState[PREV_STATES + ply - 3].to
    && boardState[PREV_STATES + ply - 1].to == boardState[PREV_STATES + ply - 3].fr)
        return true;

    return false;

}

//--------------------------------
void CheckForInterrupt()
{
    if(analyze)
        ply = ply;
    else if(timeMaxNodes != 0)
    {
        if(nodes >= timeMaxNodes - 500)
            stop = true;
    }
    else if(spentExactTime)
    {
        double time1 = timer.getElapsedTimeInMicroSec();
        timeSpent = time1 - time0;
        if(timeSpent >= timeToThink - 50000)
            stop = true;
    }
}

//--------------------------------
void MkMove(Move m)
{
    bool specialMove = false;
    ply++;
    boardState[PREV_STATES + ply].cstl   = boardState[PREV_STATES + ply - 1].cstl;
    boardState[PREV_STATES + ply].capt  = b[MOVETO(m)];
    UC fr = men[MOVEPC(m)];
    UC targ = MOVETO(m);
    boardState[PREV_STATES + ply].fr = fr;
    boardState[PREV_STATES + ply].to = targ;
    boardState[PREV_STATES + ply].silent = reversibleMoves;
    reversibleMoves++;
    if(MOVEFLG(m) & (mCAPT << 16))
    {
        if (MOVEFLG(m) & (mENPS << 16))
        {
            targ += wtm ? -16 : 16;
            material[!wtm]--;
        }
        UC menNum = (wtm ^ WHT);
        UC prevMenNum = menNum;
        UC maxMenCr = pieces[menNum >> 5];
        for(int menCr = 0; menCr < maxMenCr; menCr++)
        {
            prevMenNum = menNum;
            menNum = menNxt[menNum];
            if(targ == men[menNum])
                break;
        }
        boardState[PREV_STATES + ply].ncap      = prevMenNum;
        boardState[PREV_STATES + ply].ncapNxt   = menNum;
        menNxt[prevMenNum]  = menNxt[menNum];

        pieces[!wtm]--;
        material[!wtm] -= pc_streng[boardState[PREV_STATES + ply].capt & 0x0F];
        reversibleMoves = 0;
    }// if capture

    if(!(b[fr] & 0xDC) || (MOVEFLG(m) & (mCAPT << 16)))        // trick: fast exit if not K|Q|R moves, and no captures
        specialMove |= MakeCastle(m, fr);

    boardState[PREV_STATES + ply].ep = 0;
    if((b[fr] ^ wtm) == _p)
    {
        specialMove     |= MakeEP(m, fr);
        reversibleMoves = 0;
    }
#ifndef NDEBUG
    ShowMove(fr, MOVETO(m));
#endif

    men[MOVEPC(m)]   = MOVETO(m);
    b[MOVETO(m)]     = b[fr];
    b[fr]       = __;

    int prIx = (MOVEFLG(m) >> 16) & mPROM;
    UC prPc[] = {0, _q, _n, _r, _b};
    if(prIx)
    {
        b[MOVETO(m)] = prPc[prIx] ^ wtm;
        material[wtm >> 5] += pc_streng[prPc[prIx]] - 1;
        unsigned i = wtm + 1;
        for(; i < wtm + 0x20; i++)
            if(menNxt[i] == MOVEPC(m))
                break;
        boardState[PREV_STATES + ply].nprom = i;
        menNxt[i] = menNxt[MOVEPC(m)];
        menNxt[MOVEPC(m)] = menNxt[wtm];
        menNxt[wtm] = MOVEPC(m);
        reversibleMoves = 0;
    }


//    boardState[PREV_STATES + ply].ttKey = ttKey;
//    TTMove(m, fr, specialMove);
#ifdef USE_PAWN_STRUCT
    MovePawnStruct(b[MOVETO(m)], fr, m);
#endif // USE_PAWN_STRUCT
    wtm ^= WHT;
}

//--------------------------------
void UnMove(Move m)
{
    UC fr = boardState[PREV_STATES + ply].fr;
    men[MOVEPC(m)] = fr;
    b[fr] = (MOVEFLG(m) & (mPROM << 16)) ? _P ^ wtm : b[MOVETO(m)];
    b[MOVETO(m)] = boardState[PREV_STATES + ply].capt;
//    ttKey = boardState[PREV_STATES + ply].ttKey;
    reversibleMoves = boardState[PREV_STATES + ply].silent;
    wtm ^= WHT;

    if(MOVEFLG(m) & (mCAPT << 16))
    {
        if(MOVEFLG(m) & (mENPS << 16))
        {
            material[!wtm]++;
            if(wtm)
            {
                b[MOVETO(m) - 16] = _p;
                men[boardState[PREV_STATES + ply].ncapNxt] = MOVETO(m) - 16;
            }
            else
            {
                b[MOVETO(m) + 16] = _P;
                men[boardState[PREV_STATES + ply].ncapNxt] = MOVETO(m) + 16;
            }
        }// if en_pass

        menNxt[boardState[PREV_STATES + ply].ncap] = boardState[PREV_STATES + ply].ncapNxt;
        pieces[!wtm]++;
        material[!wtm] += pc_streng[boardState[PREV_STATES + ply].capt & 0x0F];
    }// if capture

    int prIx = (MOVEFLG(m) >> 16) & mPROM;
    UC prPc[] = {0, _q, _n, _r, _b};
    if(prIx)
    {
        material[wtm >> 5] -= pc_streng[prPc[prIx]] - 1;
        UC nprm = boardState[PREV_STATES + ply].nprom;
        menNxt[wtm] = menNxt[MOVEPC(m)];
        menNxt[MOVEPC(m)] = menNxt[nprm];
        menNxt[nprm] = MOVEPC(m);
    }

    if(MOVEFLG(m) & (mCSTL << 16))
        UnMakeCastle(m);

#ifdef USE_PAWN_STRUCT
    MovePawnStruct(b[fr], fr, m);
    #ifndef NDEBUG
        int col, row;
        for(col = 0; col < 8; col++)
        {
            int max_w = 0, min_w = 7, max_b = 0, min_b = 7;
            for(row = 0; row < 8; row++)
            {
                if(b[XY2SQ(col, row)] != _P)
                    continue;
                if(row > max_w)
                    max_w = row;
                if(row < min_w)
                    min_w = row;
            }
            if(min_w != pmin[col][1] || max_w != pmax[col][1])
                ply = ply;
            for(row = 0; row < 8; row++)
            {
                if(b[XY2SQ(col, row)] != _p)
                    continue;
                if(7 - row > max_b)
                    max_b = 7 - row;
                if(7 - row < min_b)
                    min_b = 7 - row;
            }
            if(min_b != pmin[col][0] || max_b != pmax[col][0])
                ply = ply;
        }
    #endif
#endif // USE_PAWN_STRUCT

    ply--;
    valOpn = boardState[PREV_STATES + ply].valOpn;
    valEnd = boardState[PREV_STATES + ply].valEnd;

#ifndef NDEBUG
    curVar[5*ply] = '\0';
#endif
}

//--------------------------------
bool DrawByRepetitionInRoot(Move lastMove)
{
    if(reversibleMoves == 0)
        return false;

    UC menTmp[sizeof(men)];

    memcpy(menTmp, men, sizeof(men));
    menTmp[MOVEPC(lastMove)] = boardState[PREV_STATES + ply].fr;
    for(unsigned short i = 0; i < reversibleMoves - 1; ++i)
    {
        if(doneMoves[i] == (unsigned)-1)
            return false;
        menTmp[MOVEPC(doneMoves[i])] = MOVESCR(doneMoves[i]);
        if(memcmp(menTmp, men, sizeof(men)) == 0)
            return true;
        ++i;
        if(doneMoves[i] == (unsigned)-1)
            return false;
        menTmp[MOVEPC(doneMoves[i])] = MOVESCR(doneMoves[i]);
    }
    return false;
}

//--------------------------------------
void MakeNullMove()
{
    UC _to = boardState[PREV_STATES + ply].to;
#ifndef NDEBUG
    strcpy(&curVar[5*ply], "NULL ");
    if((!stopPly || rootPly == stopPly) && strcmp(stop_str, cv) == 0)
        ply = ply;
#endif

    boardState[PREV_STATES + ply + 1] = boardState[PREV_STATES + ply];
    boardState[PREV_STATES + ply].to = 0xFF;
    boardState[PREV_STATES + ply  + 1].ep = 0;
    ply++;
//    ttKey ^= -1L;
//    bFlg[ply].ttKey = ttKey;
    wtm ^= WHT;
}

//--------------------------------------
void UnMakeNullMove()
{
    wtm ^= WHT;
    curVar[5*ply] = '\0';
    ply--;
//    ttKey ^= -1L;
}

//-----------------------------
bool NullMove(int depth, short beta, bool ic)
{
    if(ic || depth < 3 || material[wtm >> 5] - pieces[wtm >> 5] < 3)
        return false;

    if(boardState[PREV_STATES + ply - 1].to == 0xFF
//    && boardState[PREV_STATES + ply - 2].to == 0xFF
      )
        return false;

    UC store_ep     = boardState[PREV_STATES + ply].ep;
    UC _to = boardState[PREV_STATES + ply].to;
    MakeNullMove();
//    if(store_ep)
//        ttKey = TTSetKey();

    int r = depth > 6 ? 3 : 2;
    short x = -Search(depth - r - 1, -beta, -beta + 1);

    UnMakeNullMove();
    boardState[PREV_STATES + ply].to    = _to;
    boardState[PREV_STATES + ply].ep    = store_ep;
//    if(store_ep)
//        ttKey = TTSetKey();

    return (x >= beta);
}

/*
r4rk1/ppqn1pp1/4pn1p/2b2N1P/8/5N2/PPPBQPP1/2KRR3 w - - 0 18 Nxh6?! speed test position
2k1r2r/1pp3pp/p2b4/2p1n2q/6b1/1NQ1B3/PPP2PPP/R3RNK1 b - - bm Nf3+; speed test position
r4rk1/1b3p1p/pq2pQp1/n5N1/P7/2RBP3/5PPP/3R2K1 w - - 0 1 bm Nxh7;   speed test position
1B6/3k4/p7/1p4b1/1P1PK1p1/P7/8/8 w - - 2 59 am Bf4;                speed test position

1nbq1r1k/3rbp1p/p1p1pp1Q/1p4N1/P1pPN3/6P1/1P2PPBP/R4RK1 b - - 0 1 pv @ terminal node fixed
2q1r1k1/1b3pbp/3p2p1/1Ppn3n/5P2/5B2/1P1B2PP/R2NNQ1K b - - bm Ndxf4; wrong Ambigous() fixed
3r1rk1/4qp2/n2Np2Q/p3P3/pnRR4/5bP1/1P3P1P/6K1 b - - 0 5 can't e5f6 after f7f5 fixed
8/6p1/2P1Qb1k/R4p1p/3PpP1P/1K2P3/5BP1/8 b - - 0 96
1q3r2/2NR1N1k/1p4p1/2pQ3p/8/4bP2/P5PP/7K b - - 0 1 wrong score after Rxf7 fixed

5rk1/1p4pp/2p5/2P1Nb2/3P2pq/7r/P2Q1P1P/R4RK1 w - - 5 26 eval of king safety
8/5R2/8/4K3/1r4p1/8/6k1/8 w - - 2 80 strange value
6k1/5p2/2p3p1/p4n2/P1P1pB2/7P/R1P1BPK1/1r6 b - - 6 36 pawn eval
8/2p2kp1/Qp5p/7r/2P5/7P/Pq4P1/5R1K b - - 0 48 am Ke7 eval of king safety

1r6/3R4/7p/8/4k1P1/6KP/8/8 b - - 0 1 eval: black's better?
3r4/3P1p2/p4Pk1/4r1p1/1p4Kp/3R4/8/8 b - - 0 1 Rxd7& @ply 5: OK, there's no stealmate in QS
4n3/3N4/4P3/3P2p1/p1K5/7k/8/8 w - - 0 58 pawn eval?

8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1 fixed MovePawnStruct()
4k3/8/p1KP2p1/P4p1p/5P1P/5P2/8/8 w - - 0 47 search tree explosion @ply 9
r1b2rk1/ppq3pp/2nb1n2/3pp3/3P3B/3B1N2/PP2NPPP/R2Q1RK1 w - - 0 1 qb3? fixed: different values at ply 1 and 2, pv is the same
r1bqkb1r/pppppppp/2n2n2/8/3P4/2N2N2/PPP1PPPP/R1BQKB1R b KQkq d3 0 3 return value -32760 at level 1000 nodes per move
8/8/5pp1/6kp/4K3/5r2/1R6/8 b - - 0 84 g5g4 fast eval king distance testing
7k/1R5p/1p2BK2/4P3/8/8/5P2/6r1 b - - 0 65 g1g6 fast eval king distance testing
rnb1k2r/ppppqppp/5n2/8/1b2P3/2NN4/PPPP1PPP/R1BQKB1R b KQkq - 0 5 b4c3 b2c3 fast eval king distance testing
r1bqk1nr/pppp1ppp/2n2b2/8/2B1P3/2N2N2/PPPP3p/R1BQ1R1K w kq - 0 9 h1h2 fast eval king distance testing
rnbqk2r/pppp1pp1/4pn1p/6B1/3PP3/2P5/P1P2PPP/R2QKBNR w KQkq - 0 6 g5f6 fast eval king distance testing
8/8/8/8/8/3k4/4p1K1/3n4 b - - 1 106 e2e1q fast eval king distance testing
8/8/8/8/2K5/8/pk6/8 b - - 1 59 a2a1q fast eval king distance testing
1r6/5pp1/p2k4/Pp2p1p1/1P1pP3/1K1P1P1P/6P1/2R5 w - b6 0 25 a5b6 fast eval king distance testing
6k1/8/8/4pQ2/3p1q2/KP6/P7/8 w - - 1 52 Qxf4? eval of unstoppables improved
8/8/5p2/5rk1/Q6p/P3K3/8/8 w - - 1 74 am Qe4
r6r/pbpq1p2/1pnp1k1p/3Pp1p1/2P4P/2QBPPB1/P5P1/2R2RK1 b - - 0 8 search explosion @ply 7
3r1rk1/1p3ppp/pnq5/4p1P1/6Q1/1BP1R3/PP3PP1/2K4R b - - 0 17 rotten position for black
8/6k1/8/6pn/pQ6/Pp3P1K/1P3P1P/6r1 w - - 2 31 draw is inevitable
7k/7P/2Kr1PP1/8/8/8/8/1R6 w - - 27 73 am Kb5 Kb6 Kb7;
7k/8/8/1p2P2p/r2N4/8/7r/4BKR1 w - - 2 28 am Bc3;
8/3k4/1p2p3/1Kp1P3/8/8/5pB1/8 w - - 6 82 am Kxb6;
3k4/3R4/p3B3/2B1p3/1n2Pp2/5Pq1/7p/7K b - - 15 66 null move bug fixed
5k2/8/5PK1/7p/8/6P1/8/8 b - - 4 54  search explosion @ply 11
6k1/4b3/1p6/1Pr1PRp1/5p1p/5P1P/6P1/3R3K w - - 1 64 am Rd7;
2kb1R2/8/2p2B2/1pP2P2/1P6/1K6/8/3r4 w - - 0 1 bm Be7; got it faster without null move
8/1k4N1/8/8/2Q5/8/8/3K4 w - - 93 172 search explosion @ply 19
1r6/8/8/8/8/8/k5K1/r7 b - - 96 191 search explosion @ply 13
*/

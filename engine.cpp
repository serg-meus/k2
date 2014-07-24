#include "engine.h"

//--------------------------------
char        stop_str[] = "";
unsigned    stopPly   = 0;

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
int         finalyMadeMoves, movsRemain;
#ifdef NOT_USE_HASH_FOR_DRAW
    Move        doneMoves[FIFTY_MOVES];
#endif
bool        spentExactTime;
unsigned    resignCr;
bool        timeCommandSent;

#ifndef NOT_USE_HASH_TABLE
    unsigned long hashSize = 64*HASH_ENTRIES_PER_MB;
    std::unordered_map<UQ, hashEntryStruct> hash_table(hashSize);
//    UQ  doneHashKeys[FIFTY_MOVES + MAX_PLY];
    std::array<UQ, FIFTY_MOVES + MAX_PLY> doneHashKeys;
#endif // NOT_USE_HASH_TABLE
//--------------------------------
short Search(int depth, short alpha, short beta)
{
    if(ply >= MAX_PLY - 1 || DrawDetect())
    {
        pv[ply][0] = 0;
        return 0;
    }
    bool in_check = Attack(men[wtm + 1], wtm ^ WHT);
    if(in_check)
        depth++;
#ifndef NOT_USE_FUTILITY
    if(depth <= 1 && depth >= 0 && !in_check
    && beta < K_VAL - MAX_PLY
    && Futility(depth, beta))
        return beta;
#endif // NOT_USE_FUTILITY

#ifndef NOT_USE_HASH_TABLE
    short _alpha = alpha;
    bool best_move_hashed = false;
    hashEntryStruct entry;
    if(hash_table.count(hash_key) > 0 && !stop)
    {
        entry = hash_table[hash_key];
        if(entry.depth >= depth)
        {
            UC hbnd = entry.bound_type;
            short hval = -entry.value;
            if(((hbnd == hUPPER || hbnd == hEXACT) && hval >= beta)
            || ((hbnd == hLOWER || hbnd == hEXACT) && hval <= alpha)
            )
            {
                pv[ply][0] = 0;
                return hval;
            }// if(bnd
        }// if(entry.depth
        else
        {
//            if(entry.only_move)
//                depth++;
            if(entry.bound_type == hEXACT
            || entry.bound_type == hUPPER)
                best_move_hashed = true;
        }
    }
#endif // NOT_USE_HASH_TABLE
    if(depth <= 0)
    {
        return Quiesce(alpha, beta);
    }
    nodes++;
    if((nodes & 511) == 511)
        CheckForInterrupt();


#ifndef NOT_USE_NULL_MOVE
    if(NullMove(depth, beta, in_check))
        return beta;
#endif // NOT_USE_NULL_MOVE

    Move moveList[MAX_MOVES];
#ifdef NOT_USE_HASH_TABLE
    if(genBestMove)
        bestMoveToGen = pv[0][1 + ply];
#else
    if(best_move_hashed)
    {
        genBestMove     = true;
        bestMoveToGen   = entry.best_move;
    }
    else
        genBestMove = false;
#endif // NOT_USE_HASH_TABLE
    int i = 0, legals = 0;
    int top = GenMoves(moveList, APPRICE_ALL);

    Move m;
    short x;

    boardState[PREV_STATES + ply].valOpn = valOpn;
    boardState[PREV_STATES + ply].valEnd = valEnd;

    bool mateFound = alpha >= K_VAL - MAX_PLY + 1;
//    if(analyze)
//        mateFound = false;
    for(; i < top && !stop && !mateFound; i++)
    {
        Next(moveList, i, top, &m);

        MkMove(m);
#ifndef NDEBUG
        if((!stopPly || rootPly == stopPly) && strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
        if(!Legal(m, in_check))
        {
            UnMove(m);
            continue;
        }

        FastEval(m);

#ifndef NOT_USE_PVS
        if(legals && depth > 1 && beta != alpha + 1
        && ABSI(alpha) < K_VAL - MAX_PLY + 1)
        {
            x = -Search(depth - 1 /*- lmr*/, -alpha - 1, -alpha);
            if(x > alpha)
                x = -Search(depth - 1, -beta, -alpha);
        }
        else
            x = -Search(depth - 1 /*- lmr*/, -beta, -alpha);
#else
        x = -Search(depth - 1, -beta, -alpha);
#endif // NOT_USE_PVS
        genBestMove = false;

        legals++;

        if(x >= beta)
            i += BETA_CUTOFF - 1;                                       // an ugly way to exit, but...
        else if(x > alpha)
        {
            alpha = x;
            StorePV(m);
        }
        UnMove(m);
    }// for i

    if(!legals && alpha < K_VAL - MAX_PLY + 1)
    {
        pv[ply][0] = 0;
        return in_check ? -K_VAL + ply : 0;
    }
#ifndef NOT_USE_HASH_TABLE
    else if(legals)
    {
        size_t sz = hash_table.size();
        int exist = hash_table.count(hash_key);
        if(exist || sz < hashSize)
        {
#ifndef NDEBUG
            if(hash_key == 0)
                ply = ply;
#endif //NGEBUG
            hashEntryStruct hs;
            hs.depth = depth;

            if(legals == 1 && i <= top + 1)
                hs.only_move = true;
            else
                hs.only_move = false;

            if(i >= BETA_CUTOFF)
            {
                hs.bound_type   = hUPPER;
                hs.value        = -beta;
                hs.best_move    = m;
                hash_table[hash_key] = hs;
            }
            else if(alpha > _alpha && pv[ply][0] > 0)
            {
                hs.bound_type   = hEXACT;
                hs.value        = -alpha;
                hs.best_move    = pv[ply][1];
                hash_table[hash_key] = hs;
            }
            else if(alpha <= _alpha)
            {
                hs.bound_type   = hLOWER;
                hs.value        = -_alpha;
                hash_table[hash_key] = hs;
            }
            else
                ply = ply;
        }// if(exist && sz < ...
    }// else if(legals
#endif // NOT_USE_HASH_TABLE
    if(i >= BETA_CUTOFF)
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
    if((nodes & 511) == 511)
        CheckForInterrupt();

    pv[ply][0] = 0;

    boardState[PREV_STATES + ply].valOpn = valOpn;
    boardState[PREV_STATES + ply].valEnd = valEnd;

    short x = Eval(alpha, beta);

    if(-x >= beta)
        return beta;
    else if(-x > alpha)
        alpha = -x;

    Move moveList[MAX_MOVES];
    unsigned i = 0;
    unsigned top = GenCaptures(moveList);

    for(; i < top && !stop; i++)
    {
        Move m;
        Next(moveList, i, top, &m);

        MkMove(m);
#ifndef NDEBUG
        if((!stopPly || rootPly == stopPly) && strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
        if((boardState[PREV_STATES + ply].capt & ~WHT) == _k)
        {
            UnMove(m);
            return K_VAL;
        }
        FastEval(m);

        x = -Quiesce(-beta, -alpha);

        if(x >= beta)
            i += BETA_CUTOFF - 1;
        else if(x > alpha)
        {
            alpha   = x;
            StorePV(m);
        }
        UnMove(m);
    }// for(i

    if(i > top + 1)
    {
        qCutCr++;
        if(i < BETA_CUTOFF + sizeof(qCutNumCr)/sizeof(*qCutNumCr))
            qCutNumCr[i - BETA_CUTOFF]++;
        return beta;
    }
    return alpha;
}

//--------------------------------
void Perft(int depth)
{
    Move moveList[MAX_MOVES];
    bool in_check = Attack(men[wtm + 1], wtm ^ WHT);
    GenMoves(moveList, APPRICE_NONE);
    int top = moveCr;
    for(int i = 0; i < top; i++)
    {
        Move m = moveList[i];
        MkMove(m);
#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
//        bool legal = !Attack(men[(wtm ^ WHT) + 1], wtm);
        bool legal = Legal(m, in_check);
        if(depth > 1 && legal)
            Perft(depth - 1);
        if(depth == 1 && legal)
            nodes++;
        UnMove(m);
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
    if(i < BETA_CUTOFF + sizeof(cutNumCr)/sizeof(*cutNumCr))
        cutNumCr[i - BETA_CUTOFF]++;
#ifndef NDEBUG
    else if(i > BETA_CUTOFF + 10 && dpt > 4)
        ply = ply;
#else
    UNUSED(dpt);
#endif
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
    InitSearch();

    short sc = 0, _sc_;
    rootPly = 1;
    rootTop = 0;
    for(; rootPly <= MAX_PLY && !stop; ++rootPly)
    {
#ifndef NDEBUG
        if(rootPly == 7)
            ply = ply;
#endif
        genBestMove = false;
        _sc_     = sc;

        sc = RootSearch(rootPly, -INF, INF);

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
            if(rootTop == 1 && rootPly >= 6 )
                break;
        }
        if(rootPly >= timeMaxPly)
            break;

    }// for(rootPly

    if(sc < -RESIGN_VALUE)
        resignCr++;
    else
        resignCr = 0;

    if((!stop && !analyze && sc < -K_VAL + MAX_PLY)
    || (!analyze && sc < -RESIGN_VALUE && resignCr > RESIGN_MOVES))
    {
        std::cout << "resign" << std::endl;
        busy = false;
    }

    totalNodes      += nodes;
    totalTimeSpent  += timeSpent;

    timer.stop();

    if(!_abort_)
        PrintSearchResult();
    if(_abort_)
        analyze = false;

    busy = false;
}

//--------------------------------
short RootSearch(int depth, short alpha, short beta)
{
    bool in_check = Attack(men[wtm + 1], wtm ^ WHT);
    boardState[PREV_STATES + ply].valOpn = valOpn;
    boardState[PREV_STATES + ply].valEnd = valEnd;
    if(!rootTop)
        RootMoveGen(in_check);

    rootMoveCr = 0;

    Move m = 0x000100FF, probed = 0x000100FF;

    UQ prevDeltaNodes = 0;

    short x;

    for(; rootMoveCr < rootTop && alpha < K_VAL - 99 && !stop; rootMoveCr++)
    {
        m = rootMoveList[rootMoveCr];
        if(m == probed)
            continue;
        MkMove(m);

#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0 && (!stopPly || rootPly == stopPly))
            ply = ply;
#endif // NDEBUG

        FastEval(m);
        UQ _nodes = nodes;

#ifndef NOT_USE_HASH_FOR_DRAW
        if(!DrawByRepetition())
        {
            if(rootMoveCr > 0)
                x = -Search(depth - 1, -alpha - 1, - alpha);
            if(rootMoveCr <= 0 || x > alpha)
                x = -Search(depth - 1, -beta, -alpha);
        }
#else
        if(!DrawByRepetitionInRoot(m))
        {
            if(rootMoveCr > 0)
                x = -Search(depth - 1, -alpha - 1, -alpha);
            if(rootMoveCr <= 0 || x > alpha)
                x = -Search(depth - 1, -beta, -alpha);
        }
#endif // NOT_USE_HASH_FOR_DRAW
        else
        {
            x = 0;
            pv[1][0] = 0;
        }

        genBestMove = false;

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
            rootMoveCr += BETA_CUTOFF - 1;
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
        return in_check ? -K_VAL + ply : 0;
    }
    if(rootMoveCr > rootTop + 1)
    {
        UpdateStatistics(m, depth, rootMoveCr);
        return beta;
    }
    return alpha;
}

//--------------------------------
void RootMoveGen(bool in_check)
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
        if(!Legal(m, in_check))
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
    pv[0][1] = rootMoveList[0];
}

//--------------------------------
void InitSearch()
{
    InitTime();
    timer.start();
    time0 = timer.getElapsedTimeInMicroSec();

    nodes   = 0;
    qnodes  = 0;
    cutCr   = 0;
    qCutCr  = 0;
    futCr   = 0;

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

//    if(!uci)
//    {
    std::cout   << "( tRemain=" << timeRemains/1e6
                << ", t2thnk=" << timeToThink/1e6
                << ", tExact = " << (spentExactTime ? "true " : "false ")
                << " )" << std::endl;
    std::cout   << "Ply Value  Time    Nodes        Principal Variation" << std::endl;
//    }


#ifndef NOT_USE_HASH_TABLE
    hash_table.clear();
#endif // NOT_USE_HASH_TABLE
}

//--------------------------------
void PrintSearchResult()
{
    char mov[6];
    char proms[] = {'?', 'q', 'n', 'r', 'b'};

    Move  p = pv[0][1];
    int  f  = men[MOVEPC(p)];
    mov[0]  = COL(f) + 'a';
    mov[1]  = ROW(f) + '1';
    mov[2]  = COL(MOVETO(p)) + 'a';
    mov[3]  = ROW(MOVETO(p)) + '1';
    mov[4]  = (MOVEFLG(p) & (mPROM << 16)) ? proms[(MOVEFLG(p) >> 16) & mPROM] : '\0';
    mov[5]  = '\0';

    if(!uci && !MakeMoveFinaly(mov))
    {
        std::cout << "tellusererror err01" << std::endl << "resign" << std::endl;
    }

//    if(!uci)
//    {
/*        std::cout << "( q/n = " << (int)(qnodes/(nodes/100 + 1)) << "%, ";

        std::cout << "cut = [";
        for(unsigned i = 0; i < sizeof(cutNumCr)/sizeof(*cutNumCr); i++)
            std::cout  << (int)(cutNumCr[i]/(cutCr/100 + 1)) << " ";
        std::cout << "]% )" << std::endl;

        std::cout << "( qCut = [";
        for(unsigned i = 0; i < sizeof(qCutNumCr)/sizeof(*qCutNumCr); i++)
            std::cout  << (int)(qCutNumCr[i]/(qCutCr/100 + 1)) << " ";
        std::cout << "]% )" << std::endl;
*/
        std::cout   << "( tSpent=" << timeSpent/1e6
                    << ", nodes=" << nodes
                    << ", futCr = " << futCr
                    << " )" << std::endl;
//    }
    if(!uci)
        std::cout << "move " << mov << std::endl;
    else
        std::cout << "bestmove " << mov << std::endl;
}

//--------------------------------
void PlyOutput(short sc)
{
        using namespace std;

        double time1 = timer.getElapsedTimeInMicroSec();
        int tsp = (int)((time1 - time0)/1000.);

        if(uci)
        {
            cout << "info depth " << rootPly;

            if(ABSI(sc) < K_VAL - MAX_PLY)
                cout << " score cp " << sc;
            else
            {
                if(sc > 0)
                    cout << " score mate " << (K_VAL - sc + 1) / 2;
                else
                    cout << " score mate -" << (K_VAL + sc + 1) / 2;
            }
            cout << " time " << tsp
                << " nodes " << nodes
                << " pv ";
            ShowPV(0);
            cout << endl;
        }
        else
        {
            cout << setfill(' ') << setw(4)  << left << rootPly;
            cout << setfill(' ') << setw(7)  << left << sc;
            cout << setfill(' ') << setw(8)  << left << tsp / 10;
            cout << setfill(' ') << setw(12) << left << nodes << ' ';
            ShowPV(0);
            cout << endl;
        }
}

//-----------------------------
void InitTime()                                                         // too complex
{
    if(!timeCommandSent)
        timeRemains -= timeSpent;
    timeRemains = ABSI(timeRemains);                                    //<< NB: strange
    int movsAfterControl;
    if(movesPerSession == 0)
        movsAfterControl = finalyMadeMoves/2;
    else
        movsAfterControl = (finalyMadeMoves/2) % movesPerSession;

    if(movesPerSession != 0)
        movsRemain = movesPerSession - (uci ? 0 : movsAfterControl);
    else
        movsRemain = 32;
    if(timeBase == 0)
        movsRemain = 1;

    if(!spentExactTime && (movsRemain <= 5
    || (timeInc == 0 && movesPerSession == 0
    && timeRemains < timeBase / 4)))
    {
        spentExactTime = true;
    }

    if(movsAfterControl == 0 && finalyMadeMoves/2 != 0)
    {
        if(!timeCommandSent)
            timeRemains += timeBase;

        if(movsRemain > 5)
            spentExactTime = false;
    }
    if(!timeCommandSent)
        timeRemains += timeInc;
//    if(movsRemain == 0)
//        ply = ply;
    timeToThink = timeRemains / movsRemain;
    if(movsRemain != 1 && !spentExactTime)
        timeToThink /= 3;                           // average branching factor
    else if(timeInc != 0 && timeBase != 0)
        timeToThink = timeInc;

    timeCommandSent = false;
}

//-----------------------------
bool ShowPV(int _ply)
{
    char pc2chr[] = "?KQRBNP??????????????????????????KQRBNP";
    bool ans = true;
    int i = 0, stp = pv[ply][0];

    if(uci)
    {
        for(; i < stp; i++)
        {
            Move m = pv[_ply][i + 1];
            UC fr = men[MOVEPC(m)];
            std::cout  << (char)(COL(fr) + 'a') << (char)(ROW(fr) + '1')
                << (char)(COL(MOVETO(m)) + 'a') << (char)(ROW(MOVETO(m)) + '1');
            char proms[] = {'?', 'q', 'n', 'r', 'b'};
            if(MOVEFLG(m) & (mPROM << 16))
                std::cout << proms[(MOVEFLG(m) >> 16) & mPROM];
            std::cout << " ";
            MkMove(m);
            bool in_check = Attack(men[wtm + 1], wtm ^ WHT);
            if(!Legal(m, in_check))
                ans = false;
        }
    }
    else
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
            bool in_check = Attack(men[wtm + 1], wtm ^ WHT);
            if(!Legal(m, in_check))
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
bool MakeMoveFinaly(char *mov)
{
    int ln = strlen(mov);
    if(ln < 4 || ln > 5)
        return false;
    bool in_check = Attack(men[wtm + 1], wtm ^ WHT);
    RootMoveGen(in_check);

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
#ifdef NOT_USE_HASH_FOR_DRAW
            UC fr = men[MOVEPC(m)];
#endif //NOT_USE_HASH_FOR_DRAW
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
#ifndef NOT_USE_HASH_FOR_DRAW
            for(int i = 0; i < FIFTY_MOVES; ++i)
                doneHashKeys[i] = doneHashKeys[i + 1];
#else
            memmove(&doneMoves[1], &doneMoves[0],
                    sizeof(doneMoves) - sizeof(Move));
            Move m1 = (fr << MOVE_SCORE_SHIFT) | (m & EXCEPT_SCORE);
            doneMoves[0] = m1;
#endif // NOT_USE_HASH_FOR_DRAW

            finalyMadeMoves++;
            return true;
        }
    }
    return false;
}

//--------------------------------
void InitEngine()
{
    InitEval();

#ifndef NOT_USE_HASH_TABLE
    hash_key = InitHashKey();
#endif // NOT_USE_HASH_TABLE

    std::cin.rdbuf()->pubsetbuf(NULL, 0);
    std::cout.rdbuf()->pubsetbuf(NULL, 0);

    _abort_         = false;
    stop            = false;
    totalNodes      = 0;
    totalTimeSpent  = 0;

    spentExactTime  = false;
    finalyMadeMoves = 0;
    resignCr        = 0;
    timeSpent       = 0;
    futCr           = 0;
}

//--------------------------------
bool FenStringToEngine(char *fen)
{
    bool ans = FenToBoard(fen);

    if(!ans)
        return false;

    EvalAllMaterialAndPST();
    timeSpent   = 0;
    timeRemains = timeBase;
    finalyMadeMoves = 0;
#ifndef NOT_USE_HASH_TABLE
    hash_key = InitHashKey();
#endif // NOT_USE_HASH_TABLE

//    std::cout << "( " << fen << " )" << std::endl;
    return true;
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

    if(reversibleMoves == FIFTY_MOVES)
        return true;

#ifndef NOT_USE_HASH_FOR_DRAW
    return DrawByRepetition();
#else
    return SimpleDrawByRepetition();
#endif // USE_HASH_FOR_DRAW

}

//--------------------------------
bool SimpleDrawByRepetition()
{
    if(reversibleMoves < 4)
        return false;
    if(ply + finalyMadeMoves < 4)
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
    if(uci && (nodes & 0x07FFFF) == 0x07FFFF)
    {
        double t = timer.getElapsedTimeInMicroSec();

        std::cout << "info nodes " << nodes
            << " nps " << (int)(1000000 * nodes / (t - time0 + 1));

        Move m = rootMoveList[rootMoveCr];
        UC fr = boardState[PREV_STATES + 1].fr;
        std::cout << " currmove "
            << (char)(COL(fr) + 'a') << (char)(ROW(fr) + '1')
            << (char)(COL(MOVETO(m)) + 'a') << (char)(ROW(MOVETO(m)) + '1');
        char proms[] = {'?', 'q', 'n', 'r', 'b'};
        if(MOVEFLG(m) & (mPROM << 16))
            std::cout << proms[(MOVEFLG(m) >> 16) & mPROM];

        std::cout << " currmovenumber " << rootMoveCr + 1;
        std::cout << " hashfull ";
#ifndef NOT_USE_HASH_TABLE
        UQ hsz = hash_table.size();
        hsz = 1000*hsz / hashSize;
            std::cout << hsz;
#else
        std::cout << "0";
#endif // NOT_USE_HASH_TABLE
        std::cout << std::endl;

    }
    if(timeMaxNodes != 0)
    {
        if(nodes >= timeMaxNodes - 512)
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
    boardState[PREV_STATES + ply].reversibleCr = reversibleMoves;
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
#endif // NDEBUG

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
        for(; i < wtm + 0x20; i++)                                      //<< NB: wrong
            if(menNxt[i] == MOVEPC(m))
                break;
        boardState[PREV_STATES + ply].nprom = i;
        menNxt[i] = menNxt[MOVEPC(m)];
        menNxt[MOVEPC(m)] = menNxt[wtm];
        menNxt[wtm] = MOVEPC(m);
        reversibleMoves = 0;
    }
#ifndef NOT_USE_HASH_TABLE
    doneHashKeys.at(FIFTY_MOVES + ply - 1) = hash_key;
    MoveHashKey(m, fr, specialMove);
    #ifndef NDEBUG
        UQ tmp_key = InitHashKey() ^ -1ULL;
        if(tmp_key != hash_key)
            ply = ply;
    #endif //NDEBUG

#endif // NOT_USE_HASH_TABLE

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

    reversibleMoves = boardState[PREV_STATES + ply].reversibleCr;
#ifndef NOT_USE_HASH_TABLE
    hash_key = doneHashKeys.at(FIFTY_MOVES + ply - 1);
#endif // NOT_USE_HASH_TABLE
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
#endif // USE_PAWN_STRUCT

    ply--;
    valOpn = boardState[PREV_STATES + ply].valOpn;
    valEnd = boardState[PREV_STATES + ply].valEnd;

#ifndef NDEBUG
    curVar[5*ply] = '\0';
#endif // NDEBUG
}

//--------------------------------
#ifdef NOT_USE_HASH_FOR_DRAW
bool DrawByRepetitionInRoot(Move lastMove)
{
    if(reversibleMoves == 0)
        return false;

    UC menTmp[sizeof(men)];

    memcpy(menTmp, men, sizeof(men));
    menTmp[MOVEPC(lastMove)] = boardState[PREV_STATES + ply].fr;

    unsigned short max_count;
    if(reversibleMoves > ply + finalyMadeMoves)
        max_count = ply + finalyMadeMoves;
    else
        max_count = reversibleMoves;

    for(unsigned short i = 0; i < max_count - 1; ++i)
    {
        menTmp[MOVEPC(doneMoves[i])] = MOVESCR(doneMoves[i]);
        if(memcmp(menTmp, men, sizeof(men)) == 0)
            return true;
        ++i;
        menTmp[MOVEPC(doneMoves[i])] = MOVESCR(doneMoves[i]);
    }
    return false;
}
#endif

//--------------------------------------
void MakeNullMove()
{
#ifndef NDEBUG
    strcpy(&curVar[5*ply], "NULL ");
    if((!stopPly || rootPly == stopPly) && strcmp(stop_str, cv) == 0)
        ply = ply;
#endif // NDEBUG

    boardState[PREV_STATES + ply + 1] = boardState[PREV_STATES + ply];
    boardState[PREV_STATES + ply].to = MOVE_IS_NULL;
    boardState[PREV_STATES + ply  + 1].ep = 0;

    ply++;

#ifndef NOT_USE_HASH_TABLE
    hash_key ^= -1ULL;
    doneHashKeys.at(FIFTY_MOVES + ply - 1) = hash_key;
#endif // NOT_USE_HASH_TABLE

    wtm ^= WHT;
}

//--------------------------------------
void UnMakeNullMove()
{
    wtm ^= WHT;
    curVar[5*ply] = '\0';
    ply--;
#ifndef NOT_USE_HASH_TABLE
    hash_key ^= -1ULL;
#endif // NOT_USE_HASH_TABLE
}

//-----------------------------
bool NullMove(int depth, short beta, bool in_check)
{
    if(in_check || depth < 3
    || material[wtm >> 5] - pieces[wtm >> 5] < 3)
        return false;

    if(boardState[PREV_STATES + ply - 1].to == MOVE_IS_NULL
//    && boardState[PREV_STATES + ply - 2].to == MOVE_IS_NULL
      )
        return false;

    UC store_ep     = boardState[PREV_STATES + ply].ep;
    UC store_to     = boardState[PREV_STATES + ply].to;
    US store_rv     = reversibleMoves;
    reversibleMoves = 0;

    MakeNullMove();
#ifndef NOT_USE_HASH_TABLE
    if(store_ep)
        hash_key = InitHashKey();
#endif // NOT_USE_HASH_TABLE

    int r = depth > 6 ? 3 : 2;
    short x = -Search(depth - r - 1, -beta, -beta + 1);

    UnMakeNullMove();
    boardState[PREV_STATES + ply].to    = store_to;
    boardState[PREV_STATES + ply].ep    = store_ep;
    reversibleMoves                     = store_rv;

#ifndef NOT_USE_HASH_TABLE
    if(store_ep)
        hash_key = InitHashKey();
#endif // NOT_USE_HASH_TABLE

    return (x >= beta);
}

//-----------------------------
bool Futility(int depth, short beta)
{
    if(boardState[PREV_STATES + ply].capt == 0
    && boardState[PREV_STATES + ply - 1].to   != 0xFF
    )
    {
        short margin = depth == 0 ? 350 : 550;
        margin += beta;
        if((wtm  &&  valOpn > margin &&  valEnd > margin)
        || (!wtm && -valOpn > margin && -valEnd > margin))
        {
            futCr++;
            return true;
        }
    }
    return false;
}

#ifndef NOT_USE_HASH_TABLE
//--------------------------------
bool DrawByRepetition()
{
    if(reversibleMoves < 4)
        return false;

    unsigned max_count;
    if(reversibleMoves > ply + finalyMadeMoves)
        max_count = ply + finalyMadeMoves;
    else
        max_count = reversibleMoves;

    if(max_count > FIFTY_MOVES + ply)
        max_count = FIFTY_MOVES + ply;                                  // on case that GUI does not recognize 50 move rule
    unsigned i;
    for(i = 4; i <= max_count; i += 2)
    {
        if(hash_key == doneHashKeys.at(FIFTY_MOVES + ply - i))
            return true;
    }

    return false;
}
#endif // NOT_USE_HASH_TABLE

//--------------------------------
void ShowFen()
{
    char whites[] = "KQRBNP";
    char blacks[] = "kqrbnp";
    UC pt;
    int blankCr;
    for(int row = 7; row >= 0; --row)
    {
        blankCr = 0;
        for(int col = 0; col < 8; ++col)
        {
            pt = b[XY2SQ(col, row)];
            if(pt == __)
            {
                blankCr++;
                continue;
            }
            if(blankCr != 0)
                std::cout << blankCr;
            blankCr = 0;
            if(pt & WHT)
                std::cout << whites[pt - WHT - 1];
            else
                std::cout << blacks[pt - 1];
        }
        if(blankCr != 0)
            std::cout << blankCr;
        if(row != 0)
            std::cout << "/";
    }
    std::cout << " " << (wtm ? 'w' : 'b');

    std::cout << std::endl;
}

//--------------------------------
void ReHash(int size_MB)
{
    busy = true;
    hashSize = size_MB*HASH_ENTRIES_PER_MB;
    hash_table.rehash(hashSize);
    busy = false;
}

/*
r4rk1/ppqn1pp1/4pn1p/2b2N1P/8/5N2/PPPBQPP1/2KRR3 w - - 0 18 Nxh6?! speed test position
2k1r2r/1pp3pp/p2b4/2p1n2q/6b1/1NQ1B3/PPP2PPP/R3RNK1 b - - bm Nf3+; speed test position
r4rk1/1b3p1p/pq2pQp1/n5N1/P7/2RBP3/5PPP/3R2K1 w - - 0 1 bm Nxh7;   speed test position
1B6/3k4/p7/1p4b1/1P1PK1p1/P7/8/8 w - - 2 59 am Bf4;                speed test position

5rk1/1p4pp/2p5/2P1Nb2/3P2pq/7r/P2Q1P1P/R4RK1 w - - 5 26 eval of king safety
8/2p2kp1/Qp5p/7r/2P5/7P/Pq4P1/5R1K b - - 0 48 am Ke7 eval of king safety
r6r/pbpq1p2/1pnp1k1p/3Pp1p1/2P4P/2QBPPB1/P5P1/2R2RK1 b - - 0 8 eval of king safety

3r4/3P1p2/p4Pk1/4r1p1/1p4Kp/3R4/8/8 b - - 0 1 Rxd7 ?
4n3/3N4/4P3/3P2p1/p1K5/7k/8/8 w - - 0 58 wrong connected promos eval

8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1 fixed MovePawnStruct()
r1b2rk1/ppq3pp/2nb1n2/3pp3/3P3B/3B1N2/PP2NPPP/R2Q1RK1 w - - 0 1 qb3? fixed: different values at ply 1 and 2, pv is the same
r1bqkb1r/pppppppp/2n2n2/8/3P4/2N2N2/PPP1PPPP/R1BQKB1R b KQkq d3 0 3 return value -32760 at level 1000 nodes per move

6k1/8/8/4pQ2/3p1q2/KP6/P7/8 w - - 1 52 Qxf4? eval of unstoppables improved
3r1rk1/1p3ppp/pnq5/4p1P1/6Q1/1BP1R3/PP3PP1/2K4R b - - 0 17 rotten position
8/6k1/8/6pn/pQ6/Pp3P1K/1P3P1P/6r1 w - - 2 31 draw is inevitable
7k/7P/2Kr1PP1/8/8/8/8/1R6 w - - 27 73 am Kb5 Kb6 Kb7;
3k4/3R4/p3B3/2B1p3/1n2Pp2/5Pq1/7p/7K b - - 15 66 null move draw bug fixed
6k1/4b3/1p6/1Pr1PRp1/5p1p/5P1P/6P1/3R3K w - - 1 64 am Rd7;
2kb1R2/8/2p2B2/1pP2P2/1P6/1K6/8/3r4 w - - 0 1 bm Be7; got it faster without null move
r1bq1rk1/b2nnppB/p3p3/P3P3/2pP4/2P2N2/4QPPP/R1B2RK1 b - - 0 17 rotten position
8/p2r4/1n1R4/R3P1k1/P1p5/2P3p1/2P4p/7K b - - 4 47 wrong connected promos eval
8/k7/2K5/1P6/8/8/8/8 w - - 0 1 fixed hash cut errors
1r6/p1p1nk1p/2pb1p2/3p1b1P/3P4/3NKP2/PPP1N1r1/R1B4R w - - 0 1 am h6;
8/1p6/1P6/8/7K/8/8/1k6 w - - 0 1 hash errors fixed
r1r3k1/1pQ3p1/1R5p/3qNp2/p2P4/4B3/5PPP/6K1 w - - 2 28  rotten position
r7/2P5/8/8/1K6/4k2B/8/8 w - - 13 76 KBk eval fixed
8/5kp1/1r1pR3/p2K3p/2P5/7P/6P1/8 w - - 0 56 am Rxd6;
8/5kp1/3K4/p6p/2P5/7P/6P1/8 b - - 0 2; fixed different evals in debug and release
8/6p1/8/3k4/K5Pp/4R2P/8/1r6 w - - 3 74 am Rb3;
r5k1/2rb1ppp/ppq2P2/2npP3/3B4/2RBQ3/2P3PP/5RK1 b - - 0 25  am g6; @ply 5
8/8/8/8/8/1pk5/8/K7 b - - 11 93 draw is inevitable
8/8/1p6/8/Pp6/1k2K3/8/8 w - - 0 11 strange eval
rnb2rk1/pp2qppp/4p3/2ppP3/3P2Q1/2PB1N2/P1P2PPP/R3K2R b KQ - 3 10 am c4;
3r4/p4p1p/2P2k2/1P3p2/P6p/8/2R5/6K1 w - - 0 44  pawn eval
r1bqk1nr/pp1p1pbp/6p1/2B1p3/4P3/2N5/PPP2PPP/R2QKB1R b KQkq - 1 8 am Nf6;
r1bq1rk1/p1p1nppp/1pn1p3/3pP3/3P3P/2PB1N2/P1P2PP1/1RBQK2R b K h3 0 9 am Bb7
r2q1rk1/1pp2ppp/p1n1b3/2P5/1P3R2/P2BQ3/1B1P2PP/R5K1 b - - 0 19 am Qd7
8/2pk1p1p/8/2p3KP/4P3/p1PP4/Bp3P2/8 w - - 4 44 pawn eval
r3r2k/1p1R3p/p4p2/P7/n1p2B2/2P1P2R/2P5/5K2 b - - 3 46 am Rad8;
2rr2k1/1qp2p1p/2bn2pP/1pQpP1P1/p2P1B2/P1PB1P2/4R3/2K1R3 b - - 0 42 rotten position
6k1/1q3p1p/4pQp1/2p1P3/p1p1bPP1/P1B4P/1P3R1K/3r4 w - - 3 41 Rf1? @ply 6 ?
5R2/8/8/8/6p1/5n2/2k4P/5K2 w KQkq - 0 1 bm Rxf3; add setboard castle rights check
3r1bn1/pp3p1r/1q2b1p1/2p1pk1p/1PP1N3/P3P1PP/QBnP4/RN1K1BR1 w - - 2 23 search explosion @ply 2
rn3rk1/3pbppp/b4n2/2pP4/1qB5/1QN2N2/PP3PPP/R1B1K2R w KQ - 7 15 am Ne5
r1b1k2r/pp3ppp/2p1q3/3nP3/3P4/b1BB1NP1/P1P4P/R2QK2R w KQkq - 5 15  am Ng5
3r2k1/1q3pp1/p6p/4R2Q/1n6/1p1P2P1/1P3P1P/4R1K1 b - - 2 32 am Nxd3; @ply 9
8/6B1/4k3/7P/5p2/3r2p1/p4PK1/8 b - - 1 87 am Rf3 wrong hashing fixed
2b4k/8/2p2p2/5p1p/3P4/2pK1NQP/1qP2PP1/8 w - - 7 44 search explosion @ply 12
rnbqkb1r/1p1n1ppp/p3p3/1BppP3/3P4/2N2N2/PPP2PPP/R1BQK2R w KQkq - 0 7 am Ba4; @ply 6
8/5p2/4p3/6k1/8/P3K1pP/8/8 w - - 0 48 am a4
8/7p/4b2P/2K5/1B4p1/1k3pP1/5P2/8 w - - 76 100 am Be1
5nkr/4Qbpp/2p2p2/1qP2N2/3P4/p7/5PPP/3RB1K1 w - - 0 34 am Rd2
8/p1k4p/3p1p2/1p1P1K2/2p2P2/7P/1N6/8 w - - 0 40 bm Ke4
8/8/8/8/1N1k4/8/1r6/3K4 w - - 0 1 am Nc6
8/8/8/R7/4bk2/7K/8/6r1 w - - 98 180 bm Ra2; fifty move rule bug with mate at last ply fixed
3q3k/4bQp1/p1p1P2p/3p4/r1p4P/6P1/5P2/1B2R1K1 w - - 10 36 search explosion @ply 11
1r6/p1p2ppp/3n1k2/1r6/4B3/P6P/1P1R1PP1/4RK2 w - - 1 31 am Bxh7
2r2b1r/3kp1q1/1Qp1Rp2/2P5/3P3p/2N5/PP3PPB/4R1K1 w - - 0 1 fixed out of board array bounds
8/1p6/P1p5/P1P4P/4k3/2Kp4/8/5q2 b - - 0 68 uci mate score display fixed
8/1b1pk1P1/3p1p1P/3P1K2/P1P5/8/4B3/8 w - - 3 54 g8R?
5k2/1p4p1/p2q2bp/4p3/3pP3/1P1Pn2P/P1PQ2P1/2R3K1 b - - 1 28 am Ke7; king safety

1r4k1/p6p/2p1b1p1/2q1bp1B/4p3/4N3/PPPRQPPP/2K5 w - - 1 25; king safety (KS) eval
r5k1/1p3q1p/3p2p1/2pPp3/2Pb1rp1/3P3P/PPQ1RPB1/R5K1 w - - 0 26; KS eval
r4rk1/3b2qp/1p1p4/1Pp1P3/2Pb1p2/1KNR1Q2/P2NB1P1/8 w - - 1 46; KS eval
1r3rk1/3b2b1/p1pq3p/3p2p1/N3p3/2P1Q3/PP1N1PPP/1R1R2K1 b - - 1 21; KS eval
6k1/Qn2q1pp/8/2p5/3pN3/P4r2/1P3P1P/R1B3K1 w - - 0 29; KS eval, bad rook, bishop and queen, no only move ext
r5k1/1pp2r1p/1b1p4/3Pp3/p1Q1P1p1/PnBP1BPq/1P3P1P/4RRK1 w - - 1 27; KS eval, no only move ext
8/b7/P5k1/1P6/2KpN1p1/8/8/8 w - - 14 66; am Kd5
5k2/p6P/1pp1p1P1/8/7r/3B4/2P2P2/5K2 b - - 0 45; connected promos eval
k4q2/1p6/8/2p1P3/r7/8/3Q1P1r/4RKR1 w - - 0 53 am Rg2; no only move ext
k3b1rr/p7/P3p3/4P1b1/3NK3/5R2/1P3B2/R7 w - - 9 58; king is too central
8/4p1k1/3pP1p1/3P2P1/5P2/8/r1BK4/8 b - - 68 84; eval: white's better?
*/

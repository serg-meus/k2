#include "engine.h"

//--------------------------------
char        stop_str[] = "";
unsigned    stopPly   = 0;

Timer       timer;
double      time0;
double      timeSpent, timeToThink;
unsigned    timeMaxNodes, timeMaxPly;
unsigned    rootPly, rootTop, rootMoveCr;
bool        stop, analyze, busy;
Move        rootMoveList[MAX_MOVES];
UQ          qnodes, cutCr, cutNumCr[5], qCutCr, qCutNumCr[5];
UQ          nullProbeCr, nullCutCr, hashProbeCr, hashHitCr, hashCutCr, hashHitCutCr;
UQ          totalNodes, futProbes, futHits;
double      timeBase, timeInc, timeRemains, totalTimeSpent;
unsigned    movesPerSession;
int         finalyMadeMoves, movsRemain;
bool        spentExactTime;
unsigned    resignCr, pv_stable_cr;
bool        timeCommandSent;

transposition_table tt;
UQ  doneHashKeys[FIFTY_MOVES + max_ply];

//--------------------------------
short Search(int depth, short alpha, short beta, int lmr_)
{
    if(ply >= max_ply - 1 || DrawDetect())
    {
        pv[ply][0].flg = 0;
        return 0;
    }
    bool in_check = Attack(*king_coord[wtm], !wtm);
    if(in_check)
        depth += 1 + lmr_;

#ifndef DONT_USE_MATE_DISTANCE_PRUNING
    short mate_score = (short)(K_VAL - ply);
    if(alpha >= mate_score)
       return alpha;
    if(beta <= -mate_score)
        return beta;
#endif  // DONT_USE_MATE_DISTANCE_PRUNING

#ifndef DONT_USE_FUTILITY
    if(depth <= 1 && depth >= 0 && !in_check
    && beta < (short)(K_VAL - max_ply)
    && Futility(depth, beta))
        return beta;
#endif // DONT_USE_FUTILITY

    short x, _alpha = alpha;
    bool in_hash = false;
    tt_entry entry;
    if(depth > 0 && HashProbe(depth, alpha, beta, &entry, &in_hash))
        return -entry.value;

    if(depth <= 0)
    {
        x = Quiesce(alpha, beta);
/*
        if(x > alpha && x < beta)
        {
            alpha = x;
            pv[ply][0].flg = 1;
        }
        Move nm;
        nm.flg = 0xFF;
        StoreResultInHash(0, _alpha, alpha, beta, 2, x >= beta, nm);
        pv[ply][0].flg = 0;
*/
        return x;
    }


    nodes++;
    if((nodes & 511) == 511)
        CheckForInterrupt();

#ifndef DONT_USE_NULL_MOVE
/*    if(in_hash && entry.avoid_null_move
    && entry.bound_type == hUPPER && -entry.value <= alpha)
        ply = ply;
    else */
    if(beta - alpha == 1 && NullMove(depth, beta, in_check, lmr_))
        return beta;
#endif // DONT_USE_NULL_MOVE

#ifndef DONT_USE_ONLY_MOVE_EXTENSION
    if(in_hash && entry.only_move)
        depth += 1;
#endif // DONT_USE_ONLY_MOVE_EXTENSION

    Move moveList[MAX_MOVES], m, m_best;
    unsigned move_cr = 0, legals = 0, max_moves = 999;
    bool beta_cutoff = false;

    boardState[prev_states + ply].valOpn = valOpn;
    boardState[prev_states + ply].valEnd = valEnd;

//    bool mateFound = alpha >= (short)(K_VAL - max_ply + 1);

    for(; move_cr < max_moves && !stop/* && !mateFound*/; move_cr++)
    {
        m = Next(moveList, move_cr, &max_moves,
                 &in_hash, entry, wtm, false);
        if(max_moves <= 0)
            break;
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
#ifdef NDEBUG
        if(depth > 5 && legals == 0
        && beta != alpha + 1 && m.scr < PV_FOLLOW)
            ply = ply;
#endif // NDEBUG
        FastEval(m);

#ifndef DONT_USE_LMR
        int lmr = 1;
        if(depth < 3 || m.flg || in_check)
            lmr = 0;
        else if(legals < 4/* || m.scr > 80*/)
            lmr = 0;
        else if((b[m.to] & ~white) == _p && TestPromo(COL(m.to), !wtm))
            lmr = 0;
#else
        int lmr = 0;
#endif  // DONT_USE_LMR

        if(legals == 0)
            x = -Search(depth - 1, -beta, -alpha, 0);
        else if(beta - alpha > 1)
        {
            x = -Search(depth - 1 - lmr, -alpha - 1, -alpha, lmr);
            if(x > alpha/* && x < beta*/)
                x = -Search(depth - 1, -beta, -alpha, 0);
        }
        else
        {
            x = -Search(depth - 1 - lmr, -beta, -alpha, lmr);
            if(lmr && x > alpha)
                x = -Search(depth - 1, -beta, -alpha, 0);
        }
        legals++;

        if(x >= beta)
        {
            beta_cutoff = true;
            UnMove(m);
            break;
        }
        else if(x > alpha)
        {
            alpha = x;
            StorePV(m);
        }
        UnMove(m);
    }// for move_cr

    if(!legals && _alpha < (short)(K_VAL - max_ply + 1))
    {
        pv[ply][0].flg = 0;
        return in_check ? -K_VAL + ply : 0;
    }
    else if(legals)
    {
        StoreResultInHash(depth, _alpha, alpha, beta, legals,
                          beta_cutoff, (beta_cutoff ? m : moveList[0]));
#ifndef DONT_USE_ONLY_MOVE_EXTENSION
        if(legals == 1 && !in_hash && hash_table.size() < hashMaxSize)
        {
            bool om = DetectOnlyMove(beta_cutoff, in_check,
                                     move_cr, max_moves, moveList);
            hash_table[hash_key].only_move = om;
        }
#endif // DONT_USE_ONLY_MOVE_EXTENSION
    }// else if legals

    if(beta_cutoff)
    {
#ifndef DONT_SHOW_STATISTICS
        if(in_hash && legals == 1)
            hashHitCutCr++;
#endif //DONT_SHOW_STATISTICS
        assert(legals > 0);
        UpdateStatistics(m, depth, legals - 1);
        return beta;
    }
    return alpha;
}

//-----------------------------
short Quiesce(short alpha, short beta)
{
    if(ply >= max_ply - 1)
        return 0;
    nodes++;
    qnodes++;
    if((nodes & 511) == 511)
        CheckForInterrupt();

    pv[ply][0].flg = 0;

    boardState[prev_states + ply].valOpn = valOpn;
    boardState[prev_states + ply].valEnd = valEnd;

    short x = Eval(/*alpha, beta*/);

    if(-x >= beta)
        return beta;
    else if(-x > alpha)
        alpha = -x;

    Move moveList[MAX_MOVES];
    unsigned move_cr = 0, max_moves = 999, legals = 0;
    bool beta_cutoff = false;

    for(; move_cr < max_moves && !stop; move_cr++)
    {
        tt_entry hs;
        bool bm_not_hashed = false;
        Move m = Next(moveList, move_cr, &max_moves, &bm_not_hashed, hs, wtm, true);
        if(max_moves <= 0)
            break;

#ifndef DONT_USE_SEE_CUTOFF
        if(m.scr <= BAD_CAPTURES && SEE_main(m) < 0)
            continue;
#endif
#ifndef DONT_USE_DELTA_PRUNING
        if(material[0] + material[1] > 24
        && (wtm ? valOpn : -valOpn)
        + 100*pc_streng[b[m.to]/2] < alpha - 450)
            continue;
#endif
        MkMove(m);
        legals++;
#ifndef NDEBUG
        if((!stopPly || rootPly == stopPly) && strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
        if((boardState[prev_states + ply].capt & ~white) == _k)
        {
            UnMove(m);
            return K_VAL;
        }
        FastEval(m);

        x = -Quiesce(-beta, -alpha);

        if(x >= beta)
        {
            beta_cutoff = true;
            UnMove(m);
            break;
        }
        else if(x > alpha)
        {
            alpha   = x;
            StorePV(m);
        }
        UnMove(m);
    }// for(move_cr

    if(beta_cutoff)
    {
#ifndef DONT_SHOW_STATISTICS
        qCutCr++;
        assert(legals > 0);
        if(legals - 1 < (int)(sizeof(qCutNumCr)/sizeof(*qCutNumCr)))
            qCutNumCr[legals - 1]++;
#endif // DONT_SHOW_STATISTICS
        return beta;
    }
    return alpha;
}

//--------------------------------
void Perft(int depth)
{
    Move moveList[MAX_MOVES];
    bool in_check = Attack(*king_coord[wtm], !wtm);
    int max_moves = GenMoves(moveList, APPRICE_NONE, nullptr);
    for(int move_cr = 0; move_cr < max_moves; move_cr++)
    {
#ifndef NDEBUG
        if((unsigned)depth == timeMaxPly)
            tmpCr = nodes;
#endif
        Move m = moveList[move_cr];
        MkMove(m);
#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
//        bool legal = !Attack(men[(wtm ^ white) + 1], wtm);
        bool legal = Legal(m, in_check);
        if(depth > 1 && legal)
            Perft(depth - 1);
        if(depth == 1 && legal)
            nodes++;
#ifndef NDEBUG
        if((unsigned)depth == timeMaxPly && legal)
            std::cout << cv << nodes - tmpCr << std::endl;
#endif
        UnMove(m);
    }
}

//-----------------------------
void StorePV(Move m)
{
    int nextLen = pv[ply][0].flg;
    pv[ply - 1][0].flg = nextLen + 1;
    pv[ply - 1][1] = m;
    memcpy(&pv[ply - 1][2], &pv[ply][1], sizeof(Move)*nextLen);
}

//-----------------------------
void UpdateStatistics(Move m, int dpt, unsigned move_cr)
{
#ifndef DONT_SHOW_STATISTICS
        cutCr++;
        if(move_cr < sizeof(cutNumCr)/sizeof(*cutNumCr))
            cutNumCr[move_cr]++;
#else
    UNUSED(move_cr);
#endif // DONT_SHOW_STATISTICS
    if(m.flg)
        return;
    if(m != kil[ply][0] && m != kil[ply][1])
    {
        kil[ply][1] = kil[ply][0];
        kil[ply][0] = m;
    }

#ifndef DONT_USE_HISTORY
    auto it = coords[wtm].begin();
    it = m.pc;
    UC fr = *it;
    unsigned &h = history[wtm][(b[fr]/2) - 1][m.to];
    h += dpt*dpt + 1;
#else
    UNUSED(dpt);
#endif // DONT_USE_HISTORY
}

//--------------------------------
void MainSearch()
{
    busy = true;
    InitSearch();

    short sc = 0, sc_;
    rootPly = 2;
    rootTop = 0;
    pv_stable_cr = 0;
    for(; rootPly <= max_ply && !stop; ++rootPly)
    {
        sc_ = sc;
        sc = RootSearch(rootPly, -INF, INF);
        if(stop && sc == -INF)
            sc = sc_;

        double time1 = timer.getElapsedTimeInMicroSec();
        timeSpent = time1 - time0;

        if(!analyze && !ponder)
        {
            if(timeSpent > timeToThink && !timeMaxNodes)
                break;
            if(ABSI(sc) > (short)(K_VAL - max_ply) && !stop)
                break;
            if(rootTop == 1 && rootPly >= 8)
                break;
        }
        if(rootPly >= timeMaxPly)
            break;

    }// for(rootPly

    if(ponder && spentExactTime)
    {
        ponder = false;
        spentExactTime = false;
    }
    if(sc < -RESIGN_VALUE)
        resignCr++;
    else
        resignCr = 0;

    if((!stop && !analyze && sc < short(-K_VAL + max_ply))
    || (!analyze && sc < -RESIGN_VALUE && resignCr > RESIGN_MOVES))
    {
        std::cout << "resign" << std::endl;
        busy = false;
    }

    totalNodes      += nodes;
    totalTimeSpent  += timeSpent;

    timer.stop();

    if(!analyze || uci)
        PrintSearchResult();

    if(!xboard)
        analyze = false;
    busy = false;
}

//--------------------------------
short RootSearch(int depth, short alpha, short beta)
{
    bool in_check = Attack(*king_coord[wtm], !wtm);
    boardState[prev_states + ply].valOpn = valOpn;
    boardState[prev_states + ply].valEnd = valEnd;
    if(!rootTop)
        RootMoveGen(in_check);

    rootMoveCr = 0;

    UQ prevDeltaNodes = 0;

    short x;
    Move  m;
    bool beta_cutoff = false;

    for(; rootMoveCr < rootTop/* && alpha < K_VAL - 99*/ && !stop; rootMoveCr++)
    {
        m = rootMoveList[rootMoveCr];

        MkMove(m);

#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0 && (!stopPly || rootPly == stopPly))
            ply = ply;
#endif // NDEBUG

        FastEval(m);
        UQ _nodes = nodes;

        if(uci && rootPly > 6)
            ShowCurrentUciInfo();

        if(DrawByRepetition())
        {
            x = 0;
            pv[1][0].flg = 0;
        }
        else
        {
#ifndef DONT_USE_PVS_IN_ROOT
            if(rootMoveCr == 0 || pv_stable_cr < 2)
            {
                x = -Search(depth - 1, -beta, -alpha, 0);
                if(stop)
                    x = -INF;
            }
            else
            {
                x = -Search(depth - 1, -alpha - 1, -alpha, 0);
                if(stop)
                    x = -INF;
                if(!stop && x > alpha)
                {
                    pv_stable_cr = 0;
                    ShowPVfailHigh(m, x);

                    short x_ = -Search(depth - 1, -beta, -alpha, 0);
                    if(stop)
                        ply = ply;
                    if(!stop)
                        x = x_;
                    if(x > alpha)
                    {
                        pv[0][0].flg    = 1;
                        pv[0][1]        = m;
                    }
                }
            }
#else
        x = -Search(depth - 1, -beta, -alpha, 0);
        if(stop)
            x = -INF;
#endif // DONT_USE_PVS_IN_ROOT
        }

        UQ dn = nodes - _nodes;

        if(x <= alpha && rootMoveCr > 2 && dn > prevDeltaNodes && depth > 2)
        {
            Move tmp = rootMoveList[rootMoveCr];
            rootMoveList[rootMoveCr] =
                rootMoveList[rootMoveCr - 1];
            rootMoveList[rootMoveCr - 1] = tmp;
        }
        prevDeltaNodes = dn;

        if(x >= beta)
        {
            beta_cutoff = true;
            UnMove(m);
            break;
        }
        else if(x > alpha)
        {
            if(rootMoveCr > 0)
                pv_stable_cr = 0;
            alpha = x;
            StorePV(m);
            UnMove(m);

            if(depth > 3 && x != -INF
            && !(stop && rootMoveCr < 2))
                PlyOutput(x);
            if(rootMoveCr != 0)
            {
                Move tmp = rootMoveList[rootMoveCr];
                memmove(&rootMoveList[1], &rootMoveList[0],
                        sizeof(Move)*rootMoveCr);
                rootMoveList[0] = tmp;
            }
        }
        else
        {
            UnMove(m);
//            if(MOVEPC(probed) == 0xFF)
//                probed = m;
        }
    }// for(; rootMoveCr < rootTop

    pv_stable_cr++;
    if(depth <= 3 && rootTop)
        PlyOutput(alpha);
    if(!rootTop)
    {
        pv[0][0].flg = 0;
        return in_check ? -K_VAL + ply : 0;
    }
    if(beta_cutoff)
    {
        UpdateStatistics(m, depth, rootMoveCr);
        return beta;
    }
    return alpha;
}

//--------------------------------
void ShowPVfailHigh(Move m, short x)
{
    UnMove(m);
    char mstr[6];
    MoveToStr(m, wtm, mstr);

    Move tmp0       = pv[0][0];
    Move tmp1       = pv[0][1];
    pv[0][0].flg    = 1;
    pv[0][1]        = m;

    PlyOutput(x);
    pv[0][0].flg    = tmp0.flg;
    pv[0][1]        = tmp1;

    MkMove(m);
    FastEval(m);
}

//--------------------------------
void RootMoveGen(bool in_check)
{
    Move moveList[MAX_MOVES], m;
//    rootTop = GenMoves(moveList, APPRICE_ALL, nullptr);
    unsigned max_moves = 999;
    boardState[prev_states + ply].valOpn = valOpn;
    boardState[prev_states + ply].valEnd = valEnd;
    for(unsigned move_cr = 0; move_cr < max_moves; move_cr++)
    {
        tt_entry hs;
        bool bm_not_hashed = false;
        m = Next(moveList, move_cr, &max_moves, &bm_not_hashed, hs, wtm, false);
    }
    for(unsigned move_cr = 0; move_cr < max_moves; move_cr++)
    {
        m = moveList[move_cr];
        MkMove(m);
        if(Legal(m, in_check))
            rootMoveList[rootTop++] = m;
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
    futProbes   = 0;
    nullCutCr   = 0;
    nullProbeCr = 0;
    hashCutCr   = 0;
    hashProbeCr = 0;
    hashHitCr   = 0;
    hashHitCutCr = 0;
    futHits     = 0;

    unsigned i, j;
    for(i = 0; i < sizeof(cutNumCr)/sizeof(*cutNumCr); i++)
        cutNumCr[i] = 0;
    for(i = 0; i < sizeof(qCutNumCr)/sizeof(*qCutNumCr); i++)
        qCutNumCr[i] = 0;
    for(i = 0; i < max_ply; i ++)
        for(j = 0; j < max_ply + 1; j++)
        {
            pv[i][j].to = 0;
            pv[i][j].flg = 0;
            pv[i][j].scr = 0;
        }
    for(i = 0; i < max_ply; i++)
    {
        kil[i][0].flg =  0xFF;                                              //>> NB
        kil[i][1].flg =  0xFF;
    }

    stop = false;

    if(!uci && !xboard)
    {
    std::cout   << "( tRemain=" << timeRemains/1e6
                << ", t2thnk=" << timeToThink/1e6
                << ", tExact = " << (spentExactTime ? "true " : "false ")
                << " )" << std::endl;
    std::cout   << "Ply Value  Time    Nodes        Principal Variation" << std::endl;
    }

    tt.clear();

#ifndef DONT_USE_HISTORY
    memset(history, 0, sizeof(history));
#endif // DONT_USE_HISTORY
}

//--------------------------------
void MoveToStr(Move m, bool stm, char *out)
{
    char proms[] = {'?', 'q', 'n', 'r', 'b'};

    auto it = coords[stm].begin();
    it      = m.pc;
    int  f  = *it;
    out[0]  = COL(f) + 'a';
    out[1]  = ROW(f) + '1';
    out[2]  = COL(m.to) + 'a';
    out[3]  = ROW(m.to) + '1';
    out[4]  = (m.flg & mPROM) ? proms[m.flg & mPROM] : '\0';
    out[5]  = '\0';
}

//--------------------------------
void PrintSearchResult()
{
    char mov[6];
    MoveToStr(pv[0][1], wtm, mov);

    if(!uci && !MakeMoveFinaly(mov))
    {
        std::cout << "tellusererror err01" << std::endl << "resign" << std::endl;
    }

    if(!uci)
        std::cout << "move " << mov << std::endl;
    else
    {
        std::cout << "bestmove " << mov;
        if(!analyze && pv[0][0].flg > 1)
        {
            char pndr[6];
            MoveToStr(pv[0][2], !wtm, pndr);
            std::cout << " ponder " << pndr;
        }
        std::cout << std::endl;
    }

    if(xboard || uci)
        return;

#ifndef DONT_SHOW_STATISTICS
    std::cout << "( nodes = " << nodes
              << ", cuts = [";
    for(unsigned i = 0; i < sizeof(cutNumCr)/sizeof(*cutNumCr); i++)
        std::cout  << (int)(cutNumCr[i]/(cutCr/100 + 1)) << " ";
    std::cout << "]% )" << std::endl;

    std::cout << "( q_nodes = " << qnodes
              << ", q_cuts = [";
    for(unsigned i = 0; i < sizeof(qCutNumCr)/sizeof(*qCutNumCr); i++)
        std::cout  << (int)(qCutNumCr[i]/(qCutCr/100 + 1)) << " ";
    std::cout << "]%, ";

    std::cout << "q/n = " << (int)(qnodes/(nodes/100 + 1))
              << "% )" << std::endl;

    std::cout   << "( hash probes = " << hashProbeCr
                << ", cutoffs by value = "
                << (int)(hashCutCr/(hashProbeCr/100 + 1)) << "%, "
                << "cutoffs by best move = "
                << (int)(hashHitCutCr/(hashHitCr/100 + 1)) << "% )"
                << std::endl
                << "( hash full = " << (int)100*tt.size()/tt.max_size()
                << "% (" << tt.size()/sizeof(tt_entry)
                << "/" << tt.max_size()/sizeof(tt_entry)
                << " entries )" << std::endl;
#ifndef DONT_USE_NULL_MOVE
    std::cout   << "( null move probes = " << hashProbeCr
                << ", cutoffs = "
                << (int)(nullCutCr/(nullProbeCr/100 + 1)) << "% )"
                << std::endl;
#endif // DONT_USE_NULL_MOVE

#ifndef DONT_USE_FUTILITY
    std::cout << "( futility probes = " << futProbes
              << ", hits = " << (int)(futHits/(futProbes/100 + 1))
              << "% )" << std::endl;
#endif //DONT_USE_FUTILITY
    std::cout   << "( tSpent=" << timeSpent/1e6
                << " )" << std::endl;
#endif
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

            if(ABSI(sc) < (short)(K_VAL - max_ply))
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
    timeToThink = timeRemains / movsRemain;
    if(movsRemain != 1 && !spentExactTime)
        timeToThink /= 2;                           // average branching factor
    else if(timeInc != 0 && timeBase != 0)
        timeToThink = timeInc;

    timeCommandSent = false;
}

//-----------------------------
bool ShowPV(int _ply)
{
    char pc2chr[] = "??KKQQRRBBNNPP";
    bool ans = true;
    int i = 0, stp = pv[_ply][0].flg;

    if(uci)
    {
        for(; i < stp; i++)
        {
            Move m = pv[_ply][i + 1];
            auto it = coords[wtm].begin();
            it = m.pc;
            UC fr = *it;
            std::cout  << (char)(COL(fr) + 'a') << (char)(ROW(fr) + '1')
                << (char)(COL(m.to) + 'a') << (char)(ROW(m.to) + '1');
            char proms[] = {'?', 'q', 'n', 'r', 'b'};
            if(m.flg & mPROM)
                std::cout << proms[m.flg & mPROM];
            std::cout << " ";
            MkMove(m);
            bool in_check = Attack(*king_coord[wtm], !wtm);
            if(!Legal(m, in_check))
                ans = false;
        }
    }
    else
        for(; i < stp; i++)
        {
            Move m = pv[_ply][i + 1];
            auto it = coords[wtm].begin();
            it = m.pc;
            char pc = pc2chr[b[*it]];
            if(pc == 'K' && COL(*it) == 4 && COL(m.to) == 6)
                std::cout << "OO";
            else if(pc == 'K' && COL(*it) == 4 && COL(m.to) == 2)
                std::cout << "OOO";
            else if(pc != 'P')
            {
                std::cout << pc;
                Ambiguous(m);
                if(m.flg & mCAPT)
                    std::cout << 'x';
                std::cout << (char)(COL(m.to) + 'a');
                std::cout << (char)(ROW(m.to) + '1');
            }
            else if(m.flg & mCAPT)
            {
                auto it = coords[wtm].begin();
                it = m.pc;
                std::cout << (char)(COL(*it) + 'a') << "x"
                     << (char)(COL(m.to) + 'a') << (char)(ROW(m.to) + '1');
            }
            else
                std::cout << (char)(COL(m.to) + 'a') << (char)(ROW(m.to) + '1');
            char proms[] = "?QNRB";
            if(pc == 'P' && (m.flg & mPROM))
                std::cout << proms[m.flg& mPROM];
            std::cout << ' ';
            MkMove(m);
            bool in_check = Attack(*king_coord[wtm], !wtm);
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
    auto it = coords[wtm].begin();
    it = m.pc;
    UC fr0 = *it;
    UC pt0 = b[fr0]/2;

    for(auto it = coords[wtm].begin();
        it != coords[wtm].end();
        ++it)
    {
        if(it == m.pc)
            continue;
        UC fr = *it;

        UC pt = b[fr]/2;
        if(pt != pt0)
            continue;
//        pt &= ~white;
        if(!(attacks[120 + fr - m.to] & (1 << pt)))
            continue;
        if(slider[pt] && !SliderAttack(fr, m.to))
            continue;
        Move x = m;
        x.scr = fr;
        marr[ambCr++]   = x;
    }
    if(!ambCr)
        return;
    bool sameCols = false, sameRows = false;
    for(unsigned i = 0; i < ambCr; i++)
    {
        if(COL(marr[i].scr) == COL(fr0))
            sameCols = true;
        if(ROW(marr[i].scr) == ROW(fr0))
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
    bool in_check = Attack(*king_coord[wtm], !wtm);
    rootTop = 0;
    RootMoveGen(in_check);

    char rMov[6];
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    for(unsigned i = 0; i < rootTop; ++i)
    {
        Move m  = rootMoveList[i];
        auto it = coords[wtm].begin();
        it = m.pc;
        rMov[0] = COL(*it) + 'a';
        rMov[1] = ROW(*it) + '1';
        rMov[2] = COL(m.to) + 'a';
        rMov[3] = ROW(m.to) + '1';
        rMov[4] = (m.flg & mPROM) ? proms[m.flg & mPROM] : '\0';
        rMov[5] = '\0';

        if(strcmp(mov, rMov) == 0)
        {
            auto it = coords[wtm].begin();
            it = m.pc;
//            UC fr = *it;
            MkMove(m);

            FastEval(m);

            short _valOpn_ = valOpn;
            short _valEnd_ = valEnd;

            memmove(&boardState[0], &boardState[1],
                    (prev_states + 2)*sizeof(BrdState));
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
            for(int i = 0; i < FIFTY_MOVES; ++i)
                doneHashKeys[i] = doneHashKeys[i + 1];

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
    hash_key = InitHashKey();

    std::cin.rdbuf()->pubsetbuf(nullptr, 0);
    std::cout.rdbuf()->pubsetbuf(nullptr, 0);

    stop            = false;
    totalNodes      = 0;
    totalTimeSpent  = 0;

    spentExactTime  = false;
    finalyMadeMoves = 0;
    resignCr        = 0;
    timeSpent       = 0;
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
    hash_key = InitHashKey();
    std::cout << "( " << "hash_key = "
              << std::hex << hash_key
              << std::dec << " )" << std::endl;
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

    return DrawByRepetition();
}

//--------------------------------
void CheckForInterrupt()
{
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
    boardState[prev_states + ply].cstl   = boardState[prev_states + ply - 1].cstl;
    boardState[prev_states + ply].capt  = b[m.to];
    auto it = coords[wtm].begin();
    it      = m.pc;
    UC fr   = *it;
    UC targ = m.to;
    boardState[prev_states + ply].fr = fr;
    boardState[prev_states + ply].to = targ;
    boardState[prev_states + ply].reversibleCr = reversibleMoves;
    reversibleMoves++;
    if(m.flg & mCAPT)
    {
        if (m.flg & mENPS)
        {
            targ += wtm ? -16 : 16;
            material[!wtm]--;
        }
        else
            material[!wtm] -=
                    pc_streng[boardState[prev_states + ply].capt/2 - 1];

        auto it_cap = coords[!wtm].begin();
        auto it_end = coords[!wtm].end();
        for(; it_cap != it_end; ++it_cap)
            if(*it_cap == targ)
                break;
        assert(it_cap != it_end);
        boardState[prev_states + ply].captured_it = it_cap;
        coords[!wtm].erase(it_cap);

        pieces[!wtm]--;
        reversibleMoves = 0;
    }// if capture

    if((b[fr] <= _R) || (m.flg & mCAPT))        // trick: fast exit if not K|Q|R moves, and no captures
        specialMove |= MakeCastle(m, fr);

    boardState[prev_states + ply].ep = 0;
    if((b[fr] ^ wtm) == _p)
    {
        specialMove     |= MakeEP(m, fr);
        reversibleMoves = 0;
    }
#ifndef NDEBUG
    ShowMove(fr, m.to);
#endif // NDEBUG

    b[m.to]     = b[fr];
    b[fr]       = __;

    int prIx = m.flg & mPROM;
    UC prPc[] = {0, _q, _n, _r, _b};
    if(prIx)
    {
        b[m.to] = prPc[prIx] ^ wtm;
        material[wtm] += pc_streng[prPc[prIx]/2 - 1] - 1;
        boardState[prev_states + ply].nprom = ++it;
        --it;
        coords[wtm].move_element(king_coord[wtm], it);
        reversibleMoves = 0;
    }
    *it   = m.to;

    doneHashKeys[FIFTY_MOVES + ply - 1] = hash_key;
    MoveHashKey(m, fr, specialMove);
/*#ifndef NDEBUG
        UQ tmp_key = InitHashKey() ^ -1ULL;
        if(tmp_key != hash_key)
            ply = ply;
#endif //NDEBUG
*/
#ifndef DONT_USE_PAWN_STRUCT
    MovePawnStruct(b[m.to], fr, m);
#endif // DONT_USE_PAWN_STRUCT
    wtm ^= white;
}

//--------------------------------
void UnMove(Move m)
{
    UC fr = boardState[prev_states + ply].fr;
    auto it = coords[!wtm].begin();
    it = m.pc;
    *it = fr;
    b[fr] = (m.flg & mPROM) ? _P ^ wtm : b[m.to];
    b[m.to] = boardState[prev_states + ply].capt;

    reversibleMoves = boardState[prev_states + ply].reversibleCr;

    hash_key = doneHashKeys[FIFTY_MOVES + ply - 1];

    wtm ^= white;

    if(m.flg & mCAPT)
    {
        auto it_cap = coords[!wtm].begin();
        it_cap = boardState[prev_states + ply].captured_it;

        if(m.flg & mENPS)
        {
            material[!wtm]++;
            if(wtm)
            {
                b[m.to - 16] = _p;
                *it_cap = m.to - 16;
            }
            else
            {
                b[m.to + 16] = _P;
                *it_cap = m.to + 16;
            }
        }// if en_pass
        else
            material[!wtm]
                += pc_streng[boardState[prev_states + ply].capt/2 - 1];

        coords[!wtm].restore(it_cap);
        pieces[!wtm]++;
    }// if capture

    int prIx = m.flg & mPROM;
    UC prPc[] = {0, _q, _n, _r, _b};
    if(prIx)
    {
        auto it_prom = coords[wtm].begin();
        it_prom = boardState[prev_states + ply].nprom;
        auto before_king = king_coord[wtm];
        --before_king;
        coords[wtm].move_element(it_prom, before_king);
        material[wtm] -= pc_streng[prPc[prIx]/2 - 1] - 1;
    }

    if(m.flg & mCSTL)
        UnMakeCastle(m);

#ifndef DONT_USE_PAWN_STRUCT
    MovePawnStruct(b[fr], fr, m);
#endif // DONT_USE_PAWN_STRUCT

    ply--;
    valOpn = boardState[prev_states + ply].valOpn;
    valEnd = boardState[prev_states + ply].valEnd;

#ifndef NDEBUG
    curVar[5*ply] = '\0';
#endif // NDEBUG
}

//--------------------------------------
void MakeNullMove()
{
#ifndef NDEBUG
    strcpy(&curVar[5*ply], "NULL ");
    if((!stopPly || rootPly == stopPly) && strcmp(stop_str, cv) == 0)
        ply = ply;
#endif // NDEBUG

    boardState[prev_states + ply + 1] = boardState[prev_states + ply];
    boardState[prev_states + ply].to = MOVE_IS_NULL;
    boardState[prev_states + ply  + 1].ep = 0;

    ply++;

    hash_key ^= -1ULL;
    doneHashKeys[FIFTY_MOVES + ply - 1] = hash_key;

    wtm ^= white;
}

//--------------------------------------
void UnMakeNullMove()
{
    wtm ^= white;
    curVar[5*ply] = '\0';
    ply--;
    hash_key ^= -1ULL;
}

//-----------------------------
bool NullMove(int depth, short beta, bool in_check, int lmr_)
{
    if(in_check || depth < 3
    || material[wtm] - pieces[wtm] < 3)
        return false;
#ifndef DONT_SHOW_STATISTICS
    nullProbeCr++;
#endif // DONT_SHOW_STATISTICS

    if(boardState[prev_states + ply - 1].to == MOVE_IS_NULL
//    && boardState[prev_states + ply - 2].to == MOVE_IS_NULL
      )
        return false;

    UC store_ep     = boardState[prev_states + ply].ep;
    UC store_to     = boardState[prev_states + ply].to;
    US store_rv     = reversibleMoves;
    reversibleMoves = 0;

    MakeNullMove();
    if(store_ep)
        hash_key = InitHashKey();

    int r = depth > 6 ? 3 : 2;
    if(lmr_ > 0 && depth - r - 1 <= 0)
        r--;
    short x = -Search(depth - r - 1, -beta, -beta + 1, 0);

    UnMakeNullMove();
    boardState[prev_states + ply].to    = store_to;
    boardState[prev_states + ply].ep    = store_ep;
    reversibleMoves                     = store_rv;

    if(store_ep)
        hash_key = InitHashKey();

#ifndef DONT_SHOW_STATISTICS
    if(x >= beta)
        nullCutCr++;
#endif // DONT_SHOW_STATISTICS

    return (x >= beta);
}

//-----------------------------
bool Futility(int depth, short beta)
{
    if(boardState[prev_states + ply].capt == 0
    && boardState[prev_states + ply - 1].to   != 0xFF
    )
    {
#ifndef DONT_SHOW_STATISTICS
            futProbes++;
#endif // DONT_SHOW_STATISTICS
        short margin = depth == 0 ? 350 : 550;
        margin += beta;
        if((wtm  &&  valOpn > margin &&  valEnd > margin)
        || (!wtm && -valOpn > margin && -valEnd > margin))
        {
#ifndef DONT_SHOW_STATISTICS
            futHits++;
#endif // DONT_SHOW_STATISTICS
            return true;
        }
    }
    return false;
}

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
        if(hash_key == doneHashKeys[FIFTY_MOVES + ply - i])
            return true;
    }

    return false;
}

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
            if(pt & white)
                std::cout << whites[pt/2 - 1];
            else
                std::cout << blacks[pt/2 - 1];
        }
        if(blankCr != 0)
            std::cout << blankCr;
        if(row != 0)
            std::cout << "/";
    }
    std::cout << " " << (wtm ? 'w' : 'b') << " ";

    UC cstl = boardState[prev_states + 0].cstl;
    if(cstl & 0x0F)
    {
        if(cstl & 0x01)
            std::cout << "K";
        if(cstl & 0x02)
            std::cout << "Q";
        if(cstl & 0x04)
            std::cout << "k";
        if(cstl & 0x08)
            std::cout << "q";
    }
    else
        std::cout << "-";


    std::cout << " -";                                                  // No en passant yet

    std::cout << " " << reversibleMoves;
    std::cout << " " << finalyMadeMoves/2 + 1;

    std::cout << std::endl;
}

//--------------------------------
void ReHash(int size_mb)
{
    busy = true;
    tt.resize(size_mb);
    busy = false;
}

//--------------------------------
bool HashProbe(int depth, short alpha, short beta,
               tt_entry *entry,
               bool *in_hash)
{
    if(tt.count(hash_key, entry) == 0 || stop)
        return false;

#ifndef DONT_SHOW_STATISTICS
    hashProbeCr++;
#endif //DONT_SHOW_STATISTICS
    UC hbnd = entry->bound_type;
#ifndef DONT_USE_ONLY_MOVE_EXTENTION
    if(entry->depth >= depth)
#else
    if(entry->depth >= depth + entry->only_move)
#endif // DONT_USE_ONLY_MOVE_EXTENTION
    {
        short hval = entry->value;
        if( hbnd == hEXACT
        || (hbnd == hUPPER && hval >= -alpha)                       // -alpha is beta for parent node
        || (hbnd == hLOWER && hval <= -beta) )                      // -beta is alpha for parent node
        {
#ifndef DONT_SHOW_STATISTICS
            hashCutCr++;
#endif //DONT_SHOW_STATISTICS
            pv[ply][0].flg = 0;
            return true;
        }// if(bnd
    }// if(entry.depth >= depth
#ifndef DONT_SHOW_STATISTICS
    hashHitCr++;
#endif //DONT_SHOW_STATISTICS
    *in_hash = true;
    return false;
}

//-----------------------------
bool PseudoLegal(Move m, bool stm)
{
    if(m.flg == 0xFF)
        return false;
    auto it = coords[stm].begin();
    for(; it != coords[stm].end(); ++it)
        if(it == m.pc)
            break;
    if(it == coords[stm].end())
        return false;
    it = m.pc;
    UC fr = *it;
    UC pt = b[m.to];
    int dCOL = COL(m.to) - COL(fr);
    int dROW = ROW(m.to) - ROW(fr);
    if(!dCOL && !dROW)
        return false;
    if(!LIGHT(b[fr], stm))
        return false;
    if((b[fr]/2) != _p/2 && ((!DARK(pt, stm) && (m.flg & mCAPT))
    || (pt != __ && !(m.flg & mCAPT))))
        return false;
    switch(b[fr]/2)
    {
        case _p/2 :
            if((m.flg & mPROM) && ((stm && ROW(fr) != 6)
            || (!stm && ROW(fr) != 1)))
                return false;
            if((m.flg & mCAPT) && (ABSI(dCOL) != 1
            || (stm && dROW != 1) || (!stm && dROW != -1)
            || (!(m.flg & mENPS) && !DARK(pt, stm))))
                return false;
            if((m.flg & mENPS) && pt != __)
                return false;
            break;
        case _n/2 :
            if(ABSI(dCOL) + ABSI(dROW) != 3)
                return false;
            break;
        case _b/2 :
        case _r/2 :
        case _q/2 :
            if(!(attacks[120 + m.to - fr] & (1 << (b[fr]/2)))
            || !SliderAttack(m.to, fr))
                return false;
            break;
        case _k/2 :
            if(!(m.flg & mCSTL))
            {
                if((ABSI(dCOL) > 1 || ABSI(dROW) > 1))
                    return false;
                else
                    return true;
            }
            if(ABSI(dCOL) != 2 || ABSI(dROW) != 0)
                return false;
            if(b[XY2SQ(COL(m.to), ROW(fr))] != __)
                return false;
            if(b[XY2SQ((COL(m.to)+COL(fr))/2, ROW(fr))] != __)
                return false;
            break;
    }
    return true;
}

//--------------------------------
Move Next(Move *list, unsigned cur, unsigned *max_moves,
          bool *in_hash, tt_entry entry,
          UC stm, bool only_captures)
{
    Move ans;
    if(cur == 0)
    {
        if(!*in_hash)
        {
            if(!only_captures)
                *max_moves = GenMoves(list, APPRICE_ALL, nullptr);
            else
            {
                *max_moves = GenCaptures(list);
                AppriceQuiesceMoves(list, *max_moves);
            }
        }
        else
        {
            ans = entry.best_move;
            if(PseudoLegal(ans, stm))
            {
                ans.scr = PV_FOLLOW;
                return ans;
            }
            else
            {
                *in_hash = false;
                *max_moves = GenMoves(list, APPRICE_ALL, nullptr);
            }
        }
    }
    else if(cur == 1 && *in_hash)
    {
        *max_moves = GenMoves(list, APPRICE_ALL, &entry.best_move);
        for(unsigned i = cur; i < *max_moves; i++)
            if(list[i].scr == PV_FOLLOW && i != 0)
            {
                ans = list[0];
                list[0] = list[i];
                list[i] = ans;
                break;
            }
    }
    int max = -32000;
    unsigned imx = cur;

    for(unsigned i = cur; i < *max_moves; i++)
    {
        UC sc = list[i].scr;
        if(sc > max)
        {
            max = sc;
            imx = i;
        }
    }
    ans = list[imx];
    if(imx != cur)
    {
        list[imx] = list[cur];
        list[cur] = ans;
    }
    return ans;
}

//-----------------------------
void StoreResultInHash(int depth, short _alpha, short alpha,            // save results of search to hash table
                       short beta, unsigned legals,                     // note that this result is for 'parent' node (negative score, alpha = -beta, etc)
                       bool beta_cutoff, Move best_move)
{
    if(beta_cutoff)
        tt.add(hash_key, -beta, best_move, depth, hLOWER);
    else if(alpha > _alpha && pv[ply][0].flg > 0)
        tt.add(hash_key, -alpha, pv[ply][1], depth, hEXACT);
    else if(alpha <= _alpha)
    {
        Move no_move;
        no_move.flg = 0xFF;
        tt.add(hash_key, -_alpha, legals > 0 ? best_move : no_move, depth, hUPPER);
    }
#ifndef DONT_USE_ONLY_MOVE_EXTENSION
    if(!in_hash)
        hs.only_move = false;
#endif // DONT_USE_ONLY_MOVE_EXTENSION
}

//-----------------------------
bool DetectOnlyMove(bool beta_cutoff, bool in_check,
                    unsigned move_cr, unsigned max_moves,
                    Move *moveList)
{
    if(!beta_cutoff)
        return true;

    unsigned cr = move_cr;
    while(++cr < max_moves)
    {
        bool nh = false;
        tt_entry hs;
        hs.best_move.flg = 0xFF;
        Move tmp = Next(moveList, cr, &max_moves,
                 &nh, hs, wtm, false);
        MkMove(tmp);
        if(Legal(tmp, in_check))
        {
            UnMove(tmp);
            break;
        }
        UnMove(tmp);
    }// while(move_cr < max_moves
    if(cr >= max_moves)
        return true;

    return false;
}

//-----------------------------
void ShowCurrentUciInfo()
{
    double t = timer.getElapsedTimeInMicroSec();

    std::cout << "info nodes " << nodes
        << " nps " << (int)(1000000 * nodes / (t - time0 + 1));

    Move m = rootMoveList[rootMoveCr];
    UC fr = boardState[prev_states + 1].fr;
    std::cout << " currmove "
        << (char)(COL(fr) + 'a') << (char)(ROW(fr) + '1')
        << (char)(COL(m.to) + 'a') << (char)(ROW(m.to) + '1');
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    if(m.flg & mPROM)
        std::cout << proms[m.flg & mPROM];

    std::cout << " currmovenumber " << rootMoveCr + 1;
    std::cout << " hashfull ";

    UQ hsz = 1000*tt.size() / tt.max_size();
    std::cout << hsz << std::endl;
}

//-----------------------------
void PonderHit()
{
    double time1 = timer.getElapsedTimeInMicroSec();
    double timeSpent = time1 - time0;
//    std::cout << "( time spent = " << timeSpent << " )" << std::endl;
//    std::cout << "( time to think = " << timeToThink << " )" << std::endl;
    if(timeSpent >= 5*timeToThink)
        spentExactTime  = true;
    else
        ponder = false;
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

5r2/8/3k4/8/8/8/8/K6B b - - 98 139; fifty-move rule
2r1rbk1/p1Bq1ppp/Ppn1b3/1Npp4/B7/3P2Q1/1PP2PPP/R4RK1 w - - 0 1 bm Nxa7;

8/6R1/3p4/3P3p/p4K1k/2r5/1R6/8 w - - 0 1; experiments with singular reply
1k1q4/ppp1r3/8/6pp/3b4/P5B1/1PQ2PPP/2R3K1 w - - 0 1 am Rd1 @ply 7

r1b2rk1/2q1bppp/2np1n2/1p2p3/p2PP3/4BN1P/PPBN1PP1/R2QR1K1 b - - 0 1; SEE bug with pawns fixed
8/8/8/2PK4/4R2p/6k1/8/2r5 w - - 0 1 am Rc4
6k1/pp3ppp/1qr2n2/8/4P3/P1N3PP/1P3QK1/3R4 b - - 0 1 am Rc5 @nodes <= 40000 fixed
8/1p3r1p/p7/4Qp2/3Rp1kP/Pq6/1P3PP1/4K3 b - - 7 32 am Kxh4 @ply 7 lmr issue
8/5k2/4P2p/5P1p/4K1bP/4N3/3p4/2b5 b - - 0 57 PseudoLegal() bug f ixed after f7e8 e4e5 c1a3
rnbqk2r/pppp1ppp/5n2/4N3/2B1P3/8/PPPP1bPP/RNBQK2R w KQkq - 0 5 SEE bug
3r1q1r/2k2P1p/1p1p1Rp1/2p5/4Q3/1P4P1/P1K5/4R3 b - - 1 26 am Rd7 lmr issue
r1b1r1k1/ppb2pp1/7p/2p2P2/4P2q/8/PP1P1PBP/RQB2RK1 w - - 1 18 am h3 @ply 10
8/8/8/p7/P7/8/4K1k1/8 w - - 0 83 draw is inevitable
2rq1rk1/1p2b1pp/p2p4/2PP4/NP2p3/1Q1b1p2/P4PPP/R1B1R1K1 w - - 0 14 am g2g3 @ply 9 lmr sucks?
2r3k1/ppBR3B/5p2/3P2p1/6r1/2p2NPp/P4P2/6K1 b - - 1 24 bug with illegal move fixed
8/2pk2pB/1P1n2P1/6p1/4r3/3K4/8/1R6 b - - 2 40 ab c5 bug in Next() fixed
2k5/8/8/8/7p/8/6P1/5K2 w - - 0 1 bm Kg1; bug in PV @ply ~28 fixed
8/6p1/4k1Pp/6P1/5K2/8/8/8 b - - 0 49 search explosion
5Q1r/8/4k3/8/8/5RK1/8/8 b - - 0 56 search explosion
rn1qkbnr/1b3ppp/p3p3/1pppP3/5P2/2NB1N2/PPPP2PP/R1BQK2R w KQkq d6 0 7 am 0-0
8/5k2/3p4/1P1r1P2/8/4K1P1/7R/8 b - - 0 60 am Rxb5
8/6p1/8/6P1/K7/8/1kB5/8 w - - 0 1 bm Bb1
8/5kp1/1p1P3p/7P/P7/PK2r3/8/3R4 w - - 3 56 search explosion
8/3K4/BbP5/p5p1/8/5k2/8/8 w - - 4 67 am c7
*/

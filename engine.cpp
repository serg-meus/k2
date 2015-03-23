#include "engine.h"

//--------------------------------
char        stop_str[] = "";
unsigned    stop_ply   = 0;

Timer       timer;
double      time0;
double      time_spent, time_to_think;
unsigned    max_nodes_to_search, max_search_depth;
unsigned    root_ply, root_moves, root_move_cr;
bool        stop, infinite_analyze, busy;
Move        root_move_array[MAX_MOVES];
UQ          q_nodes, cut_cr, cut_num_cr[5], q_cut_cr, q_cut_num_cr[5];
UQ          null_probe_cr, null_cut_cr, hash_probe_cr;
UQ          hash_hit_cr, hash_cut_cr, hash_cutoff_by_best_move_cr;
UQ          total_nodes, futility_probes, futility_hits;
double      time_base, time_inc, time_remains, total_time_spent;
unsigned    moves_per_session;
int         finaly_made_moves, moves_remains;
bool        spent_exact_time;
unsigned    resign_cr, pv_stable_cr;
bool        time_command_sent;

transposition_table tt;
UQ  doneHashKeys[FIFTY_MOVES + max_ply];

//--------------------------------
short Search(int depth, short alpha, short beta, int lmr_parent)
{
    if(ply >= max_ply - 1 || DrawDetect())
    {
        pv[ply][0].flg = 0;
        return 0;
    }
    bool in_check = Attack(*king_coord[wtm], !wtm);
    if(in_check)
        depth += 1 + lmr_parent;

#ifndef DONT_USE_MATE_DISTANCE_PRUNING
    short mate_sc = (short)(K_VAL - ply);
    if(alpha >= mate_sc)
       return alpha;
    if(beta <= -mate_sc)
        return beta;
#endif  // DONT_USE_MATE_DISTANCE_PRUNING

#ifndef DONT_USE_FUTILITY
    if(depth <= 2 && depth >= 0 && !in_check
    && beta < mate_score
    && Futility(depth, beta))
        return beta;
#endif // DONT_USE_FUTILITY

    short x, _alpha = alpha;
    bool in_hash = false;
    tt_entry *entry = nullptr;
    if(depth > 0 && HashProbe(depth, &alpha, beta, &entry, &in_hash))
        return -alpha;

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
    if(beta - alpha == 1
    && NullMove(depth, beta, in_check, lmr_parent))
        return beta;

#endif // DONT_USE_NULL_MOVE

#ifndef DONT_USE_ONLY_MOVE_EXTENSION
    if(in_hash && entry->only_move && depth < 4)
        depth += 1;
#endif // DONT_USE_ONLY_MOVE_EXTENSION

    Move move_array[MAX_MOVES], m, m_best;
    unsigned move_cr = 0, legals = 0, max_moves = 999;
    bool beta_cutoff = false;

    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    for(; move_cr < max_moves && !stop; move_cr++)
    {
        m = Next(move_array, move_cr, &max_moves,
                 &in_hash, entry, wtm, false);
        if(max_moves <= 0)
            break;
        MkMove(m);
#ifndef NDEBUG
        if((!stop_ply || root_ply == stop_ply) && strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
        if(!Legal(m, in_check))
        {
            UnMove(m);
            continue;
        }

#ifndef DONT_USE_IID
    if(depth > 4 && legals == 0
    && m.scr < PV_FOLLOW)
    {
        UnMove(m);
        x = Search(depth/2, beta - 30 , beta + 30, 0);
        in_hash = true;
        m = Next(move_array, move_cr, &max_moves,
                 &in_hash, entry, wtm, false);
        MkMove(m);
        if(!Legal(m, in_check))
        {
            UnMove(m);
            continue;
        }
    }
#endif //DONT_USE_IID

        FastEval(m);

#ifndef DONT_USE_LMR
        int lmr = 1;
        if(depth < 3 || m.flg || in_check)
            lmr = 0;
        else if(legals < 4/* || m.scr >= 160*/)
            lmr = 0;
        else if((b[m.to] & ~white) == _p && TestPromo(COL(m.to), !wtm))
            lmr = 0;
//        else if(king_dist[ABSI(m.to - *king_coord[wtm])] < 2)
//            lmr = 0;
//        else if(depth > 4 && legals > 20)
//            lmr = 2;
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

    if(!legals && _alpha <= mate_score)
    {
        pv[ply][0].flg = 0;
        return in_check ? -K_VAL + ply : 0;
    }
    else if(legals)
    {
        StoreResultInHash(depth, _alpha, alpha, beta, legals,
                          beta_cutoff, (beta_cutoff ? m : move_array[0]));
#ifndef DONT_USE_ONLY_MOVE_EXTENSION
        if(legals == 1 && !in_hash)
        {
            bool om = DetectOnlyMove(beta_cutoff, in_check,
                                     move_cr, max_moves, move_array);
            if(om)
            {
                tt_entry *e;
                tt.count(hash_key, &e);
                e->only_move = true;
                tt.count(hash_key, &entry);
            }
        }
#endif // DONT_USE_ONLY_MOVE_EXTENSION
    }

    if(beta_cutoff)
    {
#ifndef DONT_SHOW_STATISTICS
        if(in_hash && legals == 1)
            hash_cutoff_by_best_move_cr++;
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
    q_nodes++;
    if((nodes & 511) == 511)
        CheckForInterrupt();

    pv[ply][0].flg = 0;

    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;

    short x = Eval(/*alpha, beta*/);

    if(-x >= beta)
        return beta;
    else if(-x > alpha)
        alpha = -x;

    Move move_array[MAX_MOVES];
    unsigned move_cr = 0, max_moves = 999, legals = 0;
    bool beta_cutoff = false;

    for(; move_cr < max_moves && !stop; move_cr++)
    {
        tt_entry *hs = nullptr;
        bool bm_not_hashed = false;
        Move m = Next(move_array, move_cr, &max_moves, &bm_not_hashed, hs, wtm, true);
        if(max_moves <= 0)
            break;

#ifndef DONT_USE_SEE_CUTOFF
        if(m.scr <= BAD_CAPTURES && SEE_main(m) < 0)
            continue;
#endif
#ifndef DONT_USE_DELTA_PRUNING
        if(material[0] + material[1] > 24
        && (wtm ? val_opn : -val_opn)
        + 100*pc_streng[b[m.to]/2] < alpha - 450)
            continue;
#endif
        MkMove(m);
        legals++;
#ifndef NDEBUG
        if((!stop_ply || root_ply == stop_ply) && strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
        if((b_state[prev_states + ply].capt & ~white) == _k)
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
        q_cut_cr++;
        assert(legals > 0);
        if(legals - 1 < (int)(sizeof(q_cut_num_cr)/sizeof(*q_cut_num_cr)))
            q_cut_num_cr[legals - 1]++;
#endif // DONT_SHOW_STATISTICS
        return beta;
    }
    return alpha;
}

//--------------------------------
void Perft(int depth)
{
    Move move_array[MAX_MOVES];
    bool in_check = Attack(*king_coord[wtm], !wtm);
    int max_moves = GenMoves(move_array, APPRICE_NONE, nullptr);
    for(int move_cr = 0; move_cr < max_moves; move_cr++)
    {
#ifndef NDEBUG
        if((unsigned)depth == max_search_depth)
            tmpCr = nodes;
#endif
        Move m = move_array[move_cr];
        MkMove(m);
#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
//        bool legal = !Attack(king_coord[!wtm], wtm);
        bool legal = Legal(m, in_check);
        if(depth > 1 && legal)
            Perft(depth - 1);
        if(depth == 1 && legal)
            nodes++;
#ifndef NDEBUG
        if((unsigned)depth == max_search_depth && legal)
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
        cut_cr++;
        if(move_cr < sizeof(cut_num_cr)/sizeof(*cut_num_cr))
            cut_num_cr[move_cr]++;
#else
    UNUSED(move_cr);
#endif // DONT_SHOW_STATISTICS
    if(m.flg)
        return;
    if(m != killers[ply][0] && m != killers[ply][1])
    {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = m;
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
    root_ply = 2;
    root_moves = 0;
    pv_stable_cr = 0;
    for(; root_ply <= max_ply && !stop; ++root_ply)
    {
        sc_ = sc;
        sc = RootSearch(root_ply, -INF, INF);
        if(stop && sc == -INF)
            sc = sc_;

        double time1 = timer.getElapsedTimeInMicroSec();
        time_spent = time1 - time0;

        if(!infinite_analyze && !pondering_in_process)
        {
            if(time_spent > time_to_think && !max_nodes_to_search)
                break;
            if(ABSI(sc) > mate_score && !stop)
                break;
            if(root_moves == 1 && root_ply >= 8)
                break;
        }
        if(root_ply >= max_search_depth)
            break;

    }// for(root_ply

    if(pondering_in_process && spent_exact_time)
    {
        pondering_in_process = false;
        spent_exact_time = false;
    }
    if(sc < -RESIGN_VALUE)
        resign_cr++;
    else
        resign_cr = 0;

    if((!stop && !infinite_analyze && sc < -mate_score)
    || (!infinite_analyze && sc < -RESIGN_VALUE && resign_cr > RESIGN_MOVES))
    {
        std::cout << "resign" << std::endl;
        busy = false;
    }

    total_nodes      += nodes;
    total_time_spent  += time_spent;

    timer.stop();

    if(!infinite_analyze || uci)
        PrintSearchResult();

    if(!xboard)
        infinite_analyze = false;
    busy = false;
}

//--------------------------------
short RootSearch(int depth, short alpha, short beta)
{
    bool in_check = Attack(*king_coord[wtm], !wtm);
    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;
    if(!root_moves)
        RootMoveGen(in_check);

    root_move_cr = 0;

    UQ prevDeltaNodes = 0;

    short x;
    Move  m;
    bool beta_cutoff = false;

    for(; root_move_cr < root_moves/* && alpha < mate_score*/ && !stop; root_move_cr++)
    {
        m = root_move_array[root_move_cr];

        MkMove(m);

#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0 && (!stop_ply || root_ply == stop_ply))
            ply = ply;
#endif // NDEBUG

        FastEval(m);
        UQ _nodes = nodes;

        if(uci && root_ply > 6)
            ShowCurrentUciInfo();

        if(DrawByRepetition())
        {
            x = 0;
            pv[1][0].flg = 0;
        }
        else
        {
#ifndef DONT_USE_PVS_IN_ROOT
            if(root_move_cr == 0 || pv_stable_cr < 2)
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

        if(x <= alpha && root_move_cr > 2 && dn > prevDeltaNodes && depth > 2)
        {
            Move tmp = root_move_array[root_move_cr];
            root_move_array[root_move_cr] =
                root_move_array[root_move_cr - 1];
            root_move_array[root_move_cr - 1] = tmp;
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
            if(root_move_cr > 0)
                pv_stable_cr = 0;
            alpha = x;
            StorePV(m);
            UnMove(m);

            if(depth > 3 && x != -INF
            && !(stop && root_move_cr < 2))
                PlyOutput(x);
            if(root_move_cr != 0)
            {
                Move tmp = root_move_array[root_move_cr];
                memmove(&root_move_array[1], &root_move_array[0],
                        sizeof(Move)*root_move_cr);
                root_move_array[0] = tmp;
            }
        }
        else
            UnMove(m);

    }// for(; root_move_cr < root_moves

    pv_stable_cr++;
    if(depth <= 3 && root_moves)
        PlyOutput(alpha);
    if(!root_moves)
    {
        pv[0][0].flg = 0;
        return in_check ? -K_VAL + ply : 0;
    }
    if(beta_cutoff)
    {
        UpdateStatistics(m, depth, root_move_cr);
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
    Move move_array[MAX_MOVES], m;
    unsigned max_moves = 999;
    b_state[prev_states + ply].val_opn = val_opn;
    b_state[prev_states + ply].val_end = val_end;
    for(unsigned move_cr = 0; move_cr < max_moves; move_cr++)
    {
        tt_entry *hs = nullptr;
        bool bm_not_hashed = false;
        m = Next(move_array, move_cr, &max_moves, &bm_not_hashed, hs, wtm, false);
    }
    for(unsigned move_cr = 0; move_cr < max_moves; move_cr++)
    {
        m = move_array[move_cr];
        MkMove(m);
        if(Legal(m, in_check))
            root_move_array[root_moves++] = m;
        UnMove(m);
    }

    pv[0][1] = root_move_array[0];
}

//--------------------------------
void InitSearch()
{
    InitTime();
    timer.start();
    time0 = timer.getElapsedTimeInMicroSec();

    nodes   = 0;
    q_nodes  = 0;
    cut_cr   = 0;
    q_cut_cr  = 0;
    futility_probes   = 0;
    null_cut_cr   = 0;
    null_probe_cr = 0;
    hash_cut_cr   = 0;
    hash_probe_cr = 0;
    hash_hit_cr   = 0;
    hash_cutoff_by_best_move_cr = 0;
    futility_hits     = 0;

    unsigned i, j;
    for(i = 0; i < sizeof(cut_num_cr)/sizeof(*cut_num_cr); i++)
        cut_num_cr[i] = 0;
    for(i = 0; i < sizeof(q_cut_num_cr)/sizeof(*q_cut_num_cr); i++)
        q_cut_num_cr[i] = 0;
    for(i = 0; i < max_ply; i ++)
        for(j = 0; j < max_ply + 1; j++)
        {
            pv[i][j].to = 0;
            pv[i][j].flg = 0;
            pv[i][j].scr = 0;
        }
    for(i = 0; i < max_ply; i++)
    {
        killers[i][0].flg =  0xFF;                                              //>> NB
        killers[i][1].flg =  0xFF;
    }

    stop = false;

    if(!uci && !xboard)
    {
    std::cout   << "( tRemain=" << time_remains/1e6
                << ", t2thnk=" << time_to_think/1e6
                << ", tExact = " << (spent_exact_time ? "true " : "false ")
                << " )" << std::endl;
    std::cout   << "Ply Value  Time    Nodes        Principal Variation" << std::endl;
    }

//    tt.clear();

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
        if(!infinite_analyze && pv[0][0].flg > 1)
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
    for(unsigned i = 0; i < sizeof(cut_num_cr)/sizeof(*cut_num_cr); i++)
        std::cout  << (int)(cut_num_cr[i]/(cut_cr/100 + 1)) << " ";
    std::cout << "]% )" << std::endl;

    std::cout << "( q_nodes = " << q_nodes
              << ", q_cuts = [";
    for(unsigned i = 0; i < sizeof(q_cut_num_cr)/sizeof(*q_cut_num_cr); i++)
        std::cout  << (int)(q_cut_num_cr[i]/(q_cut_cr/100 + 1)) << " ";
    std::cout << "]%, ";

    std::cout << "q/n = " << (int)(q_nodes/(nodes/100 + 1))
              << "% )" << std::endl;

    std::cout   << "( hash probes = " << hash_probe_cr
                << ", cutoffs by value = "
                << (int)(hash_cut_cr/(hash_probe_cr/100 + 1)) << "%, "
                << "cutoffs by best move = "
                << (int)(hash_cutoff_by_best_move_cr/(hash_hit_cr/100 + 1)) << "% )"
                << std::endl
                << "( hash full = " << (int)100*tt.size()/tt.max_size()
                << "% (" << tt.size()/sizeof(tt_entry)
                << "/" << tt.max_size()/sizeof(tt_entry)
                << " entries )" << std::endl;
#ifndef DONT_USE_NULL_MOVE
    std::cout   << "( null move probes = " << hash_probe_cr
                << ", cutoffs = "
                << (int)(null_cut_cr/(null_probe_cr/100 + 1)) << "% )"
                << std::endl;
#endif // DONT_USE_NULL_MOVE

#ifndef DONT_USE_FUTILITY
    std::cout << "( futility probes = " << futility_probes
              << ", hits = " << (int)(futility_hits/(futility_probes/100 + 1))
              << "% )" << std::endl;
#endif //DONT_USE_FUTILITY
    std::cout   << "( tSpent=" << time_spent/1e6
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
            cout << "info depth " << root_ply;

            if(ABSI(sc) < mate_score)
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
            cout << setfill(' ') << setw(4)  << left << root_ply;
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
    if(!time_command_sent)
        time_remains -= time_spent;
    time_remains = ABSI(time_remains);                                    //<< NB: strange
    int movsAfterControl;
    if(moves_per_session == 0)
        movsAfterControl = finaly_made_moves/2;
    else
        movsAfterControl = (finaly_made_moves/2) % moves_per_session;

    if(moves_per_session != 0)
        moves_remains = moves_per_session - (uci ? 0 : movsAfterControl);
    else
        moves_remains = 32;
    if(time_base == 0)
        moves_remains = 1;

    if(!spent_exact_time && (moves_remains <= 8
    || (time_inc == 0 && moves_per_session == 0
    && time_remains < time_base / 4)))
    {
        spent_exact_time = true;
    }

    if(movsAfterControl == 0 && finaly_made_moves/2 != 0)
    {
        if(!time_command_sent)
            time_remains += time_base;

        if(moves_remains > 5)
            spent_exact_time = false;
    }
    if(!time_command_sent)
        time_remains += time_inc;
    time_to_think = time_remains / moves_remains;
    if(moves_remains != 1 && !spent_exact_time)
        time_to_think /= 2;                           // average branching factor
    else if(time_inc != 0 && time_base != 0)
        time_to_think = time_inc;

    time_command_sent = false;
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
    root_moves = 0;
    RootMoveGen(in_check);

    char rMov[6];
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    for(unsigned i = 0; i < root_moves; ++i)
    {
        Move m  = root_move_array[i];
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

            MkMove(m);

            FastEval(m);

            short _valOpn_ = val_opn;
            short _valEnd_ = val_end;

            memmove(&b_state[0], &b_state[1],
                    (prev_states + 2)*sizeof(BrdState));
            ply--;
            EvalAllMaterialAndPST();
            if(val_opn != _valOpn_ || val_end != _valEnd_)
            {
                std::cout << "telluser err02: wrong score. Fast: "
                        << _valOpn_ << '/' << _valEnd_
                        << ", all: " << val_opn << '/' << val_end
                        << std::endl << "resign"
                        << std::endl;
            }
            for(int i = 0; i < FIFTY_MOVES; ++i)
                doneHashKeys[i] = doneHashKeys[i + 1];

            finaly_made_moves++;
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
    total_nodes      = 0;
    total_time_spent  = 0;

    spent_exact_time  = false;
    finaly_made_moves = 0;
    resign_cr        = 0;
    time_spent       = 0;

    tt.clear();
}

//--------------------------------
bool FenStringToEngine(char *fen)
{
    bool ans = FenToBoard(fen);

    if(!ans)
        return false;

    EvalAllMaterialAndPST();
    time_spent   = 0;
    time_remains = time_base;
    finaly_made_moves = 0;
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

    if(reversible_moves == FIFTY_MOVES)
        return true;

    return DrawByRepetition();
}

//--------------------------------
void CheckForInterrupt()
{
    if(max_nodes_to_search != 0)
    {
        if(nodes >= max_nodes_to_search - 512)
            stop = true;
    }
    if(infinite_analyze || pondering_in_process)
        return;
    if(spent_exact_time)
    {
        double time1 = timer.getElapsedTimeInMicroSec();
        time_spent = time1 - time0;
        if(time_spent >= time_to_think - 50000)
            stop = true;
    }
    else if((nodes & 8191) == 8191)
    {
        double time1 = timer.getElapsedTimeInMicroSec();
        time_spent = time1 - time0;
        if(time_spent >= 10*time_to_think - 50000)
            stop = true;
    }

}

//--------------------------------
void MkMove(Move m)
{
    bool specialMove = false;
    ply++;
    b_state[prev_states + ply].cstl   = b_state[prev_states + ply - 1].cstl;
    b_state[prev_states + ply].capt  = b[m.to];
    auto it = coords[wtm].begin();
    it      = m.pc;
    UC fr   = *it;
    UC targ = m.to;
    b_state[prev_states + ply].fr = fr;
    b_state[prev_states + ply].to = targ;
    b_state[prev_states + ply].reversibleCr = reversible_moves;
    reversible_moves++;
    if(m.flg & mCAPT)
    {
        if (m.flg & mENPS)
        {
            targ += wtm ? -16 : 16;
            material[!wtm]--;
        }
        else
            material[!wtm] -=
                    pc_streng[b_state[prev_states + ply].capt/2 - 1];

        auto it_cap = coords[!wtm].begin();
        auto it_end = coords[!wtm].end();
        for(; it_cap != it_end; ++it_cap)
            if(*it_cap == targ)
                break;
        assert(it_cap != it_end);
        b_state[prev_states + ply].captured_it = it_cap;
        coords[!wtm].erase(it_cap);

        pieces[!wtm]--;
        reversible_moves = 0;
    }// if capture

    if((b[fr] <= _R) || (m.flg & mCAPT))        // trick: fast exit if not K|Q|R moves, and no captures
        specialMove |= MakeCastle(m, fr);

    b_state[prev_states + ply].ep = 0;
    if((b[fr] ^ wtm) == _p)
    {
        specialMove     |= MakeEP(m, fr);
        reversible_moves = 0;
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
        b_state[prev_states + ply].nprom = ++it;
        --it;
        coords[wtm].move_element(king_coord[wtm], it);
        reversible_moves = 0;
    }
    *it   = m.to;

    doneHashKeys[FIFTY_MOVES + ply - 1] = hash_key;
    MoveHashKey(m, fr, specialMove);
#ifndef NDEBUG
        UQ tmp_key = InitHashKey() ^ -1ULL;
        if(tmp_key != hash_key)
            ply = ply;
        assert(tmp_key == hash_key);
#endif //NDEBUG

#ifndef DONT_USE_PAWN_STRUCT
    MovePawnStruct(b[m.to], fr, m);
#endif // DONT_USE_PAWN_STRUCT
    wtm ^= white;
}

//--------------------------------
void UnMove(Move m)
{
    UC fr = b_state[prev_states + ply].fr;
    auto it = coords[!wtm].begin();
    it = m.pc;
    *it = fr;
    b[fr] = (m.flg & mPROM) ? _P ^ wtm : b[m.to];
    b[m.to] = b_state[prev_states + ply].capt;

    reversible_moves = b_state[prev_states + ply].reversibleCr;

    hash_key = doneHashKeys[FIFTY_MOVES + ply - 1];

    wtm ^= white;

    if(m.flg & mCAPT)
    {
        auto it_cap = coords[!wtm].begin();
        it_cap = b_state[prev_states + ply].captured_it;

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
                += pc_streng[b_state[prev_states + ply].capt/2 - 1];

        coords[!wtm].restore(it_cap);
        pieces[!wtm]++;
    }// if capture

    int prIx = m.flg & mPROM;
    UC prPc[] = {0, _q, _n, _r, _b};
    if(prIx)
    {
        auto it_prom = coords[wtm].begin();
        it_prom = b_state[prev_states + ply].nprom;
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
    val_opn = b_state[prev_states + ply].val_opn;
    val_end = b_state[prev_states + ply].val_end;

#ifndef NDEBUG
    cur_moves[5*ply] = '\0';
#endif // NDEBUG
}

//--------------------------------------
void MakeNullMove()
{
#ifndef NDEBUG
    strcpy(&cur_moves[5*ply], "NULL ");
    if((!stop_ply || root_ply == stop_ply) && strcmp(stop_str, cv) == 0)
        ply = ply;
#endif // NDEBUG

    b_state[prev_states + ply + 1] = b_state[prev_states + ply];
    b_state[prev_states + ply].to = MOVE_IS_NULL;
    b_state[prev_states + ply  + 1].ep = 0;

    ply++;

    hash_key ^= -1ULL;
    doneHashKeys[FIFTY_MOVES + ply - 1] = hash_key;

    wtm ^= white;
}

//--------------------------------------
void UnMakeNullMove()
{
    wtm ^= white;
    cur_moves[5*ply] = '\0';
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
    null_probe_cr++;
#endif // DONT_SHOW_STATISTICS

    if(b_state[prev_states + ply - 1].to == MOVE_IS_NULL
//    && b_state[prev_states + ply - 2].to == MOVE_IS_NULL
      )
        return false;

    UC store_ep     = b_state[prev_states + ply].ep;
    UC store_to     = b_state[prev_states + ply].to;
    US store_rv     = reversible_moves;
    reversible_moves = 0;

    MakeNullMove();
    if(store_ep)
        hash_key = InitHashKey();

    int r = depth > 6 ? 3 : 2;
    if(lmr_ > 0 && depth - r - 1 <= 0)
        r--;
    short x = -Search(depth - r - 1, -beta, -beta + 1, 0);

    UnMakeNullMove();
    b_state[prev_states + ply].to    = store_to;
    b_state[prev_states + ply].ep    = store_ep;
    reversible_moves                     = store_rv;

    if(store_ep)
        hash_key = InitHashKey();

#ifndef DONT_SHOW_STATISTICS
    if(x >= beta)
        null_cut_cr++;
#endif // DONT_SHOW_STATISTICS

    return (x >= beta);
}

//-----------------------------
bool Futility(int depth, short beta)
{
    if(b_state[prev_states + ply].capt == 0
    && b_state[prev_states + ply - 1].to   != 0xFF
    )
    {
#ifndef DONT_SHOW_STATISTICS
            futility_probes++;
#endif // DONT_SHOW_STATISTICS
//        short margin = depth == 0 ? 350 : 550;
        short margin = depth < 2 ? 110 : 310;
        margin += beta;
        if((wtm  &&  val_opn > margin &&  val_end > margin)
        || (!wtm && -val_opn > margin && -val_end > margin))
        {
#ifndef DONT_SHOW_STATISTICS
            futility_hits++;
#endif // DONT_SHOW_STATISTICS
            return true;
        }
    }
    return false;
}

//--------------------------------
bool DrawByRepetition()
{
    if(reversible_moves < 4)
        return false;

    unsigned max_count;
    if(reversible_moves > ply + finaly_made_moves)
        max_count = ply + finaly_made_moves;
    else
        max_count = reversible_moves;

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

    UC cstl = b_state[prev_states + 0].cstl;
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

    std::cout << " " << reversible_moves;
    std::cout << " " << finaly_made_moves/2 + 1;

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
bool HashProbe(int depth, short *alpha, short beta,
               tt_entry **entry,
               bool *in_hash)
{
    if(tt.count(hash_key, entry) == 0 || stop)
        return false;

#ifndef DONT_SHOW_STATISTICS
    hash_probe_cr++;
#endif //DONT_SHOW_STATISTICS
    UC hbnd = (*entry)->bound_type;
#ifndef DONT_USE_ONLY_MOVE_EXTENTION
    if((*entry)->depth >= depth)
#else
    if((*entry)->depth >= depth + (*entry)->only_move)
#endif // DONT_USE_ONLY_MOVE_EXTENTION
    {
        short hval = (*entry)->value;
        if(hval > mate_score && hval != INF)
            hval += (*entry)->depth - ply;
        else if(hval < -mate_score && hval != -INF)
            hval -= (*entry)->depth - ply;

        if( hbnd == hEXACT
        || (hbnd == hUPPER && hval >= -*alpha)                      // -alpha is beta for parent node
        || (hbnd == hLOWER && hval <= -beta) )                      // -beta is alpha for parent node
        {
#ifndef DONT_SHOW_STATISTICS
            hash_cut_cr++;
#endif //DONT_SHOW_STATISTICS
            pv[ply][0].flg = 0;
            *alpha = hval;
            return true;
        }// if(bnd
    }// if((*entry).depth >= depth
#ifndef DONT_SHOW_STATISTICS
    hash_hit_cr++;
#endif //DONT_SHOW_STATISTICS
    *in_hash = true;
    return false;
}

//-----------------------------
bool PseudoLegal(Move &m, bool stm)
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
    if((b[fr]/2) != _p/2 && (m.flg & mPROM))
        return false;
    bool long_move;
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
            if((m.flg & mENPS)
            && (pt != __ || b_state[prev_states + ply].ep == 0))
                return false;
            if(ROW(m.to) == (stm ? 7 : 0)
            && !(m.flg & mPROM))
                return false;
            if(!(m.flg & mCAPT))
            {
                if(!stm)
                    dROW = -dROW;
                if(pt != __ || dCOL != 0
                || (stm && dROW <= 0))
                    return false;
                long_move = (ROW(fr) == (stm ? 1 : 6));
                if(long_move ? dROW > 2 : dROW != 1)
                    return false;
                if(long_move && dROW == 2
                && b[XY2SQ(COL(fr), (ROW(fr) + ROW(m.to))/2)] != __)
                    return false;
            }

            break;
        case _n/2 :
            m.flg &= mCAPT;
            if(ABSI(dCOL) + ABSI(dROW) != 3)
                return false;
            if(ABSI(dCOL) != 1 && ABSI(dROW) != 1)
                return false;
            break;
        case _b/2 :
        case _r/2 :
        case _q/2 :
            m.flg &= mCAPT;
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
            if(COL(fr) != 4 || ROW(fr) != stm ? 0 : 7)
                return false;
            if((b_state[prev_states + ply].cstl &
            (m.flg >> 3 >> (2*stm))) == 0)
                return false;
            if(b[XY2SQ(COL(m.to), ROW(fr))] != __)
                return false;
            if(b[XY2SQ((COL(m.to)+COL(fr))/2, ROW(fr))] != __)
                return false;
            if((m.flg &mCS_K)
            && (b[XY2SQ(7, ROW(fr))] & ~white) != _r)
                return false;
            if((m.flg &mCS_Q)
            && (b[XY2SQ(0, ROW(fr))] & ~white) != _r)
                return false;
            break;
    }
    return true;
}

//--------------------------------
Move Next(Move *move_array, unsigned cur, unsigned *max_moves,
          bool *in_hash, tt_entry *entry,
          UC stm, bool only_captures)
{
    Move ans;
    if(cur == 0)
    {
        if(!*in_hash)
        {
            if(!only_captures)
                *max_moves = GenMoves(move_array, APPRICE_ALL, nullptr);
            else
            {
                *max_moves = GenCaptures(move_array);
                if(*max_moves > 1 && move_array[0].scr == move_array[1].scr)
                    if(SEE_main(move_array[0]) < SEE_main(move_array[1]))
                        std::swap(move_array[0], move_array[1]);
            }
        }
        else
        {
            ans = entry->best_move;

            bool pseudo_legal = PseudoLegal(ans, stm);
#ifndef NDEBUG
            int mx_ = GenMoves(move_array, APPRICE_NONE, nullptr);
            int i = 0;
            for(; i < mx_; ++i)
                if(move_array[i] == ans)
                    break;
            bool tt_move_found = i < mx_;
            if(tt_move_found != pseudo_legal)
                PseudoLegal(ans, stm);
            assert(tt_move_found == pseudo_legal);
#endif
            if(pseudo_legal)
            {
                ans.scr = PV_FOLLOW;
                return ans;
            }
            else
            {
                *in_hash = false;
                *max_moves = GenMoves(move_array, APPRICE_ALL, nullptr);
            }
        }// else (if *in_hash)
    }// if cur == 0
    else if(cur == 1 && *in_hash)
    {
        *max_moves = GenMoves(move_array, APPRICE_ALL, &entry->best_move);
        for(unsigned i = cur; i < *max_moves; i++)
            if(move_array[i].scr == PV_FOLLOW && i != 0)
            {
                ans = move_array[0];
                move_array[0] = move_array[i];
                move_array[i] = ans;
                break;
            }
    }

    if(only_captures)                                                   // already sorted
        return move_array[cur];

    int max = -32000;
    unsigned imx = cur;

    for(unsigned i = cur; i < *max_moves; i++)
    {
        UC sc = move_array[i].scr;
        if(sc > max)
        {
            max = sc;
            imx = i;
        }
    }
    ans = move_array[imx];
    if(imx != cur)
    {
        move_array[imx] = move_array[cur];
        move_array[cur] = ans;
    }
    return ans;
}

//-----------------------------
void StoreResultInHash(int depth, short _alpha, short alpha,            // save results of search to hash table
                       short beta, unsigned legals,                     // note that this result is for 'parent' node (negative score, alpha = -beta, etc)
                       bool beta_cutoff, Move best_move)
{
    if(beta_cutoff)
    {
        if(beta > mate_score && beta != INF)
            beta += ply - depth - 1;
        else if(beta < -mate_score && beta != -INF)
            beta -= ply - depth - 1;
        tt.add(hash_key, -beta, best_move, depth, hLOWER);

    }
    else if(alpha > _alpha && pv[ply][0].flg > 0)
    {
        if(alpha > mate_score && alpha != INF)
            alpha += ply - depth;
        else if(alpha < - mate_score && alpha != -INF)
            alpha -= ply - depth;
        tt.add(hash_key, -alpha, pv[ply][1], depth, hEXACT);
    }
    else if(alpha <= _alpha)
    {
        if(_alpha > mate_score && _alpha != INF)
            _alpha += ply - depth;
        else if(_alpha < -mate_score && _alpha != -INF)
            _alpha -= ply - depth;
        Move no_move;
        no_move.flg = 0xFF;
        tt.add(hash_key, -_alpha, legals > 0 ? best_move : no_move, depth, hUPPER);
    }
}

//-----------------------------
bool DetectOnlyMove(bool beta_cutoff, bool in_check,
                    unsigned move_cr, unsigned max_moves,
                    Move *move_array)
{
    if(!beta_cutoff)
        return true;

    unsigned cr = move_cr;
    while(++cr < max_moves)
    {
        bool nh = false;
        tt_entry hs;
        hs.best_move.flg = 0xFF;
        Move tmp = Next(move_array, cr, &max_moves,
                 &nh, &hs, wtm, false);
        MkMove(tmp);
        if(Legal(tmp, in_check))
        {
            UnMove(tmp);
            break;
        }
        UnMove(tmp);
    }// while(++cr < max_moves
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

    Move m = root_move_array[root_move_cr];
    UC fr = b_state[prev_states + 1].fr;
    std::cout << " currmove "
        << (char)(COL(fr) + 'a') << (char)(ROW(fr) + '1')
        << (char)(COL(m.to) + 'a') << (char)(ROW(m.to) + '1');
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    if(m.flg & mPROM)
        std::cout << proms[m.flg & mPROM];

    std::cout << " currmovenumber " << root_move_cr + 1;
    std::cout << " hashfull ";

    UQ hsz = 1000*tt.size() / tt.max_size();
    std::cout << hsz << std::endl;
}

//-----------------------------
void PonderHit()
{
    double time1 = timer.getElapsedTimeInMicroSec();
    double time_spent = time1 - time0;
    if(time_spent >= 5*time_to_think)
        spent_exact_time  = true;
    else
        pondering_in_process = false;
}

/*
r4rk1/ppqn1pp1/4pn1p/2b2N1P/8/5N2/PPPBQPP1/2KRR3 w - - 0 18 Nxh6?! speed test position
2k1r2r/1pp3pp/p2b4/2p1n2q/6b1/1NQ1B3/PPP2PPP/R3RNK1 b - - bm Nf3+; speed test position
r4rk1/1b3p1p/pq2pQp1/n5N1/P7/2RBP3/5PPP/3R2K1 w - - 0 1 bm Nxh7;   speed test position
1B6/3k4/p7/1p4b1/1P1PK1p1/P7/8/8 w - - 2 59 am Bf4;                speed test position

3r4/3P1p2/p4Pk1/4r1p1/1p4Kp/3R4/8/8 b - - 0 1 bm e7 ?
4n3/3N4/4P3/3P2p1/p1K5/7k/8/8 w - - 0 58 wrong connected promos eval
8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1 bm Rxb2; id "WAC.002";
r1b2rk1/ppq3pp/2nb1n2/3pp3/3P3B/3B1N2/PP2NPPP/R2Q1RK1 w - - 0 1 low value of first move cuts in QS
8/6k1/8/6pn/pQ6/Pp3P1K/1P3P1P/6r1 w - - 2 31 draw due to perpetual check
2kb1R2/8/2p2B2/1pP2P2/1P6/1K6/8/3r4 w - - 0 1 bm Kc3 Be7
r1r3k1/1pQ3p1/1R5p/3qNp2/p2P4/4B3/5PPP/6K1 w - - 2 28 bm Qe7 draw
r7/2P5/8/8/1K6/4k2B/8/8 w - - 13 76 need to shorten game by fast draw
3r4/p4p1p/2P2k2/1P3p2/P6p/8/2R5/6K1 w - - 0 44  pawn eval
8/8/8/R7/4bk2/7K/8/6r1 w - - 98 180 bm Ra2; fifty move rule bug with mate at last ply fixed
8/1b1pk1P1/3p1p1P/3P1K2/P1P5/8/4B3/8 w - - 3 54 bm g8R
8/4p1k1/3pP1p1/3P2P1/5P2/8/r1BK4/8 b - - 68 84; eval: white's better?
2r1rbk1/p1Bq1ppp/Ppn1b3/1Npp4/B7/3P2Q1/1PP2PPP/R4RK1 w - - 0 1 bm Nxa7;
8/5k2/3p4/1P1r1P2/8/4K1P1/7R/8 b - - 0 60 am Rxb5
8/6p1/8/6P1/K7/8/1kB5/8 w - - 0 1 bm Bb1
8/7p/8/7P/1p6/1p5P/1P2Q1pk/1K6 w - - 0 1; search explosion
8/8/8/3r4/2K2p2/5N1P/5k2/8 w - - 0 58 search explosion @ply 17

8/1b1nkp1p/4pq2/1B6/PP1p1pQ1/2r2N2/5PPP/4R1K1 w - - 0 1 am Nxd4
3r1rk1/q4pp1/n1bNp2p/p7/pn2P1N1/6P1/1P1Q1PBP/2RR2K1 w - - 4 1 bm Nxh6
4r2k/p2qr1pp/1pp2p2/2p1nP1N/4R3/1P1P2RP/1PP2QP1/7K w - - 0 1 bm Rxg7
r1bqnrk1/p3np2/2pp3p/4P1p1/1PB5/2N2N2/P2Q1PPP/R3R1K1 w - g6 0 1 bm Nxg5
8/p2B1pk1/7p/P3p1p1/3p4/4nPP1/1Pr1PK1P/1R6 b - - 5 1 am Rxb2
7k/p3Rb1p/P1p2rp1/2Pr1p2/5P1P/5PK1/8/4RB2 b - - 10 1 am Rxc5
r3k2r/1b1p1pp1/p2qp3/2bP4/Pp3B2/6P1/1PP1QPB1/R4RK1 b kq - 0 1 bm Qxd5
r5k1/pQp2qpp/8/4pbN1/3P4/6P1/PPr4P/1K1R3R b - - 0 1 bm Rc1+
rn3rk1/pbppq1pp/1p2pb2/4N2Q/3PN3/3B4/PPP2PPP/R3K2R w KQ - 0 1 bm Qxh7
r4bkr/ppp3pp/4NP2/3qn3/2pp2P1/8/PPP2P1P/R1BQR1K1 w - - 0 1 f7
1k1r2nr/ppp1q1p1/3pbp1p/4n3/Q3P2B/2P2N2/P3BPPP/1R3RK1 w - - 0 1 Rxb7
8/8/1p6/p7/P1N1P1pk/1Pn1K3/8/8 w - - 0 1 am Nxb6
8/1pPK3b/8/8/8/5k2/8/8 w - - 0 1 bm Kc8
8/k7/2K5/1P6/8/8/8/8 w - - 0 1 bm Kc7
8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1 bm Kb1
8/1p6/1P6/8/7K/8/8/1k6 w - - 0 1 bm Kg3
2k5/8/8/8/7p/8/6P1/5K2 w - - 0 1 bm Kg1
3b3k/3P4/p7/4P2p/6p1/4NbP1/5P1P/6K1 w - - 3 1 rotten position
4r1k1/p1qr1p2/2pb1Bp1/1p3Q1p/3P1n1R/1B3P2/PP3PK1/7R w - - 0 1 bm Qxf4
r1bqkb1r/pp2pppp/2n2n2/1N1p4/3P1B2/8/PPP2PPP/R2QKBNR b KQkq - 0 1
r4r1k/2Rq2pp/p5b1/1p1pB1b1/3Pn1P1/1B5P/PP1N4/3Q1RK1 b - - 0 1 bm Qxc7
2b1r1k1/r1p1nppp/1p6/p3P3/Nn3P2/P4B2/1P3BPP/2RR2K1 b - - 0 1 am Na2

6rk/3R2p1/p1q4p/5Q1R/2P5/1Pr5/P4K1P/8 w - - 6 27 KS eval - white is worse
3r2k1/pq3ppp/R4n2/4N1Q1/1P3P2/P5PP/8/3r1RK1 w - - 3 24  KS eval - white is worse
r3r3/6b1/7p/2pkpPp1/P3R1P1/2B5/7P/4R1K1 w - - 0 20 KS eval - wrong advantage for white
4q1k1/2p2r1p/4NnpQ/3nB3/b2P2P1/5B1P/2p2P2/R5K1 w - - 1 31 KS eval - wrong advantage for white
4r1k1/1p3p2/pQ1p2pp/2p5/2P1q1b1/2P1rN2/P3BRPP/4R1K1 w - - 2 18 KS eval - wrong advantage for black
6k1/5ppp/r3p3/2bpP1P1/5K2/2P1B2P/p3bP2/RR6 b - - 0 33 KS eval - wrong advantage for black
8/p3b2p/4k3/1pp1P1p1/4b3/1PB1NrPP/P3KP2/2R5 b - - 1 22 KS eval - wrong advantage for black
8/2p5/3p4/5K2/3k4/2n5/3N2PP/8 b - - 2 46 king end tropism eval - white king's better
2k1r2r/p4q1P/2p5/5p2/1bb4Q/1N2pP2/PPP1N3/1K1R3R w - - 5 19 KS eval - black is worse
3r1rk1/1p3ppp/pnq5/4p1P1/6Q1/1BP1R3/PP3PP1/2K4R b - - 0 17 KS eval - white is better
1r1q1rk1/1b1n1ppp/p1pQp3/3p4/4P3/2N2B2/PPP2PPP/R3R1K1 w - - 3 8 am e5

r3kb1r/1b1n1p1p/pq2Np2/1p2p2Q/5P2/2N5/PPP3PP/2KR1B1R w kq - 0 1 bm Rxd7 low value of first move cuts in QS
r4rk1/pb3ppp/1p3q2/1Nbp4/2P1nP2/P4BP1/1PQ4P/R1B2R1K b - - 0 1 Qg6 is the best?
8/p5p1/p3k3/6PP/3NK3/8/Pb6/8 b - - 2 48 am Bxd4
*/

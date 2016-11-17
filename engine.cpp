#include "engine.h"





//--------------------------------
char        stop_str[] = "";
unsigned    stop_ply   = 0;

Timer       timer;
double      time0;
double      time_spent, time_to_think;
unsigned    max_nodes_to_search, max_search_depth;
unsigned    root_ply, max_root_moves, root_move_cr;
bool        stop, infinite_analyze, busy;
UQ          q_nodes, cut_cr, cut_num_cr[5], q_cut_cr, q_cut_num_cr[5];
UQ          null_probe_cr, null_cut_cr, hash_probe_cr;
UQ          hash_hit_cr, hash_cut_cr;
UQ          hash_best_move_cr, hash_cutoff_by_best_move_cr;
UQ          total_nodes, futility_probes, futility_hits;
UQ          killer1_probes, killer1_hits, killer2_probes, killer2_hits;
double      time_base, time_inc, time_remains, total_time_spent;
unsigned    moves_per_session;
int         finaly_made_moves, moves_remains;
bool        spent_exact_time;
unsigned    resign_cr;
bool        time_command_sent;


std::vector<std::pair<UQ, Move> > root_moves;

transposition_table tt;





//--------------------------------
short Search(int depth, short alpha, short beta,
             signed char node_type)
{
    if(ply >= max_ply - 1 || DrawDetect())
    {
        pv[ply][0].flg = 0;
        return 0;
    }
    bool in_check = Attack(*king_coord[wtm], !wtm);

    if(depth < 0)
        depth = 0;
    if(in_check)
        depth++;

#ifndef DONT_USE_RECAPTURE_EXTENSION
    if(!in_check
    && b_state[prev_states + ply - 1].capt
    && b_state[prev_states + ply].capt
    && b_state[prev_states + ply].to == b_state[prev_states + ply - 1].to
    && b_state[prev_states + ply - 1].scr > BAD_CAPTURES
    && b_state[prev_states + ply].scr > BAD_CAPTURES
    )
        depth++;
#endif // DONT_USE_RECAPTURE_EXTENSION

#ifndef DONT_USE_MATE_DISTANCE_PRUNING
    short mate_sc = (short)(K_VAL - ply);
    if(alpha >= mate_sc)
       return alpha;
    if(beta <= -mate_sc)
        return beta;
#endif // DONT_USE_MATE_DISTANCE_PRUNING

#ifndef DONT_USE_FUTILITY
    if(depth <= 2 && !in_check
//    && alpha < mate_score && beta >= -mate_score
    && beta < mate_score
    && Futility(depth, beta))
        return beta;
#endif // DONT_USE_FUTILITY

#ifndef DONT_USE_NULL_MOVE
    if(NullMove(depth, beta, in_check))
        return beta;
#endif // DONT_USE_NULL_MOVE

    short x, initial_alpha = alpha;
    tt_entry *entry;
    if(depth > 0 && (entry = HashProbe(depth, &alpha, beta)) != nullptr
       && alpha != initial_alpha)
        return -alpha;

    if(depth <= 0)
        return QSearch(alpha, beta);

    nodes++;
    if((nodes & nodes_to_check_stop) == nodes_to_check_stop)
        CheckForInterrupt();

    Move move_array[MAX_MOVES], cur_move;
    unsigned move_cr = 0, legal_moves = 0, max_moves = 999, first_legal = 0;
    bool beta_cutoff = false;

    for(; move_cr < max_moves; move_cr++)
    {
        cur_move = Next(move_array, move_cr, &max_moves,
                 entry, wtm, all_moves, in_check, cur_move);
        if(max_moves <= 0)
            break;

#ifndef DONT_USE_ONE_REPLY_EXTENSION
        if(in_check && max_moves == 1)
//        && (depth >= 3 || b_state[prev_states + ply - 1].scr > BAD_CAPTURES))
            depth++;
#endif //DONT_USE_ONE_REPLY_EXTENSION

        bool is_special_move = MkMoveFast(cur_move);
#ifndef NDEBUG
        if((!stop_ply || root_ply == stop_ply) && strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG

#ifndef DONT_USE_ONE_REPLY_EXTENSION
        if(!in_check && !Legal(cur_move, in_check))
#else
        if(!Legal(cur_move, in_check))
#endif // DONT_USE_ONE_REPLY_EXTENSION
        {
            UnMoveFast(cur_move);
            continue;
        }

#ifndef DONT_USE_LMP
        if(depth <= 2 && !cur_move.flg && !in_check
        && node_type == all_node && legal_moves > 4)
        {
            UnMoveFast(cur_move);
            break;
        }
#endif // DONT_USE_LMP

        MkMoveIncrementally(cur_move, is_special_move);

#ifndef DONT_USE_IID
    if(node_type != all_node
    && depth >= 6 && legal_moves == 0                                         // first move and
    && cur_move.scr < PV_FOLLOW)                                               // no move from hash table
    {
        UnMove(cur_move);
        short iid_low_bound  = alpha <= -mate_score ? alpha : alpha - 30;
        short iid_high_bound = beta  >=  mate_score ? beta  : beta  + 30;
        x = Search(node_type == pv_node ? depth - 2 : depth/2,
                   iid_low_bound, iid_high_bound, node_type);
//        if(x <= iid_low_bound || x >= iid_high_bound)
//            return x;
        HashProbe(depth, &alpha, beta);
        cur_move = Next(move_array, 0, &max_moves,
                 &in_hash, entry, wtm, all_moves);
        MkMove(cur_move);
        if(!Legal(cur_move, in_check))
        {
            UnMove(cur_move);
            continue;
        }
    }
#endif // DONT_USE_IID

        FastEval(cur_move);

#ifndef DONT_USE_LMR
        int lmr = 1;
        if(depth < 3 || cur_move.flg || in_check)
            lmr = 0;
        else if(legal_moves < 4)
            lmr = 0;
        else if(TO_BLACK(b[cur_move.to]) == _p
        && IsPasser(COL(cur_move.to), !wtm))
            lmr = 0;
        else if(depth <= 4 && legal_moves > 8)
            lmr = 2;
#else
        int lmr = 0;
#endif  // DONT_USE_LMR

        if(legal_moves == 0)
            x = -Search(depth - 1, -beta, -alpha, -node_type);
        else if(beta > alpha + 1)
        {
            x = -Search(depth - 1 - lmr, -alpha - 1, -alpha, cut_node);
            if(x > alpha/* && x < beta*/)
                x = -Search(depth - 1, -beta, -alpha, pv_node);
        }
        else
        {
            x = -Search(depth - 1 - lmr, -beta, -alpha, -node_type);
            if(lmr && x > alpha)
                x = -Search(depth - 1, -beta, -alpha, pv_node);
        }
        UnMove(cur_move);

        if(legal_moves == 0)
            first_legal = move_cr;
        legal_moves++;

        if(x >= beta)
        {
            beta_cutoff = true;
            break;
        }
        else if(x > alpha)
        {
            alpha = x;
            StorePV(cur_move);
        }
        if(stop)
            break;
    }// for move_cr

    if(legal_moves == 0 && initial_alpha <= mate_score)
    {
        pv[ply][0].flg = 0;
        return in_check ? -K_VAL + ply : 0;
    }
    else if(legal_moves)
        StoreResultInHash(depth, initial_alpha, alpha, beta, legal_moves,
                          beta_cutoff,
                          (beta_cutoff ? cur_move : move_array[first_legal]),
                          in_check && max_moves == 1);
    if(beta_cutoff)
    {
#ifndef DONT_SHOW_STATISTICS
        if(entry != nullptr && entry->best_move.flg != 0xFF)
            hash_best_move_cr++;
        if(legal_moves == 1)
        {
            if(entry != nullptr && entry->best_move.flg != 0xFF)
                hash_cutoff_by_best_move_cr++;
            else if(cur_move.scr == FIRST_KILLER)
                killer1_hits++;
            else if(cur_move.scr == SECOND_KILLER)
                killer2_hits++;
        }
#endif // DONT_SHOW_STATISTICS
        UpdateStatistics(cur_move, depth, legal_moves - 1);
        return beta;
    }
    return alpha;
}





//-----------------------------
short QSearch(short alpha, short beta)
{
    if(ply >= max_ply - 1)
        return 0;

    pv[ply][0].flg = 0;

    if(material[0] + material[1] > 24
    && ReturnEval(wtm) > beta + 250)
        return beta;

    short x = Eval();

    if(-x >= beta)
        return beta;
    else if(-x > alpha)
        alpha = -x;

    nodes++;
    q_nodes++;
    if((nodes & nodes_to_check_stop) == nodes_to_check_stop)
        CheckForInterrupt();

    Move move_array[MAX_MOVES];
    unsigned move_cr = 0, max_moves = 999, legal_moves = 0;
    bool beta_cutoff = false;

    for(; move_cr < max_moves; move_cr++)
    {
        Move cur_move = Next(move_array, move_cr, &max_moves,
                      nullptr, wtm, captures_only, false, cur_move);
        if(max_moves <= 0)
            break;

#ifndef DONT_USE_SEE_CUTOFF
        if(cur_move.scr <= BAD_CAPTURES)
            break;
#endif

#ifndef DONT_USE_DELTA_PRUNING
        if(material[white] + material[black]
                - pieces[white] - pieces[black] > 24
        && TO_BLACK(b[cur_move.to]) != _k
        && !(cur_move.flg & mPROM))
        {
            short cur_eval = ReturnEval(wtm);
            short capture = 100*pc_streng[GET_INDEX(b[cur_move.to])];
            short margin = 100;
            if(cur_eval + capture + margin < alpha)
                break;
        }
#endif
        MkMoveAndEval(cur_move);
        legal_moves++;
#ifndef NDEBUG
        if((!stop_ply || root_ply == stop_ply) && strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
        if(TO_BLACK(b_state[prev_states + ply].capt) == _k)
        {
            UnMoveAndEval(cur_move);
            return K_VAL;
        }
        FastEval(cur_move);

        x = -QSearch(-beta, -alpha);

        UnMoveAndEval(cur_move);

        if(x >= beta)
        {
            beta_cutoff = true;
            break;
        }
        else if(x > alpha)
        {
            alpha   = x;
            StorePV(cur_move);
        }

        if(stop)
            break;
    }// for(move_cr

    if(beta_cutoff)
    {
#ifndef DONT_SHOW_STATISTICS
        q_cut_cr++;
        if(legal_moves - 1 < (int)(sizeof(q_cut_num_cr)/sizeof(*q_cut_num_cr)))
            q_cut_num_cr[legal_moves - 1]++;
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
        Move cur_move = move_array[move_cr];
        MkMoveFast(cur_move);
#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0)
            ply = ply;
#endif // NDEBUG
//        bool legal = !Attack(king_coord[!wtm], wtm);
        bool legal = Legal(cur_move, in_check);
        if(depth > 1 && legal)
            Perft(depth - 1);
        if(depth == 1 && legal)
            nodes++;
#ifndef NDEBUG
        if((unsigned)depth == max_search_depth && legal)
            std::cout << cv << nodes - tmpCr << std::endl;
#endif
        UnMoveFast(cur_move);
    }
}





//-----------------------------
void StorePV(Move move)
{
    int nextLen = pv[ply + 1][0].flg;
    pv[ply][0].flg = nextLen + 1;
    pv[ply][1] = move;
    memcpy(&pv[ply][2], &pv[ply + 1][1], sizeof(Move)*nextLen);
}





//-----------------------------
void UpdateStatistics(Move move, int depth, unsigned move_cr)
{
#ifndef DONT_SHOW_STATISTICS
        cut_cr++;
        if(move_cr < sizeof(cut_num_cr)/sizeof(*cut_num_cr))
            cut_num_cr[move_cr]++;
        if(move.scr == FIRST_KILLER)
            killer1_probes++;
        if(move.scr == SECOND_KILLER)
            killer2_probes++;
#else
    UNUSED(move_cr);
#endif // DONT_SHOW_STATISTICS
    if(move.flg)
        return;
    if(move != killers[ply][0] && move != killers[ply][1])
    {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = move;
    }

#ifndef DONT_USE_HISTORY
    auto it = coords[wtm].begin();
    it = move.pc;
    UC fr = *it;
    unsigned &h = history[wtm][GET_INDEX(b[fr]) - 1][move.to];
    h += depth*depth + 1;
#else
    UNUSED(depth);
#endif // DONT_USE_HISTORY
}





//--------------------------------
void MainSearch()
{
    busy = true;
    InitSearch();

    short x, prev_x;

    root_ply = 1;
    x = QSearch(-INF, INF);
    pv[0][0].flg = 0;

    max_root_moves = 0;
    for(; root_ply <= max_ply; ++root_ply)
    {
        prev_x = x;
        const short asp_margin = 47;
#ifndef DONT_USE_ASPIRATION_WINDOWS
        x = RootSearch(root_ply, x - asp_margin, x + asp_margin);
        if(!stop && x <= prev_x - asp_margin)
        {
            x = RootSearch(root_ply, -INF, prev_x - asp_margin);
            if (!stop && x >= prev_x - asp_margin)
                x = RootSearch(root_ply, -INF, INF);
        }
        else if(!stop && x >= prev_x + asp_margin)
        {
            x = RootSearch(root_ply, prev_x + asp_margin, INF);
            if(!stop && x <= prev_x + asp_margin)
                x = RootSearch(root_ply, -INF, INF);
        }
#else
        x = RootSearch(root_ply, -INF, INF);
#endif //DONT_USE_ASPIRATION_WINDOWS

        if(stop && x == -INF)
            x = prev_x;

        double time1 = timer.getElapsedTimeInMicroSec();
        time_spent = time1 - time0;

        if(!infinite_analyze && !pondering_in_process)
        {
            if(time_spent > time_to_think
            && !max_nodes_to_search
            && root_ply >= 2)
                break;
            if(ABSI(x) > mate_score && !stop && root_ply >= 2
            && time_spent > time_to_think/4)
                break;
            if(max_root_moves == 1 && root_ply >= 8)
                break;
        }
        if(root_ply >= max_search_depth)
            break;
        if(stop)
            break;

    }// for(root_ply

    if(pondering_in_process && spent_exact_time)
    {
        pondering_in_process = false;
        spent_exact_time = false;
    }
    if(x < -RESIGN_VALUE)
        resign_cr++;
    else
        resign_cr = 0;

    if((!stop && !infinite_analyze && x < -mate_score)
    || (!infinite_analyze && resign_cr > RESIGN_MOVES))
    {
        std::cout << "resign" << std::endl;
        busy = false;
    }

    total_nodes      += nodes;
    total_time_spent += time_spent;
    timer.stop();

    if(!infinite_analyze || uci)
        PrintFinalSearchResult();

    if(!xboard)
        infinite_analyze = false;

    busy = false;
}





//--------------------------------
short RootSearch(int depth, short alpha, short beta)
{
    bool in_check = Attack(*king_coord[wtm], !wtm);
    if(max_root_moves == 0)
        RootMoveGen(in_check);
    if(max_root_moves > 0)
        pv[0][1] = root_moves.at(0).second;

    root_move_cr = 0;

    short x;
    Move  cur_move;
    bool beta_cutoff = false;
    const UQ unconfirmed_fail_high = -1,
            max_root_move_priority = ULLONG_MAX;

    for(; root_move_cr < max_root_moves; root_move_cr++)
    {
        cur_move = root_moves.at(root_move_cr).second;

        MkMove(cur_move);
        nodes++;

#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0 && (!stop_ply || root_ply == stop_ply))
            ply = ply;
#endif // NDEBUG

        FastEval(cur_move);
        UQ prev_nodes = nodes;

        if(uci && root_ply > 6)
            ShowCurrentUciInfo();

        bool fail_high = false;
#ifndef DONT_USE_PVS_IN_ROOT
        if(root_move_cr == 0)
        {
            x = -Search(depth - 1, -beta, -alpha, pv_node);
            if(!stop && x <= alpha)
            {
                ShowPVfailHighOrLow(cur_move, x, '?');
//                UnMove(cur_move);
//                break;
            }
        }
        else
        {
            x = -Search(depth - 1, -alpha - 1, -alpha, cut_node);
            if(!stop && x > alpha)
            {
                fail_high = true;
                ShowPVfailHighOrLow(cur_move, x, '!');

                short x_ = -Search(depth - 1, -beta, -alpha, pv_node);
                if(!stop)
                    x = x_;
                if(x > alpha)
                {
                    pv[0][0].flg = 1;
                    pv[0][1] = cur_move;
                }
                else
                    root_moves.at(root_move_cr).first = unconfirmed_fail_high;
            }
        }
#else
    x = -Search(depth - 1, -beta, -alpha, pv_node);
#endif // DONT_USE_PVS_IN_ROOT
        if(stop && !fail_high)
            x = -INF;

        UQ delta_nodes = nodes - prev_nodes;

        if(root_moves.at(root_move_cr).first != unconfirmed_fail_high)
            root_moves.at(root_move_cr).first = delta_nodes;
        else
            root_moves.at(root_move_cr).first = max_root_move_priority;

        if(x >= beta)
        {
            ShowPVfailHighOrLow(cur_move, x, '!');
            beta_cutoff = true;
            std::swap(root_moves.at(root_move_cr),
                      root_moves.at(0));
            UnMove(cur_move);
            break;
        }
        else if(x > alpha)
        {
            UnMove(cur_move);
            alpha = x;
            StorePV(cur_move);
            if(depth > 3 && x != -INF && !stop)
                PrintCurrentSearchResult(x, ' ');
            std::swap(root_moves.at(root_move_cr),
                      root_moves.at(0));
        }
        else
            UnMove(cur_move);

        if(stop)
            break;
    }// for(; root_move_cr < max_root_moves

    std::sort(root_moves.rbegin(), root_moves.rend() - 1);

    if(depth <= 3 && max_root_moves != 0)
        PrintCurrentSearchResult(alpha, ' ');
    if(max_root_moves == 0)
    {
        pv[0][0].flg = 0;
        return in_check ? -K_VAL + ply : 0;
    }
    if(beta_cutoff)
    {
        UpdateStatistics(cur_move, depth, root_move_cr);
        return beta;
    }
    return alpha;
}





//--------------------------------
void ShowPVfailHighOrLow(Move m, short x, char type_of_bound)
{
    UnMove(m);
    char mstr[6];
    MoveToStr(m, wtm, mstr);

    Move tmp0       = pv[0][0];
    Move tmp1       = pv[0][1];
    pv[0][0].flg    = 1;
    pv[0][1]        = m;

    PrintCurrentSearchResult(x, type_of_bound);
    pv[0][0].flg    = tmp0.flg;
    pv[0][1]        = tmp1;

    MkMove(m);
    FastEval(m);
}





//--------------------------------
void RootMoveGen(bool in_check)
{
    Move move_array[MAX_MOVES], cur_move;
    unsigned max_moves = 999;

    short alpha = -INF, beta = INF;
    cur_move.flg = 0xFF;

    HashProbe(max_ply, &alpha, beta);

    for(unsigned move_cr = 0; move_cr < max_moves; move_cr++)
    {
        cur_move = Next(move_array, move_cr, &max_moves,
                 nullptr, wtm, all_moves, false, cur_move);
    }

    root_moves.clear();
    for(unsigned move_cr = 0; move_cr < max_moves; move_cr++)
    {
        cur_move = move_array[move_cr];
        MkMoveFast(cur_move);
        if(Legal(cur_move, in_check))
        {
            root_moves.push_back(std::pair<UQ, Move>(0, cur_move));
            max_root_moves++;
        }
        UnMoveFast(cur_move);
    }
#if (!defined(DONT_USE_RANDOMNESS) && defined(NDEBUG))
    if(root_ply != 1)
        return;
    std::srand(std::time(nullptr));
    const unsigned max_moves_to_shuffle = 4;
    unsigned moves_to_shuffle = std::min(max_root_moves, max_moves_to_shuffle);
    for(unsigned i = 0; i < moves_to_shuffle; ++i)
    {
        int rand_ix = std::rand() % moves_to_shuffle;
        std::swap(root_moves.at(i), root_moves.at(rand_ix));
    }
#endif // NDEBUG, RANDOMNESS
    pv[0][1] = (*root_moves.begin()).second;
}





//--------------------------------
void InitSearch()
{
    InitTime();
    timer.start();
    time0 = timer.getElapsedTimeInMicroSec();

    nodes           = 0;
    q_nodes         = 0;
    cut_cr          = 0;
    q_cut_cr        = 0;
    futility_probes = 0;
    null_cut_cr     = 0;
    null_probe_cr   = 0;
    hash_cut_cr     = 0;
    hash_probe_cr   = 0;
    hash_hit_cr     = 0;
    hash_cutoff_by_best_move_cr = 0;
    hash_best_move_cr = 0;
    futility_hits   = 0;
    killer1_probes  = 0;
    killer2_probes  = 0;
    killer1_hits    = 0;
    killer2_hits    = 0;

    memset(q_cut_num_cr, 0, sizeof(cut_num_cr));
    memset(cut_num_cr, 0, sizeof(cut_num_cr));
    memset(pv, 0, sizeof(pv));
    memset(killers, 0, sizeof(killers));

    stop = false;

    if(!uci && !xboard)
    {
    std::cout   << "( time total = " << time_remains/1e6
                << "s, assigned to move = " << time_to_think/1e6
                << "s, stop on timer = " << (spent_exact_time ? "true " : "false ")
                << " )" << std::endl;
    std::cout   << "Ply Value  Time    Nodes        Principal Variation" << std::endl;
    }
#ifdef CLEAR_HASH_TABLE_AFTER_EACH_MOVE
    tt.clear();
#endif // CLEAR_HASH_TABLE_AFTER_EACH_MOVE

#ifndef DONT_USE_HISTORY
    memset(history, 0, sizeof(history));
#endif // DONT_USE_HISTORY
}





//--------------------------------
void MoveToStr(Move move, bool stm, char *out)
{
    char proms[] = {'?', 'q', 'n', 'r', 'b'};

    auto it = coords[stm].begin();
    it      = move.pc;
    int  f  = *it;
    out[0]  = COL(f) + 'a';
    out[1]  = ROW(f) + '1';
    out[2]  = COL(move.to) + 'a';
    out[3]  = ROW(move.to) + '1';
    out[4]  = (move.flg & mPROM) ? proms[move.flg & mPROM] : '\0';
    out[5]  = '\0';
}





//--------------------------------
void PrintFinalSearchResult()
{
    char move_str[6];
    MoveToStr(pv[0][1], wtm, move_str);

    if(!uci && !MakeMoveFinaly(move_str))
        std::cout << "tellusererror err01" << std::endl << "resign" << std::endl;

    if(!uci)
        std::cout << "move " << move_str << std::endl;
    else
    {
        std::cout << "bestmove " << move_str;
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
    if(cut_cr == 0)
        cut_cr = 1;
    for(unsigned i = 0; i < sizeof(cut_num_cr)/sizeof(*cut_num_cr); i++)
        std::cout  << std::setprecision(1) << std::fixed
                   << 100.*cut_num_cr[i]/cut_cr << " ";
    std::cout << "]% )" << std::endl;

    std::cout << "( q_nodes = " << q_nodes
              << ", q_cuts = [";
    if(q_cut_cr == 0)
        q_cut_cr = 1;
    for(unsigned i = 0; i < sizeof(q_cut_num_cr)/sizeof(*q_cut_num_cr); i++)
        std::cout  << std::setprecision(1) << std::fixed
                   << 100.*q_cut_num_cr[i]/q_cut_cr << " ";
    std::cout << "]%, ";
    if(nodes == 0)
        nodes = 1;
    std::cout << "q/n = " << std::setprecision(1) << std::fixed
              << 100.*q_nodes/nodes
              << "% )" << std::endl;

    if(hash_probe_cr == 0)
        hash_probe_cr = 1;
    if(hash_hit_cr == 0)
        hash_hit_cr = 1;
    if(hash_best_move_cr == 0)
        hash_best_move_cr = 1;
    std::cout   << "( hash probes = " << hash_probe_cr
                << ", cuts by val = "
                << std::setprecision(1) << std::fixed
                << 100.*hash_cut_cr/hash_probe_cr << "%, "
                << "cuts by best move = "
                << 100.*hash_cutoff_by_best_move_cr/hash_best_move_cr << "% )"
                << std::endl
                << "( hash full = " << (int)100*tt.size()/tt.max_size()
                << "% (" << tt.size()/sizeof(tt_entry)
                << "/" << tt.max_size()/sizeof(tt_entry)
                << " entries )" << std::endl;
#ifndef DONT_USE_NULL_MOVE
    if(null_probe_cr == 0)
        null_probe_cr = 1;
    std::cout   << "( null move probes = " << hash_probe_cr
                << ", cutoffs = "
                << std::setprecision(1) << std::fixed
                << 100.*null_cut_cr/null_probe_cr << "% )"
                << std::endl;
#endif // DONT_USE_NULL_MOVE

#ifndef DONT_USE_FUTILITY
    if(futility_probes == 0)
        futility_probes = 1;
    std::cout << "( futility probes = " << futility_probes
              << ", hits = " << std::setprecision(1) << std::fixed
              << 100.*futility_hits/futility_probes
              << "% )" << std::endl;
#endif // DONT_USE_FUTILITY
    if(killer1_probes == 0)
        killer1_probes = 1;
    std::cout << "( killer1 probes = " << killer1_probes
              << ", hits = " << std::setprecision(1) << std::fixed
              << 100.*killer1_hits/killer1_probes
              << "% )" << std::endl;
    if(killer1_probes == 0)
        killer1_probes = 1;
    std::cout << "( killer2 probes = " << killer2_probes
              << ", hits = " << std::setprecision(1) << std::fixed
              << 100.*killer2_hits/killer2_probes
              << "% )" << std::endl;
    std::cout   << "( time spent =" << time_spent/1.e6
                << "s )" << std::endl;
#endif // DONT_SHOW_STATISTICS
}





//--------------------------------
void PrintCurrentSearchResult(short max_value, char type_of_bound)
{
    using namespace std;

    double time1 = timer.getElapsedTimeInMicroSec();
    int spent_time = (int)((time1 - time0)/1000.);

    if(uci)
    {
        cout << "info depth " << root_ply;

        if(ABSI(max_value) < mate_score)
            cout << " score cp " << max_value;
        else
        {
            if(max_value > 0)
                cout << " score mate " << (K_VAL - max_value + 1) / 2;
            else
                cout << " score mate -" << (K_VAL + max_value + 1) / 2;
        }
        if(type_of_bound == '!')
            cout << " upperbound";
        else if(type_of_bound == '?')
            cout << " lowerbound";

        cout << " time " << spent_time
            << " nodes " << nodes
            << " pv ";
    }
    else
    {
        cout << setfill(' ') << setw(4)  << left << root_ply;
        cout << setfill(' ') << setw(7)  << left << max_value;
        cout << setfill(' ') << setw(8)  << left << spent_time / 10;
        cout << setfill(' ') << setw(12) << left << nodes << ' ';
    }
    ShowPV(0);
    if(!uci && type_of_bound != ' ')
        cout << type_of_bound;
    cout << endl;
}





//-----------------------------
void InitTime()                                                         // too complex
{
    if(!time_command_sent)
        time_remains -= time_spent;
    time_remains = ABSI(time_remains);                                  //<< NB: strange
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
    if(time_inc == 0 && moves_remains != 1 && !spent_exact_time)
        time_to_think /= 2;
    else if(time_inc != 0 && time_base != 0)
        time_to_think += time_inc/4;

    time_command_sent = false;
}





//-----------------------------
bool ShowPV(int cur_ply)
{
    char pc2chr[] = "??KKQQRRBBNNPP";
    bool ans = true;
    int i = 0, pv_len = pv[cur_ply][0].flg;

    if(uci)
    {
        for(; i < pv_len; i++)
        {
            Move cur_move = pv[cur_ply][i + 1];
            auto it = coords[wtm].begin();
            it = cur_move.pc;
            UC from_coord = *it;
            std::cout  << (char)(COL(from_coord) + 'a')
                << (char)(ROW(from_coord) + '1')
                << (char)(COL(cur_move.to) + 'a') << (char)(ROW(cur_move.to) + '1');
            char proms[] = {'?', 'q', 'n', 'r', 'b'};
            if(cur_move.flg & mPROM)
                std::cout << proms[cur_move.flg & mPROM];
            std::cout << " ";
            MkMoveFast(cur_move);
            bool in_check = Attack(*king_coord[wtm], !wtm);
            if(!Legal(cur_move, in_check))
                ans = false;
        }
    }
    else
    {
        for(; i < pv_len; i++)
        {
            Move cur_move = pv[cur_ply][i + 1];
            auto it = coords[wtm].begin();
            it = cur_move.pc;
            char piece_char = pc2chr[b[*it]];
            if(piece_char == 'K' && COL(*it) == 4 && COL(cur_move.to) == 6)
                std::cout << "OO";
            else if(piece_char == 'K' && COL(*it) == 4
                    && COL(cur_move.to) == 2)
                std::cout << "OOO";
            else if(piece_char != 'P')
            {
                std::cout << piece_char;
                FindAndPrintForAmbiguousMoves(cur_move);
                if(cur_move.flg & mCAPT)
                    std::cout << 'x';
                std::cout << (char)(COL(cur_move.to) + 'a');
                std::cout << (char)(ROW(cur_move.to) + '1');
            }
            else if(cur_move.flg & mCAPT)
            {
                it = coords[wtm].begin();
                it = cur_move.pc;
                std::cout << (char)(COL(*it) + 'a')
                    << "x"
                    << (char)(COL(cur_move.to) + 'a')
                    << (char)(ROW(cur_move.to) + '1');
            }
            else
                std::cout << (char)(COL(cur_move.to) + 'a')
                    << (char)(ROW(cur_move.to) + '1');
            char proms[] = "?QNRB";
            if(piece_char == 'P' && (cur_move.flg & mPROM))
                std::cout << proms[cur_move.flg& mPROM];
            std::cout << ' ';
            MkMoveFast(cur_move);
            bool in_check = Attack(*king_coord[wtm], !wtm);
            if(!Legal(cur_move, in_check))
                ans = false;
        }
    }
    for(; i > 0; i--)
        UnMoveFast(*(Move *) &pv[cur_ply][i]);
    return ans;
}





//-----------------------------
void FindAndPrintForAmbiguousMoves(Move move)
{
    Move move_array[8];
    unsigned amb_cr = 0;
    auto it = coords[wtm].begin();
    it = move.pc;
    UC init_from_coord = *it;
    UC init_piece_type = GET_INDEX(b[init_from_coord]);

    for(it = coords[wtm].begin();
        it != coords[wtm].end();
        ++it)
    {
        if(it == move.pc)
            continue;
        UC from_coord = *it;

        UC piece_type = GET_INDEX(b[from_coord]);
        if(piece_type != init_piece_type)
            continue;
        if(!(attacks[120 + from_coord - move.to] & (1 << piece_type)))
            continue;
        if(slider[piece_type] && !SliderAttack(from_coord, move.to))
            continue;
        Move tmp = move;
        tmp.scr = from_coord;
        move_array[amb_cr++] = tmp;
    }

    if(!amb_cr)
        return;

    bool same_cols = false, same_rows = false;
    for(unsigned i = 0; i < amb_cr; i++)
    {
        if(COL(move_array[i].scr) == COL(init_from_coord))
            same_cols = true;
        if(ROW(move_array[i].scr) == ROW(init_from_coord))
            same_rows = true;
    }
    if(same_cols && same_rows)
        std::cout << (char)(COL(init_from_coord) + 'a') << (char)(ROW(init_from_coord) + '1');
    else if(same_cols)
        std::cout << (char)(ROW(init_from_coord) + '1');
    else
        std::cout << (char)(COL(init_from_coord) + 'a');

}





//-----------------------------
bool MakeMoveFinaly(char *move_str)
{
    int ln = strlen(move_str);
    if(ln < 4 || ln > 5)
        return false;
    bool in_check = Attack(*king_coord[wtm], !wtm);
    max_root_moves = 0;
    RootMoveGen(in_check);

    char cur_move_str[6];
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    for(unsigned i = 0; i < max_root_moves; ++i)
    {
        Move cur_move  = root_moves.at(i).second;
        auto it = coords[wtm].begin();
        it = cur_move.pc;
        cur_move_str[0] = COL(*it) + 'a';
        cur_move_str[1] = ROW(*it) + '1';
        cur_move_str[2] = COL(cur_move.to) + 'a';
        cur_move_str[3] = ROW(cur_move.to) + '1';
        cur_move_str[4] = (cur_move.flg & mPROM) ? proms[cur_move.flg & mPROM] : '\0';
        cur_move_str[5] = '\0';

        if(strcmp(move_str, cur_move_str) != 0)
            continue;

        it = coords[wtm].begin();
        it = cur_move.pc;

        MkMove(cur_move);
        FastEval(cur_move);

        short store_val_opn = val_opn;
        short store_val_end = val_end;

        memmove(&b_state[0], &b_state[1],
                (prev_states + 2)*sizeof(BrdState));
        ply--;
        InitEvaOfMaterialAndPst();
        if(val_opn != store_val_opn || val_end != store_val_end)
        {
            std::cout << "telluser err02: wrong score. Fast: "
                    << store_val_opn << '/' << store_val_end
                    << ", all: " << val_opn << '/' << val_end
                    << std::endl << "resign"
                    << std::endl;
        }
        for(int j = 0; j < FIFTY_MOVES; ++j)
            doneHashKeys[j] = doneHashKeys[j + 1];

        finaly_made_moves++;
        return true;
    }// for i
    return false;
}





//--------------------------------
void InitEngine()
{
    InitHashTable();
    hash_key = InitHashKey();

    std::cin.rdbuf()->pubsetbuf(nullptr, 0);
    std::cout.rdbuf()->pubsetbuf(nullptr, 0);

    stop = false;
    total_nodes = 0;
    total_time_spent = 0;

    spent_exact_time  = false;
    finaly_made_moves = 0;
    resign_cr = 0;
    time_spent = 0;

//    tt.clear();
#ifndef DONT_USE_HISTORY
    memset(history, 0, sizeof(history));
#endif // DONT_USE_HISTORY
}





//--------------------------------
bool FenStringToEngine(char *fen)
{
    bool ans = FenToBoard(fen);

    if(!ans)
        return false;

    InitEvaOfMaterialAndPst();
    time_spent   = 0;
    time_remains = time_base;
    finaly_made_moves = 0;
    hash_key = InitHashKey();

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

    const unsigned nodes_to_check_stop2 =
            (16*(nodes_to_check_stop + 1) - 1);
    double margin = 20000; //0.02s
    if(spent_exact_time)
    {
        double time1 = timer.getElapsedTimeInMicroSec();
        time_spent = time1 - time0;
        if(moves_remains == 1)
            margin *= 5;
        if(time_spent >= time_to_think - margin && root_ply >= 2)
            stop = true;
    }
    else if((nodes & nodes_to_check_stop2)
             == nodes_to_check_stop2)
    {
        double time1 = timer.getElapsedTimeInMicroSec();
        time_spent = time1 - time0;
        if(time_spent >= 8*time_to_think
        && root_ply > 2)
            stop = true;
    }

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
bool NullMove(int depth, short beta, bool in_check)
{
    if(in_check || depth < 2
    || material[wtm] - pieces[wtm] < 3)
        return false;
#ifndef DONT_SHOW_STATISTICS
    null_probe_cr++;
#endif // DONT_SHOW_STATISTICS

    if(b_state[prev_states + ply - 1].to == MOVE_IS_NULL
    && b_state[prev_states + ply - 2].to == MOVE_IS_NULL
      )
        return false;

    UC store_ep  = b_state[prev_states + ply].ep;
    UC store_to = b_state[prev_states + ply].to;
    US store_rv = reversible_moves;
    reversible_moves = 0;

    MakeNullMove();
    if(store_ep)
        hash_key = InitHashKey();

    int r = depth > 6 ? 3 : 2;

    short x = -Search(depth - r - 1, -beta, -beta + 1, all_node);

    UnMakeNullMove();
    b_state[prev_states + ply].to = store_to;
    b_state[prev_states + ply].ep = store_ep;
    reversible_moves = store_rv;

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
    && b_state[prev_states + ply - 1].to != MOVE_IS_NULL
    )
    {
#ifndef DONT_SHOW_STATISTICS
            futility_probes++;
#endif // DONT_SHOW_STATISTICS
        short margin = depth < 2 ? 185 : 255;
        short score = ReturnEval(wtm);
        if(score > margin + beta)
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

    for(int row = 7; row >= 0; --row)
    {
        int blank_cr = 0;
        for(int col = 0; col < 8; ++col)
        {
            UC piece = b[XY2SQ(col, row)];
            if(piece == __)
            {
                blank_cr++;
                continue;
            }
            if(blank_cr != 0)
                std::cout << blank_cr;
            blank_cr = 0;
            if(piece & white)
                std::cout << whites[GET_INDEX(piece) - 1];
            else
                std::cout << blacks[GET_INDEX(piece) - 1];
        }
        if(blank_cr != 0)
            std::cout << blank_cr;
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


    std::cout << " -";  // No en passant yet

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
tt_entry* HashProbe(int depth, short *alpha, short beta)
{
    tt_entry* entry = tt.count(hash_key);
    if(entry == nullptr || stop)
        return nullptr;

#ifndef DONT_SHOW_STATISTICS
    hash_probe_cr++;
#endif // DONT_SHOW_STATISTICS
    UC hbnd = entry->bound_type;
    if(entry->depth >= depth)
    {
        short hval = entry->value;
        if(hval > mate_score && hval != INF)
            hval += entry->depth - ply;
        else if(hval < -mate_score && hval != -INF)
            hval -= entry->depth - ply;

        if( hbnd == hEXACT
        || (hbnd == hUPPER && hval >= -*alpha)  //-alpha = beta for parent node
        || (hbnd == hLOWER && hval <= -beta) )  //-beta = alpha for parent node
        {
#ifndef DONT_SHOW_STATISTICS
            hash_cut_cr++;
#endif // DONT_SHOW_STATISTICS
            pv[ply][0].flg = 0;
            *alpha = hval;
            return entry;
        }// if(bnd
    }// if((*entry).depth >= depth
#ifndef DONT_SHOW_STATISTICS
    if(entry->best_move.flg != 0xFF)
        hash_hit_cr++;
#endif // DONT_SHOW_STATISTICS
    return entry;
}





//-----------------------------
bool MoveIsPseudoLegal(Move &move, bool stm)
{
    if(move.flg == 0xFF)
        return false;
    auto it = coords[stm].begin();
    for(; it != coords[stm].end(); ++it)
        if(it == move.pc)
            break;
    if(it == coords[stm].end())
        return false;
    it = move.pc;
    UC from_coord = *it;
    UC piece = b[move.to];
    int delta_col = COL(move.to) - COL(from_coord);
    int delta_row = ROW(move.to) - ROW(from_coord);
    if(!delta_col && !delta_row)
        return false;
    if(!LIGHT(b[from_coord], stm))
        return false;
    if(TO_BLACK(b[from_coord]) != _p
       && ((!DARK(piece, stm) && (move.flg & mCAPT))
    || (piece != __ && !(move.flg & mCAPT))))
        return false;
    if(TO_BLACK(b[from_coord]) != _p && (move.flg & mPROM))
        return false;
    bool long_move;
    switch (TO_BLACK(b[from_coord]))
    {
        case _p :
            if((move.flg & mPROM) && ((stm && ROW(from_coord) != 6)
            || (!stm && ROW(from_coord) != 1)))
                return false;
            if((move.flg & mCAPT) && (ABSI(delta_col) != 1
            || (stm && delta_row != 1) || (!stm && delta_row != -1)
            || (!(move.flg & mENPS) && !DARK(piece, stm))))
                return false;
            if((move.flg & mENPS)
            && (piece != __ || b_state[prev_states + ply].ep == 0))
                return false;
            if(ROW(move.to) == (stm ? 7 : 0)
            && !(move.flg & mPROM))
                return false;
            if(!(move.flg & mCAPT))
            {
                if(!stm)
                    delta_row = -delta_row;
                if(piece != __ || delta_col != 0
                || (stm && delta_row <= 0))
                    return false;
                long_move = (ROW(from_coord) == (stm ? 1 : 6));
                if(long_move ? delta_row > 2 : delta_row != 1)
                    return false;
                if(long_move && delta_row == 2
                && b[XY2SQ(COL(from_coord), (ROW(from_coord) + ROW(move.to))/2)] != __)
                    return false;
            }

            break;
        case _n :
            move.flg &= mCAPT;
            if(ABSI(delta_col) + ABSI(delta_row) != 3)
                return false;
            if(ABSI(delta_col) != 1 && ABSI(delta_row) != 1)
                return false;
            break;
        case _b :
        case _r :
        case _q :
            move.flg &= mCAPT;
            if(!(attacks[120 + move.to - from_coord] & (1 << GET_INDEX(b[from_coord])))
            || !SliderAttack(move.to, from_coord))
                return false;
            break;
        case _k :
            if(!(move.flg & mCSTL))
            {
                if((ABSI(delta_col) > 1 || ABSI(delta_row) > 1))
                    return false;
                else
                    return true;
            }
            if(ABSI(delta_col) != 2 || ABSI(delta_row) != 0)
                return false;
            if(COL(from_coord) != 4 || ROW(from_coord) != stm ? 0 : 7)
                return false;
            if((b_state[prev_states + ply].cstl &
            (move.flg >> 3 >> (2*stm))) == 0)
                return false;
            if(b[XY2SQ(COL(move.to), ROW(from_coord))] != __)
                return false;
            if(b[XY2SQ((COL(move.to)+COL(from_coord))/2, ROW(from_coord))] != __)
                return false;
            if((move.flg &mCS_K)
            && TO_BLACK(b[XY2SQ(7, ROW(from_coord))]) != _r)
                return false;
            if((move.flg &mCS_Q)
            && TO_BLACK(b[XY2SQ(0, ROW(from_coord))]) != _r)
                return false;
            break;

        default :
            return false;
    }
    return true;
}





//--------------------------------
Move Next(Move *move_array, unsigned cur_move, unsigned *max_moves,
          tt_entry *entry, UC stm, bool only_captures, bool in_check,
          Move prev_move)
{
#ifdef DONT_USE_ONE_REPLY_EXTENSION
    UNUSED(in_check);
#endif

    Move ans;
    if(cur_move == 0)
    {
        if(entry == nullptr)
        {
            if(!only_captures)
                *max_moves = GenMoves(move_array, APPRICE_ALL, nullptr);
            else
                *max_moves = GenCaptures(move_array);

            if(*max_moves > 1
            && move_array[0].scr > BAD_CAPTURES
            && move_array[0].scr == move_array[1].scr)
            {
                if(SEE_main(move_array[0]) < SEE_main(move_array[1]))
                    std::swap(move_array[0], move_array[1]);
            }
        }
        else
        {
            ans = entry->best_move;

            bool pseudo_legal = MoveIsPseudoLegal(ans, stm);
#ifndef NDEBUG
            int mx_ = GenMoves(move_array, APPRICE_NONE, nullptr);
            int i = 0;
            for(; i < mx_; ++i)
                if(move_array[i] == ans)
                    break;
            bool tt_move_found = i < mx_;
            if(tt_move_found != pseudo_legal)
                MoveIsPseudoLegal(ans, stm);
            assert(tt_move_found == pseudo_legal);
#endif
            if(pseudo_legal)
            {
                ans.scr = PV_FOLLOW;
#ifndef DONT_USE_ONE_REPLY_EXTENSION
                if(entry->one_reply)
                {
                    MkMoveFast(ans);
                    if(!Legal(ans, in_check))
                    {
                        UnMoveFast(ans);
                        *entry = nullptr;
                        *max_moves = GenMoves(move_array, APPRICE_ALL, nullptr);
                    }
                    else
                    {
                        UnMoveFast(ans);
                        *max_moves = 1;
                        move_array[0] = ans;
                        return ans;
                    }
                }
                else
                    return ans;
#else
                return ans;
#endif // DONT_USE_ONE_REPLY_EXTENSION
            }
            else
            {
                entry = nullptr;
                *max_moves = GenMoves(move_array, APPRICE_ALL, nullptr);
            }
        }// else (entry == nullptr)
    }// if cur == 0
    else if(cur_move == 1 && entry != nullptr)
    {
        *max_moves = GenMoves(move_array, APPRICE_ALL, &prev_move);
        if(*max_moves <= 1)
        {
            *max_moves = 0;
            return move_array[0];
        }   
        unsigned i = 0;
        for(; i < *max_moves; i++)
            if(move_array[i].scr == PV_FOLLOW)
            {
                std::swap(move_array[0], move_array[i]);
                break;
            }
        assert(i < *max_moves);
    }

    if(only_captures)  // already sorted
        return move_array[cur_move];

#ifndef DONT_USE_ONE_REPLY_EXTENSION
    if(in_check && (cur_move == 0 || (cur_move == 1 && entry != nullptr)))
    {
        unsigned move_cr, legal_cr = cur_move;
        for(move_cr = cur_move; move_cr < *max_moves; ++move_cr)
        {
            Move m = move_array[move_cr];
            MkMoveFast(m);
            if(!Legal(m, in_check))
            {
                UnMoveFast(m);
                continue;
            }
            UnMoveFast(m);
            move_array[legal_cr++] = move_array[move_cr];
        }
        *max_moves = legal_cr;
        if(cur_move > 0 && legal_cr == 1)
            *max_moves = 0;
    }
#endif //DONT_USE_ONE_REPLY_EXTENSION

    int max_score = -INF;
    unsigned max_index = cur_move;

    for(unsigned i = cur_move; i < *max_moves; i++)
    {
        UC score = move_array[i].scr;
        if(score > max_score)
        {
            max_score = score;
            max_index = i;
        }
    }
    ans = move_array[max_index];
    if(max_index != cur_move)
    {
        move_array[max_index] = move_array[cur_move];
        move_array[cur_move] = ans;
    }
    return ans;
}





//-----------------------------
void StoreResultInHash(int depth, short init_alpha, short alpha,
                       short beta, unsigned legals,
                       bool beta_cutoff, Move best_move,
                       bool one_reply)
{
    if(stop)
        return;
    if(beta_cutoff)
    {
        if(beta > mate_score && beta != INF)
            beta += ply - depth - 1;
        else if(beta < -mate_score && beta != -INF)
            beta -= ply - depth - 1;
        tt.add(hash_key, -beta, best_move, depth,
               hLOWER, finaly_made_moves/2, one_reply);

    }
    else if(alpha > init_alpha && pv[ply][0].flg > 0)
    {
        if(alpha > mate_score && alpha != INF)
            alpha += ply - depth;
        else if(alpha < - mate_score && alpha != -INF)
            alpha -= ply - depth;
        tt.add(hash_key, -alpha, pv[ply][1], depth,
                hEXACT, finaly_made_moves/2, one_reply);
    }
    else if(alpha <= init_alpha)
    {
        if(init_alpha > mate_score && init_alpha != INF)
            init_alpha += ply - depth;
        else if(init_alpha < -mate_score && init_alpha != -INF)
            init_alpha -= ply - depth;
        Move no_move;
        no_move.flg = 0xFF;
        tt.add(hash_key, -init_alpha,
               legals > 0 ? best_move : no_move, depth,
               hUPPER, finaly_made_moves/2, one_reply);
    }
}





//-----------------------------
void ShowCurrentUciInfo()
{
    double t = timer.getElapsedTimeInMicroSec();

    std::cout << "info nodes " << nodes
        << " nps " << (int)(1000000 * nodes / (t - time0 + 1));

    Move move = root_moves.at(root_move_cr).second;
    UC from_coord = b_state[prev_states + 1].fr;
    std::cout << " currmove "
        << (char)(COL(from_coord) + 'a') << (char)(ROW(from_coord) + '1')
        << (char)(COL(move.to) + 'a') << (char)(ROW(move.to) + '1');
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    if(move.flg & mPROM)
        std::cout << proms[move.flg & mPROM];

    std::cout << " currmovenumber " << root_move_cr + 1;
    std::cout << " hashfull ";

    UQ hash_size = 1000*tt.size() / tt.max_size();
    std::cout << hash_size << std::endl;
}





//-----------------------------
void PonderHit()
{
    double time1 = timer.getElapsedTimeInMicroSec();
    time_spent = time1 - time0;
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
4n3/3N4/4P3/3P2p1/p1K5/7k/8/8 w - - 0 58 wrong connected passers eval
8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - - 0 1 bm Rxb2; id "WAC.002";
r1b2rk1/ppq3pp/2nb1n2/3pp3/3P3B/3B1N2/PP2NPPP/R2Q1RK1 w - - 0 1 low value of first move cuts in QS
8/6k1/8/6pn/pQ6/Pp3P1K/1P3P1P/6r1 w - - 2 31 draw due to perpetual check
2kb1R2/8/2p2B2/1pP2P2/1P6/1K6/8/3r4 w - - 0 1 bm Rxd8 Kc3 Be7
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

1r1q1rk1/1b1n1ppp/p1pQp3/3p4/4P3/2N2B2/PPP2PPP/R3R1K1 w - - 3 8 am e5

r3kb1r/1b1n1p1p/pq2Np2/1p2p2Q/5P2/2N5/PPP3PP/2KR1B1R w kq - 0 1 bm Rxd7 low value of first move cuts in QS
r4rk1/pb3ppp/1p3q2/1Nbp4/2P1nP2/P4BP1/1PQ4P/R1B2R1K b - - 0 1 Qg6 is the best?
8/p5p1/p3k3/6PP/3NK3/8/Pb6/8 b - - 2 48 am Bxd4
r1bqkb1r/pp1n1pp1/2p1pn1p/6N1/3P4/3B1N2/PPP2PPP/R1BQK2R w KQkq - 0 1 bm Nxe6 Deep Blue
1rq2b1r/2N1k1pp/1pQp4/4n3/2P5/8/PP4PP/4RRK1 w - - 0 1 bm Rxe5
5rk1/pq2nbp1/1p5p/2n2P2/4P1Q1/1PN4N/PB6/5K1R w - - 0 1 bm Qxg7
2q2r2/3n1p2/p2p2k1/3PpRp1/P1n1P3/2P2QB1/1rB1R1P1/6K1 w - - bm Rxg5+; id "arasan16.3";
r3r1k1/3q1ppp/p4n2/1p1P4/2P2bb1/5N1P/PR1NBPP1/3Q1RK1 b - - 0 21 bm Bxh3
6R1/p2r4/1p1p1b2/1P1P1p1k/1K1PpB2/4P2P/8/8 b - - 51 72 am Rd8
2r3k1/p6p/q4p2/r1p1pP2/2PpP3/BR1Qb2P/P5P1/1R5K b - - 0 31 am Kg7

2r3k1/3b1pp1/p3p2p/q2pB2P/1brP4/2NRQP2/1PPK2P1/1R6 b - - 1 16 KS eval
8/4B2k/2p3pp/4P3/6P1/2K5/5P1P/1r6 w - - 0 45 eval for material and PST
8/6p1/1p1k2p1/8/7P/6P1/5P2/K7 b - - 0 35
2R1n1k1/1p3ppp/p3p3/1q1p3N/3P2Q1/Pr2P3/1P3PPP/6K1 b - - 3 16 sd 4
rn3rk1/p3qppp/bp3n2/2bp4/N7/1P3NP1/P1QBPPBP/R3K2R w KQ - 6 5 sd 4 am Nxc5
8/8/4b3/b2p4/P4pkP/1R6/5B2/2K5 b - - 0 48 black pawns eval
rr4k1/1pq2p1p/p2b2p1/2pPp3/P1P1P3/4N2P/1R2QPP1/1R4K1 b - - 1 28 am Ra7

4r1k1/5p1p/r2p2n1/p1pP3Q/4P3/2N3R1/2q2PP1/1R4K1 b - - 0 19 KS eval
r4rk1/p1p3pp/1p1ppn2/5p2/P1PP2qP/1P1BPbP1/1B1Q1P2/R4RK1 w - - 1 10 KS eval
5rk1/1p1n1pp1/1q3np1/2bpp1B1/r5P1/3P3P/P1PN1PB1/R2Q1RK1 w - - 0 8 KS eval

6r1/8/4k2B/4P1pP/3K2P1/8/8/8 b - - 4 73 am Rd8

*/

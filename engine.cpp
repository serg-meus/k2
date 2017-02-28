#include "engine.h"





k2engine::k2engine() :
    engine_version{"0.88dev"},
    stop_str{""},
    stop_ply{0}
{
    busy = false;
    uci = false;
    xboard = false;
    pondering_in_process = false;
    stop = false;
    resign_cr = 0;

    max_search_depth = k2chess::max_ply;
    time_remains = 300000000;
    time_base = 300000000;
    time_inc = 0;
    moves_per_session = 0;
    max_nodes_to_search = 0;
    time_command_sent = false;
    infinite_analyze = false;
    spent_exact_time = false;
    randomness = true;

    InitEngine();
}





//--------------------------------
k2chess::score_t k2engine::Search(depth_t depth, score_t alpha, score_t beta,
                                  node_type_t node_type)
{
    if(ply >= max_ply - 1 || DrawDetect())
    {
        pv[ply][0].flag = 0;
        return 0;
    }
    bool in_check = Attack(*king_coord[wtm], !wtm);

    if(depth < 0)
        depth = 0;
    if(in_check)
        depth++;

    if(!in_check
            && state[ply - 1].captured_piece
            && state[ply].captured_piece
            && state[ply].to_coord == state[ply - 1].to_coord
            && state[ply - 1].priority > bad_captures
            && state[ply].priority > bad_captures
      )
        depth++;

    auto mate_sc = king_value - ply;
    if(alpha >= mate_sc)
        return alpha;
    if(beta <= -mate_sc)
        return beta;

    if(depth <= 2 && !in_check && beta < mate_score
            && Futility(depth, beta))
        return beta;

    if(NullMove(depth, beta, in_check))
        return beta;

    score_t x, initial_alpha = alpha;
    hash_entry_s *entry = nullptr;
    if(depth > 0 && HashProbe(depth, &alpha, beta, &entry))
        return -alpha;

    if(depth <= 0)
        return QSearch(alpha, beta);

    nodes++;
    if((nodes & nodes_to_check_stop) == nodes_to_check_stop)
        CheckForInterrupt();

    move_c move_array[move_array_size], cur_move;
    movcr_t move_cr = 0, legal_moves = 0, first_legal = 0;
    movcr_t max_moves = init_max_moves;
    bool beta_cutoff = false;

    for(; move_cr < max_moves; move_cr++)
    {
        cur_move = Next(move_array, move_cr, &max_moves,
                        entry, wtm, all_moves, cur_move);
        if(max_moves <= 0)
            break;

        bool is_special_move = MkMoveFast(cur_move);
#ifndef NDEBUG
        if((!stop_ply || root_ply == stop_ply) && strcmp(stop_str, cv) == 0)
            std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG

        if(!Legal(state[ply].from_coord, cur_move.to_coord, in_check))
        {
            UnMoveFast(cur_move);
            continue;
        }

        if(depth <= 2 && !cur_move.flag && !in_check
                && node_type == all_node && legal_moves > 4)
        {
            UnMoveFast(cur_move);
            break;
        }

        MkMoveIncrementally(cur_move, is_special_move);
        FastEval(cur_move);

        auto lmr = 1;
        if(depth < 3 || cur_move.flag || in_check)
            lmr = 0;
        else if(legal_moves < 4)
            lmr = 0;
        else if(to_black(b[cur_move.to_coord]) == black_pawn
                && IsPasser(get_col(cur_move.to_coord), !wtm))
            lmr = 0;
        else if(depth <= 4 && legal_moves > 8)
            lmr = 2;

        if(legal_moves == 0)
            x = -Search(depth - 1, -beta, -alpha, -node_type);
        else if(beta > alpha + 1)
        {
            x = -Search(depth - 1 - lmr, -alpha - 1, -alpha, cut_node);
            if(x > alpha)
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
    }

    if(legal_moves == 0 && initial_alpha <= mate_score)
    {
        pv[ply][0].flag = 0;
        return in_check ? -king_value + ply : 0;
    }
    else if(legal_moves)
        StoreResultInHash(depth, initial_alpha, alpha, beta, legal_moves,
                          beta_cutoff,
                          (beta_cutoff ? cur_move : move_array[first_legal]));
    if(beta_cutoff)
    {
        if(entry != nullptr && entry->best_move.flag != not_a_move)
            hash_best_move_cr++;
        if(legal_moves == 1)
        {
            if(entry != nullptr && entry->best_move.flag != not_a_move)
                hash_cutoff_by_best_move_cr++;
            else if(cur_move.priority == first_killer)
                killer1_hits++;
            else if(cur_move.priority == second_killer)
                killer2_hits++;
        }
        UpdateStatistics(cur_move, depth, legal_moves - 1);
        return beta;
    }
    return alpha;
}





//-----------------------------
k2chess::score_t k2engine::QSearch(score_t alpha, score_t beta)
{
    if(ply >= max_ply - 1)
        return 0;

    pv[ply][0].flag = 0;

    if(material[0] + material[1] > 24
            && ReturnEval(wtm) > beta + 250)
        return beta;

    auto x = Eval();

    if(-x >= beta)
        return beta;
    else if(-x > alpha)
        alpha = -x;

    nodes++;
    q_nodes++;
    if((nodes & nodes_to_check_stop) == nodes_to_check_stop)
        CheckForInterrupt();

    move_c move_array[move_array_size];
    movcr_t move_cr = 0, legal_moves = 0, max_moves = init_max_moves;
    bool beta_cutoff = false;

    for(; move_cr < max_moves; move_cr++)
    {
        move_c cur_move = Next(move_array, move_cr, &max_moves,
                               nullptr, wtm, captures_only, cur_move);
        if(max_moves <= 0)
            break;

        if(cur_move.priority <= bad_captures)
            break;

        if(material[white] + material[black]
                - pieces[white] - pieces[black] > 24
                && to_black(b[cur_move.to_coord]) != black_king
                && !(cur_move.flag & is_promotion))
        {
            auto cur_eval = ReturnEval(wtm);
            auto capture = 100*pc_streng[get_index(b[cur_move.to_coord])];
            auto margin = 100;
            if(cur_eval + capture + margin < alpha)
                break;
        }
        MkMoveAndEval(cur_move);
        legal_moves++;
#ifndef NDEBUG
        if((!stop_ply || root_ply == stop_ply) && strcmp(stop_str, cv) == 0)
            std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG
        if(to_black(state[ply].captured_piece) == black_king)
        {
            UnMoveAndEval(cur_move);
            return king_value;
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
            alpha = x;
            StorePV(cur_move);
        }

        if(stop)
            break;
    }

    if(beta_cutoff)
    {
        q_cut_cr++;
        if(legal_moves < 1 + (sizeof(q_cut_num_cr)/sizeof(*q_cut_num_cr)))
            q_cut_num_cr[legal_moves - 1]++;
        return beta;
    }
    return alpha;
}





//--------------------------------
void k2engine::Perft(depth_t depth)
{
    move_c move_array[move_array_size];
    bool in_check = Attack(*king_coord[wtm], !wtm);
    auto max_moves = GenMoves(move_array, nullptr, not_apprice);
    for(auto move_cr = 0; move_cr < max_moves; move_cr++)
    {
#ifndef NDEBUG
        node_t tmpCr;
        if(depth == max_search_depth)
            tmpCr = nodes;
#endif
        auto cur_move = move_array[move_cr];
        MkMoveFast(cur_move);
#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0)
            std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG
//        bool legal = !Attack(king_coord[!wtm], wtm);
        bool legal = Legal(state[ply].from_coord, cur_move.to_coord, in_check);
        if(depth > 1 && legal)
            Perft(depth - 1);
        if(depth == 1 && legal)
            nodes++;
#ifndef NDEBUG
        if(depth == max_search_depth && legal)
            std::cout << cv << nodes - tmpCr << std::endl;
#endif
        UnMoveFast(cur_move);
    }
}





//-----------------------------
void k2engine::StorePV(move_c move)
{
    auto nextLen = pv[ply + 1][0].flag;
    pv[ply][0].flag = nextLen + 1;
    pv[ply][1] = move;
    memcpy(&pv[ply][2], &pv[ply + 1][1], sizeof(move_c)*nextLen);
}





//-----------------------------
void k2engine::UpdateStatistics(move_c move, depth_t depth, movcr_t move_cr)
{

    cut_cr++;
    if(move_cr < sizeof(cut_num_cr)/sizeof(*cut_num_cr))
        cut_num_cr[move_cr]++;
    if(move.priority == first_killer)
        killer1_probes++;
    if(move.priority == second_killer)
        killer2_probes++;

    if(move.flag)
        return;
    if(move != killers[ply][0] && move != killers[ply][1])
    {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = move;
    }

    auto it = coords[wtm].begin();
    it = move.piece_iterator;
    auto from_coord = *it;
    auto &h = history[wtm][get_index(b[from_coord]) - 1][move.to_coord];
    h += depth*depth + 1;
}





//--------------------------------
void k2engine::MainSearch()
{
    busy = true;
    InitSearch();

    score_t x, prev_x;

    root_ply = 1;
    x = QSearch(-infinite_score, infinite_score);
    pv[0][0].flag = 0;
    if(initial_score == infinite_score)
        initial_score = x;

    max_root_moves = 0;
    for(; root_ply <= max_ply; ++root_ply)
    {
        prev_x = x;
        const score_t asp_margin = 47;

        x = RootSearch(root_ply, x - asp_margin, x + asp_margin);
        if(!stop && x <= prev_x - asp_margin)
        {
            x = RootSearch(root_ply, -infinite_score, prev_x - asp_margin);
            if (!stop && x >= prev_x - asp_margin)
                x = RootSearch(root_ply, -infinite_score, infinite_score);
        }
        else if(!stop && x >= prev_x + asp_margin)
        {
            x = RootSearch(root_ply, prev_x + asp_margin, infinite_score);
            if(!stop && x <= prev_x + asp_margin)
                x = RootSearch(root_ply, -infinite_score, infinite_score);
        }

        if(stop && x == -infinite_score)
            x = prev_x;

        double time1 = timer.getElapsedTimeInMicroSec();
        time_spent = time1 - time0;

        if(!infinite_analyze && !pondering_in_process)
        {
            if(time_spent > time_to_think
                    && !max_nodes_to_search
                    && root_ply >= 2)
                break;
            if(std::abs(x) > mate_score && std::abs(prev_x) > mate_score
                    && !stop && root_ply >= 2)
                break;
            if(max_root_moves == 1 && root_ply >= 8)
                break;
        }
        if(root_ply >= max_search_depth)
            break;
        if(stop)
            break;
    }

    if(pondering_in_process && spent_exact_time)
    {
        pondering_in_process = false;
        spent_exact_time = false;
    }
    if(x < initial_score - score_to_resign || x < -mate_score)
        resign_cr++;
    else if(max_root_moves == 1 && resign_cr > 0)
        resign_cr++;
    else
        resign_cr = 0;

    if(!infinite_analyze
            && (x < -mate_score || resign_cr > moves_to_resign))
        std::cout << "resign" << std::endl;

    total_nodes += nodes;
    total_time_spent += time_spent;
    timer.stop();

    if(!infinite_analyze || uci)
        PrintFinalSearchResult();

    if(!xboard)
        infinite_analyze = false;

    busy = false;
}





//--------------------------------
k2chess::score_t k2engine::RootSearch(depth_t depth, score_t alpha,
                                      score_t beta)
{
    bool in_check = Attack(*king_coord[wtm], !wtm);
    if(max_root_moves == 0)
        RootMoveGen(in_check);
    if(max_root_moves > 0)
        pv[0][1] = root_moves.at(0).second;

    root_move_cr = 0;

    score_t x;
    move_c cur_move;
    bool beta_cutoff = false;
    const node_t unconfirmed_fail_high = -1,
                 max_root_move_priority = std::numeric_limits<node_t>::max();

    for(; root_move_cr < max_root_moves; root_move_cr++)
    {
        cur_move = root_moves.at(root_move_cr).second;

        MkMove(cur_move);
        nodes++;

#ifndef NDEBUG
        if(strcmp(stop_str, cv) == 0 && (!stop_ply || root_ply == stop_ply))
            std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG

        FastEval(cur_move);
        auto prev_nodes = nodes;

        if(uci && root_ply > 6)
            ShowCurrentUciInfo();

        bool fail_high = false;

        if(root_move_cr == 0)
        {
            x = -Search(depth - 1, -beta, -alpha, pv_node);
            if(!stop && x <= alpha)
                ShowPVfailHighOrLow(cur_move, x, '?');
        }
        else
        {
            x = -Search(depth - 1, -alpha - 1, -alpha, cut_node);
            if(!stop && x > alpha)
            {
                fail_high = true;
                ShowPVfailHighOrLow(cur_move, x, '!');

                auto x_ = -Search(depth - 1, -beta, -alpha, pv_node);
                if(!stop)
                    x = x_;
                if(x > alpha)
                {
                    pv[0][0].flag = 1;
                    pv[0][1] = cur_move;
                }
                else
                    root_moves.at(root_move_cr).first = unconfirmed_fail_high;
            }
        }

        if(stop && !fail_high)
            x = -infinite_score;

        auto delta_nodes = nodes - prev_nodes;

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
            if(depth > 3 && x != -infinite_score && !stop)
                PrintCurrentSearchResult(x, ' ');
            std::swap(root_moves.at(root_move_cr),
                      root_moves.at(0));
        }
        else
            UnMove(cur_move);

        if(stop)
            break;
    }

    std::sort(root_moves.rbegin(), root_moves.rend() - 1);

    if(depth <= 3 && max_root_moves != 0)
        PrintCurrentSearchResult(alpha, ' ');
    if(max_root_moves == 0)
    {
        pv[0][0].flag = 0;
        return in_check ? -king_value + ply : 0;
    }
    if(beta_cutoff)
    {
        UpdateStatistics(cur_move, depth, root_move_cr);
        return beta;
    }
    return alpha;
}





//--------------------------------
void k2engine::ShowPVfailHighOrLow(move_c m, score_t x, char type_of_bound)
{
    UnMove(m);
    char mstr[6];
    MoveToStr(m, wtm, mstr);

    auto tmp0 = pv[0][0];
    auto tmp1 = pv[0][1];
    pv[0][0].flag = 1;
    pv[0][1] = m;

    PrintCurrentSearchResult(x, type_of_bound);
    pv[0][0].flag = tmp0.flag;
    pv[0][1] = tmp1;

    MkMove(m);
    FastEval(m);
}





//--------------------------------
void k2engine::RootMoveGen(bool in_check)
{
    move_c move_array[move_array_size], cur_move;
    auto max_moves = init_max_moves;

    score_t alpha = -infinite_score, beta = infinite_score;
    cur_move.flag = not_a_move;

    hash_entry_s *entry = nullptr;
    HashProbe(max_ply, &alpha, beta, &entry);

    for(movcr_t move_cr = 0; move_cr < max_moves; move_cr++)
    {
        cur_move = Next(move_array, move_cr, &max_moves,
                        nullptr, wtm, all_moves, cur_move);
    }

    root_moves.clear();
    for(movcr_t move_cr = 0; move_cr < max_moves; move_cr++)
    {
        cur_move = move_array[move_cr];
        MkMoveFast(cur_move);
        if(Legal(state[ply].from_coord, cur_move.to_coord, in_check))
        {
            root_moves.push_back(std::pair<node_t, move_c>(0, cur_move));
            max_root_moves++;
        }
        UnMoveFast(cur_move);
    }

    if(root_ply != 1)
        return;

    if(randomness)
    {
        std::srand(std::time(nullptr));
        const movcr_t max_moves_to_shuffle = 4;
        auto moves_to_shuffle = std::min(max_root_moves, max_moves_to_shuffle);
        for(movcr_t i = 0; i < moves_to_shuffle; ++i)
        {
            movcr_t rand_ix = std::rand() % moves_to_shuffle;
            std::swap(root_moves.at(i), root_moves.at(rand_ix));
        }
    }
    pv[0][1] = (*root_moves.begin()).second;
}




//--------------------------------
void k2engine::InitSearch()
{
    InitTime();
    timer.start();
    time0 = timer.getElapsedTimeInMicroSec();

    nodes = 0;
    q_nodes = 0;
    cut_cr = 0;
    q_cut_cr = 0;
    futility_probes = 0;
    null_cut_cr = 0;
    null_probe_cr = 0;
    hash_cut_cr = 0;
    hash_probe_cr = 0;
    hash_hit_cr = 0;
    hash_cutoff_by_best_move_cr = 0;
    hash_best_move_cr = 0;
    futility_hits = 0;
    killer1_probes = 0;
    killer2_probes = 0;
    killer1_hits = 0;
    killer2_hits = 0;

    memset(q_cut_num_cr, 0, sizeof(cut_num_cr));
    memset(cut_num_cr, 0, sizeof(cut_num_cr));
    memset(pv, 0, sizeof(pv));
    memset(killers, 0, sizeof(killers));

    stop = false;

    if(!uci && !xboard)
    {
        std::cout << "( time total = " << time_remains/1e6
                  << "s, assigned to move = " << time_to_think/1e6
                  << "s, stop on timer = "
                  << (spent_exact_time ? "true " : "false ")
                  << " )" << std::endl;
        std::cout << "Ply Value  Time    Nodes        Principal Variation"
                  << std::endl;
    }

    memset(history, 0, sizeof(history));
}




//--------------------------------
void k2engine::MoveToStr(move_c move, bool stm, char *out)
{
    char proms[] = {'?', 'q', 'n', 'r', 'b'};

    auto it = coords[stm].begin();
    it = move.piece_iterator;
    auto f = *it;
    out[0] = get_col(f) + 'a';
    out[1] = get_row(f) + '1';
    out[2] = get_col(move.to_coord) + 'a';
    out[3] = get_row(move.to_coord) + '1';
    out[4] = (move.flag & is_promotion) ?
              proms[move.flag & is_promotion] : '\0';
    out[5] = '\0';
}





//--------------------------------
void k2engine::PrintFinalSearchResult()
{
    char move_str[6];
    MoveToStr(pv[0][1], wtm, move_str);

    if(!uci && !MakeMoveFinaly(move_str))
        std::cout << "tellusererror err01"
                  << std::endl << "resign" << std::endl;

    if(!uci)
        std::cout << "move " << move_str << std::endl;
    else
    {
        std::cout << "bestmove " << move_str;
        if(!infinite_analyze && pv[0][0].flag > 1)
        {
            char pndr[6];
            MoveToStr(pv[0][2], !wtm, pndr);
            std::cout << " ponder " << pndr;
        }
        std::cout << std::endl;
    }

    if(xboard || uci)
        return;

    std::cout << "( nodes = " << nodes
              << ", cuts = [";
    if(cut_cr == 0)
        cut_cr = 1;
    for(size_t i = 0; i < sizeof(cut_num_cr)/sizeof(*cut_num_cr); i++)
        std::cout << std::setprecision(1) << std::fixed
                  << 100.*cut_num_cr[i]/cut_cr << " ";
    std::cout << "]% )" << std::endl;

    std::cout << "( q_nodes = " << q_nodes
              << ", q_cuts = [";
    if(q_cut_cr == 0)
        q_cut_cr = 1;
    for(size_t i = 0; i < sizeof(q_cut_num_cr)/sizeof(*q_cut_num_cr); i++)
        std::cout << std::setprecision(1) << std::fixed
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
    std::cout << "( hash probes = " << hash_probe_cr
              << ", cuts by val = "
              << std::setprecision(1) << std::fixed
              << 100.*hash_cut_cr/hash_probe_cr << "%, "
              << "cuts by best move = "
              << 100.*hash_cutoff_by_best_move_cr/hash_best_move_cr << "% )"
              << std::endl
              << "( hash full = "
              << (i32)100*hash_table.size()/hash_table.max_size()
              << "% (" << hash_table.size()/sizeof(hash_entry_s)
              << "/" << hash_table.max_size()/sizeof(hash_entry_s)
              << " entries )" << std::endl;

    if(null_probe_cr == 0)
        null_probe_cr = 1;
    std::cout << "( null move probes = " << hash_probe_cr
              << ", cutoffs = "
              << std::setprecision(1) << std::fixed
              << 100.*null_cut_cr/null_probe_cr << "% )"
              << std::endl;

    if(futility_probes == 0)
        futility_probes = 1;
    std::cout << "( futility probes = " << futility_probes
              << ", hits = " << std::setprecision(1) << std::fixed
              << 100.*futility_hits/futility_probes
              << "% )" << std::endl;

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
    std::cout << "( time spent = " << time_spent/1.e6
              << "s )" << std::endl;
}





//--------------------------------
void k2engine::PrintCurrentSearchResult(score_t max_value, char type_of_bound)
{
    using namespace std;

    double time1 = timer.getElapsedTimeInMicroSec();
    i32 spent_time = ((time1 - time0)/1000.);

    if(uci)
    {
        cout << "info depth " << root_ply;

        if(std::abs(max_value) < mate_score)
            cout << " score cp " << max_value;
        else
        {
            if(max_value > 0)
                cout << " score mate " << (king_value - max_value + 1) / 2;
            else
                cout << " score mate -" << (king_value + max_value + 1) / 2;
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
        cout << setfill(' ') << setw(4) << left << root_ply;
        cout << setfill(' ') << setw(7) << left << max_value;
        cout << setfill(' ') << setw(8) << left << spent_time / 10;
        cout << setfill(' ') << setw(12) << left << nodes << ' ';
    }
    ShowPV(0);
    if(!uci && type_of_bound != ' ')
        cout << type_of_bound;
    cout << endl;
}





//-----------------------------
void k2engine::InitTime()
{
    if(!time_command_sent)
        time_remains -= time_spent;
    time_remains = std::abs(time_remains);  //<< NB: strange
    depth_t movsAfterControl;
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

    if(!spent_exact_time && (moves_remains <= moves_for_time_exact_mode
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
bool k2engine::ShowPV(depth_t cur_ply)
{
    char pc2chr[] = "??KKQQRRBBNNPP";
    bool ans = true;
    depth_t ply_cr = 0;
    auto pv_len = pv[cur_ply][0].flag;

    if(uci)
    {
        for(; ply_cr < pv_len; ply_cr++)
        {
            move_c cur_move = pv[cur_ply][ply_cr + 1];
            auto it = coords[wtm].begin();
            it = cur_move.piece_iterator;
            auto from_coord = *it;
            std::cout << (char)(get_col(from_coord) + 'a')
                      << (char)(get_row(from_coord) + '1')
                      << (char)(get_col(cur_move.to_coord) + 'a')
                      << (char)(get_row(cur_move.to_coord) + '1');
            char proms[] = {'?', 'q', 'n', 'r', 'b'};
            if(cur_move.flag & is_promotion)
                std::cout << proms[cur_move.flag & is_promotion];
            std::cout << " ";
            MkMoveFast(cur_move);
            bool in_check = Attack(*king_coord[wtm], !wtm);
            if(!Legal(state[ply].from_coord, cur_move.to_coord, in_check))
                ans = false;
        }
    }
    else
    {
        for(; ply_cr < pv_len; ply_cr++)
        {
            auto cur_move = pv[cur_ply][ply_cr + 1];
            auto it = coords[wtm].begin();
            it = cur_move.piece_iterator;
            char piece_char = pc2chr[b[*it]];
            if(piece_char == 'K' && get_col(*it) == 4
                    && get_col(cur_move.to_coord) == 6)
                std::cout << "OO";
            else if(piece_char == 'K' && get_col(*it) == 4
                    && get_col(cur_move.to_coord) == 2)
                std::cout << "OOO";
            else if(piece_char != 'P')
            {
                std::cout << piece_char;
                FindAndPrintForAmbiguousMoves(cur_move);
                if(cur_move.flag & is_capture)
                    std::cout << 'x';
                std::cout << (char)(get_col(cur_move.to_coord) + 'a');
                std::cout << (char)(get_row(cur_move.to_coord) + '1');
            }
            else if(cur_move.flag & is_capture)
            {
                it = coords[wtm].begin();
                it = cur_move.piece_iterator;
                std::cout << (char)(get_col(*it) + 'a')
                          << "x"
                          << (char)(get_col(cur_move.to_coord) + 'a')
                          << (char)(get_row(cur_move.to_coord) + '1');
            }
            else
                std::cout << (char)(get_col(cur_move.to_coord) + 'a')
                          << (char)(get_row(cur_move.to_coord) + '1');
            char proms[] = "?QNRB";
            if(piece_char == 'P' && (cur_move.flag & is_promotion))
                std::cout << proms[cur_move.flag & is_promotion];
            std::cout << ' ';
            MkMoveFast(cur_move);
            bool in_check = Attack(*king_coord[wtm], !wtm);
            if(!Legal(state[ply].from_coord, cur_move.to_coord, in_check))
                ans = false;
        }
    }
    for(; ply_cr > 0; ply_cr--)
        UnMoveFast(*(move_c *) &pv[cur_ply][ply_cr]);
    return ans;
}





//-----------------------------
void k2engine::FindAndPrintForAmbiguousMoves(move_c move)
{
    move_c move_array[8];
    auto amb_cr = 0;
    auto it = coords[wtm].begin();
    it = move.piece_iterator;
    auto init_from_coord = *it;
    auto init_piece_type = get_index(b[init_from_coord]);

    for(it = coords[wtm].begin(); it != coords[wtm].end(); ++it)
    {
        if(it == move.piece_iterator)
            continue;
        auto from_coord = *it;

        auto piece_type = get_index(b[from_coord]);
        if(piece_type != init_piece_type)
            continue;
        if(!(attacks[120 + from_coord - move.to_coord] & (1 << piece_type)))
            continue;
        if(slider[piece_type] && !SliderAttack(from_coord, move.to_coord))
            continue;
        auto tmp = move;
        tmp.priority = from_coord;
        move_array[amb_cr++] = tmp;
    }

    if(!amb_cr)
        return;

    bool same_cols = false, same_rows = false;
    for(auto i = 0; i < amb_cr; i++)
    {
        if(get_col(move_array[i].priority) == get_col(init_from_coord))
            same_cols = true;
        if(get_row(move_array[i].priority) == get_row(init_from_coord))
            same_rows = true;
    }
    if(same_cols && same_rows)
        std::cout << (char)(get_col(init_from_coord) + 'a')
                  << (char)(get_row(init_from_coord) + '1');
    else if(same_cols)
        std::cout << (char)(get_row(init_from_coord) + '1');
    else
        std::cout << (char)(get_col(init_from_coord) + 'a');
}





//-----------------------------
bool k2engine::MakeMoveFinaly(char *move_str)
{
    auto ln = strlen(move_str);
    if(ln < 4 || ln > 5)
        return false;
    bool in_check = Attack(*king_coord[wtm], !wtm);
    max_root_moves = 0;
    RootMoveGen(in_check);

    char cur_move_str[6];
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    for(movcr_t i = 0; i < max_root_moves; ++i)
    {
        auto cur_move = root_moves.at(i).second;
        auto it = coords[wtm].begin();
        it = cur_move.piece_iterator;
        cur_move_str[0] = get_col(*it) + 'a';
        cur_move_str[1] = get_row(*it) + '1';
        cur_move_str[2] = get_col(cur_move.to_coord) + 'a';
        cur_move_str[3] = get_row(cur_move.to_coord) + '1';
        cur_move_str[4] = (cur_move.flag & is_promotion) ?
                          proms[cur_move.flag & is_promotion] : '\0';
        cur_move_str[5] = '\0';

        if(strcmp(move_str, cur_move_str) != 0)
            continue;

        it = coords[wtm].begin();
        it = cur_move.piece_iterator;

        MkMove(cur_move);
        FastEval(cur_move);

        auto store_val_opn = val_opn;
        auto store_val_end = val_end;

        memmove(&b_state[0], &b_state[1],
                (prev_states + 2)*sizeof(state_s));
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
        for(auto j = 0; j < FIFTY_MOVES; ++j)
            doneHashKeys[j] = doneHashKeys[j + 1];

        finaly_made_moves++;
        return true;
    }
    return false;
}





//--------------------------------
void k2engine::InitEngine()
{
    InitChess();
    InitEval();
    hash_key = InitHashKey();

    std::cin.rdbuf()->pubsetbuf(nullptr, 0);
    std::cout.rdbuf()->pubsetbuf(nullptr, 0);

    stop = false;
    total_nodes = 0;
    total_time_spent = 0;

    spent_exact_time = false;
    finaly_made_moves = 0;
    resign_cr = 0;
    time_spent = 0;

    memset(history, 0, sizeof(history));
}





//--------------------------------
bool k2engine::FenStringToEngine(char *fen)
{
    bool ans = FenToBoard(fen);

    if(!ans)
        return false;

    InitEvaOfMaterialAndPst();
    time_spent = 0;
    time_remains = time_base;
    finaly_made_moves = 0;
    hash_key = InitHashKey();

    initial_score = infinite_score;

    return true;
}





//--------------------------------
bool k2engine::DrawDetect()
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
void k2engine::CheckForInterrupt()
{
    if(max_nodes_to_search != 0)
    {
        if(nodes >= max_nodes_to_search - nodes_to_check_stop)
            stop = true;
    }
    if(infinite_analyze || pondering_in_process)
        return;

    const node_t nodes_to_check_stop2 = (16*(nodes_to_check_stop + 1) - 1);
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
    else if((nodes & nodes_to_check_stop2) == nodes_to_check_stop2)
    {
        double time1 = timer.getElapsedTimeInMicroSec();
        time_spent = time1 - time0;
        if(time_spent >= moves_for_time_exact_mode*time_to_think
                && root_ply > 2)
            stop = true;
    }

}





//--------------------------------------
void k2engine::MakeNullMove()
{
#ifndef NDEBUG
    strcpy(&cur_moves[5*ply], "NULL ");
    if((!stop_ply || root_ply == stop_ply) && strcmp(stop_str, cv) == 0)
        std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG

    state[ply + 1] = state[ply];
    state[ply].to_coord = is_null_move;
    state[ply + 1].ep = 0;

    ply++;

    hash_key ^= key_for_side_to_move;
    doneHashKeys[FIFTY_MOVES + ply - 1] = hash_key;

    wtm ^= white;
}





//--------------------------------------
void k2engine::UnMakeNullMove()
{
    wtm ^= white;
    cur_moves[5*ply] = '\0';
    ply--;
    hash_key ^= -1ULL;
}





//-----------------------------
bool k2engine::NullMove(depth_t depth, score_t beta, bool in_check)
{
    if(in_check || depth < 2
            || material[wtm] - pieces[wtm] < 3)
        return false;

    null_probe_cr++;

    if(state[ply - 1].to_coord == is_null_move
            && state[ply - 2].to_coord == is_null_move
      )
        return false;

    auto store_ep = state[ply].ep;
    auto store_to = state[ply].to_coord;
    auto store_rv = reversible_moves;
    reversible_moves = 0;

    MakeNullMove();
    if(store_ep)
        hash_key = InitHashKey();

    auto r = depth > 6 ? 3 : 2;

    auto x = -Search(depth - r - 1, -beta, -beta + 1, all_node);

    UnMakeNullMove();
    state[ply].to_coord = store_to;
    state[ply].ep = store_ep;
    reversible_moves = store_rv;

    if(store_ep)
        hash_key = InitHashKey();

    if(x >= beta)
        null_cut_cr++;

    return (x >= beta);
}





//-----------------------------
bool k2engine::Futility(depth_t depth, score_t beta)
{
    if(state[ply].captured_piece == 0
            && state[ply - 1].to_coord != is_null_move
      )
    {
        futility_probes++;

        auto margin = depth < 2 ? 185 : 255;
        auto score = ReturnEval(wtm);
        if(score > margin + beta)
        {
            futility_hits++;
            return true;
        }
    }
    return false;
}





//--------------------------------
bool k2engine::DrawByRepetition()
{
    if(reversible_moves < 4)
        return false;

    depth_t max_count;
    if(reversible_moves > ply + finaly_made_moves)
        max_count = ply + finaly_made_moves;
    else
        max_count = reversible_moves;

    // on case that GUI does not recognize 50 move rule
    if(max_count > FIFTY_MOVES + (depth_t)ply)
        max_count = FIFTY_MOVES + ply;

    for(auto i = 4; i <= max_count; i += 2)
    {
        if(hash_key == doneHashKeys[FIFTY_MOVES + ply - i])
            return true;
    }

    return false;
}





//--------------------------------
void k2engine::ShowFen()
{
    char whites[] = "KQRBNP";
    char blacks[] = "kqrbnp";

    for(int row = 7; row >= 0; --row)
    {
        auto blank_cr = 0;
        for(auto col = 0; col < 8; ++col)
        {
            auto piece = b[get_coord(col, row)];
            if(piece == empty_square)
            {
                blank_cr++;
                continue;
            }
            if(blank_cr != 0)
                std::cout << blank_cr;
            blank_cr = 0;
            if(piece & white)
                std::cout << whites[get_index(piece) - 1];
            else
                std::cout << blacks[get_index(piece) - 1];
        }
        if(blank_cr != 0)
            std::cout << blank_cr;
        if(row != 0)
            std::cout << "/";
    }
    std::cout << " " << (wtm ? 'w' : 'b') << " ";

    auto cstl = state[0].cstl;
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
void k2engine::ReHash(size_t size_mb)
{
    busy = true;
    hash_table.resize(size_mb);
    busy = false;
}





//--------------------------------
bool k2engine::HashProbe(depth_t depth, score_t *alpha,
        score_t beta, hash_entry_s **entry)
{
    *entry = hash_table.count(hash_key);
    if(*entry == nullptr || stop)
        return false;

    hash_probe_cr++;

    auto hbnd = (*entry)->bound_type;
    if((*entry)->depth >= depth)
    {
        auto hval = (*entry)->value;
        if(hval > mate_score && hval != infinite_score)
            hval += (*entry)->depth - ply;
        else if(hval < -mate_score && hval != -infinite_score)
            hval -= (*entry)->depth - ply;

        if(hbnd == exact_value
                //-alpha = beta for parent node
                || (hbnd == upper_bound && hval >= -*alpha)
                //-beta = alpha for parent node
                || (hbnd == lower_bound && hval <= -beta) )
        {
            hash_cut_cr++;
            pv[ply][0].flag = 0;
            *alpha = hval;
            return true;
        }// if(bnd
    }// if((*entry).depth >= depth

    if((*entry)->best_move.flag != not_a_move)
        hash_hit_cr++;

    return false;
}





//-----------------------------
bool k2engine::MoveIsPseudoLegal(move_c &move, bool stm)
{
    if(move.flag == not_a_move)
        return false;
    auto it = coords[stm].begin();
    for(; it != coords[stm].end(); ++it)
        if(it == move.piece_iterator)
            break;
    if(it == coords[stm].end())
        return false;
    it = move.piece_iterator;
    auto from_coord = *it;
    auto piece = b[move.to_coord];
    int delta_col = get_col(move.to_coord) - get_col(from_coord);
    int delta_row = get_row(move.to_coord) - get_row(from_coord);
    if(!delta_col && !delta_row)
        return false;
    if(!is_light(b[from_coord], stm))
        return false;
    if(to_black(b[from_coord]) != black_pawn
            && ((!is_dark(piece, stm) && (move.flag & is_capture))
                || (piece != empty_square && !(move.flag & is_capture))))
        return false;
    if(to_black(b[from_coord]) != black_pawn && (move.flag & is_promotion))
        return false;
    bool long_move;
    switch (to_black(b[from_coord]))
    {
    case black_pawn :
        if((move.flag & is_promotion)
                && ((stm && get_row(from_coord) != 6)
                    || (!stm && get_row(from_coord) != 1)))
            return false;
        if((move.flag & is_capture)
                && (std::abs(delta_col) != 1 || (stm && delta_row != 1)
                    || (!stm && delta_row != -1)
                    || (!(move.flag & is_en_passant)
                        && !is_dark(piece, stm))))
            return false;
        if((move.flag & is_en_passant)
                && (piece != empty_square
                    || state[ply].ep == 0))
            return false;
        if(get_row(move.to_coord) == (stm ? 7 : 0)
                && !(move.flag & is_promotion))
            return false;
        if(!(move.flag & is_capture))
        {
            if(!stm)
                delta_row = -delta_row;
            if(piece != empty_square || delta_col != 0
                    || (stm && delta_row <= 0))
                return false;
            long_move = (get_row(from_coord) == (stm ? 1 : 6));
            if(long_move ? delta_row > 2 : delta_row != 1)
                return false;
            if(long_move && delta_row == 2
                    && b[get_coord(get_col(from_coord),
                                   (get_row(from_coord) +
                                    get_row(move.to_coord))/2)]
                    != empty_square)
                return false;
        }

        break;
    case black_knight :
        move.flag &= is_capture;
        if(std::abs(delta_col) + std::abs(delta_row) != 3)
            return false;
        if(std::abs(delta_col) != 1 && std::abs(delta_row) != 1)
            return false;
        break;
    case black_bishop :
    case black_rook :
    case black_queen :
        move.flag &= is_capture;
        if(!(attacks[120 + move.to_coord - from_coord]
                & (1 << get_index(b[from_coord])))
                || !SliderAttack(move.to_coord, from_coord))
            return false;
        break;
    case black_king :
        if(!(move.flag & is_castle))
        {
            if((std::abs(delta_col) > 1 || std::abs(delta_row) > 1))
                return false;
            else
                return true;
        }
        if(std::abs(delta_col) != 2 || std::abs(delta_row) != 0)
            return false;
        if(get_col(from_coord) != 4 || get_row(from_coord) != stm ? 0 : 7)
            return false;
        if((state[ply].cstl &
                (move.flag >> 3 >> (2*stm))) == 0)
            return false;
        if(b[get_coord(get_col(move.to_coord), get_row(from_coord))]
                != empty_square)
            return false;
        if(b[get_coord((get_col(move.to_coord) + get_col(from_coord))/2,
                       get_row(from_coord))] != empty_square)
            return false;
        if((move.flag &is_castle_kingside)
                && to_black(b[get_coord(7, get_row(from_coord))])
                != black_rook)
            return false;
        if((move.flag &is_castle_queenside)
                && to_black(b[get_coord(0, get_row(from_coord))])
                != black_rook)
            return false;
        break;

    default :
        return false;
    }
    return true;
}





//--------------------------------
k2chess::move_c k2engine::Next(move_c *move_array, movcr_t cur_move,
                               movcr_t *max_moves, hash_entry_s *entry,
                               side_to_move_t stm, bool only_captures,
                               move_c prev_move)
{
    move_c ans;
    if(cur_move == 0)
    {
        if(entry == nullptr)
        {
            if(!only_captures)
                *max_moves = GenMoves(move_array, nullptr, apprice_all);
            else
                *max_moves = GenCaptures(move_array);

            if(*max_moves > 1
                    && move_array[0].priority > bad_captures
                    && move_array[0].priority == move_array[1].priority)
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
            auto mx_ = GenMoves(move_array, nullptr, not_apprice);
            auto i = 0;
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
                ans.priority = move_from_hash;
                return ans;
            }
            else
            {
                entry = nullptr;
                *max_moves = GenMoves(move_array, nullptr, apprice_all);
            }
        }
    }
    else if(cur_move == 1 && entry != nullptr)
    {
        *max_moves = GenMoves(move_array, &prev_move, apprice_all);
        if(*max_moves <= 1)
        {
            *max_moves = 0;
            return move_array[0];
        }
        auto i = 0;
        for(; i < *max_moves; i++)
            if(move_array[i].priority == move_from_hash)
            {
                std::swap(move_array[0], move_array[i]);
                break;
            }
        assert(i < *max_moves);
    }

    if(only_captures)  // already sorted
        return move_array[cur_move];


    auto max_score = 0;
    auto max_index = cur_move;

    for(auto i = cur_move; i < *max_moves; i++)
    {
        auto score = move_array[i].priority;
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
void k2engine::StoreResultInHash(depth_t depth, score_t init_alpha,
                                 score_t alpha, score_t beta, movcr_t legals,
                                 bool beta_cutoff, move_c best_move)
{
    if(stop)
        return;
    if(beta_cutoff)
    {
        CorrectHashScore(&beta, depth);
        hash_table.add(hash_key, -beta, best_move, depth,
                       lower_bound, finaly_made_moves/2, false, nodes);
    }
    else if(alpha > init_alpha && pv[ply][0].flag > 0)
    {
        CorrectHashScore(&alpha, depth);
        hash_table.add(hash_key, -alpha, pv[ply][1], depth,
                       exact_value, finaly_made_moves/2, false, nodes);
    }
    else if(alpha <= init_alpha)
    {
        CorrectHashScore(&init_alpha, depth);
        move_c no_move;
        no_move.flag = not_a_move;
        hash_table.add(hash_key, -init_alpha,
                       legals > 0 ? best_move : no_move, depth,
                       upper_bound, finaly_made_moves/2, false, nodes);
    }
}





//-----------------------------
void k2engine::ShowCurrentUciInfo()
{
    double t = timer.getElapsedTimeInMicroSec();

    std::cout << "info nodes " << nodes
              << " nps " << (i32)(1000000 * nodes / (t - time0 + 1));

    auto move = root_moves.at(root_move_cr).second;
    auto from_coord = state[1].from_coord;
    std::cout << " currmove "
              << (char)(get_col(from_coord) + 'a')
              << (char)(get_row(from_coord) + '1')
              << (char)(get_col(move.to_coord) + 'a')
              << (char)(get_row(move.to_coord) + '1');
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    if(move.flag & is_promotion)
        std::cout << proms[move.flag & is_promotion];

    std::cout << " currmovenumber " << root_move_cr + 1;
    std::cout << " hashfull ";

    size_t hash_size = 1000*hash_table.size() / hash_table.max_size();
    std::cout << hash_size << std::endl;
}





//-----------------------------
void k2engine::PonderHit()
{
    double time1 = timer.getElapsedTimeInMicroSec();
    time_spent = time1 - time0;
    if(time_spent >= 5*time_to_think)
        spent_exact_time = true;
    else
        pondering_in_process = false;
}





//-----------------------------
void k2engine::ClearHash()
{
    hash_table.clear();
}

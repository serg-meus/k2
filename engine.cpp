#include "engine.h"





k2engine::k2engine() :
    engine_version{"0.95"},
    debug_variation{""},
    debug_ply{}
{
    busy = false;
    uci = false;
    xboard = false;
    pondering_in_process = false;
    stop = false;
    enable_output = true;
    state = &eng_state[prev_states];
    seed = 0;

    time_control.max_search_depth = k2chess::max_ply;
    time_control.time_remains = 300000000;  // 5 min
    time_control.time_base = 300000000;
    time_control.time_inc = 0;
    time_control.moves_per_session = 0;
    time_control.max_nodes_to_search = 0;
    time_control.time_command_sent = false;
    time_control.spent_exact_time = false;
    time_control.moves_to_go = default_moves_to_go;
    time_control.infinite_analyze = false;
    time_control.time_spent = 0;
    time_control.total_time_spent = 0;
#ifndef NDEBUG
    randomness = false;
#else
    randomness = true;
#endif
    initial_score = 0;

    hash_key = InitHashKey();

    std::cin.rdbuf()->pubsetbuf(nullptr, 0);
    std::cout.rdbuf()->pubsetbuf(nullptr, 0);

    stop = false;
    stats.total_nodes = 0;
    halfmoves_made = 0;
    resign_cr = 0;

    memset(history, 0, sizeof(history));
    memset(eng_state, 0, sizeof(eng_state));
}





//--------------------------------
k2chess::eval_t k2engine::Search(depth_t depth, eval_t alpha, eval_t beta,
                                 const node_type_t node_type)
{
    bool in_check = IsInCheck();
    if(ply >= max_ply - 1 || DrawDetect(in_check))
    {
        pv[ply].length = 0;
        return 0;
    }
    state[ply].in_check = in_check;

    if(depth < 0)
        depth = 0;
    if(in_check)
        depth++;
    else if(IsRecapture())
        depth++;

    if(MateDistancePruning(alpha, &beta) ||
            FutilityPruning(depth, beta, in_check) ||
            NullMovePruning(depth, beta, in_check))
        return beta;

    eval_t x, initial_alpha = alpha;
    hash_entry_s *entry = nullptr;
    if(HashProbe(depth, &alpha, beta, &entry))
        return -alpha;

    move_c hash_best_move;
    bool hash_one_reply = false;
    if(entry == nullptr)
        hash_best_move.flag = not_a_move;
    else
    {
        hash_best_move = entry->best_move;
        hash_one_reply = entry->one_reply;
    }

    if(depth <= 0)
        return QSearch(alpha, beta);

    UpdateAttackTables();
    assert(in_check == (bool)(attacks[!wtm][king_coord(wtm)]));

    if((stats.nodes & nodes_to_check_stop) == nodes_to_check_stop &&
            CheckForInterrupt())
        return alpha;

    move_c move_array[move_array_size], cur_move;
    size_t move_cr = 0, max_moves = init_max_moves;
    for(; move_cr < max_moves; move_cr++)
    {
        cur_move = GetNextMove(move_array, move_cr, &max_moves,
                               hash_best_move, hash_one_reply,
                               all_moves, cur_move);
        if(max_moves <= 0)
            break;

        // one reply extension
        if(max_moves == 1 && depth >= one_reply_min_depth)
            depth++;

        // late move pruning
        if(depth <= lmp_min_depth && !cur_move.flag &&
                !in_check && !state[ply - 1].in_check &&
                node_type != pv_node && move_cr > lmp_max_move)
            break;

        MakeMove(cur_move);
        stats.nodes++;

#ifndef NDEBUG
        if((!debug_ply || root_ply == debug_ply) &&
                strcmp(debug_variation, cv) == 0)
            std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG

        FastEval(cur_move);

        const auto lmr = LateMoveReduction(depth, cur_move, in_check,
                                           move_cr, node_type);
        if(move_cr == 0)
            x = -Search(depth - 1, -beta, -alpha, -node_type);
        else if(beta > alpha + 1)
        {
            x = -Search(depth - 1 - lmr, -alpha - 1, -alpha, cut_node);
            if(x > alpha && !stop)
                x = -Search(depth - 1, -beta, -alpha, pv_node);
        }
        else
        {
            x = -Search(depth - 1 - lmr, -beta, -alpha, -node_type);
            if(lmr && x > alpha && !stop)
                x = -Search(depth - 1, -beta, -alpha, pv_node);
        }
        TakebackMove(cur_move);
        if(stop)
            return alpha;

        if(x >= beta)
        {
            StoreInHash(depth, beta, cur_move, lower_bound, max_moves == 1);
            UpdateStatistics(cur_move, depth, move_cr, entry);
            return beta;
        }
        else if(x > alpha)
        {
            alpha = x;
            StorePV(cur_move);
            StoreInHash(depth, alpha, cur_move, exact_value, max_moves == 1);
        }
    }
    if(move_cr == 0)
    {
        pv[ply].length = 0;
        return in_check ? -king_value + ply : 0;
    }
    else if(alpha == initial_alpha)
        StoreInHash(depth, initial_alpha,
                    max_moves == 1 ? cur_move : move_array[0],
                    upper_bound, max_moves == 1);

    return alpha;
}





//-----------------------------
k2chess::eval_t k2engine::QSearch(eval_t alpha, const eval_t beta)
{
    if(ply >= max_ply - 1)
        return 0;
    pv[ply].length = 0;

    if(material[0] + material[1] > qs_min_material_to_drop &&
            ReturnEval(wtm) > beta + qs_beta_exceed_to_drop)
        return beta;

    hash_entry_s *entry = nullptr;
    if(HashProbe(0, &alpha, beta, &entry))
        return -alpha;

    UpdateAttackTables();

    auto x = -Eval();
    if(x >= beta)
        return beta;
    else if(x > alpha)
        alpha = x;

    if((stats.nodes & nodes_to_check_stop) == nodes_to_check_stop &&
            CheckForInterrupt())
        return alpha;

    move_c move_array[move_array_size], no_move;
    no_move.flag = not_a_move;
    size_t move_cr = 0, max_moves = init_max_moves;
    for(; move_cr < max_moves; move_cr++)
    {
        move_c cur_move = GetNextMove(move_array, move_cr, &max_moves,
                                      no_move, false, captures_only, cur_move);
        if(max_moves <= 0 || cur_move.priority <= bad_captures ||
                DeltaPruning(alpha, cur_move))
            break;
        MakeMove(cur_move);
        stats.nodes++;
        stats.q_nodes++;
#ifndef NDEBUG
        if((!debug_ply || root_ply == debug_ply) &&
                strcmp(debug_variation, cv) == 0)
            std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG

        FastEval(cur_move);
        x = -QSearch(-beta, -alpha);
        TakebackMove(cur_move);
        if(stop)
            return alpha;

        if(x >= beta)
        {
            StoreInHash(0, beta, cur_move, lower_bound, false);
            stats.q_cut_cr++;
            if(move_cr <
                    sizeof(stats.q_cut_num_cr)/sizeof(*(stats.q_cut_num_cr)))
                stats.q_cut_num_cr[move_cr]++;
            return beta;
        }
        else if(x > alpha)
        {
            StoreInHash(0, alpha, cur_move, exact_value, false);
            alpha = x;
            StorePV(cur_move);
        }
    }
    return alpha;
}





//--------------------------------
void k2engine::Perft(const depth_t depth)
{
    move_c move_array[move_array_size];
    auto max_moves = GenMoves(move_array, false);
    for(size_t move_cr = 0; move_cr < max_moves; move_cr++)
    {
#ifndef NDEBUG
        node_t tmpCr;
        if(depth == time_control.max_search_depth)
            tmpCr = stats.nodes;
#endif
        auto cur_move = move_array[move_cr];
        k2chess::MakeMove(cur_move);
#ifndef NDEBUG
        if(strcmp(debug_variation, cv) == 0)
            std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG
        if(depth > 1)
        {
            k2chess::UpdateAttackTables(cur_move);
            Perft(depth - 1);
        }
        else if(depth == 1)
            stats.nodes++;
#ifndef NDEBUG
        if(depth == time_control.max_search_depth)
            std::cout << cv << stats.nodes - tmpCr << std::endl;
#endif
        k2chess::TakebackMove(cur_move);
        if(depth > 1)
            TakebackAttacks();
    }
}





//--------------------------------
bool k2engine::GetRootSearchBounds(const eval_t x, const eval_t prev_x,
                                   eval_t &alpha, eval_t &beta)
{
    if(stop)
        return false;

    if(alpha == beta)
    {
        alpha = AddEval(x, -aspiration_margin);
        beta = AddEval(x, aspiration_margin);
    }
    else if(x <= alpha)
    {
        if(beta != infinite_score)
            beta = AddEval(prev_x, aspiration_margin);
        alpha = -infinite_score;
    }
    else if(x >= beta)
    {
        if(alpha != -infinite_score)
            alpha = AddEval(prev_x, -aspiration_margin);
        beta = infinite_score;
    }
    else
        return false;

    assert(alpha < beta);
    return true;
}





//--------------------------------
void k2engine::MainSearch()
{
    busy = true;
    InitSearch();

    root_ply = 1;
    auto x = QSearch(-infinite_score, infinite_score);
    pv[0].length = 0;
    if(initial_score == infinite_score)
        initial_score = x;

    GenerateRootMoves();
    if(root_moves.size() > 0)
        pv[0].moves[0] = root_moves.at(0).second;

    for(; root_ply <= max_ply; ++root_ply)
    {
        const auto prev_x = x;
        eval_t alpha = 0, beta = 0;
        while(GetRootSearchBounds(x, prev_x, alpha, beta))
            x = RootSearch(root_ply, alpha, beta);
        if(stop)
            x = prev_x;

        const double time1 = time_control.timer.getElapsedTimeInMicroSec();
        time_control.time_spent = time1 - time_control.time0;

        if(stop || root_ply >= time_control.max_search_depth)
            break;
        if(!time_control.infinite_analyze && !pondering_in_process &&
                CanFinishMainSearch(x, prev_x))
            break;
    }

    if(pondering_in_process && time_control.spent_exact_time)
    {
        pondering_in_process = false;
        time_control.spent_exact_time = false;
    }
    CheckForResign(x);

    stats.total_nodes += stats.nodes;
    time_control.total_time_spent += time_control.time_spent;
    time_control.timer.stop();

    if(!time_control.infinite_analyze || (uci && enable_output))
        PrintFinalSearchResult();
    if(uci)
        time_control.infinite_analyze = false;

    busy = false;
}





//--------------------------------
k2chess::eval_t k2engine::RootSearch(const depth_t depth, eval_t alpha,
                                     const eval_t beta)
{
    const bool in_check = attacks[!wtm][king_coord(wtm)];
    bool beta_cutoff = false;
    const node_t unconfirmed_fail_high = -1;
    const node_t max_root_move_priority = std::numeric_limits<node_t>::max();
    eval_t x;
    move_c cur_move;
    for(root_move_cr = 0; root_move_cr < root_moves.size(); root_move_cr++)
    {
        cur_move = root_moves.at(root_move_cr).second;
        MakeMove(cur_move);
        stats.nodes++;
#ifndef NDEBUG
        if(strcmp(debug_variation, cv) == 0 &&
                (!debug_ply || root_ply == debug_ply))
            std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG

        FastEval(cur_move);

        auto prev_nodes = stats.nodes;
        bool fail_high = false;
        if(root_move_cr == 0)
        {
            x = -Search(depth - 1, -beta, -alpha, pv_node);
            if(uci && root_ply > min_depth_to_show_uci_info)
                ShowCurrentUciInfo();
            if(!stop && x <= alpha)
                ShowPVfailHighOrLow(cur_move, x, '?');
        }
        else
        {
            x = -Search(depth - 1, -alpha - 1, -alpha, cut_node);
            if(uci && root_ply > min_depth_to_show_uci_info)
                ShowCurrentUciInfo();
            if(!stop && x > alpha)
            {
                fail_high = true;
                ShowPVfailHighOrLow(cur_move, x, '!');

                const auto x_ = -Search(depth - 1, -beta, -alpha, pv_node);
                if(uci && root_ply > min_depth_to_show_uci_info)
                    ShowCurrentUciInfo();
                if(!stop)
                    x = x_;
                if(!stop && x >= beta)
                {
                    pv[0].moves[0] = cur_move;
                    pv[1].length = 1;
                }
                else
                    root_moves.at(root_move_cr).first = unconfirmed_fail_high;
            }
        }
        if(stop && !fail_high)
            x = -infinite_score;

        const auto delta_nodes = stats.nodes - prev_nodes;
        if(root_moves.at(root_move_cr).first != unconfirmed_fail_high)
            root_moves.at(root_move_cr).first = delta_nodes;
        else
            root_moves.at(root_move_cr).first = max_root_move_priority;

        if(x >= beta)
        {
            ShowPVfailHighOrLow(cur_move, x, '!');
            beta_cutoff = true;
            std::swap(root_moves.at(root_move_cr), root_moves.at(0));
            TakebackMove(cur_move);
            break;
        }
        else if(x > alpha)
        {
            alpha = x;
            TakebackMove(cur_move);
            StorePV(cur_move);
            if(x != -infinite_score && !stop)
                PrintCurrentSearchResult(x, ' ');
            std::swap(root_moves.at(root_move_cr), root_moves.at(0));
        }
        else
            TakebackMove(cur_move);

        if(stop)
            break;
    }
    if(root_moves.size() == 0)
    {
        pv[ply].length = 0;
        return in_check ? -king_value + ply : 0;
    }
    else
        std::stable_sort(root_moves.rbegin(), root_moves.rend() - 1);

    if(beta_cutoff)
    {
        UpdateStatistics(cur_move, depth, root_move_cr + 1, nullptr);
        return beta;
    }
    return alpha;
}





//-----------------------------
void k2engine::StorePV(const move_c move)
{
    const auto next_len = pv[ply + 1].length;
    pv[ply].length = next_len + 1;
    pv[ply].moves[0] = move;
    memcpy(&pv[ply].moves[1], &pv[ply + 1].moves[0], sizeof(move_c)*next_len);
}





//-----------------------------
void k2engine::UpdateStatistics(const move_c move, const depth_t depth,
                                const size_t move_cr,
                                const hash_entry_s *entry)
{
    if(entry != nullptr && entry->best_move.flag != not_a_move)
        stats.hash_best_move_cr++;
    if(move_cr == 0)
    {
        if(entry != nullptr && entry->best_move.flag != not_a_move)
            stats.hash_cutoff_by_best_move_cr++;
        else if(move.priority == first_killer)
            stats.killer1_hits++;
        else if(move.priority == second_killer)
            stats.killer2_hits++;
    }
    stats.cut_cr++;
    if(move_cr < sizeof(stats.cut_num_cr)/sizeof(*(stats.cut_num_cr)))
        stats.cut_num_cr[move_cr]++;
    if(move.priority == first_killer)
        stats.killer1_probes++;
    if(move.priority == second_killer)
        stats.killer2_probes++;

    if(move.flag)
        return;
    if(move != killers[ply][0] && move != killers[ply][1])
    {
        killers[ply][1] = killers[ply][0];
        killers[ply][0] = move;
    }

    auto &h = history[wtm][get_type(b[move.from_coord]) - 1][move.to_coord];
    h += depth*depth + 1;
}





//--------------------------------
void k2engine::ShowPVfailHighOrLow(const move_c move, const eval_t val,
                                   const u8 type_of_bound)
{
    TakebackMove(move);
    char mstr[6];
    MoveToCoordinateNotation(move, mstr);

    const auto tmp_length = pv[0].length;
    const auto tmp_move = pv[0].moves[0];
    pv[ply].length = 1;
    pv[0].moves[0] = move;

    PrintCurrentSearchResult(val, type_of_bound);
    pv[ply].length = tmp_length;
    pv[0].moves[0] = tmp_move;

    MakeMove(move);
    k2chess::UpdateAttackTables(move);
    FastEval(move);
}





//--------------------------------
void k2engine::GenerateRootMoves()
{
    move_c move_array[move_array_size], cur_move, no_move;
    no_move.flag = not_a_move;
    auto max_moves = init_max_moves;

    eval_t alpha = -infinite_score, beta = infinite_score;
    cur_move.flag = not_a_move;

    hash_entry_s *entry = nullptr;
    HashProbe(max_ply, &alpha, beta, &entry);

    for(size_t move_cr = 0; move_cr < max_moves; move_cr++)
    {
        cur_move = GetNextMove(move_array, move_cr, &max_moves,
                               no_move, false, all_moves, cur_move);
    }
    root_moves.clear();
    for(size_t move_cr = 0; move_cr < max_moves; move_cr++)
    {
        cur_move = move_array[move_cr];
        auto rms = &root_moves_to_search;
        if(rms->size() != 0 &&
                std::find(rms->begin(), rms->end(), cur_move) == rms->end())
            continue;
        root_moves.push_back(std::pair<node_t, move_c>(0, cur_move));
    }
    if(root_ply != 1)
        return;

    if(randomness && root_moves.size() >= max_moves_to_shuffle)
    {
        const auto phase1 = seed & 3;
        std::swap(root_moves[0], root_moves[phase1]);
        const auto phase2 = seed >> 2;
        for(auto i = 0; i < phase2; ++i)
        {
            const auto tmp = root_moves[0];
            for(size_t j = 0; j < max_moves_to_shuffle - 1; ++j)
            {
                root_moves[j] = root_moves[j + 1];
            }
            root_moves[max_moves_to_shuffle - 1] = tmp;
        }
    }
    pv[0].moves[0] = (*root_moves.begin()).second;
}




//--------------------------------
void k2engine::InitSearch()
{
    InitTime();
    time_control.timer.start();
    time_control.time0 = time_control.timer.getElapsedTimeInMicroSec();

    stats.nodes = 0;
    stats.q_nodes = 0;
    stats.cut_cr = 0;
    stats.q_cut_cr = 0;
    stats.futility_probes = 0;
    stats.null_cut_cr = 0;
    stats.null_probe_cr = 0;
    stats.hash_cut_cr = 0;
    stats.hash_probe_cr = 0;
    stats.hash_hit_cr = 0;
    stats.hash_cutoff_by_best_move_cr = 0;
    stats.hash_best_move_cr = 0;
    stats.futility_hits = 0;
    stats.killer1_probes = 0;
    stats.killer2_probes = 0;
    stats.killer1_hits = 0;
    stats.killer2_hits = 0;

    memset(stats.q_cut_num_cr, 0, sizeof(stats.cut_num_cr));
    memset(stats.cut_num_cr, 0, sizeof(stats.cut_num_cr));
    memset(pv, 0, sizeof(pv));
    memset(killers, 0, sizeof(killers));

    stop = false;

    if(enable_output && !uci && !xboard)
    {
        std::cout << "( time total = " << time_control.time_remains/1e6
                  << "s, assigned to move = "
                  << time_control.time_to_think/1e6
                  << "s, stop on timer = "
                  << (time_control.spent_exact_time ? "true " : "false ")
                  << " )" << std::endl;
        std::cout << "Ply Value  Time    Nodes        Principal Variation"
                  << std::endl;
    }

    memset(history, 0, sizeof(history));
    k2chess::state[ply].attacks_updated = true;
}





//--------------------------------
bool k2engine::CanFinishMainSearch(const eval_t x, const eval_t prev_x)
{
    if(time_control.time_spent > time_control.time_to_think &&
            time_control.max_nodes_to_search == 0
            && root_ply >= 2)
        return true;
    if(std::abs(x) > mate_score && std::abs(prev_x) > mate_score
            && !stop && root_ply >= max_depth_for_single_move)
    {
        ClearHash();
        return true;
    }
    if(root_moves.size() == 1 && root_ply >= max_depth_for_single_move
            && root_moves_to_search.empty())
        return true;
    return false;
}





//--------------------------------
void k2engine::CheckForResign(const eval_t x)
{
    if(x < initial_score - eval_to_resign || x < -mate_score)
        resign_cr++;
    else if(root_moves.size() == 1 && resign_cr > 0)
        resign_cr++;
    else
        resign_cr = 0;
    if(enable_output && !time_control.infinite_analyze
            && (x < -mate_score || resign_cr > moves_to_resign))
        std::cout << "resign" << std::endl;
}





//--------------------------------
void k2engine::PrintFinalSearchResult()
{
    char move_str[6];
    MoveToCoordinateNotation(pv[0].moves[0], move_str);

    if(!uci && !MakeMove(move_str))
        std::cout << "tellusererror err01"
                  << std::endl << "resign" << std::endl;
    if(!uci)
        std::cout << "move " << move_str << std::endl;
    else
    {
        std::cout << "bestmove " << move_str;
        if(!time_control.infinite_analyze)
        {
            char pndr[6] = "a1a1";
            if(pv[0].length > 1)
                MoveToCoordinateNotation(pv[0].moves[1], pndr);
            else
            {
                MakeMove(pv[0].moves[0]);
                hash_entry_s *entry = hash_table.count(hash_key);
                if(entry != nullptr)
                    MoveToCoordinateNotation(entry->best_move, pndr);
                else
                {
                    move_c move_array[move_array_size];
                    auto max_moves = GenMoves(move_array, true);
                    if(max_moves)
                        MoveToCoordinateNotation(move_array[0], pndr);
                }
                TakebackMove(pv[0].moves[0]);
            }
            std::cout << " ponder " << pndr;
        }
        std::cout << std::endl;
    }

    if(xboard || uci || !enable_output)
        return;

    std::cout << "( nodes = " << stats.nodes
              << ", cuts = [";
    if(stats.cut_cr == 0)
        stats.cut_cr = 1;
    for(size_t i = 0;
        i < sizeof(stats.cut_num_cr)/sizeof(*(stats.cut_num_cr));
        ++i)
        std::cout << std::setprecision(1) << std::fixed
                  << 100.*stats.cut_num_cr[i]/stats.cut_cr << " ";
    std::cout << "]% )" << std::endl;

    std::cout << "( q_nodes = " << stats.q_nodes
              << ", q_cuts = [";
    if(stats.q_cut_cr == 0)
        stats.q_cut_cr = 1;
    for(size_t i = 0;
        i < sizeof(stats.q_cut_num_cr)/sizeof(*(stats.q_cut_num_cr));
        ++i)
        std::cout << std::setprecision(1) << std::fixed
                  << 100.*stats.q_cut_num_cr[i]/stats.q_cut_cr << " ";
    std::cout << "]%, ";
    if(stats.nodes == 0)
        stats.nodes = 1;
    std::cout << "q/n = " << std::setprecision(1) << std::fixed
              << 100.*stats.q_nodes/stats.nodes
              << "% )" << std::endl;

    if(stats.hash_probe_cr == 0)
        stats.hash_probe_cr = 1;
    if(stats.hash_hit_cr == 0)
        stats.hash_hit_cr = 1;
    if(stats.hash_best_move_cr == 0)
        stats.hash_best_move_cr = 1;
    std::cout << "( hash probes = " << stats.hash_probe_cr
              << ", cuts by val = "
              << std::setprecision(1) << std::fixed
              << 100.*stats.hash_cut_cr/stats.hash_probe_cr << "%, "
              << "cuts by best move = "
              << 100.*stats.hash_cutoff_by_best_move_cr /
                 stats.hash_best_move_cr << "% )"
              << std::endl
              << "( hash full = "
              << (i32)100*hash_table.size()/hash_table.max_size()
              << "% (" << hash_table.size()/sizeof(hash_entry_s)
              << "/" << hash_table.max_size()/sizeof(hash_entry_s)
              << " entries )" << std::endl;

    if(stats.null_probe_cr == 0)
        stats.null_probe_cr = 1;
    std::cout << "( null move probes = " << stats.null_probe_cr
              << ", cutoffs = "
              << std::setprecision(1) << std::fixed
              << 100.*stats.null_cut_cr/stats.null_probe_cr << "% )"
              << std::endl;

    if(stats.futility_probes == 0)
        stats.futility_probes = 1;
    std::cout << "( futility probes = " << stats.futility_probes
              << ", hits = " << std::setprecision(1) << std::fixed
              << 100.*stats.futility_hits/stats.futility_probes
              << "% )" << std::endl;

    if(stats.killer1_probes == 0)
        stats.killer1_probes = 1;
    std::cout << "( killer1 probes = " << stats.killer1_probes
              << ", hits = " << std::setprecision(1) << std::fixed
              << 100.*stats.killer1_hits/stats.killer1_probes
              << "% )" << std::endl;
    if(stats.killer1_probes == 0)
        stats.killer1_probes = 1;
    std::cout << "( killer2 probes = " << stats.killer2_probes
              << ", hits = " << std::setprecision(1) << std::fixed
              << 100.*stats.killer2_hits/stats.killer2_probes
              << "% )" << std::endl;
    std::cout << "( time spent = " << time_control.time_spent/1.e6
              << "s )" << std::endl;
}





//--------------------------------
void k2engine::PrintCurrentSearchResult(const eval_t max_value,
                                        const u8 type_of_bound)
{
    using namespace std;

    if(!enable_output)
        return;

    const double time1 = time_control.timer.getElapsedTimeInMicroSec();
    const i32 spent_time = ((time1 - time_control.time0)/1000.);

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
             << " nodes " << stats.nodes
             << " pv ";
    }
    else
    {
        cout << setfill(' ') << setw(4) << left << root_ply;
        cout << setfill(' ') << setw(7) << left << max_value;
        cout << setfill(' ') << setw(8) << left << spent_time / 10;
        cout << setfill(' ') << setw(12) << left << stats.nodes << ' ';
    }
    if(enable_output)
        PrintMoveSequence(pv[0].moves, pv[0].length, uci);
    if(!uci && type_of_bound != ' ')
        cout << type_of_bound;
    cout << endl;
}





//-----------------------------
void k2engine::InitTime()
{
    if(!time_control.time_command_sent)
        time_control.time_remains -= time_control.time_spent;
    if(time_control.time_remains < 0)
        time_control.time_remains = 0;

    if(time_control.moves_per_session == 0)
        time_control.moves_to_go = default_moves_to_go;
    else if(uci)
        time_control.moves_to_go = time_control.moves_per_session;
    else
        time_control.moves_to_go = time_control.moves_per_session -
                (halfmoves_made/2 % time_control.moves_per_session);

    if(time_control.time_base == 0)
        time_control.moves_to_go = 1;

    if(time_control.moves_to_go <= moves_for_time_exact_mode ||
            (time_control.time_inc == 0 &&
             time_control.moves_per_session == 0 &&
             time_control.time_remains <
             time_control.time_base/exact_time_base_divider))
        time_control.spent_exact_time = true;


    if((uci && time_control.move_remains == 1) ||
            (!uci && time_control.moves_per_session != 0 &&
             (halfmoves_made/2 % time_control.moves_per_session) == 0))
    {
        if(!time_control.time_command_sent)
            time_control.time_remains += time_control.time_base;

        if(time_control.moves_to_go > min_moves_for_exact_time)
            time_control.spent_exact_time = false;
        if(time_control.moves_to_go > 1)
        {
            ClearHash();
            memset(killers, 0, sizeof(killers));
            memset(history, 0, sizeof(history));
#ifndef NDEBUG
            std::cout << "( ht cleared )\n";
#endif
        }
        time_control.move_remains = 0;
    }
#ifndef NDEBUG
    if(enable_output)
        std::cout << "( halfmoves=" << halfmoves_made <<
                     ", movestogo=" << (int)time_control.moves_to_go <<
                     ", movespersession=" << time_control.moves_per_session <<
                     ", exacttime=" << (int)time_control.spent_exact_time <<
                     " )\n";
#endif
    if(!time_control.time_command_sent)
        time_control.time_remains += time_control.time_inc;
    time_control.time_to_think =
            time_control.time_remains/time_control.moves_to_go;
    if(time_control.time_inc == 0 && time_control.moves_to_go != 1 &&
            !time_control.spent_exact_time)
        time_control.time_to_think /= time_to_think_divider;
    else if(time_control.time_inc != 0 && time_control.time_base != 0)
        time_control.time_to_think +=
                time_control.time_inc/increment_time_divider;

    time_control.time_command_sent = false;
    if(uci)
        time_control.move_remains =
                time_control.moves_per_session == 1 ? 1 : 0;
}




//-----------------------------
bool k2engine::MakeMove(const char *move_str)
{
    const auto ln = strlen(move_str);
    if(ln < 4 || ln > 5)
        return false;
    GenerateRootMoves();

    char cur_move_str[6];
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    for(size_t i = 0; i < root_moves.size(); ++i)
    {
        const auto cur_move = root_moves.at(i).second;
        cur_move_str[0] = get_col(cur_move.from_coord) + 'a';
        cur_move_str[1] = get_row(cur_move.from_coord) + '1';
        cur_move_str[2] = get_col(cur_move.to_coord) + 'a';
        cur_move_str[3] = get_row(cur_move.to_coord) + '1';
        cur_move_str[4] = (cur_move.flag & is_promotion) ?
                          proms[cur_move.flag & is_promotion] : '\0';
        cur_move_str[5] = '\0';

        if(strcmp(move_str, cur_move_str) != 0)
            continue;

        MakeMove(cur_move);
        k2chess::UpdateAttackTables(cur_move);
        FastEval(cur_move);

        const auto store_val_opn = val_opn;
        const auto store_val_end = val_end;

        memmove(&b_state[0], &b_state[1],
                (prev_states + 2)*sizeof(k2chess::state_s));
        memmove(&e_state[0], &e_state[1],
                (prev_states + 2)*sizeof(k2eval::state_s));
        memmove(&eng_state[0], &eng_state[1],
                (prev_states + 2)*sizeof(k2engine::state_s));
        ply--;
        InitEvalOfMaterialAndPst();
        if(enable_output && (val_opn != store_val_opn ||
                             val_end != store_val_end))
        {
            std::cout << "telluser err02: wrong score. Fast: "
                      << store_val_opn << '/' << store_val_end
                      << ", all: " << val_opn << '/' << val_end
                      << std::endl << "resign"
                      << std::endl;
        }
        for(auto j = 0; j < fifty_moves; ++j)
            done_hash_keys[j] = done_hash_keys[j + 1];

        halfmoves_made++;
        return true;
    }
    return false;
}





//--------------------------------
bool k2engine::SetupPosition(const char *fen)
{
    const bool ans = k2eval::SetupPosition(fen);

    if(!ans)
        return false;

    time_control.time_spent = 0;
    time_control.time_remains = time_control.time_base;
    halfmoves_made = 0;
    hash_key = InitHashKey();

    initial_score = infinite_score;
    resign_cr = 0;
    memset(eng_state, 0, sizeof(eng_state));

    return true;
}





//--------------------------------
bool k2engine::DrawDetect(const bool in_check)
{
    if(material[0] + material[1] == 0)
        return true;
    if(pieces[0] + pieces[1] == 3
            && (material[0]/centipawn == 4 || material[1]/centipawn == 4))
        return true;
    if(pieces[0] == 2 && pieces[1] == 2
            && material[0]/centipawn == 4 && material[1]/centipawn == 4)
        return true;

    if(reversible_moves == fifty_moves)
    {
        if(!in_check)
            return true;
        reversible_moves++;
        const auto x = Search(1, -infinite_score,
                              -mate_score + 1, cut_node);
        reversible_moves--;
        return x > -mate_score;
    }

    return DrawByRepetition();
}





//--------------------------------
bool k2engine::CheckForInterrupt()
{
    if(time_control.max_nodes_to_search != 0)
    {
        if(stats.nodes >=
                time_control.max_nodes_to_search - nodes_to_check_stop)
            stop = true;
            return true;
    }
    if(time_control.infinite_analyze ||
            (pondering_in_process && !time_control.spent_exact_time))
        return false;

    const node_t nodes_to_check_stop2 = (16*(nodes_to_check_stop + 1) - 1);
    if(time_control.spent_exact_time)
    {
        const double time1 = time_control.timer.getElapsedTimeInMicroSec();
        time_control.time_spent = time1 - time_control.time0;
        auto margin = search_stop_time_margin;
        if(time_control.moves_to_go == 1)
            margin *= 3;
        if(time_control.time_spent >= time_control.time_to_think - margin &&
                root_ply >= 3)
        {
            stop = true;
            return true;
        }
    }
    else if((stats.nodes & nodes_to_check_stop2) == nodes_to_check_stop2)
    {
        const double time1 = time_control.timer.getElapsedTimeInMicroSec();
        time_control.time_spent = time1 - time_control.time0;
        if(time_control.time_spent >=
                moves_for_time_exact_mode*time_control.time_to_think
                && root_ply > 2)
        {
            stop = true;
            return true;
        }
    }
    return false;
}





//--------------------------------------
void k2engine::MakeNullMove()
{
#ifndef NDEBUG
    strcpy(&cur_moves[5*ply], "NULL ");
    if((!debug_ply || root_ply == debug_ply) &&
            strcmp(debug_variation, cv) == 0)
        std::cout << "( breakpoint )" << std::endl;
#endif // NDEBUG

    k2chess::state[ply + 1] = k2chess::state[ply];
    k2eval::state[ply + 1] = k2eval::state[ply];
    k2engine::state[ply + 1] = k2engine::state[ply];
    k2chess::state[ply].move.to_coord = is_null_move;
    k2chess::state[ply + 1].en_passant_rights = 0;

    ply++;

    hash_key ^= key_for_side_to_move;
    done_hash_keys[fifty_moves + ply - 1] = hash_key;

    wtm ^= white;
}





//--------------------------------------
void k2engine::UnMakeNullMove()
{
    wtm ^= white;
    cur_moves[5*ply] = '\0';
    ply--;
    hash_key ^= key_for_side_to_move;
}





//-----------------------------
bool k2engine::NullMovePruning(const depth_t depth, const eval_t beta,
                               const bool in_check)
{
    if(in_check || depth < null_move_min_depth
            || material[wtm]/100 - pieces[wtm] < null_move_min_strength)
        return false;

    stats.null_probe_cr++;

    if(k2chess::state[ply - 1].move.to_coord == is_null_move
            && k2chess::state[ply - 2].move.to_coord == is_null_move)
        return false;

    const auto store_ep = k2chess::state[ply].en_passant_rights;
    const auto store_move = k2chess::state[ply].move;
    const auto store_rv = reversible_moves;
    reversible_moves = 0;

    MakeNullMove();
    if(store_ep)
        hash_key = InitHashKey();

    const auto r = depth > null_move_switch_r_depth ? null_move_max_r :
                                                      null_move_min_r;

    const auto x = -Search(depth - r - 1, -beta, -beta + 1, all_node);

    UnMakeNullMove();
    if(k2chess::state[ply + 1].attacks_updated)
        k2chess::state[ply].attacks_updated = true;
    k2chess::state[ply].move = store_move;
    k2chess::state[ply].en_passant_rights = store_ep;
    reversible_moves = store_rv;

    if(store_ep)
        hash_key = InitHashKey();
    if(x >= beta)
        stats.null_cut_cr++;
    return (x >= beta);
}





//--------------------------------
bool k2engine::DrawByRepetition() const
{
    if(reversible_moves < 4)
        return false;

    depth_t max_count;
    if(reversible_moves > ply + halfmoves_made)
        max_count = ply + halfmoves_made;
    else
        max_count = reversible_moves;

    // on case that GUI does not recognize 50 move rule
    if(max_count > fifty_moves + (depth_t)ply)
        max_count = fifty_moves + ply;

    for(auto i = 4; i <= max_count; i += 2)
    {
        if(hash_key == done_hash_keys[fifty_moves + ply - i])
            return true;
    }

    return false;
}





//--------------------------------
void k2engine::ShowFen() const
{
    char whites[] = "KQRBNP";
    char blacks[] = "kqrbnp";

    for(auto row = 7; row >= 0; --row)
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
                std::cout << whites[get_type(piece) - 1];
            else
                std::cout << blacks[get_type(piece) - 1];
        }
        if(blank_cr != 0)
            std::cout << blank_cr;
        if(row != 0)
            std::cout << "/";
    }
    std::cout << " " << (wtm ? 'w' : 'b') << " ";

    const auto cstl = k2chess::state[0].castling_rights;
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
    std::cout << " " << halfmoves_made/2 + 1;

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
bool k2engine::HashProbe(const depth_t depth, eval_t * const alpha,
                         const eval_t beta, hash_entry_s **entry)
{
    *entry = hash_table.count(hash_key);
    if(*entry == nullptr || stop)
        return false;

    stats.hash_probe_cr++;

    if((*entry)->best_move.flag != not_a_move)
        stats.hash_hit_cr++;

    const auto hbnd = (*entry)->bound_type;
    if((*entry)->depth < depth)
        return false;

    auto hval = (*entry)->value;
    if(hval > mate_score && hval != infinite_score)
        hval += (*entry)->depth - ply;
    else if(hval < -mate_score && hval != -infinite_score)
        hval -= (*entry)->depth - ply;

    if(hbnd == exact_value
            || (hbnd == upper_bound && hval >= -*alpha)
            || (hbnd == lower_bound && hval <= -beta) )
    {
        stats.hash_cut_cr++;
        pv[ply].length = 0;
        *alpha = hval;
        return true;
    }
    return false;
}





//--------------------------------
bool k2engine::GetFirstMove(move_c * const move_array,
                            size_t * const max_moves,
                            const move_c hash_best_move,
                            const bool hash_one_reply,
                            const bool only_captures,
                            move_c * const ans) const
{
    if(hash_best_move.flag == not_a_move)
    {
        if(!only_captures)
            *max_moves = GenMoves(move_array, false);
        else
            *max_moves = GenMoves(move_array, true);

        AppriceMoves(move_array, *max_moves, hash_best_move);

        if(*max_moves > 1
                && move_array[0].priority > bad_captures
                && move_array[0].priority == move_array[1].priority)
        {
            if(StaticExchangeEval(move_array[0]) <
                    StaticExchangeEval(move_array[1]))
                std::swap(move_array[0], move_array[1]);
        }
    }
    else
    {
        *ans = hash_best_move;
        const bool legal = IsPseudoLegal(*ans) && IsLegal(*ans);
#ifndef NDEBUG
        const auto mx_ = GenMoves(move_array, false);
        size_t i = 0;
        for(; i < mx_; ++i)
            if(move_array[i] == *ans)
                break;
        const bool tt_move_found = i < mx_;
        if(tt_move_found != legal)
        {
            IsPseudoLegal(*ans);
            IsLegal(*ans);
            GenMoves(move_array, false);
        }
        assert(tt_move_found == legal);
#endif
        if(legal)
        {
            ans->priority = move_from_hash;
            if(hash_one_reply)
                *max_moves = 1;
            return true;
        }
        else
        {
            *max_moves = GenMoves(move_array, false);
            AppriceMoves(move_array, *max_moves, hash_best_move);
        }
    }
    return false;
}




//--------------------------------
bool k2engine::GetSecondMove(move_c * const move_array,
                             size_t * const max_moves,
                             const move_c prev_move,
                             move_c * const ans) const
{
    *max_moves = GenMoves(move_array, false);
    AppriceMoves(move_array, *max_moves, prev_move);

    if(*max_moves <= 1)
    {
        *max_moves = 0;
        *ans = move_array[0];
        assert(IsPseudoLegal(*ans));
        return true;
    }
    size_t i = 0;
    for(; i < *max_moves; i++)
        if(move_array[i].priority == move_from_hash)
        {
            std::swap(move_array[0], move_array[i]);
            break;
        }
    assert(i < *max_moves);
    return false;
}





//--------------------------------
size_t k2engine::FindMaxMoveIndex(move_c * const move_array,
                                  const size_t max_moves,
                                  const size_t cur_move) const
{
    auto max_score = 0;
    auto max_index = cur_move;
    for(auto i = cur_move; i < max_moves; i++)
    {
        const auto score = move_array[i].priority;
        if(score > max_score)
        {
            max_score = score;
            max_index = i;
        }
    }
    return max_index;
}





//--------------------------------
k2chess::move_c k2engine::GetNextMove(move_c * const move_array,
                                      const size_t cur_move_cr,
                                      size_t * const max_moves,
                                      move_c hash_best_move,
                                      const bool hash_one_reply,
                                      const bool only_captures,
                                      const move_c prev_move) const
{
    move_c ans;
    if(cur_move_cr == 0)
    {
        if(GetFirstMove(move_array, max_moves, hash_best_move,
                        hash_one_reply, only_captures, &ans))
            return ans;
    }
    else if(cur_move_cr == 1 && hash_best_move.flag != not_a_move)
    {
        if(GetSecondMove(move_array, max_moves, prev_move, &ans))
            return ans;
    }
    auto max_index = FindMaxMoveIndex(move_array, *max_moves, cur_move_cr);
    std::swap(move_array[max_index], move_array[cur_move_cr]);
    assert(*max_moves == 0 || IsPseudoLegal(move_array[cur_move_cr]));
    return move_array[cur_move_cr];
}





//-----------------------------
void k2engine::StoreInHash(const depth_t depth, eval_t score,
                           const move_c best_move,
                           const hbound_t bound_type,
                           const bool one_reply)
{
    if(stop)
        return;
    CorrectHashScore(&score, depth);
    hash_table.add(hash_key, -score, best_move, depth,
                   bound_type, halfmoves_made/2, one_reply, stats.nodes);
    assert(IsPseudoLegal(best_move));
}





//-----------------------------
void k2engine::ShowCurrentUciInfo()
{
    if(!enable_output)
        return;
    const double t = time_control.timer.getElapsedTimeInMicroSec();

    std::cout << "info nodes " << stats.nodes
              << " nps " <<
                 (i32)(1000000 * stats.nodes / (t - time_control.time0 + 1));

    const auto move = root_moves.at(root_move_cr).second;
    char move_str[6];
    MoveToCoordinateNotation(move, move_str);
    std::cout << " currmove " << move_str;
    std::cout << " currmovenumber " << root_move_cr + 1;
    std::cout << " hashfull ";

    size_t hash_size = 1000.*hash_table.size() / hash_table.max_size();
    std::cout << hash_size << std::endl;
}





//-----------------------------
void k2engine::PonderHit()
{
    const double time1 = time_control.timer.getElapsedTimeInMicroSec();
    time_control.time_spent = time1 - time_control.time0;
    if(time_control.time_spent >=
            ponder_time_factor*time_control.time_to_think)
    {
        time_control.spent_exact_time = true;
        if(enable_output)
            std::cout << "( ponderhit: need to exit immidiately)\n";
    }
    else
    {
        pondering_in_process = false;
        if(enable_output)
            std::cout << "( ponderhit: " << std::setprecision(1) <<
                         std::fixed << time_control.time_to_think << ")\n";
    }
}





//-----------------------------
void k2engine::ClearHash()
{
    hash_table.clear();
}





//-----------------------------
bool k2engine::IsInCheck()
{
    if(k2chess::state[ply - 1].move.to_coord == is_null_move)
        return false;
    bool ans = false;
    const auto k_coord = king_coord(wtm);
    const auto move = k2chess::state[ply].move;
    const auto d_row = get_row(k_coord) - get_row(move.to_coord);
    const auto ac = std::abs(get_col(k_coord) - get_col(move.to_coord));
    const auto ar = std::abs(d_row);
    const auto type = get_type(b[move.to_coord]);
    if((move.flag & is_promotion) && is_slider[type])
    {
        bool correct = type == queen ||
                (type == rook && sgn(ac) + sgn(ar) == 1) ||
                (type == bishop && sgn(ac) + sgn(ar) == 2);
        if(correct && IsSliderAttack(move.to_coord, k_coord))
            return true;
    }
    switch(type)
    {
        case pawn :
            ans = ((wtm && d_row == -1) || (!wtm && d_row == 1)) && ac == 1;
            break;
        case knight :
            ans = (ac == 2 && ar == 1) || (ac == 1 && ar == 2);
            break;
        case bishop :
            ans = ac == ar && IsSliderAttack(k_coord, move.to_coord);
            break;
        case rook :
            ans = (ac == 0 || ar == 0) && IsSliderAttack(k_coord,
                                                         move.to_coord);
            break;
        case queen :
            ans = (ac == ar || ac == 0 || ar == 0) &&
                    IsSliderAttack(k_coord, move.to_coord);
            break;
        case king :
            if(move.flag & is_castle)
            {
                const auto d_col = (move.flag & is_left_castle) ? -1 : 1;
                const auto r_col = default_king_col + d_col;
                const auto ok_coord = king_coord(!wtm);
                if(r_col == get_col(k_coord) &&
                        IsSliderAttack(ok_coord - d_col, k_coord))
                    return true;
            }
            break;
        default :
            assert(true);
    }
    if(ans)
        return true;

    if((move.flag & (is_en_passant | is_promotion)) &&
            IsDiscoveredEnPassant(wtm, move))
        return true;
    else
    {
        const auto d_col2 = get_col(k_coord) - get_col(move.from_coord);
        const auto d_row2 = get_row(k_coord) - get_row(move.from_coord);
        if((std::abs(d_col2) == std::abs(d_row2) ||
             d_col2 == 0 || d_row2 == 0) && IsDiscoveredAttack(move))
        return true;
    }

    return false;
}

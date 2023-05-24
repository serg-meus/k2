#include "engine.h"

using eval_t = eval::eval_t;
using move_s = chess::move_s;


int engine::search(const int depth_orig, const int alpha_orig, const int beta,
                   const int node_type) {
    move_s cur_move, tt_move = not_a_move;
    int val = 0, alpha = alpha_orig;
    if (tt_probe(depth_orig, alpha, beta, tt_move, node_type))
        return alpha;
    const bool in_check = is_in_check(side);
    int depth = (in_check && depth_orig >= 0) ? depth_orig + 1 : depth_orig;
    std::vector<move_s> moves;
    gen_stage stage = gen_stage::init;
    unsigned best_move_num = 0, move_num = unsigned(-1), legal_moves = 0;
    if (depth <= 0) {
        val = Eval();
    if(val >= beta)
        return beta;
    else if(val > alpha)
        alpha = val;
    }
    while (!stop && (cur_move = next_move(moves, tt_move, move_num,
                                          stage, depth)) != not_a_move) {
        make_move(cur_move);
        if (!was_legal(cur_move)) {
            unmake_move();
            continue;
        }
        legal_moves++;
        val = search_cur_pos(depth, alpha, beta, cur_move,
                             move_num, node_type, in_check);
        unmake_move();
        if (val >= beta || val > alpha) {
            best_move_num = move_num;
            if (val >= beta) {
                update_cutoff_stats(depth, cur_move);
                break;
            }
            alpha = val;
        }
    }
    move_s best_move = moves.size() ? moves.at(best_move_num) : not_a_move;
    return search_result(val, alpha_orig, alpha, beta, depth, depth_orig,
        best_move, legal_moves, in_check);
}


int engine::search_cur_pos(const int depth, const int alpha, const int beta,
                           const move_s cur_move, const unsigned move_num,
                           const int node_type, const bool in_check) {
    if ((max_nodes != 0 && nodes >= max_nodes) ||
            (max_nodes == 0 && time_elapsed() > max_time_for_move)) {
        stop = true;
        return 0;
    }
    if (depth <= 0)
        return -search(depth - 1, -beta, -alpha, -node_type);
    if (search_draw() || hash_keys.find(hash_key) != hash_keys.end())
        return search_result(0, 0, 0, 0, depth, depth, not_a_move, 1, false);
    int val;
    const int lmr = late_move_reduction(depth, cur_move, in_check,
                                         move_num, node_type);
    if(move_num == 0)
        val = -search(depth - 1, -beta, -alpha, -node_type);
    else if(beta > alpha + 1)
    {
        val = -search(depth - 1 - lmr, -alpha - 1, -alpha, cut_node);
        if(val > alpha)
            val = -search(depth - 1, -beta, -alpha, pv_node);
    }
    else
    {
        val = -search(depth - 1 - lmr, -alpha - 1, -alpha, -node_type);
        if(lmr && val > alpha)
            val = -search(depth - 1, -alpha - 1, -alpha, cut_node);
    }
    return val;
}


bool engine::tt_probe(const int depth, int &alpha, const int beta,
                      move_s &tt_move, const int node_type) {
    const tt_entry_c *entry = tt.find(hash_key, u32(hash_key >> 32));
    if (entry == nullptr)
        return false;
    tt_move = entry->result.best_move;
    if (entry->depth < depth && depth > 0)
        return false;
    const auto bnd = tt_bound(entry->result.bound_type);
    const auto val = entry->result.value;
    if ((bnd == tt_bound::exact && node_type != pv_node) ||
            (bnd == tt_bound::lower && val <= alpha) ||
            (bnd == tt_bound::upper && val >= beta) ) {
        alpha = val;
        return true;
    }
    return false;
}


int engine::search_result(const int val, const int alpha_orig,
                          const int alpha, const int beta, const int depth,
                          const int depth_orig, move_s best_move,
                          const unsigned legal_moves, const bool in_check) {
    if (stop)
        return 0;
    int x;
    tt_bound bound_type;
    if (!legal_moves && depth > 0) {
        x = in_check ? -material[king_ix] + int(ply) : 0;
        bound_type = tt_bound::exact;
        best_move = not_a_move;
    }
    else {
        x = val >= beta ? beta : (alpha > alpha_orig ? alpha : alpha_orig);
        bound_type = val >= beta ? tt_bound::upper :
            (alpha > alpha_orig ? tt_bound::exact : tt_bound::lower);
    }
    tt_entry_c entry(u32(hash_key), best_move, i16(x),
                     u8(bound_type), false, i8(depth_orig));
    tt.add(entry, u32(hash_key >> 32));
    return x;
}


move_s engine::next_move(std::vector<move_s> &moves,
                         move_s &tt_move, unsigned &move_num, gen_stage &stage,
                         const int depth) const {
    ++move_num;
    if (stage == gen_stage::init) {
        stage = gen_stage::tt;
        if (tt_move != not_a_move && is_pseudo_legal(tt_move) &&
                (depth > 0 || tt_move.is_capture)) {
            tt_move.priority = 255;
            moves.push_back(tt_move);
            return tt_move;
        }
    }
    if (stage == gen_stage::tt) {
        stage = gen_stage::captures;
        gen_pseudo_legal_moves(moves, gen_mode::only_captures);
        if (tt_move != not_a_move && (tt_move.is_capture || tt_move.promo) &&
                move_num == 1)
            erase_move(moves, tt_move, 1);
        apprice_and_sort_moves(moves, move_num);
        if (move_num < moves.size()) {
            return moves.at(move_num);
        }
    }
    if (stage == gen_stage::captures) {
        if (move_num < moves.size()) {
            return moves.at(move_num);
        }
        if (depth <= 0)
            return not_a_move;
        stage = gen_stage::killer1;
    }
    if (stage == gen_stage::killer1) {
        stage = gen_stage::killer2;
        move_s k1 = killers.at(ply).at(0);
        if (k1 != tt_move && is_pseudo_legal(k1)) {
            k1.priority = 254;
            moves.push_back(k1);
            return k1;
        }
    }
    if (stage == gen_stage::killer2) {
        stage = gen_stage::silent_gen;
        move_s k2 = killers.at(ply).at(1);
        if (k2 != tt_move && is_pseudo_legal(k2)) {
            k2.priority = 253;
            moves.push_back(k2);
            return k2;
        }
    }
    if (stage == gen_stage::silent_gen) {
        stage = gen_stage::silent_probe;
        gen_pseudo_legal_moves(moves, gen_mode::only_silent);
        if (tt_move != not_a_move && !tt_move.is_capture && !tt_move.promo)
            erase_move(moves, tt_move, move_num);
        erase_move(moves, killers.at(ply).at(0), move_num);
        erase_move(moves, killers.at(ply).at(1), move_num);
    }
    if (stage == gen_stage::silent_probe) {
        if (move_num < moves.size())
            return moves.at(move_num);
    }
    return not_a_move;
}


void engine::erase_move(std::vector<move_s> &moves, const move_s move,
                        const unsigned first_ix) const {
    auto it = std::find(moves.begin() + first_ix, moves.end(), move);
    if (it == moves.end())
        return;
    moves.erase(it);
}


void engine::apprice_and_sort_moves(std::vector<move_s> &moves,
                                    unsigned first_move_num) const {
    if (moves.size() == 0)
        return;
    for (auto i = first_move_num; i < moves.size(); ++i)
        apprice_move(moves.at(i));
    std::sort(moves.begin() + int(first_move_num), moves.end());
}


void engine::apprice_move(move_s &move) const {
    if (!move.is_capture && !move.promo) {
        move.priority = 64;
        return;
    }
    int see = static_exchange_eval(move);
    int ans = see/20 + (see > 0 ? 200 : 64);
    if (see < -material[king_ix]/2)
        ans = 0;
    assert(u8(ans) >= 200 || u8(ans) <= 64);
    move.priority = u8(ans);
}


u64 engine::perft(const int depth, const bool verbose) {
    u64 tt_nodes = tt_probe_perft(depth);
    if (tt_nodes != 0 && tt_nodes != u64(-1))
        return tt_nodes;
    u64 perft_nodes = 0;
    std::vector<move_s> moves;
    moves.reserve(48);
    gen_pseudo_legal_moves(moves);
    for(auto move : moves) {
        make_move(move);
        if(!was_legal(move)) {
            unmake_move();
            continue;
        }
        u64 delta_nodes = (depth == 1) ? 1 : perft(depth - 1, false);
        perft_nodes += delta_nodes;
        unmake_move();
        if(verbose)
            std::cout << move_to_str(move) << ": " << delta_nodes << std::endl;
    }
    if (depth > 1 && tt_nodes != u64(-1)) {
        tt_entry_c entry(u32(hash_key), perft_nodes, i8(depth));
        tt.add(entry, u32(hash_key >> 32));
    }
    return perft_nodes;
}


u64 engine::tt_probe_perft(const int depth) {
    u64 ans = 0;
    if (depth > 1) {
        auto *node = tt.find(hash_key, u32(hash_key >> 32));
        if (node != nullptr) {
            if(node->depth == depth)
                return u64(node->nodes);
            ans = u64(-1);
        }
    }
    return ans;
}


eval_t engine::static_exchange_eval(const move_s move) const {
    std::array<int, 30> see_vals;
    const auto to_bb = one_nth_bit(move.to_coord);
    u8 ix = find_index(!side, to_bb);
    if (ix == u8(-1) && move.index == pawn_ix &&
            get_col(move.from_coord) != get_col(move.to_coord))
        ix = pawn_ix;
    auto val = ix != u8(-1) ? material.at(ix) : 0;
    see_vals[0] = val;
    auto occ = (bb[0][occupancy_ix] | bb[1][occupancy_ix] | to_bb) &
        ~one_nth_bit(move.from_coord);
    bool color = side;
    unsigned depth = 1;
    u64 attacker_bb;
    eval_t mat = 0, new_mat = material.at(move.index);
    for (; depth < 30; ++depth) {
        color = !color;
        const u8 attacker_ix = min_attacker(move.to_coord, occ, color,
                                            attacker_bb);
        if (attacker_ix == u8(-1))
            break;
        mat = new_mat;
        new_mat = material.at(attacker_ix);
        val = eval_t(depth % 2 ? val - mat : val + mat);
        see_vals.at(depth) = depth % 2 ? -val : val;
        occ ^= lower_bit(attacker_bb);
    }
    for(--depth; depth != 0; depth--) {
        auto new_val = std::max(-see_vals.at(depth - 1), see_vals.at(depth));
        see_vals[depth - 1] = -new_val;
    }
    return eval_t(see_vals[0]);
}


std::string engine::pv_string(int dpt) {
    std::string str_out;
    int i = 0;
    for(;i < dpt; ++i) {
        const tt_entry_c *entry = tt.find(hash_key, u32(hash_key >> 32));
        const auto &bmov = entry->result.best_move;
        if (entry == nullptr || bmov == not_a_move || !is_pseudo_legal(bmov))
            break;
        auto move_str = move_to_str(bmov);
        make_move(bmov);
        if (!was_legal(bmov)) {
            unmake_move();
            break;
        }
        str_out += move_str + " ";
    }
    for(int j = 0; j < i; ++j)
        unmake_move();
    return str_out;
}


void engine::update_cutoff_stats(const int depth, const move_s move) {
    if (depth <= 0 || move.is_capture || move.promo ||
            move == killers.at(ply).at(0))
        return;
    if (move == killers.at(ply).at(1))
        std::swap(killers.at(ply).at(0), killers.at(ply).at(1));
    else {
        killers.at(ply).at(1) = killers.at(ply).at(0);
        killers.at(ply).at(0) = move;
    }
    assert(killers.at(ply).at(0) != killers.at(ply).at(1));
}

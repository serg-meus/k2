#include "engine.h"

using eval_t = eval::eval_t;


engine::engine() : tt(64*megabyte) {
}


u64 engine::perft(const u8 depth, const bool verbose) {
    u64 tt_nodes = tt_probe_perft(depth);
    if (tt_nodes != 0 && tt_nodes != u64(-1))
        return tt_nodes;
    u64 nodes = 0;
    std::vector<move_s> moves;
    moves.reserve(48);
    gen_pseudo_legal_moves(moves);
    for(auto move : moves) {
        make_move(move);
        if(!was_legal(move)) {
            unmake_move();
            continue;
        }
        u64 delta_nodes = (depth == 1) ? 1 : perft(u8(depth - 1), false);
        nodes += delta_nodes;
        if(verbose)
            std::cout << move_to_str(move) << ": " << delta_nodes << std::endl;
        unmake_move();
    }
	if (depth > 1 && tt_nodes != u64(-1)) {
        tt_entry_c entry(u32(hash_key), nodes, depth);
		tt.add(entry, u32(hash_key >> 32));
	}
    return nodes;
}


u64 engine::tt_probe_perft(const u8 depth) {
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


eval_t engine::static_exchange_eval(const move_s move) {
    std::array<int, 30> see_vals;
    const auto to_bb = one_nth_bit(move.to_coord);
    auto val = material.at(find_index(!side, to_bb));
    see_vals[0] = val;
    auto occ = (bb[0][occupancy_ix] | bb[1][occupancy_ix] | to_bb) &
        ~one_nth_bit(move.from_coord);
    bool color = side;
    unsigned depth = 1;
    u64 attacker_bb;
    eval_t mat = 0, new_mat = material.at(move.index);
    for (; depth < 30; ++depth) {
        color = !color;
        const u8 attacker_ix = min_attacker(move.to_coord, occ, color, attacker_bb);
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


u8 engine::min_attacker(const u8 to_coord, const u64 occ, const bool color,
                        u64 &attacker_bb) {
    attacker_bb = all_pawn_attacks(one_nth_bit(to_coord), !color, u64(-1))
        & bb[color][pawn_ix] & occ;
    if (attacker_bb)
        return pawn_ix;
    attacker_bb = knight_attacks(to_coord) & bb[color][knight_ix] & occ;
    if (attacker_bb)
        return knight_ix;
    const u64 b_att = bishop_attacks(to_coord, occ);
    attacker_bb = b_att & bb[color][bishop_ix] & occ;
    if (attacker_bb)
        return bishop_ix;
    const u64 r_att = rook_attacks(to_coord, occ);
    attacker_bb = r_att & bb[color][rook_ix] & occ;
    if (attacker_bb)
        return rook_ix;
    attacker_bb = (b_att | r_att) & bb[color][queen_ix] & occ;
    if (attacker_bb)
        return queen_ix;
    attacker_bb = king_attacks(to_coord) & bb[color][king_ix] & occ;
    if (attacker_bb)
        return king_ix;
    return u8(-1);
}

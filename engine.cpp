#include "engine.h"

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
#include "eval.h"
#include "transposition_table.h"
#include <chrono>
#include <set>


class engine : public eval {

    public:

    const int all_node = -1, pv_node = 0, cut_node = 1;
    static const i8 max_ply = 126;

    const double infinity = std::numeric_limits<double>::infinity();

    u64 nodes, max_nodes;
    double max_time_for_move;
    int move_cr;
    unsigned ply;
    bool stop;

    engine() : nodes(0), max_nodes(0), max_time_for_move(0), move_cr(0),
            ply(0), stop(false), done_moves(), t_beg(), tt(64*megabyte),
            hash_keys(), killers(), history(), pv(), follow_pv(0),
            stages(), cv() {
        for (auto i = 0; i <= max_ply; ++i) {
            std::vector<move_s> tmp;
            tmp.reserve(max_ply);
            pv.push_back(tmp);
        }
        stages = {&engine::gen_pv, &engine::gen_tt, &engine::gen_cap,
                  &engine::probe_cap, &engine::gen_killer1,
                  &engine::gen_killer2, &engine::gen_silent,
                  &engine::probe_rest};
    }

    engine(const engine&);

    u64 perft(const int depth, const bool verbose);

    protected:

    std::vector<move_s> done_moves;
    const move_s not_a_move = {0, 0, 0, 0};

    enum class tt_bound {exact = 0, lower = 1, upper = 2};

    typedef std::chrono::time_point<std::chrono::high_resolution_clock>
        time_point_t;

    struct tt_entry_result_c {
        move_s best_move;
        i16 value;
        u8 bound_type;
        bool one_reply;
    };

    static_assert(sizeof(tt_entry_result_c) == 8, "Wrong entry size");

    struct tt_entry_c {
        union {
            u64 nodes;
            tt_entry_result_c result;
            };
        u32 key;
        i8 depth;

        tt_entry_c() : nodes(0), key(0), depth(0) {}

        tt_entry_c(u64 inkey) : nodes(0), key(0), depth(0)
        {
            key = u32(inkey);
        }

        tt_entry_c(const u32 k, const u64 n, const i8 d) :
            nodes(0), key(0), depth(0)
        {
            this->key = k;
            this->nodes = n;
            this->depth = d;
        }

        tt_entry_c(const u32 k, const move_s bm, const i16 val,
                   const u8 bnd, const bool onr, const i8 d) :
                   nodes(0), key(0), depth(0)
        {
            this->key = k;
            this->result.best_move = bm;
            this->result.value = val;
            this->result.bound_type = bnd;
            this->result.one_reply = onr;
            this->depth = d;
        }

        bool operator ==(const tt_entry_c &x)
        {
            return key == x.key;
        }
    };

    typedef move_s(engine::*stage_func_ptr)(std::vector<move_s> &, move_s &,
                                            unsigned &, unsigned &, const int);

    time_point_t t_beg;
    transposition_table_c<tt_entry_c, u32, 8> tt;
    std::set<u64> hash_keys;
    std::array<std::array<move_s, 2>, max_ply> killers;
    std::array<std::array<std::array<u64, 64>, 6>, 2> history;
    std::vector<std::vector<move_s>> pv;
    bool follow_pv;
    std::vector<stage_func_ptr> stages;
    std::string cv;

    static const unsigned megabyte = 1000000/sizeof(tt_entry_c);

    int search(int depth, const int alpha, const int beta,
               const int node_typ);
    u64 tt_probe_perft(const int depth);
    eval_t static_exchange_eval(const move_s move) const;
    move_s next_move(std::vector<move_s> &moves, move_s &tt_move,
                     unsigned &move_num, unsigned &stage,
                     const int depth);
    bool tt_probe(const int depth, int &alpha, const int beta,
                  move_s &tt_move, const int node_type);
    int search_cur_pos(const int depth, const int alpha, const int beta,
                       const move_s cur_move, unsigned int move_num,
                       const int node_type, const bool in_check);
    int search_result(const int val, const int alpha_orig,
                      const int alpha, const int beta, const int depth,
                      const int depth_orig, move_s best_move,
                      const unsigned legal_moves, const bool in_check);
    void apprice_and_sort_moves(std::vector<move_s> &moves,
                                const unsigned first_move_num,
                                const gen_mode mode) const;
    void apprice_move(move_s &move, const u64 max_hist) const;
    std::string pv_string();
    void update_cutoff_stats(const int depth, const move_s move);
    void erase_move(std::vector<move_s> &moves, const move_s move,
                        const unsigned first_ix) const;
    bool null_move_pruning(const int dpt, const int beta,const bool in_check);
    move_s gen_pv(std::vector<move_s> &moves, move_s &tt_move,
                  unsigned &move_num, unsigned &stage, const int depth);
    move_s gen_tt(std::vector<move_s> &moves, move_s &tt_move,
                  unsigned &move_num, unsigned &stage, const int depth);
    move_s gen_cap(std::vector<move_s> &moves, move_s &tt_move,
                   unsigned &move_num, unsigned &stage, const int depth);
    move_s probe_cap(std::vector<move_s> &moves, move_s &tt_move,
                     unsigned &move_num, unsigned &stage, const int depth);
    move_s gen_killer1(std::vector<move_s> &moves, move_s &tt_move,
                       unsigned &move_num, unsigned &stage, const int depth);
    move_s gen_killer2(std::vector<move_s> &moves, move_s &tt_move,
                       unsigned &move_num, unsigned &stage, const int depth);
    move_s gen_silent(std::vector<move_s> &moves, move_s &tt_move,
                      unsigned &move_num, unsigned &stage, const int depth);
    move_s probe_rest(std::vector<move_s> &moves, move_s &tt_move,
                        unsigned &move_num, unsigned &stage, const int depth);
    void reduce_history();

    void make_move(const move_s &move) {
        nodes++;
        ply++;
        board::make_move(move);
        done_moves.push_back(move);
#ifndef NDEBUG
        cv += move_to_str(move) + " ";
        if (cv == " ")
            std::cout << "Breakpoint" << std::endl;
#endif
    }

    void unmake_move() {
        ply--;
#ifndef NDEBUG
        cv.erase(cv.size() - 5);
#endif
        done_moves.pop_back();
        board::unmake_move();
    }

public:

    bool setup_position(const std::string &fen) {
        hash_keys.clear();
        done_moves.clear();
        bool ans = chess::setup_position(fen);
        hash_keys.insert(hash_key);
        std::fill(killers.begin(), killers.end(),
                  std::array<move_s, 2>({not_a_move, not_a_move}));
        for (auto clr: {black, white})
            for (auto ix: {pawn_ix, knight_ix, bishop_ix, rook_ix, queen_ix,
                           king_ix})
                std::fill(history.at(clr).at(ix).begin(),
                          history.at(clr).at(ix).end(),
                          0);
        for (auto &p: pv)
            p.clear();
        cv.clear();
        move_cr = 0;
        return ans;
    }

    bool enter_move(const std::string &str) {
        return chess::enter_move(str);
    }

    bool enter_move(const move_s &move) {
        if (!is_pseudo_legal(move))
            return false;
        make_move(move);
        if (!was_legal(move)) {
            unmake_move();
            return false;
        }
        if (move.index == pawn_ix || !is_silent(move))
            hash_keys.clear();
        hash_keys.insert(hash_key);
        return true;
    }

protected:

    bool search_draw() {
        return is_N_fold_repetition(1) || is_draw_by_N_move_rule(50) ||
            is_stalemate() || is_draw_by_material();
    }

    int late_move_reduction(const int depth, const move_s cur_move,
                            const bool in_check, unsigned int move_num,
                            const int node_type) const {
        if(depth < 3 || cur_move.is_capture || cur_move.promo ||
                in_check || move_num < 3)
            return 0;
        if(cur_move.index == pawn_ix &&
                (is_passer(!side, cur_move.to_coord, bb[side][pawn_ix]) ||
                 is_pawn_attack(cur_move.to_coord)))
            return 0;
        return 1 + int(node_type != pv_node && move_num > 6);
    }

    bool is_pawn_attack(const u8 coord) const {
        const u64 p_bb = one_nth_bit(coord);
        const u64 occ1 = bb[side][occupancy_ix] ^ bb[side][pawn_ix];
        return bool(all_pawn_attacks(p_bb, !side, occ1)) &&
            bool(all_pawn_attacks(p_bb, side, bb[!side][pawn_ix]));
    }

    double time_elapsed() {
        auto t_end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = t_end - t_beg;
        return elapsed.count();
    }

    int update_depth(const int depth_orig, const bool in_check) const {
        int ans = (in_check && depth_orig >= 0) ? depth_orig + 1 : depth_orig;
        return ans + int(is_recapture());
    }

    bool is_recapture() const {
        if (ply < 2)
            return false;
        const auto prev_move = done_moves.at(ply - 1);
        const auto prev_prev = done_moves.at(ply - 2);
        return (prev_move.is_capture && prev_prev.is_capture &&
                prev_move.to_coord == prev_prev.to_coord &&
                prev_move.priority > 64 && prev_prev.priority > 64);
    }

    bool razoring(int &val, const int depth, const int alpha,
                  const int beta, const int node_type) {
        if (node_type != pv_node && depth == 1 && val < beta - 60) {
            val = search(-2, alpha, beta, cut_node);
            return true;
        }
        if (node_type != pv_node && depth > 1 && depth <= 3 && val < beta - 250) {
            auto x = search(-2, alpha, beta, cut_node);
            if (x < beta) {
                val = x;
                return true;
            }
        }
        return false;
    }

    void store_pv(const move_s &cur_move) {
        std::swap(pv.at(ply), pv.at(ply + 1));
        pv.at(ply).insert(pv.at(ply).begin(), cur_move);
    }
};

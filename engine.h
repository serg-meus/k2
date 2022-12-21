#include "eval.h"
#include "transposition_table.h"
#include <chrono>

class engine : public eval {

    public:

    const int all_node = -1, pv_node = 0, cut_node = 1;
    enum class gen_stage {init, tt, captures, killer1, killer2, bad_captures, silent};
    const i8 max_depth = 127;

    u64 nodes;

    engine();
    engine(const engine&);
    int search(int depth, const int alpha, const int beta, const int node_typ);
    u64 perft(const int depth, const bool verbose);

    protected:

    const move_s not_a_move = {0, 0, 0, 0};

    enum class tt_bound {exact = 0, lower = 1, upper = 2};

    struct tt_entry_result_c {
        move_s best_move;
        i16 value;
        u8 bound_type;
        bool one_reply;
    };

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

    transposition_table_c<tt_entry_c, u32, 8> tt;
    static const unsigned megabyte = 1000000/sizeof(tt_entry_c);

    u64 tt_probe_perft(const int depth);
    eval_t static_exchange_eval(const move_s move) const;
    u8 min_attacker(const u8 to_coord, const u64 occ, const bool color,
                    u64 &attacker_bb) const;
    move_s next_move(std::vector<move_s> &moves, move_s &tt_move,
                     unsigned &move_num, gen_stage &stage, const int depth) const;
    bool tt_probe(const int depth, int &alpha, const int beta, move_s &tt_move);
    int search_cur_pos(const int depth, const int alpha, const int beta,
                       const move_s cur_move, unsigned int move_num,
                       const int node_type, const bool in_check);
    int search_result(const int val, const int alpha_orig,
                        const int alpha, const int beta, int depth,
                        move_s best_move, const unsigned legal_moves,
                        const bool in_check);
    void apprice_and_sort_moves(std::vector<move_s> &moves,
                                unsigned first_move_num) const;
    void apprice_move(move_s &move) const;

    void make_move(const move_s &move) {
        nodes++;
        board::make_move(move);
    }

    int late_move_reduction(const int depth, const move_s cur_move,
                            const bool in_check, unsigned int move_num,
                            const int node_type) const
    {
        if(depth < 3 || !is_silent(cur_move) || in_check || move_num < 3)
            return 0;
        if(cur_move.index == pawn_ix /*&& is_passer*/)
            return 0;
        return 1 + int(node_type != pv_node && move_num > 6);
    }

};

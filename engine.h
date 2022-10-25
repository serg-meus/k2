#include "eval.h"
#include "transposition_table.h"
#include <iostream>
#include <chrono>

class engine : public eval {

    public:

    engine();
    u64 perft(const u8 depth, const bool verbose);

    protected:

    enum class tt_bound_type {exact = 0, lower = 1, upper = 2};

    struct tt_entry_result_c {
        move_s best_move;
        i16 val;
        u8 bound_type;
        bool one_reply;
    };

    struct tt_entry_c {
        union {
            u64 nodes;
            tt_entry_result_c result;
            };
        u32 key;
        u8 depth;

        tt_entry_c() : nodes(0), key(0), depth(0) {}

        tt_entry_c(u64 inkey) : nodes(0), key(0), depth(0)
        {
            key = u32(inkey);
        }

        tt_entry_c(const u32 k, const u64 n, const u8 d) : nodes(0), key(0), depth(0)
        {
            this->key = k;
            this->nodes = n;
            this->depth = d;
        }

        bool operator ==(const tt_entry_c &x)
        {
            return key == x.key;
        }
    };

    transposition_table_c<tt_entry_c, u32, 8> tt;
    static const unsigned megabyte = 1000000/sizeof(tt_entry_c);
    
    u64 tt_probe_perft(const u8 depth);
};

#include "movegen.h"
#include "transposition_table.h"
#include <random>




class k2hash : public k2movegen
{

public:


    k2hash();


protected:


    typedef u8 hdepth_t;
    typedef u32 short_key_t;
    typedef u64 hash_key_t;
    typedef u64 node_t;

    const static depth_t fifty_moves = 100;
    const hash_key_t key_for_side_to_move = -1ULL;
    enum class bound {exact = 0, lower = 1, upper = 2};

    class hash_entry_c
    {

    public:

        short_key_t key;
        eval_t value;
        move_c best_move;
        hdepth_t depth;
        unsigned bound_type : 2;
        unsigned one_reply : 1;

        hash_entry_c() {}

        hash_entry_c(hash_key_t key)
        {
            this->key = static_cast<short_key_t>(key);
        }

        hash_entry_c(const hash_key_t &key, const eval_t &value,
                     const move_c &best_move, const depth_t &depth,
                     const k2hash::bound &bound_type, const bool &one_reply)
        {
            this->key = static_cast<short_key_t>(key);
            this->value = value;
            this->best_move = best_move;
            this->depth = depth;
            this->bound_type = static_cast<unsigned>(bound_type);
            this->one_reply = one_reply;
        }

        operator ==(const hash_entry_c &x)
        {
            return key == x.key;
        }

        short_key_t hash() const {return 0;}

    };

    hash_key_t hash_key;
    hash_key_t zobrist[2*piece_types][board_width][board_height];
    hash_key_t zobrist_en_passant[board_width + 1];
    hash_key_t zobrist_castling[16];
    hash_key_t done_hash_keys[fifty_moves + max_ply];

    void RunUnitTests();


protected:


    hash_key_t InitHashKey() const;

    void MakeMove(const move_c m)
    {
        bool special_move = k2eval::MakeMove(m);
        MoveHashKey(m, special_move);
    }

    void TakebackMove(const move_c m)
    {
        k2eval::TakebackMove(m);
        hash_key = done_hash_keys[fifty_moves + ply];
    }

    void MoveHashKey(const move_c move, const bool special);

    piece_t piece_hash_index(piece_t piece) const
    {
        return piece - 2;
    }

};

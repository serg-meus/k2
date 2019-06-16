#include "movegen.h"
#include <random>




class k2hash : public k2movegen
{

public:


    k2hash();


protected:


    typedef u8 hbound_t;
    typedef u8 hdepth_t;
    typedef u32 short_key_t;
    typedef u64 hash_key_t;
    typedef u64 node_t;

    const static depth_t fifty_moves = 100;
    const hash_key_t key_for_side_to_move = -1ULL;
    const hbound_t
    exact_value = 1,
    upper_bound = 2,
    lower_bound = 3;

    struct hash_entry_s
    {


    public:


        short_key_t key;
        eval_t value;
        move_c best_move;

        hdepth_t depth;
        hbound_t bound_type : 2;
        hbound_t age : 2;
        hbound_t one_reply : 1;
        hbound_t in_check : 1;
        hbound_t avoid_null_move : 1;
    };

    class hash_table_c
    {


    public:


        hash_table_c();
        hash_table_c(const size_t size_mb);
        ~hash_table_c();

        bool set_size(size_t size_mb);
        hash_entry_s* count(hash_key_t key) const;
        void add(hash_key_t key, eval_t value, move_c best_move, depth_t depth,
                 hbound_t bound_type, depth_t half_mov_cr, bool one_reply,
                 node_t nodes);
        void clear();
        hash_entry_s& operator [](const hash_key_t key) const;
        bool resize(size_t size_mb);

        size_t size() const
        {
            return _size;
        }

        size_t max_size() const
        {
            return sizeof(hash_entry_s)*buckets*entries_in_a_bucket;
        }


    protected:


        hash_entry_s *data;

        const size_t entries_in_a_bucket;
        size_t buckets;
        hash_key_t mask;
        size_t _size;
    };

    hash_key_t hash_key;
    hash_key_t zorb[2*piece_types][board_width][board_height];
    hash_key_t zorb_en_passant[board_width + 1];
    hash_key_t zorb_castling[16];
    hash_key_t done_hash_keys[fifty_moves + max_ply];


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


private:


    void MoveHashKey(const move_c move, const bool special);

    piece_t piece_hash_index(piece_t piece) const
    {
        return piece - 2;
    }

};

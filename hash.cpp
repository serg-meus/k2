#include "hash.h"





//--------------------------------
bool k2hash::InitHashTable()
{
    InitMoveGen();

    std::uniform_int_distribution<hash_key_t> zorb_distr(0, (hash_key_t)-1);
    std::mt19937 rnd_gen;

    for(auto i = 0; i < 12; ++i)
        for(auto j = 0; j < 8; ++j)
            for(auto k = 0; k < 8; ++k)
                zorb[i][j][k] = zorb_distr(rnd_gen);

    for(size_t i = 1;
    i < sizeof(zorb_en_passant)/sizeof(*zorb_en_passant);
    ++i)
        zorb_en_passant[i] = zorb_distr(rnd_gen);

    for(size_t i = 1;
    i < sizeof(zorb_castling)/sizeof(*zorb_castling);
    ++i)
        zorb_castling[i] = zorb_distr(rnd_gen);

    zorb_en_passant[0] = 0;
    zorb_castling[0] = 0;

    return true;
}





//--------------------------------
k2hash::hash_key_t k2hash::InitHashKey()
{
    hash_key_t ans = 0;

    for(auto fr : coords[wtm])
        ans ^= zorb[piece_hash_index(b[fr])][get_col(fr)][get_row(fr)];

    for(auto fr : coords[!wtm])
        ans ^= zorb[piece_hash_index(b[fr])][get_col(fr)][get_row(fr)];

    if(!wtm)
        ans ^= key_for_side_to_move;
    state_s &f = b_state[prev_states + ply];
    ans ^= zorb_en_passant[f.ep] ^ zorb_castling[f.cstl];

    return ans;
}





//--------------------------------
void k2hash::MoveHashKey(move_c m, bool special)
{
    doneHashKeys[FIFTY_MOVES + ply - 1] = hash_key;
    auto fr = b_state[prev_states + ply].fr;

    auto pt   = b[m.to];
    auto &f = b_state[prev_states + ply],
            &_f = b_state[prev_states + ply - 1];

    hash_key ^= zorb[piece_hash_index(pt)][get_col(fr)][get_row(fr)]
           ^ zorb[piece_hash_index(pt)][get_col(m.to)][get_row(m.to)];

    if(f.capt)
        hash_key ^= zorb[piece_hash_index(f.capt)][get_col(m.to)][get_row(m.to)];
    if(_f.ep)
        hash_key ^= zorb_en_passant[_f.ep];

    if(m.flg & mPROM)
        hash_key ^= zorb[piece_hash_index(_P ^ wtm)][get_col(fr)][get_row(fr)]
               ^ zorb[piece_hash_index(pt)][get_col(fr)][get_row(fr)];
    else if(m.flg & mENPS)
        hash_key ^= zorb[piece_hash_index(_p ^ wtm)][get_col(m.to)]
            [get_row(m.to) + (wtm ? 1 : -1)];
    else if(m.flg & mCSTL)
    {
        if(!wtm)
        {
            if(m.flg & mCS_K)
                hash_key ^= zorb[piece_hash_index(_R)][7][0]
                         ^  zorb[piece_hash_index(_R)][5][0];
            else
                hash_key ^= zorb[piece_hash_index(_R)][0][0]
                         ^  zorb[piece_hash_index(_R)][3][0];
        }
        else
        {
            if(m.flg & mCS_K)
                hash_key ^= zorb[piece_hash_index(_r)][7][7]
                         ^  zorb[piece_hash_index(_r)][5][7];
            else
                hash_key ^= zorb[piece_hash_index(_r)][0][7]
                         ^  zorb[piece_hash_index(_r)][3][7];
        }
        hash_key ^= zorb_castling[_f.cstl]
                 ^  zorb_castling[f.cstl];
    }
    hash_key ^= key_for_side_to_move;
    if(special)
    {
        if(to_black(pt) == _p && !f.capt)
            hash_key ^= zorb_en_passant[f.ep];
        else
            hash_key ^= zorb_castling[_f.cstl] ^ zorb_castling[f.cstl];
    }

#ifndef NDEBUG
    auto tmp_key = InitHashKey();
    if(tmp_key != hash_key)
        ply = ply;
    assert(tmp_key == hash_key);
#endif //NDEBUG
}





//--------------------------------
k2hash::hash_table_c::hash_table_c() : entries_in_a_bucket(4)
{
    set_size(64);
}





//--------------------------------
k2hash::hash_table_c::hash_table_c(size_t size_mb) : entries_in_a_bucket(4)
{
    set_size(size_mb);
}





//--------------------------------
k2hash::hash_table_c::~hash_table_c()
{
    delete[] data;
}





//--------------------------------
bool k2hash::hash_table_c::set_size(size_t size_mb)
{
    bool ans = true;
    buckets = 0;
    mask = 0;
    size_t sz = size_mb * 1000 / sizeof(hash_entry_s)
                * 1000 / entries_in_a_bucket;
    unsigned MSB_count = 0;
    while(sz >>= 1)
        MSB_count++;
    sz = (1 << MSB_count);

    if((data = new hash_entry_s[entries_in_a_bucket*sz]) == nullptr)
    {
        std::cout << "error: can't allocate memory for hash table"
                  << std::endl;
        ans = false;
    }

    if(ans)
    {
        buckets = sz;
        mask = sz - 1;

        clear();
    }
    else
    {
        buckets = 0;
        mask = 0;
        data = new hash_entry_s[1];

        memset(data, 0, sizeof(hash_entry_s));
    }

    return ans;
}





//--------------------------------
void k2hash::hash_table_c::clear()
{
    _size = 0;
    memset(data, 0,
           sizeof(hash_entry_s)*buckets*entries_in_a_bucket);
}





//--------------------------------
void k2hash::hash_table_c::add(hash_key_t key, score_t value, move_c best,
                              depth_t depth, hbound_t bound_type, depth_t age,
                              bool one_reply, node_t nodes)
{
    size_t i;
    auto *bucket = &data[entries_in_a_bucket*(key & mask)];          // looking for already existed entries for the same position
    for(i = 0; i < entries_in_a_bucket; ++i)
        if(bucket[i].key == (key >> 32))
            break;

    if(i >= entries_in_a_bucket)
    {
        for(i = 0; i < entries_in_a_bucket; ++i)                        // looking for empty entries
            if(bucket[i].key == 0 && bucket[i].depth == 0)
            {
                _size += sizeof(hash_entry_s);
                break;
            }

        if(i >= entries_in_a_bucket)
        {
            for(i = 0; i < entries_in_a_bucket; ++i)                    // looking for entries with lower depth
            {
                auto bckt_age = bucket[i].age;
                if(bckt_age > (age & 0x03))
                    bckt_age -= 3;
                if(bucket[i].depth + bckt_age < depth + age)
                    break;
            }

            if(i >= entries_in_a_bucket)                                // if not found anything, rewrite random entry in a bucket
                i = (unsigned)(nodes ^ key)
                        & (entries_in_a_bucket - 1);
        }
    }
    bucket[i].key           = key >> 32;
    bucket[i].best_move     = best;
    bucket[i].depth         = depth;
    bucket[i].bound_type    = bound_type;
    bucket[i].value         = value;
    bucket[i].one_reply     = one_reply;
    bucket[i].age = age & 0x03;
}





//--------------------------------
k2hash::hash_entry_s* k2hash::hash_table_c::count(hash_key_t key)
{
    hash_entry_s *bucket = &data[entries_in_a_bucket*(key & mask)];
    hash_entry_s *ans = nullptr;
    for(size_t i = 0; i < entries_in_a_bucket; ++i, ++bucket)
        if(bucket->key == key >> 32)
        {
            ans = bucket;
            break;
        }
    return ans;
}





//--------------------------------
k2hash::hash_entry_s& k2hash::hash_table_c::operator [](hash_key_t key)
{
    size_t i, ans = 0;
    hash_entry_s *bucket = &data[entries_in_a_bucket*(key & mask)];
    for(i = 0; i < entries_in_a_bucket; ++i)
        if(bucket[i].key == key >> 32)
            ans++;

    return bucket[ans];
}





//--------------------------------
bool k2hash::hash_table_c::resize(size_t size_mb)
{
    delete[] data;

    return set_size(size_mb);
}

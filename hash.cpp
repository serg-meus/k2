#include "hash.h"





//------f--------------------------
k2hash::k2hash()
{

    std::uniform_int_distribution<hash_key_t> zorb_distr(0, (hash_key_t) - 1);
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

    hash_key = InitHashKey();
}





//--------------------------------
k2hash::hash_key_t k2hash::InitHashKey()
{
    hash_key_t ans = 0;

    for(auto from_coord : coords[wtm])
        ans ^= zorb[piece_hash_index(b[from_coord])]
               [get_col(from_coord)][get_row(from_coord)];

    for(auto from_coord : coords[!wtm])
        ans ^= zorb[piece_hash_index(b[from_coord])]
               [get_col(from_coord)][get_row(from_coord)];

    if(!wtm)
        ans ^= key_for_side_to_move;
    auto &f = k2chess::state[ply];
    ans ^= zorb_en_passant[f.en_passant_rights] ^
            zorb_castling[f.castling_rights];

    return ans;
}





//--------------------------------
void k2hash::MoveHashKey(move_c m, bool special)
{
    doneHashKeys[FIFTY_MOVES + ply - 1] = hash_key;
    auto from_coord = k2chess::state[ply].from_coord;

    auto pt = b[m.to_coord];
    auto &f = k2chess::state[ply];
    auto &_f = k2chess::state[ply - 1];

    hash_key ^= zorb[piece_hash_index(pt)]
                [get_col(from_coord)][get_row(from_coord)]
                ^ zorb[piece_hash_index(pt)]
                [get_col(m.to_coord)][get_row(m.to_coord)];

    if(f.captured_piece)
        hash_key ^= zorb[piece_hash_index(f.captured_piece)]
                    [get_col(m.to_coord)][get_row(m.to_coord)];
    if(_f.en_passant_rights)
        hash_key ^= zorb_en_passant[_f.en_passant_rights];

    if(m.flag & is_promotion)
        hash_key ^= zorb[piece_hash_index(white_pawn ^ wtm)]
                    [get_col(from_coord)][get_row(from_coord)]
                    ^ zorb[piece_hash_index(pt)]
                    [get_col(from_coord)][get_row(from_coord)];
    else if(m.flag & is_en_passant)
        hash_key ^= zorb[piece_hash_index(black_pawn ^ wtm)]
                    [get_col(m.to_coord)][get_row(m.to_coord)
                                          + (wtm ? 1 : -1)];
    else if(m.flag & is_castle)
    {
        if(!wtm)
        {
            if(m.flag & is_right_castle)
                hash_key ^= zorb[piece_hash_index(white_rook)][7][0]
                            ^ zorb[piece_hash_index(white_rook)][5][0];
            else
                hash_key ^= zorb[piece_hash_index(white_rook)][0][0]
                            ^ zorb[piece_hash_index(white_rook)][3][0];
        }
        else
        {
            if(m.flag & is_right_castle)
                hash_key ^= zorb[piece_hash_index(black_rook)][7][7]
                            ^ zorb[piece_hash_index(black_rook)][5][7];
            else
                hash_key ^= zorb[piece_hash_index(black_rook)][0][7]
                            ^ zorb[piece_hash_index(black_rook)][3][7];
        }
        hash_key ^= zorb_castling[_f.castling_rights]
                    ^ zorb_castling[f.castling_rights];
    }
    hash_key ^= key_for_side_to_move;
    if(special)
    {
        if(get_type(pt) == pawn && !f.captured_piece)
            hash_key ^= zorb_en_passant[f.en_passant_rights];
        else
            hash_key ^= zorb_castling[_f.castling_rights] ^
                    zorb_castling[f.castling_rights];
    }

#ifndef NDEBUG
    auto tmp_key = InitHashKey();
    if(tmp_key != hash_key)
        std::cout << "( breakpoint )" << std::endl;
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
void k2hash::hash_table_c::add(hash_key_t key, eval_t value, move_c best,
                               depth_t depth, hbound_t bound_type,
                               depth_t age, bool one_reply, node_t nodes)
{
    size_t i;
    // looking for already existed entries for the same position
    auto *bucket = &data[entries_in_a_bucket*(key & mask)];
    for(i = 0; i < entries_in_a_bucket; ++i)
        if(bucket[i].key == (key >> 32))
            break;

    if(i >= entries_in_a_bucket)
    {
        // looking for empty entries
        for(i = 0; i < entries_in_a_bucket; ++i)
            if(bucket[i].key == 0 && bucket[i].depth == 0)
            {
                _size += sizeof(hash_entry_s);
                break;
            }

        if(i >= entries_in_a_bucket)
        {
            // looking for entries with lower depth
            for(i = 0; i < entries_in_a_bucket; ++i)
            {
                auto bckt_age = bucket[i].age;
                if(bckt_age > (age & 0x03))
                    bckt_age -= 3;
                if(bucket[i].depth + bckt_age < depth + age)
                    break;
            }
            // if not found anything, rewrite random entry in a bucket
            if(i >= entries_in_a_bucket)
                i = (unsigned)(nodes ^ key)
                    & (entries_in_a_bucket - 1);
        }
    }
    bucket[i].key = key >> 32;
    bucket[i].best_move = best;
    bucket[i].depth = depth;
    bucket[i].bound_type = bound_type;
    bucket[i].value = value;
    bucket[i].one_reply = one_reply;
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

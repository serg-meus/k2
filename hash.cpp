#include "hash.h"





//--------------------------------
UQ hash_key;

UQ  zorb[12][8][8],
    zorb_en_passant[9],
    zorb_castling[16];

UQ  doneHashKeys[FIFTY_MOVES + max_ply];





//--------------------------------
bool InitHashTable()
{
    InitMoveGen();

    std::uniform_int_distribution<UQ> zorb_distr(0, (UQ)-1);
    std::mt19937 rnd_gen;

    for(unsigned i = 0; i < 12; ++i)
        for(unsigned j = 0; j < 8; ++j)
            for(unsigned k = 0; k < 8; ++k)
                zorb[i][j][k] = zorb_distr(rnd_gen);

    for(unsigned i = 1;
    i < sizeof(zorb_en_passant)/sizeof(*zorb_en_passant);
    ++i)
        zorb_en_passant[i] = zorb_distr(rnd_gen);

    for(unsigned i = 1;
    i < sizeof(zorb_castling)/sizeof(*zorb_castling);
    ++i)
        zorb_castling[i] = zorb_distr(rnd_gen);

    zorb_en_passant[0] = 0;
    zorb_castling[0] = 0;

    return true;
}





//--------------------------------
UQ InitHashKey()
{
    UQ ans = 0;

    for(auto fr : coords[wtm])
        ans ^= zorb[MEN_TO_ZORB(b[fr])][COL(fr)][ROW(fr)];

    for(auto fr : coords[!wtm])
        ans ^= zorb[MEN_TO_ZORB(b[fr])][COL(fr)][ROW(fr)];

    if(!wtm)
        ans ^= (UQ)-1;
    BrdState &f = b_state[prev_states + ply];
    ans ^= zorb_en_passant[f.ep] ^ zorb_castling[f.cstl];

    return ans;
}





//--------------------------------
void MoveHashKey(Move m, bool special)
{
    doneHashKeys[FIFTY_MOVES + ply - 1] = hash_key;
    UC fr = b_state[prev_states + ply].fr;

    UC pt   = b[m.to];
    BrdState &f = b_state[prev_states + ply],
            &_f = b_state[prev_states + ply - 1];

    hash_key ^= zorb[MEN_TO_ZORB(pt)][COL(fr)][ROW(fr)]
           ^ zorb[MEN_TO_ZORB(pt)][COL(m.to)][ROW(m.to)];

    if(f.capt)
        hash_key ^= zorb[MEN_TO_ZORB(f.capt)][COL(m.to)][ROW(m.to)];
    if(_f.ep)
        hash_key ^= zorb_en_passant[_f.ep];

    if(m.flg & mPROM)
        hash_key ^= zorb[MEN_TO_ZORB(_P ^ wtm)][COL(fr)][ROW(fr)]
               ^ zorb[MEN_TO_ZORB(pt)][COL(fr)][ROW(fr)];
    else if(m.flg & mENPS)
        hash_key ^= zorb[MEN_TO_ZORB(_p ^ wtm)][COL(m.to)]
            [ROW(m.to) + (wtm ? 1 : -1)];
    else if(m.flg & mCSTL)
    {
        if(!wtm)
        {
            if(m.flg & mCS_K)
                hash_key ^= zorb[MEN_TO_ZORB(_R)][7][0]
                         ^  zorb[MEN_TO_ZORB(_R)][5][0];
            else
                hash_key ^= zorb[MEN_TO_ZORB(_R)][0][0]
                         ^  zorb[MEN_TO_ZORB(_R)][3][0];
        }
        else
        {
            if(m.flg & mCS_K)
                hash_key ^= zorb[MEN_TO_ZORB(_r)][7][7]
                         ^  zorb[MEN_TO_ZORB(_r)][5][7];
            else
                hash_key ^= zorb[MEN_TO_ZORB(_r)][0][7]
                         ^  zorb[MEN_TO_ZORB(_r)][3][7];
        }
        hash_key ^= zorb_castling[_f.cstl]
                 ^  zorb_castling[f.cstl];
    }
    hash_key ^= -1L;
    if(special)
    {
        if(TO_BLACK(pt) == _p && !f.capt)
            hash_key ^= zorb_en_passant[f.ep];
        else
            hash_key ^= zorb_castling[_f.cstl] ^ zorb_castling[f.cstl];
    }

#ifndef NDEBUG
    UQ tmp_key = InitHashKey();
    if(tmp_key != hash_key)
        ply = ply;
    assert(tmp_key == hash_key);
#endif //NDEBUG
}





//--------------------------------
transposition_table::transposition_table() : entries_in_a_bucket(4)
{
    set_size(64);
}





//--------------------------------
transposition_table::transposition_table(unsigned size_mb) : entries_in_a_bucket(4)
{
    set_size(size_mb);
}





//--------------------------------
transposition_table::~transposition_table()
{
    delete[] data;
}





//--------------------------------
bool transposition_table::set_size(unsigned size_mb)
{
    bool ans = true;
    buckets = 0;
    mask = 0;
    unsigned sz = size_mb * 1000 / sizeof(tt_entry)
                * 1000 / entries_in_a_bucket;
    unsigned MSB_count = 0;
    while(sz >>= 1)
        MSB_count++;
    sz = (1 << MSB_count);

    if((data = new tt_entry[entries_in_a_bucket*sz]) == nullptr)
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
        data = new tt_entry[1];

        memset(data, 0, sizeof(tt_entry));
    }

    return ans;
}





//--------------------------------
void transposition_table::clear()
{
    _size = 0;
    memset(data, 0,
           sizeof(tt_entry)*buckets*entries_in_a_bucket);
}





//--------------------------------
void transposition_table::add(UQ key, short value, Move best,
                              UI depth, UI bound_type, UI age,
                              bool one_reply)
{
    unsigned i;
    tt_entry *bucket = &data[entries_in_a_bucket*(key & mask)];          // looking for already existed entries for the same position
    for(i = 0; i < entries_in_a_bucket; ++i)
        if(bucket[i].key == (key >> 32))
            break;

    if(i >= entries_in_a_bucket)
    {
        for(i = 0; i < entries_in_a_bucket; ++i)                        // looking for empty entries
            if(bucket[i].key == 0 && bucket[i].depth == 0)
            {
                _size += sizeof(tt_entry);
                break;
            }

        if(i >= entries_in_a_bucket)
        {
            for(i = 0; i < entries_in_a_bucket; ++i)                    // looking for entries with lower depth
            {
                int bckt_age = bucket[i].age;
                if(bckt_age > (int)(age & 0x03))
                    bckt_age -= 3;
                if(bucket[i].depth + bckt_age < int(depth + age))
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
unsigned transposition_table::count(UQ key)
{
    unsigned i, ans = 0;
    tt_entry *bucket = &data[entries_in_a_bucket*(key & mask)];
    for(i = 0; i < entries_in_a_bucket; ++i)
        if(bucket[i].key == key >> 32)
            ans++;
    assert(ans <= 1);
    return ans;
}





//--------------------------------
unsigned transposition_table::count(UQ key, tt_entry **entry)
{
    unsigned i, ans = 0;
    tt_entry *bucket = &data[entries_in_a_bucket*(key & mask)];
    for(i = 0; i < entries_in_a_bucket; ++i, ++bucket)
        if(bucket->key == key >> 32)
        {
            ans++;
            break;
        }
    *entry = bucket;
    return ans;
}





//--------------------------------
tt_entry& transposition_table::operator [](UQ key)
{
    unsigned i, ans = 0;
    tt_entry *bucket = &data[entries_in_a_bucket*(key & mask)];
    for(i = 0; i < entries_in_a_bucket; ++i)
        if(bucket[i].key == key >> 32)
            ans++;

    return bucket[ans];
}





//--------------------------------
bool transposition_table::resize(unsigned size_mb)
{
    delete[] data;

    return set_size(size_mb);
}





//--------------------------------
void MkMove(Move m)
{
    bool special_move = MkMoveAndEval(m);
    MoveHashKey(m, special_move);
}





//--------------------------------
void UnMove(Move m)
{
    UnMoveAndEval(m);
    hash_key = doneHashKeys[FIFTY_MOVES + ply];
}





//--------------------------------
void MkMoveIncrementally(Move m, bool special_move)
{
    MkEvalAfterFastMove(m);
    MoveHashKey(m, special_move);
}

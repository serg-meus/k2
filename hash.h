#include "movegen.h"
#include "extern.h"
#include <random>





//--------------------------------
#define MEN_TO_ZORB(X) ((X) - 2)
const depth_t FIFTY_MOVES = 101;       // because last 50th move can be a mate
const hash_key_t key_for_side_to_move = -1ULL;




//--------------------------------
enum {hNONE, hEXACT, hUPPER, hLOWER};





typedef u8 hbound_t;
typedef u8 hdepth_t;
typedef u32 short_key_t;


//--------------------------------
struct hash_entry_s
{
///////
public:
    short_key_t key;
    score_t value;
    move_c    best_move;

    hdepth_t  depth;
    hbound_t  bound_type  : 2;
    hbound_t  age         : 2;
    hbound_t  one_reply   : 1;
    hbound_t  in_check    : 1;
    hbound_t  avoid_null_move : 1;
};

//--------------------------------
class hash_table_c
{
///////
public:

    hash_table_c();
    hash_table_c(size_t size_mb);
    ~hash_table_c();

    bool set_size(size_t size_mb);
    hash_entry_s* count(hash_key_t key);
    void add(hash_key_t key, score_t value, move_c best, depth_t depth,
             hbound_t bound_type, depth_t half_mov_cr, bool one_reply);
    void clear();
    hash_entry_s& operator [](hash_key_t key);
    bool resize(size_t size_mb);
    size_t size()     {return _size;}
    size_t max_size() {return sizeof(hash_entry_s)*buckets*entries_in_a_bucket;}

//////////
protected:
    hash_entry_s *data;

    const size_t entries_in_a_bucket;
    size_t buckets;
    hash_key_t mask;
    size_t _size;
};





//--------------------------------
bool InitHashTable();
hash_key_t  InitHashKey();
void MoveHashKey(move_c m, bool special);
void MkMove(move_c m);
void UnMove(move_c m);
void MkMoveIncrementally(move_c m, bool special_move);

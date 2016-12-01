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
struct tt_entry
{
///////
public:
    short_key_t key;
    score_t value;
    Move    best_move;

    hdepth_t  depth;
    hbound_t  bound_type  : 2;
    hbound_t  age         : 2;
    hbound_t  one_reply   : 1;
    hbound_t  in_check    : 1;
    hbound_t  avoid_null_move : 1;
};

//--------------------------------
class transposition_table
{
///////
public:

    transposition_table();
    transposition_table(size_t size_mb);
    ~transposition_table();

    bool set_size(size_t size_mb);
    tt_entry* count(hash_key_t key);
    void add(hash_key_t key, score_t value, Move best, depth_t depth,
             hbound_t bound_type, depth_t half_mov_cr, bool one_reply);
    void clear();
    tt_entry& operator [](hash_key_t key);
    bool resize(size_t size_mb);
    size_t size()     {return _size;}
    size_t max_size() {return sizeof(tt_entry)*buckets*entries_in_a_bucket;}

//////////
protected:
    tt_entry *data;

    const size_t entries_in_a_bucket;
    size_t buckets;
    hash_key_t mask;
    size_t _size;
};





//--------------------------------
bool InitHashTable();
hash_key_t  InitHashKey();
void MoveHashKey(Move m, bool special);
void MkMove(Move m);
void UnMove(Move m);
void MkMoveIncrementally(Move m, bool special_move);

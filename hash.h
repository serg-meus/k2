#include "movegen.h"
#include "extern.h"
#include <random>





//--------------------------------
#define MEN_TO_ZORB(X) ((X) - 2)
#define FIFTY_MOVES 101       // because last 50th move can be a mate
const u64 key_for_side_to_move = -1ULL;




//--------------------------------
enum {hNONE, hEXACT, hUPPER, hLOWER};





//--------------------------------
struct tt_entry
{
///////
public:
    u32      key;
    short   value;
    Move    best_move;

    u8  depth;
    u8  bound_type  : 2;
    u8  age         : 2;
    u8  one_reply   : 1;
    u8  in_check    : 1;
    u8  avoid_null_move : 1;
};

//--------------------------------
class transposition_table
{
///////
public:

    transposition_table();
    transposition_table(unsigned size_mb);
    ~transposition_table();

    bool set_size(unsigned size_mb);
    tt_entry* count(u64 key);
    void add(u64 key, short value, Move best, u32 depth,
             u32 bound_type, uint32_t half_mov_cr, bool one_reply);
    void clear();
    tt_entry& operator [](u64 key);
    bool resize(unsigned size_mb);
    u64 size()       {return _size;}
    u64 max_size()   {return sizeof(tt_entry)*buckets*entries_in_a_bucket;}

//////////
protected:
    tt_entry *data;

    const unsigned entries_in_a_bucket;
    unsigned buckets;
    u64 mask;
    u64 _size;
};





//--------------------------------
bool InitHashTable();
u64   InitHashKey();
void MoveHashKey(Move m, bool special);
void MkMove(Move m);
void UnMove(Move m);
void MkMoveIncrementally(Move m, bool special_move);

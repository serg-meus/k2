#include "movegen.h"
#include "extern.h"
#include <random>





//--------------------------------
#define MEN_TO_ZORB(X) ((X) - 2)
#define FIFTY_MOVES 101       // because last 50th move can be a mate





//--------------------------------
enum {hNONE, hEXACT, hUPPER, hLOWER};





//--------------------------------
struct tt_entry
{
///////
public:
    UI      key;
    short   value;
    Move    best_move;

    UC  depth;
    UC  bound_type  : 2;
    UC  age         : 2;
    UC  one_reply   : 1;
    UC  in_check    : 1;
    UC  avoid_null_move : 1;
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
    tt_entry* count(UQ key);
    void add(UQ key, short value, Move best, UI depth,
             UI bound_type, uint32_t half_mov_cr, bool one_reply);
    void clear();
    tt_entry& operator [](UQ key);
    bool resize(unsigned size_mb);
    UQ size()       {return _size;}
    UQ max_size()   {return sizeof(tt_entry)*buckets*entries_in_a_bucket;}

//////////
protected:
    tt_entry *data;

    const unsigned entries_in_a_bucket;
    unsigned buckets;
    UQ mask;
    UQ _size;
};





//--------------------------------
bool InitHashTable();
UQ   InitHashKey();
void MoveHashKey(Move m, bool special);
void MkMove(Move m);
void UnMove(Move m);
void MkMoveIncrementally(Move m, bool special_move);

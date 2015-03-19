#include "chess.h"
#include "extern.h"
#include <random>

//--------------------------------
#define MEN_TO_ZORB(X) ((X) - 2)

//--------------------------------
bool    InitHashTable();
UQ      InitHashKey();
void    MoveHashKey(Move m, UC fr, int special);

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
    UC  only_move   : 1;
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
    unsigned count(UQ key);
    unsigned count(UQ key, tt_entry **entry);
    void add(UQ key, short value, Move best, UI depth, UI bound_type);
    void clear();
    tt_entry& operator [](UQ key);
    bool resize(unsigned size_mb);
    UQ size()       {return _size;}
    UQ max_size()   {return sizeof(tt_entry)*buckets*entries_in_a_bucket;}

//////////
protected:
    tt_entry **data;

    const unsigned entries_in_a_bucket = 4;
    unsigned buckets;
    UQ mask;
    UQ _size;
};


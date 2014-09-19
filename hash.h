#include "chess.h"
#include "extern.h"
#include <random>
#include <map>

#define MEN_TO_ZORB(X) ((X) - 2)


bool    InitHashTable();
UQ      InitHashKey();
void    MoveHashKey(Move m, UC fr, int special);

/*
class TT
{
///////
public:

    TT();
    TT(int size_mb);

    set_size(int size_mb);

};
*/

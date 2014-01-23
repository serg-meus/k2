#include "chess.h"
#include "extern.h"
#include <random>
#include <map>

//#define MEN_TO_ZORB(X) ((X) < WHT ? ((X) - 1) : ((X) - WHT + 5))
//#define MEN_TO_ZORB(X) ((((X) & WHT) >> 2) - (((X) & WHT) >> 4) + (((X) & ~WHT) - 1))
#define MEN_TO_ZORB(X) ((((((X) & WHT) - 1) >> 5) & 6) + ((X) & ~WHT) - 1)
//#define MEN_TO_ZORB(X) men2zorb[X]
//#define MEN_TO_ZORB(X) ((X) < WHT ? ((X) + 5) : ((X) - WHT - 1))

bool    InitHashTable();
UQ      InitHashKey();
void    MoveHashKey(Move m, UC fr, int special);

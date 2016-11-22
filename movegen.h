#include "eval.h"
#include "extern.h"
#include <limits.h>





//--------------------------------
//#define DONT_USE_SEE_SORTING
//#define DONT_USE_SEE_CUTOFF


#define MOVE_FROM_PV    200
#define SECOND_KILLER   198
#define FIRST_KILLER    199
#define PV_FOLLOW       255
#define KING_CAPTURE    255
#define BAD_CAPTURES    63

#define APPRICE_NONE    0
#define APPRICE_CAPT    1
#define APPRICE_ALL     2





//--------------------------------
void    InitMoveGen();
int     GenMoves(Move *list, int apprice, Move *best_move);
void    GenPawn(Move *list, int *movCr, short_list<UC, lst_sz>::iterator it);
void    GenPawnCap(Move *list, int *movCr, short_list<UC, lst_sz>::iterator it);
void    GenCastles(Move *list, int *movCr);
int     GenCaptures(Move *list);
void    AppriceMoves(Move *list, int moveCr, Move *best_move);
void    AppriceQuiesceMoves(Move *list, int moveCr);
short   SEE(UC to, short frStreng, short val, bool stm);
short_list<UC, lst_sz>::iterator SeeMinAttacker(UC to);
short   SEE_main(Move m);
void    SortQuiesceMoves(Move *move_array, int moveCr);
void    PushMove(Move *move_array, int *movCr,
              short_list<UC, lst_sz>::iterator it, UC to, UC flg);
//--------------------------------

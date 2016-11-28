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
void    GenPawn(Move *list, int *movCr, short_list<u8, lst_sz>::iterator it);
void    GenPawnCap(Move *list, int *movCr, short_list<u8, lst_sz>::iterator it);
void    GenCastles(Move *list, int *movCr);
int     GenCaptures(Move *list);
void    AppriceMoves(Move *list, int moveCr, Move *best_move);
void    AppriceQuiesceMoves(Move *list, int moveCr);
short   SEE(u8 to, short frStreng, short val, bool stm);
short_list<u8, lst_sz>::iterator SeeMinAttacker(u8 to);
short   SEE_main(Move m);
void    SortQuiesceMoves(Move *move_array, int moveCr);
void    PushMove(Move *move_array, int *movCr,
              short_list<u8, lst_sz>::iterator it, u8 to, u8 flg);
//--------------------------------

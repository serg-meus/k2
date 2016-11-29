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
int     GenMoves(Move *list, Move *best_move, u8 apprice);
void    GenPawn(Move *list, u8 *movCr, iterator it);
void    GenPawnCap(Move *list, u8 *movCr, iterator it);
void    GenCastles(Move *list, u8 *movCr);
int     GenCaptures(Move *list);
void    AppriceMoves(Move *list, u8 moveCr, Move *best_move);
void    AppriceQuiesceMoves(Move *list, u8 moveCr);
short   SEE(u8 to, i16 frStreng, i16 val, bool stm);
iterator SeeMinAttacker(u8 to);
short   SEE_main(Move m);
void    SortQuiesceMoves(Move *move_array, u8 moveCr);
void    PushMove(Move *move_array, int *movCr,
              iterator it, u8 to, u8 flg);
//--------------------------------

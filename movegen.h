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




typedef u8  movcr_t;


//--------------------------------
void    InitMoveGen();
movcr_t GenMoves(Move *list, Move *best_move, u8 apprice);
void    GenPawn(Move *list, movcr_t *movCr, iterator it);
void    GenPawnCap(Move *list, movcr_t *movCr, iterator it);
void    GenCastles(Move *list, movcr_t *movCr);
movcr_t GenCaptures(Move *list);
void    AppriceMoves(Move *list, movcr_t moveCr, Move *best_move);
void    AppriceQuiesceMoves(Move *list, movcr_t moveCr);
streng_t SEE(coord_t to, streng_t frStreng, score_t val, bool stm);
iterator SeeMinAttacker(coord_t to);
streng_t SEE_main(Move m);
void    SortQuiesceMoves(Move *move_array, movcr_t moveCr);
void    PushMove(Move *move_array, movcr_t *movCr, iterator it,
                 coord_t to, move_flag_t flg);
//--------------------------------

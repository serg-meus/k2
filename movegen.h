#include "hash.h"
#include "extern.h"
//--------------------------------
#define PUSH_MOVE(PC, TO, FLG)  {*(ml++) = (TO) | ((PC) << 8) | (FLG); moveCr++;}
#define LIGHT(X, s2m)   ((X) && ((X) ^ (s2m)) < WHT)
#define DARK(X, s2m)    (((X) ^ (s2m)) > WHT)
#define MOVESCR(X)      (((X) >> 24) & 0xFF)

#define MOVE_FROM_PV    121
#define EQUAL_CAPTURE   119
#define SECOND_KILLER   123
#define FIRST_KILLER    125
#define KING_CAPTURE    255
#define PV_FOLLOW       255

#define APPRICE_NONE    0
#define APPRICE_CAPT    1
#define APPRICE_ALL     2

#define EXCEPT_SCORE    0x00FFFFFF
//--------------------------------
void    InitMoveGen();
int     GenMoves(Move *list, int apprice);
void    GenPawn(UC pc);
void    GenPawnCap(UC pc);
void    GenCastles();
int     GenCaptures(Move *list);
void    AppriceMoves(Move *list);
void    Next(Move *m, int cur, int top, Move *ans);
void    AppriceQuiesceMoves(Move *list);
//--------------------------------

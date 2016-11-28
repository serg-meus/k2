#include "chess.h"
#include <fstream>                  // to work with files (ifstream, getline())
#include <cstdlib>                  // to convert strings to floats (atof())
#include <iostream>
#include "extern.h"





//--------------------------------
#define MAXI(X, Y)       ((X) > (Y) ? (X) : (Y))
#define LIGHT(X, s2m)   ((X) && (((X) & white) == (s2m)))
#define DARK(X, s2m)    ((X) && (((X) & white) != (s2m)))

//#define TUNE_PARAMETERS
#define NPARAMS 1

#ifdef TUNE_PARAMETERS
    #include <vector>
#endif // TUNE_PARAMETERS





//--------------------------------
enum PriceList
{
    P_VAL_OPN   = 100,
    N_VAL_OPN   = 395,
    B_VAL_OPN   = 405,
    R_VAL_OPN   = 600,
    Q_VAL_OPN   = 1200,

    P_VAL_END   = 128,
    N_VAL_END   = 369,
    B_VAL_END   = 405,
    R_VAL_END   = 640,
    Q_VAL_END   = 1200,

    K_VAL       = 32000,
    INF         = 32760,

    CLAMPED_R   = 82,
};





//--------------------------------
void  InitEval();
void  FastEval(Move m);
short Eval();
void  InitEvaOfMaterialAndPst();
void  EvalPawns(u8 stm);
void  ClampedRook(u8 stm);
bool  IsUnstoppablePawn(u8 x, u8 y, u8 stm);
void  KingSafety(u8 king_color);
bool  IsPasser(u8 col, u8 stm);
void  BishopMobility(u8 stm);
short ReturnEval(u8 stm);
void  MaterialImbalances();
short EvalDebug();
short KingWeakness(u8 king_color);
bool  MkMoveAndEval(Move m);
void  UnMoveAndEval(Move m);
void  MkEvalAfterFastMove(Move m);
short CountKingTropism(u8 king_color);
void  MoveKingTropism(u8 fr, Move m, u8 king_color);
short KingOpenFiles(u8 king_color);
void  MovePawnStruct(u8 movedPiece, u8 fr, Move m);
short OneBishopMobility(u8 b_coord);

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
const score_t
    P_VAL_OPN = 100,
    N_VAL_OPN = 395,
    B_VAL_OPN = 405,
    R_VAL_OPN = 600,
    Q_VAL_OPN = 1200,

    P_VAL_END = 128,
    N_VAL_END = 369,
    B_VAL_END = 405,
    R_VAL_END = 640,
    Q_VAL_END = 1200,

    K_VAL = 32000,
    INF = 32760,
    CLAMPED_R  = 82;





//--------------------------------
void  InitEval();
void  FastEval(Move m);
score_t Eval();
void  InitEvaOfMaterialAndPst();
void  EvalPawns(side_to_move_t stm);
void  ClampedRook(side_to_move_t stm);
bool  IsUnstoppablePawn(coord_t x, coord_t y, side_to_move_t stm);
void  KingSafety(side_to_move_t king_color);
bool  IsPasser(coord_t col, side_to_move_t stm);
void  BishopMobility(side_to_move_t stm);
score_t ReturnEval(side_to_move_t stm);
void  MaterialImbalances();
score_t EvalDebug();
score_t KingWeakness(side_to_move_t king_color);
bool  MkMoveAndEval(Move m);
void  UnMoveAndEval(Move m);
void  MkEvalAfterFastMove(Move m);
score_t CountKingTropism(side_to_move_t king_color);
void  MoveKingTropism(coord_t fr, Move m, side_to_move_t king_color);
score_t KingOpenFiles(side_to_move_t king_color);
void  MovePawnStruct(piece_t movedPiece, coord_t fr, Move m);
score_t OneBishopMobility(coord_t b_coord);

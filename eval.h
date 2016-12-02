#include "chess.h"
#include "extern.h"
#include <fstream>                  // to work with files (ifstream, getline())
#include <cstdlib>                  // to convert strings to floats (atof())
#include <iostream>





//--------------------------------
#define LIGHT(X, s2m)   ((X) && (((X) & white) == (s2m)))
#define DARK(X, s2m)    ((X) && (((X) & white) != (s2m)))

//#define TUNE_PARAMETERS
#define NPARAMS 1

#ifdef TUNE_PARAMETERS
    #include <vector>
#endif // TUNE_PARAMETERS





//--------------------------------
const score_t
    pawn_val_opn = 100,
    kinght_val_opn = 395,
    bishop_val_opn = 405,
    rook_val_opn = 600,
    queen_val_opn = 1200,

    pawn_val_end = 128,
    kinght_val_end = 369,
    bishop_val_end = 405,
    rook_val_end = 640,
    queen_val_end = 1200,

    king_value = 32000,
    infinit_score = 32760;





//--------------------------------
void  InitEval();
void  FastEval(move_c m);
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
bool  MkMoveAndEval(move_c m);
void  UnMoveAndEval(move_c m);
void  MkEvalAfterFastMove(move_c m);
score_t CountKingTropism(side_to_move_t king_color);
void  MoveKingTropism(coord_t fr, move_c m, side_to_move_t king_color);
score_t KingOpenFiles(side_to_move_t king_color);
void  MovePawnStruct(piece_t movedPiece, coord_t fr, move_c m);
score_t OneBishopMobility(coord_t b_coord);

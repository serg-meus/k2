#include "chess.h"

#include <fstream>                  // to work with files (ifstream, getline())
#include <cstdlib>                  // to convert strings to floats (atof())
#include <iostream>
#include <vector>





//--------------------------------
class k2eval : public k2chess
{

public:
    k2eval();

protected:

typedef i8 pst_t;
typedef u8 rank_t;

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

score_t val_opn, val_end;
rank_t pawn_max[10][2], pawn_min[10][2];
score_t material_values_opn[7];
score_t material_values_end[7];
dist_t king_dist[120];
tropism_t king_tropism[2];
tropism_t tropism_factor[2][7];

const pst_t pst[6][2][8][8];
//std::vector <float> param;


public:


score_t EvalDebug();
bool FenToBoard(char *);


protected:


void  InitEval();
void  FastEval(move_c m);
score_t Eval();
void  InitEvaOfMaterialAndPst();
score_t ReturnEval(side_to_move_t stm);
void InitPawnStruct();
void SetPawnStruct(coord_t col);
bool  IsPasser(coord_t col, side_to_move_t stm);
bool  MkMoveAndEval(move_c m);
void  UnMoveAndEval(move_c m);
void  MkEvalAfterFastMove(move_c m);
bool is_light(piece_t piece, side_to_move_t stm);
bool is_dark(piece_t piece, side_to_move_t stm);


private:


void  EvalPawns(side_to_move_t stm);
void  ClampedRook(side_to_move_t stm);
bool  IsUnstoppablePawn(coord_t x, coord_t y, side_to_move_t stm);
void  KingSafety(side_to_move_t king_color);
void  BishopMobility(side_to_move_t stm);
void  MaterialImbalances();
score_t KingWeakness(side_to_move_t king_color);
score_t CountKingTropism(side_to_move_t king_color);
void  MoveKingTropism(coord_t fr, move_c m, side_to_move_t king_color);
score_t KingOpenFiles(side_to_move_t king_color);
void  MovePawnStruct(piece_t movedPiece, coord_t fr, move_c m);
score_t OneBishopMobility(coord_t b_coord);

};

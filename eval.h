#include "movegen.h"

#include <fstream>                  // to work with files (ifstream, getline())
#include <cstdlib>                  // to convert strings to floats (atof())
#include <iostream>

#define USELESS_HALFMOVES_TO_DRAW 100
#define MAXI(X, Y)       ((X) > (Y) ? (X) : (Y))

//#define CHECK_PREDICTED_VALUE

//#define EVAL_KING_TROPISM



//--------------------------------
enum PriceList
{
    P_VAL_OPN   = 100,
    N_VAL_OPN   = 395,
    B_VAL_OPN   = 405,
    R_VAL_OPN   = 600,
    Q_VAL_OPN   = 1200,

    P_VAL_END   = 125,
    N_VAL_END   = 395,
    B_VAL_END   = 405,
    R_VAL_END   = 600,
    Q_VAL_END   = 1200,

    K_VAL       = 32000,
    INF         = 32760,

    SHIELD_K    = 30,
    CLAMPED_R   = 250,
//    OPP_NEAR_K  = 10,
    UNSTOP_P    = 650,
    DBL_PROMO_P = 65,
    OPP_NEAR_K  = 20,

};

//--------------------------------


//--------------------------------
void InitEval();
void FastEval(Move m);
short Eval(/*short alpha, short beta*/);
void EvalAllMaterialAndPST();
void EvalPawns(bool stm);
void SimpleKingShield(UC stm);
void ClampedRook(UC stm);
void SimpleKingNearOpp(UC stm);
bool TestUnstoppable(int x, int y, UC stm);
void SimpleKingDist(UC stm);
short EvalAllKingDist(UC stm, UC king_coord);
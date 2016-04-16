#include "movegen.h"
#include <fstream>                  // to work with files (ifstream, getline())
#include <cstdlib>                  // to convert strings to floats (atof())
#include <iostream>
#include "extern.h"





//--------------------------------
#define MAXI(X, Y)       ((X) > (Y) ? (X) : (Y))

#define TUNE_PARAMETERS
#define NPARAMS 2

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

    CLAMPED_R   = 250
};





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
short SimpleKingDist(UC stm);
short EvalAllKingDist(UC stm, UC king_coord);
void KingSafety(UC king_color);
bool TestPromo(int col, UC stm);
void BishopMobility(UC stm);
short ReturnEval(UC stm);
void MaterialImbalances();
short EvalDebug();
void KingSafety2(UC king_color);
void KingSafety3(UC king_color);
short KingWeakness(UC king_color);
short KingShieldFactor(UC stm);
int CountAttacksOnOneSquare(UC king_color, UC attacked_coord,
                            int &weight, unsigned char *attacker_coords,
                            unsigned char &attacker_cr);
int CountAttacksOnKingShelter(UC king_color, int &weight, int &attackers);

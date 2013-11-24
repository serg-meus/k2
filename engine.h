#include "eval.h"
#include "extern.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include "Timer.h"

#define MAX_MOVES       256
#define RESIGN_VALUE    750
#define RESIGN_MOVES    3

#define BETA_CUTOFF 1000

//#define TUNE_PARAMETERS
#define USE_PVS
#define USE_NMR
#define USE_FTL

#ifdef TUNE_PARAMETERS
    #include <vector>
#endif // TUNE_PARAMETERS

#ifdef CHECK_PREDICTED_VALUE
struct PredictedInfo
{
    Move    oppMove;
    short   score;
    short   depth;
    short   state;
};
#endif // CHECK_PREDICTED_VALUE
//--------------------------------

void InitEngine();
void Perft(int depth);
short Search(int depth, short alpha, short beta);
short Quiesce(short alpha, short beta);
void StorePV(Move m);
void UpdateStatistics(Move m,/* int dpt,*/ unsigned i);
short RootSearch(int depth, short alpha, short beta);
void RootMoveGen(bool ic);
void MainSearch();
void InitSearch();
void PrintSearchResult();
void PlyOutput(short sc);
void InitTime();
bool ShowPV(int _ply);
void Ambiguous(Move m);
bool MakeLegalMove(char *mov);
void EvalAllMaterialAndPST();
bool FenToBoardAndVal(char *fen);
bool DrawDetect();
bool SimpleDrawByRepetition();
void CheckForInterrupt();
void MkMove(Move m);
void UnMove(Move m);
bool DrawByRepetitionInRoot(Move lastMove);
void MakeNullMove();
void UnMakeNullMove();
bool NullMove(int depth, short beta, bool ic);

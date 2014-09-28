#include "eval.h"
#include "extern.h"
#include <iomanip>
#include <ctime>
#include "Timer.h"

//--------------------------------
#define ENGINE_VERSION "065x"
//--------------------------------
//#define DONT_SHOW_STATISTICS
//#define DONT_USE_NULL_MOVE
//#define DONT_USE_FUTILITY
//#define DONT_USE_DELTA_PRUNING
//#define DONT_USE_HISTORY
//#define DONT_USE_LMR
#define DONT_USE_ONLY_MOVE_EXTENSION
//#define DONT_USE_MATE_DISTANCE_PRUNING
//#define DONT_USE_PVS_IN_ROOT

//--------------------------------
#define UNUSED(x) (void)(x)

#define MAX_MOVES       256
#define RESIGN_VALUE    850
#define RESIGN_MOVES    3

#define MOVE_IS_NULL 0xFF

//--------------------------------

void InitEngine();
void Perft(int depth);
short Search(int depth, short alpha, short beta, int lmr);
short Quiesce(short alpha, short beta);
void StorePV(Move m);
void UpdateStatistics(Move m, int dpt, unsigned i);
short RootSearch(int depth, short alpha, short beta);
void RootMoveGen(bool ic);
void MainSearch();
void InitSearch();
void PrintSearchResult();
void PlyOutput(short sc);
void InitTime();
bool ShowPV(int _ply);
void Ambiguous(Move m);
bool MakeMoveFinaly(char *mov);
void EvalAllMaterialAndPST();
bool FenStringToEngine(char *fen);
bool DrawDetect();
bool SimpleDrawByRepetition();
void CheckForInterrupt();
void MkMove(Move m);
void UnMove(Move m);
bool DrawByRepetitionInRoot(Move lastMove);
void MakeNullMove();
void UnMakeNullMove();
bool NullMove(int depth, short beta, bool ic, int lmr_);
bool Futility(int depth, short beta);
bool DrawByRepetition();
void ShowFen();
void ReHash(int size_mb);
bool HashProbe(int depth, short alpha, short beta,
               tt_entry *entry, bool *best_move_hashed);
bool PseudoLegal(Move m, bool stm);
Move Next(Move *max_moves, unsigned cur, unsigned *top,
          bool *best_move_hashed, tt_entry entry,
          UC stm, bool captures_only);
void StoreResultInHash(int depth, short _alpha, short alpha, short beta,
                       unsigned legals,
                       bool beta_cutoff, Move best_move);
bool DetectOnlyMove(bool beta_cutoff, bool in_check,
                    unsigned move_cr, unsigned max_moves,
                    Move *moveList);
void ShowCurrentUciInfo();
void MoveToStr(Move m, bool stm, char *out);
void PonderHit();

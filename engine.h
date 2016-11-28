#include "hash.h"
#include "extern.h"
#include <iomanip>
#include <ctime>
#include <algorithm>                // for sorting of vectors
#include "Timer.h"





//--------------------------------
#define ENGINE_VERSION "087"

//#define DONT_SHOW_STATISTICS
//#define DONT_USE_FUTILITY
//#define DONT_USE_NULL_MOVE
//#define DONT_USE_DELTA_PRUNING
//#define DONT_USE_HISTORY
//#define DONT_USE_LMR
//#define DONT_USE_MATE_DISTANCE_PRUNING
//#define DONT_USE_PVS_IN_ROOT
//#define DONT_USE_RECAPTURE_EXTENSION
//#define DONT_USE_RANDOMNESS
//#define DONT_USE_ASPIRATION_WINDOWS
//#define CLEAR_HASH_TABLE_AFTER_EACH_MOVE
#define DONT_USE_ONE_REPLY_EXTENSION
//#define DONT_USE_LMP





//--------------------------------
#define UNUSED(x) (void)(x)

#define MAX_MOVES       256
#define RESIGN_VALUE    850
#define RESIGN_MOVES    3
#define MOVE_IS_NULL 0xFF





const int mate_score = K_VAL - (short)max_ply;
const bool all_moves = false;
const bool captures_only = true;
const unsigned nodes_to_check_stop = 7;
const unsigned init_max_moves = 2;  // any number greater than 1
const int moves_for_time_exact_mode = 8;
const u8 not_a_move = 0xFF;

#ifdef TUNE_PARAMETERS
extern std::vector <float> param;
#endif





enum {all_node = -1, pv_node = 0, cut_node = 1};





//--------------------------------
void InitEngine();
void Perft(i16 depth);
short Search(i16 depth, i16 alpha, i16 beta, i8 node_type);
short QSearch(i16 alpha, i16 beta);
void StorePV(Move m);
void UpdateStatistics(Move m, i16 dpt, u32 i);
short RootSearch(i16 depth, i16 alpha, i16 beta);
void RootMoveGen(bool ic);
void MainSearch();
void InitSearch();
void PrintFinalSearchResult();
void PrintCurrentSearchResult(i16 sc, char exclimation);
void InitTime();
bool ShowPV(u8 _ply);
void FindAndPrintForAmbiguousMoves(Move m);
bool MakeMoveFinaly(char *mov);
bool FenStringToEngine(char *fen);
bool DrawDetect();
void CheckForInterrupt();
void MakeNullMove();
void UnMakeNullMove();
bool NullMove(i16 depth, i16 beta, bool ic);
bool Futility(i16 depth, i16 beta);
bool DrawByRepetition();
void ShowFen();
void ReHash(u32 size_mb);
tt_entry* HashProbe(i16 depth, i16 *alpha, i16 beta);
bool MoveIsPseudoLegal(Move &m, bool stm);
Move Next(Move *move_array, u8 cur, u8 *top, tt_entry *entry,
          u8 stm, bool captures_only, bool in_check, Move prev_move);
void StoreResultInHash(i16 depth, i16 _alpha, i16 alpha, i16 beta, u8 legals,
                       bool beta_cutoff, Move best_move, bool one_reply);
void ShowCurrentUciInfo();
void MoveToStr(Move m, bool stm, char *out);
void PonderHit();
void ShowPVfailHighOrLow(Move m, short x, char exclimation);

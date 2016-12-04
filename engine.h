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






typedef u64 count_t;
typedef i8 node_type_t;

const node_type_t all_node = -1, pv_node = 0, cut_node = 1;

const score_t mate_score = king_value - (depth_t)max_ply;
const bool all_moves = false;
const bool captures_only = true;
const node_t nodes_to_check_stop = 7;
const movcr_t init_max_moves = 2;  // any number greater than 1
const depth_t moves_for_time_exact_mode = 8;
const move_flag_t not_a_move = 0xFF;
const size_t move_array_size = 256;
const score_t score_to_resign = 850;
const depth_t moves_to_resign = 3;
const coord_t is_null_move = 0xFF;

#ifdef TUNE_PARAMETERS
extern std::vector <float> param;
#endif





//--------------------------------
void InitEngine();
void Perft(depth_t depth);
score_t Search(depth_t depth, score_t alpha, score_t beta,
               node_type_t node_type);
score_t QSearch(score_t alpha, score_t beta);
void StorePV(move_c m);
void UpdateStatistics(move_c m, depth_t dpt, movcr_t move_cr);
score_t RootSearch(depth_t depth, score_t alpha, score_t beta);
void RootMoveGen(bool in_check);
void MainSearch();
void InitSearch();
void PrintFinalSearchResult();
void PrintCurrentSearchResult(score_t sc, char exclimation);
void InitTime();
bool ShowPV(depth_t _ply);
void FindAndPrintForAmbiguousMoves(move_c m);
bool MakeMoveFinaly(char *mov);
bool FenStringToEngine(char *fen);
bool DrawDetect();
void CheckForInterrupt();
void MakeNullMove();
void UnMakeNullMove();
bool NullMove(depth_t depth, score_t beta, bool ic);
bool Futility(depth_t depth, score_t beta);
bool DrawByRepetition();
void ShowFen();
void ReHash(size_t size_mb);
hash_entry_s* HashProbe(depth_t depth, score_t *alpha, score_t beta);
bool MoveIsPseudoLegal(move_c &m, bool stm);
move_c Next(move_c *move_array, movcr_t cur, movcr_t *top, hash_entry_s *entry,
          side_to_move_t stm, bool captures_only, bool in_check,
          move_c prev_move);
void StoreResultInHash(depth_t depth, score_t _alpha, score_t alpha,
                       score_t beta, movcr_t legals,
                       bool beta_cutoff, move_c best_move, bool one_reply);
void ShowCurrentUciInfo();
void MoveToStr(move_c m, bool stm, char *out);
void PonderHit();
void ShowPVfailHighOrLow(move_c m, score_t x, char exclimation);

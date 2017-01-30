#include "hash.h"
#include <iomanip>
#include <ctime>
#include <algorithm>  // for sorting of vectors
#include "Timer.h"





#define UNUSED(x) (void)(x)





class k2engine : public k2hash
{

public:

    k2engine();

protected:

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
    const static size_t move_array_size = 256;
    const score_t score_to_resign = 850;
    const depth_t moves_to_resign = 3;
    const coord_t is_null_move = 0xFF;

//extern std::vector <float> param;


public:


    double time_base, time_inc, time_remains, total_time_spent;
    depth_t moves_per_session, max_search_depth;
    bool time_command_sent;
    bool stop, infinite_analyze, busy;
    bool uci, xboard;
    bool pondering_in_process;
    node_t nodes, max_nodes_to_search, total_nodes;
    const char *engine_version;


protected:


    const char *stop_str;
    depth_t stop_ply;

    Timer timer;
    double time0;
    double time_spent, time_to_think;
    node_t q_nodes;
    depth_t root_ply;
    movcr_t max_root_moves, root_move_cr;
    count_t cut_cr, cut_num_cr[5], q_cut_cr, q_cut_num_cr[5];
    count_t null_probe_cr, null_cut_cr, hash_probe_cr;
    count_t hash_hit_cr, hash_cut_cr;
    count_t hash_best_move_cr, hash_cutoff_by_best_move_cr;
    count_t futility_probes, futility_hits;
    count_t killer1_probes, killer1_hits, killer2_probes, killer2_hits;
    depth_t finaly_made_moves, moves_remains;
    bool spent_exact_time;
    depth_t resign_cr;


    std::vector<std::pair<node_t, move_c> > root_moves;
    hash_table_c hash_table;


public:


    void InitEngine();
    void MainSearch();
    void Perft(depth_t depth);
    bool MakeMoveFinaly(char *mov);
    bool FenStringToEngine(char *fen);
    void ShowFen();
    void ReHash(size_t size_mb);
    void ClearHash();
    void EvalDebug()
    {
        k2eval::EvalDebug();
    }
    void PonderHit();
    bool WhiteIsOnMove()
    {
        return wtm == white;
    }


protected:


    score_t Search(depth_t depth, score_t alpha, score_t beta,
                   node_type_t node_type);
    score_t QSearch(score_t alpha, score_t beta);
    void StorePV(move_c m);
    void UpdateStatistics(move_c m, depth_t dpt, movcr_t move_cr);
    score_t RootSearch(depth_t depth, score_t alpha, score_t beta);
    void RootMoveGen(bool in_check);
    void InitSearch();
    void PrintFinalSearchResult();
    void PrintCurrentSearchResult(score_t sc, char exclimation);
    void InitTime();
    bool ShowPV(depth_t _ply);
    void FindAndPrintForAmbiguousMoves(move_c m);
    bool DrawDetect();
    void CheckForInterrupt();
    void MakeNullMove();
    void UnMakeNullMove();
    bool NullMove(depth_t depth, score_t beta, bool ic);
    bool Futility(depth_t depth, score_t beta);
    bool DrawByRepetition();
    bool HashProbe(depth_t depth, score_t *alpha, score_t beta,
                            hash_entry_s **entry);
    bool MoveIsPseudoLegal(move_c &m, bool stm);
    move_c Next(move_c *move_array, movcr_t cur, movcr_t *top,
                hash_entry_s *entry, side_to_move_t stm,
                bool captures_only, move_c prev_move);
    void StoreResultInHash(depth_t depth, score_t _alpha, score_t alpha,
                           score_t beta, movcr_t legals,
                           bool beta_cutoff, move_c best_moveW);
    void ShowCurrentUciInfo();
    void MoveToStr(move_c m, bool stm, char *out);
    void ShowPVfailHighOrLow(move_c m, score_t x, char exclimation);

};

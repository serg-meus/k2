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

    const eval_t mate_score = king_value - (depth_t)max_ply;
    const bool all_moves = false;
    const bool captures_only = true;
    const node_t nodes_to_check_stop = 7;
    const movcr_t init_max_moves = 2;  // any number greater than 1
    const depth_t moves_for_time_exact_mode = 8;
    const move_flag_t not_a_move = 0xFF;
    const static size_t move_array_size = 256;
    const eval_t eval_to_resign = 700;
    const depth_t moves_to_resign = 3;
    const coord_t is_null_move = 0xFF;


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


    const char *debug_variation;
    depth_t debug_ply;

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
    bool randomness;

    std::vector<std::pair<node_t, move_c> > root_moves;
    hash_table_c hash_table;

    // Structure for storing current state of engine
    struct state_s
    {
        coord_t to_coord;  // coordinate of last move destination
        priority_t priority;  // move priority assigned by move genererator
        bool in_check;
    };
    state_s eng_state[prev_states + max_ply]; // engine state for each ply
    state_s *state;  // pointer to engine state


public:


    void MainSearch();
    void Perft(depth_t depth);
    bool MakeMove(const char *mov);
    bool SetupPosition(const char *fen);
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


    eval_t Search(depth_t depth, eval_t alpha, eval_t beta,
                   node_type_t node_type);
    eval_t QSearch(eval_t alpha, eval_t beta);
    void StorePV(move_c m);
    void UpdateStatistics(move_c m, depth_t dpt, movcr_t move_cr,
                          hash_entry_s *entry);
    eval_t RootSearch(depth_t depth, eval_t alpha, eval_t beta);
    void RootMoveGen();
    void InitSearch();
    void PrintFinalSearchResult();
    void PrintCurrentSearchResult(eval_t sc, char exclimation);
    void InitTime();
    bool ShowPV(depth_t _ply);
    void FindAndPrintForAmbiguousMoves(move_c m);
    bool DrawDetect();
    void CheckForInterrupt();
    void MakeNullMove();
    void UnMakeNullMove();
    bool NullMove(depth_t depth, eval_t beta, bool ic);
    bool Futility(depth_t depth, eval_t beta);
    bool DrawByRepetition();
    bool HashProbe(depth_t depth, eval_t *alpha, eval_t beta,
                            hash_entry_s **entry);
    move_c NextMove(move_c *move_array, movcr_t cur, movcr_t *top,
                hash_entry_s *entry, bool captures_only, move_c prev_move);
    void StoreInHash(depth_t depth, eval_t score, move_c best_move,
                           hbound_t bound);
    void ShowCurrentUciInfo();
    void MoveToStr(move_c m, bool stm, char *out);
    void ShowPVfailHighOrLow(move_c m, eval_t x, char exclimation);
    bool GetFirstMove(move_c *move_array, movcr_t *max_moves,
                       hash_entry_s *entry, bool only_captures, move_c *ans);
    bool GetSecondMove(move_c *move_array, movcr_t *max_moves,
                       move_c prev_move, move_c *ans);
    size_t FindMaxMoveIndex(move_c *move_array, movcr_t max_moves,
                            movcr_t cur_move);

    void CorrectHashScore(eval_t *x, depth_t depth)
    {
        if(*x > mate_score && *x != infinite_score)
            *x += ply - depth - 1;
        else if(*x < -mate_score && *x != -infinite_score)
            *x -= ply - depth - 1;
    }

    void MakeMove(const move_c move)
    {
        state[ply].to_coord = move.to_coord;
        state[ply].priority = move.priority;
        k2hash::MakeMove(move);
    }

    void TakebackMove(const move_c move)
    {
        k2hash::TakebackMove(move);
    }
};

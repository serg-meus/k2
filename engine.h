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
    bool uci, xboard, enable_output;
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
    bool NullMovePruning(depth_t depth, eval_t beta, bool ic);
    bool DrawByRepetition();
    bool HashProbe(depth_t depth, eval_t *alpha, eval_t beta,
                            hash_entry_s **entry);
    move_c NextMove(move_c *move_array, movcr_t cur_move_cr,
                    movcr_t *max_moves, hash_entry_s *entry,
                    bool captures_only, move_c prev_move);
    void StoreInHash(depth_t depth, eval_t score, move_c best_move,
                           hbound_t bound);
    void ShowCurrentUciInfo();
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

    bool IsRecapture(const bool in_check)
    {
        return(!in_check && k2chess::state[ply - 1].captured_piece
                && k2chess::state[ply].captured_piece
                && state[ply].to_coord == state[ply - 1].to_coord
                && state[ply - 1].priority > bad_captures
                && state[ply].priority > bad_captures);
    }

    depth_t LateMoveReduction(const depth_t depth, move_c cur_move,
                              bool in_check,  movcr_t move_cr)
    {
        auto ans = 1;
        if(depth < 3 || cur_move.flag || in_check)
            ans = 0;
        else if(move_cr < 4)
            ans = 0;
        else if(get_type(b[cur_move.to_coord]) == pawn &&
                IsPasser(get_col(cur_move.to_coord), !wtm))
            ans = 0;
        else if(depth <= 4 && move_cr > 8)
            ans = 2;
        return ans;
    }

    bool DeltaPruning(eval_t alpha, move_c cur_move)
    {
        if(material[white]/100 + material[black]/100
                - pieces[white] - pieces[black] > 24 &&
                get_type(b[cur_move.to_coord]) != king &&
                !(cur_move.flag & is_promotion))
        {
            auto cur_eval = ReturnEval(wtm);
            auto capture = values[get_type(b[cur_move.to_coord])];
            auto margin = 100;
            return cur_eval + capture + margin < alpha;
        }
        else
            return false;
    }

    bool FutilityPruning(depth_t depth, eval_t beta, bool in_check)
    {
        if(depth > 2 || in_check || beta >= mate_score)
            return false;
        if(k2chess::state[ply].captured_piece != empty_square ||
                state[ply - 1].to_coord == is_null_move)
            return false;

        futility_probes++;
        auto margin = depth < 2 ? 185 : 255;
        auto score = ReturnEval(wtm);
        if(score <= margin + beta)
            return false;

        futility_hits++;
        return true;
    }

    bool MateDistancePruning(eval_t alpha, eval_t *beta)
    {
        auto mate_sc = king_value - ply;
        if(alpha >= mate_sc)
        {
            *beta = alpha;
            return true;
        }
        if(*beta <= -mate_sc)
            return true;
        return false;
    }
};

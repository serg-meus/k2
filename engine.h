#include "hash.h"
#include <iomanip>
#include <ctime>
#include "Timer.h"


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
    const size_t init_max_moves = 2;  // any number greater than 1
    const static size_t move_array_size = 256;
    const coord_t is_null_move = 0xFF;

    const depth_t null_move_min_depth = 2;
    const eval_t null_move_min_strength = 0;
    const depth_t null_move_switch_r_depth = 6;
    const depth_t null_move_max_r = 4;
    const depth_t null_move_min_r = 3;
    const depth_t lmr_min_depth = 3;
    const size_t lmr_max_move = 4;
    const size_t lmr_big_max_move = 7;
    const depth_t lmp_min_depth = 2;
    const size_t lmp_max_move = 6;
    const eval_t delta_pruning_margin = 100;
    const depth_t futility_max_depth = 3;
    const eval_t futility_marg0 = 185;
    const eval_t futility_marg1 = 220;
    const eval_t futility_marg2 = 255;
    const depth_t one_reply_min_depth = 2;
    const eval_t qs_min_material_to_drop = 2400;
    const depth_t qs_beta_exceed_to_drop = 250;
    const eval_t aspiration_margin = 32;
    const depth_t max_depth_for_single_move = 8;

    const size_t max_moves_to_shuffle = 4;
    const eval_t eval_to_resign = 1300;
    const depth_t moves_to_resign = 3;

    const size_t moves_for_time_exact_mode = 8;
    const depth_t default_moves_to_go = 32;
    const depth_t exact_time_base_divider = 4;
    const size_t min_moves_for_exact_time = 5;
    const depth_t time_to_think_divider = 2;
    const depth_t increment_time_divider = 4;

    const depth_t min_depth_to_show_uci_info = 6;
    const double search_stop_time_margin = 20000; //0.02s
    const depth_t ponder_time_factor = 5;


public:

    struct time_s
    {
        Timer timer;
        double time0;
        double time_spent, time_to_think;
        depth_t move_remains;
        bool spent_exact_time;
        double time_base, time_inc, time_remains, total_time_spent;
        depth_t moves_per_session, max_search_depth;
        node_t max_nodes_to_search;
        size_t moves_to_go;
        bool time_command_sent, infinite_analyze;
    };

    struct stats_s
    {
        node_t nodes, total_nodes;
        node_t q_nodes;
        count_t cut_cr, cut_num_cr[5], q_cut_cr, q_cut_num_cr[5];
        count_t null_probe_cr, null_cut_cr, hash_probe_cr;
        count_t hash_hit_cr, hash_cut_cr;
        count_t hash_best_move_cr, hash_cutoff_by_best_move_cr;
        count_t futility_probes, futility_hits;
        count_t killer1_probes, killer1_hits, killer2_probes, killer2_hits;
    };

    time_s time_control;
    stats_s stats;

    bool stop, busy;
    bool uci, xboard, enable_output;
    bool pondering_in_process;

    const char *engine_version;


protected:

    // Structure for storing current state of engine
    struct state_s
    {
        bool in_check;
        eval_vect eval;
    };

    const char *debug_variation;
    depth_t root_ply, debug_ply, halfmoves_made, resign_cr;
    size_t root_move_cr;
    bool randomness;

    std::vector<std::pair<node_t, move_c> > root_moves;
    std::vector<move_c> root_moves_to_search;
    hash_table_c hash_table;

    state_s eng_state[prev_states + max_ply]; // engine state for each ply
    state_s *state;  // pointer to engine state
    depth_t seed;  // randomness state (0-15)


public:


    void MainSearch();
    void Perft(const depth_t depth);
    bool MakeMove(const char *mov);
    bool SetupPosition(const char *fen);
    void ShowFen() const;
    void ReHash(size_t size_mb);
    void ClearHash();
    void PonderHit();

    void EvalDebug()
    {
        k2eval::EvalDebug(wtm);
    }

    bool WhiteIsOnMove() const
    {
        return wtm == white;
    }

    eval_t AddEval(const i32 x1, const i32 x2) const
    {
        if(x1 + x2 >= infinite_score)
            return infinite_score;
        else if(x1 + x2 <= -infinite_score)
            return -infinite_score;
        return x1 + x2;
    }


protected:


    eval_t Search(depth_t depth, eval_t alpha, eval_t beta,
                  const node_type_t node_type);
    eval_t QSearch(eval_t alpha, const eval_t beta);
    void StorePV(const move_c m);
    void UpdateStatistics(move_c m, depth_t dpt, size_t move_cr,
                          const hash_entry_s *entry);
    eval_t RootSearch(const depth_t depth, eval_t alpha, const eval_t beta);
    void GenerateRootMoves();
    void InitSearch();
    void PrintFinalSearchResult();
    void PrintCurrentSearchResult(eval_t sc, const u8 exclimation);
    void InitTime();
    bool DrawDetect(const bool in_check);
    bool CheckForInterrupt();
    void MakeNullMove();
    void UnMakeNullMove();
    bool NullMovePruning(depth_t depth, eval_t beta, bool ic);
    bool DrawByRepetition() const;
    bool HashProbe(const depth_t depth, eval_t * const alpha,
                   const eval_t beta, hash_entry_s **entry);
    move_c GetNextMove(move_c * const move_array, const size_t cur_move_cr,
                       size_t * const max_moves, move_c hash_best_move,
                       const bool hash_one_reply, const bool captures_only,
                       const move_c prev_move) const;
    void StoreInHash(const depth_t depth, eval_t score,
                     const move_c best_move, const hbound_t bound,
                     const bool one_reply);
    void ShowCurrentUciInfo();
    void ShowPVfailHighOrLow(move_c m, eval_t x, const u8 exclimation);
    bool GetFirstMove(move_c * const move_array,
                      size_t * const max_moves,
                      const move_c hash_best_move,
                      const bool hash_one_reply,
                      const bool only_captures,
                      move_c * const ans) const;
    bool GetSecondMove(move_c * const move_array, size_t * const max_moves,
                       move_c prev_move, move_c * const ans) const;
    size_t FindMaxMoveIndex(move_c * const move_array,
                            const size_t max_moves,
                            const size_t cur_move) const;
    bool CanFinishMainSearch(const eval_t x, const eval_t prev_x);
    void CheckForResign(const eval_t x);
    bool IsInCheck() const;
    bool GetRootSearchBounds(const eval_t x,
                             eval_t *alpha, eval_t *beta) const;

    void CorrectHashScore(eval_t * const x, const depth_t depth) const
    {
        if(*x > mate_score && *x != infinite_score)
            *x += ply - depth - 1;
        else if(*x < -mate_score && *x != -infinite_score)
            *x -= ply - depth - 1;
    }

    void MakeMove(const move_c move)
    {
        k2hash::MakeMove(move);
        state[ply].eval = state[ply - 1].eval + FastEval(wtm, move);
    }

    void TakebackMove(const move_c move)
    {
        k2hash::TakebackMove(move);
        if(k2chess::state[ply + 1].attacks_updated)
            TakebackAttacks();
    }

    bool IsRecapture() const
    {
        const auto &cur = k2chess::state[ply];
        const auto &prev = k2chess::state[ply - 1];
        return(cur.captured_piece && prev.captured_piece &&
               cur.move.to_coord == prev.move.to_coord && cur.move.priority >
               bad_captures && prev.move.priority > bad_captures);
    }

    depth_t LateMoveReduction(const depth_t depth, const move_c cur_move,
                              const bool in_check, const size_t move_cr,
                              const node_type_t node_type) const
    {
        if(depth < lmr_min_depth || cur_move.flag ||
                in_check || move_cr < lmr_max_move)
            return 0;
        if(get_type(b[cur_move.to_coord]) == pawn &&
                IsPasser(get_col(cur_move.to_coord), !wtm))
            return 0;
        return 1 + (node_type != pv_node && move_cr > lmr_big_max_move);
    }

    bool DeltaPruning(const eval_t alpha, const move_c cur_move) const
    {
        if(quantity[black][rook] == 0 && quantity[white][rook] == 0 &&
                quantity[black][queen] == 0 && quantity[white][queen] == 0)
            return false;
        if(get_type(b[cur_move.to_coord]) == king ||
                (cur_move.flag & is_promotion))
            return false;

        const auto cur_eval = GetEvalScore(wtm, state[ply].eval);
        const auto capture = values[get_type(b[cur_move.to_coord])];
        const auto margin = delta_pruning_margin;
        return cur_eval + capture + margin < alpha;
    }

    bool FutilityPruning(const depth_t depth, const eval_t beta,
                         const bool in_check)
    {
        const eval_t margin[] = {futility_marg0, futility_marg1,
                                  futility_marg2, futility_marg2};
        if(depth > futility_max_depth || in_check || beta >= mate_score)
            return false;
        if(k2chess::state[ply].captured_piece != empty_square ||
                k2chess::state[ply - 1].move.to_coord == is_null_move)
            return false;
        if((material[black] == 0 || material[white] == 0))
        {
            if(material[black] + material[white] != 1 ||
                    quantity[black][pawn] + quantity[white][pawn] == 0)
            return false;
        }
        stats.futility_probes++;
        auto score = GetEvalScore(wtm, state[ply].eval);
        if(score <= margin[depth] + beta)
            return false;

        stats.futility_hits++;
        return true;
    }

    bool MateDistancePruning(const eval_t alpha, eval_t * const beta) const
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

    void UpdateAttackTables()
    {
        auto delta = 0;
        if(k2chess::state[ply - 1].move.to_coord == is_null_move)
            delta++;
        if(delta && k2chess::state[ply - 2].move.to_coord == is_null_move)
            delta++;
        if(delta == 1)
            wtm = !wtm;
        ply -= delta;

        k2chess::UpdateAttackTables(k2chess::state[ply + delta].move);

        if(delta == 1)
            wtm = !wtm;
        ply += delta;
    }
};

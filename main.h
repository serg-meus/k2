#include "engine.h"
#include <thread>
#include <fstream>


int main(int argc, char* argv[]);


class k2main : public k2engine
{

public:


    k2main();
    void start();

    void LevelCommand(const std::string in);
    void MemoryCommand(const std::string in);

    void RunUnitTests()
    {
        k2movegen::RunUnitTests();
        SetupPosition(start_position);
    }


protected:


    bool force;
    bool quit;
    bool pondering_enabled;
    bool use_thread;

    std::thread thr;

    typedef void(k2main::*method_ptr)(const std::string);

    struct command_s
    {
        std::string command;
        method_ptr mptr;
    };

    std::vector<std::pair<std::string, float> > training_positions;
    double tuning_factor = 1.3;

    std::vector<std::pair<std::string, eval_t *> > eval_params =
    {
        {"pawn_val_end", &material_values_end[pawn]},
        {"knight_val_opn", &material_values_opn[knight]},
        {"knight_val_end", &material_values_end[knight]},
        {"bishop_val_opn", &material_values_opn[bishop]},
        {"bishop_val_end", &material_values_end[bishop]},
        {"rook_val_opn", &material_values_opn[rook]},
        {"rook_val_end", &material_values_end[rook]},
        {"queen_val_opn", &material_values_opn[queen]},
        {"queen_val_end", &material_values_end[queen]},
        {"pawn_dbl_iso_opn", &pawn_dbl_iso_opn},
        {"pawn_dbl_iso_end", &pawn_dbl_iso_end},
        {"pawn_iso_opn", &pawn_iso_opn},
        {"pawn_iso_end", &pawn_iso_end},
        {"pawn_dbl_opn", &pawn_dbl_opn},
        {"pawn_dbl_end", &pawn_dbl_end},
        {"pawn_hole_opn", &pawn_hole_opn},
        {"pawn_hole_end", &pawn_hole_end},
        {"pawn_gap_opn", &pawn_gap_opn},
        {"pawn_gap_end", &pawn_gap_end},
        {"pawn_king_tropism1", &pawn_king_tropism1},
        {"pawn_king_tropism2", &pawn_king_tropism2},
        {"pawn_king_tropism3", &pawn_king_tropism3},
        {"pawn_pass_1", &pawn_pass_1},
        {"pawn_pass_2", &pawn_pass_2},
        {"pawn_pass_3", &pawn_pass_3},
        {"pawn_pass_4", &pawn_pass_4},
        {"pawn_pass_5", &pawn_pass_5},
        {"pawn_pass_6", &pawn_pass_6},
        {"pawn_blk_pass_1", &pawn_blk_pass_1},
        {"pawn_blk_pass_2", &pawn_blk_pass_2},
        {"pawn_blk_pass_3", &pawn_blk_pass_3},
        {"pawn_blk_pass_4", &pawn_blk_pass_4},
        {"pawn_blk_pass_5", &pawn_blk_pass_5},
        {"pawn_blk_pass_6", &pawn_blk_pass_6},
        {"pawn_pass_opn_divider", &pawn_pass_opn_divider},
        {"pawn_pass_connected", &pawn_pass_connected},
        {"pawn_unstoppable_1", &pawn_unstoppable_1},
        {"pawn_unstoppable_2", &pawn_unstoppable_2},
        {"king_saf_no_shelter", &king_saf_no_shelter},
        {"king_saf_no_queen", &king_saf_no_queen},
        {"king_saf_attack1", &king_saf_attack1},
        {"king_saf_attack2", &king_saf_attack2},
        {"king_saf_central_files", &king_saf_central_files},
        {"rook_on_last_rank", &rook_on_last_rank},
        {"rook_semi_open_file", &rook_semi_open_file},
        {"rook_open_file", &rook_open_file},
        {"rook_max_pawns_for_open_file", &rook_max_pawns_for_open_file},
        {"bishop_pair", &bishop_pair},
        {"mob_queen", &mob_queen},
        {"mob_rook", &mob_rook},
        {"mob_bishop", &mob_bishop},
        {"mob_knight", &mob_knight},
        {"imbalance_king_in_corner", &imbalance_king_in_corner},
        {"imbalance_multicolor1", &imbalance_multicolor1},
        {"imbalance_multicolor2", &imbalance_multicolor2},
        {"imbalance_multicolor3", &imbalance_multicolor3},
        {"imbalance_no_pawns", &imbalance_no_pawns},
        {"side_to_move_bonus", &side_to_move_bonus},
    };


    bool ExecuteCommand(const std::string in);
    bool LooksLikeMove(const std::string in) const;
    void NewCommand(const std::string in);
    void SetboardCommand(const std::string in);
    void QuitCommand(const std::string in);
    void PerftCommand(const std::string in);
    void GoCommand(const std::string in);
    void ForceCommand(const std::string in);
    void SetNodesCommand(const std::string in);
    void SetTimeCommand(const std::string in);
    void SetDepthCommand(const std::string in);
    void Unsupported(const std::string in);
    void GetFirstArg(const std::string in, std::string * const first_word,
                     std::string * const all_the_rest) const;
    void ProtoverCommand(const std::string in);
    void StopEngine();
    void StopCommand(const std::string in);
    void ResultCommand(const std::string in);
    void XboardCommand(const std::string in);
    void TimeCommand(const std::string in);
    void EvalCommand(const std::string in);
    void TestCommand(const std::string in);
    void FenCommand(const std::string in);
    void UciCommand(const std::string in);
    void SetOptionCommand(const std::string in);
    void IsReadyCommand(const std::string in);
    void PositionCommand(const std::string in);
    void ProcessMoveSequence(const std::string in);
    void UciGoCommand(const std::string in);
    void EasyCommand(const std::string in);
    void HardCommand(const std::string in);
    void PonderhitCommand(const std::string in);
    void AnalyzeCommand(const std::string in);
    void ExitCommand(const std::string in);
    void SetvalueCommand(const std::string in);
    void OptionCommand(const std::string in);
    void PostCommand(const std::string in);
    void NopostCommand(const std::string in);
    void SeedCommand(const std::string in);
    void TuningLoadCommand(const std::string in);
    void TuningResultCommand(const std::string in);
    void TuneParamCommand(const std::string in);

    bool SetParamValue(const std::string param, double val);
    bool SetPstValue(const std::string param, double val);
    double GetEvalError();

    bool test_perft(const char *pos, depth_t depth, node_t node_cr)
    {
        stats.nodes = 0;
        SetupPosition(pos);
        Perft(depth);
        stats.total_nodes += stats.nodes;
        if(stats.nodes != node_cr)
            std::cout << "failed at " << pos << std::endl;
        return stats.nodes == node_cr;
    }

    bool test_search(const char *pos, depth_t depth, const char *best_move,
                     bool avoid_move)
    {
        bool ans = true;
        SetupPosition(pos);
        ClearHash();
        time_control.max_search_depth = depth;
        time_control.infinite_analyze = true;
        MainSearch();
        char move_str[6];
        MoveToCoordinateNotation(pv[0].moves[0], move_str);
        if(strcmp(move_str, best_move) == 0)
            ans = !avoid_move;
        else
            ans = avoid_move;

        if(!ans)
            std::cout << "failed at " << pos << std::endl;
        return ans;
    }

    double sigmoid(const double s)
    {
        return 1/(1 + pow(10, -tuning_factor*s/400));
    }
};

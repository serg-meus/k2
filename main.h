#include "engine.h"
#include <thread>
#include <fstream>
#include <unordered_map>


int main(int argc, char* argv[]);


class k2main : public k2engine
{

public:


    k2main();
    void start();

    void LevelCommand(const std::string &in);
    void MemoryCommand(const std::string &in);

    void RunUnitTests()
    {
        k2hash::RunUnitTests();
        SetupPosition(start_position);
    }


protected:


    bool force;
    bool quit;
    bool pondering_enabled;
    bool use_thread;

    std::thread thr;

    typedef void(k2main::*method_ptr)(const std::string &);

    struct tune_param_s
    {
        std::string name;
        bool is_mid;
        eval_t left_arg;
        eval_t right_arg;
        double left_err;
        double right_err;
    };

    enum class tune_flag {calc_both, calc_left, calc_right};

    struct parsed_pos_s
    {
        bool wtm;
        piece_t b[board_size];
        coord_t coords[sides][max_pieces_one_side];
        attack_t attacks[sides][board_size], exist_mask[sides];
        attack_t type_mask[sides][piece_types + 1], slider_mask[sides];
        tiny_count_t directions[sides][max_pieces_one_side][max_rays];
        tiny_count_t sum_directions[sides][max_pieces_one_side];
        piece_val_t material[sides];
        tiny_count_t pieces[sides];
        tiny_count_t quantity[sides][piece_types + 1];
        rank_t p_max[board_width + 2][sides], p_min[board_width + 2][sides];
        castle_t castling_rights;

        double result;

    };
    bool ExecuteCommand(const std::string &in);
    bool LooksLikeMove(const std::string &in) const;
    void NewCommand(const std::string &in);
    void SetboardCommand(const std::string &in);
    void QuitCommand(const std::string &in);
    void PerftCommand(const std::string &in);
    void GoCommand(const std::string &in);
    void ForceCommand(const std::string &in);
    void SetNodesCommand(const std::string &in);
    void SetTimeCommand(const std::string &in);
    void SetDepthCommand(const std::string &in);
    void Unsupported(const std::string &in);
    std::string GetFirstArg(const std::string &in,
                            std::string * const all_the_rest) const;
    void ProtoverCommand(const std::string &in);
    void StopEngine();
    void StopCommand(const std::string &in);
    void ResultCommand(const std::string &in);
    void XboardCommand(const std::string &in);
    void TimeCommand(const std::string &in);
    void EvalCommand(const std::string &in);
    void TestCommand(const std::string &in);
    void FenCommand(const std::string &in);
    void UciCommand(const std::string &in);
    void SetOptionCommand(const std::string &in);
    void IsReadyCommand(const std::string &in);
    void PositionCommand(const std::string &in);
    void ProcessMoveSequence(const std::string &in);
    void UciGoCommand(const std::string &in);
    void PonderhitCommand(const std::string &in);
    void AnalyzeCommand(const std::string &in);
    void ExitCommand(const std::string &in);
    void SetvalueCommand(const std::string &in);
    void OptionCommand(const std::string &in);
    void PostCommand(const std::string &in);
    void NopostCommand(const std::string &in);
    void SeedCommand(const std::string &in);

    bool test_perft(const char *pos, const depth_t depth, const node_t node_cr)
    {
        stats.nodes = 0;
        SetupPosition(pos);
        Perft(depth);
        stats.total_nodes += stats.nodes;
        if(stats.nodes != node_cr)
            std::cout << "failed at " << pos << std::endl;
        return stats.nodes == node_cr;
    }

    bool test_search(const char *pos, const depth_t depth,
                     const char *best_move, const bool avoid_move)
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
};

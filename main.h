#include "engine.h"
#include <thread>


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

    bool test_perft(const char *pos, depth_t depth, node_t node_cr)
    {
        nodes = 0;
        SetupPosition(pos);
        Perft(depth);
        total_nodes += nodes;
        if(nodes != node_cr)
            std::cout << "failed at " << pos << std::endl;
        return nodes == node_cr;
    }

    bool test_search(const char *pos, depth_t depth, const char *best_move,
                     bool avoid_move)
    {
        bool ans = true;
        SetupPosition(pos);
        ClearHash();
        max_search_depth = depth;
        infinite_analyze = true;
        MainSearch();
        char move_str[6];
        MoveToStr(pv[0].moves[0], wtm, move_str);
        if(!CheckBoardConsistency())
            ans = false;
        else if(strcmp(move_str, best_move) == 0)
            ans = !avoid_move;
        else
            ans = avoid_move;

        if(!ans)
            std::cout << "failed at " << pos << std::endl;
        return ans;
    }
};

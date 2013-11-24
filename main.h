#include "engine.h"
//--------------------------------
#define ENGINE_VERSION "038"
//--------------------------------
#ifdef NDEBUG
    #define USE_THREAD_FOR_INPUT
#endif // NDEBUG

#ifdef USE_THREAD_FOR_INPUT
    #include <thread>                                                   // for std::thread
#endif // USE_THREAD_FOR_INPUT


//--------------------------------
extern double timeRemains;
extern double timeBase;
extern unsigned movesPerSession;
extern double timeInc;
extern unsigned timeMaxNodes;
extern unsigned timeMaxPly;
extern bool stop;
extern bool _abort_;
extern bool busy;
extern UQ totalNodes;
extern double totalTimeSpent;
extern bool timeCommandSent;

#ifdef TUNE_PARAMETERS
    extern std::vector <float> tuning_vec;
#endif // TUNE_PARAMETERS


//--------------------------------
struct cmdStruct
{
    std::string command;
    void (*foo)(std::string);
};

//--------------------------------
int main(int argc, char* argv[]);
bool CmdProcess(std::string in);
bool LooksLikeMove(std::string in);
void NewCommand(std::string in);
void SetboardCommand(std::string in);
void QuitCommand(std::string in);
void PerftCommand(std::string in);
void GoCommand(std::string in);
void LevelCommand(std::string in);
void ForceCommand(std::string in);
void SetNodesCommand(std::string in);
void SetTimeCommand(std::string in);
void SetDepthCommand(std::string in);
void Unsupported(std::string in);
void GetFirstArg(std::string in, std::string *firstWord, std::string *remainingWords);
void ProtoverCommand(std::string in);
void StopEngine();
void InterrogationCommand(std::string in);
void ResultCommand(std::string in);
void TimeCommand(std::string in);
void EvalCommand(std::string in);
void TestCommand(std::string in);

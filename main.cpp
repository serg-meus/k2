#include "main.h"
//--------------------------------
// K2, the chess engine
// Author: Sergey Meus (serg_meus@mail.ru)
// Krasnoyarsk Kray, Russia
// Copyright 2012-2014
//--------------------------------

cmdStruct commands[]
{
//   Command        Function
    {"new",         NewCommand},
    {"setboard",    SetboardCommand},
    {"set",         SetboardCommand},
    {"quit",        QuitCommand},
    {"q",           QuitCommand},
    {"perft",       PerftCommand},
    {"go",          GoCommand},
    {"level",       LevelCommand},
    {"force",       ForceCommand},
    {"sn",          SetNodesCommand},
    {"st",          SetTimeCommand},
    {"sd",          SetDepthCommand},
    {"protover",    ProtoverCommand},
    {"?",           StopCommand},
    {"result",      ResultCommand},
    {"time",        TimeCommand},
    {"eval",        EvalCommand},
    {"test",        TestCommand},
    {"fen",         FenCommand},
    {"xboard",      XboardCommand},
    {"easy",        EasyCommand},
    {"hard",        HardCommand},
    {"memory",      MemoryCommand},
    {"analyze",     AnalyzeCommand},
    {"exit",        ExitCommand},

    {"undo",        Unsupported},
    {"remove",      Unsupported},
    {"computer",    Unsupported},
    {"random",      Unsupported},
    {"post",        Unsupported},
    {"nopost",      Unsupported},
    {"random",      Unsupported},

    {"otim",        Unsupported},
    {"accepted",    Unsupported},
    {".",           Unsupported},
    {"",            Unsupported},

    {"uci",         UciCommand},
    {"setoption",   SetOptionCommand},
    {"isready",     IsReadyCommand},
    {"position",    PositionCommand},
    {"ucinewgame",  NewCommand},
    {"stop",        StopCommand},
    {"ponderhit",   PonderhitCommand},
    {"setvalue",    SetvalueCommand},
};

bool force  = false;
bool quit   = false;
bool xboard = false;
bool uci    = false;
bool ponder = false;

#ifdef USE_THREAD_FOR_INPUT
    std::thread t;                                                      // for compilers with C++11 support
#endif // USE_THREAD_FOR_INPUT                                          // under Linux -pthread must be used for gcc linker

//--------------------------------
int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

#ifdef TUNE_PARAMETERS
    for(int i = 0; i < NPARAMS; ++i)
        param.push_back(0);
#endif
    InitEngine();

    timeMaxPly      = max_ply;
    timeRemains     = 300000000;
    timeBase        = 300000000;
    timeInc         = 0;
    movesPerSession = 0;
    timeMaxNodes    = 0;
    timeCommandSent = false;

    char in[0x4000];
    while(!quit)
   {
        if(!std::cin.getline(in, sizeof(in), '\n'))
            std::cin.clear();

        if(CmdProcess((std::string)in))
        {
            // NiCheGoNeDeLaYem!
        }
        else if(!busy && LooksLikeMove((std::string)in))
        {
            if(!MakeMoveFinaly(in))
                std::cout << "Illegal move" << std::endl;
            else if(!force)
            {
#ifdef USE_THREAD_FOR_INPUT
                if(t.joinable())
                    t.join();
                t = std::thread(MainSearch);
#else
                MainSearch();
#endif // USE_THREAD_FOR_INPUT
            }
        }
        else
            std::cout << "Unknown command: " << in << std::endl;
    }
    if(busy)
        StopEngine();
}

//--------------------------------
bool CmdProcess(std::string in)
{
    std::string firstWord, remains;

    GetFirstArg(in, &firstWord, &remains);

//    std::cout << firstWord << std::endl;
//    std::cout << remains << std::endl;

    unsigned i;
    for(i = 0; i < sizeof(commands) / sizeof(cmdStruct); ++i)
    {
        if(firstWord == commands[i].command)
        {
            commands[i].foo(remains);
            return true;
        }
    }
    return false;
}

//--------------------------------
void GetFirstArg(std::string in, std::string (*firstWord), std::string (*remainingWords))
{
    if(in.empty())
        return;
    std::string delimiters = " ;,\t\r\n";
    *firstWord = in;
    int firstSymbol = firstWord->find_first_not_of(delimiters);
    if(firstSymbol == -1)
        firstSymbol = 0;
    *firstWord = firstWord->substr(firstSymbol, (int)firstWord->size());
    int secondSymbol = firstWord->find_first_of(delimiters);
    if(secondSymbol == -1)
        secondSymbol = (int)firstWord->size();

    *remainingWords = firstWord->substr(secondSymbol, (int)firstWord->size());
    *firstWord = firstWord->substr(0, secondSymbol);
}

//--------------------------------
bool LooksLikeMove(std::string in)
{
    if(in.size() < 4 || in.size() > 5)
        return false;
    if(in.at(0) > 'h' || in.at(0) < 'a')
        return false;
    if(in.at(1) > '8' || in.at(1) < '0')
        return false;
    if(in.at(2) > 'h' || in.at(2) < 'a')
        return false;
    if(in.at(3) > '8' || in.at(3) < '0')
        return false;

    return true;

}

//--------------------------------
void StopEngine()
{
#ifdef USE_THREAD_FOR_INPUT
        stop = true;
        t.join();
#endif // USE_THREAD_FOR_INPUT
}

//--------------------------------
void NewCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        StopEngine();
    force = false;
    ponder = false;
    if(!xboard && !uci)
    {
        std::cout
                << "( Total node count: " << totalNodes
                << ", total time spent: " << totalTimeSpent / 1000000.0
                << " )" << std::endl
                << "( MNPS = " << totalNodes / (totalTimeSpent + 1e-5)
                << " )" << std::endl;
    }
    InitEngine();
}

//-------------------------------
void SetboardCommand(std::string in)
{
    if(busy)
        return;

    int firstSymbol = in.find_first_not_of(" \t");
    in.erase(0, firstSymbol);

    if(!FenStringToEngine((char *)in.c_str()))
        std::cout << "Illegal position" << std::endl;
    else if(analyze && xboard)
        AnalyzeCommand(in);
}

//--------------------------------
void QuitCommand(std::string in)
{
    UNUSED(in);
#ifdef USE_THREAD_FOR_INPUT
    if(busy || t.joinable())
        StopEngine();
#endif // USE_THREAD_FOR_INPUT

    quit = true;
}

//--------------------------------
void PerftCommand(std::string in)
{
    if(busy)
        return;
    Timer t;
    double tick1, tick2, deltaTick;
    t.start();
    tick1 = t.getElapsedTimeInMicroSec();

    nodes = 0;
    timeMaxPly = atoi(in.c_str());
    Perft(timeMaxPly);
    timeMaxPly = max_ply;
    tick2 = t.getElapsedTimeInMicroSec();
    deltaTick = tick2 - tick1;

    std::cout << std::endl << "nodes = " << nodes << std::endl
        << "dt = " << deltaTick / 1000000. << std::endl
        << "Mnps = " << nodes / (deltaTick + 1) << std::endl << std::endl;
}

//--------------------------------
void GoCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        return;

    if(uci)
        UciGoCommand(in);
    else
        force = false;
#ifdef USE_THREAD_FOR_INPUT
    if(t.joinable())
        t.join();
    t = std::thread(MainSearch);
#else
    MainSearch();
#endif // USE_THREAD_FOR_INPUT

}

//--------------------------------
void LevelCommand(std::string in)
{
    if(busy)
        return;
    std::string arg1, arg2, arg3;
    GetFirstArg(in, &arg1, &arg2);
    GetFirstArg(arg2, &arg2, &arg3);
    double base, mps, inc;
    mps = atoi(arg1.c_str());

    int colon = arg2.find(':');
    if(colon != -1)
    {
        arg2.at(colon) = '.';
        int size_of_seconds = arg2.size() - colon - 1;
        base = atof(arg2.c_str());
        if(base < 0)
            base = -base;
        if(size_of_seconds == 1)
            base = 0.1*(base - (int)base) + (int)base;
        int floorBase = (int)base;
        base = (base - floorBase)*100/60 + floorBase;

    }
    else
        base = atof(arg2.c_str());

    inc = atof(arg3.c_str());

    timeBase        = 60*1000000.*base;
    timeInc         = 1000000*inc;
    movesPerSession         = mps;
    timeRemains     = timeBase;
    timeMaxNodes    = 0;
    timeMaxPly      = max_ply;
}

//--------------------------------
void ForceCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        StopEngine();
    force = true;
}

//--------------------------------
void SetNodesCommand(std::string in)
{
    if(busy)
        return;
    timeBase     = 0;
    movesPerSession      = 0;
    timeInc      = 0;
    timeMaxNodes = atoi(in.c_str());
    timeMaxPly   = max_ply;
}

//--------------------------------
void SetTimeCommand(std::string in)             //<< NB: wrong
{
    if(busy)
        return;
    timeBase     = 0;
    movesPerSession      = 1;
    timeInc      = atof(in.c_str())*1000000.;
    timeMaxNodes = 0;
    timeMaxPly   = max_ply;
    timeRemains  = 0;
}

//--------------------------------
void SetDepthCommand(std::string in)
{
    if(busy)
        return;
    timeMaxPly   = atoi(in.c_str());
}

//--------------------------------
void ProtoverCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        return;
    xboard  = true;
    uci     = false;

    std::cout << "feature "
            "myname=\"K2 v." ENGINE_VERSION "\" "
            "setboard=1 "
            "analyze=1 "
            "san=0 "
            "colors=0 "
            "pause=0 "
            "usermove=0 "
            "time=1 "
            "draw=0 "
            "sigterm=0 "
            "sigint=0 "
            "memory=1 "
            "done=1 " << std::endl;
}

//--------------------------------
void StopCommand(std::string in)
{
    UNUSED(in);
#ifdef USE_THREAD_FOR_INPUT
    if(busy)
    {
        stop = true;
        t.join();
    }
#endif // USE_THREAD_FOR_INPUT
}

//--------------------------------
void ResultCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        StopEngine();
}

//--------------------------------
void TimeCommand(std::string in)
{
    if(busy)
    {
        std::cout << "telluser time command recieved while engine is busy" << std::endl;
        return;
    }
//    std::cout << "telluser " << in << std::endl;
    double tb = atof(in.c_str()) * 10000;
    timeRemains = tb;
    timeCommandSent = true;
}

//--------------------------------
void EvalCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        return;
    std::cout << "Fast eval: " << valOpn << " / "
        << valEnd << std::endl;
    short x = Eval(/*-INF, INF*/);
    std::cout << "Eval: " << (wtm ? -x : x) << std::endl;
    std::cout << "(positive is white advantage)" << std::endl;
#ifndef DONT_USE_HASH_TABLE
    std::cout << "hash key = 0x" << std::hex << hash_key << std::dec << std::endl;
#endif // DONT_USE_HASH_TABLE

}

//--------------------------------
void TestCommand(std::string in)
{
    UNUSED(in);
}

//--------------------------------
void FenCommand(std::string in)
{
    UNUSED(in);
    ShowFen();
}

//--------------------------------
void XboardCommand(std::string in)
{
    UNUSED(in);
    xboard  = true;
    uci     = false;
    std::cout << "( build date: " << __DATE__ << " )" << std::endl;
}

//--------------------------------
void Unsupported(std::string in)
{
    UNUSED(in);
}

//--------------------------------
void UciCommand(std::string in)
{
    UNUSED(in);
    uci = true;
    std::cout << "id name K2 v." ENGINE_VERSION << std::endl;
    std::cout << "id author Sergey Meus" << std::endl;
    std::cout << "option name Hash type spin default 64 min 2 max 1024" << std::endl;
    std::cout << "uciok" << std::endl;
}

//--------------------------------
void SetOptionCommand(std::string in)
{
    std::string arg1, arg2;
    GetFirstArg(in, &arg1, &arg2);
    if(arg1 != "name")
        return;
    GetFirstArg(arg2, &arg1, &arg2);

    if(arg1 == "Hash" || arg1 == "hash")
    {
        GetFirstArg(arg2, &arg1, &arg2);
        if(arg1 != "value")
            return;
        GetFirstArg(arg2, &arg1, &arg2);
        int size_MB = atoi(arg1.c_str());
        ReHash(size_MB);
    }
}

//--------------------------------
void IsReadyCommand(std::string in)
{
    UNUSED(in);
/*    while(busy)
    {
        // Do Nothing
    }
*/
    std::cout << "readyok" << std::endl;
}

//--------------------------------
void PositionCommand(std::string in)
{

    std::string arg1, arg2;
    GetFirstArg(in, &arg1, &arg2);

    if(arg1 == "fen")
    {
        std::string fenstring;
        int beg = arg2.find_first_not_of(" \t");
        fenstring = arg2.substr(beg, arg2.size());
        if(!FenStringToEngine((char *)fenstring.c_str()))
        {
            std::cout << "Illegal position" << std::endl;
            return;
        }
    }// if(arg1 == "fen"
    else
        InitEngine();

    int moves = arg2.find("moves");
    if(moves == -1)
        return;

    std::string mov_seq = arg2.substr(moves + 5, in.size());
    int beg = mov_seq.find_first_not_of(" \t");
    mov_seq = mov_seq.substr(beg, mov_seq.size());

    ProcessMoveSequence(mov_seq);
}

//--------------------------------
void ProcessMoveSequence(std::string in)
{
    std::string arg1, arg2;
    arg1 = in;
    while(true)
    {
        GetFirstArg(arg1, &arg1, &arg2);
        if(arg1.empty())
            break;
//        std::cout << arg1 << std::endl;
        if(!MakeMoveFinaly((char *)arg1.c_str()))
            break;
        arg1 = arg2;
    }
}

//--------------------------------
void UciGoCommand(std::string in)
{
    ponder = false;
    std::string arg1, arg2;
    arg1 = in;
    while(true)
    {
        GetFirstArg(arg1, &arg1, &arg2);
//        std::cout << arg1 << std::endl;
        if(arg1.empty())
            break;
        if(arg1 == "infinite")
        {
            analyze = true;
            break;
        }

        else if(arg1 == "wtime" || arg1 == "btime")
        {
            char clr = arg1[0];
            GetFirstArg(arg2, &arg1, &arg2);
            if((clr == 'w' && wtm) || (clr == 'b' && !wtm))
            {
                timeBase        = 1000.*atof(arg1.c_str());
                timeRemains     = timeBase;
                timeMaxNodes    = 0;
                timeMaxPly      = max_ply;
                timeCommandSent = true;                                 // crutch: engine must know that time changed by GUI
            }
            arg1 = arg2;
        }
        else if(arg1 == "winc" || arg1 == "binc")
        {
            char clr = arg1[0];
            GetFirstArg(arg2, &arg1, &arg2);
            if((clr == 'w' && wtm) || (clr == 'b' && !wtm))
                timeInc         = 1000.*atof(arg1.c_str());
            arg1 = arg2;
        }
        else if(arg1 == "movestogo")
        {
            GetFirstArg(arg2, &arg1, &arg2);
            movesPerSession = atoi(arg1.c_str());
            arg1 = arg2;
        }
        else if(arg1 == "movetime")
        {
            GetFirstArg(arg2, &arg1, &arg2);
            timeBase     = 0;
            movesPerSession      = 1;
            timeInc      = atof(arg1.c_str())*1000;
            timeMaxNodes = 0;
            timeMaxPly   = max_ply;

            arg1 = arg2;
        }
        else if(arg1 == "depth")
        {
            GetFirstArg(arg2, &arg1, &arg2);
            timeMaxPly = atoi(arg1.c_str());
            timeBase        = INFINITY;
            timeRemains     = timeBase;
            timeMaxNodes    = 0;

            arg1 = arg2;
        }
        else if(arg1 == "nodes")
        {
            GetFirstArg(arg2, &arg1, &arg2);
            timeBase        = INFINITY;
            timeRemains     = timeBase;
            timeMaxNodes    = atoi(arg1.c_str());
            timeMaxPly      = max_ply;

            arg1 = arg2;
        }
        else if(arg1 == "ponder")
        {
            ponder = true;
            arg1 = arg2;
        }
    }//while(true
}

//--------------------------------
void EasyCommand(std::string in)
{
    UNUSED(in);
//    ponder  = false;
}

//--------------------------------
void HardCommand(std::string in)
{
    UNUSED(in);
//    ponder  = true;
}

//--------------------------------
void PonderhitCommand(std::string in)
{
    UNUSED(in);
    PonderHit();
}

//--------------------------------
void MemoryCommand(std::string in)
{
    std::string arg1, arg2;
    arg1 = in;
    GetFirstArg(arg2, &arg1, &arg2);
    int size_MB = atoi(arg1.c_str());
    ReHash(size_MB);
}

//--------------------------------
void AnalyzeCommand(std::string in)
{
    UNUSED(in);
    force = false;
    analyze = true;

    #ifdef USE_THREAD_FOR_INPUT
    if(t.joinable())
        t.join();
    t = std::thread(MainSearch);
    #else
    MainSearch();
    #endif // USE_THREAD_FOR_INPUT
}

//--------------------------------
void ExitCommand(std::string in)
{
    StopCommand(in);
    analyze = false;
}

//--------------------------------
void SetvalueCommand(std::string in)
{
#ifdef TUNE_PARAMETERS
    std::string arg1, arg2;
    GetFirstArg(in, &arg1, &arg2);
    if(arg1 == "PromoEnd")
        param.at(0) = atof(arg2.c_str());
    else if(arg1 == "DblPromoEnd")
        param.at(1) = atof(arg2.c_str());

    InitEngine();
#else
    UNUSED(in);
#endif
}

#include "main.h"
//--------------------------------
// k2, the chess engine
// Author: Sergey Meus (serg_meus@mail.ru)
// Krasnoyarsk Kray, Russia
// Copyright 2012-2013
//--------------------------------

cmdStruct commands[]
{
//   Command    Function

    {"new",     NewCommand},
    {"setboard",SetboardCommand},
    {"set",     SetboardCommand},
    {"quit",    QuitCommand},
    {"q",       QuitCommand},
    {"perft",   PerftCommand},
    {"go",      GoCommand},
    {"level",   LevelCommand},
    {"force",   ForceCommand},
    {"sn",      SetNodesCommand},
    {"st",      SetTimeCommand},
    {"sd",      SetDepthCommand},
    {"protover",ProtoverCommand},
    {"?",       InterrogationCommand},
    {"result",  ResultCommand},
    {"time",    TimeCommand},
    {"eval",    EvalCommand},
    {"test",    TestCommand},

    {"xboard",  Unsupported},
    {"analyze", Unsupported},
    {"exit",    Unsupported},
    {"undo",    Unsupported},
    {"remove",  Unsupported},
    {"computer",Unsupported},
    {"easy",    Unsupported},
    {"hard",    Unsupported},
    {"otim",    Unsupported},

    {"accepted",Unsupported},
    {".",       Unsupported},
    {"",        Unsupported},
};

bool force  = false;
bool quit   = false;

#ifdef USE_THREAD_FOR_INPUT
    std::thread t;                                                      // for compilers with C++11 support
#endif // USE_THREAD_FOR_INPUT                                          // under Linux -pthread must be used for gcc linker



//--------------------------------
int main(int argc, char* argv[])
{
#ifdef TUNE_PARAMETERS
    for(int i = 1; i < argc; ++i)
        if(i < argc)
            tuning_vec.push_back(atof(argv[i]));
        else
            tuning_vec.push_back(0.);
#else
    UNUSED(argc);
    UNUSED(argv);
#endif // TUNE_PARAMETERS

    InitEngine();

    timeMaxPly      = MAX_PLY;
    timeRemains     = 300000000;
    timeBase        = 300000000;
    timeInc         = 0;
    movesPerSession = 0;
    timeMaxNodes    = 0;
    timeCommandSent = false;


    char in[256];
    while(!quit)
    {
        if(!std::cin.getline(in, sizeof(in)), "\n")
            std::cin.clear();
        if(CmdProcess((std::string)in))
        {
            // NiCheGoNeDeLaYem!

        }
        else if(!busy && LooksLikeMove((std::string)in))
        {
            if(!MakeLegalMove(in))
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
        _abort_ = true;
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
    std::cout << "( Total node count: " << totalNodes
                << ", total time spent: " << totalTimeSpent / 1000000.0
                << " )" << std::endl
                << "( MNPS = " << totalNodes / (totalTimeSpent + 1e-5)
                << " )" << std::endl;
    InitEngine();
}

//-------------------------------
void SetboardCommand(std::string in)
{
    if(busy)
        return;

    int firstSymbol = in.find_first_not_of(" \t");
    in.erase(0, firstSymbol);

    if(!FenToBoardAndVal((char *)in.c_str()))
        std::cout << "Illegal position" << std::endl;
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
    Perft(atoi(in.c_str()));
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
    timeMaxPly      = MAX_PLY;
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
    timeMaxPly   = MAX_PLY;
}

//--------------------------------
void SetTimeCommand(std::string in)             //<< NB: wrong
{
    if(busy)
        return;
    timeBase     = 0;
    movesPerSession      = 0;
    timeInc      = atoi(in.c_str())*1000000.;
    timeMaxNodes = 0;
    timeMaxPly   = MAX_PLY;
}

//--------------------------------
void SetDepthCommand(std::string in)
{
    if(busy)
        return;
    timeBase     = 0;
    movesPerSession      = 0;
    timeInc      = 0;
    timeMaxNodes = 0;
    timeMaxPly   = atoi(in.c_str());
}

//--------------------------------
void ProtoverCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        return;
    std::cout << "feature "
            "myname=\"k2 v." ENGINE_VERSION "\" "

            "setboard=1 "
            "analyze=0 "
            "san=0 "
            "colors=0 "
            "pause=0 "
            "usermove=0 "
            "time=1 "
            "draw=0 "
            "done=1 " << std::endl;
}

//--------------------------------
void InterrogationCommand(std::string in)
{
    UNUSED(in);
#ifdef USE_THREAD_FOR_INPUT
    if(busy)
    {
        stop = true;
        _abort_ = false;
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
    short x = Eval();
    std::cout << "Eval: " << (wtm ? -x : x) << std::endl;
    std::cout << "(positive is white advantage)" << std::endl;
}

//--------------------------------
void TestCommand(std::string in)
{
    UNUSED(in);

}

//--------------------------------
void Unsupported(std::string in)
{
    UNUSED(in);

}


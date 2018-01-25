#include "main.h"

//--------------------------------
// K2, the chess engine
// Author: Sergey Meus (serg_meus@mail.ru)
// Krasnoyarsk Krai, Russia
// 2012-2017
//--------------------------------





//--------------------------------
int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    k2main *engine = new k2main();

#ifndef NDEBUG
    engine->RunUnitTests();
    std::cout << "All unit tests passed" << std::endl;
#endif // NDEBUG

    engine->start();

    delete engine;
}





//--------------------------------
k2main::k2main()
{
    force = false;
    quit = false;
    pondering_enabled = false;

    ClearHash();

//    tuning_factors.assign(2, 1); // two params with value 1
}





//--------------------------------
void k2main::start()
{
    char in[0x4000];
    while(!quit)
    {
        if(!std::cin.getline(in, sizeof(in), '\n'))
        {
            if (std::cin.eof())
                quit = true;
            else
                std::cin.clear();
        }

        if(ExecuteCommand((std::string)in))
        {
            // NiCheGoNeDeLaYem!
        }
        else if(!busy && LooksLikeMove((std::string)in))
        {
            if(!MakeMove(in))
                std::cout << "Illegal move" << std::endl;
            else if(!force)
            {
#ifndef DONT_USE_THREAD_FOR_INPUT
                if(thr.joinable())
                    thr.join();
                thr = std::thread(&k2engine::MainSearch, this);
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
bool k2main::ExecuteCommand(std::string in)
{
    command_s commands[] =
    {
//      Command         Method
        {"new",         &k2main::NewCommand},
        {"setboard",    &k2main::SetboardCommand},
        {"set",         &k2main::SetboardCommand},
        {"quit",        &k2main::QuitCommand},
        {"q",           &k2main::QuitCommand},
        {"perft",       &k2main::PerftCommand},
        {"go",          &k2main::GoCommand},
        {"level",       &k2main::LevelCommand},
        {"force",       &k2main::ForceCommand},
        {"sn",          &k2main::SetNodesCommand},
        {"st",          &k2main::SetTimeCommand},
        {"sd",          &k2main::SetDepthCommand},
        {"protover",    &k2main::ProtoverCommand},
        {"?",           &k2main::StopCommand},
        {"result",      &k2main::ResultCommand},
        {"time",        &k2main::TimeCommand},
        {"eval",        &k2main::EvalCommand},
        {"test",        &k2main::TestCommand},
        {"fen",         &k2main::FenCommand},
        {"xboard",      &k2main::XboardCommand},
        {"easy",        &k2main::EasyCommand},
        {"hard",        &k2main::HardCommand},
        {"memory",      &k2main::MemoryCommand},
        {"analyze",     &k2main::AnalyzeCommand},
        {"exit",        &k2main::ExitCommand},
        {"undo",        &k2main::Unsupported},
        {"remove",      &k2main::Unsupported},
        {"computer",    &k2main::Unsupported},
        {"random",      &k2main::Unsupported},
        {"post",        &k2main::Unsupported},
        {"nopost",      &k2main::Unsupported},
        {"random",      &k2main::Unsupported},
        {"otim",        &k2main::Unsupported},
        {"accepted",    &k2main::Unsupported},
        {".",           &k2main::Unsupported},
        {"",            &k2main::Unsupported},
        {"uci",         &k2main::UciCommand},
        {"setoption",   &k2main::SetOptionCommand},
        {"isready",     &k2main::IsReadyCommand},
        {"position",    &k2main::PositionCommand},
        {"ucinewgame",  &k2main::NewCommand},
        {"stop",        &k2main::StopCommand},
        {"ponderhit",   &k2main::PonderhitCommand},
        {"setvalue",    &k2main::SetvalueCommand},
        {"option",      &k2main::OptionCommand}
    };

    std::string command_str, args;

    GetFirstArg(in, &command_str, &args);

    for(size_t i = 0; i < sizeof(commands) / sizeof(command_s); ++i)
    {
        if(command_str == commands[i].command)
        {
            auto command = commands[i];
            method_ptr method = command.mptr;
            ((*this).*method)(args);
            return true;
        }
    }
    return false;
}





//--------------------------------
void k2main::GetFirstArg(std::string in, std::string (*first_word),
                         std::string (*all_the_rest))
{
    if(in.empty())
        return;
    std::string delimiters = " ;,\t\r\n=";
    *first_word = in;
    int first_symbol = first_word->find_first_not_of(delimiters);
    if(first_symbol == -1)
        first_symbol = 0;
    *first_word = first_word->substr(first_symbol, (int)first_word->size());
    int second_symbol = first_word->find_first_of(delimiters);
    if(second_symbol == -1)
        second_symbol = (int)first_word->size();

    *all_the_rest = first_word->substr(second_symbol,
                                       (int)first_word->size());
    *first_word = first_word->substr(0, second_symbol);
}





//--------------------------------
bool k2main::LooksLikeMove(std::string in)
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
void k2main::StopEngine()
{
#ifndef DONT_USE_THREAD_FOR_INPUT
    stop = true;
    thr.join();
#endif // USE_THREAD_FOR_INPUT
}





//--------------------------------
void k2main::NewCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        StopEngine();
    force = false;
    pondering_in_process = false;
    if(!xboard && !uci)
    {
        if(total_time_spent == 0)
            total_time_spent = 1e-5;
        std::cout
                << "( Total node count: " << total_nodes
                << ", total time spent: " << total_time_spent / 1000000.0
                << " )" << std::endl
                << std::setprecision(4) << std::fixed
                << "( MNPS = " << total_nodes / total_time_spent
                << " )" << std::endl;
    }
    SetupPosition(start_position);
    ClearHash();
}





//-------------------------------
void k2main::SetboardCommand(std::string in)
{
    if(busy)
        return;

    auto firstSymbol = in.find_first_not_of(" \t");
    in.erase(0, firstSymbol);

    if(!SetupPosition((char *)in.c_str()))
        std::cout << "Illegal position" << std::endl;
    else if(infinite_analyze && xboard)
        AnalyzeCommand(in);
}





//--------------------------------
void k2main::QuitCommand(std::string in)
{
    UNUSED(in);
#ifndef DONT_USE_THREAD_FOR_INPUT
    if(busy || thr.joinable())
        StopEngine();
#endif // USE_THREAD_FOR_INPUT

    quit = true;
}





//--------------------------------
void k2main::PerftCommand(std::string in)
{
    if(busy)
        return;
    Timer timer;
    double tick1, tick2, deltaTick;
    timer.start();
    tick1 = timer.getElapsedTimeInMicroSec();

    nodes = 0;
    max_search_depth = atoi(in.c_str());
    Perft(max_search_depth);
    max_search_depth = k2chess::max_ply;
    tick2 = timer.getElapsedTimeInMicroSec();
    deltaTick = tick2 - tick1;

    std::cout << std::endl << "nodes = " << nodes << std::endl
              << "dt = " << deltaTick / 1000000. << std::endl
              << "Mnps = " << nodes / (deltaTick + 1)
              << std::endl << std::endl;
}





//--------------------------------
void k2main::GoCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        return;

    if(uci)
        UciGoCommand(in);
    else
        force = false;

#ifndef DONT_USE_THREAD_FOR_INPUT
    if(thr.joinable())
        thr.join();
    thr = std::thread(&k2engine::MainSearch, this);
#else
    MainSearch();
#endif // USE_THREAD_FOR_INPUT

}





//--------------------------------
void k2main::LevelCommand(std::string in)
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

    time_base = 60*1000000.*base;
    time_inc = 1000000*inc;
    moves_per_session = mps;
    time_remains = time_base;
    max_nodes_to_search = 0;
    max_search_depth = k2chess::max_ply;
}





//--------------------------------
void k2main::ForceCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        StopEngine();
    force = true;
}





//--------------------------------
void k2main::SetNodesCommand(std::string in)
{
    if(busy)
        return;
    time_base = 0;
    moves_per_session = 0;
    time_inc = 0;
    max_nodes_to_search = atoi(in.c_str());
    max_search_depth = k2chess::max_ply;
}





//--------------------------------
void k2main::SetTimeCommand(std::string in)
{
    if(busy)
        return;
    time_base = 0;
    moves_per_session = 1;
    time_inc = atof(in.c_str())*1000000.;
    max_nodes_to_search = 0;
    max_search_depth = k2chess::max_ply;
    time_remains = 0;
}





//--------------------------------
void k2main::SetDepthCommand(std::string in)
{
    if(busy)
        return;
    max_search_depth = atoi(in.c_str());
}





//--------------------------------
void k2main::ProtoverCommand(std::string in)
{
    using namespace std;

    UNUSED(in);
    if(busy)
        return;
    xboard = true;
    uci = false;

    string feat = "feature ";

    cout << feat << "myname=\"K2 v." << engine_version << "\"" << endl;
    cout << feat << "setboard=1" << endl;
    cout << feat << "analyze=1" << endl;
    cout << feat << "san=0" << endl;
    cout << feat << "colors=0" << endl;
    cout << feat << "pause=0" << endl;
    cout << feat << "usermove=0" << endl;
    cout << feat << "time=1" << endl;
    cout << feat << "draw=0" << endl;
    cout << feat << "sigterm=0" << endl;
    cout << feat << "sigint=0" << endl;
    cout << feat << "memory=1" << endl;
    cout << feat << "option=\"Randomness -check 1\"" << endl;
    cout << feat << "done=1" << endl;
}





//--------------------------------
void k2main::StopCommand(std::string in)
{
    UNUSED(in);
#ifndef DONT_USE_THREAD_FOR_INPUT
    if(busy)
    {
        stop = true;
        thr.join();
    }
#endif // USE_THREAD_FOR_INPUT
}





//--------------------------------
void k2main::ResultCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        StopEngine();
}





//--------------------------------
void k2main::TimeCommand(std::string in)
{
    if(busy)
    {
        std::cout << "telluser time command recieved while engine is busy"
                  << std::endl;
        return;
    }
    double tb = atof(in.c_str()) * 10000;
    time_remains = tb;
    time_command_sent = true;
}





//--------------------------------
void k2main::EvalCommand(std::string in)
{
    UNUSED(in);
    if(busy)
        return;

    EvalDebug();
}





//--------------------------------
void k2main::TestCommand(std::string in)
{
    UNUSED(in);
}





//--------------------------------
void k2main::FenCommand(std::string in)
{
    UNUSED(in);
    ShowFen();
}





//--------------------------------
void k2main::XboardCommand(std::string in)
{
    UNUSED(in);
    xboard = true;
    uci = false;
    std::cout << "( build time: "
              << __DATE__ << " " << __TIME__
              << " )" << std::endl;
}





//--------------------------------
void k2main::Unsupported(std::string in)
{
    UNUSED(in);
}





//--------------------------------
void k2main::UciCommand(std::string in)
{
    UNUSED(in);
    uci = true;
    std::cout << "id name K2 v." << engine_version << std::endl;
    std::cout << "id author Sergey Meus" << std::endl;
    std::cout << "option name Hash type spin default 64 min 0 max 2048"
              << std::endl;
    std::cout << "option name Randomness type check default true"
              << std::endl;
    std::cout << "uciok" << std::endl;
}





//--------------------------------
void k2main::SetOptionCommand(std::string in)
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
        auto size_MB = atoi(arg1.c_str());
        ReHash(size_MB);
    }
    else if(arg1 == "Randomness")
    {
        GetFirstArg(arg2, &arg1, &arg2);
        if(arg1 != "value")
            return;
        GetFirstArg(arg2, &arg1, &arg2);
        if(arg1 == "true")
            randomness = true;
        else
            randomness = false;
    }
}





//--------------------------------
void k2main::IsReadyCommand(std::string in)
{
    UNUSED(in);
    std::cout << "\nreadyok\n";  // '\n' to avoid multithreading problems
}





//--------------------------------
void k2main::PositionCommand(std::string in)
{

    std::string arg1, arg2;
    GetFirstArg(in, &arg1, &arg2);

    if(arg1 == "fen")
    {
        std::string fenstring;
        auto beg = arg2.find_first_not_of(" \t");
        fenstring = arg2.substr(beg, arg2.size());
        if(!SetupPosition((char *)fenstring.c_str()))
        {
            std::cout << "Illegal position" << std::endl;
            return;
        }
    }
    else
        SetupPosition(start_position);

    int moves = arg2.find("moves");
    if(moves == -1)
        return;

    auto mov_seq = arg2.substr(moves + 5, in.size());
    auto beg = mov_seq.find_first_not_of(" \t");
    mov_seq = mov_seq.substr(beg, mov_seq.size());

    ProcessMoveSequence(mov_seq);
}





//--------------------------------
void k2main::ProcessMoveSequence(std::string in)
{
    std::string arg1, arg2;
    arg1 = in;
    while(true)
    {
        GetFirstArg(arg1, &arg1, &arg2);
        if(arg1.empty())
            break;
        if(!MakeMove((char *)arg1.c_str()))
            break;
        arg1 = arg2;
    }
}





//--------------------------------
void k2main::UciGoCommand(std::string in)
{
    pondering_in_process = false;
    bool no_movestogo_arg = true;
    std::string arg1, arg2;
    arg1 = in;
    while(true)
    {
        GetFirstArg(arg1, &arg1, &arg2);
        if(arg1.empty())
            break;
        if(arg1 == "infinite")
        {
            infinite_analyze = true;
            break;
        }

        else if(arg1 == "wtime" || arg1 == "btime")
        {
            char clr = arg1[0];
            GetFirstArg(arg2, &arg1, &arg2);
            if((clr == 'w' && WhiteIsOnMove())
                    || (clr == 'b' && !WhiteIsOnMove()))
            {
                time_base = 1000.*atof(arg1.c_str());
                time_remains = time_base;
                max_nodes_to_search = 0;
                max_search_depth = k2chess::max_ply;

                // crutch: engine must know that time changed by GUI
                time_command_sent = true;
            }
            arg1 = arg2;
        }
        else if(arg1 == "winc" || arg1 == "binc")
        {
            char clr = arg1[0];
            GetFirstArg(arg2, &arg1, &arg2);
            if((clr == 'w' && WhiteIsOnMove())
                    || (clr == 'b' && !WhiteIsOnMove()))
                time_inc         = 1000.*atof(arg1.c_str());
            arg1 = arg2;
        }
        else if(arg1 == "movestogo")
        {
            GetFirstArg(arg2, &arg1, &arg2);
            moves_per_session = atoi(arg1.c_str());
            arg1 = arg2;
            no_movestogo_arg = false;
        }
        else if(arg1 == "movetime")
        {
            GetFirstArg(arg2, &arg1, &arg2);
            time_base = 0;
            moves_per_session = 1;
            time_inc = 1000.*atof(arg1.c_str());
            max_nodes_to_search = 0;
            max_search_depth = k2chess::max_ply;
            time_remains = 0;

            arg1 = arg2;
        }
        else if(arg1 == "depth")
        {
            GetFirstArg(arg2, &arg1, &arg2);
            max_search_depth = atoi(arg1.c_str());
            time_base = INFINITY;
            time_remains = time_base;
            max_nodes_to_search = 0;

            arg1 = arg2;
        }
        else if(arg1 == "nodes")
        {
            GetFirstArg(arg2, &arg1, &arg2);
            time_base = INFINITY;
            time_remains = time_base;
            max_nodes_to_search = atoi(arg1.c_str());
            max_search_depth = k2chess::max_ply;

            arg1 = arg2;
        }
        else if(arg1 == "ponder")
        {
            pondering_in_process = true;
            arg1 = arg2;
        }
        else
            break;

    }
    if(no_movestogo_arg)
        moves_per_session = 0;
}





//--------------------------------
void k2main::EasyCommand(std::string in)
{
    UNUSED(in);
    pondering_enabled = false;
}





//--------------------------------
void k2main::HardCommand(std::string in)
{
    UNUSED(in);
    pondering_enabled = true;
}





//--------------------------------
void k2main::PonderhitCommand(std::string in)
{
    UNUSED(in);
    PonderHit();
}





//--------------------------------
void k2main::MemoryCommand(std::string in)
{
    std::string arg1, arg2;
    arg1 = in;
    GetFirstArg(arg2, &arg1, &arg2);
    auto size_MB = atoi(arg1.c_str());
    ReHash(size_MB);
}





//--------------------------------
void k2main::AnalyzeCommand(std::string in)
{
    UNUSED(in);
    force = false;
    infinite_analyze = true;

#ifndef DONT_USE_THREAD_FOR_INPUT
    if(thr.joinable())
        thr.join();
    thr = std::thread(&k2engine::MainSearch, this);
#else
    MainSearch();
#endif // USE_THREAD_FOR_INPUT
}





//--------------------------------
void k2main::ExitCommand(std::string in)
{
    StopCommand(in);
    infinite_analyze = false;
}





//--------------------------------
void k2main::SetvalueCommand(std::string in)
{
    std::string arg1, arg2;
    GetFirstArg(in, &arg1, &arg2);
/*
        if(arg1 == "k_saf")
        {
            tuning_factors.at(0) = atof(arg2.c_str());
        }

        else if(arg1 == "k_saf1")
        {
            tuning_factors.at(1) = atof(arg2.c_str());
        }

        else
        {
            std::cout << "error: wrong parameter name" << std ::endl
                         << "resign" << std::endl;
            tuning_factors.clear();
        }
*/
    SetupPosition(start_position);
}





//--------------------------------
void k2main::OptionCommand(std::string in)
{
    std::string arg1, arg2;
    GetFirstArg(in, &arg1, &arg2);

    if(arg1 == "Randomness")
    {
        if(arg2 == "1")
            randomness = true;
        else
            randomness = false;
    }
}

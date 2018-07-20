#include "main.h"

//--------------------------------
// K2, the chess engine
// Author: Sergey Meus (serg_meus@mail.ru)
// Krasnoyarsk Krai, Russia
// 2012-2018
//--------------------------------





//--------------------------------
int main(int argc, char* argv[])
{
    (void)(argc);
    (void)(argv);

    k2main *engine = new k2main();

#ifndef NDEBUG
    engine->RunUnitTests();
    std::cout << "All unit tests passed" << std::endl;
#endif // NDEBUG

    engine->start();

    delete engine;
}





//--------------------------------
k2main::k2main() : force(false), quit(false), use_thread(true)
{
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
                if(use_thread)
                {
                    if(thr.joinable())
                        thr.join();
                    thr = std::thread(&k2engine::MainSearch, this);
                }
                else
                    MainSearch();
            }
        }
        else
            std::cout << "Unknown command: " << in << std::endl;
    }
    if(busy)
        StopEngine();
}





//--------------------------------
bool k2main::ExecuteCommand(const std::string in)
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
        {"post",        &k2main::PostCommand},
        {"nopost",      &k2main::NopostCommand},
        {"exit",        &k2main::ExitCommand},
        {"undo",        &k2main::Unsupported},
        {"remove",      &k2main::Unsupported},
        {"computer",    &k2main::Unsupported},
        {"random",      &k2main::Unsupported},
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
        {"option",      &k2main::OptionCommand},
        {"seed",        &k2main::SeedCommand},
    };

    std::string command_str, args;

    GetFirstArg(in, &command_str, &args);

    for(size_t i = 0; i < sizeof(commands) / sizeof(command_s); ++i)
    {
        if(command_str != commands[i].command)
            continue;

        auto command = commands[i];
        method_ptr method = command.mptr;
        ((*this).*method)(args);
        return true;
    }
    return false;
}





//--------------------------------
void k2main::GetFirstArg(const std::string in,  std::string * const first_word,
                         std::string * const all_the_rest) const
{
    if(in.empty())
        return;
    const std::string delimiters = " ;,\t\r\n=";
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
bool k2main::LooksLikeMove(const std::string in) const
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
    if(!use_thread)
        return;
    stop = true;
    thr.join();
}





//--------------------------------
void k2main::NewCommand(const std::string in)
{
    (void)(in);

    if(busy)
        StopEngine();
    force = false;
    pondering_in_process = false;
    if(!xboard && !uci && enable_output)
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
    seed = std::time(nullptr) % (1 << max_moves_to_shuffle);
    if(uci)
        moves_per_session = 0;
}





//-------------------------------
void k2main::SetboardCommand(const std::string in)
{
    if(busy)
        return;

    const auto firstSymbol = in.find_first_not_of(" \t");
    const auto in1 = in.substr(firstSymbol, in.size() - firstSymbol);

    if(!SetupPosition(const_cast<char *>(in1.c_str())))
        std::cout << "Illegal position" << std::endl;
    else if(infinite_analyze && xboard)
        AnalyzeCommand(in);
}





//--------------------------------
void k2main::QuitCommand(const std::string in)
{
    (void)(in);

    if(use_thread && (busy || thr.joinable()))
        StopEngine();
    quit = true;
}





//--------------------------------
void k2main::PerftCommand(const std::string in)
{
    if(busy)
        return;

    Timer clock;
    clock.start();
    const auto tick1 = clock.getElapsedTimeInMicroSec();

    nodes = 0;
    max_search_depth = atoi(in.c_str());
    Perft(max_search_depth);
    max_search_depth = k2chess::max_ply;
    const auto tick2 = clock.getElapsedTimeInMicroSec();
    const auto deltaTick = tick2 - tick1;

    std::cout << std::endl << "nodes = " << nodes << std::endl
              << "dt = " << deltaTick / 1000000. << std::endl
              << "Mnps = " << nodes / (deltaTick + 1)
              << std::endl << std::endl;
}





//--------------------------------
void k2main::GoCommand(const std::string in)
{
    (void)(in);

    if(busy)
        return;

    if(uci)
        UciGoCommand(in);
    else
        force = false;

    if(use_thread)
    {
        if(thr.joinable())
            thr.join();
        thr = std::thread(&k2engine::MainSearch, this);
    }
    else
        MainSearch();
}





//--------------------------------
void k2main::LevelCommand(const std::string in)
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
void k2main::ForceCommand(const std::string in)
{
    (void)(in);

    if(busy)
        StopEngine();

    force = true;
}





//--------------------------------
void k2main::SetNodesCommand(const std::string in)
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
void k2main::SetTimeCommand(const std::string in)
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
void k2main::SetDepthCommand(const std::string in)
{
    if(busy)
        return;
    max_search_depth = atoi(in.c_str());
}





//--------------------------------
void k2main::ProtoverCommand(const std::string in)
{
    using namespace std;

    (void)(in);

    if(busy)
        return;
    xboard = true;
    uci = false;

    const string feat = "feature ";

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
    cout << feat << "option=\"Separate_thread_for_input -check 1\"" << endl;
    cout << feat << "done=1" << endl;
}





//--------------------------------
void k2main::StopCommand(const std::string in)
{
    (void)(in);

    if(use_thread && busy)
    {
        stop = true;
        thr.join();
    }
}





//--------------------------------
void k2main::ResultCommand(const std::string in)
{
    (void)(in);

    if(busy)
        StopEngine();
}





//--------------------------------
void k2main::TimeCommand(const std::string in)
{
    if(busy)
    {
        std::cout << "telluser time command recieved while engine is busy"
                  << std::endl;
        return;
    }
    const auto tb = atof(in.c_str()) * 10000;
    time_remains = tb;
    time_command_sent = true;
}





//--------------------------------
void k2main::EvalCommand(const std::string in)
{
    (void)(in);

    if(busy)
        return;

    EvalDebug();
    xboard = false;
    uci = false;
}





//--------------------------------
void k2main::TestCommand(const std::string in)
{
    (void)(in);

    if(busy)
        return;

    bool ans = false;
    bool store_uci = uci;
    bool store_enable_output = enable_output;
    bool store_randomness = randomness;
    bool store_inf_analyze = infinite_analyze;
    depth_t store_max_depth = max_search_depth;
    Timer clock;
    clock.start();
    auto tick1 = clock.getElapsedTimeInMicroSec();

    while(true)
    {
        if(!test_perft("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -",
                   4, 2103487))
            break;
        if(!test_perft("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 4, 43238))
            break;
        if(!test_perft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/"
                       "PPPBBPPP/R3K2R w KQkq -", 3, 97862))
            break;

        auto tick2 = clock.getElapsedTimeInMicroSec();
        auto delta_tick = tick2 - tick1;
        std::cout << "Perft: total nodes = " << total_nodes
                  << ", dt = " << delta_tick / 1000000.
                  << ", Mnps = " << total_nodes / (delta_tick + 1)
                  << std::endl;

        total_nodes = 0;
        enable_output = false;
        uci = true;
        randomness = false;
        clock.start();
        tick1 = clock.getElapsedTimeInMicroSec();

        if(!test_search("1rq2b1r/2N1k1pp/1pQp4/4n3/2P5/8/PP4PP/4RRK1 w - - 0 1"
                        " bm Rxe5", 7, "e1e5", false))
            break;
        if(!test_search("1k1r2nr/ppp1q1p1/3pbp1p/4n3/Q3P2B/2P2N2/P3BPPP/"
                        "1R3RK1 w - - 0 1 bm Rxb7", 10, "b1b7", false))
            break;
        if(!test_search("8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1 bm Kb1",
                        22, "a1b1", false))
            break;
        if(!test_search("3q2k1/3r1ppp/4nP2/1Q1pp1P1/4P2P/1P3R2/P1r1N3/"
                        "1R4K1 w - - 0 28 am fxg7", 11, "f6g7", true))
            break;
        if(!test_search("3r2k1/1p3ppp/2pq4/p1n5/P6P/1P6/1PB2QP1/1K2R3 "
                        "w - - am Rd1", 8, "e1d1", true))
            break;

        tick2 = clock.getElapsedTimeInMicroSec();
        delta_tick = tick2 - tick1;
        std::cout << "Search: total nodes = " << total_nodes
                  << ", dt = " << delta_tick / 1000000.
                  << ", Mnps = " << total_nodes / (delta_tick + 1)
                  << std::endl;
        total_nodes = 0;

        std::cout << "All integration tests passed\n";
        ans = true;
        break;
    }
    max_search_depth = store_max_depth;
    enable_output = store_enable_output;
    uci = store_uci;
    randomness = store_randomness;
    infinite_analyze = store_inf_analyze;
    SetupPosition(start_position);
    if(!ans)
        std::cout << "Integration testing FAILED\n";
}





//--------------------------------
void k2main::FenCommand(const std::string in)
{
    (void)(in);

    ShowFen();
}





//--------------------------------
void k2main::XboardCommand(const std::string in)
{
    (void)(in);

    xboard = true;
    uci = false;
    std::cout << "( build time: "
              << __DATE__ << " " << __TIME__
              << " )" << std::endl;
    total_nodes = 0;
}





//--------------------------------
void k2main::Unsupported(const std::string in)
{
    (void)(in);
}





//--------------------------------
void k2main::UciCommand(const std::string in)
{
    using namespace std;

    (void)(in);

    uci = true;
    cout << "id name K2 v." << engine_version << endl;
    cout << "id author Sergey Meus" << endl;
    cout << "option name Hash type spin default 64 min 0 max 2048" << endl;
    cout << "option name Ponder type check default false" << endl;
    cout << "option name Randomness type check default true" << endl;
    cout << "option name Separate_thread_for_input type check default true"
         << endl;
    cout << "uciok" << endl;

    total_nodes = 0;
}





//--------------------------------
void k2main::SetOptionCommand(const std::string in)
{
    std::string arg1, arg2;
    GetFirstArg(in, &arg1, &arg2);
    if(arg1 != "name")
    {
        std::cout << "Error: incorrect command options\n";
        return;
    }
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
    else if (arg1 == "Separate_thread_for_input")
    {
        GetFirstArg(arg2, &arg1, &arg2);
        if(arg1 != "value")
            return;
        GetFirstArg(arg2, &arg1, &arg2);
        if(arg1 == "true")
            use_thread = true;
        else
            use_thread = false;
    }
}





//--------------------------------
void k2main::IsReadyCommand(const std::string in)
{
    (void)(in);

    std::cout << "\nreadyok\n";  // '\n' to avoid multithreading problems
}





//--------------------------------
void k2main::PositionCommand(const std::string in)
{

    std::string arg1, arg2;
    GetFirstArg(in, &arg1, &arg2);

    if(arg1 == "fen")
    {
        std::string fenstring;
        auto beg = arg2.find_first_not_of(" \t");
        fenstring = arg2.substr(beg, arg2.size());
        if(!SetupPosition(const_cast<char *>(fenstring.c_str())))
        {
            std::cout << "Illegal position" << std::endl;
            return;
        }
    }
    else
        SetupPosition(start_position);

    const int moves = arg2.find("moves");
    if(moves == -1)
        return;

    auto mov_seq = arg2.substr(moves + 5, in.size());
    const auto beg = mov_seq.find_first_not_of(" \t");
    mov_seq = mov_seq.substr(beg, mov_seq.size());

    ProcessMoveSequence(mov_seq);
}





//--------------------------------
void k2main::ProcessMoveSequence(const std::string in)
{
    std::string arg1, arg2;
    arg1 = in;
    while(true)
    {
        GetFirstArg(arg1, &arg1, &arg2);
        if(arg1.empty())
            break;
        if(!MakeMove(const_cast<char *>(arg1.c_str())))
            break;
        arg1 = arg2;
    }
}





//--------------------------------
void k2main::UciGoCommand(const std::string in)
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
            if(moves_per_session == 0)
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
void k2main::EasyCommand(const std::string in)
{
    (void)(in);
}





//--------------------------------
void k2main::HardCommand(const std::string in)
{
    (void)(in);
}





//--------------------------------
void k2main::PonderhitCommand(const std::string in)
{
    (void)(in);

    PonderHit();
}





//--------------------------------
void k2main::MemoryCommand(const std::string in)
{
    std::string arg1, arg2;
    arg1 = in;
    GetFirstArg(arg2, &arg1, &arg2);
    auto size_MB = atoi(arg1.c_str());
    ReHash(size_MB);
}





//--------------------------------
void k2main::AnalyzeCommand(const std::string in)
{
    (void)(in);

    force = false;
    infinite_analyze = true;

    if(use_thread)
    {
        if(thr.joinable())
            thr.join();
        thr = std::thread(&k2engine::MainSearch, this);
    }
    else
        MainSearch();
}





//--------------------------------
void k2main::ExitCommand(const std::string in)
{
    StopCommand(in);
    infinite_analyze = false;
}





//--------------------------------
void k2main::SetvalueCommand(const std::string in)
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
void k2main::OptionCommand(const std::string in)
{
    std::string arg1, arg2;
    GetFirstArg(in, &arg1, &arg2);

    if(arg1 == "Randomness")
    {
        if(arg2 == "1" || arg2 == "true")
            randomness = true;
        else
            randomness = false;
    }
    else if(arg1 == "Separate_thread_for_input")
    {
        if(arg2 == "1" || arg2 == "true")
            use_thread = true;
        else
            use_thread = false;
    }
}





//--------------------------------
void k2main::PostCommand(const std::string in)
{
    (void)(in);

    enable_output = true;
}





//--------------------------------
void k2main::NopostCommand(const std::string in)
{
    (void)(in);

    enable_output = false;
}





//--------------------------------
void k2main::SeedCommand(const std::string in)
{
    std::string arg1, arg2;
    arg1 = in;
    GetFirstArg(arg2, &arg1, &arg2);
    seed = atoi(arg1.c_str());
}

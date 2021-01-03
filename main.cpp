#include "main.h"

//--------------------------------
// K2, the chess engine
// Author: Sergey Meus (serg_meus@mail.ru)
// Krasnoyarsk Krai, Russia
// 2012-2021
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
bool k2main::ExecuteCommand(const std::string &in)
{
    std::unordered_map<std::string, method_ptr> commands =
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
        {"easy",        &k2main::Unsupported},
        {"hard",        &k2main::Unsupported},
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
        {"tuningload",  &k2main::TuningLoadCommand},
        {"tuningresult",&k2main::TuningResultCommand},
        {"tuneparam",   &k2main::TuneParamCommand},
        {"tune",        &k2main::TuneCommand},
    };

    std::string command_str, args;
    command_str = GetFirstArg(in, &args);

    const auto command = commands.find(command_str);
    if(command == commands.end())
        return false;

    method_ptr method = command->second;
    ((*this).*method)(args);
    return true;
}





//--------------------------------
std::string k2main::GetFirstArg(const std::string &in,
                                std::string * const all_the_rest) const
{
    const std::string delimiters = " ;,\t\r\n=";
    std::string first_word = in;
    int first_symbol = first_word.find_first_not_of(delimiters);
    if(first_symbol == -1)
        first_symbol = 0;
    first_word = first_word.substr(first_symbol, (int)first_word.size());
    int second_symbol = first_word.find_first_of(delimiters);
    if(second_symbol == -1)
        second_symbol = (int)first_word.size();

    auto tmp_str = first_word.substr(second_symbol, (int)first_word.size());
    int third_symbol = tmp_str.find_first_not_of(delimiters);
    if(third_symbol == -1)
        third_symbol = 0;
    *all_the_rest = tmp_str.substr(third_symbol, (int)tmp_str.size());
    first_word = first_word.substr(0, second_symbol);

    return first_word;
}





//--------------------------------
bool k2main::LooksLikeMove(const std::string &in) const
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
void k2main::NewCommand(const std::string &in)
{
    (void)(in);

    if(busy)
        StopEngine();
    force = false;
    pondering_in_process = false;
    if(!xboard && !uci && enable_output)
    {
        if(time_control.total_time_spent == 0)
            time_control.total_time_spent = 1e-5;
        std::cout
                << "( Total node count: "
                << stats.total_nodes
                << ", total time spent: "
                << time_control.total_time_spent / 1000000.0
                << " )" << std::endl
                << std::setprecision(4) << std::fixed
                << "( MNPS = " << stats.total_nodes /
                   time_control.total_time_spent
                << " )" << std::endl;
    }
    SetupPosition(start_position);
    ClearHash();
    seed = std::time(nullptr) % (1 << max_moves_to_shuffle);
    if(uci)
        time_control.moves_per_session = 0;
}





//-------------------------------
void k2main::SetboardCommand(const std::string &in)
{
    if(busy)
        return;
    busy = true;

    const auto firstSymbol = in.find_first_not_of(" \t");
    const auto in1 = in.substr(firstSymbol, in.size() - firstSymbol);

    if(!SetupPosition(const_cast<char *>(in1.c_str())))
        std::cout << "Illegal position" << std::endl;
    else if(time_control.infinite_analyze && xboard)
        AnalyzeCommand(in);
    busy = false;
}





//--------------------------------
void k2main::QuitCommand(const std::string &in)
{
    (void)(in);

    if(use_thread && (busy || thr.joinable()))
        StopEngine();
    quit = true;
}





//--------------------------------
void k2main::PerftCommand(const std::string &in)
{
    if(busy)
        return;
    busy = true;

    Timer clock;
    clock.start();
    const auto tick1 = clock.getElapsedTimeInMicroSec();

    stats.nodes = 0;
    time_control.max_search_depth = atoi(in.c_str());

    Perft(time_control.max_search_depth);

    time_control.max_search_depth = k2chess::max_ply;
    const auto tick2 = clock.getElapsedTimeInMicroSec();
    const auto deltaTick = tick2 - tick1;

    std::cout << std::endl << "nodes = " << stats.nodes << std::endl
              << "time = " << deltaTick / 1000000. << std::endl
              << "Mnps = " << stats.nodes / (deltaTick + 1)
              << std::endl << std::endl;
    busy = false;
}





//--------------------------------
void k2main::GoCommand(const std::string &in)
{
    if(busy)
        return;

    if(uci || !in.empty())
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
void k2main::LevelCommand(const std::string &in)
{
    if(busy)
        return;
    std::string arg1, arg2, arg3;
    arg1 = GetFirstArg(in, &arg2);
    arg2 = GetFirstArg(arg2, &arg3);
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

    time_control.time_base = 60*1000000.*base;
    time_control.time_inc = 1000000*inc;
    time_control.moves_per_session = mps;
    time_control.time_remains = time_control.time_base;
    time_control.max_nodes_to_search = 0;
    time_control.max_search_depth = k2chess::max_ply;
}





//--------------------------------
void k2main::ForceCommand(const std::string &in)
{
    (void)(in);

    if(busy)
        StopEngine();

    force = true;
}





//--------------------------------
void k2main::SetNodesCommand(const std::string &in)
{
    if(busy)
        return;
    time_control.time_base = 0;
    time_control.moves_per_session = 0;
    time_control.time_inc = 0;
    time_control.max_nodes_to_search = atoi(in.c_str());
    time_control.max_search_depth = k2chess::max_ply;
}





//--------------------------------
void k2main::SetTimeCommand(const std::string &in)
{
    if(busy)
        return;
    time_control.time_base = 0;
    time_control.moves_per_session = 1;
    time_control.time_inc = atof(in.c_str())*1000000.;
    time_control.max_nodes_to_search = 0;
    time_control.max_search_depth = k2chess::max_ply;
    time_control.time_remains = 0;
}





//--------------------------------
void k2main::SetDepthCommand(const std::string &in)
{
    if(busy)
        return;
    time_control.max_search_depth = atoi(in.c_str());
}





//--------------------------------
void k2main::ProtoverCommand(const std::string &in)
{
    using namespace std;

    (void)(in);

    if(busy)
        return;
    xboard = true;
    uci = false;

    const string feat = "feature ";

    cout << feat << "myname=\"K2 v" << engine_version << "\"" << endl;
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
void k2main::StopCommand(const std::string &in)
{
    (void)(in);

    if(use_thread && busy)
    {
        stop = true;
        thr.join();
    }
}





//--------------------------------
void k2main::ResultCommand(const std::string &in)
{
    (void)(in);

    if(busy)
        StopEngine();
}





//--------------------------------
void k2main::TimeCommand(const std::string &in)
{
    if(busy)
    {
        std::cout << "telluser time command recieved while engine is busy"
                  << std::endl;
        return;
    }
    const auto tb = atof(in.c_str()) * 10000;
    time_control.time_remains = tb;
    time_control.time_command_sent = true;
}





//--------------------------------
void k2main::EvalCommand(const std::string &in)
{
    (void)(in);

    if(busy)
        return;

    EvalDebug();
    xboard = false;
    uci = false;
}





//--------------------------------
void k2main::TestCommand(const std::string &in)
{
    (void)(in);

    if(busy)
        return;
    busy = true;

    bool ans = false;
    bool store_uci = uci;
    bool store_enable_output = enable_output;
    bool store_randomness = randomness;
    bool store_inf_analyze = time_control.infinite_analyze;
    depth_t store_max_depth = time_control.max_search_depth;
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
        std::cout << "Perft: OK, total nodes = " << stats.total_nodes
                  << ", time = " << delta_tick / 1000000.
                  << ", Mnps = " << stats.total_nodes / (delta_tick + 1)
                  << std::endl;

        stats.total_nodes = 0;
        enable_output = false;
        uci = true;
        randomness = false;
        clock.start();
        tick1 = clock.getElapsedTimeInMicroSec();

        if(!test_search("1rq2b1r/2N1k1pp/1pQp4/4n3/2P5/8/PP4PP/4RRK1 w - -"
                        " bm Rxe5", 7, "e1e5", false))
            break;
        if(!test_search(
                "1k1r2nr/ppp1q1p1/3pbp1p/4n3/Q3P2B/2P2N2/P3BPPP/1R3RK1 w - -"
                " bm Rxb7", 10, "b1b7", false))
            break;
        if(!test_search("8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - bm Kb1",
                        24, "a1b1", false))
            break;
        if(!test_search(
                "3q2k1/3r1ppp/4nP2/1Q1pp1P1/4P2P/1P3R2/P1r1N3/1R4K1 w - -"
                " am fxg7", 11, "f6g7", true))
            break;
        if(!test_search("3r2k1/1p3ppp/2pq4/p1n5/P6P/1P6/1PB2QP1/1K2R3 w - -"
                        "am Rd1", 8, "e1d1", true))
            break;

        tick2 = clock.getElapsedTimeInMicroSec();
        delta_tick = tick2 - tick1;
        std::cout << "Search: OK, total nodes = " << stats.total_nodes
                  << ", time " << delta_tick / 1000000.
                  << ", Mnps = " << stats.total_nodes / (delta_tick + 1)
                  << std::endl;
        stats.total_nodes = 0;

        std::cout << "All integration tests passed\n";
        ans = true;
        break;
    }
    time_control.max_search_depth = store_max_depth;
    enable_output = store_enable_output;
    uci = store_uci;
    randomness = store_randomness;
    time_control.infinite_analyze = store_inf_analyze;
    SetupPosition(start_position);
    if(!ans)
        std::cout << "Integration testing FAILED\n";
    busy = false;
}





//--------------------------------
void k2main::FenCommand(const std::string &in)
{
    (void)(in);

    ShowFen();
}





//--------------------------------
void k2main::XboardCommand(const std::string &in)
{
    (void)(in);

    xboard = true;
    uci = false;
    std::cout << "( build time: "
              << __DATE__ << " " << __TIME__
              << " )" << std::endl;
    stats.total_nodes = 0;
}





//--------------------------------
void k2main::Unsupported(const std::string &in)
{
    (void)(in);
}





//--------------------------------
void k2main::UciCommand(const std::string &in)
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

    stats.total_nodes = 0;
}





//--------------------------------
void k2main::SetOptionCommand(const std::string &in)
{
    std::string arg1, arg2;
    arg1 = GetFirstArg(in, &arg2);
    if(arg1 != "name")
    {
        std::cout << "Error: incorrect command options\n";
        return;
    }
    arg1 = GetFirstArg(arg2, &arg2);

    if(arg1 == "Hash" || arg1 == "hash")
    {
        arg1 =  GetFirstArg(arg2, &arg2);
        if(arg1 != "value")
            return;
        arg1 = GetFirstArg(arg2, &arg2);
        auto size_MB = atoi(arg1.c_str());
        ReHash(size_MB);
    }
    else if(arg1 == "Randomness" || arg1 == "rand")
    {
        arg1 = GetFirstArg(arg2, &arg2);
        if(arg1 != "value")
            return;
        arg1 = GetFirstArg(arg2, &arg2);
        if(arg1 == "true")
            randomness = true;
        else
            randomness = false;
    }
    else if(arg1 == "Separate_thread_for_input" || arg1 == "thread")
    {
        arg1 = GetFirstArg(arg2, &arg2);
        if(arg1 != "value")
            return;
        arg1 = GetFirstArg(arg2, &arg2);
        if(arg1 == "true")
            use_thread = true;
        else
            use_thread = false;
    }
    else
        std::cout << "Error: incorrect command options\n";
}





//--------------------------------
void k2main::IsReadyCommand(const std::string &in)
{
    (void)(in);
    if(busy)
        thr.join();
    std::cout << "\nreadyok\n";  // '\n' to avoid multithreading problems
}





//--------------------------------
void k2main::PositionCommand(const std::string &in)
{
    busy = true;
    std::string arg1, arg2;
    arg1 = GetFirstArg(in, &arg2);

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
    {
        busy = false;
        return;
    }

    auto mov_seq = arg2.substr(moves + 5, in.size());
    const auto beg = mov_seq.find_first_not_of(" \t");
    mov_seq = mov_seq.substr(beg, mov_seq.size());

    ProcessMoveSequence(mov_seq);
    busy = false;
}





//--------------------------------
void k2main::ProcessMoveSequence(const std::string &in)
{
    std::string arg1, arg2;
    arg1 = in;
    while(true)
    {
        arg1 = GetFirstArg(arg1, &arg2);
        if(arg1.empty())
            break;
        if(!MakeMove(const_cast<char *>(arg1.c_str())))
            break;
        arg1 = arg2;
    }
}





//--------------------------------
void k2main::UciGoCommand(const std::string &in)
{
    pondering_in_process = false;
    bool no_movestogo_arg = true;
    std::string arg1, arg2;
    arg1 = in;
    root_moves_to_search.clear();
    while(true)
    {
        arg1 = GetFirstArg(arg1, &arg2);
        if(arg1.empty())
            break;
        if(arg1 == "infinite")
        {
            time_control.infinite_analyze = true;
            arg1 = arg2;
        }

        else if(arg1 == "wtime" || arg1 == "btime")
        {
            char clr = arg1[0];
            arg1 = GetFirstArg(arg2, &arg2);
            if((clr == 'w' && WhiteIsOnMove())
                    || (clr == 'b' && !WhiteIsOnMove()))
            {
                time_control.time_base = 1000.*atof(arg1.c_str());
                time_control.time_remains = time_control.time_base;
                time_control.max_nodes_to_search = 0;
                time_control.max_search_depth = k2chess::max_ply;

                // crutch: engine must know that time changed by GUI
                time_control.time_command_sent = true;
            }
            arg1 = arg2;
        }
        else if(arg1 == "winc" || arg1 == "binc")
        {
            char clr = arg1[0];
            arg1 = GetFirstArg(arg2, &arg2);
            if((clr == 'w' && WhiteIsOnMove())
                    || (clr == 'b' && !WhiteIsOnMove()))
                time_control.time_inc = 1000.*atof(arg1.c_str());
            arg1 = arg2;
        }
        else if(arg1 == "movestogo")
        {
            arg1 = GetFirstArg(arg2, &arg2);
            time_control.moves_per_session = atoi(arg1.c_str());
            arg1 = arg2;
            no_movestogo_arg = false;
        }
        else if(arg1 == "movetime")
        {
            arg1 = GetFirstArg(arg2, &arg2);
            time_control.time_base = 0;
            time_control.moves_per_session = 1;
            time_control.time_inc = 1000.*atof(arg1.c_str());
            time_control.max_nodes_to_search = 0;
            time_control.max_search_depth = k2chess::max_ply;
            time_control.time_remains = 0;

            arg1 = arg2;
        }
        else if(arg1 == "depth")
        {
            arg1 = GetFirstArg(arg2, &arg2);
            time_control.max_search_depth = atoi(arg1.c_str());
            time_control.time_base = INFINITY;
            time_control.time_remains = time_control.time_base;
            time_control.max_nodes_to_search = 0;

            arg1 = arg2;
        }
        else if(arg1 == "nodes")
        {
            arg1 = GetFirstArg(arg2, &arg2);
            time_control.time_base = INFINITY;
            time_control.time_remains = time_control.time_base;
            time_control.max_nodes_to_search = atoi(arg1.c_str());
            time_control.max_search_depth = k2chess::max_ply;

            arg1 = arg2;
        }
        else if(arg1 == "ponder")
        {
            pondering_in_process = true;
            arg1 = arg2;
        }
        else if(arg1 == "searchmoves")
        {
            while(true)
            {
                arg1 = GetFirstArg(arg2, &arg2);
                if(!LooksLikeMove(arg1))
                    break;
                auto move = MoveFromStr(arg1.c_str());
                arg1 = arg2;
                if(move.flag == not_a_move || !IsPseudoLegal(move) ||
                        !IsLegal(move))
                    continue;
                root_moves_to_search.push_back(move);
            }
            arg1 = arg2;
        }
        else
            break;

    }
    if(no_movestogo_arg)
        time_control.moves_per_session = 0;
}





//--------------------------------
void k2main::PonderhitCommand(const std::string &in)
{
    (void)(in);

    PonderHit();
}





//--------------------------------
void k2main::MemoryCommand(const std::string &in)
{
    std::string arg1, arg2;
    arg1 = in;
    arg1 = GetFirstArg(arg1, &arg2);
    auto size_MB = atoi(arg1.c_str());
    ReHash(size_MB);
}





//--------------------------------
void k2main::AnalyzeCommand(const std::string &in)
{
    (void)(in);

    force = false;
    time_control.infinite_analyze = true;

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
void k2main::ExitCommand(const std::string &in)
{
    StopCommand(in);
    time_control.infinite_analyze = false;
}





//--------------------------------
k2chess::piece_type_t k2main::GetTypeForPst(const char char_type)
{
    char chars[] = "kqrbnp";
    piece_type_t ans = 0;
    for(; ans < piece_types; ++ans)
        if(chars[ans] == char_type)
            break;
    return ans;
}





//--------------------------------
bool k2main::SetPstValue(const std::string &param, const eval_t val,
                         const bool is_mid)
{
    if(param.size() != 7)
        return false;
    const coord_t col = param[5] - 'a', row = param[6] - '1';
    const auto type = GetTypeForPst(param[4]);
    if(type >= piece_types || col > max_col || row > max_row)
        return false;

    if(is_mid)
        pst[type][max_row - row][col].mid = val;
    else
        pst[type][max_row - row][col].end = val;
    return true;
}





//--------------------------------
bool k2main::SetParamValue(const std::string &param, const eval_t val,
                           const bool is_mid)
{
    if(param.rfind("pst_", 0) == 0)
        return SetPstValue(param, val, is_mid);

    auto p = eval_params.find(param);
    if(p == eval_params.end())
        return false;

    if(is_mid)
        p->second->mid = val;
    else
        p->second->end = val;
    return true;
}





//--------------------------------
bool k2main::GetParamValue(const std::string &param, eval_t * const val,
                           const bool is_mid)
{
    if(param.rfind("pst_", 0) == 0)
        return GetPstValue(param, val, is_mid);
    else
        for(auto p : eval_params)
        {
            if(param != p.first)
                continue;
            *val = is_mid ? p.second->mid : p.second->end;
            return true;
        }
    return false;
}





//--------------------------------
bool k2main::GetPstValue(const std::string &param, eval_t * const val,
                           const bool is_mid)
{
    if(param.size() != 7)
        return false;
    const coord_t col = param[5] - 'a', row = param[6] - '1';
    const auto type = GetTypeForPst(param[4]);
    if(type >= piece_types || col > max_col || row > max_row)
        return false;
    if(is_mid)
        *val = pst[type][max_row - row][col].mid;
    else
        *val = pst[type][max_row - row][col].end;
    return true;
}





//--------------------------------
bool k2main::ParamNameAndStage(std::string &arg, bool &is_mid)
{
    int comma_pos = arg.find_first_of('.');
    const auto mid_end = arg.substr(comma_pos + 1, 3);
    if(comma_pos == -1 || (mid_end != "mid" && mid_end != "end"))
    {
        std::cout << "error: param name must contain stage (.mid or .end)\n";
        return false;
    }
    arg = arg.substr(0, comma_pos);
    is_mid = mid_end == "mid";
    return true;
}





//--------------------------------
void k2main::SetvalueCommand(const std::string &in)
{
    std::string arg1, arg2, tmp;
    arg1 = GetFirstArg(in, &arg2);
    bool is_mid;
    if(!ParamNameAndStage(arg1, is_mid))
        return;
    arg2 = GetFirstArg(arg2, &tmp);
    if(!SetParamValue(arg1, atof(arg2.c_str()), is_mid))
        std::cout << "error: wrong parameter name" << std ::endl;
}





//--------------------------------
k2main::parsed_pos_s k2main::TuningParsePos(std::string &fen,
                                            const double result)
{
    SetupPosition(fen.c_str());
    parsed_pos_s pos;
    pos.wtm = wtm;
    memcpy(pos.b, b, sizeof(b));
    memcpy(pos.coords, coords, sizeof(coords));
    memcpy(pos.attacks, attacks, sizeof(attacks));
    memcpy(pos.exist_mask, exist_mask, sizeof(exist_mask));
    memcpy(pos.type_mask, type_mask, sizeof(type_mask));
    memcpy(pos.slider_mask, slider_mask, sizeof(slider_mask));
    memcpy(pos.directions, directions, sizeof(directions));
    memcpy(pos.sum_directions, sum_directions, sizeof(sum_directions));
    memcpy(pos.material, material, sizeof(material));
    memcpy(pos.pieces, pieces, sizeof(pieces));
    memcpy(pos.quantity, quantity, sizeof(quantity));
    memcpy(pos.p_max, p_max, sizeof(p_max));
    memcpy(pos.p_min, p_min, sizeof(p_min));
    pos.castling_rights = k2chess::state[0].castling_rights;

    pos.result = result;
    return pos;
}





//--------------------------------
void k2main::TuningApplyPosData(parsed_pos_s *pos)
{
    wtm = pos->wtm;
    memcpy(b, pos->b, sizeof(b));
    memcpy(coords, pos->coords, sizeof(coords));
    memcpy(attacks, pos->attacks, sizeof(attacks));
    memcpy(exist_mask, pos->exist_mask, sizeof(exist_mask));
    memcpy(type_mask, pos->type_mask, sizeof(type_mask));
    memcpy(slider_mask, pos->slider_mask, sizeof(slider_mask));
    memcpy(directions, pos->directions, sizeof(directions));
    memcpy(sum_directions, pos->sum_directions, sizeof(sum_directions));
    memcpy(material, pos->material, sizeof(material));
    memcpy(pieces, pos->pieces, sizeof(pieces));
    memcpy(quantity, pos->quantity, sizeof(quantity));
    memcpy(p_max, pos->p_max, sizeof(p_max));
    memcpy(p_min, pos->p_min, sizeof(p_min));
    k2chess::state[0].castling_rights = pos->castling_rights;
    state[0].eval = InitEvalOfMaterial() + InitEvalOfPST();
}





//--------------------------------
void k2main::TuningLoadCommand(const std::string &in)
{
    using namespace std;
    Timer clock;
    clock.start();
    const auto tick1 = clock.getElapsedTimeInMicroSec();

    training_positions.clear();
    string line;
    ifstream myfile(in);
    if(!myfile)
    {
        cout << "Error: epd file not found" << endl;
        return;
    }
    cout << "Allocating memory..." << endl;
    size_t lines = 0;
    while(getline(myfile, line))
        lines++;
    training_positions.reserve(lines);

    cout << "Loading..." << endl;
    myfile.clear();
    myfile.seekg(0);
    while(getline(myfile, line))
    {
        const auto pos2 = line.rfind("\"");
        const auto pos1 = line.rfind("\"", pos2 - 1);
        const auto res_str = line.substr(pos1 + 1, pos2 - pos1 - 1);
        float result;
        if(res_str == "1-0")
            result = 1;
        else if(res_str == "0-1")
            result = 0;
        else if(res_str == "1/2-1/2")
            result = 0.5;
        else
        {
            cout << "Error: epd file labeled incorrectly" << endl;
            return;
        }
        const auto parsed_pos = TuningParsePos(line, result);
        training_positions.push_back(parsed_pos);
    }
    const auto tick2 = clock.getElapsedTimeInMicroSec();
    const auto deltaTick = tick2 - tick1;

    cout << training_positions.size() << " positions loaded successfully, "
         << "elapsed time is " << deltaTick / 1000000. << " s." << endl;
}





//--------------------------------
double k2main::GetEvalError()
{
    double sum = 0;
    for(auto pos : training_positions)
    {
        TuningApplyPosData(&pos);
        double eval = Eval(pos.wtm, state[0].eval);
        if(!pos.wtm)
            eval = -eval;
        const auto sigm = sigmoid(eval);
        const auto dif = (pos.result - sigm);
        const auto sq_dif = dif*dif;
        sum += sq_dif;
    }
    return sum/training_positions.size();
}





//--------------------------------
void k2main::TuningResultCommand(const std::string &in)
{
    (void)(in);
    using namespace std;

    Timer clock;
    clock.start();
    const auto tick1 = clock.getElapsedTimeInMicroSec();

    const auto ans = GetEvalError();

    const auto tick2 = clock.getElapsedTimeInMicroSec();
    const auto deltaTick = tick2 - tick1;

    cout << "Eval error: " << scientific << ans << fixed <<
            ", elapsed time: " << deltaTick / 1000000. << " s." << endl;
}





//--------------------------------
bool k2main::TuneOneParam(const std::string &param, const bool is_mid,
                          eval_t &left_arg, eval_t &right_arg,
                          double &left_err, double &right_err,
                          tune_flag &flag, const double ratio)
{
    const bool last_iter = right_arg - left_arg <= 2;
    eval_t delta = last_iter ? 0 : round((right_arg - left_arg)*ratio);
    if(delta == 2)
        delta = 1;
    const auto new_left_arg = left_arg + delta;
    const auto new_right_arg = right_arg - delta;
    if(flag != tune_flag::calc_left)
    {
        SetParamValue(param, new_left_arg, is_mid);
        left_err = GetEvalError();
    }
    if(flag != tune_flag::calc_right)
    {
        SetParamValue(param, new_right_arg, is_mid);
        right_err = GetEvalError();
    }
    std::cout << " value: [" << left_arg << " .. " << right_arg << "] => [" <<
                 new_left_arg << " .. " << new_right_arg <<
                 "], err: [" << 100*left_err << ", " << 100*right_err << "]";
    if(last_iter)
    {
        SetParamValue(param, left_arg + 1, is_mid);
        const auto new_err = GetEvalError();
        if(new_err < left_err && new_err < right_err)
            left_arg = right_arg = left_arg + 1;
        else if(left_err < new_err && left_err < right_err)
            right_arg = left_arg;
        else
            left_arg = right_arg;
        return false;
    }

    if(left_err >= right_err)
        left_arg = new_left_arg;
    else
        right_arg = new_right_arg;

    if(flag != tune_flag::calc_both && left_err >= right_err)
    {
        left_err = right_err;
        flag = tune_flag::calc_left;
    }
    else if(flag != tune_flag::calc_both && left_err < right_err)
    {
        right_err = left_err;
        flag = tune_flag::calc_right;
    }
    return true;
}





//--------------------------------
void k2main::TuneParamCommand(const std::string &in)
{
    using namespace std;
    string param, arg1, arg2;
    param = GetFirstArg(in, &arg1);
    bool is_mid;
    if(!ParamNameAndStage(param, is_mid))
        return;
    arg1 = GetFirstArg(arg1, &arg2);
    eval_t left_arg = atof(arg1.c_str());
    arg1 = GetFirstArg(arg2, &arg2);
    eval_t right_arg = atof(arg1.c_str());

    if(!SetParamValue(param, 0, is_mid))
    {
        cout << "Error: wrong param name" << endl;
        return;
    }
    tune_flag flag = tune_flag::calc_both;
    double left_err = 0, right_err = 0;

    for(auto it = 0; it < 100; ++it)
    {
        cout << it << ". ";
        if(!TuneOneParam(param, is_mid, left_arg, right_arg,
                         left_err, right_err, flag, golden_ratio))
            break;
        cout << endl;
    }
    const auto x = (left_arg + right_arg)/2;
    SetParamValue(param, x, is_mid);
    cout << endl << "Finaly: " << x << endl;
}





//--------------------------------
void k2main::TuneCommand(const std::string &in)
{
    using namespace std;
    auto param_vect = TuneFillParamVect(in);
    int it = 0;
    bool finish = false;
    while(!finish)
    {
        finish = true;
        for(auto &p : param_vect)
        {
            if(p.left_arg != p.right_arg)
                cout << endl << it++ << ". " << p.name << " " <<
                        (p.is_mid ? "mid" : "end");
            tune_flag flag = tune_flag::calc_both;
            if(TuneOneParam(p.name, p.is_mid, p.left_arg, p.right_arg,
                            p.left_err, p.right_err, flag, silver_ratio))
                finish = false;
            SetParamValue(p.name, (p.left_arg + p.right_arg)/2, p.is_mid);
        }
    }
    cout << endl << endl;
    for(auto p : param_vect)
    {
        const auto val = (p.left_arg + p.right_arg)/2;
        if(p.is_mid)
            cout << p.name << " = {" << val << ", ";
        else
            cout << val << "}," << endl;
    }
}





//--------------------------------
std::vector<k2main::tune_param_s>
k2main::TuneFillParamVect(const std::string &in)
{
    using namespace std;
    vector<k2main::tune_param_s> param_vect;
    std::string all_the_rest;
    auto arg1 = GetFirstArg(in, &all_the_rest);
    const eval_t margin = atof(arg1.c_str());
    if(margin == 0 && arg1 != "0")
    {
        cout << "Error: first input must be a number with eval margin" << endl;
        return param_vect;
    }
    while(!all_the_rest.empty())
    {
        auto param = GetFirstArg(all_the_rest, &arg1);
        all_the_rest = arg1;
        eval_t cur_val = 0;
        if(!GetParamValue(param, &cur_val, true))
        {
            cout << "Error: wrong param name" << " " << param << endl;
            param_vect.clear();
            return param_vect;
        }
        param_vect.push_back({param, true, AddEval(cur_val, -margin),
                              AddEval(cur_val, margin), 0, 0});
        GetParamValue(param, &cur_val, false);
        param_vect.push_back({param, false, AddEval(cur_val, -margin),
                              AddEval(cur_val, margin), 0, 0});
    }
    return param_vect;
}





//--------------------------------
void k2main::OptionCommand(const std::string &in)
{
    std::string arg1, arg2;
    arg1 = GetFirstArg(in, &arg2);

    if(arg1 == "Randomness" || arg1 == "rand")
    {
        if(arg2 == "1" || arg2 == "true")
            randomness = true;
        else
            randomness = false;
    }
    else if(arg1 == "Separate_thread_for_input" || arg1 == "thread")
    {
        if(arg2 == "1" || arg2 == "true")
            use_thread = true;
        else
            use_thread = false;
    }
    else
        std::cout << "Error: incorrect command options\n";
}





//--------------------------------
void k2main::PostCommand(const std::string &in)
{
    (void)(in);

    enable_output = true;
}





//--------------------------------
void k2main::NopostCommand(const std::string &in)
{
    (void)(in);

    enable_output = false;
}





//--------------------------------
void k2main::SeedCommand(const std::string &in)
{
    std::string arg1, arg2;
    arg1 = in;
    arg1 = GetFirstArg(arg2, &arg2);
    seed = atoi(arg1.c_str());
}

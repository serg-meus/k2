#include "k2.h"

/*
K2, the chess engine
Author: Sergey Meus (serg_meus@mail.ru), Krasnoyarsk Krai, Russia
2012-2025
*/

using std::cout;
using std::endl;


int main(int argc, char* argv[])
{
    (void)(argc);
    (void)(argv);

    std::unique_ptr<k2> eng(new k2);
    eng->commands =
    {
        {"help",        {&k2::help_command,     false}},
        {"new",         {&k2::new_command,      true}},
        {"force",       {&k2::force_command,    true}},
        {"setboard",    {&k2::setboard_command, false}},
        {"set",         {&k2::setboard_command, false}},
        {"quit",        {&k2::quit_command,     true}},
        {"q",           {&k2::quit_command,     true}},
        {"perft",       {&k2::perft_command,    false}},
        {"memory",      {&k2::memory_command,   false}},
        {"post",        {&k2::post_command,     false}},
        {"nopost",      {&k2::nopost_command,   false}},
        {"eval",        {&k2::eval_command,     false}},
        {"go",          {&k2::go_command,       false}},
        {"sd",          {&k2::sd_command,       false}},
        {"sn",          {&k2::sn_command,       false}},
        {"st",          {&k2::st_command,       false}},
        {"protover",    {&k2::protover_command, false}},
        {"level",       {&k2::level_command,    false}},
        {"uci",         {&k2::uci_command,      false}},
        {"isready",     {&k2::isready_command,  false}},
        {"ucinewgame",  {&k2::new_command,      true}},
        {"position",    {&k2::position_command, false}},
        {"setoption",   {&k2::setoption_command,false}},
        {"stop",        {&k2::void_command,     true}},
        {"?",           {&k2::void_command,     true}},
    };
    cout << "K2, the chess engine by Sergey Meus" << endl;
    eng->start();
}


void k2::start()
{
    std::string in_str;
    while (!quit) {
        if (!std::getline(std::cin, in_str)) {
            if (std::cin.eof())
                quit = true;
            else
                std::cin.clear();
        }
        if (execute_command(in_str)) {
        }
        else if (looks_like_move(in_str)) {
            if (game_over() || !enter_move(in_str)) {
                cout << "Illegal move" << endl;
                continue;
            }
            if (!silent_mode && !xboard && !uci)
                cout << board_to_ascii() << endl;
            if (!uci && game_over())
                cout << game_text_result() << endl;
            if (!force && !game_over())
                go_command("");
        }
        else
            cout << "Unknown command: " << in_str << endl;
    }
}


void k2::main_search() {
    nodes = 0;
    ply = 0;
    stop = false;
    auto moves = gen_moves();
    std::random_shuffle(moves.begin(), moves.end());
    move_s best_move = moves.at(0);
    i8 depth = 1;
    int val = 0, alpha , beta, margin = aspiration_margin, mate_cr = 0;
    for (; !stop && depth <= max_depth && mate_cr < max_mate_cr; ++depth) {
        for (unsigned i = 1; i < pv.size(); ++i)
            pv.at(i).clear();
        follow_pv = true;
        alpha = beta = 0;
        while (root_bounds(val, alpha, beta, margin))
            best_move = root_search(depth, alpha, beta, moves, val);
        if (val >= material_values[king_ix] - max_ply)
            mate_cr++;
        if (time_elapsed() >= time_for_move)
            stop = true;
    }
    if (!stop)
        print_search_iteration_result(i8(depth - 1), val, "");
    pv.at(0).push_back(best_move);
}


k2::move_s k2::root_search(const i8 depth, int alpha,
                           const int beta, std::vector<move_s> &moves,
                           int &val) {
    const bool in_check = is_in_check(side);
    move_s best_move = moves.at(0);
    int alpha_orig = alpha;
    pv.at(0).clear();
    unsigned move_num = 0;
    for (; move_num < moves.size(); ++move_num) {
        const auto s = move_to_str(moves.at(move_num));
        if (not_search_moves.size() && not_search_moves.count(s))
            continue;
        if (search_moves.size() && !search_moves.count(s))
            continue;
        make_move(moves.at(move_num));
        val = search_cur_pos(depth, alpha, beta, moves.at(move_num), move_num,
                             move_num ? cut_node : pv_node, in_check);
        unmake_move();
        if (stop)
            break;
        else if (val >= beta) {
            pv.at(0).clear();
            pv.at(0).push_back(moves.at(move_num));
            std::rotate(moves.rbegin() + int(moves.size() - move_num - 1),
                        moves.rbegin() + int(moves.size() - move_num),
                        moves.rend());
            print_search_iteration_result(depth, beta, "\b!");
            break;
        }
        else if (val > alpha) {
            alpha = val;
            store_pv(moves.at(move_num));
            std::rotate(moves.rbegin() + int(moves.size() - move_num - 1),
                        moves.rbegin() + int(moves.size() - move_num),
                        moves.rend());
            print_search_iteration_result(depth, alpha, "");
        }
    }
    if (stop)
        print_search_iteration_result(i8(depth - (move_num == 1)), alpha, "");
    else if (val < beta && alpha_orig == alpha) {
        pv.at(0).clear();
        pv.at(0).push_back(moves.at(0));
        print_search_iteration_result(depth, alpha, "\b?");
    }
    search_result(val, alpha_orig, alpha, beta, depth, depth,
                  best_move, unsigned(moves.size()), in_check);
    reduce_history();
    return best_move;
}


bool k2::root_bounds(int val, int &alpha, int &beta, int &margin) const

{
    static int prev_val;
    if(stop)
        return false;

    if(alpha == beta)
    {
        alpha = sum_eval(val, -margin);
        beta = sum_eval(val, margin);
    }
    else if(val <= alpha)
    {
        margin = aspiration_margin;
        alpha = sum_eval(alpha, aspiration_factor*(alpha - beta));
        beta = sum_eval(beta, margin);
    }
    else if(val >= beta)
    {
        margin = aspiration_margin;
        alpha = sum_eval(alpha, -aspiration_margin);
        beta = sum_eval(beta, aspiration_factor*(beta - alpha));
    }
    else {
        int goal = 32*std::abs(val - prev_val)/aspiration_goal;
        margin += 8*(goal - margin)/aspiration_k_flt;
        alpha = sum_eval(val, -margin);
        beta = sum_eval(val, margin);
//        std::cout << alpha << ", " << beta << ", " << margin << std::endl;
        prev_val = val;
        return false;
    }
    prev_val = val;
    return true;
}


bool k2::looks_like_move(const std::string &in) const {
    if (in.size() < 4 || in.size() > 5)
        return false;
    if (in[0] > 'h' || in[0] < 'a' || in[1] > '8' || in[1] < '0' ||
            in[2] > 'h' || in[2] < 'a' || in[3] > '8' || in[3] < '0')
        return false;
    if (in.size() == 5 && in[4] != 'q' && in[4] != 'r' &&
            in[4] != 'b' && in[4] != 'n')
        return false;
    return true;
}


bool k2::execute_command(const std::string &in) {
    auto params = split(in);
    if (params.size() == 0)
        return true;
    auto param = params[0];
    auto el = commands.find(param);
    if (el == commands.end())
        return false;
    std::string args;
    int first_pos = int(in.find_first_of(" "));
    if (first_pos != -1)
        args = in.substr(size_t(first_pos + 1), in.size());
    if (use_thread) {
        bool store_silent_mode = silent_mode;
        if (el->second.second) {  // immediate command
            stop = true;
            silent_mode = true;
        }
        if (thr.joinable())
            thr.join();
        silent_mode = store_silent_mode;
    }
    ((*this).*(el->second.first))(args);
    return true;
}


void k2::new_command(const std::string &in) {
    auto seed = unsigned(in == "" ? time(nullptr) & 0x0f :
                         std::atoi(in.c_str()));
    std::srand(seed);
    if (!silent_mode)
        cout << "( seed set to " << seed << " )" << endl;
    tt.clear();
    force = false;
    current_clock = time_per_time_control;
    setup_position(start_pos);
}


void k2::force_command(const std::string &in) {
    (void)(in);
    force = true;
}


void k2::quit_command(const std::string &in) {
    (void)(in);
    quit = true;
}


void k2::perft_command(const std::string &in) {
    const u8 depth = u8(std::atoi(in.c_str()));
    if (depth == 0) {
        cout << "Wrong depth" << endl;
        return;
    }
    timer_start();
    u64 perft_nodes = perft(depth, true);
    cout << "\nNodes searched:  " << perft_nodes << '\n';
    auto t1 = time_elapsed();
    cout << "Time spent: " << t1 << '\n';
    cout << "Nodes per second = " <<
        double(perft_nodes)/(t1 + 1e-6) << endl;
}


void k2::setboard_command(const std::string &in) {
    if (in.size() <= 0 || !setup_position(in)) {
        cout << "Wrong position" << endl;
        return;
    }
    if(!silent_mode && !xboard && !uci)
        cout << board_to_ascii() << endl;
}


void k2::memory_command(const std::string &in) {
    auto size_mb = unsigned(std::atoi(in.c_str()));
    if (size_mb == 0 && in != "0") {
        cout << "Wrong size" << endl;
        return;
    }
    tt.resize(size_mb*megabyte);
}


void k2::help_command(const std::string &in) {
    (void)(in);
    cout <<
        "\nType in move (e.g. e2e4) or one of the following commands:\n\n";
    for (auto cmd: commands)
        cout << cmd.first << '\n';
    cout << endl;
}


void k2::post_command(const std::string &in) {
    (void)(in);
    silent_mode = false;
    if (!xboard && !uci)
        cout << board_to_ascii() << endl;
}


void k2::nopost_command(const std::string &in) {
    (void)(in);
    silent_mode = true;
}


void k2::eval_command(const std::string &in) {
    (void)(in);
    cout << "Eval from white's perspective: ";
    auto E = Eval();
    cout << (side ? E : -E) << endl;
}


void k2::go_command(const std::string &in) {
    uci_go_command(in);
    if(use_thread) {
        if(thr.joinable())
            thr.join();
        thr = std::thread(&k2::execute_search, this);
    }
    else
        execute_search();
}


void k2::execute_search() {
    force = false;
    set_time_for_move();
    timer_start();
    main_search();
    move_s best_move = pv.at(0).at(0);
    if(uci)
        cout << "best";
    cout << "move " << move_to_str(best_move) << endl;
    if (!uci) {
        make_move(best_move);
        if (game_over())
            cout << game_text_result() << endl;
    }
    if (!silent_mode && !xboard && !uci)
        cout << board_to_ascii() << endl;
    update_clock();
    pv.at(0).clear();
}


void k2::sd_command(const std::string &in) {
    auto depth = i8(std::atoi(in.c_str()));
    if (depth < 0 || (depth == 0 && in != "0"))
        cout << "Wrong depth" << endl;
    max_depth = depth;
}


void k2::protover_command(const std::string &in) {
    (void)(in);

    xboard = true;

    const std::string feat = "feature ";
    cout << feat << "myname=\"K2 v1.0alpha" << "\"" << endl;
    cout << feat << "setboard=1" << endl;
    cout << feat << "analyze=0" << endl;
    cout << feat << "san=0" << endl;
    cout << feat << "colors=0" << endl;
    cout << feat << "pause=0" << endl;
    cout << feat << "usermove=0" << endl;
    cout << feat << "time=0" << endl;
    cout << feat << "draw=0" << endl;
    cout << feat << "sigterm=0" << endl;
    cout << feat << "sigint=0" << endl;
    cout << feat << "memory=1" << endl;
    cout << feat << "done=1" << endl;
}


void k2::sn_command(const std::string &in) {
    max_nodes = std::stoull(in.c_str());
}


void k2::st_command(const std::string &in) {
    moves_per_time_control = 1;
    time_per_time_control = std::stod(in.c_str());
    time_inc = 0;
    current_clock = time_per_time_control;
}


void k2::level_command(const std::string &in) {
    auto params = split(in);
    if (params.size() != 3) {
        cout << "Wrong number of params" << endl;
        return;
    }
    moves_per_time_control = std::stoi(params.at(0).c_str());
    auto time_params = split(params.at(1), ':');
    time_per_time_control = 60*std::stod(time_params.at(0).c_str());
    if (time_params.size() > 1)
        time_per_time_control += std::stod(time_params.at(1).c_str());
    time_inc = std::stod(params.at(2).c_str());
    current_clock = time_per_time_control;
}


void k2::uci_command(const std::string &in) {
    (void)(in);
    uci = true;
    cout << "id name K2 v1.0 alpha" << endl;
    cout << "id author Sergey Meus" << endl;
    cout << "option name Hash type spin default 64 min 0 max 2048" << endl;
    cout << "uciok" << endl;
}


void k2::isready_command(const std::string &in) {
    (void)(in);
    cout << "readyok\n";
}


void k2::position_command(const std::string &in) {
    auto args = split(in, ' ', 1);
    if (args.at(0) == "fen")
        setup_position(args.at(1));
    else if (args.at(0) == "startpos")
        setup_position(start_pos);
    else
        cout << "Wrong args" << endl;
    args = split(in);
    bool state = false;
    for (auto arg: args) {
        if (arg == "moves") {
            state = true;
            continue;
        }
        if (state)
            enter_move(arg);
    }
}


void k2::setoption_command(const std::string &in) {
    auto args = split(in);
    if (args.size() != 4 || args.at(0) != "name" || args.at(2) != "value") {
        cout << "Wrong args" << endl;
        return;
    }
    if (to_lower(args.at(1)) == "hash")
        memory_command(args.at(3));
}


void k2::uci_go_command(const std::string &in) {
    moves_to_go = 0;
    search_moves.clear();
    std::map<std::string, method_ptr> states = {
        {"infinite", &k2::uci_go_infinite}, {"ponder", &k2::uci_go_ponder},
        {"wtime", &k2::uci_go_wtime}, {"btime", &k2::uci_go_btime},
        {"winc", &k2::uci_go_winc}, {"binc", &k2::uci_go_binc},
        {"depth", &k2::sd_command}, {"nodes", &k2::sn_command},
        {"movetime", &k2::uci_go_movetime}, {"movestogo", &k2::uci_go_movestogo},
        {"searchmoves", &k2::uci_go_searchmoves}
    };
    method_ptr foo = nullptr;
    auto args = split(in);
    for (auto arg: args) {
        auto record = states.find(arg);
        if (record != states.end()) {
            foo = (*record).second;
            if (foo != &k2::uci_go_infinite && foo != &k2::uci_go_ponder)
                continue;
        }
        ((*this).*(foo))(arg);
    }
}


void k2::uci_go_infinite(const std::string &in) {
    (void)(in);
    max_depth = max_ply;
    current_clock = time_per_time_control = infinity;
    max_nodes = 0;
}


void k2::uci_go_ponder(const std::string &in) {
    (void)(in);
}


void k2::uci_go_wtime(const std::string &in) {
    if (side != white)
        return;
    current_clock = std::stod(in.c_str())/1000;
    max_depth = max_ply;
    max_nodes = 0;
}


void k2::uci_go_btime(const std::string &in) {
    if (side != black)
        return;
    current_clock = std::stod(in.c_str())/1000;
    max_depth = max_ply;
    max_nodes = 0;
}


void k2::uci_go_winc(const std::string &in) {
    if (side != white)
        return;
    time_inc = std::stod(in.c_str())/1000;
}


void k2::uci_go_binc(const std::string &in) {
    if (side != black)
        return;
    time_inc = std::stod(in.c_str())/1000;
}


void k2::uci_go_movetime(const std::string &in) {
    moves_to_go = 1;
    time_per_time_control = std::stod(in.c_str())/1000;
    time_inc = 0;
    current_clock = time_per_time_control;
}


void k2::uci_go_movestogo(const std::string &in) {
    moves_to_go = std::atoi(in.c_str());
}


void k2::uci_go_searchmoves(const std::string &in) {
    if (in.at(0) != '!' && in.at(0) != '~')
        search_moves.insert(in);
    else
        not_search_moves.insert(in.substr(1));
}


void k2::void_command(const std::string &in) {
    (void)(in);
}


void k2::print_search_iteration_result(i8 depth, int val, std::string ending) {
    std::string pv_str;
    static int prev_val;
    static std::string prev_pv;
    if (silent_mode)
        return;
    if (stop) {
        val = prev_val;
        pv_str = prev_pv;
    }
    else
        pv_str = prev_pv = pv_string();
    prev_val = val;
    if (!uci) {
        cout << int(depth) << ' ' << val << ' '
            << int(100*time_elapsed()) << ' '  << nodes << ' '
            << pv_str << ending << endl;
        return;
    }
    if (ending.back() == '!')
        ending = " upperbound";
    else if (ending.back() == '?')
        ending = " lowerbound";
    cout << "info depth " << int(depth) << uci_score(val)
        << ending << " time " << int(1000*time_elapsed())
        << " nodes " << nodes << " pv " << pv_str << endl;
}


std::string k2::uci_score(int val) const {
    std::string ans = " score ";
    if (std::abs(val) < material_values[king_ix] - max_ply) {
        ans += "cp " + std::to_string(val);
        return ans;
    }
    ans += "mate ";
    int mate_depth = (material_values[king_ix] - std::abs(val) + 1)/2;
    if (val < 0)
        ans += "-";
    ans += std::to_string(mate_depth);
    return ans;
}

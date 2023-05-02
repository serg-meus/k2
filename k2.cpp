#include "k2.h"

/*
K2, the chess engine
Author: Sergey Meus (serg_meus@mail.ru), Krasnoyarsk Krai, Russia
2012-2023
*/

using std::cout;
using std::endl;


int main(int argc, char* argv[])
{
    (void)(argc);
    (void)(argv);

    std::unique_ptr<k2> eng(new k2);
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
    ((*this).*(el->second))(args);
    return true;
}


void k2::new_command(const std::string &in) {
    auto seed = unsigned(in == "" ? time(0) & 0x0f : std::atoi(in.c_str()));
    std::srand(seed);
    if (!silent_mode)
        cout << "( seed set to " << seed << " )" << endl;
    tt.clear();
    force = false;
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
    if (uci)
        uci_go_command(in);
    force = false;
    set_time_for_move();
    timer_start();
    move_s best_move = search();
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
        cout << "Wron args" << endl;
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
    search_moves.insert(move_from_str(in));
}


void k2::unsupported_command(const std::string &in) {
    (void)(in);
}


k2::move_s k2::search() {
    nodes = 0;
    ply = 0;
    stop = false;
    auto moves = gen_moves();
    std::random_shuffle(moves.begin(), moves.end());
    move_s best_move = moves.at(0);
    for (i8 depth = 1; !stop && depth <= max_depth; ++depth) {
        auto move = root_search(depth, -material[king_ix], material[king_ix],
                                moves);
        if (stop)
            break;
        best_move = move;
    }
    return best_move;
}


k2::move_s k2::root_search(i8 depth, const int alpha_orig, const int beta,
                           std::vector<move_s> &moves) {
    const bool in_check = is_in_check(side);
    move_s best_move = moves.at(0);
    int alpha = alpha_orig, val = 0;
    for (unsigned i = 0; !stop && i < moves.size(); ++i) {
        if (search_moves.size() &&
                search_moves.find(moves.at(i)) != search_moves.end())
            continue;
        make_move(moves.at(i));
        val = search_cur_pos(depth, alpha, beta, moves.at(i), i,
                             i ? cut_node : pv_node, in_check);
        unmake_move();
        if (stop)
            break;
        if (val >= beta || val > alpha) {
            best_move = moves.at(i);
            if (val >= beta)
                break;
            alpha = val;
            std::rotate(moves.rbegin() + int(moves.size() - i - 1),
                        moves.rbegin() + int(moves.size() - i),
                        moves.rend());
            assert(moves.at(0) == best_move);
        }
    }
    search_result(val, alpha_orig, alpha, beta, depth,
                  best_move, unsigned(moves.size()), in_check);
    print_search_iteration_result(depth, val >= beta ? beta : alpha);
    return best_move;
}


void k2::print_search_iteration_result(i8 dpt, int val) {
	std::string pv;
	static int prev_val;
	static std::string prev_pv;
    if (silent_mode)
        return;
    if (stop) {
        --dpt;
        val = prev_val;
        pv = prev_pv;
    }
    else
        pv = prev_pv = pv_string(dpt);
    prev_val = val;
    if (!uci) {
        cout << int(dpt) << ' ' << val << ' '
            << int(100*time_elapsed()) << ' '  << nodes << ' '
            << pv << endl;
        return;
    }
    cout << "info depth " << int(dpt) << uci_score(val)
        << " time " << int(1000*time_elapsed()) << " nodes " << nodes
        << " pv " << pv << endl;
}


std::string k2::pv_string(int dpt) {
    std::string str_out;
    int i = 0;
    for(;i < dpt; ++i) {
        const tt_entry_c *entry = tt.find(hash_key, u32(hash_key >> 32));
        const auto &bmov = entry->result.best_move;
        if (entry == nullptr || bmov == not_a_move || !is_pseudo_legal(bmov))
            break;
        auto move_str = move_to_str(bmov);
        make_move(bmov);
        if (!was_legal(bmov))
            break;
        str_out += move_str + " ";
    }
    for(int j = 0; j < i; ++j)
        unmake_move();
    return str_out;
}


std::string k2::uci_score(int val) {
    std::string ans = " score ";
    if (std::abs(val) < material[king_ix] - max_ply) {
        ans += "cp " + std::to_string(val);
        return ans;
    }
    ans += "mate ";
    int mate_depth = (material[king_ix] - std::abs(val) + 1)/2;
    if (val < 0)
        ans += "-";
    ans += std::to_string(mate_depth);
    return ans;
}


void k2::set_time_for_move() {
    if (!uci)
        moves_to_go = moves_per_time_control ?
            moves_per_time_control - move_cr : 0;
    int mov2go = moves_to_go ? moves_to_go : 30;
    double k_branch = mov2go <= 4 ? 1 : 2;
    time_for_move = current_clock/mov2go/k_branch + time_inc;
    double k_max = mov2go <= 4 ? 1 : 3;
    max_time_for_move = k_max*time_for_move - time_margin;
    if (max_time_for_move >= current_clock)
        max_time_for_move = current_clock - time_margin;
}


void k2::update_clock() {
    if (uci)
        return;
    current_clock += time_inc - time_elapsed();
    move_cr++;
    if (moves_per_time_control && move_cr >= moves_per_time_control) {
        move_cr = 0;
        current_clock += time_per_time_control;
    }
}


std::string k2::game_text_result() {
    std::string ans;
    if (is_mate())
        ans = !side ? "1-0 {White mates}" : "0-1 {Black mates";
    else if (is_stalemate())
        ans = "1/2 - 1/2 {Stalemate}";
    else {
        ans = "1/2 - 1/2 {Draw by ";
        if (is_draw_by_material())
        ans += "insufficient mating material}";
        else if (is_N_fold_repetition(3 - 1))
            ans += "3-fold repetition}";
        else
            ans += "fifty moves rule";
    }
    return ans;
}

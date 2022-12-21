#include "k2.h"

/*
K2, the chess engine
Author: Sergey Meus (serg_meus@mail.ru), Krasnoyarsk Krai, Russia
2012-2022
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


k2::k2() : force(false), quit(false), silent_mode(false), xboard(false),
    max_depth(4)
{
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
            if (!enter_move(in_str))
                cout << "Illegal move" << endl;
            else {
                if (!silent_mode && !xboard)
                    cout << board_to_ascii() << endl;
                if (!force)
                    go_command("");
            }
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
    (void)(in);
    tt.clear();
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
    auto t0 = std::chrono::high_resolution_clock::now();
    u64 perft_nodes = perft(depth, true);
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = t1 - t0;
    cout << "\nNodes searched:  " << perft_nodes << endl;
    cout << "Time spent: " << elapsed.count() << endl;
    cout << "Nodes per second = " <<
        double(perft_nodes)/(elapsed.count() + 1e-6) << endl;
}


void k2::setboard_command(const std::string &in) {
    if (in.size() <= 0 || !setup_position(in)) {
        cout << "Wrong position" << endl;
        return;
    }
    if(!silent_mode && !xboard)
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
    (void)(in);
    force = false;
    move_s best_move = root_search(max_depth);
    if (!silent_mode)
        cout << "move " << move_to_str(best_move) << '\n' << endl;
    make_move(best_move);
    if (!silent_mode && !xboard)
        cout << board_to_ascii() << endl;
}


void k2::sd_command(const std::string &in) {
    auto depth = i8(std::atoi(in.c_str()));
    if (depth == 0)
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
}


void k2::unsupported_command(const std::string &in) {
    (void)(in);
}


k2::move_s k2::root_search(i8 depth_max) {
    nodes = 0;
    move_s best_move;
    for (i8 dpt = 0; dpt < depth_max; ++dpt) {
        int val = search(dpt, -material[king_ix], material[king_ix], pv_node);
        if (!silent_mode)
            cout << int(dpt + 1) << ' ' << val << " 0 " << nodes << ' '
                << pv_string(best_move, dpt + 1) << endl;
    }
    return best_move;
}


std::string k2::pv_string(move_s &best_move, int dpt) {
    std::string str_out;
    int ply = 0;
    for(;ply < dpt; ++ply) {
        const tt_entry_c *entry = tt.find(hash_key, u32(hash_key >> 32));
        if (entry == nullptr || entry->result.best_move == not_a_move)
            break;
        if (ply == 0)
            best_move = entry->result.best_move;
        str_out += move_to_str(entry->result.best_move) + " ";
        make_move(entry->result.best_move);
    }
    for(int i = 0; i < ply; ++i)
        unmake_move();
    return str_out;
}

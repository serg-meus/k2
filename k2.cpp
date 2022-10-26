#include "k2.h"

/*
K2, the chess engine
Author: Sergey Meus (serg_meus@mail.ru), Krasnoyarsk Krai, Russia
2012-2022
*/

int main(int argc, char* argv[])
{
    (void)(argc);
    (void)(argv);

    std::unique_ptr<k2> eng(new k2);
    std::cout << "K2, the chess engine by Sergey Meus" << std::endl;
    std::cout << "Type 'help' for help" << std::endl;
    eng->start();
}

k2::k2() : force(false), quit(false)
{
}

void k2::start()
{
    std::string in_str;
    while(!quit)
    {
        if (!std::getline(std::cin, in_str))
        {
            if (std::cin.eof())
                quit = true;
            else
                std::cin.clear();
        }

        if (execute_command(in_str))
        {
        }
        else if (looks_like_move(in_str))
        {
            if (!enter_move(in_str))
                std::cout << "Illegal move" << std::endl;
            else if (!force)
            {
                //auto nodes = perft(4, false);
                //std::cout << nodes << std::endl;
            }
        }
        else
            std::cout << "Unknown command: " << in_str << std::endl;
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
        std::cout << "Wrong depth" << std::endl;
        return;
    }
    auto t0 = std::chrono::high_resolution_clock::now();
    u64 nodes = perft(depth, true);
    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = t1 - t0;
    std::cout << "\nNodes searched:  " << nodes << std::endl;
    std::cout << "Time spent: " << elapsed.count() << std::endl;
    std::cout << "Nodes per second = " <<
        double(nodes)/(elapsed.count() + 1e-6) << std::endl;
}

void k2::setboard_command(const std::string &in) {
    if (in.size() <= 0 || !setup_position(in)) {
        std::cout << "Wrong position" << std::endl;
        return;
    }
}

void k2::memory_command(const std::string &in) {
    auto size_mb = unsigned(std::atoi(in.c_str()));
    if (size_mb == 0 && in != "0") {
        std::cout << "Wrong size" << std::endl;
        return;
    }
    tt.resize(size_mb*megabyte);
}

void k2::help_command(const std::string &in) {
    (void)(in);
    std::cout << "\nAvailable commands:\n";
    for (auto cmd: commands)
        std::cout << cmd.first << '\n';
    std::cout << std::endl;
}

void k2::unsupported_command(const std::string &in) {
    (void)(in);
}

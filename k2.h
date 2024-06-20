#include "engine.h"
#include <memory>
#include <map>
#include <thread>

class k2 : public engine
{

public:

    k2() : commands(), force(false), quit(false), silent_mode(false),
           xboard(false), uci(false), max_depth(max_ply), search_moves(),
           not_search_moves(), time_for_move(0), time_per_time_control(60),
           time_inc(0), current_clock(60), moves_per_time_control(0),
           moves_to_go(0), move_cr(0), use_thread(true), thr() {
        std::srand(unsigned(time(nullptr)));
    }

    void start();
    void main_search();

    typedef void(k2::*method_ptr)(const std::string &);
    std::map<std::string, std::pair<method_ptr, bool>> commands;

    void new_command(const std::string &in);
    void force_command(const std::string &in);
    void quit_command(const std::string &in);
    void perft_command(const std::string &in);
    void go_command(const std::string &in);
    void setboard_command(const std::string &in);
    void memory_command(const std::string &in);
    void help_command(const std::string &in);
    void post_command(const std::string &in);
    void nopost_command(const std::string &in);
    void eval_command(const std::string &in);
    void sd_command(const std::string &in);
    void protover_command(const std::string &in);
    void sn_command(const std::string &in);
    void st_command(const std::string &in);
    void level_command(const std::string &in);
    void uci_command(const std::string &in);
    void isready_command(const std::string &in);
    void position_command(const std::string &in);
    void setoption_command(const std::string &in);
    void void_command(const std::string &in);
    void execute_search();

protected:

    bool force, quit, silent_mode, xboard, uci;
    i8 max_depth;
    std::set<std::string> search_moves, not_search_moves;
    double time_for_move, time_per_time_control,
        time_inc, current_clock;
    int moves_per_time_control, moves_to_go, move_cr;
    bool use_thread;
    std::thread thr;

    const double time_margin = 0.02;

    bool execute_command(const std::string &in);
    void uci_go_command(const std::string &in);
    void uci_go_infinite(const std::string &in);
    void uci_go_ponder(const std::string &in);
    void uci_go_wtime(const std::string &in);
    void uci_go_btime(const std::string &in);
    void uci_go_winc(const std::string &in);
    void uci_go_binc(const std::string &in);
    void uci_go_moves(const std::string &in);
    void uci_go_movestogo(const std::string &in);
    void uci_go_movetime(const std::string &in);
    void uci_go_searchmoves(const std::string &in);

    bool looks_like_move(const std::string &in) const;
    move_s root_search(const i8 depth, const int alpha_orig, const int beta,
                       std::vector<move_s> &moves);
    void print_search_iteration_result(i8 dpt, int val);
    std::string uci_score(int val) const;


    void timer_start() {
        t_beg = std::chrono::high_resolution_clock::now();
    }

    void update_clock() {
        if (uci)
            return;
        current_clock += time_inc - time_elapsed();
        move_cr++;
        if (moves_per_time_control && move_cr >= moves_per_time_control) {
            move_cr = 0;
            current_clock += time_per_time_control;
        }
    }

    void set_time_for_move() {
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
};

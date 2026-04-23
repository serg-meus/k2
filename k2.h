#include "engine.h"
#include <memory>
#include <map>
#include <thread>
#include <fstream>

class k2 : public engine
{

public:

    k2() : commands(), train_features(), force(false), quit(false),
            silent_mode(false), xboard(false), uci(false), max_depth(max_ply),
            search_moves(), not_search_moves(), time_for_move(0),
            time_per_time_control(60), time_inc(0), current_clock(60),
            moves_per_time_control(0), moves_to_go(0), move_cr(0),
            use_thread(USE_THREAD), thr(), rnd_gen(0), training_positions() {
        rnd_gen = std::mt19937(unsigned(time(nullptr)));
        train_features = {
            &piece_values[pawn_ix], &piece_values[knight_ix],
            &piece_values[bishop_ix], &piece_values[rook_ix],
            &piece_values[queen_ix],
            &king_saf_no_shelter, &king_saf_attacks1, &king_saf_attacks2,
            &mob_Kx, &mob_Bx, &mob_Ky, &mob_By, &mob_PN, &mob_BR, &mob_QK,
            &pawn_doubled, &pawn_isolated, &pawn_dbl_iso, &pawn_hole, &pawn_gap,
            &pawn_king_tropism1, &pawn_king_tropism2, &pawn_king_tropism3,
            &pawn_pass0, &pawn_pass1, &pawn_pass2, &pawn_blk_pass0,
            &pawn_blk_pass1, &pawn_blk_pass2, &pawn_unstoppable,
            &hang_pieces1, &hang_pieces2,&bishop_pair,
            &imb_PPp, &imb_Pn, &imb_Pb, &imb_Nr, &imb_Br, &imb_no_pawns
        };
    }

    void start();
    void main_search();

    typedef void(k2::*method_ptr)(const std::string &);
    std::map<std::string, std::pair<method_ptr, bool>> commands;
    std::vector<vec2<eval_t> *> train_features;

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
    void traindata_command(const std::string &in);
    void trainresult_cmd(const std::string &in);
    void trainvec_command(const std::string &in);

protected:

    struct train_pos_s {
        std::array<std::array<u64, occupancy_ix + 1>, 2> bb;
        bool side;
        float result;
    };

    bool force, quit, silent_mode, xboard, uci;
    i8 max_depth;
    std::set<std::string> search_moves, not_search_moves;
    double time_for_move, time_per_time_control,
        time_inc, current_clock;
    int moves_per_time_control, moves_to_go, move_cr;
    bool use_thread;
    std::thread thr;
    std::mt19937 rnd_gen;
    std::vector<train_pos_s> training_positions;

    const double time_margin = 0.02;
    const int aspiration_margin = 43;
    const int aspiration_factor = 3;
    const int max_mate_cr = 2;
    const int aspiration_k_flt = 40;
    const int aspiration_goal = 16;

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
    bool root_bounds(int x, int &alpha, int &beta, int &margin) const;
    bool looks_like_move(const std::string &in) const;
    move_s root_search(const i8 depth, int alpha_orig, const int beta,
                       std::vector<move_s> &moves, int &val);
    void print_search_iteration_result(i8 dpt, int val, std::string ending);
    std::string uci_score(int val) const;
    double eval_error();
    train_pos_s parse_position(const std::string &in, float result);
    void apply_training_position(const train_pos_s &pos);


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

    int sum_eval(int x1, int x2) const
    {
        if(x1 + x2 >= material_values[king_ix])
            return material_values[king_ix];
        else if(x1 + x2 <= -material_values[king_ix])
            return -material_values[king_ix];
        return x1 + x2;
    }

    double sigmoid(double x) const
    {
        double K = 1.3;
        return 1/(1 + pow(10, -K*x/400));
    }
};

#include "engine.h"
#include <memory>
#include <map>

class k2 : public engine
{

public:

    k2();
    void start();

protected:

    bool force;
    bool quit;
    bool silent_mode;
    bool xboard;
    i8 max_depth;

    bool looks_like_move(const std::string &in) const;
    bool execute_command(const std::string &in);
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
    void unsupported_command(const std::string &in);
    move_s root_search(i8 depth_max);
    std::string pv_string(int dpt);

    typedef void(k2::*method_ptr)(const std::string &);
    std::map<std::string, method_ptr> commands =
    {
        {"help",        &k2::help_command},
        {"new",         &k2::new_command},
        {"force",       &k2::force_command},
        {"setboard",    &k2::setboard_command},
        {"set",         &k2::setboard_command},
        {"quit",        &k2::quit_command},
        {"q",           &k2::quit_command},
        {"perft",       &k2::perft_command},
        {"memory",      &k2::memory_command},
        {"post",        &k2::post_command},
        {"nopost",      &k2::nopost_command},
        {"eval",        &k2::eval_command},
        {"go",          &k2::go_command},
        {"sd",          &k2::sd_command},
        {"protover",    &k2::protover_command},
    };
};

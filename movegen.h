#include "eval.h"
#include <limits>





class k2movegen : public k2eval
{


protected:


    typedef u8 movcr_t;
    typedef u8 apprice_t;
    typedef u32 history_t;


    const priority_t
    second_killer = 198,
    first_killer  = 199,
    move_from_hash = std::numeric_limits<priority_t>::max(),
    king_capture = std::numeric_limits<priority_t>::max(),
    bad_captures = 63;

    const apprice_t
    not_apprice = 0,
    apprice_only_captures = 1,
    apprice_all  = 2;





    move_c pv[max_ply][max_ply + 1];          // the 'flg' property of first element in a row is length of PV at that depth
    move_c killers[max_ply][2];
    history_t history[2][6][128];
    history_t min_history, max_history;

protected:


    void    InitMoveGen();
    movcr_t GenMoves(move_c *list, move_c *best_move, priority_t apprice);
    streng_t SEE_main(move_c m);
    movcr_t GenCaptures(move_c *list);


private:


    void    GenPawn(move_c *list, movcr_t *movCr, iterator it);
    void    GenPawnCap(move_c *list, movcr_t *movCr, iterator it);
    void    GenCastles(move_c *list, movcr_t *movCr);
    void    AppriceMoves(move_c *list, movcr_t moveCr, move_c *best_move);
    void    AppriceQuiesceMoves(move_c *list, movcr_t moveCr);
    streng_t SEE(coord_t to, streng_t frStreng, score_t val, bool stm);
    iterator SeeMinAttacker(coord_t to);
    void    SortQuiesceMoves(move_c *move_array, movcr_t moveCr);
    void    PushMove(move_c *move_array, movcr_t *movCr, iterator it,
                     coord_t to, move_flag_t flg)
    {
        move_c    m;
        m.pc    = it;
        m.to    = to;
        m.flg   = flg;

        move_array[(*movCr)++] = m;
    }

};

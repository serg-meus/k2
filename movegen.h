#include "eval.h"
#include "extern.h"
#include <limits>



//--------------------------------
//#define DONT_USE_SEE_SORTING
//#define DONT_USE_SEE_CUTOFF



typedef u8 movcr_t;
typedef u8 apprice_t;



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





//--------------------------------
void    InitMoveGen();
movcr_t GenMoves(move_c *list, move_c *best_move, priority_t apprice);
void    GenPawn(move_c *list, movcr_t *movCr, iterator it);
void    GenPawnCap(move_c *list, movcr_t *movCr, iterator it);
void    GenCastles(move_c *list, movcr_t *movCr);
movcr_t GenCaptures(move_c *list);
void    AppriceMoves(move_c *list, movcr_t moveCr, move_c *best_move);
void    AppriceQuiesceMoves(move_c *list, movcr_t moveCr);
streng_t SEE(coord_t to, streng_t frStreng, score_t val, bool stm);
iterator SeeMinAttacker(coord_t to);
streng_t SEE_main(move_c m);
void    SortQuiesceMoves(move_c *move_array, movcr_t moveCr);
void    PushMove(move_c *move_array, movcr_t *movCr, iterator it,
                 coord_t to, move_flag_t flg);
//--------------------------------

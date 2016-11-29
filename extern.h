#if !defined(EXTERN)
#define EXTERN





extern u8 b[137];
extern short_list<u8, lst_sz> coords[2];
extern u8       slider[7];
extern u8       rays[7];
extern u8       attacks[240];
extern i8       shifts[6][8];
extern u8       pc_streng[6];
extern int      material[2];
extern int      pieces[2];
extern unsigned wtm;
extern unsigned ply;
extern u64      nodes;
extern Move     pv[max_ply][max_ply + 1];
extern Move     killers[max_ply][2];
extern char     *cv;
extern unsigned history [2][6][128];
extern i8       pst[6][2][8][8];
extern short    val_opn;
extern short    val_end;
extern u8       unused1[24];
extern BrdState b_state[prev_states + max_ply];
extern u16      reversible_moves;
extern char     cur_moves[5*max_ply];
extern u8       pawn_max[10][2];
extern u8       pawn_min[10][2];
extern u64      hash_key;
extern bool     uci;
extern bool     xboard;
extern bool     infinite_analyze;
extern short    streng[7];
extern u64      tmpCr;
extern iterator king_coord[2];
extern bool     pondering_in_process;
extern char     king_dist[120];
extern float    pst_gain[6][2];
extern short    sort_streng[7];
extern u8       quantity[2][6 + 1];
extern u64      doneHashKeys[101 + max_ply];

#endif

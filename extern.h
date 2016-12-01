#if !defined(EXTERN)
#define EXTERN





extern piece_t  b[137];
extern short_list<coord_t, lst_sz>
                coords[2];
extern bool     slider[7];
extern dist_t rays[7];
extern attack_t attacks[240];
extern shifts_t shifts[6][8];
extern streng_t pc_streng[6];
extern streng_t material[2];
extern streng_t pieces[2];
extern side_to_move_t wtm;
extern depth_t  ply;
extern node_t   nodes;
extern move_c   pv[max_ply][max_ply + 1];
extern move_c   killers[max_ply][2];
extern char     *cv;
extern history_t history [2][6][128];
extern pst_t    pst[6][2][8][8];
extern score_t  val_opn;
extern score_t  val_end;
extern state_s  b_state[prev_states + max_ply];
extern depth_t  reversible_moves;
extern char     cur_moves[5*max_ply];
extern rank_t   pawn_max[10][2];
extern rank_t   pawn_min[10][2];
extern hash_key_t hash_key;
extern bool     uci;
extern bool     xboard;
extern bool     infinite_analyze;
extern streng_t streng[7];
extern iterator king_coord[2];
extern bool     pondering_in_process;
extern dist_t   king_dist[120];
extern float    pst_gain[6][2];
extern streng_t sort_streng[7];
extern piece_num_t quantity[2][6 + 1];
extern hash_key_t doneHashKeys[101 + max_ply];

#endif

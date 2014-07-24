#if !defined(EXTERN)
#define EXTERN
//extern UC       b[128];
extern UC b[137];
extern short_list<UC, lst_sz> coords[2];
extern UC       slider[7];
extern UC       rays[7];
extern UC       attacks[240];
extern SC       shifts[6][8];
extern UC       pc_streng[6];
extern int      material[2];
extern int      pieces[2];
extern unsigned wtm;
extern unsigned ply;
extern UQ       nodes;
extern Move     pv[max_ply][max_ply + 1];
extern Move     kil[max_ply][2];
extern char     *cv;
extern unsigned history [2][6][128];
extern SC       pst[6][2][8][8];
extern short    valOpn;
extern short    valEnd;
extern UC       unused1[24];
extern BrdState boardState[prev_states + max_ply];
extern US       reversibleMoves;
extern char     curVar[5*max_ply];
extern int      pmax[10][2];
extern int      pmin[10][2];
extern UQ       hash_key;
extern bool     uci;
extern bool     xboard;
extern bool     analyze;
extern short    streng[7];
extern UQ       tmpCr;
extern short_list<UC, lst_sz>::iterator king_coord[2];
#endif

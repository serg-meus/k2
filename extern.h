#if !defined(EXTERN)
#define EXTERN
//extern UC       b[128];
extern UC       b[137];
extern std::list<UC>    pcs[2];
extern UC       slider[7];
extern UC       rays[7];
extern UC       attacks[240];
extern SC       shifts[6][8];
extern UC       pc_streng[7];
extern int      material[2];
extern int      pieces[2];
extern unsigned wtm;
extern unsigned ply;
extern int      moveCr;
extern UQ       nodes;
extern Move     pv[MAX_PLY][MAX_PLY + 1];
extern Move     kil[MAX_PLY][2];
extern char     *cv;
//extern unsigned history [2][6][128];
extern SC       pst[6][2][8][8];
extern short    valOpn;
extern short    valEnd;
extern UC       unused1[24];
extern BrdState boardState[PREV_STATES + MAX_PLY];
extern US       reversibleMoves;
extern bool     genBestMove;
extern char     curVar[5*MAX_PLY];
extern int      pmax[10][2];
extern int      pmin[10][2];
extern UQ       hash_key;
extern Move     bestMoveToGen;
extern bool     uci;
extern bool     analyze;
extern short    streng[7];
#endif

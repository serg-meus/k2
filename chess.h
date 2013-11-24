#include <cstring>
#include <stdint.h>
#include <assert.h>
//--------------------------------
#define USE_PAWN_STRUCT
//--------------------------------
#define UC uint8_t
#define SC int8_t
#define US uint16_t
#define UI uint32_t
#define UQ unsigned long long
//--------------------------------
#define MAX_PLY         100
#define PREV_STATES     4
#define WHT             0x20
#define BLK             0x00
#define ONBRD(X)        (!((X) & 0x88))
#define XY2SQ(X, Y)     (((Y) << 4) + (X))
#define COL(SQ)         ((SQ) & 7)
#define ROW(SQ)         ((SQ) >> 4)
#define ABSI(X)         ((X) > 0 ? (X) : (-(X)))
//--------------------------------
#define MOVETO(X)       ((X) & 0xFF)
#define MOVEPC(X)       (((X) >> 8) & 0xFF)
#define MOVEFLG(X)      ((X) & 0x00FF0000)
//--------------------------------
#define MOVE_SCORE_SHIFT 24
enum {__ = 0,  _k, _q, _r, _b, _n, _p, _K = 0x21, _Q, _R, _B, _N, _P};
enum {mCAPT = 0x10, mCS_K = 0x20, mCS_Q = 0x40, mCSTL = 0x60, mENPS = 0x80,
      mPR_Q = 0x01, mPR_N = 0x02, mPR_R = 0x03, mPR_B = 0x04, mPROM = 0x07};

typedef unsigned Move;  // beginning from LSB:
                        // 'to' - coords for piece to move to (8 bit);
                        // 'pc' - number of piece in 'men' array (8 bit);
                        // 'flg' - flags (mCAPT, etc) - (8 bit)
                        // 'scr' - unsigned score (priority) by move generator (8 bit)
                        //      0-15 - bad captures
                        //      16-115 - silent moves (history value)
                        //      119 - equal capture
                        //      121 - from pv
                        //      123 - second killer
                        //      125 - first killer
                        //      128 - 250 - good captures and/or promotions
                        //      255 - opp king capture or hash hit


struct BrdState
{
    UC capt;            // taken piece, 6 bits
    UC ncap;            // number of taken piece, 6 bits
    UC ncapNxt;         // point to next piece, 6 bits
    UC fr;              // from point, 7 bits
    UC cstl;            // castling rights, bits 0..3: _K, _Q, _k, _q, 4 bits


    UC ncstl_r;         // number of castled rook, 6 bits
    UC ep;              // 0 = no_ep, else ep=COL(x) + 1, not null only if opponent pawn is near, 4 bits
    UC nprom;           // number of previous piece for promoted pawn, 6 bits
    US reversibleCr;    // reversible halfmove counter
    UC to;              // to point, 7 bits (for simple repetition draw detection)
    short valOpn;       // store material and PST value considered all material is on the board
    short valEnd;       // store material and PST value considered deep endgame (kings and pawns only)
};

//--------------------------------
void InitChess();
void InitAttacks();
void InitBrd();
bool BoardToMen();
bool FenToBoard(char *p);
void ShowMove(UC fr, UC to);
bool MakeCastle(Move m, UC fr);
void UnMakeCastle(Move m);
bool MakeEP(Move m, UC fr);
bool SliderAttack(UC fr, UC to);
bool Attack(UC to, int xtm);
bool LegalSlider(UC fr, UC to, UC pt);
bool Legal(Move m, bool ic);
void ShowMove(UC fr, UC to);
void SetPawnStruct(int x);
void MovePawnStruct(UC pt, UC fr, Move m);
void InitPawnStruct();
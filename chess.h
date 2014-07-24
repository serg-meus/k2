#include <iostream>
#include <cstring>
#include <stdint.h>
#include "short_list.h"

//--------------------------------
#define ENGINE_VERSION "050x"

//--------------------------------
//#define DONT_SHOW_STATISTICS
//#define DONT_USE_PAWN_STRUCT
//#define DONT_USE_NULL_MOVE
//#define DONT_USE_FUTILITY
//#define DONT_USE_SEE_SORTING
//#define DONT_USE_SEE_CUTOFF
//#define DONT_USE_DELTA_PRUNING
#define DONT_USE_HISTORY
#define DONT_USE_LMR

//--------------------------------
#define UC uint8_t
#define SC int8_t
#define US uint16_t
#define SS int16_t
#define UI uint32_t
#define UQ unsigned long long
//--------------------------------
#define ONBRD(X)        (!((X) & 0x88))
#define XY2SQ(X, Y)     (((Y) << 4) + (X))
#define COL(SQ)         ((SQ) & 7)
#define ROW(SQ)         ((SQ) >> 4)
#define ABSI(X)         ((X) > 0 ? (X) : (-(X)))


const int lst_sz        = 32;                                                 // size of piece list for one colour
const unsigned max_ply  = 100;                                                // maximum search depth
const int prev_states   = 4;
const UC  white         = 1;
const UC  black         = 0;



//--------------------------------
enum {__ = 0,  _k = 2, _q = 4, _r = 6, _b = 8, _n = 10, _p = 12,
               _K = 3, _Q = 5, _R = 7, _B = 9, _N = 11, _P = 13};
enum {mCAPT = 0x10, mCS_K = 0x20, mCS_Q = 0x40, mCSTL = 0x60, mENPS = 0x80,
      mPR_Q = 0x01, mPR_N = 0x02, mPR_R = 0x03, mPR_B = 0x04, mPROM = 0x07};

//--------------------------------
class Move
{
public:
    UC to;
    short_list<UC, lst_sz>::iterator_entity pc;
    UC flg;
    UC scr;

    bool operator == (Move m)   {return to == m.to && pc == m.pc && flg == m.flg;}
    bool operator != (Move m)   {return to != m.to || pc != m.pc || flg != m.flg;}
};
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

//--------------------------------
struct BrdState
{
    UC capt;                                                            // taken piece, 6 bits
    short_list<UC, lst_sz>::iterator_entity captured_it;                // iterator to captured piece
    UC fr;                                                              // from point, 7 bits
    UC cstl;                                                            // castling rights, bits 0..3: _K, _Q, _k, _q, 4 bits

    short_list<UC, lst_sz>::iterator_entity castled_rook_it;            // iterator to castled rook, 8 bits
    UC ep;                                                              // 0 = no_ep, else ep=COL(x) + 1, not null only if opponent pawn is near, 4 bits
    short_list<UC, lst_sz>::iterator_entity nprom;                      // number of next piece for promoted pawn, 6 bits
    US reversibleCr;                                                    // reversible halfmove counter
    UC to;                                                              // to point, 7 bits (for simple repetition draw detection)
    short valOpn;                                                       // store material and PST value considered all material is on the board
    short valEnd;                                                       // store material and PST value considered deep endgame (kings and pawns only)
};

//--------------------------------
struct tt_entry
{
    SS  scr;
    SS  bMov;
    SS  flags;
    UC  depth;
    UC  unused;
};

//--------------------------------
#define PC_LIST_SIZE 64

class piece_list
{

protected:

    UC coord[PC_LIST_SIZE];
    UC ptr_prev[PC_LIST_SIZE];
    UC ptr_next[PC_LIST_SIZE];
    UC id[PC_LIST_SIZE];

    UC _size;
    UC _first;
    UC _last;

public:

    typedef UC iterator;
    piece_list::iterator begin();
    piece_list::iterator end();
    piece_list::iterator rbegin();
    piece_list::iterator rend();
    bool push_front(UC, UC);
    bool push_back(UC, UC);
    bool erase(piece_list::iterator);
    void clear();

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
bool PieceListCompare(UC men1, UC men2);

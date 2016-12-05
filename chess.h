#include <iostream>
#include <cstring>
#include <stdint.h>
#include <assert.h>
#include "short_list.h"





//--------------------------------
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;
typedef unsigned long long u64;

typedef i16 depth_t;
typedef u8 piece_t;
typedef u8 coord_t;

const i32 lst_sz  = 32;                                                 // size of piece list for one colour

typedef short_list<coord_t, lst_sz>::iterator_entity iterator_entity;
typedef short_list<coord_t, lst_sz>::iterator iterator;
typedef i16 score_t;
typedef i16 streng_t;
typedef u8 side_to_move_t;
typedef u8 priority_t;
typedef i16 tropism_t;
typedef u8 move_flag_t;
typedef u8 piece_index_t;
typedef u8 castle_t;
typedef u8 enpass_t;
typedef u8 attack_t;
typedef u8 dist_t;
typedef i8 shifts_t;
typedef u64 hash_key_t;
typedef u64 node_t;
typedef u32 history_t;
typedef u8 piece_num_t;
typedef u8 rank_t;
typedef i8 pst_t;





//--------------------------------
const depth_t max_ply  = 100;                                                // maximum search depth
const depth_t prev_states   = 4;
const piece_t white = 1;
const piece_t black = 0;

const piece_t   __ = 0,
                _k = 2,
                _q = 4,
                _r = 6,
                _b = 8,
                _n = 10,
                _p = 12,
                _K = 3,
                _Q = 5,
                _R = 7,
                _B = 9,
                _N = 11,
                _P = 13;

const move_flag_t   mCAPT = 0x10,
                    mCS_K = 0x20,
                    mCS_Q = 0x40,
                    mCSTL = 0x60,
                    mENPS = 0x80,
                    mPR_Q = 0x01,
                    mPR_N = 0x02,
                    mPR_R = 0x03,
                    mPR_B = 0x04,
                    mPROM = 0x07;





//--------------------------------
class move_c
{
public:
    coord_t to;
    iterator_entity pc;
    move_flag_t flg;
    priority_t scr;

    bool operator == (move_c m)   {return to == m.to && pc == m.pc && flg == m.flg;}
    bool operator != (move_c m)   {return to != m.to || pc != m.pc || flg != m.flg;}
    bool operator < (const move_c m) const {return scr < m.scr;}
};
                                                                        // 'to' - coords for piece to move to (8 bit);
                                                                        // 'pc' - number of piece in 'men' array (8 bit);
                                                                        // 'flg' - flags (mCAPT, etc) - (8 bit)
                                                                        // 'scr' - unsigned score (priority) by move generator (8 bit)
                                                                        //      0..63    - bad captures
                                                                        //      64..127  - silent moves without history (pst value)
                                                                        //      128..195 - silent moves with history value
                                                                        //      196 - equal capture
                                                                        //      197 - move from pv
                                                                        //      198 - second killer
                                                                        //      199 - first killer
                                                                        //      200..250 - good captures and/or promotions
                                                                        //      255 - opp king capture or hash hit

//--------------------------------
struct state_s
{
    piece_t capt;                                                            // taken piece, 6 bits
    iterator_entity captured_it;                                        // iterator to captured piece
    coord_t fr;                                                         // from point, 7 bits
    castle_t cstl;                                                      // castling rights, bits 0..3: _K, _Q, _k, _q, 4 bits

    iterator_entity castled_rook_it;                                    // iterator to castled rook, 8 bits
    enpass_t ep;                                                        // 0 = no_ep, else ep=get_col(x) + 1, not null only if opponent pawn is near, 4 bits
    iterator_entity nprom;                                              // number of next piece for promoted pawn, 6 bits
    depth_t reversibleCr;                                                   // reversible halfmove counter
    coord_t to;                                                              // to point, 7 bits (for simple repetition draw detection)
    score_t val_opn;                                                    // store material and PST value considered all material is on the board
    score_t val_end;                                                    // store material and PST value considered deep endgame (kings and pawns only)
    priority_t scr;                                                             // move priority by move genererator
    tropism_t tropism[2];
};





//--------------------------------
void InitChess();
void InitAttacks();
void InitBrd();
bool BoardToMen();
bool FenToBoard(char *p);
void ShowMove(coord_t fr, coord_t to);
bool MakeCastle(move_c m, coord_t fr);
void UnMakeCastle(move_c m);
bool MakeEP(move_c m, coord_t fr);
bool SliderAttack(coord_t fr, coord_t to);
bool Attack(coord_t to, side_to_move_t xtm);
bool LegalSlider(coord_t fr, coord_t to, piece_t pt);
bool Legal(move_c m, bool ic);
void SetPawnStruct(coord_t col);
void InitPawnStruct();
bool PieceListCompare(coord_t men1, coord_t men2);
void StoreCurrentBoardState(move_c m, coord_t fr, coord_t targ);
void MakeCapture(move_c m, coord_t targ);
void MakePromotion(move_c m, iterator it);
void UnmakeCapture(move_c m);
void UnmakePromotion(move_c m);
bool MkMoveFast(move_c m);
void UnMoveFast(move_c m);
bool within_board(coord_t coord);
coord_t get_coord(coord_t col, coord_t row);
coord_t get_col(coord_t coord);
coord_t get_row(coord_t coord);
coord_t get_index(piece_t X);
piece_t to_black(piece_t piece);

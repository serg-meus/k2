#include <iostream>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <assert.h>
#include <bitset>
#include <vector>
#include "short_list.h"


class k2chess
{

public:


    k2chess();
    bool SetupPosition(const char *fen);
    bool MakeMove(const char* str);
    bool TakebackMove();
    void RunUnitTests();


protected:


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

    typedef i16 streng_t;
    typedef u8 move_flag_t;
    typedef u8 castle_t;
    typedef u8 enpass_t;
    typedef u32 attack_t;
    typedef i8 shifts_t;
    typedef u8 priority_t;

    class k2list : public short_list<coord_t, 16>
    {

    public:

        piece_t *board;
        streng_t *streng;
        coord_t get_piece_type(piece_t piece)  // must be as in k2chess class
        {
            return piece/sides;
        }

        void sort()
        {
            bool replaced = true;
            if(this->size() <= 1)
                return;

            while(replaced)
            {
                replaced = false;
                iterator it;
                for(it = this->begin(); it != --(this->end()); ++it)
                {
                    iterator it_nxt = it;
                    ++it_nxt;
                    auto s_it = streng[get_piece_type(board[*it])];
                    auto s_nxt = streng[get_piece_type(board[*it_nxt])];
                    if(s_it > s_nxt)
                    {
                        this->move_element(it, it_nxt);
                        replaced = true;
                        it = it_nxt;
                    }// if(less_foo(
                }// for(it
            }// while(replaced
        }
    };
    typedef k2list::iterator_entity iterator_entity;
    typedef k2list::iterator iterator;

    const static depth_t max_ply = 100;  // maximum search depth
    const static coord_t board_width = 8;
    const static coord_t board_height = 8;
    const static u8 sides = 2;  // black and white
    const static u8 piece_types = 6;  // pawns, knights, ...
    const static bool white = true;
    const static bool black = false;
    const static depth_t prev_states = 4;
    const static coord_t max_col = board_width - 1;
    const static coord_t max_row = board_height - 1;

    const static piece_t
    empty_square = 0,
    black_king = 2,
    black_queen = 4,
    black_rook = 6,
    black_bishop = 8,
    black_knight = 10,
    black_pawn = 12,
    white_king = 3,
    white_queen = 5,
    white_rook = 7,
    white_bishop = 9,
    white_knight = 11,
    white_pawn = 13;  // don't change any value

    const move_flag_t
    is_capture = 0x10,
    is_right_castle = 0x20,
    is_left_castle = 0x40,
    is_castle = 0x60,
    is_en_passant = 0x80,
    is_promotion_to_queen = 0x01,  // don't change this and below
    is_promotion_to_knight = 0x02,
    is_promotion_to_rook = 0x03,
    is_promotion_to_bishop = 0x04,
    is_promotion = 0x07,
    is_bad_move_flag = 0xff;

    const castle_t
    white_can_castle_right = 1,
    white_can_castle_left = 2,
    black_can_castle_right = 4,
    black_can_castle_left = 8;

    const coord_t default_king_col = 4;
    const shifts_t
    cstl_move_length = 2,
    pawn_default_row = 1,
    pawn_long_move_length = 2;
    static const size_t move_max_display_length = 5;

    const piece_t
    pawn = get_piece_type(black_pawn),
    bishop = get_piece_type(black_bishop),
    knight = get_piece_type(black_knight),
    rook = get_piece_type(black_rook),
    queen = get_piece_type(black_queen),
    king = get_piece_type(black_king);

    const char *start_position =
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Class representing move
    class move_c
    {
    public:
        coord_t to_coord;  // coordinate for piece to move to
        iterator_entity piece_iterator;  // pointer to piece in piece list
        move_flag_t flag;  // special move flags (is_capture, etc)
        priority_t priority;  // priority of move assigned by move generator
        // 0..63 -- bad captures
        // 64..127 -- silent moves with low history value (PST value is used)
        // 128..195 -- silent moves with high history value
        // 198 -- second killer
        // 199 -- first killer
        // 200..250 -- good captures and/or promotions
        // 255 -- king capture or hash hit

        bool operator == (move_c m)
        {
            return to_coord == m.to_coord
                   && piece_iterator == m.piece_iterator
                   && flag == m.flag;
        }

        bool operator != (move_c m)
        {
            return to_coord != m.to_coord
                   || piece_iterator != m.piece_iterator
                   || flag != m.flag;
        }

        bool operator < (const move_c m) const
        {
            return priority < m.priority;
        }
    };

// Structure for storing current state of engine
    struct state_s
    {
        piece_t captured_piece;  // captured piece
        iterator_entity captured_it;  // iterator to captured piece
        coord_t from_coord;  // square coordinate from which move was made
        castle_t cstl;  // castling rights, k2chess::castle_kingside_w, ...
        iterator_entity castled_rook_it;  // iterator to castled rook
        enpass_t ep;  // 0 = no_ep, else ep=get_col(x) + 1,
        // not null only if opponent pawn is near
        iterator_entity nprom;  // next piece iterator for promoted pawn
        depth_t reversible_moves;  // reversible halfmove counter
    };

    bool wtm;  // side to move, k2chess::white or k2chess::black
    piece_t b[board_width*board_height];  // array representing the chess board
    k2list coords[sides];  // black/white piece coordinates

    // two tables for each color with all attacks
    attack_t attacks[sides][board_width*board_height];

    // extended attacks for SEE algorithm
    attack_t xattacks[sides][board_width*board_height];

    // number of directions to move for each kind of piece
    coord_t rays[piece_types + 1];

    // biases defining directions for 'rays'
    shifts_t delta_col[piece_types][board_width];
    shifts_t delta_row[piece_types][board_height];

    // for quick detection if piece is a slider
    bool is_slider[piece_types + 1];

    // piece strength values for material counters
    streng_t pc_streng[piece_types + 1];

    // piece strhengths for move priorities
    streng_t streng[piece_types + 1];

    // piece strhengths for initial sort piece lists
    streng_t sort_streng[piece_types + 1];

    streng_t material[sides];  // material counters
    streng_t pieces[sides];  // piece counters, including kings
    coord_t quantity[sides][piece_types + 1];

    state_s b_state[prev_states + max_ply]; // engine state for each ply depth
    state_s *state;  // pointer to engine state, state[0] = b_state[prev_states];
    depth_t ply;  // current ply depth
    depth_t reversible_moves;

    // current variation (for debug mode only)
    char cur_moves[move_max_display_length*max_ply];
    char *cv;  // current variation pointer (for debug mode only)

    iterator king_coord[sides];  // king coord iterators for black and white

    std::vector<move_c> done_moves;

    bool MakeMove(const move_c m);
    void TakebackMove(const move_c m);
    iterator find_piece(const bool stm, const coord_t coord);
    move_flag_t InitMoveFlag(const move_c move, char promo_to);

    coord_t get_coord(coord_t col, coord_t row)
    {
        return (board_width*row) + col;
    }

    coord_t get_coord(const char *str)
    {
        return get_coord(str[0] - 'a', str[1] - '1');
    }

    coord_t get_col(coord_t coord)
    {
        assert((board_height & (board_height - 1)) == 0);
        return coord & (board_height - 1);
    }

    coord_t get_row(coord_t coord)
    {
        return coord/board_width;
    }

    coord_t get_piece_type(piece_t piece)
    {
        return piece/sides;
    }

    piece_t to_black(piece_t piece)
    {
        return piece & ~white;
    }

    bool col_within(shifts_t col)
    {
        return col >= 0 && col < board_width;
    }

    bool row_within(shifts_t row)
    {
        return row >= 0 && row < board_height;
    }

    bool get_piece_color(piece_t piece)
    {
        return piece & white;
    }
    piece_t set_piece_color(piece_t piece, bool stm)
    {
        return (piece & ~white) | stm;
    }


private:


    void InitAttacks();
    bool InitPieceLists();
    void ShowMove(const coord_t from_coord, const coord_t to_coord);
    void StoreCurrentBoardState(const move_c m, const coord_t from_coord);
    void MakeCapture(const move_c m);
    void MakePromotion(const move_c m, iterator it);
    void UnmakeCapture(const move_c m);
    void UnmakePromotion(move_c m);
    bool MakeCastleOrUpdateFlags(const move_c m, const coord_t from_coord);
    void UnMakeCastle(const move_c m);
    bool MakeEnPassantOrUpdateFlags(const move_c m, const coord_t from_coord);
    void InitAttacksOnePiece(const shifts_t col, const shifts_t row);
    void UpdateAttacks();
    void UpdateAttacksOnePiece();
    char* ParseMainPartOfFen(char *ptr);
    char* ParseSideToMoveInFen(char *ptr);
    char* ParseCastlingRightsInFen(char *ptr);
    char* ParseEnPassantInFen(char *ptr);
    size_t test_count_attacked_squares(bool stm);
    size_t test_count_all_attacks(bool stm);
    void test_attack_tables(size_t att_w, size_t att_b,
                            size_t all_w, size_t all_b);
};

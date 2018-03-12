#include <iostream>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <assert.h>
#include <bitset>
#include <vector>
#include <limits.h>
#include "short_list.h"


class k2chess
{

public:


    k2chess();
    bool SetupPosition(const char *fen);
    bool MakeMove(const char* str);
    void TakebackMove();
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

    typedef i16 eval_t;
    typedef u8 move_flag_t;
    typedef u8 castle_t;
    typedef u8 enpass_t;
    typedef u32 attack_t;
    typedef i8 shifts_t;
    typedef u8 priority_t;
    typedef u8 ray_mask_t;
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
    const static u8 max_pieces_one_side = 16;
    const static u8 max_rays = 8;
    coord_t max_ray_length;

    class k2list : public short_list<coord_t, max_pieces_one_side>
    {

    public:

        piece_t *board;
        eval_t *values;

        coord_t get_type(piece_t piece)  // must be as in k2chess class
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
                    auto s_it = values[get_type(board[*it])];
                    auto s_nxt = values[get_type(board[*it_nxt])];
                    if(s_it > s_nxt)
                    {
                        direct_swap(it, it_nxt);
                        replaced = true;
                    }
                }
#ifndef NDEBUG
                for(it = this->begin(); it != --(this->end()); ++it)
                {
                    iterator it_nxt = it;
                    ++it_nxt;
                    assert(values[get_type(board[*it])] >=
                        values[get_type(board[*it])]);
                }
#endif
            }
        }
    };
    typedef k2list::iterator iterator;

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
    is_promotion = 0x07;
    const static move_flag_t not_a_move = 0x08;

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
    static const attack_t attack_digits = sizeof(attack_t)*CHAR_BIT;

    const piece_t
    pawn = get_type(black_pawn),
    bishop = get_type(black_bishop),
    knight = get_type(black_knight),
    rook = get_type(black_rook),
    queen = get_type(black_queen),
    king = get_type(black_king);

    const ray_mask_t
    rays_North = 1,
    rays_East = 2,
    rays_South = 4,
    rays_West = 8,
    rays_NE = 0x10,
    rays_SE = 0x20,
    rays_SW = 0x40,
    rays_NW = 0x80,
    rays_NNE = 1,
    rays_NEE = 2,
    rays_SEE = 4,
    rays_SSE = 8,
    rays_SSW = 0x10,
    rays_SWW = 0x20,
    rays_NWW = 0x40,
    rays_NNW = 0x80,
    rays_N_or_S = rays_North | rays_South,
    rays_E_or_W = rays_East | rays_West,
    rays_NE_or_SW = rays_NE | rays_SW,
    rays_NW_or_SE = rays_NW | rays_SE,
    rays_rook = rays_N_or_S | rays_E_or_W,
    rays_bishop = rays_NE_or_SW | rays_NW_or_SE;

    const char *start_position =
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Class representing move
    class move_c
    {
    public:
        coord_t to_coord;  // coordinate for piece to move to
        coord_t piece_index;  // pointer to piece in piece list
        move_flag_t flag;  // special move flags (is_capture, etc)
        priority_t priority;  // priority of move assigned by move generator

        bool operator == (move_c m) const
        {
            return to_coord == m.to_coord
                   && piece_index == m.piece_index
                   && flag == m.flag;
        }

        bool operator != (move_c m) const
        {
            return to_coord != m.to_coord
                   || piece_index != m.piece_index
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
        move_c move;  // last move
        piece_t captured_piece;  // captured piece
        coord_t captured_index;  // iterator to captured piece
        coord_t from_coord;  // square coordinate from which move was made
        castle_t castling_rights;  // castling rights, castle_kingside_w, ...
        coord_t castled_rook_index;  // iterator to castled rook
        enpass_t en_passant_rights;  // 0 = no en passant, 1..8 =
        // pawn col + 1, not null only if opponent pawn is near
        depth_t reversible_moves;  // reversible halfmove counter
        attack_t slider_mask;  // see slider_mask below
    };

    struct attack_params_s
    {
        coord_t from_coord;
        coord_t to_coord;
        coord_t piece_coord;
        bool color;
        coord_t type;
        bool is_move;
        bool is_captured;
        bool is_special_move;
        bool is_cstl;
        bool is_enps;
        u8 index;
        u8 cstl_index;
        u8 captured_index;
    };

    // side to move or white to move, k2chess::white (true) or k2chess::black
    bool wtm;

    // array representing the chess board
    piece_t b[board_width*board_height];

    // black/white piece coordinates
    k2list coords[sides];

    // two tables for each color with attacks of all pieces
    attack_t attacks[sides][board_width*board_height];

    // masks for fast detection of attacking sliders
    attack_t slider_mask[sides];

    // masks for fast detection of pawn possible moves
    attack_t pawn_mask[sides];

    // masks for update attack tables
    attack_t update_mask[sides];

    coord_t mobility[sides][max_pieces_one_side][max_rays];

    // number of directions to move for each kind of piece
    coord_t ray_min[piece_types + 1], ray_max[piece_types + 1];

    // ray masks for all types of pieces
    ray_mask_t all_rays[piece_types + 1];

    // biases defining directions for 'rays'
    shifts_t delta_col_kqrb[max_rays];
    shifts_t delta_row_kqrb[max_rays];
    shifts_t delta_col_knight[max_rays];
    shifts_t delta_row_knight[max_rays];

    // for quick detection if piece is a slider
    bool is_slider[piece_types + 1];

    // piece values for material counters,
    // move priorities and sorting piece lists
    eval_t values[piece_types + 1];

    eval_t material[sides];  // material counters
    eval_t pieces[sides];  // piece counters, including kings
    coord_t quantity[sides][piece_types + 1];

    state_s b_state[prev_states + max_ply]; // board state for each ply depth
    state_s *state;  // pointer to board state, state[0] = b_state[prev_states];
    depth_t ply;  // current ply depth
    depth_t reversible_moves;

    // current variation (for debug mode only)
    char cur_moves[move_max_display_length*max_ply];
    char *cv;  // current variation pointer (for debug mode only)

    iterator king_coord[sides];  // king coord iterators for black and white

    attack_t done_attacks[max_ply][sides][board_height*board_width];

    std::vector<k2list> store_coords;

    coord_t done_mobility[max_ply][sides][max_pieces_one_side][max_rays];

    bool MakeMove(const move_c m);
    size_t find_piece(const bool stm, const coord_t coord);
    move_flag_t InitMoveFlag(const move_c move, const char promo_to);
    bool IsLegal(const move_c move);
    bool IsPseudoLegal(const move_c move);
    bool IsLegalCastle(const move_c move) const;
    bool IsOnRay(const coord_t given, const coord_t k_coord,
                 const coord_t attacker_coord) const;
    bool IsDiscoveredAttack(const coord_t fr_coord, const coord_t to_coord,
                            attack_t mask);
    bool IsSliderAttack(const coord_t from_coord,
                        const coord_t to_coord) const;
    bool CheckBoardConsistency();
    void MoveToStr(const move_c m, const bool stm, char * const out);

    coord_t get_coord(const coord_t col, const coord_t row) const
    {
        return (board_width*row) + col;
    }

    coord_t get_coord(const char *str) const
    {
        return get_coord(str[0] - 'a', str[1] - '1');
    }

    coord_t get_col(const coord_t coord) const
    {
        assert((board_height & (board_height - 1)) == 0);
        return coord & (board_height - 1);
    }

    coord_t get_row(const coord_t coord) const
    {
        return coord/board_width;
    }

    coord_t get_type(const piece_t piece) const
    {
        return piece/sides;
    }

    bool col_within(const shifts_t col) const
    {
        return col >= 0 && col < board_width;
    }

    bool row_within(const shifts_t row) const
    {
        return row >= 0 && row < board_height;
    }

    bool get_color(const piece_t piece) const
    {
        assert(piece != empty_square);
        return piece & white;
    }

    bool get_color_and_not_empty(const piece_t piece) const
    {
        return (piece != empty_square) && (piece & white);
    }

    piece_t set_color(const piece_t piece, const bool stm) const
    {
        return (piece & ~white) | stm;
    }

    template <typename T> int sgn(T val) const
    {
        return (T(0) < val) - (val < T(0));
    }

    move_c MoveFromStr(const char *str)
    {
        move_c ans;
        const auto from_coord = get_coord(str);
        const auto index = find_piece(wtm, from_coord);
        if(index == -1U)
        {
            ans.flag = not_a_move;
            return ans;
        }
        ans.to_coord = get_coord(&str[2]);
        ans.piece_index = index;
        ans.flag = InitMoveFlag(ans, str[4]);
        return ans;
    }


private:


    void InitAttacks(bool stm);
    bool InitPieceLists();
    void ShowMove(const coord_t from_coord, const coord_t to_coord);
    void StoreCurrentBoardState(const move_c m, const coord_t from_coord);
    void MakeCapture(const move_c m);
    void MakePromotion(const move_c m);
    void TakebackCapture(const move_c m);
    void TakebackPromotion(move_c m);
    bool MakeCastleOrUpdateFlags(const move_c m, const coord_t from_coord);
    void TakebackCastle(const move_c m);
    bool MakeEnPassantOrUpdateFlags(const move_c m, const coord_t from_coord);
    void InitAttacksOnePiece(const coord_t coord, const bool setbit);
    void UpdateAttacks(move_c move, const coord_t from_coord);
    void UpdateAttacksOnePiece(const attack_params_s &p,
                               const bool setbit);
    char* ParseMainPartOfFen(char *ptr);
    char* ParseSideToMoveInFen(char *ptr);
    char* ParseCastlingRightsInFen(char *ptr);
    char* ParseEnPassantInFen(char *ptr);
    size_t test_count_attacked_squares(const bool stm);
    size_t test_count_all_attacks(const bool stm);
    void test_attack_tables(const size_t att_w, const size_t att_b);
    void InitAttacksPawn(const coord_t coord, const bool color, const u8 index,
                         const bool setbit);
    void InitAttacksNotPawn(const coord_t coord, const bool color,
                            const u8 index, const coord_t type,
                            const bool change_bit,
                            ray_mask_t ray_mask);
    void set_bit(const bool color, const coord_t col, const coord_t row,
                 const u8 index);
    void clear_bit(const bool color, const coord_t col, const coord_t row,
                   const u8 index);
    bool IsPseudoLegalPawn(const move_c move, const coord_t from_coord) const;
    bool IsPseudoLegalKing(const move_c move, const coord_t from_coord) const;
    bool IsPseudoLegalKnight(const move_c move, const coord_t from_coord) const;
    void InitSliderMask(bool stm);
    ray_mask_t GetRayMask(const attack_params_s &p, const bool setbit) const;
    size_t GetRayIndex(const coord_t from_coord, const coord_t to_coord,
                   coord_t *type) const;
    ray_mask_t GetRayMaskNotForMove(const coord_t target_coord,
                                    const coord_t piece_coord) const;
    void InitMobility(const bool color);
    size_t test_mobility(const bool color);
    void UpdateMasks(const move_c move, const attack_params_s &p);
    void GetAttackParams(const size_t index, const move_c move,
                         const bool stm, attack_params_s &p);
};

#include <iostream>
#include <cstring>
#include <cstdint>
#include <assert.h>
#include <vector>
#include <limits>
#include <cmath>
#include <list>
#include <algorithm>


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
    typedef u8 piece_type_t;
    typedef u8 coord_t;
    typedef i16 eval_t;
    typedef u8 move_flag_t;
    typedef u8 castle_t;
    typedef u8 enpass_t;
    typedef u32 attack_t;
    typedef i8 shifts_t;
    typedef u8 priority_t;
    typedef u8 ray_mask_t;
    typedef u8 piece_id_t;

    typedef std::list<std::pair<eval_t, piece_id_t>> k2list_t;

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
    const static coord_t piece_not_found = -1;
    coord_t max_ray_length;

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

    static const piece_type_t
    pawn = black_pawn/sides,
    bishop = black_bishop/sides,
    knight = black_knight/sides,
    rook = black_rook/sides,
    queen = black_queen/sides,
    king = black_king/sides;

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

    coord_t not_a_capture = -1;

    const char *start_position =
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

// Class representing move
    class move_c
    {
    public:
        coord_t from_coord;
        coord_t to_coord;
        move_flag_t flag;  // special move flags (is_capture, etc)
        priority_t priority;  // priority of move assigned by move generator

        bool operator == (move_c m) const
        {
            return to_coord == m.to_coord
                   && from_coord == m.from_coord
                   && flag == m.flag;
        }

        bool operator != (move_c m) const
        {
            return to_coord != m.to_coord
                   || from_coord != m.from_coord
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
        piece_id_t captured_id;  // id of captured piece
        coord_t next_piece_coord; // coord of piece after the captured
        castle_t castling_rights;  // castling rights, castle_kingside_w, ...
        piece_id_t castled_rook_id;  // id of castled rook
        enpass_t en_passant_rights;  // 0 = no en passant, 1..8 =
        // pawn col + 1, not null only if opponent pawn is near
        depth_t reversible_moves;  // reversible halfmove counter
        attack_t slider_mask;  // see slider_mask below
        bool attacks_updated;
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
        bool is_castle;
        bool is_en_passant;
        piece_id_t piece_id;
        piece_id_t castled_rook_id;
        piece_id_t captured_id;
        bool set_attack_bit;
        ray_mask_t ray_mask;
        ray_mask_t ray_id;
    };

    // side to move or white to move, k2chess::white (true) or k2chess::black
    bool wtm;

    // array representing the chess board
    piece_t b[board_width*board_height];

    // sorted by value list with piece id's
    k2list_t coord_id[sides];

    // list with delete peces id's (to avoid unwanted memory allocs)
    k2list_t deleted_id[sides];

    // array with piece coordinates sorted by their id's
    coord_t coords[sides][max_pieces_one_side];

    // array needed to fast find for piece id's by their coords
    piece_id_t find_piece_id[board_width*board_height];

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

    attack_t done_attacks[max_ply][sides][board_height*board_width];
    std::vector<k2list_t> store_coord_id;
    std::vector<std::vector<coord_t>> store_coords;
    coord_t done_mobility[max_ply][sides][max_pieces_one_side][max_rays];

    bool MakeMove(const move_c m);
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
    void MoveToCoordinateNotation(const move_c m, char * const out);
    void MakeAttacks(const move_c move);
    void TakebackMove(move_c move);
    bool PrintMoveSequence(const move_c * const moves, const size_t length,
                           const bool coordinate_notation);
    void MoveToAlgebraicNotation(const move_c move, char *out);
    void ProcessAmbiguousNotation(const move_c move, char *out);

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

    piece_type_t get_type(const piece_t piece) const
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
        const auto id = find_piece_id[from_coord];
        if(id == piece_not_found)
        {
            ans.flag = not_a_move;
            return ans;
        }
        ans.from_coord = from_coord;
        ans.to_coord = get_coord(&str[2]);
        ans.flag = InitMoveFlag(ans, str[4]);
        return ans;
    }

    void TakebackAttacks()
    {
        memcpy(attacks, done_attacks[ply], sizeof(attacks));
        memcpy(mobility, done_mobility[ply], sizeof(mobility));
    }

    coord_t king_coord(const bool stm) const
    {
        const auto id = coord_id[stm].back();
        const auto coord = coords[stm][id.second];
        return coord;
    }

    k2list_t::iterator find_piece_it(bool stm, coord_t coord)
    {
        auto it = coord_id[stm].begin();
        for(;it != coord_id[stm].end(); ++it)
        {
            const auto it_coord = coords[stm][(*it).second];
            if(it_coord == coord)
                break;
        }
        return it;
    }


private:


    void InitAttacks(const bool stm);
    bool InitPieceLists(bool stm);
    void ShowMove(const move_c move);
    void StoreCurrentBoardState(const move_c m);
    void MakeCapture(const move_c m);
    void MakePromotion(const move_c m);
    void TakebackCapture(const move_c m);
    void TakebackPromotion(move_c m);
    bool MakeCastleOrUpdateFlags(const move_c m);
    void TakebackCastle(const move_c m);
    bool MakeEnPassantOrUpdateFlags(const move_c m);
    void InitAttacksOnePiece(const coord_t coord, const bool setbit);
    void UpdateAttacks(move_c move);
    void UpdateAttacksOnePiece(attack_params_s &p);
    char* ParseMainPartOfFen(char *ptr);
    char* ParseSideToMoveInFen(char *ptr);
    char* ParseCastlingRightsInFen(char *ptr);
    char* ParseEnPassantInFen(char *ptr);
    size_t test_count_attacked_squares(const bool stm);
    size_t test_count_all_attacks(const bool stm);
    void test_attack_tables(const size_t att_w, const size_t att_b);
    void InitAttacksPawn(const coord_t coord, const bool color,
                         const piece_id_t piece_id, const bool setbit);
    void InitAttacksNotPawn(const coord_t coord, const bool color,
                            const piece_id_t piece_id, const coord_t type,
                            const bool change_bit,
                            ray_mask_t ray_mask);
    bool IsPseudoLegalPawn(const move_c move) const;
    bool IsPseudoLegalKing(const move_c move) const;
    bool IsPseudoLegalKnight(const move_c move) const;
    void InitSliderMask(bool stm);
    ray_mask_t GetRayMask(attack_params_s &p) const;
    piece_id_t GetRayId(const coord_t from_coord, const coord_t to_coord,
                   coord_t *type) const;
    ray_mask_t GetRayMaskNotForMove(const coord_t target_coord,
                                    const coord_t piece_coord) const;
    void InitMobility(const bool color);
    size_t test_mobility(const bool stm);
    void UpdateMasks(const move_c move, const attack_params_s &p);
    void GetAttackParams(const piece_id_t piece_id, const move_c move,
                         const bool stm, attack_params_s &p);
    void InitAttacksSlider(coord_t coord, attack_params_s &p);
    void InitMobilitySlider(attack_params_s &p);
    ray_mask_t GetNonSliderMask(const attack_params_s &p) const;
    ray_mask_t GetKnightMask(const coord_t piece_coord,
                              const coord_t to_coord) const;
    ray_mask_t GetKingMask(const coord_t piece_coord,
                              const coord_t to_coord) const;
    bool IsLegalKingMove(const move_c move);

    void set_bit(const bool color, const coord_t col, const coord_t row,
                 const u8 index)
    {
        attacks[color][get_coord(col, row)] |= (1 << index);
    }

    void clear_bit(const bool color, const coord_t col, const coord_t row,
                   const u8 index)
    {
        attacks[color][get_coord(col, row)] &= ~(1 << index);
    }
};

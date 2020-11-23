#include <iostream>
#include <cstring>
#include <cstdint>
#include <assert.h>
#include <vector>
#include <limits.h>
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
    typedef i16 piece_val_t;
    typedef u8 move_flag_t;
    typedef u8 castle_t;
    typedef u8 enpass_t;
    typedef u32 attack_t;
    typedef i8 shifts_t;
    typedef u8 priority_t;
    typedef u16 ray_mask_t;
    typedef u8 piece_id_t;
    typedef u8 tiny_count_t;

    const static depth_t max_ply = 100;  // maximum search depth
    const static coord_t board_width = 8;
    const static coord_t board_height = 8;
    const static coord_t board_size = board_height*board_width;
    const static tiny_count_t sides = 2;  // black and white
    const static tiny_count_t piece_types = 6;  // pawns, knights, ...
    const static bool white = true;
    const static bool black = false;
    const static tiny_count_t prev_states = 4;
    const static coord_t max_col = board_width - 1;
    const static coord_t max_row = board_height - 1;
    const static tiny_count_t max_pieces_one_side = 16;
    const static tiny_count_t max_rays = 16;
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
    white_pawn = 13;

    const move_flag_t
    is_capture = 0x10,
    is_right_castle = 0x20,
    is_left_castle = 0x40,
    is_castle = 0x60,
    is_en_passant = 0x80,
    is_promotion_to_queen = 0x01,
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
    static const tiny_count_t move_max_display_length = 5;
    static const attack_t attack_digits = sizeof(attack_t)*CHAR_BIT;

    static const piece_type_t
    pawn = black_pawn/sides,
    bishop = black_bishop/sides,
    knight = black_knight/sides,
    rook = black_rook/sides,
    queen = black_queen/sides,
    king = black_king/sides;

    const ray_mask_t
    pawn_mask_white = 0x90,
    pawn_mask_black = 0x60;

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
            return priority > m.priority;
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
        attack_t exist_mask[sides];  // see exist_mask below
        bool attacks_updated;
    };

    // side to move or white to move, k2chess::white (true) or k2chess::black
    bool wtm;

    // array representing the chess board
    piece_t b[board_size];

    // array with piece coordinates sorted by their id's
    coord_t coords[sides][max_pieces_one_side];

    // array needed to fast find for piece id's by their coords
    piece_id_t find_piece_id[board_size];

    // array needed to fast find for ray mask by two coord
    ray_mask_t find_ray_mask[4*board_size];

    // two tables for each color with attacks of all pieces
    attack_t attacks[sides][board_size];

    // bit masks for pieces which exists on the board
    attack_t exist_mask[sides];

    // bit masks for each type of pieces
    attack_t type_mask[sides][piece_types + 1];

    // bit masks for fast detection of attacking sliders
    attack_t slider_mask[sides];

    // bit masks for update attack tables
    attack_t update_mask[sides];

    // counters of attacks for each ray of each piece
    tiny_count_t directions[sides][max_pieces_one_side][max_rays];

    // counters of attacks for each piece
    tiny_count_t sum_directions[sides][max_pieces_one_side];

    // ray masks for all types of pieces
    ray_mask_t ray_mask_all[piece_types + 1];

    // biases defining directions for 'rays'
    shifts_t delta_col[max_rays];
    shifts_t delta_row[max_rays];

    // for quick detection if piece is a slider
    bool is_slider[piece_types + 1];

    // piece values for material counters,
    // move priorities and sorting piece lists
    piece_val_t values[piece_types + 1];

    piece_val_t material[sides];  // material counters
    tiny_count_t pieces[sides];  // piece counters, including kings
    tiny_count_t quantity[sides][piece_types + 1];

    state_s b_state[prev_states + max_ply]; // board state for each ply depth
    state_s *state;  // pointer to board state, state[0] = b_state[prev_states];
    tiny_count_t ply;  // current ply depth
    tiny_count_t reversible_moves;

    // current variation (for debug mode only)
    char cur_moves[move_max_display_length*max_ply];
    char *cv;  // current variation pointer (for debug mode only)

    attack_t done_attacks[max_ply][sides][board_size];
    std::vector<std::vector<coord_t>> store_coords;
    std::vector<std::vector<attack_t>> store_type_mask;
    tiny_count_t done_directions[max_ply][sides][max_pieces_one_side][max_rays];
    tiny_count_t done_sum_directions[max_ply][sides][max_pieces_one_side];

    bool MakeMove(const move_c m);
    move_flag_t InitMoveFlag(const move_c move, const char promo_to) const;
    bool IsLegal(const move_c move) const;
    bool IsPseudoLegal(const move_c move) const;
    bool IsOnRay(const coord_t given, const coord_t ray_coord1,
                 const coord_t ray_coord2) const;
    bool IsSliderAttack(const coord_t from_coord,
                        const coord_t to_coord) const;
    void MoveToCoordinateNotation(const move_c m, char * const out) const;
    void UpdateAttackTables(move_c move);
    void TakebackMove(move_c move);
    bool PrintMoveSequence(const move_c * const moves, const size_t length,
                           const bool coordinate_notation);
    void MoveToAlgebraicNotation(const move_c move, char *out) const;
    char *ProcessAmbiguousNotation(const move_c move, char *out) const;
    bool IsDiscoveredAttack(const move_c move) const;
    bool IsDiscoveredEnPassant(const bool stm, const move_c move,
                               const depth_t _ply_) const;
    bool InitPieceLists(bool stm);

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
        return (piece & ~static_cast<piece_t>(white)) | stm;
    }

    bool is_light(const piece_t piece, const bool stm) const
    {
        return piece != empty_square && (piece & white) == stm;
    }

    bool is_dark(const piece_t piece, const bool stm) const
    {
        return piece != empty_square && (piece & white) != stm;
    }

    int lower_bit_num(const unsigned mask) const
    {
        assert(mask != 0);
        return __builtin_ctz(mask);
    }

    int higher_bit_num(const unsigned mask) const
    {
        assert(mask != 0);
        return __builtin_clz(mask) ^ 31;
    }

    template <typename T> int sgn(const T val) const
    {
        return (T(0) < val) - (val < T(0));
    }

    move_c MoveFromStr(const char *str) const
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
        memcpy(directions, done_directions[ply],
               sizeof(directions));
        memcpy(sum_directions, done_sum_directions[ply],
               sizeof(sum_directions));
    }

    coord_t king_coord(const bool stm) const
    {
        return coords[stm][higher_bit_num(exist_mask[stm])];
    }


private:


    void ShowMove(const move_c move);
    void StoreCurrentBoardState(const move_c m);
    void MakeCapture(const move_c m);
    void MakePromotion(const move_c m);
    void TakebackCapture(const move_c m);
    void TakebackPromotion(move_c m);
    bool MakeCastleOrUpdateFlags(const move_c m);
    void TakebackCastle(const move_c m);
    bool MakeEnPassantOrUpdateFlags(const move_c m);
    void UpdateAttacks(const bool stm, const move_c move);
    void UpdateRayAttacks(const bool stm, const piece_id_t piece_id,
                          const ray_mask_t ray_id, const coord_t old_cr,
                          const coord_t new_cr);
    char* ParseMainPartOfFen(char *ptr);
    char* ParseSideToMoveInFen(char *ptr);
    char* ParseCastlingRightsInFen(char *ptr);
    char* ParseEnPassantInFen(char *ptr);
    size_t test_attack_tables(const bool stm) const;
    bool IsPseudoLegalPawn(const move_c move) const;
    bool IsPseudoLegalKing(const move_c move) const;
    bool IsPseudoLegalKnight(const move_c move) const;
    bool IsLegalKingMove(const move_c move) const;
    bool IsLegalCastle(const move_c move) const;
    bool IsOnRayMask(const bool stm, const coord_t piece_coord,
                     const coord_t move_coord, const piece_id_t piece_id,
                     const ray_mask_t ray_id, const bool enps) const;
    void UpdatePieceMasks(move_c move);
    void UpdateDirections(const bool stm, const move_c move);
    void UpdatePieceDirections(const bool stm,
                                      const piece_id_t piece_id,
                                      const move_c move);
    void UpdateFromRayDirections(const bool stm, const piece_id_t piece_id,
                               const coord_t coord,
                               const piece_id_t ray_id);
    void UpdateToRayDirections(const bool stm, const piece_id_t piece_id,
                             const coord_t coord, const move_c move,
                             const piece_id_t ray_id);
    coord_t GetRayAttacks(const coord_t coord,
                           piece_id_t ray_id, const bool slider) const;
    void UpdatePieceMasksForCastle(move_c move);
    void InitMasks(bool stm);
    void InitAttacks(bool stm);
    void InitDirections(const bool stm);
    void ClearPieceAttacks(const bool stm, piece_id_t piece_id);
    void UpdateMovingDirections(const bool stm, const piece_id_t piece_id,
                                   const piece_type_t type,
                                   const coord_t coord, ray_mask_t rmask);
    void UpdateOrdinaryDirections(const bool stm,
                                     const piece_id_t piece_id,
                                     const coord_t coord, const move_c move,
                                     ray_mask_t rmask);
    bool UpdateCapturingDirections(const bool stm,
                                      const piece_id_t piece_id,
                                      const coord_t coord,
                                      const piece_type_t type,
                                      const move_c move);
    void UpdatePieceAttacks(const bool stm, const piece_id_t piece_id,
                            const move_c move);
    bool IsCastledRook(const bool stm,
                       const coord_t coord, const move_c move) const;
    bool IsSameRay(const coord_t given_coord, const coord_t beg_coord,
                   const coord_t end_coord) const;
    void InitPossibleAttacksArray();

    void set_bit(const bool color, const coord_t col, const coord_t row,
                 const piece_id_t piece_id)
    {
        assert(col_within(col) && row_within(row));
        assert(color == black || color == white);
        attacks[color][get_coord(col, row)] |= (1 << piece_id);
    }

    void clear_bit(const bool color, const coord_t col, const coord_t row,
                   const piece_id_t piece_id)
    {
        assert(col_within(col) && row_within(row));
        assert(color == black || color == white);
        attacks[color][get_coord(col, row)] &= ~(1 << piece_id);
    }

    ray_mask_t GetRayMask(const bool stm, const coord_t coord,
                          const move_c move) const
    {
        const auto st = &k2chess::state[ply];
        const bool captured = (coord == move.to_coord) && stm == wtm;
        auto type = get_type(!captured ? b[coord] : st->captured_piece);
        if(!type)  // en passant
            type = pawn;
        auto rmask = ray_mask_all[type];
        if(type == pawn)
            rmask &= (stm ? pawn_mask_white : pawn_mask_black);
        return rmask;
    }

    void InitQuantity(const bool stm)
    {
        auto mask = exist_mask[stm];
        while(mask)
        {
            const auto piece_id = lower_bit_num(mask);
            mask ^= (1 << piece_id);
            quantity[stm][get_type(b[coords[stm][piece_id]])]++;
        }
    }

    coord_t pseudocoord(const coord_t coord) const
    {
        return 2*board_width*get_row(coord) + get_col(coord);
    }

    ray_mask_t GetRayMask(const coord_t beg_coord,
                        const coord_t end_coord) const
    {
        const auto beg = pseudocoord(beg_coord);
        const auto end = pseudocoord(end_coord);
        return find_ray_mask[2*board_size + end - beg];
    }
};

#include "chess.h"


//--------------------------------
class k2eval : public k2chess
{


public:

    void RunUnitTests();

    k2eval() : material_values {{0, 0}, king_val, queen_val, rook_val,
                bishop_val, knight_val, pawn_val},
    pst
{
{ //king
{{-177,-121},{8,-64},{135,-63},{73,-55},{-64,-16},{16,13},{65,-36},{21,-50}},
{{108,-50},{106,-21},{51,-6},{128,1},{13,18},{86,19},{63,-12},{21,-18}},
{{95,-26},{84,-6},{58,3},{-35,21},{-41,25},{161,26},{172,16},{4,-9}},
{{10,-31},{60,1},{-8,20},{-45,30},{-93,39},{-76,37},{77,12},{-44,-6}},
{{-106,-19},{57,-17},{-20,19},{-65,29},{-110,40},{-7,19},{7,2},{-83,-10}},
{{-11,-25},{79,-17},{17,6},{5,14},{5,18},{15,13},{31,3},{-15,-18}},
{{70,-58},{81,-30},{-7,4},{-31,6},{0,8},{-1,4},{63,-15},{40,-31}},
{{-21,-80},{57,-51},{26,-32},{-47,-14},{61,-47},{-42,-13},{54,-43},{28,-70}},
},

{ // queen
{{-45,-14},{-40,27},{-14,7},{-39,16},{26,0},{23,-15},{9,-20},{56,-8}},
{{-43,-5},{-77,19},{-43,37},{27,13},{-82,45},{-14,9},{-11,-2},{61,-34}},
{{-21,-1},{-20,-5},{-14,12},{-43,51},{-8,47},{-1,11},{-30,19},{-4,-2}},
{{-32,20},{-45,37},{-44,34},{-63,72},{-25,53},{-13,26},{-25,45},{-18,19}},
{{-4,6},{-55,53},{-10,23},{-33,67},{-3,29},{-1,23},{-4,28},{-4,13}},
{{-30,33},{18,-18},{-6,20},{15,-7},{-4,31},{7,10},{25,2},{11,16}},
{{-14,-5},{5,-13},{30,-15},{25,-3},{36,-10},{39,-24},{16,-35},{38,-15}},
{{19,-12},{21,-32},{26,-12},{45,-27},{6,13},{-2,-20},{-13,-12},{-41,-23}},
},

{ // rook
{{-10,28},{64,8},{-27,32},{8,23},{59,14},{-42,26},{-43,27},{-5,20}},
{{0,23},{3,24},{39,18},{85,3},{61,-2},{49,11},{-33,27},{9,18}},
{{-11,18},{11,15},{6,13},{-1,17},{-18,12},{-17,9},{54,0},{-11,5}},
{{-39,17},{-28,12},{3,18},{7,9},{5,7},{6,11},{-46,9},{-24,15}},
{{-35,13},{-56,21},{-36,22},{-20,11},{9,-4},{-29,3},{-9,0},{-16,0}},
{{-39,9},{-20,6},{-24,2},{-41,13},{2,-4},{4,-6},{-5,-1},{-17,-11}},
{{-29,4},{-13,1},{-31,8},{-12,8},{-2,-4},{9,-6},{-13,-8},{-58,5}},
{{10,-1},{8,4},{9,10},{25,2},{28,-2},{31,-6},{-21,1},{19,-25}},
},

{ // bishop
{{-78,5},{-61,0},{-183,16},{-159,18},{-77,11},{-108,8},{-45,-5},{1,-19}},
{{-69,15},{-11,5},{-55,12},{-92,6},{4,4},{21,-1},{0,-3},{-100,5}},
{{-57,22},{17,-8},{10,4},{18,-4},{-9,8},{11,5},{-17,6},{-35,17}},
{{-17,6},{-10,4},{-16,12},{29,14},{14,13},{7,5},{-4,-3},{-28,11}},
{{-15,5},{-19,8},{-9,12},{9,20},{17,7},{-5,5},{-10,-1},{8,-3}},
{{-7,1},{2,4},{1,9},{-1,8},{0,12},{13,6},{13,-2},{5,1}},
{{13,-9},{23,-13},{6,-5},{-7,2},{3,4},{23,-7},{54,-17},{12,-26}},
{{-31,-8},{-3,-2},{0,-6},{-6,2},{3,-3},{0,-14},{-47,7},{-26,1}},

},

{ // knight
{{-183,-66},{-165,-14},{-26,12},{-72,1},{58,-7},{-169,0},{-85,-30},{-177,-78}},
{{-150,5},{-81,23},{98,-3},{19,38},{-13,26},{83,-6},{3,-3},{-51,-31}},
{{-95,7},{72,2},{54,39},{64,46},{88,30},{156,7},{46,8},{13,-17}},
{{-13,9},{29,29},{14,62},{71,59},{19,66},{88,40},{-4,44},{28,3}},
{{-12,10},{24,16},{27,51},{9,69},{41,54},{28,51},{31,36},{-12,14}},
{{-24,4},{-7,31},{28,29},{26,48},{52,41},{26,41},{41,9},{-17,0}},
{{-22,-21},{-51,3},{-5,19},{17,23},{15,28},{33,8},{-6,-1},{-8,-29}},
{{-106,-20},{-3,-37},{-44,0},{-36,12},{15,-8},{-13,0},{-11,-14},{-16,-57}},
},

{ // pawn
{{0,0},    {0,0},    {0,0},    {0,0},   {0,0},   {0,0},    {0,0},   {0,0},},
{{119,75},{126,52},{89,32},{171,1},{124,36},{186,13},{52,58},{64,70}},
{{43,34},{-5,39},{26,15},{19,-6},{63,-11},{121,-4},{31,24},{-5,38}},
{{-10,14},{3,2},{9,-10},{36,-26},{47,-20},{35,-14},{-16,10},{-28,6}},
{{-25,3},{-29,3},{2,-13},{24,-20},{25,-14},{4,-14},{-25,-4},{-33,-10}},
{{-15,-12},{-33,-7},{-8,-13},{-12,-8},{3,-6},{-15,-4},{-5,-16},{-7,-23}},
{{-26,0},{-24,-6},{-38,7},{-26,2},{-24,17},{19,-1},{8,-11},{-21,-20}},
{{0,0},    {0,0},    {0,0},    {0,0},   {0,0},   {0,0},    {0,0},   {0,0},},
},
}

    {
        InitPawnStruct();
    }


protected:


    typedef u8 rank_t;
    typedef u8 dist_t;

    template <class T>
    class vec2
    {
    public:

        T mid, end;

        vec2() : mid(0), end(0) {}
        vec2(const T m, const T e) : mid(m), end(e) {}
        vec2(const vec2& v) : mid(v.mid), end(v.end) {}
        vec2& operator =(const vec2& v) {mid=v.mid; end=v.end; return *this;}
        bool operator ==(const vec2& v) const {return mid==v.mid && end==v.end;}
        bool operator !=(const vec2& v) const {return mid!=v.mid || end!=v.end;}
        vec2 operator +(const vec2& v) const {return vec2(mid+v.mid,end+v.end);}
        vec2 operator -(const vec2& v) const {return vec2(mid-v.mid,end-v.end);}
        vec2 operator -() const {return vec2(-mid, -end);}
        vec2 operator *(const vec2& v) const {return vec2(mid*v.mid,end*v.end);}
        vec2 operator /(const vec2& v) const {return vec2(mid/v.mid,end/v.end);}
        vec2 operator *(const T x) const {return vec2(mid*x, end*x);}
        vec2 operator /(const T x) const {return vec2(mid / x, end / x);}
        friend vec2 operator *(const T x, const vec2& v)
        {
            return vec2(v.mid*x, v.end*x);
        }
        void operator +=(const vec2 v) {mid += v.mid; end += v.end;}
        void operator -=(const vec2 v) {mid -= v.mid; end -= v.end;}
    };

    const eval_t
    centipawn = 100,
    infinite_score = 32760,
    king_value = 32000,
    material_total = 80,
    rook_max_pawns_for_open_file = 2;

    vec2<eval_t>
    pawn_val = {100, 128},
    knight_val = {450, 369},
    bishop_val = {467, 387},
    rook_val = {556, 680},
    queen_val = {1170, 1290},
    king_val = {0, 0},

    pawn_dbl_iso = {62, 44},
    pawn_iso = {30, 12},
    pawn_dbl = {-9, 11},
    pawn_hole = {24, 27},
    pawn_gap = {7, 14},
    pawn_king_tropism1 = {-4, -21},
    pawn_king_tropism2 = {-2, 28},
    pawn_king_tropism3 = {-9, 37},
    pawn_pass0 = {-1, -6},
    pawn_pass1 = {-1, -10},
    pawn_pass2 = {1, 9},
    pawn_blk_pass0 = {-10, -100},
    pawn_blk_pass1 = {6, 56},
    pawn_blk_pass2 = {0, -4},
    pawn_pass_connected = {0, 16},
    pawn_unstoppable = {12, 242},
    king_saf_no_shelter = {22, 0},
    king_saf_no_queen = {20, 20},
    king_saf_attack1 = {20, 0},
    king_saf_attack2 = {88, 0},
    king_saf_central_files = {63, 0},
    rook_last_rank = {-17, 10},
    rook_semi_open_file = {21, 10},
    rook_open_file = {71, 0},
    bishop_pair = {17, 72},
    mob_queen = {-6, 38},
    mob_rook = {6, 17},
    mob_bishop = {9, 14},
    mob_knight = {0, 0},
    imbalance_king_in_corner = {0, 200},
    imbalance_draw_divider = {32, 32},
    imbalance_multicolor = {30, 57},
    imbalance_no_pawns = {24, 24},
    side_to_move_bonus = {8, 8};

    eval_t initial_score;
    rank_t p_max[board_width + 2][sides], p_min[board_width + 2][sides];
    rank_t (*pawn_max)[sides], (*pawn_min)[sides];
    vec2<eval_t> material_values[piece_types + 1];

    vec2<eval_t> pst[piece_types][board_height][board_width];


public:


    void EvalDebug(const bool stm);

    vec2<eval_t> SetupPosition(const char *p)
    {
        k2chess::SetupPosition(p);
        InitPawnStruct();;
        return InitEvalOfMaterial() + InitEvalOfPST();
    }


protected:


    vec2<eval_t> FastEval(const bool stm, const move_c m) const;
    eval_t Eval(const bool stm, vec2<eval_t> cur_eval);
    vec2<eval_t> InitEvalOfMaterial();
    vec2<eval_t> InitEvalOfPST();
    void InitPawnStruct();
    void SetPawnStruct(const bool side, const coord_t col);
    bool IsPasser(const coord_t col, const bool stm) const;
    bool MakeMove(const move_c move);
    void TakebackMove(const move_c move);

    eval_t GetEvalScore(const bool stm, const vec2<eval_t> val) const
    {
        i32 X, Y;
        X = material[0]/centipawn + 1 + material[1]/centipawn +
                1 - pieces[0] - pieces[1];
        Y = ((val.mid - val.end)*X + material_total*val.end)/material_total;
        eval_t ans = Y;
        return stm ? ans : -ans;
    }

    dist_t king_dist(const coord_t from_coord, const coord_t to_coord) const
    {
        return std::max(std::abs(get_col(from_coord) - get_col(to_coord)),
                        std::abs(get_row(from_coord) - get_row(to_coord)));
    }

    bool is_same_color(const coord_t coord1, const coord_t coord2) const
    {
        const auto sum_coord1 = get_col(coord1) + get_row(coord1);
        const auto sum_coord2 = get_col(coord2) + get_row(coord2);
        return (sum_coord1 & 1) == (sum_coord2 & 1);
    }

    bool is_king_near_corner(const bool stm) const
    {
        const auto k = king_coord(stm);
        const auto min_dist1 = std::min(king_dist(k, get_coord(0, 0)),
                                        king_dist(k, get_coord(0, max_row)));
        const auto min_dist2 = std::min(king_dist(k, get_coord(max_col, 0)),
                                        king_dist(k, get_coord(max_col,
                                                               max_row)));
        return std::min(min_dist1, min_dist2) <= 1 &&
                (get_col(k) == 0 || get_row(k) == 0 ||
                 get_col(k) == max_col || get_row(k) == max_row);
    }


    vec2<eval_t> EvalSideToMove(const bool stm)
    {
        return stm ? side_to_move_bonus : -side_to_move_bonus;
    }


private:


    vec2<eval_t> EvalPawns(const bool side, const bool stm);
    bool IsUnstoppablePawn(const coord_t col, const bool side,
                           const bool stm) const;
    vec2<eval_t> EvalImbalances(const bool stm, vec2<eval_t> val);
    void MovePawnStruct(const bool side, const move_c move,
                        const piece_t moved_piece,
                        const piece_t captured_piece);
    vec2<eval_t> EvalMobility(bool stm);
    vec2<eval_t> EvalKingSafety(const bool king_color);
    bool KingHasNoShelter(coord_t k_col, coord_t k_row, const bool stm) const;
    bool Sheltered(const coord_t k_col, coord_t k_row, const bool stm) const;
    attack_t KingSafetyBatteries(const coord_t targ_coord, const attack_t att,
                                 const bool stm) const;
    vec2<eval_t> EvalRooks(const bool stm);
    size_t CountAttacksOnKing(const bool stm, const coord_t k_col,
                              const coord_t k_row) const;
    void DbgOut(const char *str, vec2<eval_t> val, vec2<eval_t> &sum) const;
};

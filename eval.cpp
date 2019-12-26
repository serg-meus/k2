#include "eval.h"





//-----------------------------
k2chess::eval_t k2eval::Eval(const bool stm, vec2<eval_t> cur_eval)
{
    for(auto side: {black, white})
    {
        cur_eval += EvalPawns(side, stm);
        cur_eval += EvalRooks(side);
        cur_eval += EvalKingSafety(side);
        cur_eval += EvalMobility(side);
    }
    cur_eval += EvalSideToMove(stm);
    cur_eval = EvalImbalances(stm, cur_eval);

    return GetEvalScore(stm, cur_eval);
}





//-----------------------------
k2eval::vec2<k2chess::eval_t> k2eval::FastEval(const bool stm,
                                               const move_c move) const
{
    vec2<eval_t> ans = {0, 0};

    auto x = get_col(move.to_coord);
    auto y = get_row(move.to_coord);
    auto x0 = get_col(move.from_coord);
    auto y0 = get_row(move.from_coord);
    auto to_type = get_type(b[move.to_coord]);
    if(!stm)
    {
        y = max_row - y;
        y0 = max_row - y0;
    }

    if(move.flag & is_promotion)
    {
        ans -= material_values[pawn] + pst[pawn - 1][y0][x0];

        piece_t prom_pc[] =
        {
            black_queen, black_knight, black_rook, black_bishop
        };
        const auto type = get_type(prom_pc[(move.flag & is_promotion) - 1]);
        ans += material_values[type] + pst[type - 1][y0][x0];
    }

    if(move.flag & is_capture)
    {
        auto captured_piece = k2chess::state[ply].captured_piece;
        if(move.flag & is_en_passant)
            ans += material_values[pawn] + pst[pawn - 1][max_row - y0][x];
        else
        {
            const auto type = get_type(captured_piece);
            ans += material_values[type] + pst[type - 1][max_row - y][x];
        }
    }
    else if(move.flag & is_right_castle)
    {
        const auto r_col = default_king_col + cstl_move_length - 1;
        ans += pst[rook - 1][max_row][r_col] - pst[rook - 1][max_row][max_col];
    }
    else if(move.flag & is_left_castle)
    {
        auto r_col = default_king_col - cstl_move_length + 1;
        ans += pst[rook - 1][max_row][r_col] - pst[rook - 1][max_row][0];
    }
    ans += pst[to_type - 1][y][x] - pst[to_type - 1][y0][x0];

    return stm ? -ans : ans;
}





//-----------------------------
k2eval::vec2<k2chess::eval_t> k2eval::InitEvalOfMaterial()
{
    vec2<eval_t> ans = {0, 0};
    for(auto col = 0; col <= max_col; ++col)
        for(auto row = 0; row <= max_row; ++row)
        {
            auto piece = b[get_coord(col, row)];
            if(piece == empty_square)
                continue;
            auto delta = material_values[get_type(piece)];
            ans += (piece & white) ? delta : -delta;
    }
    return ans;
}





//-----------------------------
k2eval::vec2<k2chess::eval_t> k2eval::InitEvalOfPST()
{
    vec2<eval_t> ans = {0, 0};
    for(auto col = 0; col <= max_col; ++col)
        for(auto row = 0; row <= max_row; ++row)
        {
            auto piece = b[get_coord(col, row)];
            if(piece == empty_square)
                continue;
            auto row_ = row;
            if(piece & white)
                row_ = max_row - row;

            auto type = get_type(piece);
            auto delta = pst[type - 1][row_][col];
            ans += (piece & white) ? delta : -delta;
    }
    return ans;
}





//--------------------------------
bool k2eval::IsPasser(const coord_t col, const bool stm) const
{
    auto mx = pawn_max[col][stm];

    return (mx >= max_row - pawn_min[col][!stm] &&
            mx >= max_row - pawn_min[col - 1][!stm] &&
            mx >= max_row - pawn_min[col + 1][!stm]);
}





//--------------------------------
k2eval::vec2<k2chess::eval_t> k2eval::EvalPawns(const bool side, const bool stm)
{
    vec2<eval_t> ans = {0, 0};
    bool passer, prev_passer = false;
    bool opp_only_pawns = material[!side]/centipawn == pieces[!side] - 1;

    for(auto col = 0; col <= max_col; col++)
    {
        bool doubled = false, isolany = false;

        const eval_t mx = pawn_max[col][side];
        if(mx == 0)
        {
            prev_passer = false;
            continue;
        }

        if(pawn_min[col][side] != mx)
            doubled = true;
        if(pawn_max[col - 1][side] == 0 && pawn_max[col + 1][side] == 0)
            isolany = true;
        if(doubled && isolany)
            ans -= pawn_dbl_iso;
        else if(isolany)
            ans -= pawn_iso;
        else if(doubled)
            ans -= pawn_dbl;

        passer = IsPasser(col, side);
        if(!passer)
        {
            // pawn holes occupied by enemy pieces
            if(col > 0 && col < max_col && mx < pawn_min[col - 1][side]
                    && mx < pawn_min[col + 1][side])
            {
                auto y_coord = side ? mx + 1 : max_row - mx - 1;
                auto op_piece = b[get_coord(col, y_coord)];
                bool occupied = is_dark(op_piece, side)
                                && get_type(op_piece) != pawn;
                if(occupied)
                    ans -= pawn_hole;
            }
            // gaps in pawn structure
            if(pawn_max[col - 1][side]
                    && std::abs(mx - pawn_max[col - 1][side]) > 1)
                ans -= pawn_gap;

            prev_passer = false;
            continue;
        }
        // following code executed only for passers

        // king pawn tropism
        const auto k_coord = king_coord(side);
        const auto opp_k_coord = king_coord(!side);
        const auto stop_coord = get_coord(col, side ? mx + 1 : max_row - mx-1);
        const auto k_dist = king_dist(k_coord, stop_coord);
        const auto opp_k_dist = king_dist(opp_k_coord, stop_coord);

        if(k_dist <= 1)
            ans += pawn_king_tropism1 + mx*pawn_king_tropism2;
        else if(k_dist == 2)
            ans += pawn_king_tropism3;
        if(opp_k_dist <= 1)
            ans -= pawn_king_tropism1 + mx*pawn_king_tropism2;
        else if(opp_k_dist == 2)
            ans -= pawn_king_tropism3;

        // passed pawn evaluation
        const bool blocked = b[stop_coord] != empty_square;
        const eval_t mx2 = mx*mx;
        if(blocked)
            ans += mx2*pawn_blk_pass2 + mx*pawn_blk_pass1 + pawn_blk_pass0;
        else
            ans += mx2*pawn_pass2 + mx*pawn_pass1 + pawn_pass0;

        // connected passers
        if(passer && prev_passer
                && std::abs(mx - pawn_max[col - 1][side]) <= 1)
        {
            const eval_t mmx = std::max(
                        mx, static_cast<eval_t>(pawn_max[col - 1][side]));
            if(mmx > 4)
                ans += mmx*pawn_pass_connected;
        }
        prev_passer = true;

        // unstoppable
        if(opp_only_pawns && IsUnstoppablePawn(col, side, stm))
        {
            ans.mid += pawn_unstoppable.end + mx*pawn_unstoppable.mid;
            ans.end += pawn_unstoppable.end + mx*pawn_unstoppable.mid;
        }
    }
    return side ? ans : -ans;
}





//-----------------------------
bool k2eval::IsUnstoppablePawn(const coord_t col, const bool side,
                               const bool stm) const
{
    auto pmax = pawn_max[col][side];
    if(pmax == pawn_default_row)
        pmax++;
    const auto promo_square = get_coord(col, side ? max_row : 0);
    const int dist = king_dist(king_coord(!side), promo_square);
    const auto k_coord = king_coord(side);
    if(get_col(k_coord) == col &&
            king_dist(k_coord, promo_square) <= max_row - pmax)
        pmax--;
    return dist - (side != stm) > max_row - pmax;
}





//-----------------------------
k2eval::vec2<k2chess::eval_t> k2eval::EvalMobility(bool stm)
{
    vec2<eval_t> f_type[] = {{0, 0}, {0, 0}, mob_queen, mob_rook, mob_bishop,
                          mob_knight};
    eval_t f_num[] = {-15, -15, -15, -10, -5, 5, 10, 15, 18, 20,
                      21, 21, 21, 22, 22};

    vec2<eval_t> ans = {0, 0};
    auto mask = exist_mask[stm] & slider_mask[stm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        const auto coord = coords[stm][piece_id];
        const auto type = get_type(b[coord]);
        int cr = sum_directions[stm][piece_id];
        if(type == queen)
            cr /= 2;
        assert(cr < 15);
        ans += f_num[cr]*f_type[type];
    }
    const eval_t mob_devider = 8;
    return stm ? ans/mob_devider : -ans/mob_devider;
}





//-----------------------------
k2eval::vec2<k2chess::eval_t> k2eval::EvalImbalances(const bool stm,
                                                     vec2<eval_t> val)
{
    const auto X = material[black]/centipawn + 1 + material[white]/centipawn
            + 1 - pieces[black] - pieces[white];

    if(X == 3 && (material[black]/centipawn == 4 ||
                  material[white]/centipawn == 4))
    {
        // KNk, KBk, Kkn, Kkb
        if(pieces[black] + pieces[white] == 3)
            return {0, 0};
        // KPkn, KPkb
        else if((material[white]/centipawn == 1 &&
                 material[black]/centipawn == 4) ||
                (material[black]/centipawn == 1 &&
                 material[white]/centipawn == 4))
            return val / imbalance_draw_divider;
    }
    // KNNk, KBNk, KBBk, etc
    else if(X == 6 && (material[black] == 0 || material[white] == 0))
    {
        if(quantity[white][pawn] || quantity[black][pawn])
            return val;
        if(quantity[white][knight] == 2
                || quantity[black][knight] == 2)
            return {0, 0};
        // many code for mating with only bishop and knight
        else if((quantity[white][knight] == 1 &&
                 quantity[white][bishop] == 1) ||
                (quantity[black][knight] == 1 &&
                 quantity[black][bishop] == 1))
        {
            const auto stm = quantity[black][knight] == 1 ? black : white;
            const auto bishop_mask = exist_mask[stm] & type_mask[stm][bishop];
            const auto bishop_id = lower_bit_num(bishop_mask);
            const auto bishop_coord = coords[stm][bishop_id];
            const bool is_light_b = (get_col(bishop_coord) +
                                     get_row(bishop_coord)) & 1;
            const auto corner1 = is_light_b ? get_coord(0, max_row) : 0;
            const auto corner2 = is_light_b ? get_coord(max_col, 0) :
                                              get_coord(max_col, max_row);
            const auto opp_k = king_coord(!stm);
            if(king_dist(corner1, opp_k) > 1 && king_dist(corner2, opp_k) > 1)
                return val;
            const auto opp_col = get_col(opp_k);
            const auto opp_row = get_row(opp_k);
            if(opp_col == 0 || opp_row == 0 ||
                    opp_col == max_col || opp_row == max_row)
                return stm ? imbalance_king_in_corner :
                                 -imbalance_king_in_corner;
            return val;
        }
    }
    // KXPkx, where X = {N, B, R, Q}
    else if((X == 6 || X == 10 || X == 22) &&
            quantity[white][pawn] + quantity[black][pawn] == 1)
    {
        const auto stm = quantity[white][pawn] == 1 ? white : black;
        const auto pawn_mask = exist_mask[stm] & type_mask[stm][pawn];
        const auto pawn_id = lower_bit_num(pawn_mask);
        const auto pawn_coord = coords[stm][pawn_id];
        const auto telestop = get_coord(get_col(pawn_coord),
                                        stm ? max_row : 0);
        auto king_row = get_row(king_coord(stm));
        if(!stm)
            king_row = max_row - king_row;
        const auto king_col = get_col(king_coord(stm));
        const auto pawn_col = get_col(pawn_coord);
        const bool king_in_front = king_row >
                pawn_max[get_col(pawn_coord)][stm] &&
                std::abs(king_col - pawn_col) <= 1;

        if(king_dist(king_coord(!stm), telestop) <= 1 && (!king_in_front
                || king_dist(king_coord(stm), pawn_coord) > 2))
            return val / imbalance_draw_divider;
    }
    // KBPk(p) with pawn at first(last) file and bishop with 'good' color
    else if(X == 3)
    {
        if(quantity[black][pawn] > 1 || quantity[white][pawn] > 1)
            return val;
        if(!quantity[white][bishop] && !quantity[black][bishop])
            return val;
        const bool stm = quantity[white][bishop] == 0;
        const auto pawn_col_min = pawn_max[0][!stm];
        const auto pawn_col_max = pawn_max[max_col][!stm];
        if(pawn_col_max == 0 && pawn_col_min == 0)
            return val;
        const auto bishop_mask = exist_mask[!stm] & type_mask[!stm][bishop];
        if(!bishop_mask)
            return val;
        const auto coord = coords[!stm][lower_bit_num(bishop_mask)];
        const auto sq = get_coord(pawn_col_max == 0 ? 0 : max_col,
                                  stm ? 0 : max_row);
        if(is_same_color(coord, sq))
            return val;

        if(king_dist(king_coord(stm), sq) <= 1)
            return {0, 0};
    }
    else if(X == 0 && (material[white] + material[black])/centipawn == 1)
    {
        // KPk
        const bool side = material[white]/centipawn == 1;
        const auto pawn_mask = exist_mask[side] & type_mask[side][pawn];
        const auto pawn_id = lower_bit_num(pawn_mask);
        const auto coord = coords[side][pawn_id];
        const auto colp = get_col(coord);
        const bool unstop = IsUnstoppablePawn(colp, side, stm);
        const auto k_coord = king_coord(side);
        const auto opp_k_coord = king_coord(!side);
        const auto dist_k = king_dist(k_coord, coord);
        const auto dist_opp_k = king_dist(opp_k_coord, coord);

        if(!unstop && dist_k > dist_opp_k + (stm == side))
            return {0, 0};
        else if((colp == 0 || colp == max_col))
        {
            const auto sq = get_coord(colp, side ? max_row : 0);
            if(king_dist(opp_k_coord, sq) <= 1)
                return {0, 0};
        }
    }
    // KRB(N)kr or KB(N)B(N)kb(n)
    else if((X == 13 || X == 9) &&
            !quantity[white][pawn] && !quantity[black][pawn])
    {
        const bool stm = material[white] < material[black];
        if(!is_king_near_corner(stm))
            val = val*imbalance_no_pawns / 64;
    }

    // two bishops
    if(quantity[white][bishop] == 2)
        val += bishop_pair;
    if(quantity[black][bishop] == 2)
        val -= bishop_pair;

    // pawn absence for both sides
    if(!quantity[white][pawn] && !quantity[black][pawn] &&
            material[white] != 0 && material[black] != 0)
        {
            val.mid = val.mid*imbalance_no_pawns.mid/64;
            val.end = val.end*imbalance_no_pawns.end/64;
        }

    // multicolored bishops
    if(quantity[white][bishop] == 1 && quantity[black][bishop] == 1)
    {
        const auto wb_mask = exist_mask[white] & type_mask[white][bishop];
        const auto wb_id = lower_bit_num(wb_mask);
        const auto bb_mask = exist_mask[black] & type_mask[black][bishop];
        const auto bb_id = lower_bit_num(bb_mask);
        if(is_same_color(coords[white][wb_id], coords[black][bb_id]))
            return val;
        const bool only_pawns =
                material[white]/centipawn - pieces[white] == 4 - 2 &&
                material[black]/centipawn - pieces[black] == 4 - 2;
        if(only_pawns && quantity[white][pawn] + quantity[black][pawn] == 1)
            return val / imbalance_draw_divider;
        auto Ki = only_pawns ? imbalance_multicolor.mid :
                               imbalance_multicolor.end;
        val.end = val.end*Ki / 64;
    }
    return val;
}





//-----------------------------
void k2eval::DbgOut(const char *str, vec2<eval_t> val, vec2<eval_t> &sum) const
{
    std::cout << str << '\t';
    std::cout << val.mid << '\t' << val.end << '\t' <<
                 GetEvalScore(white, val) << std::endl;
    sum += val;
}





//-----------------------------
void k2eval::EvalDebug(const bool stm)
{
    std::cout << "\t\t\tMidgame\tEndgame\tTotal" << std::endl;
    vec2<eval_t> sum = {0, 0}, zeros = {0, 0};
    DbgOut("Material both sides", InitEvalOfMaterial(), sum);
    DbgOut("PST both sides\t", InitEvalOfPST(), sum);
    DbgOut("White pawns\t", EvalPawns(white, stm), sum);
    DbgOut("Black pawns\t", EvalPawns(black, stm), sum);
    DbgOut("King safety white", EvalKingSafety(white), sum);
    DbgOut("King safety black", EvalKingSafety(black), sum);
    DbgOut("Mobility white\t", EvalMobility(white), sum);
    DbgOut("Mobility black\t", EvalMobility(black), sum);
    DbgOut("Rooks white\t", EvalRooks(white), sum);
    DbgOut("Rooks black\t", EvalRooks(black), sum);
    DbgOut("Bonus for side to move", EvalSideToMove(stm), sum);
    DbgOut("Imbalances both sides", EvalImbalances(stm, sum) - sum, zeros);
    sum = EvalImbalances(stm, sum);
    vec2<eval_t> tmp = InitEvalOfMaterial() + InitEvalOfPST();
    if(Eval(wtm, tmp) != GetEvalScore(wtm, sum))
    {
        assert(true);
        std::cout << "\nInternal error, please contact with developer\n";
        return;
    }
    std::cout << std::endl << "Grand Total:\t\t" << sum.mid << '\t' <<
                 sum.end << '\t' << GetEvalScore(white, sum) << std::endl;
    std::cout << "(positive values mean advantage for white)\n\n";
}





//-----------------------------
void k2eval::SetPawnStruct(const bool side, const coord_t col)
{
    assert(col <= max_col);
    auto y = pawn_default_row;
    const auto pwn = black_pawn | side;

    while(b[get_coord(col, side ? y : max_row - y)] != pwn && y < max_row)
        y++;
    pawn_min[col][side] = y;

    y = max_row - pawn_default_row;
    while(b[get_coord(col, side ? y : max_row - y)] != pwn && y > 0)
        y--;
    pawn_max[col][side] = y;
}





//-----------------------------
void k2eval::MovePawnStruct(const bool side, const move_c move,
                            const piece_t moved_piece,
                            const piece_t captured_piece)
{
    if(get_type(moved_piece) == pawn || (move.flag & is_promotion))
    {
        SetPawnStruct(side, get_col(move.to_coord));
        if(move.flag)
            SetPawnStruct(side, get_col(move.from_coord));
    }
    if(get_type(captured_piece) == pawn || (move.flag & is_en_passant))
        SetPawnStruct(!side, get_col(move.to_coord));

#ifndef NDEBUG
    rank_t copy_max[8][2], copy_min[8][2];
    memcpy(copy_max, pawn_max, sizeof(copy_max));
    memcpy(copy_min, pawn_min, sizeof(copy_min));
    InitPawnStruct();
    assert(!memcmp(copy_max, pawn_max, sizeof(copy_max)));
    assert(!memcmp(copy_min, pawn_min, sizeof(copy_min)));
#endif
}





//-----------------------------
void k2eval::InitPawnStruct()
{
    pawn_max = &p_max[1];
    pawn_min = &p_min[1];
    for(auto col: {-1, static_cast<int>(board_width)})
        for(auto side: {black, white})
        {
            pawn_max[col][side] = 0;
            pawn_min[col][side] = max_row;
        }
    for(auto col = 0; col < board_height; col++)
        for(auto side: {black, white})
            SetPawnStruct(side, col);
}





//-----------------------------
bool k2eval::MakeMove(const move_c move)
{
    bool is_special_move = k2chess::MakeMove(move);
    MovePawnStruct(!wtm, move, b[move.to_coord],
                   k2chess::state[ply].captured_piece);
    return is_special_move;
}





//-----------------------------
void k2eval::TakebackMove(const move_c move)
{
    k2chess::TakebackMove(move);
    MovePawnStruct(wtm, move, b[move.from_coord],
                   k2chess::state[ply + 1].captured_piece);
}





//-----------------------------
k2eval::vec2<k2chess::eval_t> k2eval::EvalRooks(const bool stm)
{
    vec2<eval_t> ans(0, 0);
    eval_t rooks_on_last_cr = 0;
    auto mask = exist_mask[stm] & type_mask[stm][rook];
    while(mask)
    {
        auto rook_id = lower_bit_num(mask);
        mask ^= (1 << rook_id);
        const auto coord = coords[stm][rook_id];
        if((stm && get_row(coord) >= max_row - 1) ||
                (!stm && get_row(coord) <= 1))
            rooks_on_last_cr++;
        if(quantity[stm][pawn] >= rook_max_pawns_for_open_file &&
                pawn_max[get_col(coord)][stm] == 0)
            ans += (pawn_max[get_col(coord)][!stm] == 0 ? rook_open_file :
                                                         rook_semi_open_file);
    }
    ans += rooks_on_last_cr*rook_last_rank;
    return stm ? ans : -ans;
}





//-----------------------------
bool k2eval::Sheltered(const coord_t k_col, coord_t k_row,
                       const bool stm) const
{
    if(!col_within(k_col))
        return false;
    if((stm && k_row > 1) || (!stm && k_row < max_row - 1))
        return false;
    const auto shft = stm ? board_width : -board_width;
    const auto coord = get_coord(k_col, k_row);
    const auto p = black_pawn ^ stm;
    if(coord + 2*shft < 0 || coord + 2*shft >= board_size)
        return false;
    if(b[coord + shft] == p)
        return true;
    else
    {
        const auto pc1 = b[coord + shft];
        const auto pc2 = b[coord + 2*shft];
        const auto pc1_r = col_within(k_col + 1) ?
                    b[coord + shft + 1] : empty_square;
        const auto pc1_l = col_within(k_col - 1) ?
                    b[coord + shft - 1] : empty_square;
        if(pc2 == p && (pc1_r == p || pc1_l == p))
            return true;
        if(pc1 != empty_square && get_color(pc1) == stm &&
                pc2 != empty_square && get_color(pc2) == stm)
            return true;

    }
    return false;
}





//-----------------------------
bool k2eval::KingHasNoShelter(coord_t k_col, coord_t k_row,
                              const bool stm) const
{
    if(k_col == 0)
        k_col++;
    else if(k_col == max_col)
        k_col--;
    const i32 cstl[] = {black_can_castle_left | black_can_castle_right,
                        white_can_castle_left | white_can_castle_right};
    if(!quantity[!stm][queen] ||
            (k2chess::state[ply].castling_rights & cstl[stm]))
        return false;
    if(!Sheltered(k_col, k_row, stm))
    {
        if(k_col > 0 && k_col < max_col &&
                (stm ? k_row < max_row : k_row > 0) &&
                b[get_coord(k_col, k_row+(stm ? 1 : -1))] != empty_square &&
                Sheltered(k_col - 1, k_row, stm) &&
                Sheltered(k_col + 1, k_row, stm))
            return false;
        else
            return true;
    }
    if(!Sheltered(k_col - 1, k_row, stm) &&
            !Sheltered(k_col + 1, k_row, stm))
        return true;
    return 0;
}





//-----------------------------
k2chess::attack_t k2eval::KingSafetyBatteries(const coord_t targ_coord,
                                              const attack_t att,
                                              const bool stm) const
{
    auto msk = att & slider_mask[!stm];
    auto ans = att;
    while(msk)
    {
        const auto piece_id = lower_bit_num(msk);
        msk ^= (1 << piece_id);
        const auto coord1 = coords[!stm][piece_id];
        auto maybe = attacks[!stm][coord1] & slider_mask[!stm];
        if(!maybe)
            continue;
        while(maybe)
        {
            const auto j = lower_bit_num(maybe);
            maybe ^= (1 << j);
            const auto coord2 = coords[!stm][j];
            const auto type1 = get_type(b[coord1]);
            const auto type2 = get_type(b[coord2]);
            bool is_ok = type1 == type2 || type2 == queen;
            if(!is_ok && type1 == queen)
            {
                const auto col1 = get_col(coord1);
                const auto col2 = get_col(coord2);
                const auto col_t = get_col(targ_coord);
                const auto row1 = get_row(coord1);
                const auto row2 = get_row(coord2);
                const auto row_t = get_row(targ_coord);
                bool like_rook = (col1 == col2 && col1 == col_t) ||
                        (row1 == row2 && row1 == row_t);
                bool like_bishop = sgn(col1 - col_t) == sgn(col2 - col1) &&
                        sgn(row1 - row_t) == sgn(row2 - row_t);
                if(type2 == rook && like_rook)
                    is_ok = true;
                else if(type2 == bishop && like_bishop)
                    is_ok = true;
            }
            if(is_ok && IsOnRay(coord1, targ_coord, coord2) &&
                    IsSliderAttack(coord1, coord2))
                ans |= (1 << j);
        }
    }
    return ans;
}





//-----------------------------
size_t k2eval::CountAttacksOnKing(const bool stm, const coord_t k_col,
                                  const coord_t k_row) const
{
    size_t ans = 0;
    for(auto delta_col = -1; delta_col <= 1; ++delta_col)
        for(auto delta_row = -1; delta_row <= 1; ++delta_row)
        {
            const auto col = k_col + delta_col;
            const auto row = k_row + delta_row;
            if(!row_within(row) || !col_within(col))
                continue;
            const auto targ_coord = get_coord(col, row);
            const auto att = attacks[!stm][targ_coord];
            if(!att)
                continue;
            const auto all_att = KingSafetyBatteries(targ_coord, att, stm);
            ans += __builtin_popcount(all_att);
        }
    return ans;
}





//-----------------------------
k2eval::vec2<k2chess::eval_t> k2eval::EvalKingSafety(const bool stm)
{
    vec2<eval_t> zeros = {0, 0};
    if(quantity[!stm][queen] < 1 && quantity[!stm][rook] < 2)
        return zeros;
    const auto k_col = get_col(king_coord(stm));
    const auto k_row = get_row(king_coord(stm));

    const eval_t attacks = CountAttacksOnKing(stm, k_col, k_row);
    const eval_t no_shelter = KingHasNoShelter(k_col, k_row, stm);
    const eval_t div = 10;
    const vec2<eval_t> f_saf = no_shelter ?
                king_saf_attack1*king_saf_attack2 / div :
                king_saf_attack2;
    vec2<eval_t> f_queen = {10, 10};
    if(!quantity[!stm][queen])
        f_queen = king_saf_no_queen;
    const vec2<eval_t> center = (std::abs(2*k_col - max_col) <= 1 &&
                              quantity[!stm][queen]) ? king_saf_central_files :
                                                       zeros;
    const eval_t att2 = attacks*attacks;
    const auto ans = center + king_saf_no_shelter*no_shelter +
            att2*f_saf / f_queen;

    return stm ? -ans : ans;
}




#ifndef NDEBUG
//-----------------------------
void k2eval::RunUnitTests()
{
    k2chess::RunUnitTests();

    assert(king_dist(get_coord("e5"), get_coord("f6")) == 1);
    assert(king_dist(get_coord("e5"), get_coord("d3")) == 2);
    assert(king_dist(get_coord("h1"), get_coord("h8")) == 7);
    assert(king_dist(get_coord("h1"), get_coord("a8")) == 7);

    SetupPosition("5k2/8/2P5/8/8/8/8/K7 w - -");
    assert(IsUnstoppablePawn(2, white, white));
    assert(!IsUnstoppablePawn(2, white, black));
    SetupPosition("k7/7p/8/8/8/8/8/1K6 b - -");
    assert(IsUnstoppablePawn(7, black, black));
    assert(!IsUnstoppablePawn(7, black, white));
    SetupPosition("8/7p/8/8/7k/8/8/K7 w - -");
    assert(IsUnstoppablePawn(7, black, black));
    assert(!IsUnstoppablePawn(7, black, white));

    SetupPosition("3k4/p7/B1Pp4/7p/K3P3/7P/2n5/8 w - -");
    assert(IsPasser(0, black));
    assert(IsPasser(2, white));
    assert(!IsPasser(3, black));
    assert(!IsPasser(4, white));
    assert(!IsPasser(7, black));
    assert(!IsPasser(7, white));
    assert(!IsPasser(1, black));

    vec2<eval_t> val, zeros = {0, 0};
    SetupPosition("4k3/1p6/8/8/1P6/8/1P6/4K3 w - -");
    val = EvalPawns(white, wtm);
    assert(val == -pawn_dbl_iso);
    val = EvalPawns(black, wtm);
    assert(val == pawn_iso);
    SetupPosition("4k3/1pp5/1p6/8/1P6/1P6/1P6/4K3 w - -");
    val = EvalPawns(white, wtm);
    assert(val == -pawn_dbl_iso);
    val = EvalPawns(black, wtm);
    assert(val == pawn_dbl);
    SetupPosition("4k3/2p1p1p1/2Np1pPp/7P/1p6/Pr1PnP2/1P2P3/4K3 b - -");
    val = EvalPawns(white, wtm);
    assert(val == static_cast<eval_t>(-2)*pawn_hole - pawn_gap);
    val = EvalPawns(black, wtm);
    assert(val == pawn_hole + pawn_gap);
    SetupPosition("8/8/3K1P2/3p2P1/1Pkn4/8/8/8 w - -");
    val = EvalPawns(white, wtm);
    assert(val == (eval_t)(3*3 + 4*4 + 5*5)*pawn_pass2 +
           (eval_t)(3 + 4 + 5)*pawn_pass1 + (eval_t)3*pawn_pass0 - pawn_iso +
           (eval_t)2*pawn_king_tropism3 - pawn_king_tropism1 -
           (eval_t)3*pawn_king_tropism2 + (eval_t)5*pawn_pass_connected);
    val = EvalPawns(black, wtm);
    assert(val == (eval_t)(-3*3)*pawn_blk_pass2 - (eval_t)3*pawn_blk_pass1 -
           pawn_blk_pass0 + pawn_iso - pawn_king_tropism1 -
           (eval_t)3*pawn_king_tropism2 + pawn_king_tropism3);

    // endings
    val = SetupPosition("5k2/8/8/N7/8/8/8/4K3 w - -");  // KNk
    val = EvalImbalances(wtm, val);
    assert(val.end < pawn_val.end/4);
    val = SetupPosition("5k2/8/8/n7/8/8/8/4K3 w - -");  // Kkn
    val = EvalImbalances(wtm, val);
    assert(val.end > -pawn_val.end/4);
    val = SetupPosition("5k2/8/8/B7/8/8/8/4K3 w - -");  // KBk
    val = EvalImbalances(wtm, val);
    assert(val.end < pawn_val.end/4);
    val = SetupPosition("5k2/8/8/b7/8/8/8/4K3 w - -");  // Kkb
    val = EvalImbalances(wtm, val);
    assert(val.end > -pawn_val.end/4);
    val = SetupPosition("5k2/p7/8/b7/8/8/P7/4K3 w - -");  // KPkbp
    val = EvalImbalances(wtm, val);
    assert(val.end < bishop_val.end);
    val = SetupPosition("5k2/8/8/n7/8/8/P7/4K3 w - -");  // KPkn
    val = EvalImbalances(wtm, val);
    assert(val.end < pawn_val.end/4);
    val = SetupPosition("5k2/p7/8/B7/8/8/8/4K3 w - -");  // KBkp
    val = EvalImbalances(wtm, val);
    assert(val.end > -pawn_val.end/4);
    val = SetupPosition("5k2/p6p/8/B7/8/8/7P/4K3 w - -");  // KBPkpp
    val = EvalImbalances(wtm, val);
    assert(val.end > 3*pawn_val.end/2);
    val = SetupPosition("5k2/8/8/NN6/8/8/8/4K3 w - -");  // KNNk
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("5k2/8/8/nn6/8/8/8/4K3 w - -");  // Kknn
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("5k2/p7/8/nn6/8/8/P7/4K3 w - -");  // KPknnp
    val = EvalImbalances(wtm, val);
    assert(val.end < -2*knight_val.end + pawn_val.end);
    val = SetupPosition("5k2/8/8/nb6/8/8/8/4K3 w - -");  // Kknb
    val = EvalImbalances(wtm, val);
    assert(val.end < -knight_val.end - bishop_val.end +
           pawn_val.end);
    val = SetupPosition("5k2/8/8/1b6/8/7p/8/7K w - -");  // Kkbp not drawn
    val = EvalImbalances(wtm, val);
    assert(val.end < -bishop_val.end- pawn_val.end/2);
    val = SetupPosition("7k/8/7P/1B6/8/8/8/6K1 w - -");  // KBPk drawn
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("5k2/8/8/3b4/p7/8/K7/8 w - -");  // Kkbp drawn
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("7k/8/P7/1B6/8/8/8/6K1 w - -");  // KBPk not drawn
    val = EvalImbalances(wtm, val);
    assert(val.end > bishop_val.end + pawn_val.end/2);
    val = SetupPosition("2k5/8/P7/1B6/8/8/8/6K1 w - -");  // KBPk not drawn
    val = EvalImbalances(wtm, val);
    assert(val.end > bishop_val.end + pawn_val.end/2);
    val = SetupPosition("1K6/8/P7/1B6/8/8/8/6k1 w - -");  // KBPk not drawn
    val = EvalImbalances(wtm, val);
    assert(val.end > bishop_val.end + pawn_val.end/2);
    val = SetupPosition("1k6/8/1P6/1B6/8/8/8/6K1 w - -");  // KBPk not drawn
    val = EvalImbalances(wtm, val);
    assert(val.end > bishop_val.end + pawn_val.end/2);
    val = SetupPosition("5k2/8/8/3b4/p4P2/8/K7/8 w - -");  // KPkbp drawn
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("5k2/8/8/3b4/p4P2/8/8/2K5 w - -");  // KPkbp not drawn
    val = EvalImbalances(wtm, val);
    assert(val.end < -bishop_val.end);

    val = SetupPosition("1k6/8/8/P7/8/4N3/8/6K1 w - -");  // KNPk
    val = EvalImbalances(wtm, val);
    assert(val.end > knight_val.end + pawn_val.end/2);
    val = SetupPosition("8/8/8/5K2/2k5/8/2P5/8 b - -");  // KPk
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("8/8/8/5K2/2k5/8/2P5/8 w - -");  // KPk
    val = EvalImbalances(wtm, val);
    assert(val.end > pawn_val.end/2);
    val = SetupPosition("k7/2K5/8/P7/8/8/8/8 w - -");  // KPk
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("8/8/8/p7/8/1k6/8/1K6 w - -");  // Kkp
    val = EvalImbalances(wtm, val);
    assert(val == zeros);
    val = SetupPosition("8/8/8/p7/8/1k6/8/2K5 w - -");  // Kkp
    val = EvalImbalances(wtm, val);
    assert(val.end < -pawn_val.end/2);
    val = SetupPosition("8/8/8/pk6/8/K7/8/8 w - -");  // Kkp
    val = EvalImbalances(wtm, val);
    assert(val.end < -pawn_val.end/2);
    val = SetupPosition("8/8/8/1k5p/8/8/8/K7 w - -");  // Kkp
    val = EvalImbalances(wtm, val);
    assert(val.end < -pawn_val.end/2);

    SetupPosition("7k/5B2/5K2/8/3N4/8/8/8 w - -");  // KBNk
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.end == 0);
    SetupPosition("7k/4B3/5K2/8/3N4/8/8/8 w - -");  // KBNk
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.end == imbalance_king_in_corner.end);
    SetupPosition("k7/5n2/8/8/8/b7/8/7K w - -");  // Kkbn
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.end == 0);
    SetupPosition("k7/5n2/8/8/8/b7/8/K7 w - -");  // Kkbn
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.end == -imbalance_king_in_corner.end);
    SetupPosition("8/8/8/8/8/2k2n2/4b3/1K6 b - -");  // Kkbn
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.end == 0);
    SetupPosition("8/1K6/8/8/8/2k2n2/4b3/8 b - -");  // Kkbn
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val.end == 0);

    // imbalances with opponent king near telestop
    val = SetupPosition("6k1/4R3/8/8/5KP1/8/3r4/8 b - -");  // KRPkr
    val = EvalImbalances(wtm, val);
    assert(val.end < pawn_val.end/4);
    val = SetupPosition("6k1/4Q3/8/8/5KP1/8/3q4/8 b - -");  // KQPkq
    val = EvalImbalances(wtm, val);
    assert(val.end < pawn_val.end/4);
    val = SetupPosition("8/4B2k/8/8/5KP1/8/3n4/8 b - -");  // KBPkn
    val = EvalImbalances(wtm, val);
    assert(val.end < pawn_val.end/4);
    val = SetupPosition("8/2n1B3/8/8/6k1/1p6/8/1K6 b - -");  // KBknp
    val = EvalImbalances(wtm, val);
    assert(val.end > -pawn_val.end/4);
    val = SetupPosition("8/2n1B3/8/8/6K1/1p6/8/1k6 b - -");  // KBknp
    val = EvalImbalances(wtm, val);
    assert(val.end < -pawn_val.end/4);
    val = SetupPosition("1r6/8/6k1/8/8/1p6/1K6/1R6 b - -");  // KRkrp
    val = EvalImbalances(wtm, val);
    assert(val.end > -pawn_val.end/4);
    val = SetupPosition("6k1/4R3/8/5K2/6P1/8/3r4/8 b - -");  // KRPkr
    val = EvalImbalances(wtm, val);
    assert(val.end > pawn_val.end/4);
    val = SetupPosition("2r5/4R3/8/8/3p4/4k3/8/3K4 b - -");  // KRkrp
    val = EvalImbalances(wtm, val);
    assert(val.end < -pawn_val.end/4);
    val = SetupPosition("2r5/4R3/8/8/3p4/8/8/3K2k1 b - -");  // KRkrp
    val = EvalImbalances(wtm, val);
    assert(val.end > -pawn_val.end/4);
    val = SetupPosition("8/3k4/6K1/4R3/8/6p1/7r/8 w - -");  // KRkrp
    val = EvalImbalances(wtm, val);
    assert(val.end < -pawn_val.end/4);

    val = SetupPosition("8/4R3/3n1K2/8/1k6/8/8/6r1 b - -");  // KRkrn
    val = EvalImbalances(wtm, val);
    assert(val.end > -pawn_val.end);
    val = SetupPosition("4R3/8/1K6/8/8/2B2k2/8/3r4 b - -");  // KRBkr
    val = EvalImbalances(wtm, val);
    assert(val.end > -pawn_val.end);
    val = SetupPosition("8/4R2K/3n4/8/1k6/8/8/6r1 b - -");  // KRkrn
    val = EvalImbalances(wtm, val);
    assert(val.end < -pawn_val.end);

    // bishop pairs
    SetupPosition("rn1qkbnr/pppppppp/8/8/8/8/PPPPPPPP/R1BQKBNR w KQkq -");
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val == bishop_pair);
    SetupPosition("1nb1kb2/4p3/8/8/8/8/4P3/1NB1K1N1 w - -");
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val == -bishop_pair);
    SetupPosition("1nb1k1b1/4p3/8/8/8/8/4P3/1NB1K1N1 w - -");
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val == -bishop_pair);
    SetupPosition("1nb1kbb1/4p3/8/8/8/8/4P3/1NB1K1N1 w - -");
    val = zeros;
    val = EvalImbalances(wtm, val);
    assert(val == zeros);  // NB

    val = SetupPosition("4kr2/8/8/8/8/8/8/1BBNK3 w - -"); // pawn absense
    val = EvalImbalances(wtm, val);
    assert(val.end < rook_val.end/2);

    // multicolored bishops
    val = SetupPosition("rn1qkbnr/p1pppppp/8/8/8/8/PPPPPPPP/RN1QKBNR w KQkq -");
    auto tmp = val;
    val = EvalImbalances(wtm, val);
    assert(val.end < pawn_val.end && val.end < tmp.end);
    val = SetupPosition("rnbqk1nr/p1pppppp/8/8/8/8/PPPPPPPP/RNBQK1NR w KQkq -");
    tmp = val;
    val = EvalImbalances(wtm, val);
    assert(val.end < pawn_val.end && val.end < tmp.end);
    val = SetupPosition("rnbqk1nr/p1pppppp/8/8/8/8/PPPPPPPP/RN1QKBNR w KQkq -");
    tmp = val;
    val = EvalImbalances(wtm, val);
    assert(val.end == tmp.end);
    val = SetupPosition("rn1qkbnr/p1pppppp/8/8/8/8/PPPPPPPP/RNBQK1NR w KQkq -");
    tmp = val;
    val = EvalImbalances(wtm, val);
    assert(val.end == tmp.end);
    val = SetupPosition("4kb2/pppp4/8/8/8/8/PPPPPPPP/4KB2 w - -");
    val = EvalImbalances(wtm, val);
    assert(val.end < 5*pawn_val.end/2);

    //king safety
    SetupPosition("4k3/4p3/8/8/8/4P3/5P2/4K3 w - -");
    assert(Sheltered(4, 0, white));
    assert(Sheltered(4, 7, black));
    SetupPosition("4k3/4r3/4p3/8/8/4B3/4R3/4K3 w - -");
    assert(Sheltered(4, 0, white));
    assert(Sheltered(4, 7, black));
    SetupPosition("4k3/8/4b3/8/8/4P3/3N4/4K3 w - -");
    assert(!Sheltered(4, 0, white));
    assert(!Sheltered(4, 7, black));
    SetupPosition("4k3/4q3/8/8/8/8/8/4K3 w - -");
    assert(!Sheltered(4, 0, white));
    assert(!Sheltered(4, 7, black));

    SetupPosition("q5k1/pp3p1p/6p1/8/8/8/PP3PPP/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("q5k1/pp4pp/5p2/8/8/7P/PP3PP1/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("q5k1/pp3pbp/6n1/8/8/6P1/PP3P1P/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("q5k1/pp3nbr/5nnq/8/8/5P1N/PP4PB/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("q5k1/pp4bn/6nr/8/8/5P2/PP4P1/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(6, 7, black) == false);
    SetupPosition("4k2r/pp6/8/8/8/8/PP6/Q5K1 w k -");
    assert(KingHasNoShelter(6, 0, white) == false);
    assert(KingHasNoShelter(4, 7, black) == false);
    SetupPosition("q3kr2/pp1p1p2/8/8/5P1P/8/PP4P1/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == true);
    assert(KingHasNoShelter(4, 7, black) == true);
    SetupPosition("q5k1/pp3p1p/7p/8/8/5P2/PP3P1P/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == true);
    assert(KingHasNoShelter(4, 7, black) == true);
    SetupPosition("q5k1/pp4p1/6p1/8/8/6N1/PP3P1P/Q5K1 w - -");
    assert(KingHasNoShelter(6, 0, white) == true);
    assert(KingHasNoShelter(4, 7, black) == true);

    SetupPosition("6k1/pp1N1ppp/6r1/8/4n2q/B3p3/PP4P1/Q4RK1 w - -");
    assert(CountAttacksOnKing(white, 6, 0) == 3 + 1 + 1 + 0 + 1);
    assert(CountAttacksOnKing(black, 6, 7) == 1 + 0 + 0 + 2 + 0);
    SetupPosition("2q2rk1/pp3pp1/2r4p/2r5/8/2P4Q/PPP2PPR/1K5R w - -");
    assert(CountAttacksOnKing(white, 1, 0) == 0);
    assert(CountAttacksOnKing(black, 6, 7) == 0);
    SetupPosition("2q2r2/pp3pk1/2r3p1/2r5/8/7Q/PPP2PPR/1K5R w - -");
    assert(CountAttacksOnKing(white, 1, 0) == 0 + 0 + 2 + 0 + 0);
    assert(CountAttacksOnKing(black, 6, 6) == 0+0+2 + 0+0+2 + 0+0+2);
}
#endif // NDEBUG

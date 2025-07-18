#include "eval.h"

using eval_t = eval::eval_t;
using nn_t = eval::nn_t;
using vec2 = eval::vec2<eval::eval_t>;


eval_t eval::Eval() {
    vec2<eval_t> ans = {0, 0};
    fill_attack_array();
    for (auto color : {black, white}) {
        ans += eval_material(color);
        ans += eval_pst(color);
        ans += eval_pawns(color);
        ans += eval_king_safety(color);
        ans += eval_mobility(color);
        // ans += eval_rooks(color);
        // ans += eval_bishops(color);
        // ans += eval_knights(color);
        ans = -ans;
    }

    // cur_eval += e.eval_side_to_move(stm);
    // cur_eval = e.eval_imbalances(stm, cur_eval);
    return interpolate_eval(ans);
}


eval_t eval::Eval_NN() {
    eval_t W = eval_t(10000*calc_nn_out(white));
    eval_t B = eval_t(10000*calc_nn_out(black));
    return side == white ? eval_t(W - B) : eval_t(B - W);
}


nn_t eval::calc_nn_out(const bool color) {
    in2 = calc_first_nn_layer(color);
    auto in3 = A2*in2 + B2;
    in3.transform_each(fptr(tanh));
    auto in4 = A3*in3 + B3;
    return in4.at(0, 0);
}


matrix<nn_t, eval::first_layer_size, 1> &
eval::calc_first_nn_layer(bool color) {
    sparce_multiply(color);
    in2 = in2 + B1;
    in2.transform_each(fptr(tanh));
    return in2;
}


void eval::sparce_multiply(const bool color) {
    in2.transform_each([](nn_t) {return 0;}); // clear array
    for (unsigned pc_ix = 0; pc_ix < occupancy_ix; ++pc_ix) {
        u64 cur = bb[color][pc_ix];
        while (cur != 0) {
            const u64 lowbit = lower_bit(cur);
            const u8 coord = trail_zeros(lowbit) ^ u8(color == white ? 0 : 56);
            const int index = int(64*pc_ix + coord);
            for (int i = 0; i < int(first_layer_size); ++i)
                in2.at(i, 0) += A1.at(i, index);
            cur ^= lowbit;
        }
    }
}


vec2 eval::eval_material(bool color) {
    vec2<eval_t> piece_values_sum;
    int mat_cr = 0;
    int pieces_cr = 0;
    for (unsigned ix = pawn_ix; ix < king_ix; ++ix) {
        eval_t n_pieces = eval_t(__builtin_popcountll(bb[color][ix]));
        piece_values_sum += vec2<eval_t>(n_pieces, n_pieces) * piece_values[ix];
        mat_cr += n_pieces*material_values[ix];
        pieces_cr += n_pieces;
    }
    material_sum[color] = mat_cr;
    num_pieces[color] = pieces_cr;
    return piece_values_sum;
}


vec2 eval::eval_pst(bool color) {
    u8 invs[] = {0, 56};
    vec2<eval_t> ans = {0, 0};
    for (unsigned ix = pawn_ix; ix <= king_ix; ++ix) {
        u64 bitboard = bb[color][ix];
        while (bitboard != 0) {
            auto bit = lower_bit(bitboard);
            auto coord = u8(trail_zeros(bit)) ^ invs[color];
            ans += pst[ix][coord];
            bitboard ^= bit;
        }
    }
    return ans;
}


vec2 eval::eval_pawns(bool color) {
    vec2<eval_t> ans = eval_double_and_isolated(color);

    u64 passers = passed_pawns(color);
    u64 non_passers = bb[color][pawn_ix] ^ passers;
    int holes = __builtin_popcountll(pawn_holes(non_passers, color));
    ans += pawn_hole*eval_t(holes);
    eval_t gaps = pawn_gaps(bb[color][pawn_ix]);
    ans += pawn_gap*gaps;
    ans -= eval_king_tropism(passers, color);
    ans -= eval_passers(passers, color);
    return -ans;
}


vec2 eval::eval_king_safety(bool color) {
    if (__builtin_popcountll(bb[!color][queen_ix]) == 0)
        return {0, 0};
    if (!bb[!color][knight_ix] && !bb[!color][bishop_ix] && !bb[!color][rook_ix])
        return {0, 0};
    vec2<eval_t> ans = {0, 0};
    u64 k_bb = bb[color][king_ix];
    if (k_bb & (file_mask('d') | file_mask('e')))
        ans += king_saf_no_shelter;
    else {
        u64 lr = roll_left(k_bb) | roll_right(k_bb);
        bool shlt1 = (bb[color][pawn_ix] & signed_shift(lr, shifts(color))) != 0;
        bool shlt2 = (bb[color][pawn_ix] & (signed_shift(k_bb, shifts(color)) |
            signed_shift(k_bb, 2*shifts(color)))) != 0;
        if (!shlt1 || !shlt2)  // at least two pawns
            ans += king_saf_no_shelter;
    }
    u64 king_nearest = k_bb | nearest_squares(k_bb);
    u64 king_zone = (king_quaterboard(k_bb) | king_neighborhood(k_bb)) &
        ~king_nearest;
    int attacks = king_attacks(color, king_nearest) +
	    king_attacks(color, king_zone)*king_saf_attacks2.mid/32;
    auto att_val = eval_t(attacks*attacks)*king_saf_attacks1/32;
    return ans + att_val;
}


int eval::king_attacks(bool color, u64 king_zone) {
    int attacks = 0;
    for (unsigned i = 0; i < attack_arr_ix[!color]; ++i)
        attacks += __builtin_popcountll(attack_arr[!color][i].first & king_zone);
    return attacks;
}


u64 eval::king_quaterboard(u64 k_bb) {
    std::array<u64, 4> quaterboards =
        {0x0f0f0f0f, 0xf0f0f0f0, u64(0x0f0f0f0f) << 32, u64(0xf0f0f0f0) << 32};
    u8 k_coord = trail_zeros(k_bb);
    unsigned col = get_col(k_coord)/4;
    unsigned row = get_row(k_coord)/4;
    return quaterboards.at(col + 2*row);
}


u64 eval::king_neighborhood(u64 k_bb) {
    k_bb |= roll_left(k_bb) | roll_right(k_bb);
    k_bb |= roll_left(k_bb) | roll_right(k_bb);
    k_bb |= (k_bb << 8) | (k_bb >> 8);
    k_bb |= (k_bb << 8) | (k_bb >> 8);
    return k_bb;
}


void eval::fill_attack_array() {
    for (auto color : {black, white}) {
        attack_arr[color][0] =
            {all_pawn_attacks(bb[color][pawn_ix], color, u64(-1)), pawn_ix};
        attack_arr_ix[color] = 1;
        for (auto piece_ix : {knight_ix, bishop_ix, rook_ix, queen_ix, king_ix}) {
            fill_attacks_piece_type(color, piece_ix);
        }
    }
}


void eval::fill_attacks_piece_type(bool color, u8 piece_ix) {
    u64 occupancy = bb[0][occupancy_ix] | bb[1][occupancy_ix];
    u64 piece_occ = bb[color][piece_ix];
    while (piece_occ != 0) {
        u64 lowbit = lower_bit(piece_occ);
        u8 from_coord = u8(trail_zeros(lowbit));
        u64 att = all_non_pawn_attacks(piece_ix, from_coord, occupancy);
        attack_arr[color][attack_arr_ix[color]++] = {att, piece_ix};
        piece_occ ^= lowbit;
    }
}


vec2 eval::eval_mobility(bool color) {
    vec2<eval_t> ans = {0, 0};
    u64 occupancy = bb[0][occupancy_ix] | bb[1][occupancy_ix];
    for (unsigned i = 0; i < attack_arr_ix[color]; ++i) {
        u8 ix = attack_arr[color][i].second;
        if (ix == pawn_ix || ix == knight_ix || ix == king_ix)
            continue;
        u64 attacks = attack_arr[color][i].first;
        u8 n_attacks = u8(__builtin_popcountll(attacks & ~occupancy));
        if (ix == queen_ix)
            n_attacks /= 2;
        eval_t mob_value = mobility_curve[n_attacks];
        ans += mob_value*mobility_factor/10;
    }
    return ans;
}


vec2 eval::eval_passers(u64 passers, bool color) {
    vec2<eval_t> ans = {0, 0};
    while (passers != 0) {
        u64 lowbit = lower_bit(passers);
        eval_t row = row_from_bb(lowbit, color);
        ans += eval_t(row*row)*pawn_pass2 + row*pawn_pass1 + pawn_pass0;
        passers ^= lowbit;
    }
    return ans;
}


vec2 eval::eval_king_tropism(u64 pass, bool color) {
    u64 king_tropism1 = king_pawn_tropism(pass, color, bb[color][king_ix], 1);
    u64 king_tropism2 = king_pawn_tropism(pass, color, bb[color][king_ix], 2);
    u64 opp_tropism1 = king_pawn_tropism(pass, color, bb[!color][king_ix], 1);
    u64 opp_tropism2 = king_pawn_tropism(pass, color, bb[!color][king_ix], 2);
    vec2<eval_t> ans = {0, 0};
    while (king_tropism1 != 0) {
        u64 lowbit = lower_bit(king_tropism1);
        ans += pawn_king_tropism1 + row_from_bb(lowbit, color)*pawn_king_tropism2;
        king_tropism1 ^= lowbit;
    }
    while (opp_tropism1 != 0) {
        u64 lowbit = lower_bit(opp_tropism1);
        ans -= pawn_king_tropism1 + row_from_bb(lowbit, color)*pawn_king_tropism2;
        opp_tropism1 ^= lowbit;
    }
    ans += pawn_king_tropism3*eval_t(__builtin_popcountll(king_tropism2));
    ans -= pawn_king_tropism3*eval_t(__builtin_popcountll(opp_tropism2));
    return ans;
}


u64 eval::double_pawns(bool color)  {
    u64 p_bb = bb[color][pawn_ix];
    return p_bb & signed_shift(p_bb, -shifts(color));
}


u64 eval::isolated_pawns(bool color) {
    u64 p_bb = bb[color][pawn_ix];
    u64 rank = 0xff & (fill_down(p_bb) | fill_up(p_bb));
    u64 rank_iso = rank & ~(rank << 1 | rank >> 1);
    return p_bb & fill_up(rank_iso);
}


vec2 eval::eval_double_and_isolated(bool color) {
    u64 doubled = double_pawns(color);
    u64 isolated = isolated_pawns(color);
    u64 dbl_iso = doubled & isolated;
    doubled = doubled ^ dbl_iso;
    isolated = isolated ^ (dbl_iso | signed_shift(dbl_iso, shifts(color)));
    auto dbl_iso_val = eval_t(__builtin_popcountll(dbl_iso)) * pawn_dbl_iso;
    auto dbl_val = eval_t(__builtin_popcountll(doubled)) * pawn_doubled;
    auto iso_val = eval_t(__builtin_popcountll(isolated)) * pawn_isolated;
    return dbl_iso_val + dbl_val + iso_val;
}


u64 eval::passed_pawns(bool color)  {
u64 args[] =  {color, bb[!color][pawn_ix]};
    return for_each_set_bit(bb[color][pawn_ix], passed_pawn, args);
}


u64 eval::passed_pawn(u64 pawn_bb, u64 *args) {
    bool color = bool(args[0]);
    u64 opp_pawns = args[1];
    u64 wide = pawn_bb | roll_left(pawn_bb) | roll_right(pawn_bb);
    u64 fill = color == white ? fill_up(wide) : fill_down(wide);
    return (fill & opp_pawns) == 0 ? pawn_bb : 0;
}


u8 eval::distance(u8 coord1, u8 coord2) {
    u8 d_col = u8(std::abs(int(get_col(coord1)) - int(get_col(coord2))));
    u8 d_row = u8(std::abs(int(get_row(coord1)) - int(get_row(coord2))));
    return std::max(d_col, d_row);
}


u64 eval::unstoppable_pawns(u64 passers, bool color, bool side_to_move) {
    u64 args[] = {color, side_to_move, bb[black][king_ix], bb[white][king_ix]};
    return for_each_set_bit(passers, eval::unstoppable_pawn, args);
}


u64 eval::unstoppable_pawn(u64 passer_bb , u64 *args) {
    bool color = args[0];
    bool side_to_move = args[1];
    u64 kings_bb[] = {args[2], args[3]};
    u64 p_fill = unstop_fill(passer_bb, color, kings_bb);
    u64 last_rank[] = {rank_mask('1'), rank_mask('8')};
    u8 transform_coord = u8(trail_zeros(p_fill & last_rank[color]));
    u8 opp_k_coord = u8(trail_zeros(kings_bb[!color]));
    u8 k_dist = distance(opp_k_coord, transform_coord);
    bool tempo = side_to_move != color;
    u8 p_dist = u8(__builtin_popcountll(p_fill));
    return k_dist > p_dist + tempo ? passer_bb : 0;
}


u64 eval::unstop_fill(u64 passer_bb, bool color, u64 *kings_bb) {
    u64 p_bb = passer_bb;
    if (color == white) {
        if ((kings_bb[white] & fill_up(p_bb)) != 0)
            p_bb >>= 8;
        if ((passer_bb & rank_mask('2')) != 0)
            p_bb <<= 8;
        return fill_up(p_bb);
    }
    if ((kings_bb[black] & fill_down(p_bb)) != 0)
        p_bb <<= 8;
    if ((passer_bb & rank_mask('7')) != 0)
        p_bb >>= 8;
    return fill_down(p_bb);
}


u64 eval::connected_passers(u64 passers) {
    u64 args[] = {passers};
    return for_each_set_bit(passers, eval::adjacent_pawn, args);
}


u64 eval::adjacent_pawn(u64 pawn_bb, u64 *args) {
    u64 pawns = args[0];
    u64 left = roll_left(pawn_bb);
    left |= (left << 8) | (left >> 8);
    u64 right = roll_right(pawn_bb);
    right |= right << 8 | right >> 8;
    return pawns & (left | right);
}


eval_t eval::pawn_gaps(u64 pawns) {
    u64 arg[] = {pawns};
    u64 B = for_each_set_bit(pawns & ~file_mask('a'), eval::one_pawn_gap, arg);
    return eval_t(__builtin_popcountll(B));
}


u64 eval::one_pawn_gap(u64 pawn_bb, u64 *args)  {
    u64 pawns = args[0];
    u64 left_3sq = pawn_bb >> 1;
    left_3sq |= (left_3sq << 8) | (left_3sq >> 8);
    u64 left_file = left_3sq | (left_3sq << 24) | (left_3sq << 48);
    left_file |= left_file >> 32;
    u64 gaps = (left_file ^ left_3sq) & pawns;
    return  left_file == 0 || gaps == 0 ? 0 : pawn_bb;
}


u64 eval::pawn_holes(u64 passers, bool color) {
    u64 opp_pieces = bb[!color][occupancy_ix] ^ bb[!color][pawn_ix];
    u64 args[] = {passers, u64(color), opp_pieces};
    return for_each_set_bit(passers, eval::one_pawn_hole, args);
}


u64 eval::one_pawn_hole(u64 pawn_bb, u64 *args) {
    u64 pawns = args[0];
    bool color = bool(args[1]);
    u64 opp_pieces = args[2];
    u64 up = color == white ? fill_up(pawn_bb) : fill_down(pawn_bb);
    u64 tmp = roll_left(up) | roll_right(up);
    if ((tmp & pawns) == 0)
        return 0;
    u64 dwn = color == white ? fill_down(pawn_bb) : fill_up(pawn_bb);
    tmp = roll_left(dwn) | roll_right(dwn);
    if ((tmp & pawns) != 0)
        return 0;
    u64 weak_sq = color == white ? pawn_bb << 8 : pawn_bb >> 8;
    return (weak_sq & opp_pieces) != 0 ? pawn_bb : 0;
}


u64 eval::king_pawn_tropism(u64 passers, bool color, u64 king_bb, u8 dist) {
    u64 args[] = {color, king_bb, u64(dist)};
    u64 ans = for_each_set_bit(passers, eval::tropism, args);
    if (dist == 1) {
        args[2] = 0;
        ans |= for_each_set_bit(passers, eval::tropism, args);
    }
    return ans;
}


u64 eval::tropism(u64 pawn_bb, u64 *args) {
    bool color = args[0];
    u64 king_bb = args[1];
    u8 dist = u8(args[2]);
    u8 stop_coord = u8(trail_zeros(signed_shift(pawn_bb, shifts(color))));
    u8 king_coord = u8(trail_zeros(king_bb));
    return distance(king_coord, stop_coord) == dist ? pawn_bb : 0;
}


u64 eval::for_each_set_bit(u64 bitboard, const eval_fptr &foo, u64 *args) {
    u64 ans = 0;
    while (bitboard != 0) {
        auto bit = lower_bit(bitboard);
        ans |= (foo)(bit, args);
        bitboard ^= bit;
    }
    return ans;
}


u64 eval::sum_for_each_set_bit(u64 bitboard, const eval_fptr &foo, u64 *args) {
    u64 ans = 0;
    while (bitboard != 0) {
        auto bit = lower_bit(bitboard);
        ans += (foo)(bit, args);
        bitboard ^= bit;
    }
    return ans;
}


eval_t eval::interpolate_eval(vec2<eval_t> val) {
        int X, Y;
        X = material_sum[0]/100 + 1 + material_sum[1]/100 + 1 -
            num_pieces[0] - num_pieces[1];
        Y = ((val.mid - val.end)*X + 80*val.end)/80;
        return side ? eval_t(-Y) : eval_t(Y);
}

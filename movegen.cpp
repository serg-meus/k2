#include "movegen.h"





//--------------------------------
k2movegen::movcr_t k2movegen::GenMoves(move_c * const move_array,
                                       const bool capt_or_promo)
{
    movcr_t move_cr = 0;

    if(!capt_or_promo)
        GenCastles(move_array, &move_cr);

    auto it = coords[wtm].begin();
    for(; it != coords[wtm].end(); ++it)
    {
        const auto from_coord = *it;
        const auto type = get_type(b[from_coord]);
        if(type == pawn)
        {
            if(capt_or_promo)
                GenPawnCapturesPromotions(move_array, &move_cr, it);
            else
                GenPawnSilent(move_array, &move_cr, it);
            continue;
        }

        for(auto ray = 0; ray < rays[type]; ray++)
        {
            const auto col0 = get_col(from_coord);
            const auto row0 = get_row(from_coord);
            const auto ray_len = mobility[wtm][it.get_array_index()][ray];
            for(int i = ray_len; i >= (capt_or_promo ? ray_len : 1); --i)
            {
                const auto col = col0 + i*delta_col[type][ray];
                const auto row = row0 + i*delta_row[type][ray];
                const auto to_coord = get_coord(col, row);
                bool empty = b[to_coord] == empty_square;
                bool capture = !empty && get_color(b[to_coord]) != wtm;
                if(capt_or_promo && !capture)
                    continue;
                if(!capt_or_promo && is_slider[type] && capture)
                    continue;
                if(col_within(col) && row_within(row) && (empty || capture))
                    PushMove(move_array, &move_cr, it.get_array_index(),
                             to_coord,  empty ? 0 : is_capture);
                if(!is_slider[type] || !empty)
                    break;
            }
        }
    }
    return move_cr;
}





//--------------------------------
void k2movegen::GenPawnSilent(move_c * const move_array,
                              movcr_t * const moveCr,
                              const iterator it)
{
    const auto from_coord = *it;
    const auto index = it.get_array_index();
    const auto col = get_col(from_coord);
    const auto row = get_row(from_coord);
    if(row == (wtm ? max_row - 1 : 1))
        return;
    auto d_row = wtm ? 1 : -1;

    const auto to_coord = get_coord(col, row + d_row);
    if(b[to_coord] == empty_square)
        PushMove(move_array, moveCr, index, to_coord, 0);

    const auto ini_row = wtm ? pawn_default_row : max_row - pawn_default_row;
    const auto to_coord2 = get_coord(col, row + 2*d_row);
    if(row == ini_row && b[to_coord2] == empty_square
            && b[to_coord] == empty_square)
        PushMove(move_array, moveCr, index, to_coord2, 0);
}





//--------------------------------
void k2movegen::GenPawnCapturesPromotions(move_c * const move_array,
                                          movcr_t * const moveCr,
                                          const iterator it)
{
    const auto from_coord = *it;
    const auto index = it.get_array_index();
    const auto col = get_col(from_coord);
    const auto row = get_row(from_coord);

    move_flag_t promo_index_beg = 0, promo_index_end = 0;
    if(row == (wtm ? max_row - pawn_default_row : pawn_default_row))
    {
        promo_index_beg = is_promotion_to_queen;
        promo_index_end = is_promotion_to_bishop;
    }
    auto d_row = wtm ? 1 : -1;
    for(auto promo_flag = promo_index_beg;
        promo_flag <= promo_index_end;
        ++promo_flag)
    {
        for(auto d_col: {-1, 1})
        {
            auto to_coord = get_coord(col + d_col, row + d_row);
            const move_flag_t flag = is_capture | promo_flag;
            if(col_within(col + d_col) && b[to_coord] != empty_square
                    && get_color(b[to_coord]) != wtm)
                PushMove(move_array, moveCr, index, to_coord, flag);
        }
        if(row != (wtm ? max_row - 1 : 1))
            continue;
        const auto to_coord = get_coord(col, row + d_row);
        if(b[to_coord] == empty_square)
            PushMove(move_array, moveCr, index, to_coord, promo_flag);
    }
    const auto ep = k2chess::state[ply].en_passant_rights;
    const auto delta = ep - 1 - col;
    const auto ep_row = wtm ?
        max_row - pawn_default_row - pawn_long_move_length :
        pawn_default_row + pawn_long_move_length;
    if(ep && std::abs(delta) == 1 && row == ep_row)
        PushMove(move_array, moveCr, index,
                 get_coord(col + delta, row + d_row),
                 is_capture | is_en_passant);
}





//--------------------------------
void k2movegen::GenCastles(move_c * const move_array, movcr_t * const moveCr)
{
    const castle_t msk = wtm ? 0x03 : 0x0C;
    const auto shft = wtm ? 0 : 2;
    const auto rights = (k2chess::state[ply].castling_rights & msk) >> shft;
    if(!rights)
        return;

    if(attacks[!wtm][*king_coord[wtm]])
        return;
    const auto row = wtm ? 0 : max_row;
    int col_beg[] = {default_king_col + 1, default_king_col - 2};
    int col_end[] = {default_king_col + 2, default_king_col - 1};
    int delta[] = {cstl_move_length, -cstl_move_length};
    move_flag_t flag[] = {is_right_castle, is_left_castle};

    for(auto i: {0, 1})
    {
        if(!(rights & (i + 1)))
            continue;
        bool occupied_or_attacked = false;
        for(auto col = col_beg[i]; col <= col_end[i]; ++col)
        {
            const auto sq = get_coord(col, row);
            if(b[sq] != empty_square || attacks[!wtm][sq])
                occupied_or_attacked = true;
        }
        if(flag[i] == is_left_castle &&
           b[get_coord(default_king_col - 3, row)] != empty_square)
            occupied_or_attacked = true;
        if(!occupied_or_attacked)
            PushMove(move_array, moveCr, king_coord[wtm].get_array_index(),
                     *king_coord[wtm] + delta[i], flag[i]);
    }
}





//--------------------------------
void k2movegen::AppriceMoves(move_c * const move_array, const movcr_t moveCr,
                             move_c * const bestMove)
{
    min_history = std::numeric_limits<history_t>::max();
    max_history = 0;

    auto bm = *move_array;
    if(bestMove == nullptr)
        bm.flag = 0xFF;
    else
        bm = *bestMove;
    for(auto i = 0; i < moveCr; ++i)
    {
        auto m = move_array[i];

        const auto it = coords[wtm].at(m.piece_index);
        const auto fr_sq = b[*it];
        const auto to_sq = b[m.to_coord];

        if(m == bm)
            move_array[i].priority = move_from_hash;
        else if(to_sq == empty_square && !(m.flag & is_promotion))
        {
            if(m == killers[ply][0])
                move_array[i].priority = first_killer;
            else if(m == killers[ply][1])
                move_array[i].priority = second_killer;
            else
            {
                const auto from_coord = *it;
                const auto type = get_type(b[from_coord]);
                auto h = history[wtm][type - 1][m.to_coord];
                if(h > max_history)
                    max_history = h;
                if(h < min_history)
                    min_history = h;

                auto y = get_row(m.to_coord);
                const auto x = get_col(m.to_coord);
                auto y0 = get_row(*it);
                const auto x0 = get_col(*it);
                if(wtm)
                {
                    y = 7 - y;
                    y0 = 7 - y0;
                }
                const auto pstVal = pst[get_type(fr_sq) - 1][0][y][x]
                              - pst[get_type(fr_sq) - 1][0][y0][x0];
                const auto priority = 96 + pstVal/2;
                move_array[i].priority = priority;
            }
        }
        else
        {
            auto src = piece_values[get_type(fr_sq)]/10;
            auto dst =  piece_values[get_type(to_sq)]/10;
            if(!(m.flag & is_capture))
                dst = 0;
            if(dst && dst - src <= 0)
            {
                auto tmp = SEE_main(m);
                dst = tmp;
                src = 0;
            }

            if(dst > 120)
            {
                move_array[i].priority = 0;
                continue;
            }

            eval_t prms[] = {0, 120, 40, 60, 40};
            if(dst <= 120 && (m.flag & is_promotion))
                dst += prms[m.flag & is_promotion];

            auto ans = dst >= src ? dst - src/16 : dst - src;

            if(dst - src >= 0)
            {
                assert(200 + ans/8 > first_killer);
                assert(200 + ans/8 <= 250);
                move_array[i].priority = (200 + ans/8);
            }
            else
            {
                if(get_type(b[*it]) != king)
                {
                    assert(-ans/2 >= 0);
                    assert(-ans/2 <= bad_captures);
                    move_array[i].priority = -ans/2;
                }
                else
                    move_array[i].priority = 0;
            }
        }
    }

    for(auto i = 0; i < moveCr; ++i)
    {
        auto m = move_array[i];
        if(m.priority >= second_killer || (m.flag & is_capture))
            continue;
        const auto it = coords[wtm].at(m.piece_index);
        const auto from_coord = *it;
        const auto type = get_type(b[from_coord]);
        auto h = history[wtm][type - 1][m.to_coord];
        if(h > 3)
        {
            h -= min_history;
            h = 64*h / (max_history - min_history + 1);
            h += 128;
            move_array[i].priority = h;
            continue;
        }
    }
}





//-----------------------------
void k2movegen::ProcessSeeBatteries(coord_t to_coord, coord_t attacker_coord)
{
    const auto mask = attacks[wtm][attacker_coord] & slider_mask[wtm];
    if(!mask)
        return;
    for(size_t i = 0; i < attack_digits; ++i)
    {
        if(!((mask >> i) & 1))
            continue;
        if(mask >> i == 0)
            break;
        const auto second_coord = *coords[wtm].at(i);
        if(!IsOnRay(second_coord, to_coord, attacker_coord))
            continue;
        if(!IsSliderAttack(attacker_coord, second_coord))
            continue;
        attacks[wtm][to_coord] |= 1 << i;
        return;
    }
}




//-----------------------------
k2chess::eval_t k2movegen::SEE(const coord_t to_coord, const eval_t fr_value,
                               eval_t val, const bool stm)
{
    const auto index = SeeMinAttacker(to_coord);
    if(index == -1U)
        return -val;
    attacks[wtm][to_coord] ^= 1 << index;
    const auto it = coords[wtm].at(index);
    const auto type = get_type(b[*it]);
    if(type != knight)
        ProcessSeeBatteries(to_coord, *it);

    val -= fr_value;
    const auto tmp1 = -val;
    wtm = !wtm;
    const auto tmp2 = -SEE(to_coord, piece_values[type], -val, stm);
    wtm = !wtm;
    val = std::min(tmp1, tmp2);

    return val;
}





//-----------------------------
size_t k2movegen::SeeMinAttacker(const coord_t to_coord)
{
    const auto att = attacks[wtm][to_coord];
    if(!att)
        return -1U;
    return __builtin_ctz(att);
}





//-----------------------------
k2chess::eval_t k2movegen::SEE_main(const move_c move)
{
    const auto it = coords[wtm].at(move.piece_index);
    const auto fr_piece = b[*it];
    const auto to_piece = b[move.to_coord];
    const auto fr_type = get_type(fr_piece);
    const auto to_type = get_type(to_piece);
    const auto src = piece_values[fr_type];
    const auto dst = (move.flag & is_capture) ? piece_values[to_type] : 0;
    const auto store_att1 = attacks[wtm][move.to_coord];
    const auto store_att2 = attacks[!wtm][move.to_coord];

    attacks[wtm][move.to_coord] ^= 1 << move.piece_index;
    if(fr_type != knight)
        ProcessSeeBatteries(move.to_coord, *it);
    wtm = !wtm;
    auto see_score = -SEE(move.to_coord, src, dst, wtm);
    wtm = !wtm;
    see_score = std::min(dst, see_score);

    attacks[wtm][move.to_coord] = store_att1;
    attacks[!wtm][move.to_coord] = store_att2;
    return see_score;
}




#ifndef NDEBUG
//--------------------------------
size_t k2movegen::test_gen_pawn(const char* coord,
                                bool captures_and_promotions)
{
    move_c move_array[move_array_size];
    movcr_t move_cr = 0;

    size_t index = find_piece(wtm, get_coord(coord));
    GenPawnCapturesPromotions(move_array, &move_cr, coords[wtm].at(index));
    if(!captures_and_promotions)
        GenPawnSilent(move_array, &move_cr, coords[wtm].at(index));
    for(auto i = 0; i < move_cr; ++i)
        if(!IsLegal(move_array[i]))
            return -1U;
    return move_cr;
}





//--------------------------------
size_t k2movegen::test_gen_castles()
{
    move_c move_array[move_array_size];
    movcr_t move_cr = 0;

    GenCastles(move_array, &move_cr);
    for(auto i = 0; i < move_cr; ++i)
        if(!IsLegal(move_array[i]))
            return -1U;
    return move_cr;
}





//--------------------------------
size_t k2movegen::test_gen_moves()
{
    move_c move_array[move_array_size];
    movcr_t move_cr = GenAllMoves(move_array);
    for(auto i = 0; i < move_cr; ++i)
        if(!IsPseudoLegal(move_array[i]))
            return -1U;
    return move_cr;
}





//--------------------------------
void k2movegen::RunUnitTests()
{
    k2eval::RunUnitTests();

    bool all_moves = false, only_captures = true;

    SetupPosition("4k3/p2pp3/8/1pp1P3/2P2p2/1P2P3/P2P2P1/4K3 w - - 0 1");

    assert(test_gen_pawn("a2", all_moves) == 2);
    assert(test_gen_pawn("b3", all_moves) == 1);
    assert(test_gen_pawn("c4", all_moves) == 1);
    assert(test_gen_pawn("e3", all_moves) == 2);
    assert(test_gen_pawn("e5", all_moves) == 1);
    assert(test_gen_pawn("a2", only_captures) == 0);
    assert(test_gen_pawn("b3", only_captures) == 0);
    assert(test_gen_pawn("c4", only_captures) == 1);
    assert(test_gen_pawn("e3", only_captures) == 1);
    assert(test_gen_pawn("e5", only_captures) == 0);

    assert(MakeMove("a2a3"));
    assert(MakeMove("d7d5"));

    assert(test_gen_pawn("e5", all_moves) == 2);
    assert(test_gen_pawn("e5", only_captures) == 1);

    assert(TakebackMove());
    assert(TakebackMove());

    assert(MakeMove("g2g4"));

    assert(test_gen_pawn("f4", all_moves) == 3);
    assert(test_gen_pawn("a7", all_moves) == 2);
    assert(test_gen_pawn("b5", all_moves) == 2);
    assert(test_gen_pawn("f4", only_captures) == 2);
    assert(test_gen_pawn("a7", only_captures) == 0);
    assert(test_gen_pawn("b5", only_captures) == 1);

    SetupPosition("1n2k1nb/P5P1/8/8/8/8/1p4p1/BNN1K2B w - - 0 1");

    assert(test_gen_pawn("a7", all_moves) == 8);
    assert(test_gen_pawn("g7", all_moves) == 4);
    assert(test_gen_pawn("a7", only_captures) == 8);
    assert(test_gen_pawn("g7", only_captures) == 4);

    assert(MakeMove("e1d1"));
    assert(test_gen_pawn("b2", all_moves) == 8);
    assert(test_gen_pawn("g2", all_moves) == 8);
    assert(test_gen_pawn("b2", only_captures) == 8);
    assert(test_gen_pawn("g2", only_captures) == 8);

    SetupPosition("r3k2r/p5pp/8/8/8/8/P4P1P/R3K2R w KQkq - 0 1");
    assert(test_gen_castles() == 2);
    assert(MakeMove("e1g1"));
    assert(test_gen_castles() == 2);
    SetupPosition("r3k2r/p5pp/7N/4B3/4b3/8/5P1P/RN2K2R w KQkq - 0 1");
    assert(test_gen_castles() == 1);
    assert(MakeMove("e1g1"));
    assert(test_gen_castles() == 1);
    SetupPosition("rn2k2r/p5pp/8/8/8/6P1/P6P/R3K1NR w KQkq - 0 1");
    assert(test_gen_castles() == 1);
    assert(MakeMove("e1e2"));
    assert(test_gen_castles() == 1);

	SetupPosition("r3k1nr/ppp1bppp/8/2Ppp3/4P2q/3P4/PP3PPP/RNBQK2R w KQq d6");
	assert(test_gen_moves() == 35);
	assert(MakeMove("a2a3"));
	assert(test_gen_moves() == 39);

    SetupPosition("3k4/3b4/8/1Q5p/6B1/1r4N1/4p1nR/4K3 w - - 0 1");
    auto index = SeeMinAttacker(get_coord("e2"));
    assert(*coords[wtm].at(index) == get_coord("g3"));
    index = SeeMinAttacker(get_coord("h5"));
    assert(*coords[wtm].at(index) == get_coord("g3"));
    index = SeeMinAttacker(get_coord("b3"));
    assert(*coords[wtm].at(index) == get_coord("b5"));
    index = SeeMinAttacker(get_coord("g2"));
    assert(*coords[wtm].at(index) == get_coord("h2"));
    index = SeeMinAttacker(get_coord("d7"));
    assert(*coords[wtm].at(index) == get_coord("g4"));
    wtm = !wtm;
    index = SeeMinAttacker(get_coord("b5"));
    assert(*coords[wtm].at(index) == get_coord("d7"));
    index = SeeMinAttacker(get_coord("e1"));
    assert(*coords[wtm].at(index) == get_coord("g2"));
    wtm = !wtm;
    index = SeeMinAttacker(get_coord("e2"));
    attacks[wtm][get_coord("e2")] ^= 1 << index;
    index = SeeMinAttacker(get_coord("e2"));
    assert(*coords[wtm].at(index) == get_coord("g4"));

    SetupPosition("1b3rk1/4n2p/6p1/5p2/6P1/3B2N1/6PP/5RK1 w - - 0 1");
    auto move = MoveFromStr("g4f5");
    assert(SEE_main(move) == piece_values[pawn]);
    SetupPosition("2b2rk1/4n2p/6p1/5p2/6P1/3B2N1/6PP/5RK1 w - - 0 1");
    move = MoveFromStr("g4f5");
    assert(SEE_main(move) == 0);
    SetupPosition("B2r2k1/4nb2/1N3n2/3R4/8/2N5/8/3R2K1 b - - 0 1");
    move = MoveFromStr("e7d5");
    assert(SEE_main(move) == piece_values[rook] - piece_values[knight]);
    move = MoveFromStr("f6d5");
    assert(SEE_main(move) == piece_values[rook] - piece_values[knight]);
    move = MoveFromStr("f7d5");
    assert(SEE_main(move) == piece_values[rook] - piece_values[bishop]);
    move = MoveFromStr("d8d5");
    assert(SEE_main(move) == 0);
    SetupPosition("3rk3/8/8/8/8/8/8/3QK3 w - - 0 1");
    move = MoveFromStr("d1d8");
    assert(SEE_main(move) == piece_values[rook] - piece_values[queen]);
    SetupPosition("q2b1rk1/5pp1/n6p/8/2Q5/7P/3P2P1/2R2BK1 w - - 0 1");
    move = MoveFromStr("c4a6");
    assert(SEE_main(move) == piece_values[knight]);
    assert(MakeMove("d2d3"));
    assert(MakeMove("h6h5"));
    assert(SEE_main(move) == piece_values[knight] - piece_values[queen]);
    SetupPosition("r1b3k1/4nr1p/5qp1/5p2/6P1/3B1RN1/5RPP/5QK1 w - - 0 1");
    move = MoveFromStr("g4f5");
    assert(SEE_main(move) == piece_values[pawn]);
    SetupPosition("r1b3k1/4nr1p/5qp1/5p2/6P1/3R1RN1/5BPP/5QK1 w - - 0 1");
    move = MoveFromStr("g4f5");
    assert(SEE_main(move) == 0);
    SetupPosition("5rk1/5q2/5r2/8/8/5R2/5R2/5QK1 w - - 0 1");
    move = MoveFromStr("f3f6");
    assert(SEE_main(move) == piece_values[rook]);
    SetupPosition("5rk1/5q2/5r2/8/8/5R2/5B2/5QK1 w - - 0 1");
    move = MoveFromStr("f3f6");
    assert(SEE_main(move) == 0);
    SetupPosition("3bn1k1/3np1p1/rpqr1p2/4P1PN/6NB/5R2/5RPP/5QK1 w - - 0 1");
    move = MoveFromStr("e5f6");
    assert(SEE_main(move) == piece_values[pawn]);
    move = MoveFromStr("g5f6");
    assert(SEE_main(move) == piece_values[pawn]);
    move = MoveFromStr("h5f6");
    assert(SEE_main(move) == 2*piece_values[pawn] - piece_values[knight]);
    SetupPosition("5b2/8/8/2k5/1p6/2P5/3B4/4K3 w - - 0 1");
    move = MoveFromStr("c3b4");
    assert(SEE_main(move) == piece_values[pawn]);
    SetupPosition("5b2/8/8/k7/1p6/2P5/3B4/4K3 w - - 0 1");
    move = MoveFromStr("c3b4");
    assert(SEE_main(move) == 0);
}
#endif // NDEBUG

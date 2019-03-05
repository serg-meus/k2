#include "movegen.h"





//--------------------------------
size_t k2movegen::GenMoves(move_c * const move_array,
                           const bool only_captures) const
{
    size_t move_cr = 0;
    
    if(!only_captures)
        GenCastles(move_array, &move_cr);

    for(auto it : coord_id[wtm])
    {
        const auto piece_id = it.second;
        const auto from_coord = coords[wtm][piece_id];
        const auto type = get_type(b[from_coord]);
        
        if(type == pawn)
        {
            GenPawnNonSilent(piece_id, move_array, &move_cr);
            if(!only_captures)
                GenPawnSilent(piece_id, move_array, &move_cr);
        }
        else if(is_slider[type])
            GenSliderMoves(only_captures, piece_id, from_coord, type,
                           move_array, &move_cr);
        else
            GenNonSliderMoves(only_captures, piece_id, from_coord, type,
                              move_array, &move_cr);
    }
    return KeepLegalMoves(move_array, move_cr);
}






//--------------------------------
void k2movegen::GenSliderMoves(const bool only_captures,
                               const piece_id_t piece_id,
                               const coord_t from_coord,
                               const piece_type_t type,
                               move_c * const move_array,
                               size_t * const move_cr) const
{
    const auto col0 = get_col(from_coord); 
    const auto row0 = get_row(from_coord);
    auto rmask = ray_mask_all[type];
    while(rmask)
    {
        const auto ray_id = __builtin_ctz(rmask);
        rmask ^= (1 << ray_id);
        const auto ray_len = directions[wtm][piece_id][ray_id];
        if(!ray_len)
            continue;
        auto col = col0 + ray_len*delta_col[ray_id];
        auto row = row0 + ray_len*delta_row[ray_id];
        auto to_coord = get_coord(col, row);
        const bool capture = is_dark(b[to_coord], wtm);
        if(capture || (!only_captures && b[to_coord] == empty_square))
            PushMove(move_array, move_cr, from_coord, to_coord,
                     capture ? is_capture : 0);
        if(only_captures)
            continue;
        while(true)
        {
            col -= delta_col[ray_id];
            row -= delta_row[ray_id];
            to_coord = get_coord(col, row);
            if(to_coord == from_coord)
                break;
            PushMove(move_array, move_cr, from_coord, to_coord, 0);
        }
    }
}





//--------------------------------
void k2movegen::GenNonSliderMoves(const bool only_captures,
                                  const piece_id_t piece_id,
                                  const coord_t from_coord,
                                  const piece_type_t type,
                                  move_c * const move_array,
                                  size_t * const move_cr) const
{
    const auto col0 = get_col(from_coord);
    const auto row0 = get_row(from_coord);
    auto rmask = ray_mask_all[type]; 
    while(rmask)
    {
        const auto ray_id = __builtin_ctz(rmask);
        rmask ^= (1 << ray_id);
        const auto ray_len = directions[wtm][piece_id][ray_id];
        if(!ray_len)
        	continue;
        const auto col = col0 + delta_col[ray_id];
        const auto row = row0 + delta_row[ray_id];
        const auto to_coord = get_coord(col, row);
        const auto empty = b[to_coord] == empty_square;
        const auto capture = (!empty && get_color(b[to_coord]) != wtm) ?
                    is_capture : 0;
        if((!only_captures && empty) || capture)
            PushMove(move_array, move_cr, from_coord, to_coord, capture);
    }
}





//--------------------------------
size_t k2movegen::KeepLegalMoves(move_c * const move_array,
                                 const size_t move_cr) const
{
    size_t new_move_cr = 0;
    for(size_t i = 0; i < move_cr; ++i)
        if(IsLegal(move_array[i]))
            move_array[new_move_cr++] = move_array[i];
    return new_move_cr;
}





//--------------------------------
void k2movegen::GenPawnSilent(const piece_id_t piece_id,
                              move_c * const move_array,
                              size_t * const moveCr) const
{
    const auto from_coord = coords[wtm][piece_id];
    const auto col = get_col(from_coord);
    const auto row = get_row(from_coord);
    if(row == (wtm ? max_row - 1 : 1))
        return;
    auto d_row = wtm ? 1 : -1;

    const auto to_coord = get_coord(col, row + d_row);
    if(b[to_coord] == empty_square)
        PushMove(move_array, moveCr, from_coord, to_coord, 0);

    const auto ini_row = wtm ? pawn_default_row : max_row - pawn_default_row;
    const auto to_coord2 = get_coord(col, row + 2*d_row);
    if(row == ini_row && b[to_coord2] == empty_square
            && b[to_coord] == empty_square)
        PushMove(move_array, moveCr, from_coord, to_coord2, 0);
}





//--------------------------------
void k2movegen::GenPawnNonSilent(const piece_id_t piece_id,
                                 move_c * const move_array,
                                 size_t * const moveCr) const
{
    const auto from_coord = coords[wtm][piece_id];
    const auto col = get_col(from_coord);
    const auto row = get_row(from_coord);

    move_flag_t promo_id_beg = 0, promo_id_end = 0;
    if(row == (wtm ? max_row - pawn_default_row : pawn_default_row))
    {
        promo_id_beg = is_promotion_to_queen;
        promo_id_end = is_promotion_to_bishop;
    }
    const auto d_row = wtm ? 1 : -1;
    for(auto promo_flag = promo_id_beg;
        promo_flag <= promo_id_end;
        ++promo_flag)
    {
        for(auto d_col: {-1, 1})
        {
            const auto to_coord = get_coord(col + d_col, row + d_row);
            const move_flag_t flag = is_capture | promo_flag;
            if(col_within(col + d_col) && b[to_coord] != empty_square
                    && get_color(b[to_coord]) != wtm)
                PushMove(move_array, moveCr, from_coord, to_coord, flag);
        }
        if(row != (wtm ? max_row - 1 : 1))
            continue;
        const auto to_coord = get_coord(col, row + d_row);
        if(b[to_coord] == empty_square)
            PushMove(move_array, moveCr, from_coord, to_coord, promo_flag);
    }
    const auto ep = k2chess::state[ply].en_passant_rights;
    const auto delta = ep - 1 - col;
    const auto ep_row = wtm ?
        max_row - pawn_default_row - pawn_long_move_length :
        pawn_default_row + pawn_long_move_length;
    if(ep && std::abs(delta) == 1 && row == ep_row)
        PushMove(move_array, moveCr, from_coord,
                 get_coord(col + delta, row + d_row),
                 is_capture | is_en_passant);
}





//--------------------------------
void k2movegen::GenCastles(move_c * const move_array,
                           size_t * const moveCr) const
{
    const castle_t msk = wtm ? 0x03 : 0x0C;
    const auto shft = wtm ? 0 : 2;
    const auto rights = (k2chess::state[ply].castling_rights & msk) >> shft;
    if(!rights)
        return;

    const auto k_coord = king_coord(wtm);
    if(attacks[!wtm][k_coord])
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
            PushMove(move_array, moveCr, k_coord, k_coord + delta[i], flag[i]);
    }
}





//--------------------------------
void k2movegen::AppriceMoves(move_c * const move_array, const size_t move_cr,
                             const move_c best_move) const
{
    history_t min_history = std::numeric_limits<history_t>::max();
    history_t max_history = 0;

    for(size_t i = 0; i < move_cr; ++i)
    {
        auto move = &move_array[i];
        const auto fr_coord = move->from_coord;
        const auto to_coord = move->to_coord;
        const auto type = get_type(b[fr_coord]);

        if(*move == best_move)
            move->priority = move_from_hash;
        else if(move->flag == 0 || move->flag & is_castle)
        {
            if(*move == killers[ply][0])
                move->priority = first_killer;
            else if(*move == killers[ply][1])
                move->priority = second_killer;
            else
            {
                const auto h = history[wtm][type - 1][to_coord];
                if(h > max_history)
                    max_history = h;
                if(h < min_history)
                    min_history = h;
                move->priority = AppriceSilentMoves(type, fr_coord, to_coord);
            }
        }
        else
            move->priority = AppriceCaptures(*move);
    }
    AppriceHistory(move_array, move_cr, min_history, max_history);
}





//-----------------------------
void k2movegen::AppriceHistory(move_c * const move_array, size_t move_cr,
                               history_t min_history,
                               history_t max_history) const
{
    for(size_t i = 0; i < move_cr; ++i)
    {
        auto move = &move_array[i];
        if(move->priority >= second_killer || (move->flag & is_capture))
            continue;
        const auto type = get_type(b[move->from_coord]);
        auto hist = history[wtm][type - 1][move->to_coord];
        if(hist > 3)
        {
            hist -= min_history;
            hist = 64*hist / (max_history - min_history + 1);
            hist += 128;
            move->priority = hist;
        }
    }
}





//-----------------------------
size_t k2movegen::AppriceSilentMoves(const piece_type_t type,
                                     const coord_t from_coord,
                                     const coord_t to_coord) const
{
    auto y = get_row(to_coord);
    const auto x = get_col(to_coord);
    auto y0 = get_row(from_coord);
    const auto x0 = get_col(from_coord);
    if(wtm)
    {
        y = max_row - y;
        y0 = max_row - y0;
    }
    const auto pstVal = pst[type - 1][0][y][x] -
            pst[type - 1][0][y0][x0];
    return 96 + pstVal/2;
}





//-----------------------------
size_t k2movegen::AppriceCaptures(const move_c move) const
{
    auto src = values[get_type(b[move.from_coord])]/10;
    auto dst = values[get_type(b[move.to_coord])]/10;
    if(move.flag & is_en_passant)
        dst = values[pawn]/10;
    else if(!(move.flag & is_capture))
        dst = 0;
    if(dst && dst - src <= 0)
    {
        dst = StaticExchangeEval(move)/10;
        src = 0;
    }
    eval_t prms[] = {0, 120, 40, 60, 40};
    if(dst <= 120 && (move.flag & is_promotion))
        dst += prms[move.flag & is_promotion];

    auto ans = dst >= src ? dst - src/16 : dst - src;
    return dst >= src ? 200 + ans/8 : -ans/2;
}





//-----------------------------
void k2movegen::ProcessSeeBatteries(const coord_t to_coord,
                                    const coord_t attacker_coord,
                                    const bool stm,
                                    attack_t *att) const
{
    auto mask = attacks[stm][attacker_coord] & slider_mask[stm];
    while(mask)
    {
        const auto piece_id = __builtin_ctz(mask);
        mask ^= (1 << piece_id);
        const auto second_coord = coords[stm][piece_id];
        if(!IsOnRay(attacker_coord, second_coord, to_coord))
            continue;
        if(!IsSliderAttack(attacker_coord, second_coord))
            continue;
        att[stm] |= 1 << piece_id;
        return;
    }
}





//-----------------------------
k2chess::eval_t k2movegen::SEE(const coord_t to_coord, const eval_t fr_value,
                               eval_t val, bool stm, attack_t *att) const
{
    const auto piece_id = SeeMinAttacker(att[stm]);
    if(piece_id == -1U)
        return -val;
    att[stm] ^= (1 << piece_id);
    const auto from_coord = coords[stm][piece_id];
    const auto type = get_type(b[from_coord]);
    if(type != knight)
        ProcessSeeBatteries(to_coord, from_coord, stm, att);

    val -= fr_value;
    const auto tmp1 = -val;
    const auto tmp2 = -SEE(to_coord, values[type], -val, !stm, att);
    val = std::min(tmp1, tmp2);

    return val;
}





//-----------------------------
size_t k2movegen::SeeMinAttacker(const attack_t att) const
{
    if(!att)
        return -1U;
    return __builtin_ctz(att);
}





//-----------------------------
k2chess::eval_t k2movegen::StaticExchangeEval(const move_c move) const
{
    const auto fr_piece = b[move.from_coord];
    const auto to_piece = b[move.to_coord];
    const auto fr_type = get_type(fr_piece);
    const auto to_type = get_type(to_piece);
    const auto src = values[fr_type];
    const auto dst = (move.flag & is_capture) ? values[to_type] : 0;

    attack_t att[sides];
    att[wtm] = attacks[wtm][move.to_coord];
    att[!wtm] = attacks[!wtm][move.to_coord];

    att[wtm] ^= 1 << find_piece_id[move.from_coord];
    if(fr_type != knight)
        ProcessSeeBatteries(move.to_coord, move.from_coord, wtm, att);
    auto see_score = -SEE(move.to_coord, src, dst, !wtm, att);
    see_score = std::min(dst, see_score);

    return see_score;
}




#ifndef NDEBUG
//--------------------------------
size_t k2movegen::test_gen_pawn(const char* str_coord,
                                bool only_captures) const
{
    move_c move_array[move_array_size];
    size_t move_cr = 0;

    size_t piece_id = find_piece_id[get_coord(str_coord)];
    GenPawnNonSilent(piece_id, move_array, &move_cr);
    if(!only_captures)
        GenPawnSilent(piece_id, move_array, &move_cr);
    for(size_t i = 0; i < move_cr; ++i)
        if(!IsLegal(move_array[i]))
            return -1U;
    return move_cr;
}





//--------------------------------
size_t k2movegen::test_gen_castles() const
{
    move_c move_array[move_array_size];
    size_t move_cr = 0;

    GenCastles(move_array, &move_cr);
    for(size_t i = 0; i < move_cr; ++i)
        if(!IsLegal(move_array[i]))
            return -1U;
    return move_cr;
}





//--------------------------------
size_t k2movegen::test_gen_moves(const bool only_captures) const
{
    move_c move_array[move_array_size];
    size_t move_cr = GenMoves(move_array, only_captures);
    for(size_t i = 0; i < move_cr; ++i)
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

    TakebackMove();
    TakebackMove();

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
    SetupPosition("8/8/8/8/8/8/6k1/4K2R w K -");
    assert(test_gen_castles() == 0);

    SetupPosition(
        "r3k1r1/ppp1bpp1/5n1p/2Ppp3/4P2q/Q1NP4/PP3PPP/R1B1K2R w KQq d6");
    assert(test_gen_moves(all_moves) == 34);
    assert(MakeMove("a1b1"));
    assert(test_gen_moves(all_moves) == 37);
    assert(MakeMove("d5d4"));
    assert(MakeMove("c5c6"));
    assert(MakeMove("d4c3"));
    assert(MakeMove("c6b7"));
    assert(MakeMove("c3b2"));
    assert(test_gen_moves(only_captures) == 14);

    SetupPosition("3k4/3b4/8/1Q5p/6B1/1r4N1/4p1nR/4K3 w - - 0 1");
    auto coord = get_coord("e2");
    auto piece_id = SeeMinAttacker(attacks[wtm][coord]);
    assert(coords[wtm][piece_id] == get_coord("g3"));
    coord = get_coord("h5");
    piece_id = SeeMinAttacker(attacks[wtm][coord]);
    assert(coords[wtm][piece_id] == get_coord("g3"));
    coord = get_coord("b3");
    piece_id = SeeMinAttacker(attacks[wtm][coord]);
    assert(coords[wtm][piece_id] == get_coord("b5"));
    coord = get_coord("g2");
    piece_id = SeeMinAttacker(attacks[wtm][coord]);
    assert(coords[wtm][piece_id] == get_coord("h2"));
    coord = get_coord("d7");
    piece_id = SeeMinAttacker(attacks[wtm][coord]);
    assert(coords[wtm][piece_id] == get_coord("g4"));
    wtm = !wtm;
    coord = get_coord("b5");
    piece_id = SeeMinAttacker(attacks[wtm][coord]);
    assert(coords[wtm][piece_id] == get_coord("d7"));
    coord = get_coord("e1");
    piece_id = SeeMinAttacker(attacks[wtm][coord]);
    assert(coords[wtm][piece_id] == get_coord("g2"));
    wtm = !wtm;
    coord = get_coord("e2");
    piece_id = SeeMinAttacker(attacks[wtm][coord]);
    attacks[wtm][get_coord("e2")] ^= 1 << piece_id;
    coord = get_coord("e2");
    piece_id = SeeMinAttacker(attacks[wtm][coord]);
    assert(coords[wtm][piece_id] == get_coord("g4"));

    SetupPosition("1b3rk1/4n2p/6p1/5p2/6P1/3B2N1/6PP/5RK1 w - - 0 1");
    auto move = MoveFromStr("g4f5");
    assert(StaticExchangeEval(move) == values[pawn]);
    SetupPosition("2b2rk1/4n2p/6p1/5p2/6P1/3B2N1/6PP/5RK1 w - - 0 1");
    move = MoveFromStr("g4f5");
    assert(StaticExchangeEval(move) == 0);
    SetupPosition("B2r2k1/4nb2/1N3n2/3R4/8/2N5/8/3R2K1 b - - 0 1");
    move = MoveFromStr("e7d5");
    assert(StaticExchangeEval(move) == values[rook] - values[knight]);
    move = MoveFromStr("f6d5");
    assert(StaticExchangeEval(move) == values[rook] - values[knight]);
    move = MoveFromStr("f7d5");
    assert(StaticExchangeEval(move) == values[rook] - values[bishop]);
    move = MoveFromStr("d8d5");
    assert(StaticExchangeEval(move) == 0);
    SetupPosition("3rk3/8/8/8/8/8/8/3QK3 w - - 0 1");
    move = MoveFromStr("d1d8");
    assert(StaticExchangeEval(move) == values[rook] - values[queen]);
    SetupPosition("q2b1rk1/5pp1/n6p/8/2Q5/7P/3P2P1/2R2BK1 w - - 0 1");
    move = MoveFromStr("c4a6");
    assert(StaticExchangeEval(move) == values[knight]);
    assert(MakeMove("d2d3"));
    assert(MakeMove("h6h5"));
    assert(StaticExchangeEval(move) == values[knight] - values[queen]);
    SetupPosition("r1b3k1/4nr1p/5qp1/5p2/6P1/3B1RN1/5RPP/5QK1 w - - 0 1");
    move = MoveFromStr("g4f5");
    assert(StaticExchangeEval(move) == values[pawn]);
    SetupPosition("r1b3k1/4nr1p/5qp1/5p2/6P1/3R1RN1/5BPP/5QK1 w - - 0 1");
    move = MoveFromStr("g4f5");
    assert(StaticExchangeEval(move) == 0);
    SetupPosition("5rk1/5q2/5r2/8/8/5R2/5R2/5QK1 w - - 0 1");
    move = MoveFromStr("f3f6");
    assert(StaticExchangeEval(move) == values[rook]);
    SetupPosition("5rk1/5q2/5r2/8/8/5R2/5B2/5QK1 w - - 0 1");
    move = MoveFromStr("f3f6");
    assert(StaticExchangeEval(move) == 0);
    SetupPosition("3bn1k1/3np1p1/rpqr1p2/4P1PN/6NB/5R2/5RPP/5QK1 w - - 0 1");
    move = MoveFromStr("e5f6");
    assert(StaticExchangeEval(move) == values[pawn]);
    move = MoveFromStr("g5f6");
    assert(StaticExchangeEval(move) == values[pawn]);
    move = MoveFromStr("h5f6");
    assert(StaticExchangeEval(move) == 2*values[pawn] - values[knight]);
    SetupPosition("5b2/8/8/2k5/1p6/2P5/3B4/4K3 w - - 0 1");
    move = MoveFromStr("c3b4");
    assert(StaticExchangeEval(move) == values[pawn]);
    SetupPosition("5b2/8/8/k7/1p6/2P5/3B4/4K3 w - - 0 1");
    move = MoveFromStr("c3b4");
    assert(StaticExchangeEval(move) == 0);
    SetupPosition("3r2k1/6p1/2q2r1p/pp1p1p2/3P4/1QN1PBP1/PP3P1P/6K1 w - -");
    move = MoveFromStr("c3d5");
    assert(StaticExchangeEval(move) == values[pawn]);
}
#endif // NDEBUG

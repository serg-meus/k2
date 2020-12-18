#include "movegen.h"





//--------------------------------
bool k2movegen::GetNextMove(std::vector<move_c> &moves,
                            move_c tt_move,
                            const bool only_captures)
{
    if(moves.size() == 0 && tt_move.flag != not_a_move &&
            GenHashMove(moves, tt_move))
        return true;
    const bool tt_move_probed = moves.size() == 1 && moves[0] == tt_move;
    if(tt_move_probed && tt_move.priority == move_one_reply)
        return false;
    if(moves.size() == 0 || tt_move_probed)
        GenerateAndAppriceAllMoves(tt_move, only_captures);

    if(GetNextLegalMove())
        moves.push_back(pseudo_legal_pool[ply][pseudo_legal_cr[ply] - 1]);
    else
        return false;

    const auto tmp = pseudo_legal_cr[ply];
    if(!only_captures && moves.size() == 1 && !GetNextLegalMove())
        moves[0].priority = move_one_reply;
    pseudo_legal_cr[ply] = tmp;
    return true;
}





//--------------------------------
void k2movegen::GenerateAndAppriceAllMoves(const move_c tt_move,
                                           const bool only_captures)
{
    auto &moves = pseudo_legal_pool[ply];
    moves.clear();
    GenPseudoLegalMoves(moves, only_captures);
    AppriceMoves(moves, only_captures);
    if(tt_move.flag != not_a_move)
    {
        const auto it = std::find(moves.begin(), moves.end(), tt_move);
        std::swap(*it, moves.back());
        moves.pop_back();
    }
    pseudo_legal_cr[ply] = 0;
}





//--------------------------------
bool k2movegen::GetNextLegalMove()
{
    auto &cr = pseudo_legal_cr[ply];
    auto &moves = pseudo_legal_pool[ply];
    while(true)
    {
        size_t max_priority_index = GetMaxPriorityMoveIndex(moves, cr);
        if(max_priority_index == moves.size())
            return false;
        if(IsLegal(moves[max_priority_index]))
        {
            std::swap(moves[max_priority_index], moves[cr++]);
            return true;
        }
        std::swap(moves[max_priority_index], moves.back());
        moves.pop_back();
    }
    return false;
}





//--------------------------------
size_t k2movegen::GetMaxPriorityMoveIndex(const std::vector<move_c> & moves,
                                          const size_t &cr)
{
    priority_t max_priority = 0;
    size_t max_index = cr;
    for(size_t i = cr; i < moves.size(); ++i)
        if(moves[i].priority > max_priority)
        {
            max_priority = moves[i].priority;
            max_index = i;
        }
    return max_index;
}





//--------------------------------
bool k2movegen::GenHashMove(std::vector<move_c> &moves,
                            move_c &tt_move) const
{
    if(!IsPseudoLegal(tt_move) || !IsLegal(tt_move))
    {
        tt_move.flag = not_a_move;
        return false;
    }
    moves.push_back(tt_move);
    return true;
}





//--------------------------------
void k2movegen::GenLegalMoves(std::vector<move_c> &moves,
                              std::vector<move_c> &pseudo_legal_moves,
                              const bool only_captures) const
{
    pseudo_legal_moves.clear();
    GenPseudoLegalMoves(pseudo_legal_moves, only_captures);
    for(auto ps: pseudo_legal_moves)
        if(IsLegal(ps))
            moves.push_back(ps);
}




//--------------------------------
void k2movegen::GenPseudoLegalMoves(std::vector<move_c> &moves,
                                    const bool only_captures) const
{
    if(!only_captures)
        GenCastles(moves);
    auto mask = exist_mask[wtm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        const auto from_coord = coords[wtm][piece_id];
        const auto type = get_type(b[from_coord]);

        if(type == pawn)
        {
            GenPawnNonSilent(moves, piece_id);
            if(!only_captures)
                GenPawnSilent(moves, piece_id);
        }
        else if(is_slider[type])
            GenSliderMoves(moves, only_captures, piece_id, from_coord, type);
        else
            GenNonSliderMoves(moves, only_captures, piece_id, from_coord, type);
    }
}





//--------------------------------
void k2movegen::GenSliderMoves(std::vector<move_c> &moves,
                               const bool only_captures,
                               const piece_id_t piece_id,
                               const coord_t from_coord,
                               const piece_type_t type) const
{
    const auto col0 = get_col(from_coord); 
    const auto row0 = get_row(from_coord);
    auto rmask = ray_mask_all[type];
    while(rmask)
    {
        const auto ray_id = lower_bit_num(rmask);
        rmask ^= (1 << ray_id);
        const auto ray_len = directions[wtm][piece_id][ray_id];
        if(!ray_len)
            continue;
        auto col = col0 + ray_len*delta_col[ray_id];
        auto row = row0 + ray_len*delta_row[ray_id];
        auto to_coord = get_coord(col, row);
        const move_flag_t capt = is_dark(b[to_coord], wtm) ? is_capture : 0;
        if(capt || (!only_captures && b[to_coord] == empty_square))
            moves.push_back({from_coord, to_coord, capt, 0});
        if(only_captures)
            continue;
        while(true)
        {
            col -= delta_col[ray_id];
            row -= delta_row[ray_id];
            to_coord = get_coord(col, row);
            if(to_coord == from_coord)
                break;
            moves.push_back({from_coord, to_coord, 0, 0});
        }
    }
}





//--------------------------------
void k2movegen::GenNonSliderMoves(std::vector<move_c> &moves,
                                  const bool only_captures,
                                  const piece_id_t piece_id,
                                  const coord_t from_coord,
                                  const piece_type_t type) const
{
    const auto col0 = get_col(from_coord);
    const auto row0 = get_row(from_coord);
    auto rmask = ray_mask_all[type]; 
    while(rmask)
    {
        const auto ray_id = lower_bit_num(rmask);
        rmask ^= (1 << ray_id);
        const auto ray_len = directions[wtm][piece_id][ray_id];
        if(!ray_len)
            continue;
        const auto col = col0 + delta_col[ray_id];
        const auto row = row0 + delta_row[ray_id];
        const auto to_coord = get_coord(col, row);
        const auto empty = b[to_coord] == empty_square;
        const move_flag_t capture = (!empty && get_color(b[to_coord]) != wtm) ?
                    is_capture : 0;
        if((!only_captures && empty) || capture)
            moves.push_back({from_coord, to_coord, capture, 0});
    }
}





//--------------------------------
void k2movegen::GenPawnSilent(std::vector<move_c> &moves,
                              const piece_id_t piece_id) const
{
    const auto from_coord = coords[wtm][piece_id];
    const auto col = get_col(from_coord);
    const auto row = get_row(from_coord);
    if(row == (wtm ? max_row - 1 : 1))
        return;
    auto d_row = wtm ? 1 : -1;

    const auto to_coord = get_coord(col, row + d_row);
    if(b[to_coord] == empty_square)
        moves.push_back({from_coord, to_coord, 0, 0});

    const auto ini_row = wtm ? pawn_default_row : max_row - pawn_default_row;
    const auto to_coord2 = get_coord(col, row + 2*d_row);
    if(row == ini_row && b[to_coord2] == empty_square
            && b[to_coord] == empty_square)
        moves.push_back({from_coord, to_coord2, 0, 0});
}





//--------------------------------
void k2movegen::GenPawnNonSilent(std::vector<move_c> &moves,
                                 const piece_id_t piece_id) const
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
                moves.push_back({from_coord, to_coord, flag, 0});
        }
        if(row != (wtm ? max_row - 1 : 1))
            continue;
        const auto to_coord = get_coord(col, row + d_row);
        if(b[to_coord] == empty_square)
            moves.push_back({from_coord, to_coord, promo_flag, 0});
    }
    const auto ep = k2chess::state[ply].en_passant_rights;
    const auto delta = ep - 1 - col;
    const auto ep_row = wtm ?
        max_row - pawn_default_row - pawn_long_move_length :
        pawn_default_row + pawn_long_move_length;
    if(ep && std::abs(delta) == 1 && row == ep_row)
        moves.push_back({from_coord, get_coord(col + delta, row + d_row),
                         static_cast<move_flag_t>(is_capture | is_en_passant),
                         0});
}





//--------------------------------
void k2movegen::GenCastles(std::vector<move_c> &moves) const
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
            moves.push_back({k_coord, static_cast<coord_t>(k_coord + delta[i]),
                             flag[i], 0});
    }
}





//--------------------------------
void k2movegen::AppriceMoves(std::vector<move_c> &moves,
                             const bool only_captures) const
{
    history_t min_history = std::numeric_limits<history_t>::max();
    history_t max_history = 0;

    for(auto &move: moves)
    {
        const auto fr_coord = move.from_coord;
        const auto to_coord = move.to_coord;
        const auto type = get_type(b[fr_coord]);

        if(!move.flag || move.flag & is_castle)
        {
            if(move == killers[ply][0])
                move.priority = first_killer;
            else if(move == killers[ply][1])
                move.priority = second_killer;
            else
            {
                const auto h = history[wtm][type - 1][to_coord];
                if(h > max_history)
                    max_history = h;
                if(h < min_history)
                    min_history = h;
                move.priority = AppriceSilentMove(type, fr_coord, to_coord);
            }
        }
        else
            move.priority = AppriceCapture(move);
    }
    if(!only_captures)
        AppriceHistory(moves, min_history, max_history);
}





//-----------------------------
void k2movegen::AppriceHistory(std::vector<move_c> &moves,
                               const history_t min_history,
                               const history_t max_history) const
{
    for(auto &move: moves)
    {
        if(!move.priority || move.priority >= second_killer ||
                (move.flag & is_capture))
            continue;
        const auto type = get_type(b[move.from_coord]);
        auto hist = history[wtm][type - 1][move.to_coord];
        assert(hist >= min_history);
        if(hist > 3)
        {
            hist -= min_history;
            hist = 128 + 64*hist/(max_history - min_history + 1);
            assert(hist <= std::numeric_limits<priority_t>::max());
            move.priority = hist;
        }
    }
}





//-----------------------------
size_t k2movegen::AppriceSilentMove(const piece_type_t type,
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
    const auto pstVal = pst[type - 1][y][x].mid - pst[type - 1][y0][x0].mid;
    return 96 + pstVal/3;
}





//-----------------------------
size_t k2movegen::AppriceCapture(const move_c move) const
{
    eval_t src = values[get_type(b[move.from_coord])]/10;
    eval_t dst = values[get_type(b[move.to_coord])]/10;
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

    size_t ans = dst >= src ? first_killer + 1 + (dst - src/4)/8 :
                              bad_captures + (dst - src)/2;
    assert(ans > 0);
    return ans;
}





//-----------------------------
void k2movegen::ProcessSeeBatteries(const coord_t to_coord,
                                    const coord_t attacker_coord,
                                    const bool stm,
                                    attack_t (&att)[sides]) const
{
    auto mask = attacks[stm][attacker_coord] & slider_mask[stm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
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
k2chess::piece_val_t k2movegen::SEE(const coord_t to_coord,
                                    const piece_val_t fr_value,
                                    piece_val_t val,
                                    bool stm, attack_t (&att)[sides]) const
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
    return lower_bit_num(att);
}





//-----------------------------
k2chess::piece_val_t k2movegen::StaticExchangeEval(const move_c move) const
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
    std::vector<move_c> moves;
    size_t piece_id = find_piece_id[get_coord(str_coord)];
    GenPawnNonSilent(moves, piece_id);
    if(!only_captures)
        GenPawnSilent(moves, piece_id);
    for(auto move: moves)
        if(!IsLegal(move))
            return -1U;
    return moves.size();
}





//--------------------------------
size_t k2movegen::test_gen_castles() const
{
    std::vector<move_c> moves;
    GenCastles(moves);
    for(auto &move: moves)
        if(!IsLegal(move))
            return -1U;
    return moves.size();
}





//--------------------------------
size_t k2movegen::test_gen_moves(const bool only_captures) const
{
    std::vector<move_c> moves;
    GenPseudoLegalMoves(moves, only_captures);
    for(auto move: moves)
        if(!IsPseudoLegal(move))
            return -1U;
    size_t legals = 0;
    for(auto move: moves)
        if(IsLegal(move))
            legals++;
    return legals;
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

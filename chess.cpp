#include "chess.h"





//--------------------------------
k2chess::k2chess() :
    ray_mask_all{0, 0x00ff, 0x00ff, 0x000f, 0x00f0, 0xff00, 0x00f0},
    delta_col{0, 1,  0, -1, 1,  1, -1, -1, 1, 2,  2,  1, -1, -2, -2, -1},
    delta_row{1, 0, -1,  0, 1, -1, -1,  1, 2, 1, -1, -2, -2, -1,  1,  2},
    is_slider{0, false, true, true, true, false, false},
    values{0, 15000, 1200, 600, 410, 400, 100}
{
    cv = cur_moves;
    memset(b_state, 0, sizeof(b_state));
    state = &b_state[prev_states];
    max_ray_length = std::max(static_cast<size_t>(board_height),
                              static_cast<size_t>(board_width));
    SetupPosition(start_position);
}





//--------------------------------
void k2chess::UpdateAttacks(const bool stm, const move_c move)
{
    auto mask = update_mask[stm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        UpdatePieceAttacks(stm, piece_id, move);
    }
}





//--------------------------------
void k2chess::UpdatePieceAttacks(const bool stm, const piece_id_t piece_id,
                                 const move_c move)
{
    const bool update_all = move.flag == not_a_move;
    const auto coord = coords[stm][piece_id];
    const bool is_move = (coord == move.to_coord ||
                          IsCastledRook(stm, coord, move));
    if(!update_all && is_move)
        ClearPieceAttacks(stm, piece_id);

    auto rmask = update_all ? (1 << max_rays) - 1 :
                              GetRayMask(stm, coord, move);
    while(rmask)
    {
        const auto ray_id = lower_bit_num(rmask);
        rmask ^= (1 << ray_id);
        const auto old_c = (update_all || is_move) ? 0 :
                done_directions[ply - 1][stm][piece_id][ray_id];
        const auto new_c = directions[stm][piece_id][ray_id];
        if(old_c != new_c)
            UpdateRayAttacks(stm, piece_id, ray_id, old_c, new_c);
    }
}





//--------------------------------
void k2chess::UpdateRayAttacks(const bool stm, const piece_id_t piece_id,
                               const ray_mask_t ray_id, const coord_t old_cr,
                               const coord_t new_cr)
{
    const auto coord = coords[stm][piece_id];
    const auto col = get_col(coord);
    const auto row = get_row(coord);
    const auto d_col = delta_col[ray_id];
    const auto d_row = delta_row[ray_id];

    if(new_cr > old_cr)
        for(auto i = old_cr + 1; i <= new_cr; ++i)
            set_bit(stm, col + i*d_col, row + i*d_row, piece_id);
    else
        for(auto i = new_cr + 1; i <= old_cr; ++i)
            clear_bit(stm, col + i*d_col, row + i*d_row, piece_id);

    sum_directions[stm][piece_id] += new_cr - old_cr;
}





//--------------------------------
void k2chess::UpdateDirections(const bool stm, const move_c move)
{
    auto mask = update_mask[stm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        UpdatePieceDirections(stm, piece_id, move);
    }

}





//--------------------------------
void k2chess::UpdatePieceDirections(const bool stm, const piece_id_t piece_id,
                                    const move_c move)
{
    const auto coord = coords[stm][piece_id];
    const auto type = get_type(b[coord]);
    if(UpdateCapturingDirections(stm, piece_id, coord, type, move))
        return;
    auto rmask = ray_mask_all[type];
    if(type == pawn)
        rmask &= (stm ? pawn_mask_white : pawn_mask_black);
    if(coord == move.to_coord || IsCastledRook(stm, coord, move))
        UpdateMovingDirections(stm, piece_id, type, coord, rmask);
    else
        UpdateOrdinaryDirections(stm, piece_id, coord, move, rmask);
}





//--------------------------------
bool k2chess::UpdateCapturingDirections(const bool stm,
                                        const piece_id_t piece_id,
                                        const coord_t coord,
                                        const piece_type_t type,
                                        const move_c move)
{
    const bool capturing_move = (move.flag & is_capture) &&
            get_color(b[move.to_coord]) != stm &&
            (type == 0 || coord == move.to_coord);
    if(!capturing_move)
        return false;
    for(auto ray_id = 0; ray_id < max_rays; ++ray_id)
        directions[stm][piece_id][ray_id] = 0;
    return true;
}





//--------------------------------
void k2chess::UpdateMovingDirections(const bool stm, const piece_id_t piece_id,
                                     const piece_type_t type,
                                     const coord_t coord,
                                     ray_mask_t rmask)
{
    while(rmask)
    {
        const auto ray_id = lower_bit_num(rmask);
        rmask ^= (1 << ray_id);
        const auto cr = GetRayAttacks(coord, ray_id, is_slider[type]);
        directions[stm][piece_id][ray_id] = cr;
    }
}





//--------------------------------
void k2chess::UpdateOrdinaryDirections(const bool stm,
                                       const piece_id_t piece_id,
                                       const coord_t coord, const move_c move,
                                       ray_mask_t rmask)
{
    const auto en_pass = move.flag & is_en_passant;
    while(rmask)
    {
        const auto ray_id = lower_bit_num(rmask);
        rmask ^= (1 << ray_id);
        bool move_from_ray = IsOnRayMask(stm, coord, move.from_coord,
                                         piece_id, ray_id, en_pass);
        bool move_to_ray = IsOnRayMask(stm, coord, move.to_coord,
                                       piece_id, ray_id, en_pass);
        if(move_from_ray)
            UpdateFromRayDirections(stm, piece_id, coord, ray_id);
        if(move_to_ray)
            UpdateToRayDirections(stm, piece_id, coord, move, ray_id);
    }
}





//--------------------------------
void k2chess::InitDirections(const bool stm)
{
    memset(directions[stm], 0, sizeof(directions[0]));
    memset(attacks[stm], 0, sizeof(attacks[0]));
    memset(sum_directions[stm], 0, sizeof(sum_directions[0]));

    auto mask = exist_mask[stm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        const auto coord = coords[stm][piece_id];
        const auto type = get_type(b[coord]);
        auto rmask = ray_mask_all[type];
        if(type == pawn)
            rmask &= (stm ? pawn_mask_white : pawn_mask_black);
        while(rmask)
        {
            const auto ray_id = lower_bit_num(rmask);
            rmask ^= (1 << ray_id);
            const auto cr = GetRayAttacks(coord, ray_id, is_slider[type]);
            directions[stm][piece_id][ray_id] = cr;
        }
    }
}





//--------------------------------
void k2chess::UpdateFromRayDirections(const bool stm,
                                      const piece_id_t piece_id,
                                      const coord_t coord,
                                      const piece_id_t ray_id)
{
    auto ans = directions[stm][piece_id][ray_id];
    const auto delta = delta_col[ray_id] + board_width*delta_row[ray_id];
    const auto cr = GetRayAttacks(coord + ans*delta, ray_id, true);
    ans += cr;
    directions[stm][piece_id][ray_id] = ans;
}





//--------------------------------
void k2chess::UpdateToRayDirections(const bool stm, const piece_id_t piece_id,
                                    const coord_t coord, const move_c move,
                                    const piece_id_t ray_id)
{
    const auto d_col = get_col(coord) - get_col(move.to_coord);
    const auto d_row = get_row(coord) - get_row(move.to_coord);
    directions[stm][piece_id][ray_id] =
            std::max(std::abs(d_col), std::abs(d_row));
}





//--------------------------------
k2chess::coord_t k2chess::GetRayAttacks(const coord_t coord, piece_id_t ray_id,
                                        const bool slider) const
{
    coord_t attack_counter = 0;
    auto col = get_col(coord), row = get_row(coord);
    piece_t cur_square;
    while(attack_counter < max_ray_length)
    {
        col += delta_col[ray_id];
        row += delta_row[ray_id];
        if(!col_within(col) || !row_within(row))
            break;
        attack_counter++;
        cur_square = b[get_coord(col, row)];
        if(!slider || cur_square != empty_square)
            break;
    }
    return attack_counter;
}





//--------------------------------
void k2chess::UpdatePieceMasks(move_c move)
{
    update_mask[black] = 0;
    update_mask[white] = 0;
    assert(find_piece_id[move.to_coord] != piece_not_found);
    update_mask[!wtm] |= (1 << find_piece_id[move.to_coord]);
    if(move.flag & is_castle)
        UpdatePieceMasksForCastle(move);
    if(move.flag & is_capture)
        update_mask[wtm] |= (1 << state[ply].captured_id);

    for(auto stm : {black, white})
    {
        const auto mask = slider_mask[stm];
        update_mask[stm] |= (attacks[stm][move.from_coord] & mask);
        update_mask[stm] |= (attacks[stm][move.to_coord] & mask);
        if(move.flag & is_en_passant)
        {
            const auto delta = wtm ? board_width : -board_width;
            update_mask[stm] |= (attacks[stm][move.to_coord + delta] & mask);
        }
    }
}





//--------------------------------
bool k2chess::IsCastledRook(const bool stm, const coord_t coord,
                            const move_c move) const
{
    if(!(move.flag & is_castle) || b[coord] != (stm ? white_rook : black_rook))
        return false;
    const auto d_coord = std::abs(king_coord(stm) - coord);

    return d_coord == 1;
}





//--------------------------------
void k2chess::UpdatePieceMasksForCastle(move_c move)
{
    update_mask[!wtm] |= (1 << state[ply].castled_rook_id);
    const auto from_col = move.flag == is_right_castle ? max_col : 0;
    const auto from_row = wtm ? max_row : 0;
    const auto rook_from_coord = get_coord(from_col, from_row);
    for(auto stm : {black, white})
    {
        update_mask[stm] |= attacks[stm][rook_from_coord] & slider_mask[stm];
        update_mask[stm] |=
                attacks[stm][coords[!wtm][state[ply].castled_rook_id]] &
                slider_mask[stm];

    }
}





//--------------------------------
bool k2chess::IsDiscoveredAttack(const move_c move) const
{
    auto mask = attacks[!wtm][move.from_coord] & slider_mask[!wtm];
    const auto k_coord = king_coord(wtm);
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        const auto attacker_coord = coords[!wtm][piece_id];
        if(!IsOnRay(move.from_coord, k_coord, attacker_coord))
            continue;
        if(IsOnRay(move.to_coord, k_coord, attacker_coord))
            continue;
        if(!IsSliderAttack(move.from_coord, k_coord))
            continue;
        return true;
    }
    return false;
}





//--------------------------------
bool k2chess::IsDiscoveredEnPassant(const bool stm, const move_c move) const
{
    const bool enps = move.flag & is_en_passant;
    const auto enps_taken_coord = get_coord(state[ply].en_passant_rights - 1,
                                   get_row(move.from_coord));
    for(auto ray_id = 0; ray_id < 8; ++ray_id)
    {
        int col = get_col(king_coord(stm));
        int row = get_row(king_coord(stm));
        for(auto i = 0; i < max_ray_length; ++i)
        {
            col += delta_col[ray_id];
            row += delta_row[ray_id];
            if(!col_within(col) || !row_within(row))
                break;
            const auto coord = get_coord(col, row);
            if(enps && coord == move.to_coord)
                break;
            if(b[coord] == empty_square)
                continue;
            if((move.flag & is_en_passant) &&
                    (coord == enps_taken_coord || coord == move.from_coord))
                continue;
            if(is_light(b[coord], stm))
                break;
            const auto type = get_type(b[coord]);
            if(type == queen)
                return true;
            const auto ray_bishop = std::abs(delta_col[ray_id]) ==
                    std::abs(delta_row[ray_id]);
            if((ray_bishop && type == bishop) ||
                    (!ray_bishop && type == rook))
                return true;
            break;
        }
    }
    return false;
}





//--------------------------------
void k2chess::ClearPieceAttacks(const bool stm, piece_id_t piece_id)
{
    for(auto &sq : attacks[stm])
        sq &= ~(1 << piece_id);
    sum_directions[stm][piece_id] = 0;
}





//--------------------------------
bool k2chess::InitPieceLists(bool stm)
{
    typedef std::pair<eval_t, piece_id_t> the_pair;
    exist_mask[stm] = 0;
    memset(coords[stm], 0, sizeof(coords[0]));

    std::vector<std::pair<eval_t, coord_t>> tmp_vect;
    tmp_vect.reserve(max_pieces_one_side);
    for(size_t coord = 0; coord < sizeof(b)/sizeof(*b); ++coord)
    {
        if(b[coord] == empty_square || get_color(b[coord]) != stm)
            continue;
        const auto type = get_type(b[coord]);
        tmp_vect.push_back(the_pair(values[type], coord));
    }
    piece_id_t id = 0;
    std::sort(tmp_vect.begin(), tmp_vect.end());
    for(auto it : tmp_vect)
    {
        const auto coord = it.second;
        coords[stm][id] = coord;
        exist_mask[stm] |= (1 << id);
        find_piece_id[coord] = id++;
    }
    return true;
}





//--------------------------------
bool k2chess::SetupPosition(const char *fen)
{
    char *ptr = const_cast<char *>(fen);
    for(auto stm : {black, white})
    {
        material[stm] = 0;
        pieces[stm] = 0;
    }
    reversible_moves = 0;
    memset(quantity, 0, sizeof(quantity));
    memset(b_state, 0, sizeof(b_state));
    memset(find_piece_id, piece_not_found, sizeof(find_piece_id));
    ply = 0;

    if((ptr = ParseMainPartOfFen(ptr)) == nullptr)
        return false;
    if((ptr = ParseSideToMoveInFen(ptr)) == nullptr)
        return false;
    if((ptr = ParseCastlingRightsInFen(ptr)) == nullptr)
        return false;
    if((ptr = ParseEnPassantInFen(ptr)) == nullptr)
        return false;
    if(*(++ptr) && *(++ptr))  // set up number of reversible moves
        while(*ptr >= '0' && *ptr <= '9')
        {
            reversible_moves *= 10;
            reversible_moves += (*ptr++ - '0');
        }
    move_c tmp;
    tmp.flag = not_a_move;
    for(auto stm : {black, white})
    {
        InitPieceLists(stm);
        InitMasks(stm);
        InitQuantity(stm);
        InitDirections(stm);
        UpdateAttacks(stm, tmp);
    }
    return true;
}





//--------------------------------
char* k2chess::ParseMainPartOfFen(char *ptr)
{
    char chars[] = "kqrbnpKQRBNP";
    piece_t pcs[] =
    {
        black_king, black_queen, black_rook, black_bishop,
        black_knight, black_pawn, white_king, white_queen,
        white_rook, white_bishop, white_knight, white_pawn
    };

    for(int row = max_row; row >= 0; row--)
    {
        for(int col = 0; col <= max_col; col++)
        {
            const auto chr = *ptr - '0';
            if(chr >= 1 && chr <= board_width)
            {
                for(auto i = 0; i < chr; ++i)
                    b[get_coord(col++, row)] = empty_square;
                col--;
            }
            else if(*ptr == '/')
                col--;
            else
            {
                size_t index = 0;
                const size_t max_index = sizeof(chars)/sizeof(*chars);
                for(; index < max_index; ++index)
                    if(*ptr == chars[index])
                        break;
                if(index >= max_index)
                    return nullptr;
                if(get_type(pcs[index]) == pawn
                        && (row < pawn_default_row
                            || row > max_col - pawn_default_row))
                    return nullptr;
                const auto piece = pcs[index];
                b[get_coord(col, row)] = piece;
                const auto color = get_color(piece);
                if(get_type(piece) != king)
                    material[color] += values[get_type(piece)];
                pieces[color]++;
            }
            ptr++;
        }
    }
    return ptr;
}





//--------------------------------
char* k2chess::ParseSideToMoveInFen(char *ptr)
{
    ptr++;
    if(*ptr == 'b')
        wtm = black;
    else if(*ptr == 'w')
        wtm = white;
    else
        return nullptr;

    return ptr;
}





//--------------------------------
char *k2chess::ParseCastlingRightsInFen(char *ptr)
{
    castle_t castling_rights = 0;
    ptr += 2;
    char cstl_chr[] = "KQkq-";
    castle_t cstl_code[] = {white_can_castle_right, white_can_castle_left,
                            black_can_castle_right, black_can_castle_left, 0};
    coord_t k_col = default_king_col;
    coord_t k_row[] = {0, 0, max_row, max_row};
    coord_t r_col[] = {max_row, 0, max_row, 0};
    coord_t r_row[] = {0, 0, max_row, max_row};
    piece_t k_piece[] = {white_king, white_king, black_king, black_king};
    piece_t r_piece[] = {white_rook, white_rook, black_rook, black_rook};
    const size_t max_index = sizeof(cstl_chr)/sizeof(*cstl_chr);
    while(*ptr != ' ')
    {
        size_t index = 0;
        for(; index < max_index; ++index)
            if(*ptr == cstl_chr[index])
                break;
        if(index >= max_index)
            return nullptr;
        if(*ptr != '-' &&
                b[get_coord(k_col, k_row[index])] == k_piece[index] &&
                b[get_coord(r_col[index], r_row[index])] == r_piece[index])
            castling_rights |= cstl_code[index];
        ptr++;
    }
    state[0].castling_rights = castling_rights;
    return ptr;
}





//--------------------------------
char* k2chess::ParseEnPassantInFen(char *ptr)
{
    state[0].en_passant_rights = 0;
    ptr++;
    if(*ptr != '-')
    {
        int col = *(ptr++) - 'a';
        int row = *(ptr++) - '1';
        int s = wtm ? -1 : 1;
        piece_t pwn = white_pawn;
        if(!wtm)
            pwn = black_pawn;
        if(b[get_coord(col - 1, row + s)] == pwn
                || b[get_coord(col + 1, row + s)] == pwn)
            state[0].en_passant_rights = col + 1;
    }
    return ptr;
}





//--------------------------------
void k2chess::InitMasks(bool stm)
{
    slider_mask[stm] = 0;
    update_mask[stm] = 0;
    memset(type_mask[stm], 0, sizeof(type_mask[0]));
    auto mask = exist_mask[stm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        update_mask[stm] |= (1 << piece_id);
        const auto coord = coords[stm][piece_id];
        const auto piece = b[coord];
        assert(piece > empty_square);
        assert(piece <= white_pawn);
        const auto type = get_type(piece);
        type_mask[stm][type] |= (1 << piece_id);
        if(is_slider[type])
            slider_mask[stm] |= (1 << piece_id);
    }
}





//--------------------------------
void k2chess::ShowMove(const move_c move)
{
    assert(ply <= max_ply);
    char *cur = cur_moves + move_max_display_length*(ply - 1);
    *(cur++) = get_col(move.from_coord) + 'a';
    *(cur++) = get_row(move.from_coord) + '1';
    *(cur++) = get_col(move.to_coord) + 'a';
    *(cur++) = get_row(move.to_coord) + '1';
    *(cur++) = ' ';
    *cur = '\0';
}





//--------------------------------
void k2chess::StoreCurrentBoardState(const move_c move)
{
    state[ply].castling_rights = state[ply - 1].castling_rights;
    state[ply].captured_piece = b[move.to_coord];
    state[ply].reversible_moves = reversible_moves;
    state[ply].move = move;
    state[ply].slider_mask = slider_mask[wtm];
    state[ply].exist_mask[black] = exist_mask[black];
    state[ply].exist_mask[white] = exist_mask[white];
}





//--------------------------------
void k2chess::MoveToCoordinateNotation(const move_c move,
                                       char * const out) const
{
    char proms[] = {'?', 'q', 'n', 'r', 'b'};
    out[0] = get_col(move.from_coord) + 'a';
    out[1] = get_row(move.from_coord) + '1';
    out[2] = get_col(move.to_coord) + 'a';
    out[3] = get_row(move.to_coord) + '1';
    out[4] = (move.flag & is_promotion) ?
              proms[move.flag & is_promotion] : '\0';
    out[5] = '\0';
}





//--------------------------------
bool k2chess::MakeCastleOrUpdateFlags(const move_c move)
{
    const auto old_rights = state[ply].castling_rights;
    auto &new_rights = state[ply].castling_rights;
    if(get_type(b[move.from_coord]) == king)  // king moves
    {
        if(wtm)
            new_rights &= ~(white_can_castle_right | white_can_castle_left);
        else
            new_rights &= ~(black_can_castle_right | black_can_castle_left);
    }
    else if(b[move.from_coord] == set_color(black_rook, wtm))  // rook moves
    {
        const auto col = get_col(move.from_coord);
        const auto row = get_row(move.from_coord);
        if((wtm && row == 0) || (!wtm && row == max_col))
        {
            if(col == max_col)
            {
                if(wtm)
                    new_rights &= ~white_can_castle_right;
                else
                    new_rights &= ~black_can_castle_right;
            }
            else if(col == 0)
            {
                if(wtm)
                    new_rights &= ~white_can_castle_left;
                else
                    new_rights &= ~black_can_castle_left;
            }
        }
    }
    if(b[move.to_coord] == set_color(black_rook, !wtm))  // rook is taken
    {
        const auto col = get_col(move.to_coord);
        const auto row = get_row(move.to_coord);
        if((wtm && row == max_row) || (!wtm && row == 0))
        {
            if(col == max_col)
            {
                if(wtm)
                    new_rights &= ~black_can_castle_right;
                else
                    new_rights &= ~white_can_castle_right;
            }
            else if(col == 0)
            {
                if(wtm)
                    new_rights &= ~black_can_castle_left;
                else
                    new_rights &= ~white_can_castle_left;
            }
        }
    }
    bool castling_rights_changed = (new_rights != old_rights);
    if(castling_rights_changed)
        reversible_moves = 0;

    if(!(move.flag & is_castle))
        return castling_rights_changed;

    coord_t rook_from_coord, rook_to_coord;
    const auto row = wtm ? 0 : max_row;
    if(move.flag == is_right_castle)
    {
        rook_from_coord = get_coord(max_col, row);
        const auto col = default_king_col + cstl_move_length - 1;
        rook_to_coord = get_coord(col, row);
    }
    else
    {
        rook_from_coord = get_coord(0, row);
        const auto col = default_king_col - cstl_move_length + 1;
        rook_to_coord = get_coord(col, row);
    }
    const auto rook_id = find_piece_id[rook_from_coord];
    assert(rook_id != piece_not_found);
    state[ply].castled_rook_id = rook_id;
    b[rook_to_coord] = b[rook_from_coord];
    b[rook_from_coord] = empty_square;
    coords[wtm][rook_id] = rook_to_coord;
    find_piece_id[rook_from_coord] = piece_not_found;
    find_piece_id[rook_to_coord] = rook_id;

    reversible_moves = 0;
    return false;
}





//--------------------------------
void k2chess::TakebackCastle(const move_c move)
{
    const auto rook_id = state[ply].castled_rook_id;
    const auto from_coord = coords[wtm][rook_id];
    const auto to_col = move.flag == is_right_castle ? max_col : 0;
    const auto to_row = wtm ? 0 : max_row;
    const auto to_coord = get_coord(to_col, to_row);
    b[to_coord] = b[from_coord];
    b[from_coord] = empty_square;
    coords[wtm][rook_id] = to_coord;
    find_piece_id[to_coord] = rook_id;
    find_piece_id[from_coord] = piece_not_found;
}





//--------------------------------
bool k2chess::MakeEnPassantOrUpdateFlags(const move_c move)
{
    const shifts_t delta = get_row(move.to_coord) - get_row(move.from_coord);
    if(std::abs(delta) == pawn_long_move_length)
    {
        bool opp_pawn_is_near = get_col(move.to_coord) < max_col &&
                b[move.to_coord + 1] == set_color(black_pawn, !wtm);
        opp_pawn_is_near |= get_col(move.to_coord) > 0 &&
                b[move.to_coord - 1] == set_color(black_pawn, !wtm);
        if(opp_pawn_is_near)
        {
            state[ply].en_passant_rights = get_col(move.to_coord) + 1;
            return true;
        }
    }
    else if(move.flag & is_en_passant)
    {
        const auto delta = wtm ? -board_width : board_width;
        b[move.to_coord + delta] = empty_square;
        find_piece_id[move.to_coord + delta] = piece_not_found;
    }

    return false;
}





//--------------------------------
void k2chess::MakeCapture(const move_c move)
{
    auto to_coord = move.to_coord;
    auto type = get_type(b[move.to_coord]);
    assert(type != king);
    if(move.flag & is_en_passant)
    {
        to_coord += wtm ? -board_width : board_width;
        material[!wtm] -= values[pawn];
        quantity[!wtm][pawn]--;
        type = pawn;
    }
    else
    {
        material[!wtm] -= values[type];
        quantity[!wtm][type]--;
    }
    const auto piece_id = find_piece_id[to_coord];
    assert(piece_id != piece_not_found);
    state[ply].captured_id = piece_id;
    exist_mask[!wtm] &= ~(1 << piece_id);
    pieces[!wtm]--;
    reversible_moves = 0;
}





//--------------------------------
void k2chess::TakebackCapture(const move_c move)
{
    auto to_coord = move.to_coord;
    piece_type_t type;
    const auto capt_id = state[ply].captured_id;
    if(move.flag & is_en_passant)
    {
        type = pawn;
        to_coord += (wtm ? -board_width : board_width);
        const auto pwn = wtm ? black_pawn : white_pawn;
        b[to_coord] = pwn;
        find_piece_id[to_coord] = capt_id;
        find_piece_id[move.to_coord] = piece_not_found;
        coords[!wtm][capt_id] = to_coord;
    }
    else
    {
        type = get_type(state[ply].captured_piece);
        find_piece_id[move.to_coord] = capt_id;
    }
    material[!wtm] += values[type];
    quantity[!wtm][type]++;
    pieces[!wtm]++;
}





//--------------------------------
void k2chess::MakePromotion(const move_c move)
{
    piece_t pcs[] = {0, black_queen, black_knight, black_rook, black_bishop};
    const auto piece_num = move.flag & is_promotion;

    const auto piece = pcs[piece_num];
    const auto type = get_type(piece);
    b[move.to_coord] = set_color(piece, wtm);
    material[wtm] += values[type] - values[pawn];
    quantity[wtm][pawn]--;
    quantity[wtm][type]++;
    reversible_moves = 0;

    std::vector<attack_t> t(std::begin(type_mask[wtm]),
                            std::end(type_mask[wtm]));
    store_type_mask.push_back(t);
    std::vector<coord_t> v(std::begin(coords[wtm]), std::end(coords[wtm]));
    store_coords.push_back(v);
    InitPieceLists(wtm);
    InitMasks(wtm);
}





//--------------------------------
void k2chess::TakebackPromotion(const move_c move)
{
    piece_t pcs[] = {0, black_queen, black_knight, black_rook, black_bishop};
    const move_flag_t piece_num = move.flag & is_promotion;
    const auto type = get_type(pcs[piece_num]);
    material[wtm] -= (values[type] - values[pawn]);
    quantity[wtm][pawn]++;
    quantity[wtm][type]--;
    slider_mask[wtm] = state[ply].slider_mask;
}





//--------------------------------
void k2chess::UpdateAttackTables(move_c move)
{
    if(state[ply].attacks_updated)
        return;
    memcpy(done_attacks[ply - 1], attacks, sizeof(attacks));
    memcpy(done_directions[ply - 1], directions,
           sizeof(directions));
    memcpy(done_sum_directions[ply - 1], sum_directions,
            sizeof(sum_directions));

    UpdatePieceMasks(move);
    UpdateDirections(wtm, move);
    UpdateAttacks(wtm, move);

    if(move.flag & is_promotion)
    {
        InitMasks(!wtm);
        InitDirections(!wtm);
        move.flag = not_a_move;
        UpdateAttacks(!wtm, move);
    }
    else
    {
        UpdateDirections(!wtm, move);
        UpdateAttacks(!wtm, move);
    }
    state[ply].attacks_updated = true;
}





//--------------------------------
bool k2chess::MakeMove(const move_c move)
{
    ply++;
    bool is_special_move = false;
    StoreCurrentBoardState(move);
    reversible_moves++;
    if(move.flag & is_capture)
        MakeCapture(move);

    // trick: fast exit if not K|Q|R moves, and no captures
    if((b[move.from_coord] <= white_rook) || (move.flag & is_capture))
        is_special_move |= MakeCastleOrUpdateFlags(move);

    state[ply].en_passant_rights = 0;
    if(get_type(b[move.from_coord]) == pawn)
    {
        is_special_move |= MakeEnPassantOrUpdateFlags(move);
        reversible_moves = 0;
    }
#ifndef NDEBUG
    ShowMove(move);
#endif // NDEBUG

    b[move.to_coord] = b[move.from_coord];
    b[move.from_coord] = empty_square;
    assert(find_piece_id[move.from_coord] != piece_not_found);
    const auto id = find_piece_id[move.from_coord];
    coords[wtm][id] = move.to_coord;
    find_piece_id[move.from_coord] = piece_not_found;
    find_piece_id[move.to_coord] = id;

    if(move.flag & is_promotion)
        MakePromotion(move);

    wtm = !wtm;
    state[ply].attacks_updated = false;
    return is_special_move;
}





//--------------------------------
void k2chess::TakebackMove(move_c move)
{
    if(move.flag & is_promotion)
    {
        const auto &t = store_type_mask.back();
        std::copy(t.begin(), t.end(), type_mask[!wtm]);
        store_type_mask.pop_back();
        const auto &v = store_coords.back();
        std::copy(v.begin(), v.end(), coords[!wtm]);
        store_coords.pop_back();
        auto mask = state[ply].exist_mask[!wtm];
        while(mask)
        {
            const auto piece_id = lower_bit_num(mask);
            mask ^= (1 << piece_id);
            find_piece_id[coords[!wtm][piece_id]] = piece_id;
        }
    }
    assert(find_piece_id[move.to_coord] != piece_not_found);
    const auto id = find_piece_id[move.to_coord];
    coords[!wtm][id] = move.from_coord;

    if(move.flag & is_promotion)
        b[move.from_coord] = set_color(white_pawn, !wtm);
    else
        b[move.from_coord] = b[move.to_coord];
    find_piece_id[move.from_coord] = find_piece_id[move.to_coord];

    b[move.to_coord] = state[ply].captured_piece;
    reversible_moves = state[ply].reversible_moves;
    wtm = !wtm;

    if(move.flag & is_capture)
        TakebackCapture(move);
    else
        find_piece_id[move.to_coord] = piece_not_found;

    if(move.flag & is_promotion)
        TakebackPromotion(move);
    else if(move.flag & is_castle)
        TakebackCastle(move);
    exist_mask[black] = state[ply].exist_mask[black];
    exist_mask[white] = state[ply].exist_mask[white];

    ply--;
#ifndef NDEBUG
    cur_moves[move_max_display_length*ply] = '\0';
#endif // NDEBUG
}





//--------------------------------
void k2chess::TakebackMove()
{
    const auto move = state[ply].move;
    TakebackMove(move);
    TakebackAttacks();
}





//--------------------------------
bool k2chess::MakeMove(const char* str)
{
    const auto len = strlen(str);
    if(len < 4 || len > 5)  // only notation like 'g1f3', 'e7e8q' supported
        return false;
    const auto move = MoveFromStr(str);
    if(move.flag == not_a_move)
        return false;
    if(!IsPseudoLegal(move) || !IsLegal(move))
        return false;
    MakeMove(move);
    UpdateAttackTables(move);
    return true;
}





//--------------------------------
k2chess::move_flag_t k2chess::InitMoveFlag(const move_c move,
                                           const char promo_to) const
{
    move_flag_t ans = 0;
    const auto piece = get_type(b[move.from_coord]);
    if(b[move.to_coord] != empty_square
            && get_color(b[move.from_coord])
            != get_color(b[move.to_coord]))
        ans |= is_capture;
    if(piece == pawn
            && get_row(move.from_coord) == (wtm ? board_height - 2 : 1))
    {
        if(promo_to == 'q' or promo_to == '\0')
            ans |= is_promotion_to_queen;
        else if(promo_to == 'r')
            ans |= is_promotion_to_rook;
        else if(promo_to == 'b')
            ans |= is_promotion_to_bishop;
        else if(promo_to == 'n')
            ans |= is_promotion_to_knight;
        else
            return not_a_move;
    }
    if(piece == pawn)
    {
        const auto en_passant_state = state[ply].en_passant_rights;
        const auto delta = (en_passant_state - 1) - get_col(move.from_coord);
        auto required_row = pawn_default_row + pawn_long_move_length;
        if(wtm)
            required_row = max_row - required_row;
        if(en_passant_state && std::abs(delta) == 1
                && get_col(move.to_coord) != get_col(move.from_coord)
                && get_row(move.from_coord) == required_row)
            ans |= is_en_passant | is_capture;
    }
    if(piece == king && get_col(king_coord(wtm)) == default_king_col)
    {
        if(get_col(move.to_coord) == default_king_col + cstl_move_length)
            ans |= is_right_castle;
        else if(get_col(move.to_coord) == default_king_col - cstl_move_length)
            ans |= is_left_castle;
    }

    return ans;
}





//--------------------------------
bool k2chess::IsPseudoLegal(const move_c move) const
{
    if(b[move.from_coord] == empty_square)
        return false;
    if(find_piece_id[move.from_coord] == piece_not_found)
        return false;

    if(move.flag & not_a_move)
        return false;
    if(move.to_coord >= board_height*board_width)
        return false;
    const auto to_piece = b[move.to_coord];
    if(to_piece != empty_square && get_color(to_piece) == wtm)
        return false;
    if((move.flag & is_capture) && !(move.flag & is_en_passant) &&
            to_piece == empty_square)
        return false;
    if(!(move.flag & is_capture) && to_piece != empty_square)
        return false;
    if(move.flag & is_castle && get_type(b[move.from_coord]) != king)
        return false;
    const auto piece = b[move.from_coord];
    if(piece == empty_square || get_color(piece) != wtm)
        return false;
    const auto type = get_type(piece);
    if(type == pawn)
        return IsPseudoLegalPawn(move);
    if(move.flag & is_promotion || move.flag & is_en_passant)
        return false;
    if(is_slider[type])
    {
        const auto fr_col = get_col(move.from_coord);
        const auto to_col = get_col(move.to_coord);
        const auto fr_row = get_row(move.from_coord);
        const auto to_row = get_row(move.to_coord);
        if(type == bishop && std::abs(fr_col - to_col) !=
                std::abs(fr_row - to_row))
            return false;
        if(type == rook && fr_col != to_col && fr_row != to_row)
            return false;
        return IsSliderAttack(move.from_coord, move.to_coord);
    }
    else if(type == king)
        return IsPseudoLegalKing(move);
    else if(type == knight)
        return IsPseudoLegalKnight(move);
    else
        return false;
}





//--------------------------------
bool k2chess::IsPseudoLegalKing(const move_c move) const
{
    const auto fr_col = get_col(move.from_coord);
    const auto fr_row = get_row(move.from_coord);
    const auto to_col = get_col(move.to_coord);
    const auto to_row = get_row(move.to_coord);

    if(std::abs(fr_col - to_col) <= 1 && std::abs(fr_row - to_row) <= 1
       && !(move.flag & is_castle))
        return true;

    if(fr_col == default_king_col && get_color(b[move.from_coord]) == wtm &&
       ((wtm && fr_row == 0 && to_row == 0) ||
                (!wtm && fr_row == max_row && to_row == max_row)))
    {
        if(state[ply].castling_rights &
           (wtm ? white_can_castle_left : black_can_castle_left) &&
                to_col == default_king_col - cstl_move_length &&
                move.flag == is_left_castle)
                return true;
        if(state[ply].castling_rights &
           (wtm ? white_can_castle_right : black_can_castle_right) &&
                to_col == default_king_col + cstl_move_length &&
                move.flag == is_right_castle)
            return true;
    }
    return false;
}





//--------------------------------
bool k2chess::IsPseudoLegalKnight(const move_c move) const
{
    const auto d_col = std::abs(get_col(move.from_coord) -
                                get_col(move.to_coord));
    const auto d_row = std::abs(get_row(move.from_coord) -
                                get_row(move.to_coord));
    return d_col + d_row == 3 && (d_col == 1 || d_row == 1);
}





//--------------------------------
bool k2chess::IsPseudoLegalPawn(const move_c move) const
{
    const auto delta = wtm ? board_width : -board_width;
    const auto is_special = is_en_passant | is_capture | is_castle;
    const auto from_row = get_row(move.from_coord);
    const auto last_row = wtm ? max_row - 1 : 1;
    if((move.flag & is_promotion) && from_row != last_row)
        return false;
    if(!(move.flag & is_promotion) && from_row == last_row)
        return false;
    if((move.flag & is_promotion) > is_promotion_to_bishop)
        return false;
    if(move.flag & is_promotion && move.flag & (is_castle | is_en_passant))
        return false;
    if(move.to_coord == move.from_coord + delta
            && b[move.to_coord] == empty_square
            && !(move.flag & is_special))
        return true;
    const auto init_row = (wtm ? pawn_default_row :
                                 max_row - pawn_default_row);
    const auto from_col = get_col(move.from_coord);
    if(move.to_coord == move.from_coord + 2*delta
            && get_row(move.from_coord) == init_row
            && b[move.to_coord] == empty_square
            && b[move.to_coord - delta] == empty_square
            && !(move.flag & is_special) && !(move.flag & is_promotion))
        return true;
    else if((move.to_coord == move.from_coord + delta + 1 &&
             from_col < max_col)
        || (move.to_coord == move.from_coord + delta - 1 && from_col > 0))
    {
        if(b[move.to_coord] != empty_square && move.flag & is_capture
                && !(move.flag & is_en_passant))
        {
            if(get_color(b[move.to_coord]) != wtm)
                return true;
        }
        else if(move.flag == (is_en_passant | is_capture))
        {
            const auto col = get_col(move.from_coord);
            auto row = pawn_default_row + pawn_long_move_length;
            if(wtm)
                row = max_row - row;
            const auto delta2 = move.to_coord - move.from_coord - delta;
            const auto rights = state[ply].en_passant_rights;
            const auto pw = set_color(black_pawn, !wtm);
            if(get_row(move.from_coord) == row && rights - 1 == col + delta2
                    && b[get_coord(col + delta2, row)] == pw)
                return true;
        }
    }
    return false;
}





//--------------------------------
bool k2chess::IsLegalCastle(const move_c move) const
{
    const auto row = wtm ? 0 : max_row;
    coord_t begin_col1, begin_col2, end_col1, end_col2;
    if(move.flag & is_left_castle)
    {
        const auto can_castle = wtm ? white_can_castle_left :
                                      black_can_castle_left;
        if(!(state[ply].castling_rights & can_castle))
            return false;
        begin_col1 = 1;
        end_col1 = default_king_col - 1;
        begin_col2 = default_king_col - cstl_move_length;
        end_col2 = default_king_col;
    }
    else
    {
        const auto can_castle = wtm ? white_can_castle_right :
                                      black_can_castle_right;
        if(!(state[ply].castling_rights & can_castle))
            return false;
        begin_col1 = default_king_col + 1;
        end_col1 = max_col - 1;
        begin_col2 = default_king_col;
        end_col2 = default_king_col + cstl_move_length;
    }
    for(auto col = begin_col1; col <= end_col1; ++col)
        if(b[get_coord(col, row)] != empty_square)
            return false;
    for(auto col = begin_col2; col <= end_col2; ++col)
        if(attacks[!wtm][get_coord(col, row)])
            return false;
    return true;
}





//--------------------------------
bool k2chess::IsOnRay(const coord_t given, const coord_t ray_coord1,
                      const coord_t ray_coord2) const
{
    if(ray_coord1 > ray_coord2 && (given > ray_coord1 || given < ray_coord2))
        return false;
    if(ray_coord1 < ray_coord2 && (given < ray_coord1 || given > ray_coord2))
        return false;

    return IsSameRay(given, ray_coord1, ray_coord2);
}





//--------------------------------
bool k2chess::IsSameRay(const coord_t given, const coord_t ray_coord1,
                       const coord_t ray_coord2) const
{
    const auto dx1 = get_col(ray_coord2) - get_col(ray_coord1);
    const auto dy1 = get_row(ray_coord2) - get_row(ray_coord1);
    const auto dx2 = get_col(given) - get_col(ray_coord1);
    const auto dy2 = get_row(given) - get_row(ray_coord1);

    if(sgn(dx1) != sgn(dx2) || sgn(dy1) != sgn(dy2))
        return false;
    if(dx1 != 0 && dy1 != 0 && std::abs(dx1) != std::abs(dy1))
        return false;
    if(dx2 != 0 && dy2 != 0 && std::abs(dx2) != std::abs(dy2))
        return false;

    return true;
}





//--------------------------------
bool k2chess::IsOnRayMask(const bool stm, const coord_t piece_coord,
                          const coord_t move_coord, const piece_id_t piece_id,
                          const ray_mask_t ray_id, const bool en_pass) const
{
    if(get_type(b[piece_coord]) == knight)
        return move_coord == piece_coord +
                get_coord(delta_col[ray_id], delta_row[ray_id]);
    const auto ray_col = get_col(piece_coord) + delta_col[ray_id];
    const auto ray_row = get_row(piece_coord) + delta_row[ray_id];
    if(!col_within(ray_col) || !row_within(ray_row))
        return false;
    if(!en_pass && !(attacks[stm][move_coord] & (1 << piece_id)))
        return false;
    const auto ray_coord = get_coord(ray_col, ray_row);
    bool same_ray = IsSameRay(move_coord, piece_coord, ray_coord);
    if(en_pass)
    {
        const auto p_coord = coords[wtm][k2chess::state[ply].captured_id];
        if(same_ray && IsSliderAttack(piece_coord, move_coord))
            return true;
        if(std::abs(move_coord - p_coord) == 1 &&
                IsSameRay(p_coord, piece_coord, ray_coord) &&
                IsSliderAttack(piece_coord, p_coord))
            return true;
        return false;
    }
    return same_ray;
}





//--------------------------------
bool k2chess::IsSliderAttack(const coord_t from_coord,
                             const coord_t to_coord) const
{
    const auto d_col = get_col(to_coord) - get_col(from_coord);
    const auto d_row = get_row(to_coord) - get_row(from_coord);

    const auto delta_coord = sgn(d_col) + board_width*sgn(d_row);
    for(coord_t i = from_coord + delta_coord;
        i != to_coord;
        i += delta_coord)
    {
        if(i >= sizeof(b)/sizeof(*b) || b[i] != empty_square)
            return false;
    }

    return true;
}





//--------------------------------
bool k2chess::IsLegalKingMove(const move_c move) const
{
    if(move.flag & is_castle)
        return IsLegalCastle(move);
    if(attacks[!wtm][move.to_coord])
        return false;

    auto att_mask = attacks[!wtm][move.from_coord] & slider_mask[!wtm];
    while(att_mask)
    {
        const auto piece_id = lower_bit_num(att_mask);
        att_mask ^= (1 << piece_id);
        const auto attacker_coord = coords[!wtm][piece_id];
        if(IsOnRay(move.from_coord, move.to_coord, attacker_coord))
            return false;
    }
    return true;
}





//--------------------------------
bool k2chess::IsLegal(const move_c move) const
{
    const auto piece_type = get_type(b[move.from_coord]);
    if(piece_type == king)
        return IsLegalKingMove(move);
    if((move.flag & is_en_passant) && IsDiscoveredEnPassant(wtm, move))
        return false;
    const auto k_coord = king_coord(wtm);
    const auto attack_sum = attacks[!wtm][k_coord];
    const auto king_attackers = __builtin_popcount(attack_sum);
    assert(king_attackers <= 2);
    if(king_attackers == 2)
        return false;
    const auto att_mask = attacks[!wtm][move.from_coord] & slider_mask[!wtm];
    if(att_mask && IsDiscoveredAttack(move))
        return false;
    if(king_attackers == 0)
        return true;

    const auto attacker_id = lower_bit_num(attack_sum);
    const auto attacker_coord = coords[!wtm][attacker_id];
    if(is_slider[get_type(b[attacker_coord])])
        return IsOnRay(move.to_coord, k_coord, attacker_coord);

    if(move.to_coord == attacker_coord)
        return true;

    if((move.flag & is_en_passant) && move.to_coord == attacker_coord +
            (wtm ? board_width : -board_width))
        return true;

    return false;
}





//-----------------------------
bool k2chess::PrintMoveSequence(const move_c * const moves,
                                const size_t length,
                                const bool coordinate_notation)
{
    bool success = true;
    char move_str[6];
    size_t i = 0;
    for(; i < length; ++i)
    {
        const auto cur_move = moves[i];
        if(!IsPseudoLegal(cur_move) || !IsLegal(cur_move))
        {
            success = false;
            break;
        }
        if(coordinate_notation)
            MoveToCoordinateNotation(cur_move, move_str);
        else
            MoveToAlgebraicNotation(cur_move, move_str);
        std::cout << move_str << " ";
        MakeMove(cur_move);
        UpdateAttackTables(cur_move);
    }
    for(; i > 0; --i)
    {
        TakebackMove(moves[i - 1]);
        TakebackAttacks();
    }
    return success;
}





//-----------------------------
void k2chess::MoveToAlgebraicNotation(const move_c move, char *out) const
{
    char pc2chr[] = "??KKQQRRBBNNPP";
    const auto piece_char = pc2chr[b[move.from_coord]];
    if(piece_char == 'K' && get_col(move.from_coord) == 4
            && get_col(move.to_coord) == 6)
    {
        *(out++) = 'O';
        *(out++) = 'O';
    }
    else if(piece_char == 'K' && get_col(move.from_coord) == 4
            && get_col(move.to_coord) == 2)
    {
        *(out++) = 'O';
        *(out++) = 'O';
        *(out++) = 'O';
    }
    else if(piece_char != 'P')
    {
        *(out++) = piece_char;
        ProcessAmbiguousNotation(move, out);
        if(move.flag & is_capture)
            *(out++) = 'x';
        *(out++) = get_col(move.to_coord) + 'a';
        *(out++) = get_row(move.to_coord) + '1';
    }
    else if(move.flag & is_capture)
    {
        *(out++) = get_col(move.from_coord) + 'a';
        *(out++) = 'x';
        *(out++) = get_col(move.to_coord) + 'a';
        *(out++) = get_row(move.to_coord) + '1';
    }
    else
    {
        *(out++) = get_col(move.to_coord) + 'a';
        *(out++) = get_row(move.to_coord) + '1';
    }
    char proms[] = "?QNRB";
    if(piece_char == 'P' && (move.flag & is_promotion))
        *(out++) = proms[move.flag & is_promotion];
    *(out++) = '\0';
}





//-----------------------------
void k2chess::ProcessAmbiguousNotation(const move_c move, char *out) const
{
    move_c move_array[8];
    auto amb_cr = 0;
    const auto init_from_coord = move.from_coord;
    const auto init_piece_type = get_type(b[init_from_coord]);

    auto mask = exist_mask[wtm];
    while(mask)
    {
        const auto piece_id = lower_bit_num(mask);
        mask ^= (1 << piece_id);
        const auto coord = coords[wtm][piece_id];
        if(coord == move.from_coord)
            continue;

        const auto type = get_type(b[coord]);
        if(type != init_piece_type)
            continue;
        if(!(attacks[wtm][move.to_coord] & (1 << piece_id)))
            continue;

        auto tmp = move;
        tmp.priority = coord;
        move_array[amb_cr++] = tmp;
    }
    if(!amb_cr)
        return;

    bool same_cols = false, same_rows = false;
    for(auto i = 0; i < amb_cr; i++)
    {
        if(get_col(move_array[i].priority) == get_col(init_from_coord))
            same_cols = true;
        if(get_row(move_array[i].priority) == get_row(init_from_coord))
            same_rows = true;
    }
    if(same_cols && same_rows)
    {
        *(out++) = get_col(init_from_coord) + 'a';
        *(out++) = get_row(init_from_coord) + '1';
    }
    else if(same_cols)
        *(out++) = get_row(init_from_coord) + '1';
    else
        *(out++) = get_col(init_from_coord) + 'a';
}





#ifndef NDEBUG
//--------------------------------
size_t k2chess::test_attack_tables(const bool stm) const
{
    size_t ans = 0;
    for(auto row = 0; row < board_width; ++row)
        for(auto col = 0; col < board_height; ++col)
            if(attacks[stm][get_coord(col, row)] != 0)
                ans++;
    return ans;
}





//--------------------------------
void k2chess::RunUnitTests()
{
    SetupPosition(start_position);
    assert(test_attack_tables(black) == 22);
    assert(test_attack_tables(white) == 22);
    SetupPosition("4k3/8/5n2/5n2/8/8/8/3RK3 w - -");
    assert(test_attack_tables(black) == 19);
    assert(test_attack_tables(white) == 15);
    SetupPosition("2r2rk1/p4q2/1p2b3/1n6/1N6/1P2B3/P4Q2/2R2RK1 w - -");
    assert(test_attack_tables(black) == 39);
    assert(test_attack_tables(white) == 39);

    SetupPosition("2k1r2r/1pp3pp/p2b4/2p1n2q/6b1/1NQ1B3/PPP2PPP/R3RNK1 b - -");
    assert(test_attack_tables(black) == 38);
    assert(test_attack_tables(white) == 32);

    assert(SetupPosition(start_position));
    assert(b[coords[white][lower_bit_num(exist_mask[white])]] == white_pawn);
    assert(b[coords[white][higher_bit_num(exist_mask[white])]] ==
            white_king);
    assert(b[coords[black][lower_bit_num(exist_mask[black])]] == black_pawn);
    assert(b[coords[black][higher_bit_num(exist_mask[black])]] ==
            black_king);
    assert(king_coord(white) == get_coord(default_king_col, 0));
    assert(king_coord(black) ==
           get_coord(default_king_col, max_row));
    assert(ply == 0);
    assert(wtm == white);

    assert(MakeMove("e2e4"));
    assert(b[get_coord("e2")] == empty_square);
    assert(b[get_coord("e4")] == white_pawn);
    assert(ply == 1);
    assert(wtm == black);
    assert(reversible_moves == 0);

    assert(MakeMove("b8c6"));
    assert(b[get_coord("b8")] == empty_square);
    assert(b[get_coord("c6")] == black_knight);
    assert(ply == 2);
    assert(wtm == white);
    assert(reversible_moves == 1);

    assert(MakeMove("f1c4"));
    assert(b[get_coord("f1")] == empty_square);
    assert(b[get_coord("e2")] == empty_square);
    assert(b[get_coord("d3")] == empty_square);
    assert(b[get_coord("c4")] == white_bishop);
    assert(reversible_moves == 2);

    assert(MakeMove("e7e6"));
    assert(b[get_coord("e7")] == empty_square);
    assert(b[get_coord("e6")] == black_pawn);
    assert(reversible_moves == 0);

    assert(MakeMove("d2d4"));
    assert(MakeMove("f8e7"));
    assert(MakeMove("e4e5"));
    assert(MakeMove("d7d5"));
    assert(MakeMove("e5d6"));
    assert(b[get_coord("e5")] == empty_square);
    assert(b[get_coord("d6")] == white_pawn);
    assert(b[get_coord("d5")] == empty_square);
    assert(ply == 9);
    assert(reversible_moves == 0);
    assert(quantity[white][pawn] == 8);
    assert(quantity[black][pawn] == 7);
    assert(material[white] == 8*values[pawn] + values[queen] +
           2*values[rook] + 2*values[bishop] + 2*values[knight]);
    assert(material[black] == 7*values[pawn] + values[queen] +
           2*values[rook] + 2*values[bishop] + 2*values[knight]);
    assert(pieces[white] == 16);
    assert(pieces[black] == 15);
    assert(find_piece_id[get_coord("d5")] == piece_not_found);
    assert(find_piece_id[get_coord("e5")] == piece_not_found);
    assert(b[coords[white][find_piece_id[get_coord("e6")]]] == white_pawn);
    assert(test_attack_tables(black) == 29);
    assert(test_attack_tables(white) == 35);

    TakebackMove();
    assert(b[get_coord("d5")] == black_pawn);
    assert(b[get_coord("e5")] == white_pawn);
    assert(b[get_coord("d6") == empty_square]);
    assert(find_piece_id[get_coord("d6")] == piece_not_found);
    assert(b[coords[white][find_piece_id[get_coord("e5")]]] == white_pawn);
    assert(b[coords[black][find_piece_id[get_coord("d5")]]] == black_pawn);

    assert(MakeMove("e5d6"));
    assert(MakeMove("e7d6"));
    assert(MakeMove("c4b5"));
    assert(MakeMove("d8d7"));
    assert(MakeMove("b5c6"));
    assert(MakeMove("d7c6"));
    assert(MakeMove("d1f3"));
    assert(MakeMove("c6f3"));
    assert(MakeMove("g1f3"));
    assert(MakeMove("d6b4"));
    assert(MakeMove("c2c3"));
    TakebackMove();
    assert(b[get_coord("c3")] == empty_square);
    assert(find_piece_id[get_coord("c3")] == piece_not_found);
    assert(MakeMove("b1d2"));
    TakebackMove();
    assert(MakeMove("c1d2"));
    assert(MakeMove("h7h5"));
    assert(MakeMove("d2c3"));
    assert(ply == 21);
    assert(reversible_moves == 1);
    assert(quantity[white][pawn] == 7);
    assert(quantity[black][pawn] == 7);
    assert(quantity[white][bishop] == 1);
    assert(quantity[black][knight] == 1);
    assert(quantity[white][queen] == 0);
    assert(quantity[black][queen] == 0);
    assert(material[white] == 7*values[pawn] +
           2*values[rook] + values[bishop] +
           2*values[knight]);
    assert(material[black] == 7*values[pawn] +
           2*values[rook] + 2*values[bishop] +
           values[knight]);
    assert(pieces[white] == 13);
    assert(pieces[black] == 13);

    assert(SetupPosition("r3k2r/ppp2ppp/8/8/8/8/PPP2PPP/R3K2R w KQkq - 0 1"));
    assert(MakeMove("e1g1"));
    assert(b[get_coord("e1")] == empty_square);
    assert(b[get_coord("g1")] == white_king);
    assert(b[get_coord("h1")] == empty_square);
    assert(b[get_coord("f1")] == white_rook);
    assert(find_piece_id[get_coord("e1")] == piece_not_found);
    assert(find_piece_id[get_coord("h1")] == piece_not_found);
    assert(b[coords[white][find_piece_id[get_coord("g1")]]] == white_king);
    assert(b[coords[white][find_piece_id[get_coord("f1")]]] == white_rook);
    assert(reversible_moves == 0);

    assert(MakeMove("e8c8"));
    assert(b[get_coord("e8")] == empty_square);
    assert(b[get_coord("c8")] == black_king);
    assert(b[get_coord("a8")] == empty_square);
    assert(b[get_coord("d8")] == black_rook);
    assert(find_piece_id[get_coord("e8")] == piece_not_found);
    assert(find_piece_id[get_coord("a8")] == piece_not_found);
    assert(b[coords[black][find_piece_id[get_coord("c8")]]] == black_king);
    assert(b[coords[black][find_piece_id[get_coord("d8")]]] == black_rook);
    assert(reversible_moves == 0);
    assert(test_attack_tables(black) == 24);
    assert(test_attack_tables(white) == 20);

    TakebackMove();
    assert(b[get_coord("e8")] == black_king);
    assert(b[get_coord("c8")] == empty_square);
    assert(b[get_coord("a8")] == black_rook);
    assert(b[get_coord("d8")] == empty_square);
    assert(find_piece_id[get_coord("c8")] == piece_not_found);
    assert(find_piece_id[get_coord("d8")] == piece_not_found);
    assert(b[coords[black][find_piece_id[get_coord("e8")]]] == black_king);
    assert(b[coords[black][find_piece_id[get_coord("a8")]]] == black_rook);

    TakebackMove();
    assert(b[get_coord("e1")] == white_king);
    assert(b[get_coord("g1")] == empty_square);
    assert(b[get_coord("h1")] == white_rook);
    assert(b[get_coord("f1")] == empty_square);
    assert(find_piece_id[get_coord("g1")] == piece_not_found);
    assert(find_piece_id[get_coord("f1")] == piece_not_found);
    assert(b[coords[white][find_piece_id[get_coord("e1")]]] == white_king);
    assert(b[coords[white][find_piece_id[get_coord("h1")]]] == white_rook);

    assert(SetupPosition("r3k2r/ppp2ppp/8/8/8/8/PPP2PPP/R3K2R w KQkq - 0 1"));
    assert(MakeMove("e1c1"));
    assert(b[get_coord("e1")] == empty_square);
    assert(b[get_coord("c1")] == white_king);
    assert(b[get_coord("a1")] == empty_square);
    assert(b[get_coord("d1")] == white_rook);
    assert(reversible_moves == 0);

    assert(MakeMove("e8g8"));
    assert(b[get_coord("e8")] == empty_square);
    assert(b[get_coord("g8")] == black_king);
    assert(b[get_coord("h8")] == empty_square);
    assert(b[get_coord("f8")] == black_rook);
    assert(reversible_moves == 0);

    assert(SetupPosition("2n1r3/3P4/7k/8/8/8/B7/K7 w - - 4 1"));
    assert(test_attack_tables(black) == 20);
    assert(test_attack_tables(white) == 11);
    assert(MakeMove("d7d8q"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("d8")] == white_queen);
    assert(quantity[white][pawn] == 0);
    assert(quantity[white][queen] = 1);
    assert(pieces[white] = 3);
    assert(pieces[black] = 3);
    assert(material[white] == values[queen] + values[bishop]);
    assert(material[black] == values[knight] + values[rook]);
    assert(reversible_moves == 0);
    assert(test_attack_tables(black) == 19);
    assert(test_attack_tables(white) == 24);
    assert(b[coords[white][2]] == white_king);
    assert(b[coords[white][1]] == white_queen);
    assert(b[coords[white][0]] == white_bishop);
    assert(coords[white][find_piece_id[get_coord("d8")]] == get_coord("d8"));
    assert(coords[white][find_piece_id[get_coord("a2")]] == get_coord("a2"));
    assert(coords[white][find_piece_id[get_coord("a1")]] == get_coord("a1"));
    assert(find_piece_id[get_coord("d7")] == piece_not_found);
    assert(b[coords[white][find_piece_id[get_coord("d8")]]] == white_queen);

    TakebackMove();
    assert(b[get_coord("d7")] == white_pawn);
    assert(b[get_coord("d8")] == empty_square);
    assert(material[white] == values[pawn] + values[bishop]);
    assert(material[black] == values[knight] + values[rook]);
    assert(quantity[white][pawn] == 1);
    assert(quantity[white][queen] == 0);
    assert(reversible_moves == 4);
    assert(b[coords[white][2]] == white_king);
    assert(b[coords[white][1]] == white_bishop);
    assert(b[coords[white][0]] == white_pawn);
    assert(coords[white][find_piece_id[get_coord("d7")]] == get_coord("d7"));
    assert(coords[white][find_piece_id[get_coord("a2")]] == get_coord("a2"));
    assert(coords[white][find_piece_id[get_coord("a1")]] == get_coord("a1"));
    assert(find_piece_id[get_coord("d8")] == piece_not_found);
    assert(b[coords[white][find_piece_id[get_coord("d7")]]] == white_pawn);

    assert(MakeMove("d7d8"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("d8")] == white_queen);
    assert(quantity[white][pawn] == 0);
    assert(quantity[white][queen] = 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 3);
    assert(material[white] == values[queen] + values[bishop]);
    assert(material[black] == values[knight] + values[rook]);
    assert(reversible_moves == 0);
    TakebackMove();

    assert(MakeMove("d7e8b"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("e8")] == white_bishop);
    assert(quantity[white][pawn] == 0);
    assert(quantity[white][bishop] == 2);
    assert(pieces[white] = 2);
    assert(pieces[black] = 2);
    assert(material[white] == 2*values[bishop]);
    assert(material[black] == values[knight]);
    assert(reversible_moves == 0);
    TakebackMove();

    assert(SetupPosition("k7/8/8/8/8/7K/3p4/2N1R3 b - - 0 1"));
    assert(MakeMove("d2c1r"));
    assert(b[get_coord("d2")] == empty_square);
    assert(b[get_coord("c1")] == black_rook);
    assert(quantity[black][pawn] == 0);
    assert(quantity[black][rook] == 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 2);
    assert(material[white] == values[rook]);
    assert(material[black] == values[rook]);
    assert(reversible_moves == 0);
    TakebackMove();

    assert(MakeMove("d2c1n"));
    assert(b[get_coord("d2")] == empty_square);
    assert(b[get_coord("c1")] == black_knight);
    assert(quantity[black][pawn] == 0);
    assert(quantity[black][knight] == 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 2);
    assert(material[white] == values[rook]);
    assert(material[black] == values[knight]);
    assert(reversible_moves == 0);
    TakebackMove();

    assert(!MakeMove("d2d1m"));

    assert(SetupPosition(start_position));
    assert(!MakeMove("c1b2"));
    assert(!MakeMove("c1a3"));
    assert(!MakeMove("c1d3"));
    assert(!MakeMove("b1d2"));
    assert(!MakeMove("a1a2"));
    assert(!MakeMove("a1a5"));
    assert(!MakeMove("a1a7"));
    assert(!MakeMove("d1d8"));
    assert(!MakeMove("d1e3"));
    assert(!MakeMove("e1g1"));
    assert(!MakeMove("e1e2"));
    assert(!MakeMove("e2e5"));
    assert(!MakeMove("e2e8"));
    assert(!MakeMove("e2d3"));

    assert(SetupPosition("8/8/1P3k1N/P1b3p1/8/8/3K4/8 w - - 0 1"));
    assert(!MakeMove("b6c5"));
    assert(!MakeMove("b6a5"));
    assert(!MakeMove("b6b5"));
    assert(!MakeMove("b6b8"));
    assert(!MakeMove("h6a8"));
    assert(!MakeMove("h6a4"));
    assert(!MakeMove("h6b5"));
    assert(!MakeMove("h6b7"));
    assert(!MakeMove("d2g1"));
    assert(MakeMove("b6b7"));
    assert(!MakeMove("g5h6"));
    assert(!MakeMove("g5f6"));
    assert(!MakeMove("g5g6"));
    assert(!MakeMove("c5h8"));
    assert(!MakeMove("c5h7"));
    assert(!MakeMove("c5h2"));
    assert(!MakeMove("c5h1"));
    assert(!MakeMove("f6h6"));
    assert(!MakeMove("b6b8"));
    assert(!MakeMove("f6g8"));
    assert(MakeMove("g5g4"));

    assert(SetupPosition("r3k2r/p5pp/7N/4B3/4b3/6P1/7P/RN2K2R w KQkq - 0 1"));
    assert(MakeMove("e1g1"));
    assert(MakeMove("e8c8"));
    TakebackMove();
    TakebackMove();
    assert(b[get_coord("e1")] == white_king);
    assert(b[get_coord("e8")] == black_king);
    assert(b[get_coord("h1")] == white_rook);
    assert(b[get_coord("a8")] == black_rook);
    assert(!MakeMove("e1c1"));
    assert(!MakeMove("e1c8"));
    assert(!MakeMove("e1g8"));
    assert(MakeMove("h2h3"));
    assert(!MakeMove("e8g8"));
    assert(!MakeMove("e8g1"));
    assert(!MakeMove("e8c1"));
    TakebackMove();
    assert(MakeMove("e1e2"));
    assert(MakeMove("e8e7"));
    assert(MakeMove("e2e1"));
    assert(MakeMove("e7e8"));
    assert(!MakeMove("e1g1"));
    assert(MakeMove("h2h3"));
    assert(!MakeMove("e8c8"));
    for(auto i = 0; i < 5; ++i)
        TakebackMove();
    assert(MakeMove("h1g1"));
    assert(MakeMove("a8b8"));
    assert(MakeMove("g1h1"));
    assert(MakeMove("b8a8"));
    assert(!MakeMove("e1g1"));
    assert(MakeMove("h2h3"));
    assert(!MakeMove("e8g8"));
    assert(SetupPosition("rn2k2r/p5pp/8/8/8/6P1/P6P/R3K1NR w KQkq - 0 1"));
    assert(MakeMove("a1b1"));
    assert(MakeMove("h8g8"));
    assert(MakeMove("b1a1"));
    assert(MakeMove("g8h8"));
    assert(!MakeMove("e1c1"));
    assert(MakeMove("h2h3"));
    assert(!MakeMove("e8g8"));
    assert(SetupPosition("r3k2r/p5pp/7N/8/2B1b3/6P1/7P/RN2K2R w KQkq - 0 1"));
    assert(MakeMove("e1g1"));

    assert(SetupPosition("r3k2r/1p5p/1N6/4B3/4b3/1n6/1P5P/R3K2R w KQkq -"));
    assert(MakeMove("e5h8"));
    assert(MakeMove("e4h1"));
    assert(!MakeMove("e1g1"));
    assert(!MakeMove("e8g8"));
    assert(MakeMove("b6a8"));
    assert(MakeMove("b3a1"));
    assert(!MakeMove("e1c1"));
    assert(!MakeMove("e8c8"));

    assert(SetupPosition("3Q4/8/7B/8/q2N4/2p2Pk1/8/2n1K2R b K - 0 1"));
    assert(MakeMove("g3g2"));
    TakebackMove();
    assert(!MakeMove("g3h4"));
    assert(!MakeMove("g3g4"));
    assert(!MakeMove("g3f4"));
    assert(!MakeMove("g3f3"));
    assert(!MakeMove("g3f2"));
    assert(!MakeMove("g3h2"));
    assert(!MakeMove("g3h3"));
    assert(MakeMove("a4b3"));
    assert(!MakeMove("e1d1"));
    assert(!MakeMove("e1d2"));
    assert(!MakeMove("e1e2"));
    assert(!MakeMove("e1f2"));
    assert(!MakeMove("e1d1"));
    assert(MakeMove("e1f1"));
    TakebackMove();
    assert(MakeMove("e1g1"));

    assert(SetupPosition("4k3/4r3/8/b7/1N5q/4B3/5P2/2R1K3 w - - 0 1"));
    assert(!MakeMove("b4a2"));
    assert(!MakeMove("e3g5"));
    assert(!MakeMove("f2f3"));
    assert(MakeMove("e1d1"));
    assert(MakeMove("e8d8"));
    assert(!MakeMove("d2c4"));
    TakebackMove();
    assert(!MakeMove("e2d3"));
    TakebackMove();
    assert(!MakeMove("f2f3"));
    assert(MakeMove("c1a1"));
    assert(MakeMove("a5b4"));
    assert(!MakeMove("e3d2"));
    SetupPosition("rnbq1k1r/1p1Pbppp/2p5/p7/1PB5/8/P1P1NnPP/RNBQK2R w KQ a6");
    assert(MakeMove("b4b5"));

    assert(SetupPosition("4k3/3ppp2/4b3/1n5Q/B7/8/4R3/4K3 b - - 0 1"));
    assert(MakeMove("b5a7"));
    TakebackMove();
    assert(MakeMove("e6g4"));
    assert(!MakeMove("f7f6"));
    TakebackMove();
    assert(MakeMove("e8d8"));
    assert(MakeMove("e2d2"));

    assert(SetupPosition("8/8/8/8/k1p4R/8/3P4/2K5 w - - 0 1"));
    assert(MakeMove("d2d4"));
    assert(!MakeMove("c4d3"));
    assert(SetupPosition("3k2q1/2p5/8/3P4/8/8/K7/8 b - - 0 1"));
    assert(MakeMove("c7c5"));
    assert(!MakeMove("d5c6"));
    assert(SetupPosition("8/8/8/8/k1p4B/8/3P4/2K5 w - - 0 1"));
    assert(MakeMove("d2d4"));
    assert(MakeMove("c4d3"));
    TakebackMove();
    assert(MakeMove("c4c3"));
    assert(MakeMove("d4d5"));
    assert(SetupPosition(
        "rnb2k1r/pp1Pbppp/2p5/q7/1PB5/8/P1P1NnPP/RNBQK2R w KQ -"));
    assert(MakeMove("b4a5"));
    assert(MakeMove("b7b5"));
    assert(MakeMove("a5b6"));
    assert(SetupPosition("8/2p5/3p4/KP5r/1R3pPk/8/4P3/8 b - g3 0 1"));
    assert(!MakeMove("f4g3"));
    assert(SetupPosition("8/8/3p4/KPp4r/1R3p1k/4P3/6P1/8 w - c6 0 2"));
    assert(!MakeMove("b5c6"));
    assert(SetupPosition("8/2p5/8/KP1p3r/1R2PpPk/8/8/8 b - g3 0 2"));
    assert(MakeMove("f4g3"));
    assert(SetupPosition("8/2p5/8/KP1p3r/1R2PpPk/8/8/8 b - e3 0 2"));
    assert(MakeMove("f4e3"));
    assert(SetupPosition("8/8/3p4/1Pp3r1/1K2Rp1k/8/4P1P1/8 w - c6 0 3"));
    assert(MakeMove("b5c6"));
    SetupPosition("8/5k2/8/3pP3/2B5/8/8/4K3 w - d6");
    assert(MakeMove("e5d6"));
    assert(!MakeMove("f7e6"));
    SetupPosition("8/3r4/5k2/8/4pP2/2K5/8/5R2 b - f3");
    assert(MakeMove("e4f3"));
    SetupPosition("8/8/K2p4/1Pp4r/1R3p1k/8/4P1P1/8 w - c6");
    assert(MakeMove("b5c6"));
    assert(MakeMove("h4g3"));
    assert(!IsLegal(MoveFromStr("a6a5")));
    assert(!IsLegal(MoveFromStr("a6b5")));

    assert(SetupPosition("3k2r1/2p5/8/3P4/8/8/K7/8 b - - 0 1"));
    assert(MakeMove("c7c5"));
    assert(MakeMove("d5c6"));
    TakebackMove();
    assert(MakeMove("d5d6"));
    assert(MakeMove("c5c4"));

    assert(SetupPosition("8/3n3R/2k1p1p1/1r1pP1P1/p2P3P/1NK5/8/8 w - - 0 1"));
    assert(MakeMove("b3a1"));
    assert(MakeMove("b5b1"));

    assert(SetupPosition("3k4/3r4/8/3b4/8/8/5P2/3K4 b - - 0 1"));
    assert(MakeMove("d5b3"));
    assert(!MakeMove("f2f4"));
    assert(MakeMove("d1e1"));

    assert(SetupPosition(("3k4/8/8/8/8/2n5/3P4/3K4 w - - 0 1")));
    assert(!MakeMove("d2d3"));
    assert(MakeMove("d2c3"));
    TakebackMove();
    assert(MakeMove("d1c2"));

    assert(SetupPosition("3k4/8/8/8/8/1b2N3/8/3K4 w - - 0 1"));
    assert(!MakeMove("e3d5"));
    assert(MakeMove("e3c2"));
    TakebackMove();
    assert(MakeMove("d1c1"));

    assert(SetupPosition("1n2k3/1pp5/8/8/8/1P6/2PPB3/4K3 w - - 0 1"));
    assert(MakeMove("e2b5"));
    assert(MakeMove("b8c6"));
    assert(MakeMove("b5a4"));
    assert(MakeMove("e8d7"));

    assert(SetupPosition("3q1rk1/3p4/8/8/8/8/3P4/3Q1RK1 b - - 0 1"));
    assert(MakeMove("f8f1"));
    assert(MakeMove("d1f1"));
    assert(MakeMove("d8f8"));
    assert(MakeMove("f1f8"));

    assert(SetupPosition("8/5k2/8/8/2n5/8/5P2/1N3K2 b - - 0 1"));
    assert(MakeMove("c4d2"));
    assert(MakeMove("b1d2"));
    assert(MakeMove("f7f6"));
    assert(MakeMove("f2f4"));

    assert(SetupPosition("8/2p2k2/8/K2P4/8/8/8/8 b - - 0 1"));
    assert(MakeMove("c7c5"));
    assert(MakeMove("d5c6"));
    assert(MakeMove("f7f6"));
    assert(MakeMove("a5b4"));

    assert(SetupPosition("6k1/8/8/8/8/8/8/4K2R w K - 0 1"));
    assert(MakeMove("e1g1"));
    assert(MakeMove("g8h8"));

    assert(SetupPosition("r3k3/8/8/8/8/8/8/1K6 b q - 0 1"));
    assert(MakeMove("e8c8"));
    assert(MakeMove("b1a1"));

    assert(SetupPosition("8/2pPk3/8/8/8/8/8/2K5 w - - 0 1"));
    assert(MakeMove("d7d8n"));
    assert(MakeMove("e7d8"));
    assert(MakeMove("c1c2"));
    assert(MakeMove("d8c8"));
    for(auto i = 0; i < 4; ++i)
        TakebackMove();

    assert(SetupPosition("4K3/2P5/2B5/8/b7/1p6/7P/3k4 w - - 0 1"));
    assert(MakeMove("c6a4"));
    TakebackMove();
    assert(!MakeMove("c6h1"));
    assert(MakeMove("c7c8"));
    assert(MakeMove("d1c1"));
    assert(MakeMove("c6a4"));
    assert(!MakeMove("b3a4"));
    for(auto i = 0; i < 3; ++i)
        TakebackMove();
    assert(MakeMove("c7c8r"));
    TakebackMove();

    assert(SetupPosition("4k3/8/8/b7/1r6/8/1P1K1N2/8 b - - 0 1"));
    assert(MakeMove("b4d4"));
    assert(!MakeMove("d2c3"));
    assert(!MakeMove("d2e1"));
    assert(!MakeMove("d2d3"));
    assert(!MakeMove("d2d1"));
    assert(!MakeMove("b2b4"));
    assert(!MakeMove("f2d3"));
    assert(MakeMove("d2c1"));
    TakebackMove();
    assert(MakeMove("d2c2"));
    TakebackMove();
    assert(MakeMove("d2e3"));
    TakebackMove();
    assert(MakeMove("d2e2"));

    assert(SetupPosition(
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -"));
    assert(MakeMove("d2d4"));
    assert(MakeMove("b2a1q"));
    assert(MakeMove("h6f7"));
    for(auto i = 0; i < 2; ++i)
        TakebackMove();
    assert(SetupPosition(
        "rnb2k1r/pp1PbBpp/1qp5/8/8/8/PPP1NnPP/RNBQK2R w KQ -"));
    assert(MakeMove("d7d8q"));
    assert(MakeMove("e7d8"));
    assert(MakeMove("a2a4"));
    assert(MakeMove("b6b4"));
    assert(!MakeMove("a4a5"));
    for(auto i = 0; i < 4; ++i)
        TakebackMove();

    assert(SetupPosition(
               "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ -"));
    assert(MakeMove("e1g1"));
    assert(MakeMove("f2d1"));
    assert(MakeMove("d7c8q"));
    TakebackMove();
    assert(b[coords[white][lower_bit_num(exist_mask[white])]] == white_pawn);
    assert(b[coords[white][higher_bit_num(exist_mask[white])]] == white_king);
    assert(b[coords[black][lower_bit_num(exist_mask[black])]] == black_pawn);
    assert(b[coords[black][higher_bit_num(exist_mask[black])]] == black_king);
}
#endif // NDEBUG

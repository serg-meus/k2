#include "chess.h"





//--------------------------------
k2chess::k2chess() :
    ray_min{0, 0, 0, 0, 4, 0, 0},
    ray_max{0, 8, 8, 4, 8, 8, 0},
    all_rays{0, 0xff, 0xff, 0x0f, 0xf0, 0xff, 0},
    delta_col_kqrb{ 0,  1,  0, -1,  1,  1, -1, -1},
    delta_row_kqrb{ 1,  0, -1,  0,  1, -1, -1,  1},
    delta_col_knight{ 1,  2,  2,  1, -1, -2, -2, -1},
    delta_row_knight{ 2,  1, -1, -2, -2, -1,  1,  2},
    is_slider{false, false, true, true, true, false, false},
    values{0, 15000, 1200, 600, 410, 400, 100}
{
    cv = cur_moves;
    memset(b_state, 0, sizeof(b_state));
    state = &b_state[prev_states];
    coords[black].board = b;
    coords[black].values = values;
    coords[white].board = b;
    coords[white].values = values;
    max_ray_length = std::max(static_cast<size_t>(board_height),
                              static_cast<size_t>(board_width));
    SetupPosition(start_position);
}





//--------------------------------
void k2chess::InitAttacks(const bool stm)
{
    memset(attacks[stm], 0, sizeof(attacks[0]));
    memset(mobility[stm], 0, sizeof(mobility[0]));
    for(auto it = coords[stm].rbegin(); it != coords[stm].rend(); ++it)
        InitAttacksOnePiece(*it, true);
}





//--------------------------------
void k2chess::InitAttacksOnePiece(const coord_t coord, const bool setbit)
{
    const auto color = get_color(b[coord]);
    const auto piece_id = find_piece(color, coord);
    const auto type = get_type(b[coord]);
    if(type == pawn)
        InitAttacksPawn(coord, color, piece_id, setbit);
    else
        InitAttacksNotPawn(coord, color, piece_id, type,
                           setbit, all_rays[type]);
}





//--------------------------------
void k2chess::InitAttacksPawn(const coord_t coord, const bool color,
                              const piece_id_t piece_id, const bool setbit)
{
    const auto col = get_col(coord);
    const auto row = get_row(coord);
    const auto d_row = color ? 1 : -1;
    if(col_within(col + 1))
    {
        if(setbit)
            set_bit(color, col + 1, row + d_row, piece_id);
        else
            clear_bit(color, col + 1, row + d_row, piece_id);

    }
    if(col_within(col - 1))
    {
        if(setbit)
            set_bit(color, col - 1, row + d_row, piece_id);
        else
            clear_bit(color, col - 1, row + d_row, piece_id);
    }
}





//--------------------------------
void k2chess::InitAttacksNotPawn(const coord_t coord, const bool color,
                                 const piece_id_t piece_id, const coord_t type,
                                 const bool setbit, ray_mask_t ray_mask)
{
    while(ray_mask)
    {
        const auto ray = __builtin_ctz(ray_mask);
        ray_mask ^= (1 << ray);
        if(!setbit)
            mobility[color][piece_id][ray] = 0;
        auto col = get_col(coord);
        auto row = get_row(coord);
        const auto d_col = type == knight ? delta_col_knight[ray] :
                                            delta_col_kqrb[ray];
        const auto d_row = type == knight ? delta_row_knight[ray] :
                                            delta_row_kqrb[ray];
        size_t i = 0;
        for(; i < max_ray_length; ++i)
        {
            col += d_col;
            row += d_row;
            if(!col_within(col) || !row_within(row))
                break;
            if(setbit)
            {
                set_bit(color, col, row, piece_id);
                const auto sq = b[get_coord(col, row)];
                if(sq == empty_square || get_color(sq) != color)
                    mobility[color][piece_id][ray]++;
                if(b[get_coord(col, row)] != empty_square)
                    break;
            }
            else
                clear_bit(color, col, row, piece_id);
            if(!is_slider[type])
                break;
        }
    }
}





//--------------------------------
void k2chess::UpdateMasks(const move_c move, const attack_params_s &p)
{
    update_mask[black] = 0;
    update_mask[white] = 0;
    update_mask[!wtm] |= (1 << move.piece_id);
    if(move.flag & is_castle)
    {
        update_mask[!wtm] |= (1 << p.castled_rook_id);
        const auto from_col = move.flag == is_right_castle ? max_col : 0;
        const auto from_row = wtm ? max_row : 0;
        const auto rook_from_coord = get_coord(from_col, from_row);
        for(auto stm : {black, white})
        {
            update_mask[stm] |= attacks[stm][rook_from_coord];
            update_mask[stm] |=
                    attacks[stm][*coords[!wtm].at(p.castled_rook_id)];
        }
    }
    if(move.flag & is_capture)
        update_mask[wtm] |= (1 << p.captured_id);

    for(auto stm : {black, white})
    {
        update_mask[stm] |= attacks[stm][p.from_coord];
        update_mask[stm] |= attacks[stm][move.to_coord];
        if(p.is_en_passant)
            update_mask[stm] |= attacks[stm]
                    [move.to_coord + (wtm ? board_width : -board_width)];
    }
}





//--------------------------------
void k2chess::GetAttackParams(const piece_id_t piece_id, const move_c move,
                              const bool stm, attack_params_s &p)
{
    p.piece_coord = *coords[stm].at(piece_id);
    p.type = get_type(b[p.piece_coord]);
    p.is_captured = (move.flag & is_capture) && stm == wtm
            && p.piece_coord == *coords[wtm].at(p.captured_id);
    p.piece_id = piece_id;
    if(p.is_captured)
    {
        p.color = stm;
        p.type = p.is_en_passant ? pawn : get_type(state[ply].captured_piece);
        p.piece_id = p.captured_id;
    }
    p.is_move = stm != wtm && piece_id == move.piece_id;
    if(p.is_move && (move.flag & is_promotion))
        p.type = pawn;
    p.is_castle = (move.flag & is_castle) && stm != wtm
            && piece_id == p.castled_rook_id;
    if(p.is_castle)
        p.piece_coord = get_coord((move.flag & is_left_castle) ?
                                    0 : max_col, wtm ? max_row : 0);
    p.is_special_move = p.is_captured || p.is_en_passant ||
            (move.flag & is_castle);
}





//--------------------------------
void k2chess::UpdateAttacks(const move_c move, const coord_t from_coord)
{
    attack_params_s p;
    p.from_coord = from_coord;
    p.to_coord = move.to_coord;
    p.castled_rook_id = state[ply].castled_rook_id;
    p.captured_id = (move.flag & is_capture) ? state[ply].captured_id :
                                                  not_a_capture;
    p.is_en_passant = move.flag & is_en_passant;
    UpdateMasks(move, p);
    for(auto stm : {black, white})
    {
        p.color = stm;
        auto msk = update_mask[stm];
        if(!msk)
            continue;
        while(msk)
        {
            const auto piece_id = __builtin_ctz(msk);
            msk ^= (1 << piece_id);
            GetAttackParams(piece_id, move, stm, p);
            p.set_attack_bit = false;
            UpdateAttacksOnePiece(p);
            if(p.is_castle)
                p.piece_coord = *coords[!wtm].at(piece_id);
            else if(p.is_move && (move.flag & is_promotion))
                p.type = get_type(b[*coords[!wtm].at(p.piece_id)]);
            p.set_attack_bit = true;
            if(!p.is_captured)
                UpdateAttacksOnePiece(p);
        }
    }
//    assert(CheckBoardConsistency());
}





//--------------------------------
bool k2chess::CheckBoardConsistency()
{
    bool ans = true;
    attack_t tmp_a[sides][board_height*board_width];
    const auto sz_a = sizeof(attacks);
    memcpy(tmp_a, attacks, sz_a);
    coord_t tmp_m[sides][max_pieces_one_side][max_rays];
    const auto sz_m = sizeof(mobility);
    memcpy(tmp_m, mobility, sz_m);
    attack_t tmp_s[sides];
    memcpy(tmp_s, slider_mask, sizeof(slider_mask));
    InitSliderMask(white);
    InitSliderMask(black);
    InitAttacks(white);
    InitAttacks(black);
    if(memcmp(tmp_a, attacks, sz_a) != 0)
        ans = false;
    if(memcmp(tmp_m, mobility, sz_m) != 0)
        ans = false;
    if((slider_mask[black] & ~tmp_s[black]) != 0)
        ans = false;
    if((slider_mask[white] & ~tmp_s[white]) != 0)
        ans = false;
    memcpy(slider_mask, tmp_s, sizeof(slider_mask));
    return ans;
}




//--------------------------------
void k2chess::UpdateAttacksOnePiece(attack_params_s &p)
{
    assert(p.type <= pawn);
    assert(p.piece_id <= attack_digits);
    auto coord = p.piece_coord;
    if(p.is_move && !p.set_attack_bit)
        coord = p.from_coord;
    if(p.type == pawn)
        InitAttacksPawn(coord, p.color, p.piece_id, p.set_attack_bit);
    else if(is_slider[p.type])
        InitAttacksSlider(coord, p);
    else
        InitAttacksNotPawn(coord, p.color, p.piece_id, p.type,
                           p.set_attack_bit, GetNonSliderMask(p));
}





//--------------------------------
void k2chess::InitAttacksSlider(coord_t coord, attack_params_s &p)
{
    const auto ray_mask = GetRayMask(p);
    InitAttacksNotPawn(coord, p.color, p.piece_id, p.type,
                       p.set_attack_bit, ray_mask);
    if(!p.is_move)
        return;

    coord = p.set_attack_bit ? p.from_coord : p.piece_coord;
    if(p.set_attack_bit)
        set_bit(p.color, get_col(coord), get_row(coord), p.piece_id);
    else
        clear_bit(p.color, get_col(coord), get_row(coord), p.piece_id);

    if(p.set_attack_bit)
        InitMobilitySlider(p);
}





//--------------------------------
void k2chess::InitMobilitySlider(attack_params_s &p)
{
    enum {ray_n, ray_e, ray_s, ray_w,
          ray_ne, ray_se, ray_sw, ray_nw};
    const auto d_col = get_col(p.to_coord) - get_col(p.from_coord);
    const auto d_row = get_row(p.to_coord) - get_row(p.from_coord);
    if(d_col == 0)
    {
        mobility[p.color][p.piece_id][ray_n] -= d_row;
        mobility[p.color][p.piece_id][ray_s] += d_row;
    }
    else if(d_row == 0)
    {
        mobility[p.color][p.piece_id][ray_e] -= d_col;
        mobility[p.color][p.piece_id][ray_w] += d_col;
    }
    else if(sgn(d_col) == sgn(d_row))
    {
        mobility[p.color][p.piece_id][ray_ne] -= d_col;
        mobility[p.color][p.piece_id][ray_sw] += d_col;
    }
    else
    {
        mobility[p.color][p.piece_id][ray_se] -= d_col;
        mobility[p.color][p.piece_id][ray_nw] += d_col;
    }
}



//--------------------------------
k2chess::ray_mask_t k2chess::GetRayMask(attack_params_s &p) const
{
    ray_mask_t ans;
    if(p.is_special_move)
        return all_rays[p.type];
    if(p.set_attack_bit)
    {
        ans = p.ray_mask;
        const ray_mask_t capture_mask[] = {rays_SW, rays_South, rays_SE,
                                           rays_West, 0, rays_East,
                                           rays_NW, rays_North, rays_NE};
        if(p.is_move && p.captured_id != not_a_capture && !p.is_en_passant)
            ans |= capture_mask[p.ray_id];
        return ans;
    }

    if(!p.is_move)
    {
        ans = GetRayMaskNotForMove(p.to_coord, p.piece_coord);
        ans |= GetRayMaskNotForMove(p.from_coord, p.piece_coord);
        p.ray_mask = ans;
        return ans;
    }
    coord_t move_type;
    const auto piece_id = GetRayId(p.from_coord, p.to_coord, &move_type);

    const ray_mask_t RB_masks[] = {rays_NW_or_SE, rays_E_or_W, rays_NE_or_SW,
                                   rays_N_or_S, 0, rays_N_or_S,
                                   rays_NE_or_SW, rays_E_or_W, rays_NW_or_SE};
    ans = RB_masks[piece_id];
    assert(ans);

    const ray_mask_t Q_masks[] = {rays_rook, rays_bishop, rays_rook,
                                  rays_bishop, 0, rays_bishop,
                                  rays_rook, rays_bishop, rays_rook};
    if(get_type(b[p.to_coord]) == queen)
        ans |= Q_masks[piece_id];

    p.ray_mask = ans;
    p.ray_id = piece_id;
    return ans;
}





//--------------------------------
k2chess::ray_mask_t k2chess::GetRayMaskNotForMove(const coord_t target_coord,
                                                  const coord_t piece_coord)
                                                  const
{
    const ray_mask_t masks[] = {rays_NE, rays_North, rays_NW,
                                rays_East, 0, rays_West,
                                rays_SE, rays_South, rays_SW};
    coord_t move_type;
    auto piece_id = GetRayId(target_coord, piece_coord, &move_type);
    const auto type = get_type(b[piece_coord]);
    if(move_type == 0 || (type != queen && move_type != type))
        piece_id = 4;
    return masks[piece_id];
}





//--------------------------------
k2chess::piece_id_t k2chess::GetRayId(const coord_t from_coord,
                                      const coord_t to_coord,
                                      coord_t *move_type) const
{
    const auto delta_col = get_col(to_coord) - get_col(from_coord);
    const auto delta_row = get_row(to_coord) - get_row(from_coord);
    *move_type = 0;
    if(delta_col == 0 || delta_row == 0)
        *move_type = rook;
    else if(delta_col == delta_row || delta_col == -delta_row)
        *move_type = bishop;
    return 3*(1 + sgn(delta_row)) + 1 + sgn(delta_col);
}





//--------------------------------
k2chess::ray_mask_t k2chess::GetNonSliderMask(const attack_params_s &p) const
{
    if(p.is_move || p.is_captured || p.is_special_move)
        return all_rays[p.type];
    if(p.type == knight)
        return GetKnightMask(p.piece_coord, p.from_coord) |
                GetKnightMask(p.piece_coord, p.to_coord);

    return GetKingMask(p.piece_coord, p.from_coord) |
            GetKingMask(p.piece_coord, p.to_coord);
}






//--------------------------------
k2chess::ray_mask_t k2chess::GetKingMask(const coord_t piece_coord,
                                            const coord_t to_coord) const
{
    const auto delta_col = get_col(to_coord) - get_col(piece_coord);
    const auto delta_row = get_row(to_coord) - get_row(piece_coord);
    if(std::abs(delta_col) > 1 || std::abs(delta_row) > 1)
        return 0;
    const ray_mask_t masks[] = {rays_NE, rays_North, rays_NW,
                                rays_East, 0, rays_West,
                                rays_SE, rays_South, rays_SW};
    coord_t move_type;
    const auto ray_id = GetRayId(to_coord, piece_coord, &move_type);
    return masks[ray_id];
}





//--------------------------------
k2chess::ray_mask_t k2chess::GetKnightMask(const coord_t piece_coord,
                                            const coord_t to_coord) const
{
    const auto delta_col = get_col(to_coord) - get_col(piece_coord);
    const auto delta_row = get_row(to_coord) - get_row(piece_coord);
    const auto abs_dcol = std::abs(delta_col);
    const auto abs_drow = std::abs(delta_row);
    if(abs_dcol + abs_drow != 3 || (abs_dcol != 1 && abs_drow != 1))
        return 0;
    if(delta_col == 2)
        return delta_row == 1 ? rays_NEE : rays_SEE;
    else if(delta_col == 1)
        return delta_row == 2 ? rays_NNE : rays_SSE;
    else if(delta_col == -1)
        return delta_row == 2 ? rays_NNW : rays_SSW;
    else if(delta_col == -2)
        return delta_row == 1 ? rays_NWW : rays_SWW;
    else
        return 0;
}





//--------------------------------
void k2chess::InitSliderMask(bool stm)
{
    slider_mask[stm] = 0;
    for(auto it = coords[stm].rbegin(); it != coords[stm].rend(); ++it)
    {
        const auto piece = b[*it];
        assert(piece > empty_square);
        assert(piece <= white_pawn);
        if(is_slider[get_type(piece)])
            slider_mask[stm] |= (1 << it.get_array_index());
    }
}





//--------------------------------
bool k2chess::InitPieceLists()
{
    coords[black].clear();
    coords[white].clear();
    for(size_t i = 0; i < sizeof(b)/sizeof(*b); i++)
    {
        if(b[i] == empty_square)
            continue;
        coords[get_color(b[i])].push_back(i);

        const auto color = get_color(b[i]);
        const auto type = get_type(b[i]);
        quantity[color][type]++;
    }

    coords[black].sort();
    coords[white].sort();

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
    InitPieceLists();
    for(auto stm : {black, white})
    {
        king_coord[stm] = --coords[stm].end();
        InitAttacks(stm);
        InitSliderMask(stm);
        update_mask[stm] = 0;
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
void k2chess::ShowMove(const coord_t from_coord, const coord_t to_coord)
{
    assert(ply <= max_ply);
    char *cur = cur_moves + move_max_display_length*(ply - 1);
    *(cur++) = get_col(from_coord) + 'a';
    *(cur++) = get_row(from_coord) + '1';
    *(cur++) = get_col(to_coord) + 'a';
    *(cur++) = get_row(to_coord) + '1';
    *(cur++) = ' ';
    *cur = '\0';
}





//--------------------------------
void k2chess::StoreCurrentBoardState(const move_c move,
                                     const coord_t from_coord)
{
    state[ply].castling_rights = state[ply - 1].castling_rights;
    state[ply].captured_piece = b[move.to_coord];

    state[ply].from_coord = from_coord;
    state[ply].reversible_moves = reversible_moves;
    state[ply].move = move;
}





//--------------------------------
void k2chess::MoveToCoordinateNotation(const move_c move, const bool stm,
                        char * const out)
{
    char proms[] = {'?', 'q', 'n', 'r', 'b'};

    const auto it = coords[stm].at(move.piece_id);
    const auto from_coord = *it;
    out[0] = get_col(from_coord) + 'a';
    out[1] = get_row(from_coord) + '1';
    out[2] = get_col(move.to_coord) + 'a';
    out[3] = get_row(move.to_coord) + '1';
    out[4] = (move.flag & is_promotion) ?
              proms[move.flag & is_promotion] : '\0';
    out[5] = '\0';
}





//--------------------------------
bool k2chess::MakeCastleOrUpdateFlags(const move_c move,
                                      const coord_t from_coord)
{
    const auto old_rights = state[ply].castling_rights;
    auto &new_rights = state[ply].castling_rights;
    if(move.piece_id == king_coord[wtm].get_array_index())  // king moves
    {
        if(wtm)
            new_rights &= ~(white_can_castle_right | white_can_castle_left);
        else
            new_rights &= ~(black_can_castle_right | black_can_castle_left);
    }
    else if(b[from_coord] == set_color(black_rook, wtm))  // rook moves
    {
        const auto col = get_col(from_coord);
        const auto row = get_row(from_coord);
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
    const auto piece_id = find_piece(wtm, rook_from_coord);
    assert(piece_id != piece_not_found);
    state[ply].castled_rook_id = piece_id;
    b[rook_to_coord] = b[rook_from_coord];
    b[rook_from_coord] = empty_square;
    *coords[wtm].at(piece_id) = rook_to_coord;

    reversible_moves = 0;
    return false;
}





//--------------------------------
void k2chess::TakebackCastle(const move_c move)
{
    const auto rook_iterator = coords[wtm].at(state[ply].castled_rook_id);
    const auto from_coord =*rook_iterator;
    const auto to_col = move.flag == is_right_castle ? max_col : 0;
    const auto to_row = wtm ? 0 : max_row;
    const auto to_coord = get_coord(to_col, to_row);
    b[to_coord] = b[from_coord];
    b[from_coord] = empty_square;
    *rook_iterator = to_coord;
}





//--------------------------------
bool k2chess::MakeEnPassantOrUpdateFlags(const move_c move,
                                         const coord_t from_coord)
{
    const shifts_t delta = get_row(move.to_coord) - get_row(from_coord);
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
        b[move.to_coord + (wtm ? -board_width : board_width)] = empty_square;
    return false;
}





//--------------------------------
void k2chess::MakeCapture(const move_c move)
{
    auto to_coord = move.to_coord;
    if (move.flag & is_en_passant)
    {
        to_coord += wtm ? -board_width : board_width;
        material[!wtm] -= values[pawn];
        quantity[!wtm][pawn]--;
    }
    else
    {
        const auto piece_id = get_type(state[ply].captured_piece);
        material[!wtm] -= values[piece_id];
        quantity[!wtm][piece_id]--;
    }
    const auto piece_id = find_piece(!wtm, to_coord);
    assert(piece_id != piece_not_found);
    state[ply].captured_id = piece_id;
    coords[!wtm].erase(piece_id);
    pieces[!wtm]--;
    reversible_moves = 0;
}





//--------------------------------
void k2chess::TakebackCapture(const move_c move)
{
    const auto it_cap = coords[!wtm].at(state[ply].captured_id);

    if(move.flag & is_en_passant)
    {
        material[!wtm] += values[pawn];
        quantity[!wtm][pawn]++;
        if(wtm)
        {
            b[move.to_coord - board_width] = black_pawn;
            *it_cap = move.to_coord - board_width;
        }
        else
        {
            b[move.to_coord + board_width] = white_pawn;
            *it_cap = move.to_coord + board_width;
        }
    }
    else
    {
        const auto captured_id = get_type(state[ply].captured_piece);
        material[!wtm] += values[captured_id];
        quantity[!wtm][captured_id]++;
    }

    coords[!wtm].restore(it_cap);
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
    store_coords.push_back(coords[wtm]);
    coords[wtm].sort();
    state[ply].slider_mask = slider_mask[wtm];
    InitSliderMask(wtm);
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
bool k2chess::MakeMove(const move_c move)
{
    bool is_special_move = false;
    ply++;
    const auto it = coords[wtm].at(move.piece_id);
    const auto from_coord = *it;
    StoreCurrentBoardState(move, from_coord);
    reversible_moves++;
    if(move.flag & is_capture)
        MakeCapture(move);

    // trick: fast exit if not K|Q|R moves, and no captures
    if((b[from_coord] <= white_rook) || (move.flag & is_capture))
        is_special_move |= MakeCastleOrUpdateFlags(move, from_coord);

    state[ply].en_passant_rights = 0;
    if(get_type(b[from_coord]) == pawn)
    {
        is_special_move |= MakeEnPassantOrUpdateFlags(move, from_coord);
        reversible_moves = 0;
    }
#ifndef NDEBUG
    ShowMove(from_coord, move.to_coord);
#endif // NDEBUG

    b[move.to_coord] = b[from_coord];
    b[from_coord] = empty_square;
    *it = move.to_coord;

    if(move.flag & is_promotion)
        MakePromotion(move);
    wtm = !wtm;

    state[ply].attacks_updated = false;
    return is_special_move;
}





//--------------------------------
void k2chess::MakeAttacks(const move_c move)
{
    if(state[ply].attacks_updated)
        return;
    memcpy(done_attacks[ply - 1], attacks, sizeof(attacks));
    memcpy(done_mobility[ply - 1], mobility, sizeof(mobility));

    if(move.flag & is_promotion)
    {
        InitAttacks(!wtm);
        InitAttacks(wtm);
    }
    else
    {
        const auto from_coord = state[ply].from_coord;
        UpdateAttacks(move, from_coord);
    }
    state[ply].attacks_updated = true;
}





//--------------------------------
void k2chess::TakebackMove()
{
    const auto move = state[ply].move;
    TakebackMove(move);
    TakebackAttacks();
}





//--------------------------------
void k2chess::TakebackMove(move_c move)
{
    if(move.flag & is_promotion)
    {
        coords[!wtm] = store_coords.back();
        store_coords.pop_back();
    }
    const auto from_coord = state[ply].from_coord;
    const auto it = coords[!wtm].at(move.piece_id);
    *it = from_coord;

    if(move.flag & is_promotion)
        b[from_coord] = set_color(white_pawn, !wtm);
    else
        b[from_coord] = b[move.to_coord];

    b[move.to_coord] = state[ply].captured_piece;
    reversible_moves = state[ply].reversible_moves;
    wtm = !wtm;

    if(move.flag & is_capture)
        TakebackCapture(move);

    if(move.flag & is_promotion)
        TakebackPromotion(move);
    else if(move.flag & is_castle)
        TakebackCastle(move);

    ply--;
#ifndef NDEBUG
    cur_moves[move_max_display_length*ply] = '\0';
#endif // NDEBUG
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
    MakeAttacks(move);
    return true;
}





//--------------------------------
k2chess::piece_id_t k2chess::find_piece(const bool stm, const coord_t coord)
{
    auto it = coords[stm].begin();
    const auto it_end = coords[stm].end();
    for(; it != it_end; ++it)
        if(*it == coord)
            break;
    if(it == coords[stm].end())
        return piece_not_found;
    return it.get_array_index();
}





//--------------------------------
k2chess::move_flag_t k2chess::InitMoveFlag(const move_c move,
                                           const char promo_to)
{
    move_flag_t ans = 0;
    const auto it = coords[wtm].at(move.piece_id);
    const auto from_coord = *it;
    const auto piece = get_type(b[from_coord]);
    if(b[move.to_coord] != empty_square
            && get_color(b[from_coord])
            != get_color(b[move.to_coord]))
        ans |= is_capture;
    if(piece == pawn
            && get_row(from_coord) == (wtm ? board_height - 2 : 1))
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
        const auto delta = (en_passant_state - 1) - get_col(from_coord);
        auto required_row = pawn_default_row + pawn_long_move_length;
        if(wtm)
            required_row = max_row - required_row;
        if(en_passant_state && std::abs(delta) == 1
                && get_col(move.to_coord) != get_col(from_coord)
                && get_row(from_coord) == required_row)
            ans |= is_en_passant | is_capture;
    }
    if(piece == king && get_col(*king_coord[wtm]) == default_king_col)
    {
        if(get_col(move.to_coord) == default_king_col + cstl_move_length)
            ans |= is_right_castle;
        else if(get_col(move.to_coord) == default_king_col - cstl_move_length)
            ans |= is_left_castle;
    }

    return ans;
}





//--------------------------------
bool k2chess::IsPseudoLegal(const move_c move)
{
    auto it = coords[wtm].begin();
    for(; it != coords[wtm].end(); ++it)
        if(it.get_array_index() == move.piece_id)
            break;
    if(it == coords[wtm].end())
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

    const auto from_coord = *it;
    if(from_coord == move.to_coord)
        return false;
    if(move.flag & is_castle && get_type(b[from_coord]) != king)
        return false;
    const auto piece = b[*it];
    if(piece == empty_square || get_color(piece) != wtm)
        return false;
    const auto type = get_type(piece);
    if(type == pawn)
        return IsPseudoLegalPawn(move, from_coord);
    if(move.flag & is_promotion || move.flag & is_en_passant)
        return false;
    if(is_slider[type])
    {
        const auto fr_col = get_col(from_coord);
        const auto to_col = get_col(move.to_coord);
        const auto fr_row = get_row(from_coord);
        const auto to_row = get_row(move.to_coord);
        if(type == bishop && std::abs(fr_col - to_col) !=
                std::abs(fr_row - to_row))
            return false;
        if(type == rook && fr_col != to_col && fr_row != to_row)
            return false;
        return IsSliderAttack(from_coord, move.to_coord);
    }
    else if(type == king)
        return IsPseudoLegalKing(move, from_coord);
    else if(type == knight)
        return IsPseudoLegalKnight(move, from_coord);
    else
        return false;
}





//--------------------------------
bool k2chess::IsPseudoLegalKing(const move_c move,
                                const coord_t from_coord) const
{
    const auto fr_col = get_col(from_coord);
    const auto fr_row = get_row(from_coord);
    const auto to_col = get_col(move.to_coord);
    const auto to_row = get_row(move.to_coord);

    if(std::abs(fr_col - to_col) <= 1 && std::abs(fr_row - to_row) <= 1
       && !(move.flag & is_castle))
        return true;

    if(fr_col == default_king_col && get_color(b[from_coord]) == wtm &&
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
bool k2chess::IsPseudoLegalKnight(const move_c move,
                                  const coord_t from_coord) const
{
    const auto d_col = std::abs(get_col(from_coord) - get_col(move.to_coord));
    const auto d_row = std::abs(get_row(from_coord) - get_row(move.to_coord));
    return d_col + d_row == 3 && (d_col == 1 || d_row == 1);
}





//--------------------------------
bool k2chess::IsPseudoLegalPawn(const move_c move,
                                const coord_t from_coord) const
{
    const auto delta = wtm ? board_width : -board_width;
    const auto is_special = is_en_passant | is_capture | is_castle;
    const auto from_row = get_row(from_coord);
    const auto last_row = wtm ? max_row - 1 : 1;
    if((move.flag & is_promotion) && from_row != last_row)
        return false;
    if(!(move.flag & is_promotion) && from_row == last_row)
        return false;
    if((move.flag & is_promotion) > is_promotion_to_bishop)
        return false;
    if(move.flag & is_promotion && move.flag & (is_castle | is_en_passant))
        return false;
    if(move.to_coord == from_coord + delta
            && b[move.to_coord] == empty_square
            && !(move.flag & is_special))
        return true;
    const auto init_row = (wtm ? pawn_default_row :
                                 max_row - pawn_default_row);
    const auto from_col = get_col(from_coord);
    if(move.to_coord == from_coord + 2*delta
            && get_row(from_coord) == init_row
            && b[move.to_coord] == empty_square
            && b[move.to_coord - delta] == empty_square
            && !(move.flag & is_special) && !(move.flag & is_promotion))
        return true;
    else if((move.to_coord == from_coord + delta + 1 && from_col < max_col)
        || (move.to_coord == from_coord + delta - 1 && from_col > 0))
    {
        if(b[move.to_coord] != empty_square && move.flag & is_capture
                && !(move.flag & is_en_passant))
        {
            if(get_color(b[move.to_coord]) != wtm)
                return true;
        }
        else if(move.flag == (is_en_passant | is_capture))
        {
            const auto col = get_col(from_coord);
            auto row = pawn_default_row + pawn_long_move_length;
            if(wtm)
                row = max_row - row;
            const auto delta2 = move.to_coord - from_coord - delta;
            const auto rights = state[ply].en_passant_rights;
            const auto pw = set_color(black_pawn, !wtm);
            if(get_row(from_coord) == row && rights - 1 == col + delta2
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

    const auto dx1 = get_col(ray_coord2) - get_col(ray_coord1);
    const auto dy1 = get_row(ray_coord2) - get_row(ray_coord1);
    const auto dx2 = get_col(given) - get_col(ray_coord1);
    const auto dy2 = get_row(given) - get_row(ray_coord1);

    if(dx1 != 0 && dy1 != 0 && std::abs(dx1) != std::abs(dy1))
        return false;
    if(dx2 != 0 && dy2 != 0 && std::abs(dx2) != std::abs(dy2))
        return false;
    if(sgn(dx1) != sgn(dx2) || sgn(dy1) != sgn(dy2))
        return false;

    return true;
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
bool k2chess::IsDiscoveredAttack(const coord_t fr_coord,
                                 const coord_t to_coord,
                                 attack_t mask)
{
    while(mask)
    {
        const auto piece_id = __builtin_ctz(mask);
        mask ^= (1 << piece_id);
        const auto attacker_coord = *coords[!wtm].at(piece_id);
        const auto k_coord = *king_coord[wtm];
        if(fr_coord == to_coord)  // special for en_passant case
        {
            const auto type = get_type(b[attacker_coord]);
            if(b[attacker_coord] == empty_square || !is_slider[type]
                    || get_color(b[attacker_coord]) == wtm)
                continue;
            if(!IsOnRay(fr_coord, k_coord, attacker_coord))
                continue;
            const auto p_coord = get_coord(state[ply].en_passant_rights - 1,
                                           get_row(fr_coord));
            const auto tmp1 = b[p_coord];
            const auto tmp2 = b[fr_coord];
            b[p_coord] = empty_square;
            b[fr_coord] = empty_square;
            const bool check = IsSliderAttack(k_coord, attacker_coord);
            b[p_coord] = tmp1;
            b[fr_coord] = tmp2;
            if(!check)
                continue;
            const auto d_col = get_col(attacker_coord) - get_col(k_coord);
            const auto d_row = get_row(attacker_coord) - get_row(k_coord);
            if(type == bishop && (d_col == 0 || d_row == 0))
                continue;
            if(type == rook && d_col != 0 && d_row != 0)
                continue;
        }
        else
        {
            if(!IsOnRay(fr_coord, k_coord, attacker_coord))
                continue;
            if(IsOnRay(to_coord, k_coord, attacker_coord))
                continue;
            if(!IsSliderAttack(fr_coord, k_coord))
                continue;
        }
        return true;
    }
    return false;
}





//--------------------------------
bool k2chess::IsLegalKingMove(const move_c move, coord_t from_coord)
{
    if(move.flag & is_castle)
        return IsLegalCastle(move);
    if(attacks[!wtm][move.to_coord] != 0)
        return false;

    auto att_mask = attacks[!wtm][from_coord] & slider_mask[!wtm];
    while(att_mask)
    {
        const auto piece_id = __builtin_ctz(att_mask);
        att_mask ^= (1 << piece_id);
        const auto attacker_coord = *coords[!wtm].at(piece_id);
        if(IsOnRay(from_coord, move.to_coord, attacker_coord))
            return false;
    }
    return true;
}





//--------------------------------
bool k2chess::IsLegal(const move_c move)
{
    const auto from_coord = *coords[wtm].at(move.piece_id);
    const auto piece_type = get_type(b[from_coord]);
    if(piece_type == king)
        return IsLegalKingMove(move, from_coord);
    if(move.flag & is_en_passant &&
            IsDiscoveredAttack(from_coord, from_coord, slider_mask[!wtm]))
        return false;
    const auto k_crd = *king_coord[wtm];
    const auto attack_sum = attacks[!wtm][k_crd];
    const auto king_attackers =
            std::bitset<attack_digits>(attack_sum).count();
    assert(king_attackers <= 2);
    if(king_attackers == 2)
        return false;
    const auto att_mask = attacks[!wtm][from_coord] & slider_mask[!wtm];
    if(att_mask && IsDiscoveredAttack(from_coord, move.to_coord, att_mask))
        return false;
    if(king_attackers == 0)
        return true;

    const auto attacker_id = __builtin_ctz(attack_sum);
    const auto attacker_coord = *coords[!wtm].at(attacker_id);
    if(is_slider[get_type(b[attacker_coord])])
        return IsOnRay(move.to_coord, k_crd, attacker_coord);

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
            MoveToCoordinateNotation(cur_move, wtm, move_str);
        else
            MoveToAlgebraicNotation(cur_move, wtm, move_str);
        std::cout << move_str << " ";
        MakeMove(cur_move);
        MakeAttacks(cur_move);
    }
    for(; i > 0; --i)
    {
        TakebackMove(moves[i - 1]);
        TakebackAttacks();
    }
    return success;
}





//-----------------------------
void k2chess::MoveToAlgebraicNotation(const move_c move,
                                              const bool stm,
                                              char *out)
{
    char pc2chr[] = "??KKQQRRBBNNPP";
    const auto from_coord = *coords[stm].at(move.piece_id);
    const auto piece_char = pc2chr[b[from_coord]];
    if(piece_char == 'K' && get_col(from_coord) == 4
            && get_col(move.to_coord) == 6)
    {
        *(out++) = 'O';
        *(out++) = 'O';
    }
    else if(piece_char == 'K' && get_col(from_coord) == 4
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
        const auto tmp = coords[stm].at(move.piece_id);
        *(out++) = get_col(*tmp) + 'a';
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
void k2chess::ProcessAmbiguousNotation(const move_c move, char *out)
{
    move_c move_array[8];
    auto amb_cr = 0;
    auto it = coords[wtm].at(move.piece_id);
    const auto init_from_coord = *it;
    const auto init_piece_type = get_type(b[init_from_coord]);

    for(it = coords[wtm].begin(); it != coords[wtm].end(); ++it)
    {
        const auto piece_id = it.get_array_index();
        if(piece_id == move.piece_id)
            continue;
        const auto from_coord = *it;

        const auto type = get_type(b[from_coord]);
        if(type != init_piece_type)
            continue;
        if(!(attacks[wtm][move.to_coord] & (1 << piece_id)))
            continue;

        auto tmp = move;
        tmp.priority = from_coord;
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
size_t k2chess::test_count_attacked_squares(const bool stm)
{
    size_t ans = 0;
    for(auto it : attacks[stm])
        if(it != 0)
            ans++;
    return ans;
}





//--------------------------------
size_t k2chess::test_count_all_attacks(const bool stm)
{
    size_t ans = 0;
    for(auto it : attacks[stm])
        ans += std::bitset<attack_digits>(it).count();
    return ans;
}





//--------------------------------
void k2chess::test_attack_tables(const size_t att_w, const size_t att_b)
{
    size_t att_squares_w, att_squares_b;
    att_squares_w = test_count_attacked_squares(white);
    att_squares_b = test_count_attacked_squares(black);
    assert(att_squares_w == att_w);
    assert(att_squares_b == att_b);
}





//--------------------------------
size_t k2chess::test_mobility(const bool color)
{
    size_t ans = 0;
    for(auto piece_id = 0; piece_id < max_pieces_one_side; ++piece_id)
        for(auto ray = 0; ray < max_rays; ++ray)
            ans += mobility[color][piece_id][ray];
    return ans;
}





//--------------------------------
void k2chess::RunUnitTests()
{
    assert(SetupPosition(start_position));
    test_attack_tables(22, 22);
    assert(test_mobility(black) == 4);
    assert(test_mobility(white) == 4);

    assert(SetupPosition("4k3/8/5n2/5n2/8/8/8/3RK3 w - - 0 1"));
    test_attack_tables(15, 19);
    assert(test_mobility(black) == 20);
    assert(test_mobility(white) == 14);

    assert(SetupPosition(
               "2r2rk1/p4q2/1p2b3/1n6/1N6/1P2B3/P4Q2/2R2RK1 w - - 0 1"));
    test_attack_tables(39, 39);
    assert(test_mobility(black) == 42);
    assert(test_mobility(white) == 42);

    assert(SetupPosition(
               "2k1r2r/1pp3pp/p2b4/2p1n2q/6b1/1NQ1B3/PPP2PPP/R3RNK1 b - -"
               "0 1"));
    test_attack_tables(32, 38);
    assert(test_mobility(black) == 34);
    assert(test_mobility(white) == 30);

    assert(SetupPosition(start_position));
    assert(b[*coords[white].begin()] == white_pawn);
    assert(b[*(--coords[white].end())] == white_king);
    assert(b[*coords[white].rbegin()] == white_king);
    assert(b[*(--coords[white].rend())] == white_pawn);
    assert(b[*coords[black].begin()] == black_pawn);
    assert(b[*(--coords[black].end())] == black_king);
    assert(b[*coords[black].rbegin()] == black_king);
    assert(b[*(--coords[black].rend())] == black_pawn);
    assert(*king_coord[white] == get_coord(default_king_col, 0));
    assert(*king_coord[black] ==
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
    assert(find_piece(black, get_coord("d5")) == piece_not_found);
    assert(test_mobility(black) == 18);
    assert(test_mobility(white) == 28);

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
    assert(reversible_moves == 0);

    assert(MakeMove("e8c8"));
    assert(b[get_coord("e8")] == empty_square);
    assert(b[get_coord("c8")] == black_king);
    assert(b[get_coord("a8")] == empty_square);
    assert(b[get_coord("d8")] == black_rook);
    assert(reversible_moves == 0);

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

    assert(SetupPosition("2n1r3/3P4/7k/8/8/8/8/K7 w - - 4 1"));
    assert(MakeMove("d7d8q"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("d8")] == white_queen);
    assert(quantity[white][pawn] == 0);
    assert(quantity[white][queen] = 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 3);
    assert(material[white] == values[queen]);
    assert(material[black] == values[knight] + values[rook]);
    assert(reversible_moves == 0);

    TakebackMove();
    assert(b[get_coord("d7")] == white_pawn);
    assert(b[get_coord("d8")] == empty_square);
    assert(material[white] == values[pawn]);
    assert(material[black] == values[knight] + values[rook]);
    assert(quantity[white][pawn] == 1);
    assert(quantity[white][queen] == 0);
    assert(reversible_moves == 4);

    assert(MakeMove("d7d8"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("d8")] == white_queen);
    assert(quantity[white][pawn] == 0);
    assert(quantity[white][queen] = 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 3);
    assert(material[white] == values[queen]);
    assert(material[black] == values[knight] + values[rook]);
    assert(reversible_moves == 0);

    TakebackMove();
    assert(MakeMove("d7e8b"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("e8")] == white_bishop);
    assert(quantity[white][pawn] == 0);
    assert(quantity[white][bishop] == 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 2);
    assert(material[white] == values[bishop]);
    assert(material[black] == values[knight]);
    assert(reversible_moves == 0);

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
    assert(SetupPosition(
        "rnb2k1r/pp1PbBpp/1qp5/8/8/8/PPP1NnPP/RNBQK2R w KQ -"));
    assert(MakeMove("d7d8q"));
    assert(MakeMove("e7d8"));
    assert(MakeMove("a2a4"));
    assert(MakeMove("b6b4"));
    assert(!MakeMove("a4a5"));
}
#endif // NDEBUG

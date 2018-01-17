#include "chess.h"





//--------------------------------
k2chess::k2chess() :
    rays{0, 8, 8, 4, 4, 8, 0},
    delta_col
        {
            { 0,  0,  0,  0,  0,  0,  0,  0},
            { 1, -1, -1,  1,  0,  1, -1,  0},  // king
            { 1, -1, -1,  1,  0,  1, -1,  0},  // queen
            { 0,  1, -1,  0,  0,  0,  0,  0},  // rook
            { 1, -1, -1,  1,  0,  0,  0,  0},  // bishop
            { 1,  2,  2,  1, -1, -2, -2, -1},  // knight
        },
    delta_row
        {
            { 0,  0,  0,  0,  0,  0,  0,  0},
            { 1,  1, -1, -1,  1,  0,  0, -1},  // king
            { 1,  1, -1, -1,  1,  0,  0, -1},  // queen
            { 1,  0,  0, -1,  0,  0,  0,  0},  // rook
            { 1,  1, -1, -1,  0,  0,  0,  0},  // bishop
            { 2,  1, -1, -2, -2, -1,  1,  2},  // knight
        },
       is_slider{false, false, true, true, true, false, false},
       piece_values{0, 15000, 1200, 600, 410, 400, 100}
{
    cv = cur_moves;
    memset(b_state, 0, sizeof(b_state));
    state = &b_state[prev_states];
    coords[black].board = b;
    coords[black].piece_values = piece_values;
    coords[white].board = b;
    coords[white].piece_values = piece_values;
    max_ray_length = std::max((size_t)board_height, (size_t)board_width);

    SetupPosition(start_position);
}





//--------------------------------
void k2chess::InitMobility(const bool color)
{
    memset(mobility[color], 0, sizeof(mobility[0]));
    for(auto it = coords[color].rbegin(); it != coords[color].rend(); ++it)
    {
		const auto type = get_type(b[*it]);
		if(type == pawn)
			continue;
		for(auto ray = 0; ray < rays[type]; ++ray)
		{
			const int d_col = delta_col[type][ray];
			const int d_row = delta_row[type][ray];
			int col = get_col(*it);
			int row = get_row(*it);
            for(size_t i = 0; i < max_ray_length; ++i)
			{
				col += d_col;
				row += d_row;
				if(!col_within(col) || !row_within(row))
					break;
				const auto sq = b[get_coord(col, row)];
				if(sq != empty_square && get_color(sq) == color)
					break;
                auto index = it.get_array_index();
                mobility[color][index][ray]++;
				if(!is_slider[type])
					break;
				if(sq != empty_square && get_color(sq) != color)
					break;
			}
		}
    }
}





//--------------------------------
void k2chess::InitAttacks(bool stm)
{
    memset(attacks[stm], 0, sizeof(attacks[0]));
    memset(xattacks[stm], 0, sizeof(xattacks[0]));

    for(auto it = coords[stm].rbegin(); it != coords[stm].rend(); ++it)
        InitAttacksOnePiece(*it, &k2chess::set_bit);

    InitMobility(stm);
}





//--------------------------------
void k2chess::InitAttacksOnePiece(const coord_t coord,
                                  const change_bit_ptr change_bit)
{
    const auto color = get_color(b[coord]);
    const auto index = find_piece(color, coord);
    const auto type = get_type(b[coord]);
    if(type == pawn)
        InitAttacksPawn(coord, color, index, change_bit);
    else
        InitAttacksNotPawn(coord, color, index, type, change_bit, all_rays);
}





//--------------------------------
void k2chess::set_bit(attack_t (* const attacks)[board_height*board_width],
                      const bool color, const coord_t col, const coord_t row,
                      const u8 index)
{
    attacks[color][get_coord(col, row)] |= (1 << index);
}





//--------------------------------
void k2chess::clear_bit(attack_t (* const attacks)[board_height*board_width],
                        const bool color, const coord_t col, const coord_t row,
                        const u8 index)
{
    attacks[color][get_coord(col, row)] &= ~(1 << index);
}





//--------------------------------
void k2chess::InitAttacksPawn(const coord_t coord, const bool color,
                              const u8 index, const change_bit_ptr change_bit)
{
    const auto col = get_col(coord);
    const auto row = get_row(coord);
    const auto d_row = color ? 1 : -1;
    if(col_within(col + 1))
        (this->*change_bit)(attacks, color, col + 1, row + d_row, index);
    if(col_within(col - 1))
        (this->*change_bit)(attacks, color, col - 1, row + d_row, index);
    (this->*change_bit)(xattacks, color, col, row + d_row, index);
    if(row == (color ? 1 : max_row - 1)
            && b[get_coord(col, row + d_row)] == empty_square)
        (this->*change_bit)(xattacks, color, col, row + 2*d_row, index);
}





//--------------------------------
void k2chess::InitAttacksNotPawn(const coord_t coord, const bool color,
                                 const u8 index, const coord_t type,
                                 const change_bit_ptr change_bit,
                                 ray_mask_t ray_mask)
{
    if(change_bit == &k2chess::set_bit)
    {
        for(auto ray = 0; ray < rays[type]; ray++, ray_mask >>= 1)
        {
            if(!(ray_mask & 1))
                continue;
            auto col = get_col(coord);
            auto row = get_row(coord);
            const auto d_col = delta_col[type][ray];
            const auto d_row = delta_row[type][ray];
            size_t i = 0;
            for(; i < max_ray_length; ++i)
            {
                col += d_col;
                row += d_row;
                if(!col_within(col) || !row_within(row))
                    break;
                set_bit(attacks, color, col, row, index);
                auto sq = b[get_coord(col, row)];
                if(sq == empty_square || get_color(sq) != color)
                    mobility[color][index][ray]++;
                if(!is_slider[type])
                    break;
                if(b[get_coord(col, row)] != empty_square)
                    break;
            }
            if(!is_slider[type] || NoExtendedAttacks(b[get_coord(col, row)],
                                                     type, color, d_col, d_row))
                continue;
            for(size_t j = i; j < max_ray_length; ++j)
            {
                col += d_col;
                row += d_row;
                if(!col_within(col) || !row_within(row))
                    break;
                set_bit(xattacks, color, col, row, index);
                if(b[get_coord(col, row)] != empty_square)
                    break;
            }
        }
    }
    else
    {
        for(auto ray = 0; ray < rays[type]; ray++, ray_mask >>= 1)
        {
            if(!(ray_mask & 1))
                continue;
            mobility[color][index][ray] = 0;
            auto col = get_col(coord);
            auto row = get_row(coord);
            const auto d_col = delta_col[type][ray];
            const auto d_row = delta_row[type][ray];
            size_t i = 0;
            for(; i < max_ray_length; ++i)
            {
                col += d_col;
                row += d_row;
                if(!col_within(col) || !row_within(row))
                    break;
                clear_bit(attacks, color, col, row, index);
                clear_bit(xattacks, color, col, row, index);
                if(!is_slider[type])
                    break;
            }
        }
    }
}





//--------------------------------
bool k2chess::NoExtendedAttacks(const piece_t sq, const coord_t type,
                                const bool color, const shifts_t delta_col,
                                const shifts_t delta_row) const
{
    const auto sq_type = get_type(sq);
    if(get_color(sq) != color)
        return true;
    if(sq_type < queen || sq_type > bishop)
        return true;
    if(sq_type != type && sq_type != queen && type != queen)
        return true;
    if(sq_type == bishop && (!delta_col || !delta_row))
        return true;
    if(sq_type == rook && delta_col && delta_row)
        return true;

    return false;
}





//--------------------------------
void k2chess::UpdateAttacks(const move_c move, const coord_t from_coord)
{
    auto moving_piece_it = coords[!wtm].at(move.piece_index);
    update_mask[!wtm] |= (1 << moving_piece_it.get_array_index());
    auto cstl_it = coords[!wtm].at(state[ply].castled_rook_index);
    if(move.flag & is_castle)
    {
        update_mask[!wtm] |= (1 << cstl_it.get_array_index());
        const auto from_col = move.flag == is_right_castle ? max_col : 0;
        const auto from_row = wtm ? max_row : 0;
        const auto rook_from_coord = get_coord(from_col, from_row);
        for(auto stm : {black, white})
        {
            update_mask[stm] |= attacks[stm][rook_from_coord];
            update_mask[stm] |= attacks[stm][*cstl_it];
        }
    }
    auto captured_it = coords[wtm].at(state[ply].captured_index);
    if(move.flag & is_capture)
        update_mask[wtm] |= (1 << captured_it.get_array_index());

    const bool is_enps = move.flag & is_en_passant;

    for(auto stm : {black, white})
    {
        update_mask[stm] |= attacks[stm][from_coord];
        update_mask[stm] |= attacks[stm][move.to_coord];
        if(is_enps)
            update_mask[stm] |= attacks[stm]
                    [move.to_coord + (wtm ? board_width : -board_width)];

        auto it = coords[stm].begin();
        for(size_t i = 0; i < attack_digits; ++i)
        {
            if(!((update_mask[stm] >> i) & 1))
                continue;
            auto piece_coord = *it[i];
            auto color = get_color(b[*it]);
            auto type = get_type(b[*it]);
            auto index = it.get_array_index();
            const bool is_captured = (move.flag & is_capture) && stm == wtm
                    && piece_coord == *captured_it;
            const auto to_coord = move.to_coord;
            if(is_captured)
            {
                color = stm;
                type = is_enps ? pawn : get_type(state[ply].captured_piece);
                index = captured_it.get_array_index();
            }
            const bool is_move = stm != wtm && it == moving_piece_it;
            if(is_move && (move.flag & is_promotion))
                type = pawn;
            const bool is_cstl = (move.flag & is_castle) && stm != wtm
                    && it == cstl_it;
            if(is_cstl)
                piece_coord = get_coord((move.flag & is_left_castle) ?
                                            0 : max_col, wtm ? max_row : 0);

            UpdateAttacksOnePiece(from_coord, to_coord, piece_coord,
                                  color, type, is_move,
                                  is_captured || is_enps ||
                                  (move.flag & is_castle),
                                  index, &k2chess::clear_bit);
            if(is_cstl)
                piece_coord = *it;
            else if(is_move && (move.flag & is_promotion))
                type = get_type(b[*it]);
            if(!is_captured)
                UpdateAttacksOnePiece(from_coord, to_coord, piece_coord,
                                      color, type, is_move,
                                      is_captured || is_enps ||
                                      (move.flag & is_castle),
                                      index, &k2chess::set_bit);
        }
        update_mask[stm] = 0;
    }
#ifndef NDEBUG
    attack_t tmp_a[sides][board_height*board_width];
    const auto sz_a = sizeof(attacks);
    memcpy(tmp_a, attacks, sz_a);
    coord_t tmp_m[sides][max_pieces_one_side][max_rays];
    const auto sz_m = sizeof(mobility);
    memcpy(tmp_m, mobility, sz_m);
    InitAttacks(white);
    InitAttacks(black);
    assert(memcmp(tmp_a, attacks, sz_a) == 0);
    assert(memcmp(tmp_m, mobility, sz_m) == 0);
#endif
}





//--------------------------------
void k2chess::UpdateAttacksOnePiece(const coord_t from_coord,
                                    const coord_t to_coord,
                                    const coord_t piece_coord,
                                    const bool color,
                                    const coord_t type, const bool is_move,
                                    const bool is_special_move,
                                    const u8 index,
                                    const change_bit_ptr change_bit)
{
    assert(type <= pawn);
    assert(index <= attack_digits);
    auto coord = piece_coord;
    if(is_move && change_bit == &k2chess::clear_bit)
        coord = from_coord;
    if(type == pawn)
        InitAttacksPawn(coord, color, index, change_bit);
    else if(is_slider[type])
    {
        auto ray_mask = GetRayMask(is_move, from_coord, to_coord,
                                         piece_coord, change_bit);
        if(is_special_move)
            ray_mask = all_rays;
        InitAttacksNotPawn(coord, color, index, type, change_bit, ray_mask);
        if(is_move)
        {
            coord = change_bit == &k2chess::clear_bit ? piece_coord :
                                                        from_coord;
            (this->*change_bit)(attacks, color, get_col(coord),
                get_row(coord), index);

            if(change_bit == &k2chess::set_bit)
            {
                enum {ray_N, ray_E, ray_W, ray_S};
                enum {ray_NE, ray_NW, ray_SW, ray_SE};
                const auto d_col = get_col(to_coord) - get_col(from_coord);
                const auto d_row = get_row(to_coord) - get_row(from_coord);
                auto is_q = 4*(get_type(b[to_coord]) == queen);
                if(d_col == 0)
                {
                    mobility[color][index][ray_N + is_q] -= d_row;
                    mobility[color][index][ray_S + is_q] += d_row;
                }
                else if(d_row == 0)
                {
                    mobility[color][index][ray_E + is_q] -= d_col;
                    mobility[color][index][ray_W + is_q] += d_col;
                }
                else if(sgn(d_col) == sgn(d_row))
                {
                    mobility[color][index][ray_NE] -= d_col;
                    mobility[color][index][ray_SW] += d_col;
                }
                else
                {
                    mobility[color][index][ray_SE] -= d_col;
                    mobility[color][index][ray_NW] += d_col;
                }
            }
        }
    }
    else
        InitAttacksNotPawn(coord, color, index, type, change_bit, all_rays);
}





//--------------------------------
k2chess::ray_mask_t k2chess::GetRayMask(const bool is_move,
                                        const coord_t from_coord,
                                        const coord_t to_coord,
                                        const coord_t piece_coord,
                                        const change_bit_ptr change_bit) const
{
    ray_mask_t ans;
    if(!is_move)
    {
        ans = GetRayMaskNotForMove(to_coord, piece_coord);
//        if(change_bit == &k2chess::set_bit)
            ans |= GetRayMaskNotForMove(from_coord, piece_coord);
        return ans;
    }
    coord_t move_type;
    auto index = GetRayIndex(from_coord, to_coord, &move_type);

    const ray_mask_t RB_masks[] = {rays_NW_or_SE, rays_E_or_W, rays_NE_or_SW,
                                   rays_N_or_S, 0, rays_N_or_S,
                                   rays_NE_or_SW, rays_E_or_W, rays_NW_or_SE};
    ans = RB_masks[index];
    assert(ans);

    const ray_mask_t Q_masks[] = {rays_rook_q, rays_bishop, rays_rook_q,
                                  rays_bishop, 0, rays_bishop,
                                  rays_rook_q, rays_bishop, rays_rook_q};
    bool queen_as_rook = false;
    if(get_type(b[to_coord]) == queen)
    {
        if(move_type == rook)
        {
            queen_as_rook = true;
            ans <<= 4;  // due to delta_col and delta_row arrays content
        }
        ans |= Q_masks[index];
    }

    const ray_mask_t capture_mask[] = {rays_SW, rays_South, rays_SE,
                                       rays_West, 0, rays_East,
                                       rays_NW, rays_North, rays_NE};
    if(change_bit == &k2chess::set_bit
            && state[ply].captured_piece != empty_square)
        ans |= (capture_mask[index] << (queen_as_rook ? 4 : 0));

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
    auto index = GetRayIndex(target_coord, piece_coord, &move_type);
    auto type = get_type(b[piece_coord]);
    if(move_type == 0 || (type != queen && move_type != type))
        index = 4;
    auto mask = masks[index];
    if(type == queen && move_type == rook)
        mask <<= 4;
    return mask;
}





//--------------------------------
size_t k2chess::GetRayIndex(const coord_t from_coord, const coord_t to_coord,
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
    done_moves.clear();
    return true;
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
            auto chr = *ptr - '0';
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
                auto piece = pcs[index];
                b[get_coord(col, row)] = piece;
                auto color = get_color(piece);
                if(get_type(piece) != king)
                    material[color] += piece_values[get_type(piece)];
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
        if(b[get_coord(k_col, k_row[index])] == k_piece[index] &&
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
        piece_t pawn = wtm ? white_pawn : black_pawn;
        if(b[get_coord(col - 1, row + s)] == pawn
                || b[get_coord(col + 1, row + s)] == pawn)
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
}





//--------------------------------
bool k2chess::MakeCastleOrUpdateFlags(const move_c move,
                                      const coord_t from_coord)
{
    const auto old_rights = state[ply].castling_rights;
    auto &new_rights = state[ply].castling_rights;
    if(move.piece_index == king_coord[wtm].get_array_index())  // king moves
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
    {
        if(castling_rights_changed)
            return true;
        else
            return false;
    }

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
    auto id = find_piece(wtm, rook_from_coord);
    assert(id != -1U);
    state[ply].castled_rook_index = id;
    b[rook_to_coord] = b[rook_from_coord];
    b[rook_from_coord] = empty_square;
    *coords[wtm].at(id) = rook_to_coord;

    reversible_moves = 0;

    return false;
}





//--------------------------------
void k2chess::TakebackCastle(const move_c move)
{
    auto rook_iterator = coords[wtm].at(state[ply].castled_rook_index);
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
        material[!wtm] -= piece_values[pawn];
        quantity[!wtm][pawn]--;
    }
    else
    {
        const auto piece_index = get_type(state[ply].captured_piece);
        material[!wtm] -= piece_values[piece_index];
        quantity[!wtm][piece_index]--;
    }
    auto id = find_piece(!wtm, to_coord);
    assert(id != -1U);
    state[ply].captured_index = id;
    coords[!wtm].erase(id);
    pieces[!wtm]--;
    reversible_moves = 0;
}





//--------------------------------
void k2chess::TakebackCapture(const move_c move)
{
    auto it_cap = coords[!wtm].at(state[ply].captured_index);

    if(move.flag & is_en_passant)
    {
        material[!wtm] += piece_values[pawn];
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
        const auto capt_index = get_type(state[ply].captured_piece);
        material[!wtm] += piece_values[capt_index];
        quantity[!wtm][capt_index]++;
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
    material[wtm] += piece_values[type] - piece_values[pawn];
    quantity[wtm][pawn]--;
    quantity[wtm][type]++;
    reversible_moves = 0;
    store_coords.push_back(coords[wtm]);
    coords[wtm].sort();
    InitSliderMask(wtm);
}





//--------------------------------
void k2chess::TakebackPromotion(const move_c move)
{
    piece_t pcs[] = {0, black_queen, black_knight, black_rook, black_bishop};
    const move_flag_t piece_num = move.flag & is_promotion;
    const auto type = get_type(pcs[piece_num]);
    material[wtm] -= (piece_values[type] - piece_values[pawn]);
    quantity[wtm][pawn]++;
    quantity[wtm][type]--;
    InitSliderMask(wtm);
}





//--------------------------------
bool k2chess::MakeMove(const move_c move)
{
    memcpy(done_attacks[ply], attacks, sizeof(attacks));
    memcpy(done_mobility[ply], mobility, sizeof(mobility));

    bool is_special_move = false;
    ply++;
    auto it = coords[wtm].at(move.piece_index);
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

    done_moves.push_back(move);
    if(move.flag & is_promotion)
    {
        InitAttacks(!wtm);
        if(move.flag & is_capture)
            InitAttacks(wtm);
    }
    else
        UpdateAttacks(move, from_coord);

    if(is_special_move)
        return true;
    else
        return false;
}





//--------------------------------
void k2chess::TakebackMove(const move_c move)
{
    done_moves.pop_back();
    if(move.flag & is_promotion)
    {
        coords[!wtm] = store_coords.back();
        store_coords.pop_back();
    }
    const auto from_coord = state[ply].from_coord;
    auto it = coords[!wtm].at(move.piece_index);
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

    memcpy(attacks, done_attacks[ply], sizeof(attacks));
    memcpy(mobility, done_mobility[ply], sizeof(mobility));

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
    auto move = MoveFromStr(str);
    if(move.flag == is_bad_move_flag)
        return false;

    if(!IsPseudoLegal(move))
        return false;
    if(!IsLegal(move))
        return false;

    MakeMove(move);

    return true;
}





//--------------------------------
bool k2chess::TakebackMove()
{
    if(done_moves.size() == 0)
        return false;

    auto move = done_moves.back();

    TakebackMove(move);
    return true;
}





//--------------------------------
size_t k2chess::find_piece(const bool stm, const coord_t coord)
{
    auto it = coords[stm].begin();
    const auto it_end = coords[stm].end();
    for(; it != it_end; ++it)
        if(*it == coord)
            break;
    if(it == coords[stm].end())
        return -1U;
    return it.get_array_index();
}





//--------------------------------
k2chess::move_flag_t k2chess::InitMoveFlag(const move_c move,
                                           const char promo_to)
{
    move_flag_t ans = 0;
    auto it = coords[wtm].at(move.piece_index);
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
            return is_bad_move_flag;
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
    auto it = coords[wtm].at(move.piece_index);
    const auto to_piece = b[move.to_coord];
    if(to_piece != empty_square && get_color(to_piece) == wtm)
        return false;

    const auto from_coord = *it;
    if(from_coord == move.to_coord)
        return false;
    const auto piece = b[*it];
    if(get_type(piece) == pawn)
        return IsPseudoLegalPawn(move, from_coord);
    else if(is_slider[get_type(piece)])
        return IsSliderAttack(from_coord, move.to_coord);
    else if(get_type(piece) == king)
        return IsPseudoLegalKing(move, from_coord);
    else
        return IsPseudoLegalKnight(move, from_coord);
}





//--------------------------------
bool k2chess::IsPseudoLegalKing(const move_c move,
                                const coord_t from_coord) const
{
    const auto fr_col = get_col(from_coord);
    const auto fr_row = get_row(from_coord);
    const auto to_col = get_col(move.to_coord);
    const auto to_row = get_row(move.to_coord);

    if(std::abs(fr_col - to_col) <= 1 && std::abs(fr_row - to_row) <= 1)
        return true;

    if(fr_col == default_king_col && get_type(b[from_coord]) == king
            && get_color(b[from_coord]) == wtm
            && (to_col == default_king_col - cstl_move_length ||
                to_col == default_king_col + cstl_move_length)
            && ((wtm && fr_row == 0 && to_row == 0) ||
                (!wtm && fr_row == max_row && to_row == max_row)))
                return true;
    return false;
}





//--------------------------------
bool k2chess::IsPseudoLegalKnight(const move_c move,
                                  const coord_t from_coord) const
{
    const auto d_col = std::abs(get_col(from_coord) - get_col(move.to_coord));
    const auto d_row = std::abs(get_row(from_coord) - get_row(move.to_coord));
    if(d_col + d_row == 3 && (d_col == 1 || d_row == 1))
        return true;
    return false;
}





//--------------------------------
bool k2chess::IsPseudoLegalPawn(const move_c move,
                                const coord_t from_coord) const
{
    const auto delta = wtm ? board_width : -board_width;
    if(move.to_coord == from_coord + delta
            && b[move.to_coord] == empty_square)
        return true;
    const auto init_row = (wtm ? pawn_default_row :
                                 max_row - pawn_default_row);
    if(move.to_coord == from_coord + 2*delta
            && get_row(from_coord) == init_row
            && b[move.to_coord] == empty_square
            && b[move.to_coord - delta] == empty_square)
        return true;
    else if((move.to_coord == from_coord + delta + 1
        || move.to_coord == from_coord + delta - 1))
    {
        if(b[move.to_coord] != empty_square)
        {
            if(get_color(b[move.to_coord]) != wtm)
                return true;
        }
        else //en passant
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
bool k2chess::IsOnRay(const coord_t k_coord, const coord_t attacker_coord,
                      const coord_t to_coord) const
{
    if(to_coord == attacker_coord)
        return true;

    const auto col = get_col(to_coord);
    const auto row = get_row(to_coord);
    const auto min_col = std::min(get_col(k_coord), get_col(attacker_coord));
    const auto min_row = std::min(get_row(k_coord), get_row(attacker_coord));
    const auto max_col = std::max(get_col(k_coord), get_col(attacker_coord));
    const auto max_row = std::max(get_row(k_coord), get_row(attacker_coord));
    if(col < min_col || col > max_col || row < min_row || row > max_row)
        return false;

    const int delta_x1 = get_col(attacker_coord) - get_col(k_coord);
    const int delta_y1 = get_row(attacker_coord) - get_row(k_coord);
    const int delta_x2 = get_col(to_coord) - get_col(k_coord);
    const int delta_y2 = get_row(to_coord) - get_row(k_coord);
    if(delta_x1 == 0 || delta_x2 == 0)
    {
        if(board_width*delta_x1/delta_y1 == board_width*delta_x2/delta_y2)
            return true;
        else
            return false;
    }
    else
    {
        if(board_width*delta_y1/delta_x1 == board_width*delta_y2/delta_x2)
            return true;
        else
            return false;
    }
}





//--------------------------------
bool k2chess::IsSliderAttack(const coord_t from_coord,
                             const coord_t to_coord) const
{
    auto d_col = get_col(to_coord) - get_col(from_coord);
    auto d_row = get_row(to_coord) - get_row(from_coord);
    if(d_col > 1)
        d_col = 1;
    else if(d_col < -1)
        d_col = -1;
    if(d_row > 1)
        d_row = 1;
    else if(d_row < -1)
        d_row = -1;

    const auto delta_coord = d_col + board_width*d_row;
    for(auto i = from_coord + delta_coord;
        i != to_coord;
        i += delta_coord)
    {
        if(b[i] != empty_square)
            return false;
    }

    return true;
}





//--------------------------------
bool k2chess::IsDiscoveredAttack(const coord_t fr_coord,
                                 const coord_t to_coord,
                                 const attack_t mask)
{
    auto it = coords[!wtm].begin();
    size_t i = 0;
    for(; i < attack_digits; ++i)
    {
        if(!((mask >> i) & 1))
            continue;
        const auto attacker_coord = *it[i];
        const auto k_coord = *king_coord[wtm];
        if(!IsOnRay(k_coord, attacker_coord, fr_coord))
            continue;
        if(IsOnRay(k_coord, attacker_coord, to_coord))
            continue;
        if(!IsSliderAttack(fr_coord, k_coord))
            continue;
        if(mask == slider_mask[!wtm])  // special for en_passant case
        {
            const auto type = get_type(b[attacker_coord]);
            const auto d_col = get_col(attacker_coord) - get_col(k_coord);
            const auto d_row = get_row(attacker_coord) - get_row(k_coord);
            if(type == bishop && (d_col == 0 || d_row == 0))
                continue;
            if(type == rook && d_col != 0 && d_row != 0)
                continue;
        }
        return true;
    }
    return false;
}





//--------------------------------
bool k2chess::IsLegal(const move_c move)
{
    auto it = coords[wtm].at(move.piece_index);
    const auto piece_type = get_type(b[*it]);
    if(piece_type == king)
    {
        if(!(move.flag & is_castle))
        {
            if(attacks[!wtm][move.to_coord] == 0)
            {
                const auto att_mask = attacks[!wtm][*it] & slider_mask[!wtm];
                if(!att_mask)
                    return true;
                for(size_t i = 0; i < attack_digits; ++i)
                {
                    if(!((att_mask >> i) & 1))
                        continue;
                    if(!(att_mask >> i))
                        break;
                    const auto attacker_coord = *coords[!wtm].at(i);
                    if(IsOnRay(move.to_coord, attacker_coord, *it))
                        return false;
                }
                return true;
            }
            else
                return false;
        }
        else
        {
            if(IsLegalCastle(move))
                return true;
            else
                return false;
        }
    }
    else
    {
        const auto k = *king_coord[wtm];
        auto attack_sum = attacks[!wtm][k];
        const auto king_attackers =
                std::bitset<attack_digits>(attack_sum).count();
        assert(king_attackers <= 2);
        if(king_attackers == 2)
            return false;
        if((move.flag & is_en_passant))
        {
            if(IsDiscoveredAttack(*it, 0, slider_mask[!wtm]))
                return false;
        }
        const auto att_mask = attacks[!wtm][*it] & slider_mask[!wtm];
        if(att_mask && IsDiscoveredAttack(*it, move.to_coord, att_mask))
            return false;
        if(king_attackers == 1)
        {
            auto attacker_id = 0;
            for(;;)
                if(attack_sum >>= 1)
                    attacker_id++;
                else
                    break;
            auto it = coords[!wtm].begin();
            const auto attacker_coord = *it[attacker_id];
            if(is_slider[get_type(b[attacker_coord])])
            {
                if(IsOnRay(*king_coord[wtm], attacker_coord, move.to_coord))
                    return true;
                else
                    return false;
            }
            else
            {
                if(move.to_coord == attacker_coord)
                    return true;
                else
                    return false;
            }
        }
    }
    return true;

}




#ifndef NDEBUG
//--------------------------------
size_t k2chess::test_count_attacked_squares(const bool stm,
                                            const bool use_extended_attacks)
{
    size_t ans = 0;
    for(auto it : use_extended_attacks ? xattacks[stm] : attacks[stm])
        if(it != 0)
            ans++;
    return ans;
}





//--------------------------------
size_t k2chess::test_count_all_attacks(const bool stm,
                                       const bool use_extended_attacks)
{
    size_t ans = 0;
    for(auto it : use_extended_attacks ? xattacks[stm] : attacks[stm])
        ans += std::bitset<attack_digits>(it).count();
    return ans;
}





//--------------------------------
void k2chess::test_attack_tables(const size_t att_w, const size_t att_b,
                                 const size_t all_w, const size_t all_b,
                                 const bool use_extended_attacks)
{
    size_t att_squares_w, att_squares_b, all_attacks_w, all_attacks_b;
    att_squares_w = test_count_attacked_squares(white, use_extended_attacks);
    att_squares_b = test_count_attacked_squares(black, use_extended_attacks);
    all_attacks_w = test_count_all_attacks(white, use_extended_attacks);
    all_attacks_b = test_count_all_attacks(black, use_extended_attacks);
    assert(att_squares_w == att_w);
    assert(att_squares_b == att_b);
    assert(all_attacks_w == all_w);
    assert(all_attacks_b == all_b);
}





//--------------------------------
size_t k2chess::test_mobility(const bool color)
{
	size_t ans = 0;
    for(auto index = 0; index < max_pieces_one_side; ++index)
        for(auto ray = 0; ray < max_rays; ++ray)
            ans += mobility[color][index][ray];
    return ans;
}





//--------------------------------
void k2chess::RunUnitTests()
{
    assert(SetupPosition(start_position));
    bool extended_attacks = true;
    test_attack_tables(22, 22, 38, 38, false);
    test_attack_tables(16, 16, 16, 16, extended_attacks);
    assert(test_mobility(black) == 4);
    assert(test_mobility(white) == 4);

    assert(SetupPosition("4k3/8/5n2/5n2/8/8/8/3RK3 w - - 0 1"));
    test_attack_tables(15, 19, 16, 21, false);
    test_attack_tables(0, 0, 0, 0, extended_attacks);
    assert(test_mobility(black) == 20);
    assert(test_mobility(white) == 14);

    assert(SetupPosition(
               "2r2rk1/p4q2/1p2b3/1n6/1N6/1P2B3/P4Q2/2R2RK1 w - - 0 1"));
    test_attack_tables(39, 39, 58, 58, false);
    test_attack_tables(14, 14, 15, 15, extended_attacks);
    assert(test_mobility(black) == 42);
    assert(test_mobility(white) == 42);

    assert(SetupPosition(
               "2k1r2r/1pp3pp/p2b4/2p1n2q/6b1/1NQ1B3/PPP2PPP/R3RNK1 b - -"
               "0 1"));
    test_attack_tables(32, 38, 58, 61, false);
    test_attack_tables(11, 15, 11, 15, extended_attacks);
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
    assert(material[white] == 8*piece_values[pawn] + piece_values[queen] +
           2*piece_values[rook] + 2*piece_values[bishop] +
           2*piece_values[knight]);
    assert(material[black] == 7*piece_values[pawn] + piece_values[queen] +
           2*piece_values[rook] + 2*piece_values[bishop] +
           2*piece_values[knight]);
    assert(pieces[white] == 16);
    assert(pieces[black] == 15);
    assert(find_piece(black, get_coord("d5")) == -1U);
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
    assert(TakebackMove());
    assert(MakeMove("b1d2"));
    assert(TakebackMove());
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
    assert(material[white] == 7*piece_values[pawn] +
           2*piece_values[rook] + piece_values[bishop] +
           2*piece_values[knight]);
    assert(material[black] == 7*piece_values[pawn] +
           2*piece_values[rook] + 2*piece_values[bishop] +
           piece_values[knight]);
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
    assert(material[white] == piece_values[queen]);
    assert(material[black] == piece_values[knight] + piece_values[rook]);
    assert(reversible_moves == 0);

    assert(TakebackMove());
    assert(b[get_coord("d7")] == white_pawn);
    assert(b[get_coord("d8")] == empty_square);
    assert(material[white] == piece_values[pawn]);
    assert(material[black] == piece_values[knight] + piece_values[rook]);
    assert(quantity[white][pawn] == 1);
    assert(quantity[white][queen] == 0);
    assert(reversible_moves == 4);

    assert(!TakebackMove());

    assert(MakeMove("d7d8"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("d8")] == white_queen);
    assert(quantity[white][pawn] == 0);
    assert(quantity[white][queen] = 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 3);
    assert(material[white] == piece_values[queen]);
    assert(material[black] == piece_values[knight] + piece_values[rook]);
    assert(reversible_moves == 0);

    assert(TakebackMove());
    assert(MakeMove("d7e8b"));
    assert(b[get_coord("d7")] == empty_square);
    assert(b[get_coord("e8")] == white_bishop);
    assert(quantity[white][pawn] == 0);
    assert(quantity[white][bishop] == 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 2);
    assert(material[white] == piece_values[bishop]);
    assert(material[black] == piece_values[knight]);
    assert(reversible_moves == 0);

    assert(SetupPosition("k7/8/8/8/8/7K/3p4/2N1R3 b - - 0 1"));
    assert(MakeMove("d2c1r"));
    assert(b[get_coord("d2")] == empty_square);
    assert(b[get_coord("c1")] == black_rook);
    assert(quantity[black][pawn] == 0);
    assert(quantity[black][rook] == 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 2);
    assert(material[white] == piece_values[rook]);
    assert(material[black] == piece_values[rook]);
    assert(reversible_moves == 0);

    assert(TakebackMove());
    assert(MakeMove("d2c1n"));
    assert(b[get_coord("d2")] == empty_square);
    assert(b[get_coord("c1")] == black_knight);
    assert(quantity[black][pawn] == 0);
    assert(quantity[black][knight] == 1);
    assert(pieces[white] = 2);
    assert(pieces[black] = 2);
    assert(material[white] == piece_values[rook]);
    assert(material[black] == piece_values[knight]);
    assert(reversible_moves == 0);

    assert(TakebackMove());
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
    assert(TakebackMove());
    assert(TakebackMove());
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
    assert(TakebackMove());
    assert(MakeMove("e1e2"));
    assert(MakeMove("e8e7"));
    assert(MakeMove("e2e1"));
    assert(MakeMove("e7e8"));
    assert(!MakeMove("e1g1"));
    assert(MakeMove("h2h3"));
    assert(!MakeMove("e8c8"));
    for(auto i = 0; i < 5; ++i)
        assert(TakebackMove());
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
    assert(TakebackMove());
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
    assert(TakebackMove());
    assert(MakeMove("e1g1"));

    assert(SetupPosition("4k3/4r3/8/b7/1N5q/4B3/5P2/4K3 w - - 0 1"));
    assert(!MakeMove("b4a2"));
    assert(!MakeMove("e3g5"));
    assert(!MakeMove("f2f3"));
    assert(MakeMove("e1d1"));
    assert(MakeMove("e8d8"));
    assert(!MakeMove("d2c4"));
    assert(TakebackMove());
    assert(!MakeMove("e2d3"));
    assert(TakebackMove());
    assert(!MakeMove("f2f3"));

    assert(SetupPosition("4k3/3ppp2/4b3/1n5Q/B7/8/4R3/4K3 b - - 0 1"));
    assert(MakeMove("b5a7"));
    assert(TakebackMove());
    assert(MakeMove("e6g4"));
    assert(!MakeMove("f7f6"));
    assert(TakebackMove());
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
    assert(TakebackMove());
    assert(MakeMove("c4c3"));
    assert(MakeMove("d4d5"));

    assert(SetupPosition("3k2r1/2p5/8/3P4/8/8/K7/8 b - - 0 1"));
    assert(MakeMove("c7c5"));
    assert(MakeMove("d5c6"));
    assert(TakebackMove());
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
    assert(TakebackMove());
    assert(MakeMove("d1c2"));

    assert(SetupPosition("3k4/8/8/8/8/1b2N3/8/3K4 w - - 0 1"));
    assert(!MakeMove("e3d5"));
    assert(MakeMove("e3c2"));
    assert(TakebackMove());
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
    assert(TakebackMove());
    assert(!MakeMove("c6h1"));
    assert(MakeMove("c7c8"));
    assert(MakeMove("d1c1"));
    assert(MakeMove("c6a4"));
    assert(!MakeMove("b3a4"));
    for(auto i = 0; i < 3; ++i)
        assert(TakebackMove());
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
    assert(TakebackMove());
    assert(MakeMove("d2c2"));
    assert(TakebackMove());
    assert(MakeMove("d2e3"));
    assert(TakebackMove());
    assert(MakeMove("d2e2"));

    assert(SetupPosition(
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -"));
    assert(MakeMove("d2d4"));
    assert(MakeMove("b2a1q"));
    assert(MakeMove("h6f7"));
}
#endif // NDEBUG

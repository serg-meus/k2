#include "hash.h"





//---------------------------------
k2hash::k2hash()
{

    std::uniform_int_distribution<hash_key_t> zobrist_distr(0, (hash_key_t)-1);
    std::mt19937 rnd_gen;

    for(auto i = 0; i < 2*piece_types; ++i)
        for(auto j = 0; j < board_width; ++j)
            for(auto k = 0; k < board_height; ++k)
                zobrist[i][j][k] = zobrist_distr(rnd_gen);

    for(size_t i = 1;
            i < sizeof(zobrist_en_passant)/sizeof(*zobrist_en_passant);
            ++i)
        zobrist_en_passant[i] = zobrist_distr(rnd_gen);

    for(size_t i = 1;
            i < sizeof(zobrist_castling)/sizeof(*zobrist_castling);
            ++i)
        zobrist_castling[i] = zobrist_distr(rnd_gen);

    zobrist_en_passant[0] = 0;
    zobrist_castling[0] = 0;

    hash_key = InitHashKey();
}





//--------------------------------
k2hash::hash_key_t k2hash::InitHashKey() const
{
    hash_key_t ans = 0;
    for(auto stm : {black, white})
    {
        auto mask = exist_mask[stm];
        while(mask)
        {
            const auto piece_id = lower_bit_num(mask);
            mask ^= (1 << piece_id);
            const auto coord = coords[stm][piece_id];
            ans ^= zobrist[piece_hash_index(b[coord])]
                    [get_col(coord)][get_row(coord)];
        }
    }
    if(!wtm)
        ans ^= key_for_side_to_move;
    const auto &f = k2chess::state[ply];
    ans ^= zobrist_en_passant[f.en_passant_rights] ^
            zobrist_castling[f.castling_rights];

    return ans;
}





//--------------------------------
void k2hash::MoveHashKey(const move_c move, const bool special)
{
    done_hash_keys[fifty_moves + ply - 1] = hash_key;

    const auto piece = b[move.to_coord];
    const auto &st = k2chess::state[ply];
    auto &prev_st = k2chess::state[ply - 1];

    hash_key ^= zobrist[piece_hash_index(piece)]
                [get_col(move.from_coord)][get_row(move.from_coord)]
                ^ zobrist[piece_hash_index(piece)]
                [get_col(move.to_coord)][get_row(move.to_coord)];

    if(st.captured_piece)
        hash_key ^= zobrist[piece_hash_index(st.captured_piece)]
                    [get_col(move.to_coord)][get_row(move.to_coord)];
    if(prev_st.en_passant_rights)
        hash_key ^= zobrist_en_passant[prev_st.en_passant_rights];

    if(move.flag & is_promotion)
        hash_key ^= zobrist[piece_hash_index(white_pawn ^ wtm)]
                    [get_col(move.from_coord)][get_row(move.from_coord)]
                    ^ zobrist[piece_hash_index(piece)]
                    [get_col(move.from_coord)][get_row(move.from_coord)];
    else if(move.flag & is_en_passant)
        hash_key ^= zobrist[piece_hash_index(black_pawn ^ wtm)]
                    [get_col(move.to_coord)][get_row(move.to_coord)
                                          + (wtm ? 1 : -1)];
    else if(move.flag & is_castle)
    {
        if(!wtm)
        {
            if(move.flag & is_right_castle)
                hash_key ^= zobrist[piece_hash_index(white_rook)][7][0]
                            ^ zobrist[piece_hash_index(white_rook)][5][0];
            else
                hash_key ^= zobrist[piece_hash_index(white_rook)][0][0]
                            ^ zobrist[piece_hash_index(white_rook)][3][0];
        }
        else
        {
            if(move.flag & is_right_castle)
                hash_key ^= zobrist[piece_hash_index(black_rook)][7][7]
                            ^ zobrist[piece_hash_index(black_rook)][5][7];
            else
                hash_key ^= zobrist[piece_hash_index(black_rook)][0][7]
                            ^ zobrist[piece_hash_index(black_rook)][3][7];
        }
        hash_key ^= zobrist_castling[prev_st.castling_rights]
                    ^ zobrist_castling[st.castling_rights];
    }
    hash_key ^= key_for_side_to_move;
    if(special)
    {
        if(get_type(piece) == pawn && !st.captured_piece)
            hash_key ^= zobrist_en_passant[st.en_passant_rights];
        else
            hash_key ^= zobrist_castling[prev_st.castling_rights] ^
                    zobrist_castling[st.castling_rights];
    }

#ifndef NDEBUG
    auto tmp_key = InitHashKey();
    if(tmp_key != hash_key)
        std::cout << "( breakpoint )" << std::endl;
    assert(tmp_key == hash_key);
#endif //NDEBUG
}





#ifndef NDEBUG
void k2hash::RunUnitTests()
{

    k2movegen::RunUnitTests();

    struct num
    {
        int n = 0;
        char c = ' ';

        num() {}
        num(int nmb){n = nmb;}
        num(int nmb, char chr) {n = nmb; c = chr;}
        unsigned char hash() const {return static_cast<unsigned char>(n % 10);}
        operator ==(const num x) {return n == x.n;}
    };

    transposition_table_c<num, unsigned char, 4> tt(16);

    assert(!tt.find(1));
    assert(tt.size() == 0 && tt.max_size() == 16);
    tt.add({1, 'a'});
    assert(tt.find(1));
    assert(tt.find({1, 'a'}));
    assert(tt.find({1, '?'}));
    assert(tt.size() == 1);

    tt.add({2, 'b'});
    assert(tt.find(2));
    assert(tt.find(1));
    assert(tt.size() == 2);
    tt.add({11, 'z'});
    tt.add({21, 'w'});
    tt.add({31, 'q'});
    tt.add({41, 'v'});
    assert(!tt.find(1));
    assert(tt.find(2) && tt.find(11) && tt.find(21) && tt.find(31) &&
           tt.find(41));
    assert(tt.size() == 5);
    tt.add({51, 'u'});
    assert(!tt.find(1) && !tt.find(11));
    assert(tt.find(2) && tt.find(21) && tt.find(31) && tt.find(41) &&
           tt.find(51));
    assert(tt.size() == 5);
    tt.add({12, 'a'});
    tt.add({22, 's'});
    tt.add({32, 'd'});
    tt.add({42, 'f'});
    assert(!tt.find(2));
    assert(tt.find(12) && tt.find(42) && tt.find(32) && tt.find(21));
    assert(tt.size() == 8);
    const auto e1 = tt.find(21);
    assert(e1->c == 'w');
    tt.add({61, 'm'});
    assert(!tt.find(31) && tt.find(21));
    assert(tt.size() == 8);
    tt.add({3, 'A'}, 3);
    assert(tt.find(3, 3)->c == 'A');
    assert(tt.size() == 9);
    tt.clear();
    assert(!tt.find(61) && !tt.find(42) && !tt.find(51) && !tt.find(32));
    assert(tt.size() == 0 && tt.max_size() == 16);
}
#endif
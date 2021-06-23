# -*- coding: utf-8 -*-
"""
Created on Tue Jun 15 11:05:49 2021

@author: MeusSV
"""

import chess
import chess.pgn
from random import shuffle

in_file = 'E:/chess/k2_games/k2_tscp2.pgn'
out_file = 'E:/chess/k2_games/k2_tscp2_wd.epd'
num_pos_per_game = 10


def main():
    with open(in_file) as pgn:
        fens = []
        while True:
            new_fens = get_fens(pgn)
            if not new_fens:
                break
            fens += new_fens
        with open(out_file, 'w') as out:
            for fen in fens:
                out.write(fen + '\n')


def get_fens(pgn):
    game = chess.pgn.read_game(pgn)
    if game is None:
        return None
    result = game.headers['Result']
    fens = fens_from_game(game)
    shuffle(fens)
    return add_labels(fens, result)


def fens_from_game(game):
    moves = game.main_line()
    brd = chess.Board()
    fens = []
    for m in moves:
        brd.push(m)
        fens.append(brd.fen())
    return fens


def add_labels(fens, result):
    ans = []
    for f in fens:
        if is_quiet(f):
            ans.append(f + ' c9 "' + result + '"')
            if len(ans) >= num_pos_per_game:
                break
    return ans


def piece_coast(sq, brd):
    dat = {brd.pawns: 1, brd.bishops | brd.knights: 3, brd.rooks: 5,
           brd.queens: 10}
    for occ in dat:
        if (1 << sq) & (brd.occupied & occ):
            return dat[occ]
    return 1  # en passant


def not_protected(sq, brd):
    clr = (1 << sq) & brd.occupied_co[True] != 0
    msk = brd.attackers_mask(clr, sq)
    return msk == 0


def is_quiet(fen):
    brd = chess.Board(fen=fen)
    if brd.is_check():
        return False
    for m in brd.generate_legal_captures():
        if piece_coast(m.from_square, brd) < piece_coast(m.to_square, brd) or \
                not_protected(m.to_square, brd):
            return False
    return True


if __name__ == '__main__':
    main()

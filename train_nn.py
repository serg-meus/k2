# -*- coding: utf-8 -*-
"""
  Trains the NN to create simple chess eval function for one side (384xNxMx1).

  File defined by pos_fname must contain labeled data: each line is position in
FEN format, then positive score for white in centipawns, then negative score
for black separated by space.
"""

from sklearn.neural_network import MLPRegressor
from sklearn.model_selection import train_test_split
import numpy as np
import pickle
import os

pos_fname = '_path_/zuri_annotated.epd'
dat_fname = 'nn_data.dat'
layer_sizes = (4, 4)
tol = 3e-4
activation = 'tanh'


def main(pos_fname, dat_fname, layer_sizes, tol, activation):
    recalc = False
    if os.path.exists(dat_fname):
        print('Reading attributes from dat file')
        with open(dat_fname, 'rb') as file:
            (attrs, labels, tmp_pos_fname) = pickle.load(file)
            if tmp_pos_fname != pos_fname:
                recalc = True
    else:
        recalc = True

    if recalc:
        print('Generating attributes and labels from dat file')
        attrs, labels = load_attrs_and_labels(pos_fname)
        with open(dat_fname, 'wb') as file:
            pickle.dump((attrs, labels, pos_fname), file)

    print('Spliting the train set')
    (attrs, attrs_tst, labels, labels_tst) = \
        train_test_split(attrs, labels, test_size=0.333)

    print('Training the net')
    nn = MLPRegressor(hidden_layer_sizes=layer_sizes, tol=5e-2,
                      activation=activation, verbose=True)
    nn.fit(attrs, labels)

    score = nn.score(attrs, labels)
    score_tst = nn.score(attrs_tst, labels_tst)
    print(f'{100*score:4f}% accuracy on train data, '
          f'{100*score_tst:4f}% on test')

    ans = {}
    for i, C in enumerate(zip(nn.coefs_, nn.intercepts_)):
        ans['A' + str(i + 1)] = C[0].T
        ans['B' + str(i + 1)] = C[1].T

    with open(dat_fname.split('.')[0] + '.h', 'w') as out:
        for i, a in enumerate(ans):
            arr = np.array2string(ans[a].reshape(-1), precision=5,
                                  suppress_small=True, threshold=np.inf,
                                  separator = ',')
            out.write(a + '({' + arr[1:-1] + '}),\n')


# generates training data
def load_attrs_and_labels(pos_fname):
    white, black = True, False
    with open(pos_fname, 'r') as file:
        attrs = []
        labels = []
        for i, line in enumerate(file):
            attrs.append(np.array(fen_to_attr(line, white), dtype='int8'))
            labels.append(game_result(line, white))
            attrs.append(np.array(fen_to_attr(line, black), dtype='int8'))
            labels.append(game_result(line, black))
    return np.array(attrs), np.array(labels)


def fen_to_attr(fen, side):
    # [p[64], n[64], b[64], r[64], q[64], k[64]]
    board = fen_to_board(fen)
    return attrs_one_side(board, side)


def attrs_one_side(board, side):
    ans = []
    if not side:
        board = reverse_board(board)
    for pc in ('P', 'N', 'B', 'R', 'Q', 'K'):
        ans += get_piece_occupancy(board, side, pc)
    return ans


def get_piece_occupancy(board, side, piece):
    ans = []
    for row in board:
        for col in row:
            ans.append(1 if col == piece else 0)
    return ans


def fen_to_board(fen):
    ans = []
    for R in fen.split()[0].split('/')[::-1]:
        ans += [fill_row(R)] if ' ' not in R else [fill_row(R.split()[0])]
    return ans


def reverse_board(board):
    ans = []
    for row in reversed(board):
        ans.append(row.swapcase())
    return ans


def fill_row(row):
    ans = ''
    for ch in row:
        ans += int(ch)*' ' if ch in '12345678' else ch
    return ans


def game_result(fen, side):
    ans_str = fen.split()[-2:]
    ans = int(ans_str[0]) if side else -int(ans_str[1])
    return ans/10000.


if __name__ == '__main__':
    main(pos_fname, dat_fname, layer_sizes, tol, activation)

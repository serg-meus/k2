# -*- coding: utf-8 -*-
"""
Created on Fri Dec 18 11:59:32 2020

Training NN to create the whole evaluation
"""

from sklearn.neural_network import MLPRegressor
from sklearn.model_selection import train_test_split
import numpy as np
import pickle
import os


# generates training data
def load_attrs_and_labels(pos_fname):
    with open(pos_fname, 'r') as file:
        attrs = []
        labels = []
        for i, line in enumerate(file):
            attrs.append(np.array(fen_to_attr(line), dtype='int8'))
            labels.append(game_result(line))
    return np.array(attrs), np.array(labels)


def fen_to_attr(fen):
    # 2*[k[64], q[64], r[64], b[64], n[64], p[48]]
    board = fen_to_board(fen)
    return attrs_one_side(board) + attrs_one_side(reverse_board(board))


def attrs_one_side(board):
    ans = []
    for pc in ('K', 'Q', 'R', 'B', 'N', 'P'):
        ans += get_piece_occupancy(board, pc)
    return ans


def get_piece_occupancy(board, piece):
    ans = []
    for row in board:
        for col in row:
            ans.append(1 if col == piece else 0)
    return ans if piece != 'P' else ans[8:-8]


def fen_to_board(fen):
    ans = []
    for R in fen.split()[0].split('/')[::-1]:
        ans += [fill_row(R)] if ' ' not in R else [fill_row(R.split()[0])]
    if fen.split()[1] == 'b':
        ans = reverse_board(ans)
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


def game_result(fen):
    results = {'1-0': 1, '0-1': 0, '1/2-1/2': 0.5}
    ans_str = fen.split()[-1].split('"')[1]
    assert(ans_str in results or is_float(ans_str))
    ans = results[ans_str] if ans_str in results else float(ans_str)
    return ans if fen.split()[1] == 'w' else 1 - ans


def fen_to_score(nn, fen):
    attr = np.array([fen_to_attr(fen)])
    pred = nn.predict(attr)
    return anti_sigmoid(pred[0] if fen.split()[1] == 'w' else 1 - pred[0])


def anti_sigmoid(s):
    s = np.clip(s, 1e-5, 1 - 1e-5)
    return -400/1.3*np.log10(1/s - 1)


def relu(x):
    out = x
    out[out < 0] = 0
    return out


def is_float(string):
    try:
        float(string)
        return True
    except ValueError:
        return False


pos_fname = 'E:/chess/zuri.epd'
dat_fname = 'my_nn7.dat'


recalc = False
if os.path.exists(dat_fname):
    with open(dat_fname, 'rb') as file:
        (attrs, labels, tmp_pos_fname) = pickle.load(file)
        if tmp_pos_fname != pos_fname:
            recalc = True
else:
    recalc = True

if recalc:
    attrs, labels = load_attrs_and_labels(pos_fname=pos_fname)
    with open(dat_fname, 'wb') as file:
        pickle.dump((attrs, labels, pos_fname), file)

(attrs, attrs_tst, labels, labels_tst) = train_test_split(attrs, labels,
                                                          test_size=0.33)

nn = MLPRegressor(hidden_layer_sizes=(32, 8), tol=5e-4,
                  activation='tanh', verbose=True)
nn.fit(attrs, labels)

score = nn.score(attrs, labels)
score_tst = nn.score(attrs_tst, labels_tst)
print(f'{100*score:4f}% accuracy on train data, {100*score_tst:4f}% on test')

# for i, C in enumerate(zip(nn.coefs_, nn.intercepts_)):
#     exec('a' + str(i + 1) + '= C[0].T')
#     exec('b' + str(i + 1) + '= C[1].T')

# nn_input = np.array(fen_to_attr('rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1')).T
# in2 = np.tanh(np.dot(a1, nn_input) + b1)
# in3 = np.tanh(np.dot(a2, in2) + b2)
# in4 = np.tanh(np.dot(a3, in3) + b3)
# out = np.dot(a4, in4) + b4
# print(out)

#!/usr/bin/python
# -*- coding: utf-8 -*-

from math import erf, log10, sqrt

files = ['E:/chess/cutechess-cli/k2match.pgn']
base_engine = 'k2_exp'


class options_c:
    def __init__(self):
        self.skip_games_on_plot = 40000
        self.skip_time_loses = True
        self.semilog_scale = False


class stats_c:
    def __init__(self):
        self.wins = 0
        self.loses = 0
        self.draws = 0

    def sum(self):
        return self.wins + self.loses + self.draws


def main(files, base_engine, options):
    files_in_memory = read_files(files)
    (parsed_string, loses_on_time) = parse_files(files_in_memory, options)
    (graph_data, stats) = evaluate_data(parsed_string, options)
    output_results(stats, graph_data, loses_on_time, options)


def read_files(input_files):
    files_in_memory = []
    for fname in input_files:
        with open(fname, 'r') as pgn_in:
            for line in pgn_in:
                files_in_memory.append(line)
    return files_in_memory


def parse_files(files_in_memory, options):
    parse_line.loses_on_time = 0
    parsed_string = ''
    state = reverse = False
    for line in files_in_memory:
        (parsed_string, state, reverse) = parse_line(parsed_string, state,
                                                     reverse, line, options)
    return (parsed_string, parse_line.loses_on_time)


def parse_line(result, state, reverse, line, options):
    if "on time" in line:
        parse_line.loses_on_time += 1
        if options.skip_time_loses:
            result = result[:-1]
    if not state and line.startswith('[White '):
        state = True
        white_engine = line.split('"')[1]
        reverse = white_engine != base_engine

    elif state and line.startswith('[Result '):
        state = False
        result += process_result(line, reverse)

    return (result, state, reverse)


def evaluate_data(parsed_string, options):
    strengths = []
    stats = stats_c()
    for res in parsed_string:
        if not update_stats(res, stats):
            break
        strengths.append(calc_elo(stats, options.skip_games_on_plot))
    return (strengths, stats)


def output_results(stats, strengths, loses_on_time, options):
    print_results(stats, loses_on_time, options)
    if stats.sum() > options.skip_games_on_plot:
        plot_results(strengths, options)


def plot_results(strengths, options):
    import matplotlib.pyplot as plt

    three_arrays = list(map(list, zip(*strengths)))
    foo = plt.semilogx if options.semilog_scale else plt.plot
    for arr in three_arrays:
        foo(arr)
    plt.grid(True)
    plt.show()


def print_results(stats, loses_on_time, options):
    print('%d games, +%d -%d =%d' % (stats.sum(), stats.wins, stats.loses,
                                     stats.draws))
    (elo, elo_up, elo_lo) = calc_elo(stats, 0)
    print('Elo gain: %.1f ± %.1f' % (elo, elo - elo_lo))
    if stats.wins + stats.loses == 0:
        return
    los = 50 + 50*erf((stats.wins - stats.loses)/sqrt(2*(stats.wins +
                      stats.loses)))
    print('Likelihood of superiority: %(los).2f%%' % {"los": los})
    if loses_on_time > 0:
        print('Loses on time: ', loses_on_time,
              '(skipped)' if options.skip_time_loses else '')


def update_stats(result, stats):
    if result == 'w':
        stats.wins += 1
    elif result == 'l':
        stats.loses += 1
    elif result == 'd':
        stats.draws += 1
    else:
        return False
    return True


def elo_formula(r):
    return 400*log10(r/(1. - r)) if r > 0 and r < 1 else float('nan')


def calc_elo(stats, games_to_skip):
    if stats.sum() <= games_to_skip:
        return (float('nan'), float('nan'), float('nan'))

    ratio = (stats.wins + stats.draws/2)/stats.sum()
    ratio_margin = sqrt(1.96*ratio*(1 - ratio)/stats.sum())
    elo = elo_formula(ratio)
    upper_ratio = ratio + ratio_margin
    if upper_ratio >= 0.999:
        upper_ratio = 0.999
    upper_elo = elo_formula(upper_ratio)
    lower_ratio = ratio - ratio_margin
    if lower_ratio <= 0.001:
        lower_ratio = 0.001
    lower_elo = elo_formula(lower_ratio)
    return [elo, upper_elo, lower_elo]


def process_result(line, reverse_result):
    if '1-0' in line:
        return 'w' if not reverse_result else 'l'
    elif '0-1' in line:
        return 'l' if not reverse_result else 'w'
    elif '1/2-1/2' in line:
        return 'd'
    elif '*' in line:
        return ''
    else:
        print(str('error in line ') + line)
        return ''


if __name__ == "__main__":
    main(files, base_engine, options_c())

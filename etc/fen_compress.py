# -*- coding: utf-8 -*-
"""
Created on Thu Jun 17 13:13:53 2021

@author: MeusSV
"""

in_file = 'E:/chess/k2_games/tmp.epd'
out_file = 'E:/chess/k2_games/k2testset.epd'


def main():
    lines_n = num_of_lines(in_file)
    fens = read_and_sort(in_file, lines_n)
    process_and_write(fens, out_file)


def read_and_sort(in_file, num_of_lines):
    with open(in_file) as fen:
        fens = []
        for i in progress_bar(range(num_of_lines), prefix='Reading:'):
            fens.append(fen.readline())
        return sorted(fens)


def process_and_write(fens, out_file):
    eq_line_nums = equal_line_nums(fens)
    new_fens = compress_dupes(fens, eq_line_nums)
    with open(out_file, 'w') as fen:
        for line in progress_bar(new_fens, prefix='Writing:'):
            fen.write(line)


def compress_dupes(fens, equal_line_nums):
    ans = []
    for i, line in enumerate(progress_bar(fens, prefix='Pass 2: ')):
        if i not in equal_line_nums and i - 1 not in equal_line_nums:
            ans.append(line)
        elif i in equal_line_nums and i - 1 not in equal_line_nums:
            summ = score(line) + score(fens[i - 1])
            n_dupes = 2
        elif i in equal_line_nums and i - 1 in equal_line_nums:
            summ += score(line)
            n_dupes += 1
        elif i not in equal_line_nums and i - 1 in equal_line_nums:
            ans.append(new_score(line, summ, n_dupes))
        else:
            assert(False)
    return ans


def score(line):
    ans = line.split()[-1].split('"')[1]
    return 0 if ans == '0-1' else 1 if ans == '1-0' else .5 \
        if ans == '1/2-1/2' else float(ans)


def new_score(line, summ, n_dupes):
    tmp = line.split()[:-1]
    tmp.append('"' + str(summ/n_dupes) + '";\n')
    return ' '.join(tmp)


def equal_line_nums(fens):
    prev_line = ''
    ans = set()
    for i, line in enumerate(progress_bar(fens, prefix='Pass 1: ')):
        if line.split()[:4] == prev_line.split()[:4]:
            ans.add(i)
        prev_line = line
    return ans


def num_of_lines(file_name):
    with open(file_name) as file:
        for i, l in enumerate(file):
            pass
        return i + 1


def progress_bar(iterable, prefix='Progress:', suffix='Finished', decimals=1,
                 length=50, fill='█', end='\r'):
    total = len(iterable)

    def _print_progress_bar(iteration):
        percent = ('{0:.' + str(decimals) +
                   'f}').format(100*(iteration/float(total)))
        filled_length = int(length*iteration // total)
        bar = fill*filled_length + '-'*(length - filled_length)
        print('\r%s |%s| %s%% %s' % (prefix, bar, percent, suffix), end=end)

    _print_progress_bar(0)
    for i, item in enumerate(iterable):
        yield item
        _print_progress_bar(i + 1)
    print()


if __name__ == '__main__':
    main()

# -*- coding: utf-8 -*-
"""
  K2tune v0.4,

the script for automated execution series of matches between base version of
chess engine and modified versions. Base version is represented by chess
engine source code, and modified versions are represented by patch files
for this source code. It must be possible to build engine executable
from command line.
  The goal is to detect patches that changes playing strength in given range.
  To play matches cutechess-cli tool is used, it's config file 'engines.json'
must be configured to run base and patched engine versions.
  Version of cutechess-cli required: 0.6.0 or higher.
  For Windows 'patch.exe' tool is required.
  Book file with starting positions in EPD or PGN format is recommended.
  After each match the result puts to standard output: patch XXXXX is
  green (upper bound reached) / red (lower bound) / yellow (within bounds).

  Typical usage:
1. Open this script in text editor and configure paths, parameters, etc.
2. Open terminal (command promt) and change current directory to path
containing this script
3. Execute command: python k2tune.py

To stop match or series of matches use task manager of operating system
and 'kill' one of engine's process.
"""

from subprocess import call
import sys
import os.path
from time import time
from platform import system

ini_file = 'k2tune.ini'
concurrency = '3'  # how many cpu cores must be used
elo_lower_bound = '-1'
elo_upper_bound = '4'
probability_lower_bound = '0.05'
probability_upper_bound = '0.013'
book = 'open-moves-even-depth20.pgn order=random'
rounds = '80000'  # each round consists of two games
base_engine_name = 'k2'  # the name in 'engines.json'
patched_engine_name = 'k2_exp'
stop_if_match_aborted = True  # do not proceed next patch on error
wait_ms = '100'  # pause in ms between two consequent games

if system() == 'Windows':
    src_path = 'e:/zerg/work/source/k2/src'
    patches_path = 'e:/zerg/yd/k2/patch'
    cutechess_path = 'e:/zerg/etc/gs/cutechess-cli'
    diff_path = 'C:/Users/MeusSV/AppData/Local/Programs/Git/usr/bin'
    cd_cmd = 'cd /d '
    del_cmd = 'del /Q '
    make_cmd = 'make -f Makefile.win'
    cutechess_exe = 'cutechess-cli'
    base_exec = 'k2.exe'
    patched_exec = 'k2_exp.exe'
    no_echo = ''
else:
    src_path = './src'
    patches_path = './patches'
    cutechess_path = '.'
    diff_path = ''
    cd_cmd = 'cd '
    del_cmd = 'rm -f '
    make_cmd = 'make'
    cutechess_exe = './cutechess-cli.sh'
    base_exec = 'k2'
    patched_exec = 'k2_exp'
    no_echo = ' > /dev/null'

log_filename = 'k2match.log'
pgn_out_filename = 'k2match.pgn'

engines_path = cutechess_path + '/engines'
src_exec_path = 'e:/zerg/work/source/k2/src/bin/Release'  # path from Makefile
if diff_path and diff_path[-1] != '/':
    diff_path += '/'

patch_cmd = diff_path + 'patch -d ' + src_path + ' -i ' + patches_path
unpatch_cmd = diff_path + 'patch -R -d ' + src_path + ' -i ' + patches_path
make_args = src_path + ' && ' + make_cmd
move_args = src_exec_path + '/' + base_exec + ' ' + engines_path + '/'

if system() == 'Windows':
    move_cmd = 'move /Y ' + move_args.replace('/', '\\')
else:
    move_cmd = 'mv -f ' + move_args

cutechess_args = cutechess_path + ' && ' + del_cmd + \
    pgn_out_filename + ' && ' + \
    cutechess_exe + ' -engine conf=' + patched_engine_name + ' ' \
    '-engine conf=' + base_engine_name + ' ' \
    '-each tc=time_control ' + \
    '-openings file=' + book + ' ' \
    '-pgnout ' + pgn_out_filename + ' ' \
    '-resign movecount=5 score=650 ' \
    '-draw movenumber=100 movecount=5 score=80 ' \
    '-ratinginterval 4 ' \
    '-concurrency ' + concurrency + ' ' \
    '-games 2 -rounds ' + rounds + ' -repeat ' \
    '-wait ' + wait_ms + ' ' \
    '-sprt elo0=' + elo_lower_bound + ' elo1=' + elo_upper_bound + \
    ' alpha=' + probability_upper_bound + \
    ' beta=' + probability_lower_bound + ' ' \
    '> ' + log_filename

make_cmd = cd_cmd + make_args + no_echo


def main():
    if not all_patches_exists(ini_file):
        return 1
    if not build_base_engine():
        return 2
    while True:
        ans = tune_main_loop(ini_file)
        if ans == 'finished' or ans == 'error':
            break
    return 3 if ans == 'error' else 0


def all_patches_exists(ini_file):
    with open(ini_file, 'r') as ini:
        for line in ini:
            if line.startswith('(done)'):
                continue
            cur_patch = line.split()[0]
            if not os.path.isfile(patches_path + '/' + cur_patch):
                print(cur_patch + ' is absent, abandoned')
                return False
        return True


def build_base_engine():
    commands = [make_cmd, move_cmd + base_exec]
    for cmd in commands:
        returncode = call(cmd, shell=True)
        if returncode != 0:
            print('\nCould not execute command: %s\n' % cmd)
            print(returncode)
            return False
    return True


def tune_main_loop(ini_file):
    cur_patch, time_control = get_cur_patch_file(ini_file)
    if not cur_patch:
        return 'finished'
    cutechess_cmd = cd_cmd + cutechess_args.replace('time_control',
                                                    time_control)
    print('processing ' + cur_patch + '...')
    sys.stdout.flush()

    commands = [patch_cmd + '/' + cur_patch + ' > ' + log_filename,
                make_cmd,
                move_cmd + patched_exec,
                unpatch_cmd + '/' + cur_patch + ' > ' + log_filename,
                cutechess_cmd]

    cur_time = [0]
    for cmd in commands:
        ans = process_command(cmd, cur_patch, cur_time, cutechess_cmd)
        if ans is None:
            return 'error'
    update_ini_file(ini_file, cur_patch, ans)
    return 'ok'


def process_command(cmd, cur_patch, cur_time, cutechess_cmd):
    # print('>>debug: ' + cmd)
    ans = ['']
    returncode = call(cmd, shell=True)
    if returncode != 0:
        print('\nCould not execute command: %s\n' % cmd)
        print(returncode)
        call(unpatch_cmd + '/' + cur_patch + no_echo, shell=True)
        return False
    elif cmd.startswith(unpatch_cmd):
        cur_time[0] = time()
    elif cmd.startswith(cutechess_cmd):
        with open(cutechess_path + '/' + log_filename) as myfile:
            lines = list(myfile)
            if not parse_match_result(lines, cur_patch, cur_time[0], ans):
                return None
            print(ans[0])
    return ans[0]


def parse_match_result(strings, cur_patch, t0, ans):
    losses_on_time = 0
    for st in strings:
        if 'on time' in st:
            losses_on_time += 1

    games = 'unknown'
    for st in reversed(strings):
        if st.startswith('Score of'):
            games = st.split()[-1]
            break

    elo = 'unknown'
    for st in reversed(strings):
        if st.lower().startswith('elo diff'):
            elo = st.split(sep=':')[-1].strip()
            break

    time_elapsed = time() - t0
    for st in reversed(strings):
        if st.startswith('SPRT'):
            add_info = ' (' + games + ' games, ' + \
                elo + ' elo, ' + str(int(time_elapsed)) + 's, ' + \
                str(losses_on_time) + ' losses on time)'
            if 'H0' in st:
                ans[0] = (cur_patch + ' is red' + add_info)
            elif 'H1' in st:
                ans[0] = (cur_patch + ' is green' + add_info)
            elif int(games) == 2*int(rounds):
                ans[0] = (cur_patch + ' is yellow' + add_info)
            else:
                ans[0] = (cur_patch + ': match aborted' + add_info)
                return False if stop_if_match_aborted else True
            break
    return True


def get_cur_patch_file(ini_file):
    with open(ini_file, 'r') as ini:
        for line in ini:
            if line.startswith('(done)'):
                continue
            return line.split()[:2]
    return None, None


def update_ini_file(ini_file, cur_patch, ans):
    patch, _ = get_cur_patch_file(ini_file)
    assert(patch == cur_patch)
    lines = []
    with open(ini_file, 'r') as ini:
        for line in ini:
            lines.append(line)
    for i, line in enumerate(lines):
        if line.split()[0] == cur_patch:
            lines[i] = '(done) ' + line[:-1] + ': ' + ans + '\n'
            break
    with open(ini_file, 'w') as out:
        for line in lines:
            out.write(line)


if __name__ == "__main__":
    main()

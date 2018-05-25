# -*- coding: utf-8 -*-
"""
  K2tune v0.2,

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
  After each match the result puts to standard output (patch XXXXX is
  good / bad / within bounds).

  Typical usage:
1. Open this script in text editor and configure paths, parameters, etc.
2. Open terminal (command promt) and change current directory to path
containing this script
3. Execute command: python k2tune.py
"""

from subprocess import call
import sys
import os.path
from time import time
from platform import system

patch_filenames = [
    'passer01.patch',
    'passer02.patch',
    'king_safety01.patch',
    'king_safety02.patch',
    ]

time_control = '40/4+0.01'
concurrency = '4'  # how many cpu cores must be used
elo_lower_bound = '0'
elo_upper_bound = '8'
book = 'hyatt.pgn format=epd order=random'
rounds = '40000'  # each round consists of two games
base_engine_name = 'k2'  # the name in 'engines.json'
patched_engine_name = 'k2_exp'
stop_if_match_aborted = False  # do not proceed next patch on error

if system() == 'Windows':
    src_path = 'd:/chess/engine_name/source'
    patches_path = 'd:/chess/engine_name/patches'
    cutechess_path = 'd:/chess/cutechess-cli'
    diff_path = 'c:/progra~1/cygwin/usr/bin/'  # N.B. slash in the end
    cd_cmd = 'cd /d '
    del_cmd = 'del /Q '
    make_cmd = 'mingw32-make -f Makefile.win'
    cutechess_cmd = 'cutechess-cli'
    base_exec = 'k2.exe'
    patched_exec = 'k2_exp.exe'
    no_echo = ''
else:
    src_path = '~/chess/engine_name/source'
    patches_path = '~/chess/engine_name/patches'
    cutechess_path = '~/chess/cutechess-cli'
    diff_path = ''
    cd_cmd = 'cd '
    del_cmd = 'rm -f '
    make_cmd = 'make'
    cutechess_cmd = './cutechess-cli.sh'
    base_exec = 'k2'
    patched_exec = 'k2_exp'
    no_echo = ' > /dev/null'

log_filename = 'k2match.log'
pgn_out_filename = 'k2match.pgn'

engines_path = cutechess_path + '/engines'
src_exec_path = src_path + '/bin/Release'  # the path from Makefile

patch_cmd = diff_path + 'patch -d ' + src_path + ' <' + patches_path
unpatch_cmd = diff_path + 'patch -R -d ' + src_path + ' <' + patches_path
make_args = src_path + ' && ' + make_cmd
move_args = src_exec_path + '/' + base_exec + ' ' + engines_path + '/'

if system() == 'Windows':
    move_cmd = 'move /Y ' + move_args.replace('/', '\\')
else:
    move_cmd = 'mv -f ' + move_args

cutechess_args = cutechess_path + ' && ' + del_cmd + \
    pgn_out_filename + ' && ' + \
    cutechess_cmd + ' -engine conf=' + patched_engine_name + ' ' \
    '-engine conf=' + base_engine_name + ' ' \
    '-openings file=' + book + ' ' \
    '-pgnout ' + pgn_out_filename + ' ' \
    '-resign movecount=5 score=620 ' \
    '-draw movenumber=90 movecount=10 score=250 ' \
    '-ratinginterval 4 ' \
    '-concurrency ' + concurrency + ' ' \
    '-each tc=' + time_control + ' -games 2 -rounds ' + rounds + ' -repeat ' \
    '-sprt elo0=' + elo_lower_bound + ' elo1=' + elo_upper_bound + \
    ' alpha=0.05 beta=0.05 ' \
    '> ' + log_filename

make_cmd = cd_cmd + make_args + no_echo
cutechess_cmd = cd_cmd + cutechess_args


def main(argv=None):

    def ParseCurrentMatchResult():

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
                    print(cur_patch + ' is bad' + add_info)
                elif 'H1' in st:
                    print(cur_patch + ' is good' + add_info)
                elif int(games) == 2*int(rounds):
                    print(cur_patch + ' is within bounds' + add_info)
                else:
                    print(cur_patch + ': match aborted' + add_info)
                    return 1 if stop_if_match_aborted else 0
                break
        return 0

    for cur_patch in patch_filenames:
        if not os.path.isfile(patches_path + '/' + cur_patch):
            print(cur_patch + ' is absent, abandoned')
            return 1

    commands = [make_cmd, move_cmd + base_exec]
    for cmd in commands:
        returncode = call(cmd, shell=True)
        if returncode != 0:
            print('\nCould not execute command: %s\n' % cmd)
            print(returncode)
            return 1

    for cur_patch in patch_filenames:
        print('processing ' + cur_patch + '...')
        sys.stdout.flush()

        commands = [patch_cmd + '/' + cur_patch + no_echo,
                    make_cmd,
                    move_cmd + patched_exec,
                    unpatch_cmd + '/' + cur_patch + no_echo,
                    cutechess_cmd]

        for cmd in commands:
            # print('>>debug: ' + cmd)
            returncode = call(cmd, shell=True)
            if returncode != 0:
                print('\nCould not execute command: %s\n' % cmd)
                print(returncode)
                call(unpatch_cmd + '/' + cur_patch + no_echo, shell=True)
                return 1
            elif unpatch_cmd in cmd:
                t0 = time()
            elif cutechess_cmd in cmd:
                with open(cutechess_path + '/' + log_filename) as myfile:
                    strings = list(myfile)
                    if ParseCurrentMatchResult() > 0:
                        return 1

if __name__ == "__main__":
    main()

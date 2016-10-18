K2, the chess engine
====================

Author: Sergey Meus, Russian Federation.

Latest release version: 0.87 (October, 2016).

Strength: about 2600 Elo.

Link for download executable: https://yadi.sk/d/VRKs5oiWwuXAh

Main features:
- supports both UCI and XB protocol;
- chess board represented as 120-element array (so-called 0x88 move generator);
- alpha-beta search function with such improvements as: principal variation
search, null move pruning, late move reduction and pruning, static exchange
evaluation (SEE), futility pruning, killer moves heuristic, history heuristic,
check extension, recapture extension, transposition table (TT);
- quiescence search function with SEE cutoff and delta pruning, without TT;
- evaluation function with separate middle- and endgame terms such as material,
piece-square tables, some pawn terms (passers, connected passers, unstoppable,
double, isolated, passer closed to king, gaps in pawn structure), king safety
(pawn shelter, penalty for opponent pieces at distance less than 4 to king,
penalty for wrong king placing and enemy rooks/queens on adjacent open files),
paired bishops, bishop mobility, clamped rook penalty, rooks on open files,
rooks on 7th and 8th rank, some endgame cases such as KPk, KN(B)k, KN(B)kp,
KN(B)N(B)k, KN(B)Pk, pawn absence for both sides, opposite-colored bishops. 

K2 is a hobby project, the aim is to have some fun with experiments on chess
algorithms.

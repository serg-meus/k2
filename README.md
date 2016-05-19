K2, the chess engine
====================

Author: Sergey Meus, Russian Federation.

Version number: 0.83 (May, 2016).

Strength: about 2500 Elo.

Link for download executable: https://yadi.sk/d/VU619uY-rmKth

Main features:
- supports both UCI and XB protocol;
- chess board represented by 120-byte array (so-called 0x88 move generator);
- alpha-beta search function with such improvements as: PVS, Null Move Pruning, LMR, SEE, futility pruning, killer moves, history heuristic, check extension, recapture extension, transposition table;
- quiescence search function with SEE cutoff and delta pruning, without TT;
- evaluation function with separate middle- and endgame terms such as material,
  piece-square tables, some pawn terms (passers, connected passers, unstoppable, double, isolated, closed to king), king safety (pawn shelter, penalty for opponent pieces at distance less than 4 to king), paired bishops, bishop mobility, clamped rook penalty, rooks on open files, rooks on 7th and 8th rank, some endgame cases (KPk, KNk, KBk, KNkp, KBkp, KNNk, KBBk, KNBk, KBPk, KNPk, pawn absence for both sides, opposite-colored bishops). 

K2 is hobby project, the aim is to have some fun with experiments on chess algorithms.

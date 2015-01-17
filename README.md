K2
==

The chess engine.

Version number: 0.71 (01/11/2015).

[Link](https://yadi.sk/d/OcnaK2IbdtMKG) for download Win32 executable.

Main features:
- supports both UCI and Xboard interface;
- chess board presented as 120-byte array (so-called 0x88 move generator);
- simple search function with null move, PVS, LMR, futility pruning and transposition table;
- quiescence search with SEE pruning;
- position evaluation function with middle-game and endgame terms such as material,
  piece-square table, some pawn terms, king safety, some endgame knowlege, etc

K2 is the hobby project, the aim is to have some fun with experiments on chess algorithms.

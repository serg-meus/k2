K2 v0.67
==
The chess engine. 

. Main features:
- supports both UCI and Xboard interface;
- chess board presented as 120-byte array (so-called 0x88 move generator);
- simple search function with null move, PVS, LMR, SEE, futility and transposition table;
- position evaluation function with middle-game and endgame terms such as material,
  piece-square table, some pawn terms, king safety, etc

K2 is the hobby project, the aim is to have some fun with experiments on chess algorithms.

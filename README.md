K2 v0.43f
==
The chess engine.
Main features:
- not so strong yet, strength is about 2100 Elo points;
- supports both UCI and Xboard interface;
- chess board presented as 120-byte array (so-called 0x88 move generator);
- simple search function with null move, PVS, futility and transposition table;
- position evaluation function with middle-game and endgame terms such as material,
  piece-square table, some pawn terms, king safety, etc

K2 is the hobby project, the aim is to have some fun with experiments on chess algorithms.

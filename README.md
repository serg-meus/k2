K2 v.043f
==

Chess engine with UCI and Xboard/Winboard interface support.

Main features:
- not so strong yet, only about 2100 Elo points;
- chess board represents as 128-byte array (so-called 0x88 move generator);
- simple search function with null move, PVS, futility and transposition table;
- simple position evaluation with middle-game and endgame terms, such as material,
  piece-square table, some pawn terms, king safety, etc

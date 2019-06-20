all:

	g++ main.cpp engine.cpp eval.cpp hash.cpp movegen.cpp chess.cpp Timer.cpp -DNDEBUG=1 -std=c++0x -O3 -pthread -o k2_095

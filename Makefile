all:

	g++ main.cpp engine.cpp eval.cpp hash.cpp movegen.cpp chess.cpp pst.cpp Timer.cpp -std=c++0x -O3 -DNDEBUG=1 -pthread -o ./bin/Release/k2

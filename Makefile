all:

	g++ main.cpp chess.cpp eval.cpp movegen.cpp -std=c++0x -O0 -pthread -o ./bin/Debug/k2

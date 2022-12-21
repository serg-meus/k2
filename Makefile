CC=g++

CFLAGS=-std=c++0x -Werror -Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual -Wconversion -Wsign-conversion -Wctor-dtor-privacy -Wfloat-equal -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wsign-promo -Wshadow -Weffc++

COPT=-O3 -DNDEBUG -flto

CPP_FILES=utils.cpp bitboards.cpp board.cpp chess.cpp eval.cpp engine.cpp k2.cpp

O_FILES=utils.o bitboards.o board.o chess.o eval.o engine.o k2.o

all: prod

prod: $(O_FILES)
	$(CC) $(COPT) $(O_FILES) -o k2

test:
	$(CC) $(CFLAGS) -O2 utils.cpp bitboards.cpp board.cpp chess.cpp eval.cpp engine.cpp test_all.cpp -o test_all

static:
	$(CC) $(CFLAG) $(COPT) -s -static $(CPP_FILES) -o k2

utils.o: utils.cpp
	$(CC) -c $(CFLAGS) $(COPT) utils.cpp

bitboards.o: bitboards.cpp
	$(CC) -c $(CFLAGS) $(COPT) bitboards.cpp

board.o: board.cpp
	$(CC) -c $(CFLAGS) $(COPT) board.cpp

chess.o: chess.cpp
	$(CC) -c $(CFLAGS) $(COPT) chess.cpp

eval.o: eval.cpp
	$(CC) -c $(CFLAGS) $(COPT) eval.cpp

engine.o: engine.cpp
	$(CC) -c $(CFLAGS) $(COPT) engine.cpp

k2.o: k2.cpp
	$(CC) -c $(CFLAGS) $(COPT) k2.cpp

prof:
	$(CC) $(CFLAGS) -pg -O2 $(CPP_FILES) k2
	./k2
	gprof k2 gmon.out > gmon.txt

wprof:
	$(CC) $(CFLAGS) -pg -O2 $(CPP_FILES) -o k2
	k2.exe
	gprof k2.exe gmon.out > gmon.txt

32bit: $(O_FILES)
	$(CC) $(CFLAGS) $(COPT) -m32 $(O_FILES) -o k2

clean:
	rm -f *.o test_all k2 gmon.* *.depend *.layout
	rm -rf bin/ obj/

wclean:
	del /q *.o *.exe gmon.* *.depend *.layout
	rmdir /s /q bin obj

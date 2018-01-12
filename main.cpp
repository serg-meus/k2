#include "main.h"

//--------------------------------
// K2, the chess engine
// Author: Sergey Meus (serg_meus@mail.ru)
// Krasnoyarsk Krai, Russia
// 2012-2017
//--------------------------------





#define UNUSED(x) (void)(x)
//--------------------------------
int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    k2movegen *movegen = new k2movegen();

#ifndef NDEBUG
    movegen->RunUnitTests();
    std::cout << "All unit tests passed" << std::endl;
#endif

    delete movegen;
}

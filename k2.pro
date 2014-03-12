TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    Timer.cpp \
    pst.cpp \
    movegen.cpp \
    main.cpp \
    hash.cpp \
    eval.cpp \
    engine.cpp \
    chess.cpp

OTHER_FILES +=

HEADERS += \
    Timer.h \
    movegen.h \
    main.h \
    hash.h \
    extern.h \
    eval.h \
    engine.h \
    chess.h

QMAKE_CXXFLAGS += -std=c++11

release
{
    DEFINES += NDEBUG
    QMAKE_LFLAGS += -static
}

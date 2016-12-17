TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    Timer.cpp \
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
    eval.h \
    engine.h \
    chess.h \
    short_list.h

QMAKE_CXXFLAGS += -std=c++0x

CONFIG -= exceptions
# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2

# add the desired -O3 if not present
QMAKE_CXXFLAGS_RELEASE *= -O3

debug_and_release {
    CONFIG -= debug_and_release
    CONFIG += debug_and_release
}

CONFIG(debug, debug|release) {
    CONFIG -= debug release
    CONFIG += debug
}

CONFIG(release, debug|release) {
    CONFIG -= debug release
    CONFIG += release
    DEFINES += NDEBUG
    LIBS += -pthread
    win32:QMAKE_LFLAGS += -static
}

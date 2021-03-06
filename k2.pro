TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    main.cpp \
    chess.cpp \
    eval.cpp \
    movegen.cpp \
    hash.cpp \
    engine.cpp \
    Timer.cpp

OTHER_FILES +=

HEADERS += \
    main.h \
    chess.h \
    eval.h \
    movegen.h \
    hash.h \
    engine.h \
    Timer.h

QMAKE_CXXFLAGS += -std=c++0x

CONFIG -= exceptions
# remove possible other optimization flags
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2

# add the desired -O3 if not present
QMAKE_CXXFLAGS_RELEASE *= -O3

LIBS += -pthread

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
    win32:QMAKE_LFLAGS += -static
    win32:DEFINES += WIN32
}

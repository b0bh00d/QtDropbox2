QT += core network testlib xml widgets qml quick

TARGET = qtdropbox2test
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

DEFINES += SRCDIR=\\\"$$PWD/\\\"
DEFINES += SHOW_OUTPUT
SOURCES += qtdropbox2test.cpp
HEADERS += qtdropbox2test.h \
           config.h

INCLUDEPATH += ../src

DISTFILES += config.h.in
QTVER=5_6_2
BUILDTYPE=Release
CONFIG(debug, debug|release) {
    BUILDTYPE=Debug
}
OUTPUT=

win32 {
    COMPILER=MSVC2013
    CONFIG(debug, debug|release) {
        OUTPUT=/debug
    }
    else {
        OUTPUT=/release
    }
}
unix {
    CONFIG += c++11
    COMPILER=GCC

    macx {
        COMPILER=clang
        LIBS += -stdlib=libc++
        CONFIG += x86_64
#        QMAKE_MAC_SDK = macosx10.11
#        QMAKE_MACOSX_DEPLOYMENT_TARGET=10.7
        QMAKE_CXXFLAGS+=-std=c++11 -stdlib=libc++
    }
}
include(../qtdropbox2.pri)

#QMAKE_SUBSTITUTES       +=  $$PWD/config.h.in
# I don't understand QtCreator's logic of putting the shadow build folder
# outside the project folder, but that's the default.
#LIBS += -lQtDropbox2 -L../../build-qtdropbox2-Desktop_Qt_$${QTVER}_$${COMPILER}_64bit-$${BUILDTYPE}$${OUTPUT}

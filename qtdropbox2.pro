INCLUDEPATH += $$PWD/src

TEMPLATE = lib
#CONFIG += dll

CONFIG += C++11
QT += network xml
QT -= gui

TARGET = QtDropbox2

DEFINES += QTDROPBOX_LIBRARY
#DEFINES += QTDROPBOX_DEBUG

documentation.commands = doxygen doc/doxygen.conf
QMAKE_EXTRA_TARGETS += documentation

unix {
    macx {
        CONFIG += x86_64
        QMAKE_MAC_SDK=macosx10.11
    }
}
win32 {
    CONFIG(debug, debug|release) {
    } else {
        QMAKE_CXXFLAGS_RELEASE += $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
        QMAKE_LFLAGS_RELEASE += $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
    }
}

include(qtdropbox2.pri)

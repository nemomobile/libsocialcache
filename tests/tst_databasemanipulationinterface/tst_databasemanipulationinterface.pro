include(../../common.pri)

TEMPLATE = app
TARGET = tst_databasemanipulationinterface
QT += sql testlib

INCLUDEPATH += ../../src/lib/

HEADERS +=  ../../src/lib/databasemanipulationinterface.h

SOURCES +=  ../../src/lib/databasemanipulationinterface.cpp \
            main.cpp


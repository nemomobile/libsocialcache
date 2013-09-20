include(../../common.pri)

TEMPLATE = app
TARGET = tst_databasemanipulationinterface
QT += sql testlib

INCLUDEPATH += ../../src/lib/

HEADERS +=  ../../src/lib/databasemanipulationinterface.h \
            ../../src/lib/semaphore_p.h

SOURCES +=  ../../src/lib/databasemanipulationinterface.cpp \
            ../../src/lib/semaphore_p.cpp \
            main.cpp


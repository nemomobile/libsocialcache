include(../../common.pri)

TEMPLATE = app
TARGET = tst_abstractsocialcachedatabase
QT += sql testlib

INCLUDEPATH += ../../src/lib/

HEADERS +=  ../../src/lib/abstractsocialcachedatabase.h \
            ../../src/lib/abstractsocialcachedatabase_p.h \
            ../../src/lib/semaphore_p.h

SOURCES +=  ../../src/lib/abstractsocialcachedatabase.cpp \
            ../../src/lib/semaphore_p.cpp \
            main.cpp


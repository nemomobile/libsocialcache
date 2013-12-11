include(../../common.pri)

TEMPLATE = app
TARGET = tst_facebookpost
QT += network sql testlib

DEFINES += NO_KEY_PROVIDER

INCLUDEPATH += ../../src/lib/

HEADERS +=  ../../src/lib/semaphore_p.h \
            ../../src/lib/socialsyncinterface.h \
            ../../src/lib/abstractsocialcachedatabase.h \
            ../../src/lib/abstractsocialcachedatabase_p.h \
            ../../src/lib/abstractsocialpostcachedatabase.h \
            ../../src/lib/facebookpostsdatabase.h

SOURCES +=  ../../src/lib/semaphore_p.cpp \
            ../../src/lib/socialsyncinterface.cpp \
            ../../src/lib/abstractsocialcachedatabase.cpp \
            ../../src/lib/abstractsocialpostcachedatabase.cpp \
            ../../src/lib/facebookpostsdatabase.cpp \
            main.cpp

target.path = /opt/tests/libsocialcache
INSTALLS += target

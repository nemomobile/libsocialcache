include(../../common.pri)

TEMPLATE = app
TARGET = tst_dropboximage
QT += network sql testlib

DEFINES += NO_KEY_PROVIDER

INCLUDEPATH += ../../src/lib/
INCLUDEPATH += ../../src/qml/

HEADERS +=  ../../src/lib/semaphore_p.h \
            ../../src/lib/socialsyncinterface.h \
            ../../src/lib/abstractsocialcachedatabase.h \
            ../../src/lib/abstractsocialcachedatabase_p.h \
            ../../src/lib/dropboximagesdatabase.h \
            ../../src/lib/abstractimagedownloader.h \
            ../../src/lib/abstractimagedownloader_p.h \
            ../../src/qml/abstractsocialcachemodel.h \
            ../../src/qml/abstractsocialcachemodel_p.h

SOURCES +=  ../../src/lib/semaphore_p.cpp \
            ../../src/lib/socialsyncinterface.cpp \
            ../../src/lib/abstractsocialcachedatabase.cpp \
            ../../src/lib/dropboximagesdatabase.cpp \
            ../../src/lib/abstractimagedownloader.cpp \
            ../../src/qml/abstractsocialcachemodel.cpp \
            main.cpp

target.path = /opt/tests/libsocialcache
INSTALLS += target

include(../../common.pri)

TEMPLATE = app
TARGET = tst_onedriveimage
QT += network sql testlib

DEFINES += NO_KEY_PROVIDER

INCLUDEPATH += ../../src/lib/
INCLUDEPATH += ../../src/qml/
INCLUDEPATH += ../../src/qml/onedrive

HEADERS +=  ../../src/lib/semaphore_p.h \
            ../../src/lib/socialsyncinterface.h \
            ../../src/lib/abstractsocialcachedatabase.h \
            ../../src/lib/abstractsocialcachedatabase_p.h \
            ../../src/lib/onedriveimagesdatabase.h \
            ../../src/qml/onedrive/onedriveimagecachemodel.h \
            ../../src/qml/onedrive/onedriveimagedownloader.h \
            ../../src/lib/abstractimagedownloader.h \
            ../../src/lib/abstractimagedownloader_p.h \
            ../../src/qml/abstractsocialcachemodel.h \
            ../../src/qml/abstractsocialcachemodel_p.h 

SOURCES +=  ../../src/lib/semaphore_p.cpp \
            ../../src/lib/socialsyncinterface.cpp \
            ../../src/lib/abstractsocialcachedatabase.cpp \
            ../../src/lib/onedriveimagesdatabase.cpp \
            ../../src/qml/onedrive/onedriveimagecachemodel.cpp \
            ../../src/qml/onedrive/onedriveimagedownloader.cpp \
            ../../src/lib/abstractimagedownloader.cpp \
            ../../src/qml/abstractsocialcachemodel.cpp \
            main.cpp

target.path = /opt/tests/libsocialcache
INSTALLS += target

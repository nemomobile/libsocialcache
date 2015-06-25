include(../../common.pri)

TEMPLATE = app
TARGET = tst_facebookimage
QT += network sql testlib

CONFIG += link_pkgconfig
PKGCONFIG += accounts-qt5

DEFINES += NO_KEY_PROVIDER

INCLUDEPATH += ../../src/lib/
INCLUDEPATH += ../../src/qml/

HEADERS +=  ../../src/lib/semaphore_p.h \
            ../../src/lib/socialsyncinterface.h \
            ../../src/lib/abstractsocialcachedatabase.h \
            ../../src/lib/abstractsocialcachedatabase_p.h \
            ../../src/lib/facebookimagesdatabase.h \
            ../../src/lib/abstractimagedownloader.h \
            ../../src/lib/abstractimagedownloader_p.h \
            ../../src/qml/abstractsocialcachemodel.h \
            ../../src/qml/abstractsocialcachemodel_p.h \
            ../../src/qml/facebook/facebookimagecachemodel.h \
            ../../src/qml/facebook/facebookimagedownloader_p.h \
            ../../src/qml/facebook/facebookimagedownloader.h

SOURCES +=  ../../src/lib/semaphore_p.cpp \
            ../../src/lib/socialsyncinterface.cpp \
            ../../src/lib/abstractsocialcachedatabase.cpp \
            ../../src/lib/facebookimagesdatabase.cpp \
            ../../src/lib/abstractimagedownloader.cpp \
            ../../src/qml/abstractsocialcachemodel.cpp \
            ../../src/qml/facebook/facebookimagecachemodel.cpp \
            ../../src/qml/facebook/facebookimagedownloader.cpp \
            main.cpp

target.path = /opt/tests/libsocialcache
INSTALLS += target

include(../../common.pri)

TEMPLATE = app
TARGET = tst_facebookimage
QT += network sql testlib

DEFINES += NO_KEY_PROVIDER

INCLUDEPATH += ../../src/lib/
INCLUDEPATH += ../../src/qml/

HEADERS +=  ../../src/lib/socialsyncinterface.h \
            ../../src/lib/databasemanipulationinterface.h \
            ../../src/lib/facebookimagesdatabase.h \
            ../../src/qml/abstractsocialcachemodel.h \
            ../../src/qml/abstractsocialcachemodel_p.h \
            ../../src/qml/facebook/abstractfacebookcachemodel.h \
            ../../src/qml/facebook/abstractfacebookcachemodel_p.h \
            ../../src/qml/facebook/facebookimagecachemodel.h
SOURCES +=  ../../src/lib/socialsyncinterface.cpp \
            ../../src/lib/databasemanipulationinterface.cpp \
            ../../src/lib/facebookimagesdatabase.cpp \
            ../../src/qml/abstractsocialcachemodel.cpp \
            ../../src/qml/facebook/abstractfacebookcachemodel.cpp \
            ../../src/qml/facebook/facebookimagecachemodel.cpp \
            main.cpp


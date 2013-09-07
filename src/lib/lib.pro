include(../../common.pri)

TEMPLATE = lib
CONFIG += qt create_prl no_install_prl create_pc
QT += sql

isEmpty(PREFIX) {
    PREFIX=/usr
}

TARGET = socialcache
target.path = $$INSTALL_ROOT$$PREFIX/lib

HEADERS = \
    databasemanipulationinterface.h \
    socialsyncinterface.h \
    facebookimagesdatabase.h \
    databasemanipulationinterface_p.h

SOURCES = \
    databasemanipulationinterface.cpp \
    socialsyncinterface.cpp \
    facebookimagesdatabase.cpp

headers.files = $$HEADERS
headers.path = /usr/include/socialcache


QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Social cache development files
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += target headers


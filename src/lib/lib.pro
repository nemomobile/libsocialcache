include(../../common.pri)

TEMPLATE = lib
CONFIG += qt create_prl no_install_prl create_pc link_pkgconfig
QT += sql
VERSION = 0.0.36

isEmpty(PREFIX) {
    PREFIX=/usr
}

TARGET = socialcache
target.path = $$INSTALL_ROOT$$PREFIX/lib

HEADERS = \
    semaphore_p.h \
    socialsyncinterface.h \
    abstractimagedownloader.h \
    abstractimagedownloader_p.h \
    abstractsocialcachedatabase.h \
    abstractsocialcachedatabase_p.h \
    abstractsocialpostcachedatabase.h \
    socialnetworksyncdatabase.h \
    facebookimagesdatabase.h \
    facebookcontactsdatabase.h \
    facebooknotificationsdatabase.h \
    facebookpostsdatabase.h \
    twitterpostsdatabase.h \
    twitternotificationsdatabase.h

SOURCES = \
    semaphore_p.cpp \
    socialsyncinterface.cpp \
    abstractimagedownloader.cpp \
    abstractsocialcachedatabase.cpp \
    abstractsocialpostcachedatabase.cpp \
    socialnetworksyncdatabase.cpp \
    facebookimagesdatabase.cpp \
    facebookcontactsdatabase.cpp \
    facebooknotificationsdatabase.cpp \
    facebookpostsdatabase.cpp \
    twitterpostsdatabase.cpp \
    twitternotificationsdatabase.cpp

headers.files = $$HEADERS
headers.path = /usr/include/socialcache


QMAKE_PKGCONFIG_NAME = lib$$TARGET
QMAKE_PKGCONFIG_DESCRIPTION = Social cache development files
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += target headers


TEMPLATE = subdirs
SUBDIRS = src tests
tests.depends = src

OTHER_FILES += rpm/libsocialcache.spec


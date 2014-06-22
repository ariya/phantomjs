SOURCES = journald.c

CONFIG += link_pkgconfig
PKGCONFIG_PRIVATE += libsystemd-journal

CONFIG -= qt

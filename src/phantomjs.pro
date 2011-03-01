TEMPLATE = app
TARGET = phantomjs
DESTDIR = ../bin
SOURCES = phantomjs.cpp
RESOURCES = phantomjs.qrc
QT += network webkit
CONFIG += console
#DEFINES += QT_NO_DEBUG_OUTPUT

include(gif/gif.pri)

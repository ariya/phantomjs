TEMPLATE = app
TARGET = phantomjs
DESTDIR = ../bin
SOURCES = phantomjs.cpp
RESOURCES = phantomjs.qrc
QT += network webkit
CONFIG += console

include(gif/gif.pri)

TEMPLATE = app
TARGET = phantomjs
DESTDIR = ../bin
HEADERS += csconverter.h
SOURCES = phantomjs.cpp csconverter.cpp
RESOURCES = phantomjs.qrc
QT += network webkit
CONFIG += console

include(gif/gif.pri)

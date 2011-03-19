TEMPLATE = app
TARGET = phantomjs
DESTDIR = ../bin
HEADERS += csconverter.h
SOURCES = phantomjs.cpp csconverter.cpp
RESOURCES = phantomjs.qrc
QT += network webkit
CONFIG += console

include(gif/gif.pri)

win32: RC_FILE = phantomjs_win.rc
os2:   RC_FILE = phantomjs_os2.rc

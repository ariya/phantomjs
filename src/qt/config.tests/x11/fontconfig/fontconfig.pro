SOURCES = fontconfig.cpp
CONFIG += x11
CONFIG -= qt
LIBS += -lfreetype -lfontconfig
include(../../unix/freetype/freetype.pri)

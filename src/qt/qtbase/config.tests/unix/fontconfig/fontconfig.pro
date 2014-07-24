SOURCES = fontconfig.cpp
CONFIG -= qt
LIBS += -lfreetype -lfontconfig
include(../../unix/freetype/freetype.pri)

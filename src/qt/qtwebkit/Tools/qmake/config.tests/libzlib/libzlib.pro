SOURCES = libzlib.cpp
OBJECTS_DIR = obj
CONFIG += console
win32: LIBS += -lzlib
else: LIBS += -lz

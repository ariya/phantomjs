SOURCES = mitshm.cpp
CONFIG += x11
CONFIG -= qt
LIBS += -lXext
hpux*:DEFINES+=Q_OS_HPUX

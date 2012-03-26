TARGET = audio
SOURCES = audio.cpp

INCLUDEPATH += $${EPOCROOT}epoc32/include/mmf/server
INCLUDEPATH += $${EPOCROOT}epoc32/include/mmf/common
INCLUDEPATH += $${EPOCROOT}epoc32/include/platform

LIBS += -lmmfdevsound
CONFIG -= qt
QT =

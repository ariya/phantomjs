TEMPLATE = app
TARGET = lexgen
include(lexgen.pri)
SOURCES += main.cpp \
    generator.cpp
QT = core

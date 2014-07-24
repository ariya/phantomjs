# -------------------------------------------------------------------
# Project file for the ImageDiff binary
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = app

TARGET = ImageDiff
DESTDIR = $$ROOT_BUILD_DIR/bin

QT = core gui widgets

SOURCES = qt/ImageDiff.cpp

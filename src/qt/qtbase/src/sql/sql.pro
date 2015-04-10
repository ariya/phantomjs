TARGET	   = QtSql
QT         = core-private

DEFINES += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x62000000

MODULE_PLUGIN_TYPES = \
    sqldrivers

QMAKE_DOCS = $$PWD/doc/qtsql.qdocconf

load(qt_module)

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h
SQL_P = sql

include(kernel/kernel.pri)
include(drivers/drivers.pri)
include(models/models.pri)

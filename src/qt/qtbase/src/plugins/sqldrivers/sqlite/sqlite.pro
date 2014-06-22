TARGET = qsqlite

SOURCES = smain.cpp
OTHER_FILES += sqlite.json
include(../../../sql/drivers/sqlite/qsql_sqlite.pri)

wince*: DEFINES += HAVE_LOCALTIME_S=0

PLUGIN_CLASS_NAME = QSQLiteDriverPlugin
include(../qsqldriverbase.pri)

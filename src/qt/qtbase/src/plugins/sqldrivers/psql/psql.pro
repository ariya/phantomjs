TARGET = qsqlpsql

SOURCES = main.cpp
OTHER_FILES += psql.json
include(../../../sql/drivers/psql/qsql_psql.pri)

PLUGIN_CLASS_NAME = QPSQLDriverPlugin
include(../qsqldriverbase.pri)

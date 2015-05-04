TARGET = qsqlodbc

SOURCES = main.cpp
OTHER_FILES += odbc.json
include(../../../sql/drivers/odbc/qsql_odbc.pri)

PLUGIN_CLASS_NAME = QODBCDriverPlugin
include(../qsqldriverbase.pri)

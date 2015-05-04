TARGET = qsqlmysql

SOURCES = main.cpp
OTHER_FILES += mysql.json
include(../../../sql/drivers/mysql/qsql_mysql.pri)

PLUGIN_CLASS_NAME = QMYSQLDriverPlugin
include(../qsqldriverbase.pri)

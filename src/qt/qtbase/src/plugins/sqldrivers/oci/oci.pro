TARGET = qsqloci

SOURCES = main.cpp
OTHER_FILES += oci.json
include(../../../sql/drivers/oci/qsql_oci.pri)

PLUGIN_CLASS_NAME = QOCIDriverPlugin
include(../qsqldriverbase.pri)

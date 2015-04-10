include(../tests.pri)
SOURCES += $${TARGET}.cpp
QT += webkit-private
DEFINES += IMPORT_DIR=\"\\\"$${ROOT_BUILD_DIR}$${QMAKE_DIR_SEP}imports\\\"\"

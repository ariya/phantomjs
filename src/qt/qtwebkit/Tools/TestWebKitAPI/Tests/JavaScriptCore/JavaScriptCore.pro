TEMPLATE = app
TARGET = tst_jsc

SOURCES += *.cpp

include(../../TestWebKitAPI.pri)

DEFINES += APITEST_SOURCE_DIR=\\\"$$PWD\\\"

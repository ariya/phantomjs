TEMPLATE = lib
CONFIG += static
TARGET = gtest

DEFINES += QT_NO_KEYWORDS

INCLUDEPATH += $$PWD/include $${ROOT_WEBKIT_DIR}/Source/WTF $${ROOT_WEBKIT_DIR}/Source/JavaScriptCore $$QT.core.includes

HEADERS = $$PWD/include/gtest/*.h $$PWD/include/gtest/internal/*.h
SOURCES = $$PWD/src/gtest-all.cc

QT =

CONFIG += compiling_thirdparty_code

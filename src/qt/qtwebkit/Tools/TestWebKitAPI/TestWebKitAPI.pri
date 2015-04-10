
INCLUDEPATH += $$PWD $${ROOT_WEBKIT_DIR}/Source/ThirdParty/gtest/include
WEBKIT += wtf javascriptcore webkit2

DEFINES += QT_NO_CAST_FROM_ASCII

QT += core core-private gui gui-private webkit quick quick-private

CONFIG += compiling_thirdparty_code

SOURCES += \
    $$PWD/JavaScriptTest.cpp \
    $$PWD/PlatformUtilities.cpp \
    $$PWD/TestsController.cpp \
    $$PWD/qt/main.cpp \
    $$PWD/qt/PlatformUtilitiesQt.cpp \
    $$PWD/qt/PlatformWebViewQt.cpp

LIBS += -L$${ROOT_BUILD_DIR}/Source/ThirdParty/gtest/$$targetSubDir() -lgtest

DEFINES += ROOT_BUILD_DIR=\\\"$${ROOT_BUILD_DIR}\\\"


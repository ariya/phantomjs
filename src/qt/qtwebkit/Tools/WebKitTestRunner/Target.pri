# -------------------------------------------------------------------
# Project file for WebKitTestRunner binary (WTR)
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = app
TARGET = WebKitTestRunner

HEADERS += \
    EventSenderProxy.h \
    GeolocationProviderMock.h \
    PlatformWebView.h \
    StringFunctions.h \
    TestController.h \
    TestInvocation.h \
    WebNotificationProvider.h \
    WorkQueueManager.h

SOURCES += \
    qt/main.cpp \
    qt/EventSenderProxyQt.cpp \
    qt/PlatformWebViewQt.cpp \
    qt/TestControllerQt.cpp \
    qt/TestInvocationQt.cpp \
    GeolocationProviderMock.cpp \
    TestController.cpp \
    TestInvocation.cpp \
    WebNotificationProvider.cpp \
    WorkQueueManager.cpp

DESTDIR = $${ROOT_BUILD_DIR}/bin

QT = core core-private gui gui-private widgets network testlib quick quick-private webkitwidgets qml-private

WEBKIT += wtf javascriptcore webkit2

DEFINES += USE_SYSTEM_MALLOC=1

INCLUDEPATH += \
    $$PWD \
    $${ROOT_WEBKIT_DIR}/Source/WebCore/platform/qt \
    $${ROOT_WEBKIT_DIR}/Tools/DumpRenderTree/qt

PREFIX_HEADER = WebKitTestRunnerPrefix.h
*-g++*:QMAKE_CXXFLAGS += "-include $$PREFIX_HEADER"
*-clang*:QMAKE_CXXFLAGS += "-include $$PREFIX_HEADER"

RESOURCES = qt/WebKitTestRunner.qrc

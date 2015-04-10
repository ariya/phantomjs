# -------------------------------------------------------------------
# Project file for the DumpRenderTree binary (DRT)
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = app

TARGET = DumpRenderTree
DESTDIR = $$ROOT_BUILD_DIR/bin

WEBKIT += wtf javascriptcore webcore

INCLUDEPATH += \
    $$PWD/ \
    $$PWD/.. \
    $${ROOT_WEBKIT_DIR}/Source/WebCore/platform/qt \
    $${ROOT_WEBKIT_DIR}/Source/WebKit/qt/WebCoreSupport \
    $${ROOT_WEBKIT_DIR}/Source/WebKit/qt/WidgetSupport \
    $${ROOT_WEBKIT_DIR}/Source/WTF

QT = core gui network testlib webkitwidgets widgets
have?(QTPRINTSUPPORT): QT += printsupport
macx: QT += xml

HEADERS += \
    $$PWD/../WorkQueue.h \
    $$PWD/../DumpRenderTree.h \
    $$PWD/../GCController.h \
    $$PWD/../TestRunner.h \
    DumpRenderTreeQt.h \
    EventSenderQt.h \
    TextInputControllerQt.h \
    WorkQueueItemQt.h \
    TestRunnerQt.h \
    testplugin.h

SOURCES += \
    $$PWD/../WorkQueue.cpp \
    $$PWD/../DumpRenderTreeCommon.cpp \
    $$PWD/../GCController.cpp \
    $$PWD/../TestRunner.cpp \
    DumpRenderTreeQt.cpp \
    EventSenderQt.cpp \
    TextInputControllerQt.cpp \
    WorkQueueItemQt.cpp \
    TestRunnerQt.cpp \
    GCControllerQt.cpp \
    testplugin.cpp \
    DumpRenderTreeMain.cpp

wince*: {
    INCLUDEPATH += $$WCECOMPAT/include
    LIBS += $$WCECOMPAT/lib/wcecompat.lib
}

DEFINES -= USE_SYSTEM_MALLOC=0
DEFINES += USE_SYSTEM_MALLOC=1

RESOURCES = DumpRenderTree.qrc

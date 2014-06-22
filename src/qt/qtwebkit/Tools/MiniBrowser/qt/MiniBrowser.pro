# -------------------------------------------------------------------
# Project file for the WebKit2 MiniBrowser binary
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = app

WEBKIT += wtf

INCLUDEPATH += \
    $${ROOT_WEBKIT_DIR}/Source/WebCore/platform/qt \
    $${ROOT_WEBKIT_DIR}/Tools/DumpRenderTree/qt/

SOURCES += \
    BrowserWindow.cpp \
    main.cpp \
    MiniBrowserApplication.cpp \
    UrlLoader.cpp \
    utils.cpp \

HEADERS += \
    BrowserWindow.h \
    MiniBrowserApplication.h \
    UrlLoader.h \
    utils.h \

TARGET = MiniBrowser
DESTDIR = $${ROOT_BUILD_DIR}/bin

have?(FONTCONFIG): PKGCONFIG += fontconfig

QT += network gui-private quick quick-private webkit webkit-private
macx: QT += xml

RESOURCES += MiniBrowser.qrc

OTHER_FILES += qml/*

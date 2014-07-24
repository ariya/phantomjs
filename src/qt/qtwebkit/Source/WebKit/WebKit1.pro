# -------------------------------------------------------------------
# Target file for the WebKit1 static library
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = lib
TARGET = WebKit1

include(WebKit1.pri)

WEBKIT += wtf javascriptcore webcore
QT += gui

# This is relied upon by our export macros and seems not to be properly
# defined by the logic in qt_module.prf as it should
DEFINES += QT_BUILD_WEBKIT_LIB

CONFIG += staticlib

SOURCES += \
    $$PWD/qt/Api/qhttpheader.cpp \
    $$PWD/qt/Api/qwebdatabase.cpp \
    $$PWD/qt/Api/qwebelement.cpp \
    $$PWD/qt/Api/qwebhistory.cpp \
    $$PWD/qt/Api/qwebhistoryinterface.cpp \
    $$PWD/qt/Api/qwebkitglobal.cpp \
    $$PWD/qt/Api/qwebplugindatabase.cpp \
    $$PWD/qt/Api/qwebpluginfactory.cpp \
    $$PWD/qt/Api/qwebsecurityorigin.cpp \
    $$PWD/qt/Api/qwebsettings.cpp \
    $$PWD/qt/Api/qwebscriptworld.cpp \
    $$PWD/qt/WebCoreSupport/ChromeClientQt.cpp \
    $$PWD/qt/WebCoreSupport/ContextMenuClientQt.cpp \
    $$PWD/qt/WebCoreSupport/DragClientQt.cpp \
    $$PWD/qt/WebCoreSupport/DumpRenderTreeSupportQt.cpp \
    $$PWD/qt/WebCoreSupport/EditorClientQt.cpp \
    $$PWD/qt/WebCoreSupport/FrameLoaderClientQt.cpp \
    $$PWD/qt/WebCoreSupport/FrameNetworkingContextQt.cpp \
    $$PWD/qt/WebCoreSupport/GeolocationPermissionClientQt.cpp \
    $$PWD/qt/WebCoreSupport/InitWebCoreQt.cpp \
    $$PWD/qt/WebCoreSupport/InspectorClientQt.cpp \
    $$PWD/qt/WebCoreSupport/InspectorServerQt.cpp \
    $$PWD/qt/WebCoreSupport/NotificationPresenterClientQt.cpp \
    $$PWD/qt/WebCoreSupport/PlatformStrategiesQt.cpp \
    $$PWD/qt/WebCoreSupport/PopupMenuQt.cpp \
    $$PWD/qt/WebCoreSupport/QtPlatformPlugin.cpp \
    $$PWD/qt/WebCoreSupport/QtPluginWidgetAdapter.cpp \
    $$PWD/qt/WebCoreSupport/QtPrintContext.cpp \
    $$PWD/qt/WebCoreSupport/QWebFrameAdapter.cpp \
    $$PWD/qt/WebCoreSupport/QWebPageAdapter.cpp \
    $$PWD/qt/WebCoreSupport/SearchPopupMenuQt.cpp \
    $$PWD/qt/WebCoreSupport/TextCheckerClientQt.cpp \
    $$PWD/qt/WebCoreSupport/TextureMapperLayerClientQt.cpp \
    $$PWD/qt/WebCoreSupport/UndoStepQt.cpp \
    $$PWD/qt/WebCoreSupport/WebEventConversion.cpp

HEADERS += \
    $$PWD/qt/Api/qhttpheader_p.h \
    $$PWD/qt/Api/qwebdatabase.h \
    $$PWD/qt/Api/qwebelement.h \
    $$PWD/qt/Api/qwebelement_p.h \
    $$PWD/qt/Api/qwebhistory.h \
    $$PWD/qt/Api/qwebhistory_p.h \
    $$PWD/qt/Api/qwebhistoryinterface.h \
    $$PWD/qt/Api/qwebplugindatabase_p.h \
    $$PWD/qt/Api/qwebpluginfactory.h \
    $$PWD/qt/Api/qwebsecurityorigin.h \
    $$PWD/qt/Api/qwebsettings.h \
    $$PWD/qt/Api/qwebscriptworld_p.h \
    $$PWD/qt/Api/qwebkitplatformplugin.h \
    $$PWD/qt/WebCoreSupport/ChromeClientQt.h \
    $$PWD/qt/WebCoreSupport/ContextMenuClientQt.h \
    $$PWD/qt/WebCoreSupport/DragClientQt.h \
    $$PWD/qt/WebCoreSupport/EditorClientQt.h \
    $$PWD/qt/WebCoreSupport/FrameLoaderClientQt.h \
    $$PWD/qt/WebCoreSupport/FrameNetworkingContextQt.h \
    $$PWD/qt/WebCoreSupport/GeolocationPermissionClientQt.h \
    $$PWD/qt/WebCoreSupport/InitWebCoreQt.h \
    $$PWD/qt/WebCoreSupport/InspectorClientQt.h \
    $$PWD/qt/WebCoreSupport/InspectorServerQt.h \
    $$PWD/qt/WebCoreSupport/NotificationPresenterClientQt.h \
    $$PWD/qt/WebCoreSupport/PlatformStrategiesQt.h \
    $$PWD/qt/WebCoreSupport/PopupMenuQt.h \
    $$PWD/qt/WebCoreSupport/QtPlatformPlugin.h \
    $$PWD/qt/WebCoreSupport/QtPluginWidgetAdapter.h \
    $$PWD/qt/WebCoreSupport/QtPrintContext.h \
    $$PWD/qt/WebCoreSupport/QWebFrameAdapter.h \
    $$PWD/qt/WebCoreSupport/QWebPageAdapter.h \
    $$PWD/qt/WebCoreSupport/SearchPopupMenuQt.h \
    $$PWD/qt/WebCoreSupport/TextCheckerClientQt.h \
    $$PWD/qt/WebCoreSupport/TextureMapperLayerClientQt.h \
    $$PWD/qt/WebCoreSupport/UndoStepQt.h \
    $$PWD/qt/WebCoreSupport/WebEventConversion.h

INCLUDEPATH += \
    $$PWD/qt/WebCoreSupport

use?(3D_GRAPHICS): WEBKIT += angle

have?(qtpositioning):enable?(GEOLOCATION) {
     HEADERS += \
        $$PWD/qt/WebCoreSupport/GeolocationClientQt.h
     SOURCES += \
        $$PWD/qt/WebCoreSupport/GeolocationClientQt.cpp
}

enable?(ICONDATABASE) {
    HEADERS += \
        $$PWD/../WebCore/loader/icon/IconDatabaseClient.h \
        $$PWD/qt/WebCoreSupport/IconDatabaseClientQt.h

    SOURCES += \
        $$PWD/qt/WebCoreSupport/IconDatabaseClientQt.cpp
}

enable?(VIDEO) {
    use?(GSTREAMER) | use?(QT_MULTIMEDIA) {
        HEADERS += $$PWD/qt/WebCoreSupport/FullScreenVideoQt.h
        SOURCES += $$PWD/qt/WebCoreSupport/FullScreenVideoQt.cpp
    }
}



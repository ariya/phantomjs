# Qt kernel module

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = kernel/qt_gui_pch.h


KERNEL_P= kernel
HEADERS += \
	kernel/qaction.h \
    kernel/qaction_p.h \
	kernel/qactiongroup.h \
	kernel/qapplication.h \
	kernel/qapplication_p.h \
	kernel/qboxlayout.h \
	kernel/qclipboard.h \
	kernel/qcursor.h \
	kernel/qdesktopwidget.h \
	kernel/qdrag.h \
	kernel/qdnd_p.h \
	kernel/qevent.h \
	kernel/qevent_p.h \
	kernel/qformlayout.h \
	kernel/qgridlayout.h \
	kernel/qkeysequence.h \
	kernel/qlayout.h \
	kernel/qlayout_p.h \
	kernel/qlayoutengine_p.h \
	kernel/qlayoutitem.h \
	kernel/qmime.h \
	kernel/qsessionmanager.h \
	kernel/qshortcut.h \
	kernel/qshortcutmap_p.h \
	kernel/qsizepolicy.h \
	kernel/qpalette.h \
	kernel/qstackedlayout.h \
	kernel/qtooltip.h \
	kernel/qwhatsthis.h \
    kernel/qwidget.h \
    kernel/qwidget_p.h \
	kernel/qwidgetaction.h \
	kernel/qwidgetaction_p.h \
	kernel/qwindowdefs.h \
	kernel/qkeymapper_p.h \
	kernel/qgesture.h \
	kernel/qgesture_p.h \
	kernel/qstandardgestures_p.h \
	kernel/qgesturerecognizer.h \
	kernel/qgesturemanager_p.h \
	kernel/qsoftkeymanager_p.h \
    kernel/qsoftkeymanager_common_p.h \
	kernel/qguiplatformplugin_p.h \

SOURCES += \
	kernel/qaction.cpp \
	kernel/qactiongroup.cpp \
	kernel/qapplication.cpp \
	kernel/qboxlayout.cpp \
	kernel/qclipboard.cpp \
	kernel/qcursor.cpp \
	kernel/qdrag.cpp \
	kernel/qdnd.cpp \
	kernel/qevent.cpp \
	kernel/qformlayout.cpp \
	kernel/qgridlayout.cpp \
	kernel/qkeysequence.cpp \
	kernel/qlayout.cpp \
	kernel/qlayoutengine.cpp \
	kernel/qlayoutitem.cpp \
	kernel/qmime.cpp \
	kernel/qpalette.cpp \
	kernel/qshortcut.cpp \
	kernel/qshortcutmap.cpp \
	kernel/qstackedlayout.cpp \
	kernel/qtooltip.cpp \
	kernel/qguivariant.cpp \
	kernel/qwhatsthis.cpp \
	kernel/qwidget.cpp \
	kernel/qwidgetaction.cpp \
	kernel/qkeymapper.cpp \
	kernel/qgesture.cpp \
	kernel/qstandardgestures.cpp \
	kernel/qgesturerecognizer.cpp \
	kernel/qgesturemanager.cpp \
	kernel/qsoftkeymanager.cpp \
    kernel/qdesktopwidget.cpp \
	kernel/qguiplatformplugin.cpp

win32 {
	DEFINES += QT_NO_DIRECTDRAW

    HEADERS += \
        kernel/qwinnativepangesturerecognizer_win_p.h

	SOURCES += \
		kernel/qapplication_win.cpp \
		kernel/qclipboard_win.cpp \
		kernel/qcursor_win.cpp \
		kernel/qdesktopwidget_win.cpp \
		kernel/qdnd_win.cpp \
		kernel/qmime_win.cpp \
		kernel/qsound_win.cpp \
		kernel/qwidget_win.cpp \
		kernel/qole_win.cpp \
        kernel/qkeymapper_win.cpp \
        kernel/qwinnativepangesturerecognizer_win.cpp

    !contains(DEFINES, QT_NO_DIRECTDRAW):LIBS += ddraw.lib
}

symbian {
    exists($${EPOCROOT}epoc32/include/platform/mw/akntranseffect.h): DEFINES += QT_SYMBIAN_HAVE_AKNTRANSEFFECT_H

    SOURCES += \
        kernel/qapplication_s60.cpp \
        kernel/qeventdispatcher_s60.cpp \
        kernel/qwidget_s60.cpp \
        kernel/qcursor_s60.cpp \
        kernel/qdesktopwidget_s60.cpp \
        kernel/qkeymapper_s60.cpp\
        kernel/qclipboard_s60.cpp\
        kernel/qdnd_s60.cpp \
        kernel/qsound_s60.cpp

    HEADERS += \
        kernel/qt_s60_p.h \
        kernel/qeventdispatcher_s60_p.h

    LIBS += -lbafl -lestor

    INCLUDEPATH += $$MW_LAYER_SYSTEMINCLUDE
    INCLUDEPATH += ../3rdparty/s60

    contains(QT_CONFIG, s60) {
        SOURCES += kernel/qsoftkeymanager_s60.cpp
        HEADERS += kernel/qsoftkeymanager_s60_p.h
    }
}


unix:x11 {
	INCLUDEPATH += ../3rdparty/xorg
	HEADERS += \
		kernel/qx11embed_x11.h \
		kernel/qx11info_x11.h \
        kernel/qkde_p.h

	SOURCES += \
		kernel/qapplication_x11.cpp \
		kernel/qclipboard_x11.cpp \
		kernel/qcursor_x11.cpp \
		kernel/qdnd_x11.cpp \
		kernel/qdesktopwidget_x11.cpp \
		kernel/qmotifdnd_x11.cpp \
		kernel/qsound_x11.cpp \
		kernel/qwidget_x11.cpp \
		kernel/qwidgetcreate_x11.cpp \
		kernel/qx11embed_x11.cpp \
		kernel/qx11info_x11.cpp \
		kernel/qkeymapper_x11.cpp \
		kernel/qkde.cpp

        contains(QT_CONFIG, glib) {
            SOURCES += \
		kernel/qguieventdispatcher_glib.cpp
            HEADERS += \
                kernel/qguieventdispatcher_glib_p.h
            QMAKE_CXXFLAGS += $$QT_CFLAGS_GLIB
	    LIBS_PRIVATE +=$$QT_LIBS_GLIB
	}
            SOURCES += \
		kernel/qeventdispatcher_x11.cpp
            HEADERS += \
                kernel/qeventdispatcher_x11_p.h
}

embedded {
	HEADERS += \
		kernel/qeventdispatcher_qws_p.h

	SOURCES += \
		kernel/qapplication_qws.cpp \
		kernel/qclipboard_qws.cpp \
		kernel/qcursor_qws.cpp \
		kernel/qdesktopwidget_qws.cpp \
		kernel/qdnd_qws.cpp \
		kernel/qeventdispatcher_qws.cpp \
		kernel/qsound_qws.cpp \
		kernel/qwidget_qws.cpp \
		kernel/qkeymapper_qws.cpp \
		kernel/qsessionmanager_qws.cpp

        contains(QT_CONFIG, glib) {
            SOURCES += \
		kernel/qeventdispatcher_glib_qws.cpp
            HEADERS += \
                kernel/qeventdispatcher_glib_qws_p.h
            QMAKE_CXXFLAGS += $$QT_CFLAGS_GLIB
            LIBS_PRIVATE +=$$QT_LIBS_GLIB
	}
}

!qpa {
        HEADERS += \
                kernel/qsound.h \
                kernel/qsound_p.h

        SOURCES += \
                kernel/qsound.cpp
}

qpa {
	HEADERS += \
		kernel/qgenericpluginfactory_qpa.h \
                kernel/qgenericplugin_qpa.h \
                kernel/qeventdispatcher_qpa_p.h \
                kernel/qwindowsysteminterface_qpa.h \
                kernel/qwindowsysteminterface_qpa_p.h \
                kernel/qplatformintegration_qpa.h \
                kernel/qplatformscreen_qpa.h \
                kernel/qplatformintegrationfactory_qpa_p.h \
                kernel/qplatformintegrationplugin_qpa.h \
                kernel/qplatformwindow_qpa.h \
                kernel/qplatformwindowformat_qpa.h \
                kernel/qplatformglcontext_qpa.h \
                kernel/qdesktopwidget_qpa_p.h \
                kernel/qplatformeventloopintegration_qpa.h \
                kernel/qplatformcursor_qpa.h \
                kernel/qplatformclipboard_qpa.h \
                kernel/qplatformnativeinterface_qpa.h

	SOURCES += \
		kernel/qapplication_qpa.cpp \
		kernel/qclipboard_qpa.cpp \
		kernel/qcursor_qpa.cpp \
		kernel/qdnd_qws.cpp \
		kernel/qdesktopwidget_qpa.cpp \
		kernel/qgenericpluginfactory_qpa.cpp \
		kernel/qgenericplugin_qpa.cpp \
		kernel/qkeymapper_qws.cpp \
                kernel/qwidget_qpa.cpp \
                kernel/qeventdispatcher_qpa.cpp \
                kernel/qwindowsysteminterface_qpa.cpp \
                kernel/qplatformintegration_qpa.cpp \
                kernel/qplatformscreen_qpa.cpp \
                kernel/qplatformintegrationfactory_qpa.cpp \
                kernel/qplatformintegrationplugin_qpa.cpp \
                kernel/qplatformwindow_qpa.cpp \
                kernel/qplatformwindowformat_qpa.cpp \
                kernel/qplatformeventloopintegration_qpa.cpp \
                kernel/qplatformglcontext_qpa.cpp \
                kernel/qplatformcursor_qpa.cpp \
                kernel/qplatformclipboard_qpa.cpp \
                kernel/qplatformnativeinterface_qpa.cpp \
                kernel/qsessionmanager_qpa.cpp

        contains(QT_CONFIG, glib) {
            SOURCES += \
		kernel/qeventdispatcher_glib_qpa.cpp
            HEADERS += \
                kernel/qeventdispatcher_glib_qpa_p.h
            QMAKE_CXXFLAGS += $$QT_CFLAGS_GLIB
            LIBS_PRIVATE +=$$QT_LIBS_GLIB
	}

        blackberry {
            SOURCES += \
                kernel/qeventdispatcher_blackberry_qpa.cpp
            HEADERS += \
                kernel/qeventdispatcher_blackberry_qpa_p.h
        }
}

!embedded:!qpa:!x11:mac {
	SOURCES += \
		kernel/qclipboard_mac.cpp \
		kernel/qmime_mac.cpp \
		kernel/qt_mac.cpp \
		kernel/qkeymapper_mac.cpp

        OBJECTIVE_HEADERS += \
                qcocoawindow_mac_p.h \
                qcocoapanel_mac_p.h \
                qcocoawindowdelegate_mac_p.h \
                qcocoaview_mac_p.h \
                qcocoaapplication_mac_p.h \
                qcocoaapplicationdelegate_mac_p.h \
                qmacgesturerecognizer_mac_p.h \
                qmultitouch_mac_p.h \
                qcocoasharedwindowmethods_mac_p.h \
                qcocoaintrospection_p.h

        OBJECTIVE_SOURCES += \
                kernel/qcursor_mac.mm \
                kernel/qdnd_mac.mm \
                kernel/qsound_mac.mm  \
                kernel/qapplication_mac.mm \
		        kernel/qwidget_mac.mm \
		        kernel/qcocoapanel_mac.mm \
                kernel/qcocoaview_mac.mm \
                kernel/qcocoawindow_mac.mm \
                kernel/qcocoawindowdelegate_mac.mm \
                kernel/qcocoamenuloader_mac.mm \
                kernel/qcocoaapplication_mac.mm \
                kernel/qcocoaapplicationdelegate_mac.mm \
                kernel/qt_cocoa_helpers_mac.mm \
                kernel/qdesktopwidget_mac.mm \
                kernel/qeventdispatcher_mac.mm \
                kernel/qcocoawindowcustomthemeframe_mac.mm \
                kernel/qmacgesturerecognizer_mac.mm \
                kernel/qmultitouch_mac.mm \
                kernel/qcocoaintrospection_mac.mm

        HEADERS += \
                kernel/qt_cocoa_helpers_mac_p.h \
                kernel/qcocoaapplication_mac_p.h \
                kernel/qcocoaapplicationdelegate_mac_p.h \
                kernel/qeventdispatcher_mac_p.h

        MENU_NIB.files = mac/qt_menu.nib
        MENU_NIB.path = Resources
        MENU_NIB.version = Versions
        QMAKE_BUNDLE_DATA += MENU_NIB
        RESOURCES += mac/macresources.qrc

        LIBS_PRIVATE += -framework AppKit
}

wince*: {
        HEADERS += \
                ../corelib/kernel/qfunctions_wince.h \
                kernel/qguifunctions_wince.h

        SOURCES += \
                ../corelib/kernel/qfunctions_wince.cpp \
                kernel/qguifunctions_wince.cpp
}

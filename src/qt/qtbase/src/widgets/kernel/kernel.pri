# Qt kernel module

# Only used on platforms with CONFIG += precompile_header
PRECOMPILED_HEADER = kernel/qt_widgets_pch.h

KERNEL_P= kernel
HEADERS += \
	kernel/qaction.h \
    kernel/qaction_p.h \
	kernel/qactiongroup.h \
	kernel/qapplication.h \
	kernel/qapplication_p.h \
        kernel/qwidgetbackingstore_p.h \
	kernel/qboxlayout.h \
	kernel/qdesktopwidget.h \
	kernel/qformlayout.h \
	kernel/qgridlayout.h \
        kernel/qlayout.h \
	kernel/qlayout_p.h \
	kernel/qlayoutengine_p.h \
	kernel/qlayoutitem.h \
        kernel/qshortcut.h \
	kernel/qsizepolicy.h \
        kernel/qstackedlayout.h \
	kernel/qtooltip.h \
	kernel/qwhatsthis.h \
        kernel/qwidget.h \
        kernel/qwidget_p.h \
	kernel/qwidgetaction.h \
	kernel/qwidgetaction_p.h \
	kernel/qgesture.h \
	kernel/qgesture_p.h \
	kernel/qstandardgestures_p.h \
	kernel/qgesturerecognizer.h \
	kernel/qgesturemanager_p.h \
        kernel/qdesktopwidget_qpa_p.h \
        kernel/qwidgetwindow_qpa_p.h \
        kernel/qwindowcontainer_p.h

SOURCES += \
	kernel/qaction.cpp \
	kernel/qactiongroup.cpp \
	kernel/qapplication.cpp \
        kernel/qwidgetbackingstore.cpp \
        kernel/qboxlayout.cpp \
	kernel/qformlayout.cpp \
	kernel/qgridlayout.cpp \
        kernel/qlayout.cpp \
	kernel/qlayoutengine.cpp \
	kernel/qlayoutitem.cpp \
        kernel/qshortcut.cpp \
        kernel/qstackedlayout.cpp \
	kernel/qtooltip.cpp \
	kernel/qwhatsthis.cpp \
	kernel/qwidget.cpp \
	kernel/qwidgetaction.cpp \
	kernel/qgesture.cpp \
	kernel/qstandardgestures.cpp \
	kernel/qgesturerecognizer.cpp \
	kernel/qgesturemanager.cpp \
        kernel/qdesktopwidget.cpp \
        kernel/qwidgetsvariant.cpp \
        kernel/qapplication_qpa.cpp \
        kernel/qdesktopwidget_qpa.cpp \
        kernel/qwidget_qpa.cpp \
        kernel/qwidgetwindow.cpp \
        kernel/qwindowcontainer.cpp

macx: {
    HEADERS += kernel/qmacgesturerecognizer_p.h
    SOURCES += kernel/qmacgesturerecognizer.cpp
}

wince*: {
        HEADERS += \
                ../corelib/kernel/qfunctions_wince.h \
                kernel/qwidgetsfunctions_wince.h

        SOURCES += \
                kernel/qwidgetsfunctions_wince.cpp
}

contains(QT_CONFIG, opengl) {
    HEADERS += kernel/qopenglwidget_p.h
    SOURCES += kernel/qopenglwidget.cpp
}

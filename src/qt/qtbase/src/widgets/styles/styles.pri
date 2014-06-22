# Qt styles module

HEADERS += \
        styles/qdrawutil.h \
        styles/qstyle.h \
        styles/qstyleanimation_p.h \
        styles/qstylefactory.h \
        styles/qstyleoption.h \
        styles/qstyleplugin.h \
        styles/qcommonstylepixmaps_p.h \
        styles/qcommonstyle.h \
        styles/qstylehelper_p.h \
        styles/qproxystyle.h \
        styles/qproxystyle_p.h \
        styles/qcommonstyle_p.h \
        styles/qstylepainter.h \
        styles/qstylesheetstyle_p.h

SOURCES += \
        styles/qdrawutil.cpp \
        styles/qstyle.cpp \
        styles/qstyleanimation.cpp \
        styles/qstylefactory.cpp \
        styles/qstyleoption.cpp \
        styles/qstyleplugin.cpp \
        styles/qstylehelper.cpp \
        styles/qcommonstyle.cpp \
        styles/qproxystyle.cpp \
        styles/qstylepainter.cpp \
        styles/qstylesheetstyle.cpp \
        styles/qstylesheetstyle_default.cpp

wince* {
    RESOURCES += styles/qstyle_wince.qrc
} else {
    RESOURCES += styles/qstyle.qrc
}

contains( styles, all ) {
    styles = fusion mac windows windowsxp windowsvista
}

!macx:styles -= mac

contains(QT_CONFIG, gtkstyle) {
    QMAKE_CXXFLAGS += $$QT_CFLAGS_QGTKSTYLE
    LIBS_PRIVATE += $$QT_LIBS_QGTKSTYLE
    styles += gtk
    CONFIG += x11
}

contains( styles, mac ) {
    HEADERS += \
        styles/qmacstyle_mac_p.h \
        styles/qmacstyle_mac_p_p.h
        OBJECTIVE_SOURCES += styles/qmacstyle_mac.mm
} else {
    DEFINES += QT_NO_STYLE_MAC
}

contains( styles, windowsvista ) {
    HEADERS += styles/qwindowsvistastyle_p.h
    HEADERS += styles/qwindowsvistastyle_p_p.h
    SOURCES += styles/qwindowsvistastyle.cpp
    !contains( styles, windowsxp ) {
        message( windowsvista requires windowsxp )
        styles += windowsxp
    }
} else {
    DEFINES += QT_NO_STYLE_WINDOWSVISTA
}

contains( styles, windowsxp ) {
    HEADERS += styles/qwindowsxpstyle_p.h
    HEADERS += styles/qwindowsxpstyle_p_p.h
    SOURCES += styles/qwindowsxpstyle.cpp
    !contains( styles, windows ) {
        message( windowsxp requires windows )
        styles  += windows
    }
} else {
    DEFINES += QT_NO_STYLE_WINDOWSXP
}

contains( styles, windows ) {
    HEADERS += styles/qwindowsstyle_p.h
    HEADERS += styles/qwindowsstyle_p_p.h
    SOURCES += styles/qwindowsstyle.cpp
} else {
    DEFINES += QT_NO_STYLE_WINDOWS
}

contains( styles, gtk ) {
        HEADERS += styles/qgtkglobal_p.h
        HEADERS += styles/qgtkstyle_p.h
        HEADERS += styles/qgtkpainter_p.h
        HEADERS += styles/qgtk2painter_p.h
        HEADERS += styles/qgtkstyle_p_p.h
        SOURCES += styles/qgtkstyle.cpp
        SOURCES += styles/qgtkpainter.cpp
        SOURCES += styles/qgtk2painter.cpp
        SOURCES += styles/qgtkstyle_p.cpp
} else {
    DEFINES += QT_NO_STYLE_GTK
}
contains( styles, fusion ) {
        HEADERS += styles/qfusionstyle_p.h
        HEADERS += styles/qfusionstyle_p_p.h
        SOURCES += styles/qfusionstyle.cpp
} else {
    DEFINES += QT_NO_STYLE_FUSION
}

contains( styles, windowsce ) {
    HEADERS += styles/qwindowscestyle_p.h
    HEADERS += styles/qwindowscestyle_p_p.h
    SOURCES += styles/qwindowscestyle.cpp
} else {
    DEFINES += QT_NO_STYLE_WINDOWSCE
}

contains( styles, windowsmobile ) {
    HEADERS += styles/qwindowsmobilestyle_p.h
    HEADERS += styles/qwindowsmobilestyle_p_p.h
    SOURCES += styles/qwindowsmobilestyle.cpp
} else {
    DEFINES += QT_NO_STYLE_WINDOWSMOBILE
}

contains( styles, android ) {
    HEADERS += styles/qandroidstyle_p.h
    SOURCES += styles/qandroidstyle.cpp
} else {
    DEFINES += QT_NO_STYLE_ANDROID
}

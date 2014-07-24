# Qt util module

HEADERS += \
        util/qsystemtrayicon.h \
        util/qcolormap.h \
        util/qcompleter.h \
        util/qcompleter_p.h \
        util/qsystemtrayicon_p.h \
        util/qscroller.h \
        util/qscroller_p.h \
        util/qscrollerproperties.h \
        util/qscrollerproperties_p.h \
        util/qflickgesture_p.h \
        util/qundogroup.h \
        util/qundostack.h \
        util/qundostack_p.h \
        util/qundoview.h

SOURCES += \
        util/qsystemtrayicon.cpp \
        util/qcolormap.cpp \
        util/qcompleter.cpp \
        util/qscroller.cpp \
        util/qscrollerproperties.cpp \
        util/qflickgesture.cpp \
        util/qundogroup.cpp \
        util/qundostack.cpp \
        util/qundoview.cpp

win32:!wince*:!winrt {
    SOURCES += util/qsystemtrayicon_win.cpp
} else:contains(QT_CONFIG, xcb) {
    SOURCES += util/qsystemtrayicon_x11.cpp
} else {
    SOURCES += util/qsystemtrayicon_qpa.cpp
}

mac {
    OBJECTIVE_SOURCES += util/qscroller_mac.mm
}

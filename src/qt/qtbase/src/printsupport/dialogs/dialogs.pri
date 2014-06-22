# Qt dialogs module

HEADERS += \
        dialogs/qabstractprintdialog.h \
        dialogs/qabstractprintdialog_p.h \
        dialogs/qpagesetupdialog_p.h \
        dialogs/qpagesetupdialog.h \
        dialogs/qprintdialog.h \
        dialogs/qprintpreviewdialog.h

mac:!ios {
    OBJECTIVE_SOURCES += dialogs/qpagesetupdialog_mac.mm \
                         dialogs/qprintdialog_mac.mm
    LIBS_PRIVATE += -framework Cocoa
}

win32 {
    SOURCES += dialogs/qpagesetupdialog_win.cpp \
               dialogs/qprintdialog_win.cpp
}

unix:!mac {
    INCLUDEPATH += $$QT_SOURCE_TREE/src/plugins/printsupport/cups
    HEADERS += dialogs/qpagesetupdialog_unix_p.h
    SOURCES += dialogs/qprintdialog_unix.cpp \
               dialogs/qpagesetupdialog_unix.cpp
    FORMS += dialogs/qprintsettingsoutput.ui \
    dialogs/qprintwidget.ui \
    dialogs/qprintpropertieswidget.ui
}

INCLUDEPATH += $$PWD

SOURCES += \
        dialogs/qabstractprintdialog.cpp \
        dialogs/qpagesetupdialog.cpp \
        dialogs/qprintpreviewdialog.cpp

FORMS += dialogs/qpagesetupwidget.ui
RESOURCES += dialogs/qprintdialog.qrc

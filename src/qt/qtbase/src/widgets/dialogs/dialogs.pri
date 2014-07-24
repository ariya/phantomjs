# Qt dialogs module

HEADERS += \
	dialogs/qcolordialog.h \
	dialogs/qcolordialog_p.h \
        dialogs/qfscompleter_p.h \
	dialogs/qdialog.h \
	dialogs/qdialog_p.h \
	dialogs/qerrormessage.h \
	dialogs/qfiledialog.h \
	dialogs/qfiledialog_p.h \
	dialogs/qfontdialog.h \
	dialogs/qfontdialog_p.h \
	dialogs/qinputdialog.h \
	dialogs/qmessagebox.h \
	dialogs/qprogressdialog.h \
        dialogs/qsidebar_p.h \
        dialogs/qfilesystemmodel.h \
        dialogs/qfilesystemmodel_p.h \
        dialogs/qfileinfogatherer_p.h \
        dialogs/qwizard.h

win32 {
    HEADERS += dialogs/qwizard_win_p.h
    SOURCES += dialogs/qwizard_win.cpp
}

wince*: FORMS += dialogs/qfiledialog_embedded.ui
else: FORMS += dialogs/qfiledialog.ui

INCLUDEPATH += $$PWD
SOURCES += \
	dialogs/qcolordialog.cpp \
	dialogs/qdialog.cpp \
	dialogs/qerrormessage.cpp \
	dialogs/qfiledialog.cpp \
	dialogs/qfontdialog.cpp \
	dialogs/qinputdialog.cpp \
	dialogs/qmessagebox.cpp \
	dialogs/qprogressdialog.cpp \
        dialogs/qsidebar.cpp \
        dialogs/qfilesystemmodel.cpp \
        dialogs/qfileinfogatherer.cpp \
	dialogs/qwizard.cpp \

RESOURCES += dialogs/qmessagebox.qrc

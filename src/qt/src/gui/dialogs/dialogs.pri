# Qt dialogs module

HEADERS += \
	dialogs/qabstractprintdialog.h \
	dialogs/qabstractprintdialog_p.h \
	dialogs/qabstractpagesetupdialog.h \
	dialogs/qabstractpagesetupdialog_p.h \
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
	dialogs/qpagesetupdialog.h \
	dialogs/qprintdialog.h \
	dialogs/qprogressdialog.h \
        dialogs/qsidebar_p.h \
        dialogs/qfilesystemmodel.h \
        dialogs/qfilesystemmodel_p.h \
        dialogs/qfileinfogatherer_p.h \
	dialogs/qwizard.h \
        dialogs/qprintpreviewdialog.h

!embedded:!qpa:mac {
    OBJECTIVE_SOURCES += dialogs/qfiledialog_mac.mm \
                         dialogs/qfontdialog_mac.mm \
                         dialogs/qnspanelproxy_mac.mm \
                         dialogs/qpagesetupdialog_mac.mm \
                         dialogs/qprintdialog_mac.mm

# Compile qcolordialog_mac.mm with exception support, disregarding the -no-exceptions 
# configure option. (qcolordialog_mac needs to catch exceptions thrown by cocoa)
    EXCEPTION_SOURCES = dialogs/qcolordialog_mac.mm
    exceptions_compiler.commands = $$QMAKE_CXX -c
    exceptions_compiler.commands += $(CXXFLAGS) $(INCPATH) ${QMAKE_FILE_IN} -o ${QMAKE_FILE_OUT}
    exceptions_compiler.commands += -fexceptions
    exceptions_compiler.dependency_type = TYPE_C
    exceptions_compiler.output = ${QMAKE_VAR_OBJECTS_DIR}${QMAKE_FILE_BASE}$${first(QMAKE_EXT_OBJ)}
    exceptions_compiler.input = EXCEPTION_SOURCES
    exceptions_compiler.variable_out = OBJECTS
    exceptions_compiler.name = compiling[exceptopns] ${QMAKE_FILE_IN}
    silent:exceptions_compiler.commands = @echo compiling[exceptopns] ${QMAKE_FILE_IN} && $$exceptions_compiler.commands
    QMAKE_EXTRA_COMPILERS += exceptions_compiler
}

win32 {
    HEADERS += dialogs/qwizard_win_p.h \
               dialogs/qfiledialog_win_p.h
    SOURCES += dialogs/qdialogsbinarycompat_win.cpp \
               dialogs/qfiledialog_win.cpp \
               dialogs/qpagesetupdialog_win.cpp \
               dialogs/qprintdialog_win.cpp \
               dialogs/qwizard_win.cpp

    !win32-borland:!wince*: LIBS += -lshell32 	# the filedialog needs this library
}

!mac:!embedded:!symbian:unix|qpa {
        HEADERS += dialogs/qpagesetupdialog_unix_p.h
	SOURCES += dialogs/qprintdialog_unix.cpp \
		   dialogs/qpagesetupdialog_unix.cpp
	FORMS += dialogs/qprintsettingsoutput.ui \
        dialogs/qprintwidget.ui \
        dialogs/qprintpropertieswidget.ui
}

embedded {
        contains(QT_CONFIG,qtopia) {
            HEADERS += dialogs/qpagesetupdialog_unix_p.h
            DEFINES += QTOPIA_PRINTDIALOG
            SOURCES += dialogs/qprintdialog_qws.cpp \
                       dialogs/qpagesetupdialog_unix.cpp
        } else {
            HEADERS += dialogs/qpagesetupdialog_unix_p.h
            SOURCES += dialogs/qprintdialog_unix.cpp \
                       dialogs/qpagesetupdialog_unix.cpp
            FORMS += dialogs/qprintsettingsoutput.ui \
                     dialogs/qprintwidget.ui \
                     dialogs/qprintpropertieswidget.ui
        }
}

wince*|symbian: FORMS += dialogs/qfiledialog_embedded.ui
else: FORMS += dialogs/qfiledialog.ui

INCLUDEPATH += $$PWD
SOURCES += \
	dialogs/qabstractprintdialog.cpp \
	dialogs/qabstractpagesetupdialog.cpp \
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
        dialogs/qpagesetupdialog.cpp \
	dialogs/qwizard.cpp \
        dialogs/qprintpreviewdialog.cpp

symbian:contains(QT_CONFIG, s60) {
    LIBS += -lcommondialogs
    SOURCES += dialogs/qfiledialog_symbian.cpp \
               dialogs/qcolordialog_symbian.cpp
}

FORMS += dialogs/qpagesetupwidget.ui
RESOURCES += dialogs/qprintdialog.qrc
RESOURCES += dialogs/qmessagebox.qrc

# Compensate for lack of platform defines in Symbian3
symbian: DEFINES += SYMBIAN_VERSION_$$upper($$replace(SYMBIAN_VERSION,\\.,_))

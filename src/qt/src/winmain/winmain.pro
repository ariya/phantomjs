# Additional Qt project file for qtmain lib on Windows
TEMPLATE = lib
TARGET	 = qtmain
DESTDIR	 = $$QMAKE_LIBDIR_QT
QT       =

CONFIG	+= staticlib warn_on
CONFIG	-= qt shared

win32 {
	win32-g++*:DEFINES += QT_NEEDS_QMAIN
	win32-borland:DEFINES += QT_NEEDS_QMAIN
	SOURCES		= qtmain_win.cpp
	CONFIG		+= png
	INCLUDEPATH	+= tmp $$QMAKE_INCDIR_QT/QtCore
}

!win32:error("$$_FILE_ is intended only for Windows!")
include(../qbase.pri)
wince*:QMAKE_POST_LINK =


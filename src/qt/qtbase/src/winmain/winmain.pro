# Additional Qt project file for qtmain lib on Windows
!win32:error("$$_FILE_ is intended only for Windows!")

TEMPLATE = lib
TARGET = qtmain
DESTDIR = $$QT.core.libs

CONFIG += static
QT = core

contains(QT_CONFIG, build_all):CONFIG += build_all

win32-msvc*:QMAKE_CFLAGS_DEBUG -= -Zi
win32-msvc*:QMAKE_CXXFLAGS_DEBUG -= -Zi
win32-msvc*:QMAKE_CFLAGS_DEBUG *= -Z7
win32-msvc*:QMAKE_CXXFLAGS_DEBUG *= -Z7
mingw: DEFINES += QT_NEEDS_QMAIN

winrt {
    SOURCES = qtmain_winrt.cpp
} else {
    SOURCES = qtmain_win.cpp
}

load(qt_installs)

TARGET = $$qtLibraryTarget($$TARGET$$QT_LIBINFIX) #do this towards the end

load(qt_targets)

wince*:QMAKE_POST_LINK =

lib_replace.match = $$[QT_INSTALL_LIBS/get]
lib_replace.replace = $$[QT_INSTALL_LIBS/raw]
lib_replace.CONFIG = path
QMAKE_PRL_INSTALL_REPLACE += lib_replace

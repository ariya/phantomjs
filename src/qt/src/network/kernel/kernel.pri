# Qt network kernel module

PRECOMPILED_HEADER = ../corelib/global/qt_pch.h
INCLUDEPATH += $$PWD

HEADERS += kernel/qauthenticator.h \
	   kernel/qauthenticator_p.h \
           kernel/qhostaddress.h \
           kernel/qhostinfo.h \
           kernel/qhostinfo_p.h \
           kernel/qurlinfo.h \
           kernel/qnetworkproxy.h \
	   kernel/qnetworkinterface.h \
	   kernel/qnetworkinterface_p.h

SOURCES += kernel/qauthenticator.cpp \
           kernel/qhostaddress.cpp \
           kernel/qhostinfo.cpp \
           kernel/qurlinfo.cpp \
           kernel/qnetworkproxy.cpp \
	   kernel/qnetworkinterface.cpp

symbian: SOURCES += kernel/qhostinfo_symbian.cpp kernel/qnetworkinterface_symbian.cpp
unix:!symbian:SOURCES += kernel/qhostinfo_unix.cpp kernel/qnetworkinterface_unix.cpp
win32:SOURCES += kernel/qhostinfo_win.cpp kernel/qnetworkinterface_win.cpp
integrity:SOURCES += kernel/qhostinfo_unix.cpp kernel/qnetworkinterface_unix.cpp

mac:LIBS_PRIVATE += -framework SystemConfiguration -framework CoreFoundation -framework CoreServices
qpa:mac:!ios:SOURCES += kernel/qnetworkproxy_mac.cpp
else:!qpa:mac:SOURCES += kernel/qnetworkproxy_mac.cpp
else:win32:SOURCES += kernel/qnetworkproxy_win.cpp
else:symbian:SOURCES += kernel/qnetworkproxy_symbian.cpp
else:blackberry:SOURCES += kernel/qnetworkproxy_blackberry.cpp
else:SOURCES += kernel/qnetworkproxy_generic.cpp

symbian: LIBS += -lcommsdat
blackberry: LIBS_PRIVATE += -lbps

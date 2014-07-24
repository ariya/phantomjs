# Qt network kernel module

PRECOMPILED_HEADER = ../corelib/global/qt_pch.h
INCLUDEPATH += $$PWD

HEADERS += kernel/qauthenticator.h \
	   kernel/qauthenticator_p.h \
           kernel/qdnslookup.h \
           kernel/qdnslookup_p.h \
           kernel/qhostaddress.h \
           kernel/qhostaddress_p.h \
           kernel/qhostinfo.h \
           kernel/qhostinfo_p.h \
           kernel/qurlinfo_p.h \
           kernel/qnetworkproxy.h \
           kernel/qnetworkproxy_p.h \
	   kernel/qnetworkinterface.h \
	   kernel/qnetworkinterface_p.h

SOURCES += kernel/qauthenticator.cpp \
           kernel/qdnslookup.cpp \
           kernel/qhostaddress.cpp \
           kernel/qhostinfo.cpp \
           kernel/qurlinfo.cpp \
           kernel/qnetworkproxy.cpp \
	   kernel/qnetworkinterface.cpp

unix:SOURCES += kernel/qdnslookup_unix.cpp kernel/qhostinfo_unix.cpp kernel/qnetworkinterface_unix.cpp

android {
    SOURCES -= kernel/qdnslookup_unix.cpp
    SOURCES += kernel/qdnslookup_android.cpp
}

win32: {
    !winrt {
        HEADERS += kernel/qnetworkinterface_win_p.h
        SOURCES += kernel/qdnslookup_win.cpp \
                   kernel/qhostinfo_win.cpp \
                   kernel/qnetworkinterface_win.cpp
        LIBS_PRIVATE += -ldnsapi
    } else {
        SOURCES += kernel/qdnslookup_winrt.cpp \
                   kernel/qhostinfo_winrt.cpp \
                   kernel/qnetworkinterface_winrt.cpp
    }
}
integrity:SOURCES += kernel/qdnslookup_unix.cpp kernel/qhostinfo_unix.cpp kernel/qnetworkinterface_unix.cpp

mac {
    LIBS_PRIVATE += -framework SystemConfiguration -framework CoreFoundation
    !ios: LIBS_PRIVATE += -framework CoreServices
}

mac:!ios:SOURCES += kernel/qnetworkproxy_mac.cpp
else:win32:SOURCES += kernel/qnetworkproxy_win.cpp
else:blackberry:SOURCES += kernel/qnetworkproxy_blackberry.cpp
else:SOURCES += kernel/qnetworkproxy_generic.cpp

blackberry: LIBS_PRIVATE += -lbps

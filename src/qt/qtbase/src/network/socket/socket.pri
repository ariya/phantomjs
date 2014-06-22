# Qt network socket

HEADERS += socket/qabstractsocketengine_p.h \
           socket/qhttpsocketengine_p.h \
           socket/qsocks5socketengine_p.h \
           socket/qabstractsocket.h \
           socket/qabstractsocket_p.h \
           socket/qtcpsocket.h \
           socket/qudpsocket.h \
           socket/qtcpserver.h \
           socket/qtcpsocket_p.h \
           socket/qlocalserver.h \
           socket/qlocalserver_p.h \
           socket/qlocalsocket.h \
           socket/qlocalsocket_p.h

SOURCES += socket/qabstractsocketengine.cpp \
           socket/qhttpsocketengine.cpp \
           socket/qsocks5socketengine.cpp \
           socket/qabstractsocket.cpp \
           socket/qtcpsocket.cpp \
           socket/qudpsocket.cpp \
           socket/qtcpserver.cpp \
           socket/qlocalsocket.cpp \
           socket/qlocalserver.cpp

!winrt {
    SOURCES += socket/qnativesocketengine.cpp
    HEADERS += socket/qnativesocketengine_p.h
}

unix: {
    SOURCES += socket/qnativesocketengine_unix.cpp \
                socket/qlocalsocket_unix.cpp \
                socket/qlocalserver_unix.cpp
}

unix:HEADERS += \
                socket/qnet_unix_p.h

win32:!winrt:SOURCES += socket/qnativesocketengine_win.cpp \
                socket/qlocalsocket_win.cpp \
                socket/qlocalserver_win.cpp

win32:!wince*:!winrt:LIBS_PRIVATE += -ladvapi32

winrt {
    SOURCES += socket/qnativesocketengine_winrt.cpp \
               socket/qlocalsocket_tcp.cpp \
               socket/qlocalserver_tcp.cpp
    HEADERS += socket/qnativesocketengine_winrt_p.h

    DEFINES += QT_LOCALSOCKET_TCP
}

wince*: {
    SOURCES -= socket/qlocalsocket_win.cpp \
               socket/qlocalserver_win.cpp
    SOURCES += socket/qlocalsocket_tcp.cpp \
               socket/qlocalserver_tcp.cpp

    DEFINES += QT_LOCALSOCKET_TCP
}

integrity: {
    SOURCES -= socket/qlocalsocket_unix.cpp \
               socket/qlocalserver_unix.cpp
    SOURCES += socket/qlocalsocket_tcp.cpp \
               socket/qlocalserver_tcp.cpp \
	       socket/qnativesocketengine_unix.cpp

    DEFINES += QT_LOCALSOCKET_TCP
}

contains(QT_CONFIG, system-proxies) {
    DEFINES += QT_USE_SYSTEM_PROXIES
}

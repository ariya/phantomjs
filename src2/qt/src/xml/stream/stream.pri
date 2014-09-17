# compatibility header:
HEADERS += stream/qxmlstream.h

!static {
    # The platforms that require the symbol to be present in QtXml:
    win32:!wince-*:SOURCES += ../corelib/xml/qxmlstream.cpp
    mac:SOURCES += ../corelib/xml/qxmlstream.cpp
    aix-*:SOURCES += ../corelib/xml/qxmlstream.cpp
}

include(../tests.pri)
TARGET = MIMESniffing
CONFIG += console

!contains(CONFIG, gprof) {
    SOURCES += ../../../../WebCore/platform/network/MIMESniffing.cpp
}

HEADERS += \
    ../../../../WebCore/platform/network/MIMESniffing.h \
    TestData.h

INCLUDEPATH += \
    ../../../../WebCore/platform/network \
    ../../../../JavaScriptCore \
    ../../../../JavaScriptCore/runtime \
    ../../../../WTF

RESOURCES += resources.qrc

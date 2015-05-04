TARGET = QtTest
QT = core-private
CONFIG += exceptions

MODULE_CONFIG = console testlib_defines

unix:!embedded:QMAKE_PKGCONFIG_DESCRIPTION = Qt \
    Unit \
    Testing \
    Library

QMAKE_DOCS = $$PWD/doc/qttestlib.qdocconf

HEADERS = qbenchmark.h \
    qbenchmark_p.h \
    qbenchmarkmeasurement_p.h \
    qbenchmarkvalgrind_p.h \
    qbenchmarkevent_p.h \
    qbenchmarkperfevents_p.h \
    qbenchmarkmetric_p.h \
    qsignalspy.h \
    qtestaccessible.h \
    qtestassert.h \
    qtestcase.h \
    qtestdata.h \
    qtestevent.h \
    qtesteventloop.h \
    qtest_global.h \
    qtest_gui.h \
    qtest_widgets.h \
    qtest.h \
    qtestkeyboard.h \
    qtestmouse.h \
    qtestspontaneevent.h \
    qtestsystem.h \
    qtesttouch.h \
    qtestblacklist_p.h

SOURCES = qtestcase.cpp \
    qtestlog.cpp \
    qtesttable.cpp \
    qtestdata.cpp \
    qtestresult.cpp \
    qasciikey.cpp \
    qplaintestlogger.cpp \
    qxmltestlogger.cpp \
    qsignaldumper.cpp \
    qabstracttestlogger.cpp \
    qbenchmark.cpp \
    qbenchmarkmeasurement.cpp \
    qbenchmarkvalgrind.cpp \
    qbenchmarkevent.cpp \
    qbenchmarkperfevents.cpp \
    qbenchmarkmetric.cpp \
    qcsvbenchmarklogger.cpp \
    qtestelement.cpp \
    qtestelementattribute.cpp \
    qtestxunitstreamer.cpp \
    qxunittestlogger.cpp \
    qtestblacklist.cpp

DEFINES *= QT_NO_CAST_TO_ASCII \
    QT_NO_CAST_FROM_ASCII \
    QT_NO_DATASTREAM
embedded:QMAKE_CXXFLAGS += -fno-rtti
wince*::LIBS += libcmt.lib \
    corelibc.lib \
    ole32.lib \
    oleaut32.lib \
    uuid.lib \
    commctrl.lib \
    coredll.lib \
    winsock.lib

mac {
    LIBS += -framework Security
    osx: LIBS += -framework ApplicationServices -framework IOKit
}

load(qt_module)

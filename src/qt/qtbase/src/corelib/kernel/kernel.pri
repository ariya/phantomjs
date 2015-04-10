# Qt core object module

HEADERS += \
        kernel/qabstracteventdispatcher.h \
        kernel/qabstractnativeeventfilter.h \
        kernel/qbasictimer.h \
        kernel/qeventloop.h\
        kernel/qpointer.h \
        kernel/qcorecmdlineargs_p.h \
        kernel/qcoreapplication.h \
        kernel/qcoreevent.h \
        kernel/qmetaobject.h \
        kernel/qmetatype.h \
        kernel/qmimedata.h \
        kernel/qobject.h \
        kernel/qobject_impl.h \
        kernel/qobjectdefs.h \
        kernel/qobjectdefs_impl.h \
        kernel/qsignalmapper.h \
        kernel/qsocketnotifier.h \
        kernel/qtimer.h \
        kernel/qtranslator.h \
        kernel/qtranslator_p.h \
        kernel/qvariant.h \
        kernel/qabstracteventdispatcher_p.h \
        kernel/qcoreapplication_p.h \
        kernel/qobjectcleanuphandler.h \
        kernel/qvariant_p.h \
        kernel/qmetaobject_p.h \
        kernel/qmetaobject_moc_p.h \
        kernel/qmetaobjectbuilder_p.h \
        kernel/qobject_p.h \
        kernel/qcoreglobaldata_p.h \
        kernel/qsharedmemory.h \
        kernel/qsharedmemory_p.h \
        kernel/qsystemsemaphore.h \
        kernel/qsystemsemaphore_p.h \
        kernel/qfunctions_p.h \
        kernel/qmath.h \
        kernel/qsystemerror_p.h \
        kernel/qmetatype_p.h \
        kernel/qmetatypeswitcher_p.h \

SOURCES += \
        kernel/qabstracteventdispatcher.cpp \
        kernel/qabstractnativeeventfilter.cpp \
        kernel/qbasictimer.cpp \
        kernel/qeventloop.cpp \
        kernel/qcoreapplication.cpp \
        kernel/qcoreevent.cpp \
        kernel/qmetaobject.cpp \
        kernel/qmetatype.cpp \
        kernel/qmetaobjectbuilder.cpp \
        kernel/qmimedata.cpp \
        kernel/qobject.cpp \
        kernel/qobjectcleanuphandler.cpp \
        kernel/qsignalmapper.cpp \
        kernel/qsocketnotifier.cpp \
        kernel/qtimer.cpp \
        kernel/qtranslator.cpp \
        kernel/qvariant.cpp \
        kernel/qcoreglobaldata.cpp \
        kernel/qsharedmemory.cpp \
        kernel/qsystemsemaphore.cpp \
        kernel/qpointer.cpp \
        kernel/qmath.cpp \
        kernel/qsystemerror.cpp

win32 {
        SOURCES += \
                kernel/qcoreapplication_win.cpp \
                kernel/qwineventnotifier.cpp \
                kernel/qsharedmemory_win.cpp \
                kernel/qsystemsemaphore_win.cpp
        HEADERS += \
                kernel/qwineventnotifier.h

        winrt {
            SOURCES += kernel/qeventdispatcher_winrt.cpp
            HEADERS += kernel/qeventdispatcher_winrt_p.h
        } else {
            SOURCES += kernel/qeventdispatcher_win.cpp
            HEADERS += kernel/qeventdispatcher_win_p.h
        }
}

wince*: {
        SOURCES += \
                kernel/qfunctions_wince.cpp
        HEADERS += \
                kernel/qfunctions_wince.h
}

winrt {
        SOURCES += \
                kernel/qfunctions_winrt.cpp
        HEADERS += \
                kernel/qfunctions_winrt.h
}

mac {
    SOURCES += \
        kernel/qcoreapplication_mac.cpp
}

mac:!nacl {
       HEADERS += \
                kernel/qcore_mac_p.h
       SOURCES += \
                kernel/qcore_mac.cpp
       OBJECTIVE_SOURCES += \
                kernel/qcore_mac_objc.mm

       # We need UIKit for UIDevice
       ios: LIBS_PRIVATE += -framework UIKit
}

nacl {
    SOURCES += \
        kernel/qfunctions_nacl.cpp
    HEADERS += \
        kernel/qfunctions_nacl.h
}

unix|integrity {
    SOURCES += \
            kernel/qcore_unix.cpp \
            kernel/qcrashhandler.cpp \
            kernel/qeventdispatcher_unix.cpp \
            kernel/qtimerinfo_unix.cpp

    HEADERS += \
            kernel/qcore_unix_p.h \
            kernel/qcrashhandler_p.h \
            kernel/qeventdispatcher_unix_p.h \
            kernel/qtimerinfo_unix_p.h

    contains(QT_CONFIG, glib) {
        SOURCES += \
            kernel/qeventdispatcher_glib.cpp
        HEADERS += \
            kernel/qeventdispatcher_glib_p.h
        QMAKE_CXXFLAGS += $$QT_CFLAGS_GLIB
        LIBS_PRIVATE +=$$QT_LIBS_GLIB
    }

   contains(QT_CONFIG, clock-gettime):include($$QT_SOURCE_TREE/config.tests/unix/clock-gettime/clock-gettime.pri)

    !android {
        SOURCES += kernel/qsharedmemory_unix.cpp \
                   kernel/qsystemsemaphore_unix.cpp
    } else {
        SOURCES += kernel/qsharedmemory_android.cpp \
                   kernel/qsystemsemaphore_android.cpp
    }
}

vxworks {
        SOURCES += \
                kernel/qfunctions_vxworks.cpp
        HEADERS += \
                kernel/qfunctions_vxworks.h
}

blackberry {
        SOURCES += \
                kernel/qeventdispatcher_blackberry.cpp \
                kernel/qppsattribute.cpp \
                kernel/qppsobject.cpp
        HEADERS += \
                kernel/qeventdispatcher_blackberry_p.h \
                kernel/qppsattribute_p.h \
                kernel/qppsattributeprivate_p.h \
                kernel/qppsobject_p.h \
                kernel/qppsobjectprivate_p.h
}

android:!android-no-sdk {
        SOURCES += \
                   kernel/qjnionload.cpp \
                   kernel/qjnihelpers.cpp \
                   kernel/qjni.cpp
        HEADERS += \
                   kernel/qjnihelpers_p.h \
                   kernel/qjni_p.h
}

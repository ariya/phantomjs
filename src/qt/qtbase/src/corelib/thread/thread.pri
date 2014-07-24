# Qt core thread module

# public headers
HEADERS += thread/qmutex.h \
           thread/qrunnable.h \
           thread/qreadwritelock.h \
           thread/qsemaphore.h \
           thread/qthread.h \
           thread/qthreadpool.h \
           thread/qthreadstorage.h \
           thread/qwaitcondition.h \
           thread/qatomic.h \
           thread/qexception.h \
           thread/qresultstore.h \
           thread/qfuture.h \
           thread/qfutureinterface.h \
           thread/qfuturesynchronizer.h \
           thread/qfuturewatcher.h \
           thread/qbasicatomic.h \
           thread/qgenericatomic.h

# private headers
HEADERS += thread/qmutex_p.h \
           thread/qmutexpool_p.h \
           thread/qfutureinterface_p.h \
           thread/qfuturewatcher_p.h \
           thread/qorderedmutexlocker_p.h \
           thread/qreadwritelock_p.h \
           thread/qthread_p.h \
           thread/qthreadpool_p.h

SOURCES += thread/qatomic.cpp \
           thread/qexception.cpp \
           thread/qresultstore.cpp \
           thread/qfutureinterface.cpp \
           thread/qfuturewatcher.cpp \
           thread/qmutex.cpp \
           thread/qreadwritelock.cpp \
           thread/qrunnable.cpp \
           thread/qmutexpool.cpp \
           thread/qsemaphore.cpp \
           thread/qthread.cpp \
           thread/qthreadpool.cpp \
           thread/qthreadstorage.cpp

unix:SOURCES += thread/qthread_unix.cpp \
                         thread/qwaitcondition_unix.cpp

win32:SOURCES += thread/qmutex_win.cpp \
                 thread/qthread_win.cpp \
		 thread/qwaitcondition_win.cpp

integrity:SOURCES += thread/qmutex_unix.cpp \
                thread/qthread_unix.cpp \
		thread/qwaitcondition_unix.cpp

unix: {
    mac {
        SOURCES += thread/qmutex_mac.cpp
    } else:linux-*:!linux-lsb-* {
        SOURCES += thread/qmutex_linux.cpp
    } else {
        SOURCES += thread/qmutex_unix.cpp
    }
}


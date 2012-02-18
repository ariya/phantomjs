SOURCES += \
        concurrent/qfuture.cpp \
        concurrent/qfutureinterface.cpp \
        concurrent/qfuturesynchronizer.cpp \
        concurrent/qfuturewatcher.cpp \
        concurrent/qrunnable.cpp \
        concurrent/qtconcurrentfilter.cpp \
        concurrent/qtconcurrentmap.cpp \
        concurrent/qtconcurrentresultstore.cpp \
        concurrent/qtconcurrentthreadengine.cpp \
        concurrent/qtconcurrentiteratekernel.cpp \
        concurrent/qtconcurrentexception.cpp \
        concurrent/qthreadpool.cpp

HEADERS += \
        concurrent/qfuture.h \
        concurrent/qfutureinterface.h \
        concurrent/qfuturesynchronizer.h \
        concurrent/qfuturewatcher.h \
        concurrent/qrunnable.h \
        concurrent/qtconcurrentcompilertest.h \
        concurrent/qtconcurrentexception.h \
        concurrent/qtconcurrentfilter.h \
        concurrent/qtconcurrentfilterkernel.h \
        concurrent/qtconcurrentfunctionwrappers.h \
        concurrent/qtconcurrentiteratekernel.h \
        concurrent/qtconcurrentmap.h \
        concurrent/qtconcurrentmapkernel.h \
        concurrent/qtconcurrentmedian.h \
        concurrent/qtconcurrentreducekernel.h \
        concurrent/qtconcurrentresultstore.h \
        concurrent/qtconcurrentrun.h \
        concurrent/qtconcurrentrunbase.h \
        concurrent/qtconcurrentstoredfunctioncall.h \
        concurrent/qtconcurrentthreadengine.h \
        concurrent/qthreadpool.h

# private headers
HEADERS += \
        concurrent/qfutureinterface_p.h \
        concurrent/qfuturewatcher_p.h \
        concurrent/qthreadpool_p.h

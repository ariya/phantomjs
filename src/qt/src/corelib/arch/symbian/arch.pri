#
# Symbian architecture
#
SOURCES +=  $$QT_ARCH_CPP/qatomic_symbian.cpp \
            $$QT_ARCH_CPP/qatomic_generic_armv6.cpp \
            $$QT_ARCH_CPP/heap_hybrid.cpp \
            $$QT_ARCH_CPP/debugfunction.cpp \
            $$QT_ARCH_CPP/qt_heapsetup_symbian.cpp

HEADERS +=  $$QT_ARCH_CPP/dla_p.h \
            $$QT_ARCH_CPP/heap_hybrid_p.h \
            $$QT_ARCH_CPP/common_p.h \
            $$QT_ARCH_CPP/page_alloc_p.h \
            $$QT_ARCH_CPP/slab_p.h \
            $$QT_ARCH_CPP/qt_hybridHeap_symbian_p.h

exists($${EPOCROOT}epoc32/include/platform/u32std.h):DEFINES += QT_SYMBIAN_HAVE_U32STD_H
exists($${EPOCROOT}epoc32/include/platform/e32btrace.h):DEFINES += QT_SYMBIAN_HAVE_E32BTRACE_H

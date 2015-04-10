win32|wince:HEADERS += arch/qatomic_msvc.h

HEADERS += \
    arch/qatomic_armv5.h \
    arch/qatomic_armv6.h \
    arch/qatomic_armv7.h \
    arch/qatomic_bootstrap.h \
    arch/qatomic_ia64.h \
    arch/qatomic_mips.h \
    arch/qatomic_x86.h \
    arch/qatomic_gcc.h \
    arch/qatomic_cxx11.h

unix {
    # fallback implementation when no other appropriate qatomic_*.h exists
    HEADERS += arch/qatomic_unix.h
    SOURCES += arch/qatomic_unix.cpp
}

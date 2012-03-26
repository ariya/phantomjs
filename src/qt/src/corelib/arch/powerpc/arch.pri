#
# PowerPC architecture
#
!*-g++* {
    *-64 {
        SOURCES += $$QT_ARCH_CPP/qatomic64.s
    } else {
        SOURCES += $$QT_ARCH_CPP/qatomic32.s
    }
}

#
# SPARC architecture
#
*-64* {
    SOURCES += $$QT_ARCH_CPP/qatomic64.s
} 
else {
    SOURCES += $$QT_ARCH_CPP/qatomic32.s \
               $$QT_ARCH_CPP/qatomic_sparc.cpp
}

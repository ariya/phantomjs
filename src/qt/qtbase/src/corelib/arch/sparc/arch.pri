#
# SPARC architecture
#
*-64* {
    SOURCES += $$PWD/qatomic64.s
}
else {
    SOURCES += $$PWD/qatomic32.s \
               $$PWD/qatomic_sparc.cpp
}

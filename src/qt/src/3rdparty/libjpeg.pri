wince*: {
    DEFINES += NO_GETENV
    contains(CE_ARCH,x86):CONFIG -= stl exceptions
    contains(CE_ARCH,x86):CONFIG += exceptions_off
}

#Disable warnings in 3rdparty code due to unused arguments
symbian: {
    QMAKE_CXXFLAGS.CW += -W nounusedarg
    TARGET.UID3=0x2001E61B
} else:contains(QMAKE_CC, gcc): {
    QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter -Wno-main
}


INCLUDEPATH += $$PWD/libjpeg
SOURCES += \
    $$PWD/libjpeg/jaricom.c \
    $$PWD/libjpeg/jcapimin.c \
    $$PWD/libjpeg/jcapistd.c \
    $$PWD/libjpeg/jcarith.c \
    $$PWD/libjpeg/jccoefct.c \
    $$PWD/libjpeg/jccolor.c \
    $$PWD/libjpeg/jcdctmgr.c \
    $$PWD/libjpeg/jchuff.c \
    $$PWD/libjpeg/jcinit.c \
    $$PWD/libjpeg/jcmainct.c \
    $$PWD/libjpeg/jcmarker.c \
    $$PWD/libjpeg/jcmaster.c \
    $$PWD/libjpeg/jcomapi.c \
    $$PWD/libjpeg/jcparam.c \
    $$PWD/libjpeg/jcprepct.c \
    $$PWD/libjpeg/jcsample.c \
    $$PWD/libjpeg/jctrans.c \
    $$PWD/libjpeg/jdapimin.c \
    $$PWD/libjpeg/jdapistd.c \
    $$PWD/libjpeg/jdarith.c \
    $$PWD/libjpeg/jdatadst.c \
    $$PWD/libjpeg/jdatasrc.c \
    $$PWD/libjpeg/jdcoefct.c \
    $$PWD/libjpeg/jdcolor.c \
    $$PWD/libjpeg/jddctmgr.c \
    $$PWD/libjpeg/jdhuff.c \
    $$PWD/libjpeg/jdinput.c \
    $$PWD/libjpeg/jdmainct.c \
    $$PWD/libjpeg/jdmarker.c \
    $$PWD/libjpeg/jdmaster.c \
    $$PWD/libjpeg/jdmerge.c \
    $$PWD/libjpeg/jdpostct.c \
    $$PWD/libjpeg/jdsample.c \
    $$PWD/libjpeg/jdtrans.c \
    $$PWD/libjpeg/jerror.c \
    $$PWD/libjpeg/jfdctflt.c \
    $$PWD/libjpeg/jfdctfst.c \
    $$PWD/libjpeg/jfdctint.c \
    $$PWD/libjpeg/jidctflt.c \
    $$PWD/libjpeg/jidctfst.c \
    $$PWD/libjpeg/jidctint.c \
    $$PWD/libjpeg/jquant1.c \
    $$PWD/libjpeg/jquant2.c \
    $$PWD/libjpeg/jutils.c \
    $$PWD/libjpeg/jmemmgr.c \
    $$PWD/libjpeg/jmemnobs.c

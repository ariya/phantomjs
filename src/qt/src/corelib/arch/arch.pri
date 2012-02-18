win32:HEADERS += arch/qatomic_windows.h \
                 arch/qatomic_generic.h

win32-g++*:HEADERS += arch/qatomic_i386.h \
                      arch/qatomic_x86_64.h

mac:HEADERS += arch/qatomic_macosx.h \
               arch/qatomic_generic.h

symbian:HEADERS +=  arch/qatomic_symbian.h \
                    arch/qatomic_generic.h

vxworks:HEADERS += arch/qatomic_vxworks.h

integrity:HEADERS += arch/qatomic_integrity.h

!wince*:!win32:!mac:!symbian:HEADERS += arch/qatomic_alpha.h \
                       arch/qatomic_avr32.h \
                       arch/qatomic_ia64.h \
                       arch/qatomic_parisc.h \
                       arch/qatomic_sparc.h \
                       arch/qatomic_arch.h \
                       arch/qatomic_generic.h \
                       arch/qatomic_powerpc.h \
                       arch/qatomic_arm.h \
                       arch/qatomic_armv5.h \
                       arch/qatomic_armv6.h \
                       arch/qatomic_armv7.h \
                       arch/qatomic_i386.h \
                       arch/qatomic_mips.h \
                       arch/qatomic_s390.h \
                       arch/qatomic_x86_64.h \
                       arch/qatomic_sh.h \
                       arch/qatomic_sh4a.h

QT_ARCH_CPP = $$QT_SOURCE_TREE/src/corelib/arch/$$QT_ARCH
DEPENDPATH += $$QT_ARCH_CPP
include($$QT_ARCH_CPP/arch.pri, "", true)

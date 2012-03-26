#
# MIPS 3/4 architecture
#

# note: even though we use inline assembler with gcc, we always
# include the compiled version to keep binary compatibility
*-64:SOURCES += $$QT_ARCH_CPP/qatomic_mips64.s
else:SOURCES += $$QT_ARCH_CPP/qatomic_mips32.s

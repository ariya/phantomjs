DEFINES *= QT_USE_BUNDLED_LIBPNG
!isEqual(QT_ARCH, i386):!isEqual(QT_ARCH, x86_64):DEFINES += PNG_NO_ASSEMBLER_CODE
INCLUDEPATH += $$PWD/libpng
SOURCES += $$PWD/libpng/png.c \
  $$PWD/libpng/pngerror.c \
  $$PWD/libpng/pngget.c \
  $$PWD/libpng/pngmem.c \
  $$PWD/libpng/pngpread.c \
  $$PWD/libpng/pngread.c \
  $$PWD/libpng/pngrio.c \
  $$PWD/libpng/pngrtran.c \
  $$PWD/libpng/pngrutil.c \
  $$PWD/libpng/pngset.c \
  $$PWD/libpng/pngtrans.c \
  $$PWD/libpng/pngwio.c \
  $$PWD/libpng/pngwrite.c \
  $$PWD/libpng/pngwtran.c \
  $$PWD/libpng/pngwutil.c

include($$PWD/zlib_dependency.pri)

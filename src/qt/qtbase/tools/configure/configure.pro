TARGET    = configure
DESTDIR   = $$PWD/../..  # build directly in source dir

CONFIG   += console flat stl rtti_off
CONFIG   -= moc qt
DEFINES  = UNICODE QT_NO_CODECS QT_NO_TEXTCODEC QT_NO_UNICODETABLES QT_LITE_COMPONENT QT_NO_COMPRESS QT_NO_THREAD QT_NO_QOBJECT QT_NO_GEOM_VARIANT _CRT_SECURE_NO_DEPRECATE
DEFINES  += QT_BOOTSTRAPPED QT_BUILD_CONFIGURE

win32 : LIBS += -lole32 -ladvapi32
mingw : LIBS += -luuid

win32-msvc* {
    QMAKE_CFLAGS_RELEASE -= -MD
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE += -MT -O1 -Os
    QMAKE_CFLAGS_DEBUG -= -MDd
    QMAKE_CFLAGS_DEBUG += -MTd
    QMAKE_CXXFLAGS_RELEASE -= -MD
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE += -MT -O1 -Os
    QMAKE_CXXFLAGS_DEBUG -= -MDd
    QMAKE_CXXFLAGS_DEBUG += -MTd
}

PRECOMPILED_HEADER = configure_pch.h

INCLUDEPATH += \
           $$QT_BUILD_TREE/include \
           $$QT_BUILD_TREE/include/QtCore \
           $$QT_BUILD_TREE/include/QtCore/$$QT.core.VERSION \
           $$QT_BUILD_TREE/include/QtCore/$$QT.core.VERSION/QtCore \
           $$QT_SOURCE_TREE/tools/shared

HEADERS  = configureapp.h environment.h tools.h\
           $$QT_SOURCE_TREE/src/corelib/tools/qarraydata.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qbytearray.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qarraydatapointer.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qarraydataops.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qbytearraymatcher.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qchar.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qhash.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qlist.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qlocale.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qvector.h \
           $$QT_SOURCE_TREE/src/corelib/codecs/qutfcodec_p.h \
           $$QT_SOURCE_TREE/src/corelib/codecs/qtextcodec.h \
           $$QT_SOURCE_TREE/src/corelib/global/qglobal.h \
           $$QT_SOURCE_TREE/src/corelib/global/qnumeric.h \
           $$QT_SOURCE_TREE/src/corelib/global/qlogging.h \
           $$QT_SOURCE_TREE/src/corelib/io/qbuffer.h \
           $$QT_SOURCE_TREE/src/corelib/io/qdatastream.h \
           $$QT_SOURCE_TREE/src/corelib/io/qdir.h \
           $$QT_SOURCE_TREE/src/corelib/io/qdiriterator.h \
           $$QT_SOURCE_TREE/src/corelib/io/qfiledevice.h \
           $$QT_SOURCE_TREE/src/corelib/io/qfile.h \
           $$QT_SOURCE_TREE/src/corelib/io/qfileinfo.h \
           $$QT_SOURCE_TREE/src/corelib/io/qfilesystementry_p.h \
           $$QT_SOURCE_TREE/src/corelib/io/qfilesystemengine_p.h \
           $$QT_SOURCE_TREE/src/corelib/io/qfilesystemmetadata_p.h \
           $$QT_SOURCE_TREE/src/corelib/io/qfilesystemiterator_p.h \
           $$QT_SOURCE_TREE/src/corelib/io/qfsfileengine.h \
           $$QT_SOURCE_TREE/src/corelib/io/qfsfileengine_iterator_p.h \
           $$QT_SOURCE_TREE/src/corelib/io/qiodevice.h \
           $$QT_SOURCE_TREE/src/corelib/io/qtextstream.h \
           $$QT_SOURCE_TREE/src/corelib/io/qtemporaryfile.h \
           $$QT_SOURCE_TREE/src/corelib/io/qstandardpaths.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qbitarray.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qdatetime.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qmap.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qregexp.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qstring.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qstringlist.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qstringmatcher.h \
           $$QT_SOURCE_TREE/src/corelib/tools/qunicodetables_p.h \
           $$QT_SOURCE_TREE/src/corelib/kernel/qsystemerror_p.h \
           $$QT_SOURCE_TREE/src/corelib/xml/qxmlstream.h \
           $$QT_SOURCE_TREE/src/corelib/xml/qxmlutils_p.h \
           $$QT_SOURCE_TREE/tools/shared/windows/registry_p.h


SOURCES  = main.cpp configureapp.cpp environment.cpp tools.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qbytearray.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qarraydata.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qbytearraymatcher.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qhash.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qlist.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qlocale.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qlocale_win.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qlocale_tools.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qvector.cpp \
           $$QT_SOURCE_TREE/src/corelib/codecs/qutfcodec.cpp \
           $$QT_SOURCE_TREE/src/corelib/codecs/qtextcodec.cpp \
           $$QT_SOURCE_TREE/src/corelib/global/qglobal.cpp \
           $$QT_SOURCE_TREE/src/corelib/global/qnumeric.cpp \
           $$QT_SOURCE_TREE/src/corelib/global/qlogging.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qbuffer.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qdatastream.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qdir.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qdiriterator.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfiledevice.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfile.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfileinfo.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qabstractfileengine.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfilesystementry.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfilesystemengine.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfilesystemengine_win.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfilesystemiterator_win.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfsfileengine.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfsfileengine_win.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qfsfileengine_iterator.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qiodevice.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qtextstream.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qtemporaryfile.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qstandardpaths.cpp \
           $$QT_SOURCE_TREE/src/corelib/io/qstandardpaths_win.cpp \
           $$QT_SOURCE_TREE/src/corelib/plugin/qsystemlibrary.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qbitarray.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qdatetime.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qmap.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qregexp.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qstring.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qstring_compat.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qstringlist.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qvsnprintf.cpp \
           $$QT_SOURCE_TREE/src/corelib/kernel/qvariant.cpp \
           $$QT_SOURCE_TREE/src/corelib/kernel/qsystemerror.cpp \
           $$QT_SOURCE_TREE/src/corelib/kernel/qmetatype.cpp \
           $$QT_SOURCE_TREE/src/corelib/global/qmalloc.cpp \
           $$QT_SOURCE_TREE/src/corelib/xml/qxmlstream.cpp \
           $$QT_SOURCE_TREE/src/corelib/xml/qxmlutils.cpp \
           $$QT_SOURCE_TREE/src/corelib/plugin/quuid.cpp \
           $$QT_SOURCE_TREE/src/corelib/tools/qcryptographichash.cpp \
           $$QT_SOURCE_TREE/tools/shared/windows/registry.cpp

DEFINES += COMMERCIAL_VERSION

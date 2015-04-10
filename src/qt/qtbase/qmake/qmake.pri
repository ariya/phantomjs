
#qmake code
SOURCES += project.cpp property.cpp main.cpp \
           library/ioutils.cpp library/proitems.cpp library/qmakevfs.cpp library/qmakeglobals.cpp \
           library/qmakeparser.cpp library/qmakeevaluator.cpp library/qmakebuiltins.cpp \
           generators/makefile.cpp \
           generators/unix/unixmake2.cpp generators/unix/unixmake.cpp meta.cpp \
           option.cpp generators/win32/winmakefile.cpp generators/win32/mingw_make.cpp \
           generators/makefiledeps.cpp generators/metamakefile.cpp generators/mac/pbuilder_pbx.cpp \
           generators/xmloutput.cpp \
           generators/win32/msvc_nmake.cpp generators/projectgenerator.cpp \
           generators/win32/msvc_vcproj.cpp \
           generators/win32/msvc_vcxproj.cpp \
           generators/win32/msvc_objectmodel.cpp generators/win32/msbuild_objectmodel.cpp \
           generators/integrity/gbuild.cpp \
           generators/win32/cesdkhandler.cpp

HEADERS += project.h property.h \
           library/qmake_global.h library/ioutils.h library/proitems.h library/qmakevfs.h library/qmakeglobals.h \
           library/qmakeparser.h library/qmakeevaluator.h library/qmakeevaluator_p.h \
           generators/makefile.h \
           generators/unix/unixmake.h meta.h option.h cachekeys.h \
           generators/win32/winmakefile.h generators/win32/mingw_make.h generators/projectgenerator.h \
           generators/makefiledeps.h generators/metamakefile.h generators/mac/pbuilder_pbx.h \
           generators/xmloutput.h generators/win32/msvc_nmake.h \
           generators/win32/msvc_vcproj.h \
           generators/win32/msvc_vcxproj.h \
           generators/win32/msvc_objectmodel.h generators/win32/msbuild_objectmodel.h \
           generators/integrity/gbuild.h \
           generators/win32/cesdkhandler.h

bootstrap { #Qt code
   SOURCES+= \
        qbitarray.cpp \
        qbuffer.cpp \
        qarraydata.cpp \
        qbytearray.cpp \
        qbytearraymatcher.cpp \
        qcryptographichash.cpp \
        qdatetime.cpp \
        qdir.cpp \
        qdiriterator.cpp \
        qfiledevice.cpp \
        qfile.cpp \
        qabstractfileengine.cpp \
        qfileinfo.cpp \
        qfilesystementry.cpp \
        qfilesystemengine.cpp \
        qfsfileengine.cpp \
        qfsfileengine_iterator.cpp \
        qglobal.cpp \
        qnumeric.cpp \
        qhash.cpp \
        qiodevice.cpp \
        qlist.cpp \
        qlinkedlist.cpp \
        qlocale.cpp \
        qlocale_tools.cpp \
        qmalloc.cpp \
        qmap.cpp \
        qmetatype.cpp \
        qregexp.cpp \
        qtextcodec.cpp \
        qutfcodec.cpp \
        qstring.cpp \
        qstring_compat.cpp \
        qstringlist.cpp \
        qtemporaryfile.cpp \
        qtextstream.cpp \
        quuid.cpp \
        qsettings.cpp \
        qlibraryinfo.cpp \
        qsystemerror.cpp \
        qvariant.cpp \
        qvector.cpp \
        qvsnprintf.cpp \
        qxmlstream.cpp \
        qxmlutils.cpp \
        qlogging.cpp \
        qjson.cpp \
        qjsondocument.cpp \
        qjsonparser.cpp \
        qjsonarray.cpp \
        qjsonobject.cpp \
        qjsonvalue.cpp

   HEADERS+= \
        qbitarray.h \
        qbuffer.h \
        qarraydata.h \
        qbytearray.h \
        qarraydataops.h \
        qarraydatapointer.h \
        qbytearraymatcher.h \
        qchar.h \
        qcryptographichash.h \
        qdatetime.h \
        qdatetime_p.h \
        qdir.h \
        qdir_p.h \
        qdiriterator.h \
        qfile.h \
        qabstractfileengine_p.h \
        qfileinfo.h \
        qglobal.h \
        qnumeric.h \
        qhash.h \
        qiodevice.h \
        qlist.h \
        qlinkedlist.h \
        qlocale.h \
        qlocale_tools_p.h \
        qmalloc.h \
        qmap.h \
        qmetatype.h \
        qregexp.h \
        qtextcodec.h \
        qutfcodec.h \
        qstring.h \
        qstringlist.h \
        qstringmatcher.h \
        qsystemerror_p.h \
        qtemporaryfile.h \
        qtextstream.h \
        quuid.h \
        qvector.h \
        qxmlstream.h \
        qxmlutils.h \
        qjson.h \
        qjsondocument.h \
        qjsonparser.h \
        qjsonwriter.h \
        qjsonarray.h \
        qjsonobject.h \
        qjsonvalue.h

    unix {
        SOURCES += qfilesystemengine_unix.cpp qfilesystemiterator_unix.cpp qfsfileengine_unix.cpp
        mac {
          SOURCES += qcore_mac.cpp qsettings_mac.cpp qlocale_mac.mm
          LIBS += -framework ApplicationServices
        } else {
          SOURCES += qlocale_unix.cpp
        }
    } else:win32 {
        SOURCES += qfilesystemengine_win.cpp qfsfileengine_win.cpp qfilesystemiterator_win.cpp qsettings_win.cpp \
            qsystemlibrary.cpp qlocale_win.cpp registry.cpp
        win32-msvc*:LIBS += ole32.lib advapi32.lib
        mingw:LIBS += -lole32 -luuid -ladvapi32 -lkernel32
    }

    qnx {
        CFLAGS += -fhonor-std
        LFLAGS += -lcpp
    }

    DEFINES += \
        QT_BOOTSTRAPPED \
        QT_NO_TEXTCODEC QT_NO_UNICODETABLES QT_NO_COMPONENT QT_NO_COMPRESS \
        QT_NO_THREAD QT_NO_QOBJECT QT_NO_GEOM_VARIANT QT_NO_DATASTREAM \
        QT_CRYPTOGRAPHICHASH_ONLY_SHA1 QT_JSON_READONLY

    INCLUDEPATH += \
        $$QT.core.includes $$QT.core_private.includes \
        $$shadowed(../src/corelib/global)
} else {
    CONFIG += qt
    QT = core
}
*-g++:profiling {
  QMAKE_CFLAGS = -pg
  QMAKE_CXXFLAGS = -pg
  QMAKE_LFLAGS = -pg
}

PRECOMPILED_HEADER = qmake_pch.h

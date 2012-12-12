# Qt core io module

HEADERS +=  \
        io/qabstractfileengine.h \
        io/qabstractfileengine_p.h \
        io/qbuffer.h \
        io/qdatastream.h \
        io/qdatastream_p.h \
        io/qdataurl_p.h \
        io/qdebug.h \
        io/qdir.h \
        io/qdir_p.h \
        io/qdiriterator.h \
        io/qfile.h \
        io/qfileinfo.h \
        io/qfileinfo_p.h \
        io/qiodevice.h \
        io/qiodevice_p.h \
        io/qnoncontiguousbytedevice_p.h \
        io/qprocess.h \
        io/qprocess_p.h \
        io/qtextstream.h \
        io/qtemporaryfile.h \
        io/qresource_p.h \
        io/qresource_iterator_p.h \
        io/qurl.h \
        io/qurltlds_p.h \
        io/qtldurl_p.h \
        io/qsettings.h \
        io/qsettings_p.h \
        io/qfsfileengine.h \
        io/qfsfileengine_p.h \
        io/qfsfileengine_iterator_p.h \
        io/qfilesystemwatcher.h \
        io/qfilesystemwatcher_p.h \
        io/qfilesystementry_p.h \
        io/qfilesystemengine_p.h \
        io/qfilesystemmetadata_p.h \
        io/qfilesystemiterator_p.h

SOURCES += \
        io/qabstractfileengine.cpp \
        io/qbuffer.cpp \
        io/qdatastream.cpp \
        io/qdataurl.cpp \
        io/qtldurl.cpp \
        io/qdebug.cpp \
        io/qdir.cpp \
        io/qdiriterator.cpp \
        io/qfile.cpp \
        io/qfileinfo.cpp \
        io/qiodevice.cpp \
        io/qnoncontiguousbytedevice.cpp \
        io/qprocess.cpp \
        io/qtextstream.cpp \
        io/qtemporaryfile.cpp \
        io/qresource.cpp \
        io/qresource_iterator.cpp \
        io/qurl.cpp \
        io/qsettings.cpp \
        io/qfsfileengine.cpp \
        io/qfsfileengine_iterator.cpp \
        io/qfilesystemwatcher.cpp \
        io/qfilesystementry.cpp \
        io/qfilesystemengine.cpp

win32 {
        SOURCES += io/qsettings_win.cpp
        SOURCES += io/qprocess_win.cpp
        SOURCES += io/qfsfileengine_win.cpp

        SOURCES += io/qfilesystemwatcher_win.cpp
        HEADERS += io/qfilesystemwatcher_win_p.h
        HEADERS += io/qwindowspipewriter_p.h
        SOURCES += io/qwindowspipewriter.cpp
        SOURCES += io/qfilesystemengine_win.cpp
        SOURCES += io/qfilesystemiterator_win.cpp
} else:unix {
        SOURCES += io/qfsfileengine_unix.cpp
        symbian {
            SOURCES += io/qfilesystemengine_symbian.cpp
            SOURCES += io/qprocess_symbian.cpp
            SOURCES += io/qfilesystemiterator_symbian.cpp
        } else {
            SOURCES += io/qfilesystemengine_unix.cpp
            SOURCES += io/qprocess_unix.cpp
            SOURCES += io/qfilesystemiterator_unix.cpp
        }
        !nacl:macx-*: {
            HEADERS += io/qfilesystemwatcher_fsevents_p.h
            SOURCES += io/qfilesystemengine_mac.cpp
            SOURCES += io/qsettings_mac.cpp io/qfilesystemwatcher_fsevents.cpp
        }

        qnx:contains(QT_CONFIG, inotify) {
            SOURCES += io/qfilesystemwatcher_inotify.cpp
            HEADERS += io/qfilesystemwatcher_inotify_p.h
        }

        linux-*:!symbian {
            SOURCES += \
                    io/qfilesystemwatcher_inotify.cpp \
                    io/qfilesystemwatcher_dnotify.cpp

            HEADERS += \
                    io/qfilesystemwatcher_inotify_p.h \
                    io/qfilesystemwatcher_dnotify_p.h
        }

        !nacl {
            freebsd-*|macx-*|darwin-*|openbsd-*:{
                SOURCES += io/qfilesystemwatcher_kqueue.cpp
                HEADERS += io/qfilesystemwatcher_kqueue_p.h
            }
        }

        symbian {
            SOURCES += io/qfilesystemwatcher_symbian.cpp
            HEADERS += io/qfilesystemwatcher_symbian_p.h
            INCLUDEPATH += $$MW_LAYER_SYSTEMINCLUDE
            LIBS += -lplatformenv -lesock
        }
}
integrity {
	SOURCES += io/qfsfileengine_unix.cpp \
            io/qfsfileengine_iterator.cpp \
            io/qfilesystemengine_unix.cpp \
            io/qfilesystemiterator_unix.cpp
}

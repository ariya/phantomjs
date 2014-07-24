# Qt core io module

HEADERS +=  \
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
        io/qfiledevice.h \
        io/qfiledevice_p.h \
        io/qfileinfo.h \
        io/qfileinfo_p.h \
        io/qipaddress_p.h \
        io/qiodevice.h \
        io/qiodevice_p.h \
        io/qlockfile.h \
        io/qlockfile_p.h \
        io/qnoncontiguousbytedevice_p.h \
        io/qprocess.h \
        io/qprocess_p.h \
        io/qtextstream.h \
        io/qtextstream_p.h \
        io/qtemporarydir.h \
        io/qtemporaryfile.h \
        io/qtemporaryfile_p.h \
        io/qresource_p.h \
        io/qresource_iterator_p.h \
        io/qsavefile.h \
        io/qstandardpaths.h \
        io/qurl.h \
        io/qurl_p.h \
        io/qurlquery.h \
        io/qurltlds_p.h \
        io/qtldurl_p.h \
        io/qsettings.h \
        io/qsettings_p.h \
        io/qfsfileengine_p.h \
        io/qfsfileengine_iterator_p.h \
        io/qfilesystemwatcher.h \
        io/qfilesystemwatcher_p.h \
        io/qfilesystemwatcher_polling_p.h \
        io/qfilesystementry_p.h \
        io/qfilesystemengine_p.h \
        io/qfilesystemmetadata_p.h \
        io/qfilesystemiterator_p.h \
        io/qfileselector.h \
        io/qfileselector_p.h \
        io/qloggingcategory.h \
        io/qloggingcategory_p.h \
        io/qloggingregistry_p.h

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
        io/qfiledevice.cpp \
        io/qfileinfo.cpp \
        io/qipaddress.cpp \
        io/qiodevice.cpp \
        io/qlockfile.cpp \
        io/qnoncontiguousbytedevice.cpp \
        io/qprocess.cpp \
        io/qtextstream.cpp \
        io/qtemporarydir.cpp \
        io/qtemporaryfile.cpp \
        io/qresource.cpp \
        io/qresource_iterator.cpp \
        io/qsavefile.cpp \
        io/qstandardpaths.cpp \
        io/qurl.cpp \
        io/qurlidna.cpp \
        io/qurlquery.cpp \
        io/qurlrecode.cpp \
        io/qsettings.cpp \
        io/qfsfileengine.cpp \
        io/qfsfileengine_iterator.cpp \
        io/qfilesystemwatcher.cpp \
        io/qfilesystemwatcher_polling.cpp \
        io/qfilesystementry.cpp \
        io/qfilesystemengine.cpp \
        io/qfileselector.cpp \
        io/qloggingcategory.cpp \
        io/qloggingregistry.cpp

win32 {
        SOURCES += io/qfsfileengine_win.cpp
        SOURCES += io/qlockfile_win.cpp

        SOURCES += io/qfilesystemwatcher_win.cpp
        HEADERS += io/qfilesystemwatcher_win_p.h
        SOURCES += io/qfilesystemengine_win.cpp
        SOURCES += io/qfilesystemiterator_win.cpp

    !winrt {
        SOURCES += io/qsettings_win.cpp
        HEADERS += io/qwindowspipewriter_p.h
        SOURCES += io/qwindowspipewriter.cpp
        SOURCES += io/qstandardpaths_win.cpp

        wince* {
            SOURCES += io/qprocess_wince.cpp
        } else {
            HEADERS += \
                io/qwinoverlappedionotifier_p.h \
                io/qwindowspipereader_p.h
            SOURCES += \
                io/qprocess_win.cpp \
                io/qwinoverlappedionotifier.cpp \
                io/qwindowspipereader.cpp
        }
    } else {
        SOURCES += io/qstandardpaths_winrt.cpp
    }
} else:unix|integrity {
        SOURCES += \
                io/qfsfileengine_unix.cpp \
                io/qfilesystemengine_unix.cpp \
                io/qlockfile_unix.cpp \
                io/qprocess_unix.cpp \
                io/qfilesystemiterator_unix.cpp \

        !nacl:mac: {
            SOURCES += io/qsettings_mac.cpp
            OBJECTIVE_SOURCES += io/qurl_mac.mm
        }
        mac {
            osx {
                OBJECTIVE_SOURCES += io/qfilesystemwatcher_fsevents.mm
                HEADERS += io/qfilesystemwatcher_fsevents_p.h
            }
            macx {
                SOURCES += io/qstandardpaths_mac.cpp
            } else:ios {
                OBJECTIVE_SOURCES += io/qstandardpaths_ios.mm
            } else {
                SOURCES += io/qstandardpaths_unix.cpp
            }
        } else:blackberry {
            SOURCES += io/qstandardpaths_blackberry.cpp
        } else:android:!android-no-sdk {
            SOURCES += io/qstandardpaths_android.cpp
        } else {
            SOURCES += io/qstandardpaths_unix.cpp
        }

        linux|if(qnx:contains(QT_CONFIG, inotify)) {
            SOURCES += io/qfilesystemwatcher_inotify.cpp
            HEADERS += io/qfilesystemwatcher_inotify_p.h
        }

        !nacl {
            freebsd-*|mac|darwin-*|openbsd-*:{
                SOURCES += io/qfilesystemwatcher_kqueue.cpp
                HEADERS += io/qfilesystemwatcher_kqueue_p.h
            }
        }
}


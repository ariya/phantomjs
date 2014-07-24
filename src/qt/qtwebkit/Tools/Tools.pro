# -------------------------------------------------------------------
# Root project file for tools
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered

build?(webkit1) {
    build?(testbrowser): SUBDIRS += QtTestBrowser/QtTestBrowser.pro
    build?(drt): SUBDIRS += DumpRenderTree/qt/DumpRenderTree.pro
}

build?(webkit2) {
    # WTR's InjectedBundle depends currently on WK1's DumpRenderTreeSupport
    build?(webkit1):build?(wtr):have?(QTQUICK): SUBDIRS += WebKitTestRunner/WebKitTestRunner.pro

    build?(minibrowser) {
        have?(QTQUICK): SUBDIRS += MiniBrowser/qt/MiniBrowser.pro
        SUBDIRS += MiniBrowser/qt/raw/MiniBrowserRaw.pro
    }

    !production_build {
        # Only tested on Linux so far.
        linux* {
            SUBDIRS += TestWebKitAPI
        }
    }
}

build?(imagediff): SUBDIRS += ImageDiff/ImageDiff.pro

build?(test_npapi): SUBDIRS += DumpRenderTree/qt/TestNetscapePlugin/TestNetscapePlugin.pro

OTHER_FILES = \
    Scripts/* \
    $$files(Scripts/webkitpy/*.py, true) \
    $$files(Scripts/webkitperl/*.p[l|m], true) \
    qmake/README \
    qmake/dump-features.pl \
    qmake/config.tests/README \
    qmake/config.tests/fontconfig/* \
    qmake/config.tests/gccdepends/* \
    qmake/config.tests/libexecdir/* \
    qmake/mkspecs/modules/* \
    qmake/mkspecs/features/*.prf \
    qmake/mkspecs/features/*.pri \
    qmake/mkspecs/features/mac/*.prf \
    qmake/mkspecs/features/unix/*.prf \
    qmake/mkspecs/features/win32/*.prf

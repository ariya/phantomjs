# -------------------------------------------------------------------
# Root project file, used to load WebKit in Qt Creator and for
# building QtWebKit.
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered

!equals(QT_MAJOR_VERSION, 5): error("Building WebKit with Qt versions older than 5.0 is not supported.")

WTF.file = Source/WTF/WTF.pro
WTF.makefile = Makefile.WTF
SUBDIRS += WTF

JavaScriptCore.file = Source/JavaScriptCore/JavaScriptCore.pro
JavaScriptCore.makefile = Makefile.JavaScriptCore
SUBDIRS += JavaScriptCore

use?(3D_GRAPHICS) {
    ANGLE.file = Source/ThirdParty/ANGLE/ANGLE.pro
    ANGLE.makefile = Makefile.ANGLE
    SUBDIRS += ANGLE
}

use?(leveldb):!use?(system_leveldb) {
    leveldb.file = Source/ThirdParty/leveldb/leveldb.pro
    leveldb.makefile = Makefile.leveldb
    SUBDIRS += leveldb
}

WebCore.file = Source/WebCore/WebCore.pro
WebCore.makefile = Makefile.WebCore
SUBDIRS += WebCore

build?(webkit1) {
    webkit1.file = Source/WebKit/WebKit1.pro
    webkit1.makefile = Makefile.WebKit1
    SUBDIRS += webkit1
}

build?(webkit2) {
    webkit2.file = Source/WebKit2/WebKit2.pro
    webkit2.makefile = Makefile.WebKit2
    SUBDIRS += webkit2
}

QtWebKit.file = Source/QtWebKit.pro
QtWebKit.makefile = Makefile.QtWebKit
SUBDIRS += QtWebKit

!production_build {
    # Only tested on Linux so far.
    linux* {
        gtest.file = Source/ThirdParty/gtest/gtest.pro
        gtest.makefile = Makefile.gtest
        SUBDIRS += gtest
    }
}

Tools.file = Tools/Tools.pro
Tools.makefile = Makefile.Tools
SUBDIRS += Tools

# Number of times incremental builds have failed: 1

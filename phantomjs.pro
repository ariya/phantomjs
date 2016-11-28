TEMPLATE = subdirs
CONFIG += ordered

!win {
    SUBDIRS += $$PWD/src/qt-qpa-platform-plugin/phantom.pro
}

SUBDIRS += $$PWD/src/phantomjs.pro

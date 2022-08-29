TEMPLATE = subdirs
CONFIG += ordered

linux {
    SUBDIRS += $$PWD/src/qt-qpa-platform-plugin/phantom.pro
}
SUBDIRS += $$PWD/src/phantomjs.pro

linux {
    phantomjs.depends = phantom
}

TEMPLATE = subdirs

contains(QT_CONFIG, dbus) {
    contains(QT_CONFIG, icd) {
        SUBDIRS += icd
    } else:linux* {
        SUBDIRS += generic
        SUBDIRS += connman networkmanager
    }
}

#win32:SUBDIRS += nla
win32:SUBDIRS += generic
win32:!wince*:SUBDIRS += nativewifi
macx:contains(QT_CONFIG, corewlan):SUBDIRS += corewlan
macx:SUBDIRS += generic
symbian:SUBDIRS += symbian
blackberry:SUBDIRS += blackberry

isEmpty(SUBDIRS):SUBDIRS = generic

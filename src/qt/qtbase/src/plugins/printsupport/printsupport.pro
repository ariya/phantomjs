TEMPLATE = subdirs

osx:   SUBDIRS += cocoa
win32: SUBDIRS += windows
unix:!mac:contains(QT_CONFIG, cups): SUBDIRS += cups

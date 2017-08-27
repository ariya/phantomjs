QT += core-private gui-private

lessThan(QT_MINOR_VERSION, 8) {
    QT += platformsupport-private
} else {
    QT += fontdatabase_support_private eventdispatcher_support_private
}

SOURCES += $$PWD/phantomintegration.cpp \
           $$PWD/phantombackingstore.cpp

HEADERS += $$PWD/phantomintegration.h \
           $$PWD/phantombackingstore.h

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

INCLUDEPATH += $$PWD

CONFIG += qpa/genericunixfontdatabase

OTHER_FILES += $$PWD/phantom.json

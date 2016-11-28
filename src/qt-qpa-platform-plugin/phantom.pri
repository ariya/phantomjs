QT += core-private gui-private platformsupport-private

SOURCES += $$PWD/phantomintegration.cpp \
           $$PWD/phantombackingstore.cpp

HEADERS += $$PWD/phantomintegration.h \
           $$PWD/phantombackingstore.h

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

INCLUDEPATH += $$PWD

CONFIG += qpa/genericunixfontdatabase

OTHER_FILES += $$PWD/phantom.json

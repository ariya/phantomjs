HEADERS += $$PWD/qfontconfigdatabase_p.h \
           $$PWD/qfontenginemultifontconfig_p.h
SOURCES += $$PWD/qfontconfigdatabase.cpp \
           $$PWD/qfontenginemultifontconfig.cpp
DEFINES -= QT_NO_FONTCONFIG
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_FONTCONFIG

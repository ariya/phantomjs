contains(QT_CONFIG, fontconfig) {
    include(../fontconfig/fontconfig.pri)
    DEFINES += Q_FONTCONFIGDATABASE
} else {
    include(../basicunix/basicunix.pri)
}

INCLUDEPATH += $$PWD
HEADERS += \
           $$PWD/qgenericunixfontdatabase.h

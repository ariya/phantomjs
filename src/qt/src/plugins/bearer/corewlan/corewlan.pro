TARGET = qcorewlanbearer
include(../../qpluginbase.pri)

QT = core network
LIBS += -framework Foundation -framework SystemConfiguration

contains(QT_CONFIG, corewlan) {
    isEmpty(QMAKE_MAC_SDK)|contains(QMAKE_MAC_SDK, "/Developer/SDKs/MacOSX10\.[67]\.sdk") {
         LIBS += -framework CoreWLAN -framework Security
    }
}

HEADERS += qcorewlanengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           ../qnetworksession_impl.cpp

OBJECTIVE_SOURCES += qcorewlanengine.mm

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/bearer
target.path += $$[QT_INSTALL_PLUGINS]/bearer
INSTALLS += target

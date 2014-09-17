# Qt network bearer management module

HEADERS += bearer/qnetworkconfiguration.h \
           bearer/qnetworksession.h \
           bearer/qnetworkconfigmanager.h \
           bearer/qnetworkconfigmanager_p.h \
           bearer/qnetworkconfiguration_p.h \
           bearer/qnetworksession_p.h \
           bearer/qbearerengine_p.h \
           bearer/qbearerplugin_p.h \
           bearer/qsharednetworksession_p.h

SOURCES += bearer/qnetworksession.cpp \
           bearer/qnetworkconfigmanager.cpp \
           bearer/qnetworkconfiguration.cpp \
           bearer/qnetworkconfigmanager_p.cpp \
           bearer/qbearerengine.cpp \
           bearer/qbearerplugin.cpp \
           bearer/qsharednetworksession.cpp

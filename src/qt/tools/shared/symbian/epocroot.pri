# Epocroot resolving is only required for tools, so omit it from all mobile/embedded builds
!symbian:!wince*:!embedded {
HEADERS += \
        $$QT_SOURCE_TREE/tools/shared/symbian/epocroot_p.h \
        $$QT_SOURCE_TREE/tools/shared/windows/registry_p.h
SOURCES += \
        $$QT_SOURCE_TREE/tools/shared/symbian/epocroot.cpp \
        $$QT_SOURCE_TREE/tools/shared/windows/registry.cpp
INCLUDEPATH += $$QT_SOURCE_TREE/tools/shared
DEFINES += QLIBRARYINFO_EPOCROOT
}

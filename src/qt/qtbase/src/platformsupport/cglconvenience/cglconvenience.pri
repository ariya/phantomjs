mac:!ios {
    INCLUDEPATH += $$PWD

    HEADERS += \
        $$PWD/cglconvenience_p.h

    OBJECTIVE_SOURCES += \
        $$PWD/cglconvenience.mm

    LIBS_PRIVATE += -framework Cocoa -framework OpenGL
}

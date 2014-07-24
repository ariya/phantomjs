contains(QT_CONFIG, xlib) {
    contains(QT_CONFIG,opengl):!contains(QT_CONFIG,opengles2) {
        contains(QT_CONFIG, xrender): LIBS_PRIVATE += -lXrender
        LIBS_PRIVATE += $$QMAKE_LIBS_X11
        HEADERS += $$PWD/qglxconvenience_p.h
        SOURCES += $$PWD/qglxconvenience.cpp
    }
}

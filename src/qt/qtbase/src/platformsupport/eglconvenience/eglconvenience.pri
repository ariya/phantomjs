contains(QT_CONFIG,egl) {
    HEADERS += \
        $$PWD/qeglconvenience_p.h \
        $$PWD/qeglpbuffer_p.h

    SOURCES += \
        $$PWD/qeglconvenience.cpp \
        $$PWD/qeglpbuffer.cpp

    contains(QT_CONFIG,opengl) {
        HEADERS += $$PWD/qeglplatformcontext_p.h
        SOURCES += $$PWD/qeglplatformcontext.cpp

        unix {
            HEADERS += \
                $$PWD/qeglplatformcursor_p.h \
                $$PWD/qeglplatformwindow_p.h \
                $$PWD/qeglplatformscreen_p.h \
                $$PWD/qeglcompositor_p.h \
                $$PWD/qeglplatformbackingstore_p.h \
                $$PWD/qeglplatformintegration_p.h

            SOURCES += \
                $$PWD/qeglplatformcursor.cpp \
                $$PWD/qeglplatformwindow.cpp \
                $$PWD/qeglplatformscreen.cpp \
                $$PWD/qeglcompositor.cpp \
                $$PWD/qeglplatformbackingstore.cpp \
                $$PWD/qeglplatformintegration.cpp
        }
    }

    # Avoid X11 header collision
    DEFINES += MESA_EGL_NO_X11_HEADERS

    contains(QT_CONFIG,xlib) {
        HEADERS += \
            $$PWD/qxlibeglintegration_p.h
        SOURCES += \
            $$PWD/qxlibeglintegration.cpp
    }
    CONFIG += egl

} else: contains(QT_CONFIG,dynamicgl) {
    HEADERS += \
        $$PWD/qeglconvenience_p.h \
        $$PWD/qeglplatformcontext_p.h \
        $$PWD/qeglpbuffer_p.h

    SOURCES += \
        $$PWD/qeglconvenience.cpp \
        $$PWD/qeglplatformcontext.cpp \
        $$PWD/qeglpbuffer.cpp
}

HEADERS += \
        $$PWD/qpaintengine_alpha_p.h \
        $$PWD/qpaintengine_preview_p.h \
        $$PWD/qprint_p.h \
        $$PWD/qprintdevice_p.h \
        $$PWD/qprintengine.h \
        $$PWD/qprinter.h \
        $$PWD/qprinter_p.h \
        $$PWD/qprinterinfo.h \
        $$PWD/qprinterinfo_p.h \
        $$PWD/qplatformprintdevice.h \
        $$PWD/qplatformprintplugin.h \
        $$PWD/qplatformprintersupport.h \
        $$PWD/qtprintsupportglobal.h

SOURCES += \
        $$PWD/qpaintengine_alpha.cpp \
        $$PWD/qpaintengine_preview.cpp \
        $$PWD/qprintdevice.cpp \
        $$PWD/qprintengine_pdf.cpp \
        $$PWD/qprinter.cpp \
        $$PWD/qprinterinfo.cpp \
        $$PWD/qplatformprintdevice.cpp \
        $$PWD/qplatformprintplugin.cpp \
        $$PWD/qplatformprintersupport.cpp

win32 {
        HEADERS += \
                $$PWD/qprintengine_win_p.h
        SOURCES += \
                $$PWD/qprintengine_win.cpp
        LIBS_PRIVATE += -lwinspool -lcomdlg32 -lgdi32 -luser32
}

unix:!mac:contains(QT_CONFIG, cups): {
        SOURCES += $$PWD/qcups.cpp
        HEADERS += $$PWD/qcups_p.h
}

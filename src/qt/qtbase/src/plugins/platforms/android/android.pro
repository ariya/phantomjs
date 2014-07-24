TARGET = qtforandroid

PLUGIN_TYPE = platforms

# STATICPLUGIN needed because there's a Q_IMPORT_PLUGIN in androidjnimain.cpp
# Yes, the plugin imports itself statically
DEFINES += QT_STATICPLUGIN

load(qt_plugin)

!contains(ANDROID_PLATFORM, android-9) {
    INCLUDEPATH += $$NDK_ROOT/platforms/android-9/arch-$$ANDROID_ARCHITECTURE/usr/include
    LIBS += -L$$NDK_ROOT/platforms/android-9/arch-$$ANDROID_ARCHITECTURE/usr/lib -ljnigraphics -landroid
} else {
    LIBS += -ljnigraphics -landroid
}

QT += core-private gui-private platformsupport-private

CONFIG += qpa/genericunixfontdatabase

OTHER_FILES += $$PWD/android.json

INCLUDEPATH += $$PWD

SOURCES += $$PWD/androidplatformplugin.cpp \
           $$PWD/androidjnimain.cpp \
           $$PWD/androidjniaccessibility.cpp \
           $$PWD/androidjniinput.cpp \
           $$PWD/androidjnimenu.cpp \
           $$PWD/androidjniclipboard.cpp \
           $$PWD/qandroidplatformintegration.cpp \
           $$PWD/qandroidplatformservices.cpp \
           $$PWD/qandroidassetsfileenginehandler.cpp \
           $$PWD/qandroidinputcontext.cpp \
           $$PWD/qandroidplatformaccessibility.cpp \
           $$PWD/qandroidplatformfontdatabase.cpp \
           $$PWD/qandroidplatformdialoghelpers.cpp \
           $$PWD/qandroidplatformclipboard.cpp \
           $$PWD/qandroidplatformtheme.cpp \
           $$PWD/qandroidplatformmenubar.cpp \
           $$PWD/qandroidplatformmenu.cpp \
           $$PWD/qandroidplatformmenuitem.cpp \
           $$PWD/qandroidsystemlocale.cpp \
           $$PWD/qandroidplatformscreen.cpp \
           $$PWD/qandroidplatformwindow.cpp \
           $$PWD/qandroidplatformopenglwindow.cpp \
           $$PWD/qandroidplatformrasterwindow.cpp \
           $$PWD/qandroidplatformbackingstore.cpp \
           $$PWD/qandroidplatformopenglcontext.cpp \
           $$PWD/qandroidplatformforeignwindow.cpp

HEADERS += $$PWD/qandroidplatformintegration.h \
           $$PWD/androidjnimain.h \
           $$PWD/androidjniaccessibility.h \
           $$PWD/androidjniinput.h \
           $$PWD/androidjnimenu.h \
           $$PWD/androidjniclipboard.h \
           $$PWD/qandroidplatformservices.h \
           $$PWD/qandroidassetsfileenginehandler.h \
           $$PWD/qandroidinputcontext.h \
           $$PWD/qandroidplatformaccessibility.h \
           $$PWD/qandroidplatformfontdatabase.h \
           $$PWD/qandroidplatformclipboard.h \
           $$PWD/qandroidplatformdialoghelpers.h \
           $$PWD/qandroidplatformtheme.h \
           $$PWD/qandroidplatformmenubar.h \
           $$PWD/qandroidplatformmenu.h \
           $$PWD/qandroidplatformmenuitem.h \
           $$PWD/qandroidsystemlocale.h \
           $$PWD/androidsurfaceclient.h \
           $$PWD/qandroidplatformscreen.h \
           $$PWD/qandroidplatformwindow.h \
           $$PWD/qandroidplatformopenglwindow.h \
           $$PWD/qandroidplatformrasterwindow.h \
           $$PWD/qandroidplatformbackingstore.h \
           $$PWD/qandroidplatformopenglcontext.h \
           $$PWD/qandroidplatformforeignwindow.h

#Non-standard install directory, QTBUG-29859
DESTDIR = $$DESTDIR/android
target.path = $${target.path}/android

# Qt accessibility module

contains(QT_CONFIG, accessibility) {
    HEADERS += \
        accessible/qaccessiblewidget.h \
        accessible/qaccessiblewidgetfactory_p.h \
        accessible/complexwidgets.h \
        accessible/itemviews.h \
        accessible/qaccessiblemenu.h \
        accessible/qaccessiblewidgets.h \
        accessible/rangecontrols.h \
        accessible/simplewidgets.h

    SOURCES += \
        accessible/qaccessiblewidget.cpp \
        accessible/qaccessiblewidgetfactory.cpp \
        accessible/complexwidgets.cpp \
        accessible/itemviews.cpp \
        accessible/qaccessiblemenu.cpp \
        accessible/qaccessiblewidgets.cpp \
        accessible/rangecontrols.cpp \
        accessible/simplewidgets.cpp
}

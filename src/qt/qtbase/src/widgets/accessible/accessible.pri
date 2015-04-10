# Qt accessibility module

contains(QT_CONFIG, accessibility) {
    HEADERS += accessible/qaccessiblewidget.h
    SOURCES += accessible/qaccessiblewidget.cpp
}

QT += declarative
INCLUDEPATH += $$PWD

# DEFINES += ACCESSIBILITYINSPECTOR_NO_UITOOLS
# QT += uitools

HEADERS += \
    $$PWD/screenreader.h \
    $$PWD/optionswidget.h \
    $$PWD/accessibilityscenemanager.h \
    $$PWD/accessibilityinspector.h
SOURCES += \
    $$PWD/optionswidget.cpp \
    $$PWD/accessibilityscenemanager.cpp \
    $$PWD/screenreader.cpp \
    $$PWD/accessibilityinspector.cpp




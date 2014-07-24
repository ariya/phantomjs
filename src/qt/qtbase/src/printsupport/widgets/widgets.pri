HEADERS += widgets/qprintpreviewwidget.h
SOURCES += widgets/qprintpreviewwidget.cpp

unix:!mac:contains(QT_CONFIG, cups): {
    HEADERS += widgets/qcupsjobwidget_p.h
    SOURCES += widgets/qcupsjobwidget.cpp
    FORMS += widgets/qcupsjobwidget.ui

    INCLUDEPATH += $$PWD
}


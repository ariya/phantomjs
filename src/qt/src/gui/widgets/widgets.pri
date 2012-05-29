# Qt widgets module

HEADERS += \
        widgets/qbuttongroup.h \
        widgets/qabstractbutton.h \
        widgets/qabstractbutton_p.h \
        widgets/qabstractslider.h \
        widgets/qabstractslider_p.h \
        widgets/qabstractspinbox.h \
        widgets/qabstractspinbox_p.h \
        widgets/qcalendartextnavigator_p.h \
        widgets/qcalendarwidget.h \
        widgets/qcheckbox.h \
        widgets/qcombobox.h \
        widgets/qcombobox_p.h \
        widgets/qcommandlinkbutton.h \
        widgets/qdatetimeedit.h \
        widgets/qdatetimeedit_p.h \
        widgets/qdial.h \
        widgets/qdialogbuttonbox.h \
        widgets/qdockwidget.h \
        widgets/qdockwidget_p.h \
        widgets/qdockarealayout_p.h \
        widgets/qfontcombobox.h \
        widgets/qframe.h \
        widgets/qframe_p.h \
        widgets/qgroupbox.h \
        widgets/qlabel.h \
        widgets/qlabel_p.h \
        widgets/qlcdnumber.h \
        widgets/qlineedit.h \
        widgets/qlineedit_p.h \
        widgets/qlinecontrol_p.h \
        widgets/qmainwindow.h \
        widgets/qmainwindowlayout_p.h \
        widgets/qmdiarea.h \
        widgets/qmdiarea_p.h \
        widgets/qmdisubwindow.h \
        widgets/qmdisubwindow_p.h \
        widgets/qmenu.h \
        widgets/qmenu_p.h \
        widgets/qmenubar.h \
        widgets/qmenubar_p.h \
        widgets/qmenudata.h \
        widgets/qprogressbar.h \
        widgets/qpushbutton.h \
        widgets/qpushbutton_p.h \
        widgets/qradiobutton.h \
        widgets/qrubberband.h \
        widgets/qscrollbar.h \
        widgets/qscrollarea_p.h \
        widgets/qsizegrip.h \
        widgets/qslider.h \
        widgets/qspinbox.h \
        widgets/qsplashscreen.h \
        widgets/qsplitter.h \
        widgets/qsplitter_p.h \
        widgets/qstackedwidget.h \
        widgets/qstatusbar.h \
        widgets/qtabbar.h \
        widgets/qtabbar_p.h \
        widgets/qtabwidget.h \
        widgets/qtextedit.h \
        widgets/qtextedit_p.h \
        widgets/qtextbrowser.h \
        widgets/qtoolbar.h \
        widgets/qtoolbar_p.h \
        widgets/qtoolbarlayout_p.h \
        widgets/qtoolbarextension_p.h \
        widgets/qtoolbarseparator_p.h \
        widgets/qtoolbox.h \
        widgets/qtoolbutton.h \
        widgets/qvalidator.h \
        widgets/qabstractscrollarea.h \
        widgets/qabstractscrollarea_p.h \
        widgets/qwidgetresizehandler_p.h \
        widgets/qfocusframe.h \
        widgets/qscrollarea.h \
        widgets/qworkspace.h \
        widgets/qwidgetanimator_p.h \
        widgets/qtoolbararealayout_p.h \
        widgets/qplaintextedit.h \
        widgets/qplaintextedit_p.h \
        widgets/qprintpreviewwidget.h
SOURCES += \
        widgets/qabstractbutton.cpp \
        widgets/qabstractslider.cpp \
        widgets/qabstractspinbox.cpp \
        widgets/qcalendarwidget.cpp \
        widgets/qcheckbox.cpp \
        widgets/qcombobox.cpp \
        widgets/qcommandlinkbutton.cpp \
        widgets/qdatetimeedit.cpp \
        widgets/qdial.cpp \
        widgets/qdialogbuttonbox.cpp \
        widgets/qdockwidget.cpp \
        widgets/qdockarealayout.cpp \
        widgets/qeffects.cpp \
        widgets/qfontcombobox.cpp \
        widgets/qframe.cpp \
        widgets/qgroupbox.cpp \
        widgets/qlabel.cpp \
        widgets/qlcdnumber.cpp \
        widgets/qlineedit_p.cpp \
        widgets/qlineedit.cpp \
        widgets/qlinecontrol.cpp \
        widgets/qmainwindow.cpp \
        widgets/qmainwindowlayout.cpp \
        widgets/qmdiarea.cpp \
        widgets/qmdisubwindow.cpp \
        widgets/qmenu.cpp \
        widgets/qmenubar.cpp \
        widgets/qmenudata.cpp \
        widgets/qprogressbar.cpp \
        widgets/qpushbutton.cpp \
        widgets/qradiobutton.cpp \
        widgets/qrubberband.cpp \
        widgets/qscrollbar.cpp \
        widgets/qsizegrip.cpp \
        widgets/qslider.cpp \
        widgets/qspinbox.cpp \
        widgets/qsplashscreen.cpp \
        widgets/qsplitter.cpp \
        widgets/qstackedwidget.cpp \
        widgets/qstatusbar.cpp \
        widgets/qtabbar.cpp \
        widgets/qtabwidget.cpp \
        widgets/qtextedit.cpp \
        widgets/qtextbrowser.cpp \
        widgets/qtoolbar.cpp \
        widgets/qtoolbarlayout.cpp \
        widgets/qtoolbarextension.cpp \
        widgets/qtoolbarseparator.cpp \
        widgets/qtoolbox.cpp \
        widgets/qtoolbutton.cpp \
        widgets/qvalidator.cpp \
        widgets/qabstractscrollarea.cpp \
        widgets/qwidgetresizehandler.cpp \
        widgets/qfocusframe.cpp \
        widgets/qscrollarea.cpp \
        widgets/qworkspace.cpp \
        widgets/qwidgetanimator.cpp \
        widgets/qtoolbararealayout.cpp \
        widgets/qplaintextedit.cpp \
        widgets/qprintpreviewwidget.cpp

x11: {
    HEADERS += \
        widgets/qabstractplatformmenubar_p.h

    SOURCES += \
        widgets/qmenubar_x11.cpp
}

!embedded:!qpa:mac {
    HEADERS += widgets/qmacnativewidget_mac.h \
               widgets/qmaccocoaviewcontainer_mac.h
    OBJECTIVE_HEADERS += widgets/qcocoatoolbardelegate_mac_p.h \ 
                         widgets/qcocoamenu_mac_p.h
    OBJECTIVE_SOURCES += widgets/qmenu_mac.mm \
                         widgets/qmaccocoaviewcontainer_mac.mm \
                         widgets/qcocoatoolbardelegate_mac.mm \
                         widgets/qmainwindowlayout_mac.mm \
                         widgets/qmacnativewidget_mac.mm \
                         widgets/qcocoamenu_mac.mm
}

wince*: {
    SOURCES += widgets/qmenu_wince.cpp
    HEADERS += widgets/qmenu_wince_resource_p.h
    RC_FILE = widgets/qmenu_wince.rc
    !static: QMAKE_WRITE_DEFAULT_RC = 1
}

symbian: {
    SOURCES += widgets/qmenu_symbian.cpp
}

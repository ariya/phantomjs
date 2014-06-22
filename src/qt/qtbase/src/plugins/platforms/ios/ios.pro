TARGET = qios

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QIOSIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += core-private gui-private platformsupport-private
LIBS += -framework Foundation -framework UIKit -framework QuartzCore

OBJECTIVE_SOURCES = \
    plugin.mm \
    qiosintegration.mm \
    qioseventdispatcher.mm \
    qioswindow.mm \
    qiosscreen.mm \
    qiosbackingstore.mm \
    qiosapplicationdelegate.mm \
    qiosapplicationstate.mm \
    qiosviewcontroller.mm \
    qioscontext.mm \
    qiosinputcontext.mm \
    qiostheme.mm \
    qiosglobal.mm \
    qiosservices.mm \
    qiosclipboard.mm

HEADERS = \
    qiosintegration.h \
    qioseventdispatcher.h \
    qioswindow.h \
    qiosscreen.h \
    qiosbackingstore.h \
    qiosapplicationdelegate.h \
    qiosapplicationstate.h \
    qiosviewcontroller.h \
    qioscontext.h \
    qiosinputcontext.h \
    qiostheme.h \
    qiosglobal.h \
    qiosservices.h \
    quiview.h \
    qiosclipboard.h

OTHER_FILES = \
    quiview_textinput.mm

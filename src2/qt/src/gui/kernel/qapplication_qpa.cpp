/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qapplication_p.h"
#include "qcolormap.h"
#include "qpixmapcache.h"
#if defined(Q_OS_BLACKBERRY)
#include "qeventdispatcher_blackberry_qpa_p.h"
#else
#if !defined(QT_NO_GLIB)
#include "qeventdispatcher_glib_qpa_p.h"
#endif
#include "qeventdispatcher_qpa_p.h"
#endif
#ifndef QT_NO_CURSOR
#include "private/qcursor_p.h"
#endif

#include "private/qwidget_p.h"
#include "private/qevent_p.h"

#include "qgenericpluginfactory_qpa.h"
#include "qplatformintegrationfactory_qpa_p.h"
#include <qdesktopwidget.h>

#include <qinputcontext.h>
#include <QPlatformCursor>
#include <qdebug.h>
#include <QWindowSystemInterface>
#include "qwindowsysteminterface_qpa_p.h"
#include <QPlatformIntegration>

#include "qdesktopwidget_qpa_p.h"

#include "qminimalintegration.h"

QT_BEGIN_NAMESPACE

static QString appName;
static QString appFont;

QWidget *qt_button_down = 0;                     // widget got last button-down

static bool app_do_modal = false;
extern QWidgetList *qt_modal_stack;              // stack of modal widgets

int qt_last_x = 0;
int qt_last_y = 0;
QPointer<QWidget> qt_last_mouse_receiver = 0;

static Qt::MouseButtons buttons = Qt::NoButton;
static ulong mousePressTime;
static Qt::MouseButton mousePressButton = Qt::NoButton;
static int mousePressX;
static int mousePressY;
static int mouse_double_click_distance = 5;

void QApplicationPrivate::processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e)
{
    switch(e->type) {
    case QWindowSystemInterfacePrivate::Mouse:
        QApplicationPrivate::processMouseEvent(static_cast<QWindowSystemInterfacePrivate::MouseEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Wheel:
        QApplicationPrivate::processWheelEvent(static_cast<QWindowSystemInterfacePrivate::WheelEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Key:
        QApplicationPrivate::processKeyEvent(static_cast<QWindowSystemInterfacePrivate::KeyEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Touch:
        QApplicationPrivate::processTouchEvent(static_cast<QWindowSystemInterfacePrivate::TouchEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::GeometryChange:
        QApplicationPrivate::processGeometryChangeEvent(static_cast<QWindowSystemInterfacePrivate::GeometryChangeEvent*>(e));
        break;
    case QWindowSystemInterfacePrivate::Enter:
        QApplicationPrivate::processEnterEvent(static_cast<QWindowSystemInterfacePrivate::EnterEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Leave:
        QApplicationPrivate::processLeaveEvent(static_cast<QWindowSystemInterfacePrivate::LeaveEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ActivatedWindow:
        QApplicationPrivate::processActivatedEvent(static_cast<QWindowSystemInterfacePrivate::ActivatedWindowEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Close:
        QApplicationPrivate::processCloseEvent(
                static_cast<QWindowSystemInterfacePrivate::CloseEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenCountChange:
        QApplicationPrivate::reportScreenCount(
                static_cast<QWindowSystemInterfacePrivate::ScreenCountEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenGeometry:
        QApplicationPrivate::reportGeometryChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenGeometryEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenAvailableGeometry:
        QApplicationPrivate::reportAvailableGeometryChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::LocaleChange:
        QApplicationPrivate::reportLocaleChange();
        break;
    case QWindowSystemInterfacePrivate::PlatformPanel:
        QApplicationPrivate::processPlatformPanelEvent(
                static_cast<QWindowSystemInterfacePrivate::PlatformPanelEvent *>(e));
        break;
    default:
        qWarning() << "Unknown user input event type:" << e->type;
        break;
    }
}

QString QApplicationPrivate::appName() const
{
    return QT_PREPEND_NAMESPACE(appName);
}

void QApplicationPrivate::createEventDispatcher()
{
    Q_Q(QApplication);
#if defined(Q_OS_BLACKBERRY)
    eventDispatcher = new QEventDispatcherBlackberryQPA(q);
#else
#if !defined(QT_NO_GLIB)
    if (qgetenv("QT_NO_GLIB").isEmpty() && QEventDispatcherGlib::versionSupported())
        eventDispatcher = new QPAEventDispatcherGlib(q);
    else
#endif
    eventDispatcher = new QEventDispatcherQPA(q);
#endif
}

static bool qt_try_modal(QWidget *widget, QEvent::Type type)
{
    QWidget * top = 0;

    if (QApplicationPrivate::tryModalHelper(widget, &top))
        return true;

    bool block_event  = false;
    bool paint_event = false;

    switch (type) {
#if 0
    case QEvent::Focus:
        if (!static_cast<QWSFocusEvent*>(event)->simpleData.get_focus)
            break;
        // drop through
#endif
    case QEvent::MouseButtonPress:                        // disallow mouse/key events
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::PlatformPanel:
        block_event         = true;
        break;
    default:
        break;
    }

    if ((block_event || paint_event) && top->parentWidget() == 0)
        top->raise();

    return !block_event;
}



void QApplicationPrivate::enterModal_sys(QWidget *widget)
{
    if (!qt_modal_stack)
        qt_modal_stack = new QWidgetList;
    qt_modal_stack->insert(0, widget);
    app_do_modal = true;
}

void QApplicationPrivate::leaveModal_sys(QWidget *widget )
{
    if (qt_modal_stack && qt_modal_stack->removeAll(widget)) {
        if (qt_modal_stack->isEmpty()) {
            delete qt_modal_stack;
            qt_modal_stack = 0;
        }
    }
    app_do_modal = qt_modal_stack != 0;
}

bool QApplicationPrivate::modalState()
{
    return app_do_modal;
}

void QApplicationPrivate::closePopup(QWidget *popup)
{
    Q_Q(QApplication);
    if (!popupWidgets)
        return;
    popupWidgets->removeAll(popup);

//###
//     if (popup == qt_popup_down) {
//         qt_button_down = 0;
//         qt_popup_down = 0;
//     }

    if (QApplicationPrivate::popupWidgets->count() == 0) {                // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = 0;

        //### replay mouse event?

        //### transfer/release mouse grab

        //### transfer/release keyboard grab

        //give back focus

        if (active_window) {
            if (QWidget *fw = active_window->focusWidget()) {
                if (fw != QApplication::focusWidget()) {
                    fw->setFocus(Qt::PopupFocusReason);
                } else {
                    QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                    q->sendEvent(fw, &e);
                }
            }
        }

    } else {
        // A popup was closed, so the previous popup gets the focus.

        QWidget* aw = QApplicationPrivate::popupWidgets->last();
        if (QWidget *fw = aw->focusWidget())
            fw->setFocus(Qt::PopupFocusReason);

        //### regrab the keyboard and mouse in case 'popup' lost the grab


    }

}

static int openPopupCount = 0;
void QApplicationPrivate::openPopup(QWidget *popup)
{
    openPopupCount++;
    if (!popupWidgets) {                        // create list
        popupWidgets = new QWidgetList;

        /* only grab if you are the first/parent popup */
        //####   ->grabMouse(popup,true);
        //####   ->grabKeyboard(popup,true);
        //### popupGrabOk = true;
    }
    popupWidgets->append(popup);                // add to end of list

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    if (popup->focusWidget()) {
        popup->focusWidget()->setFocus(Qt::PopupFocusReason);
    } else if (popupWidgets->count() == 1) { // this was the first popup
        if (QWidget *fw = QApplication::focusWidget()) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            QApplication::sendEvent(fw, &e);
        }
    }
}

void QApplicationPrivate::initializeMultitouch_sys()
{
}

void QApplicationPrivate::cleanupMultitouch_sys()
{
}

void QApplicationPrivate::initializeWidgetPaletteHash()
{
}

void QApplication::setCursorFlashTime(int msecs)
{
    QApplicationPrivate::cursor_flash_time = msecs;
}

int QApplication::cursorFlashTime()
{
    return QApplicationPrivate::cursor_flash_time;
}

void QApplication::setDoubleClickInterval(int ms)
{
    QApplicationPrivate::mouse_double_click_time = ms;
}

int QApplication::doubleClickInterval()
{
    return QApplicationPrivate::mouse_double_click_time;
}

void QApplication::setKeyboardInputInterval(int ms)
{
    QApplicationPrivate::keyboard_input_time = ms;
}

int QApplication::keyboardInputInterval()
{
    return QApplicationPrivate::keyboard_input_time;
}

#ifndef QT_NO_WHEELEVENT
void QApplication::setWheelScrollLines(int lines)
{
    QApplicationPrivate::wheel_scroll_lines = lines;
}

int QApplication::wheelScrollLines()
{
    return QApplicationPrivate::wheel_scroll_lines;
}
#endif

void QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
{
    switch (effect) {
    case Qt::UI_AnimateMenu:
        QApplicationPrivate::animate_menu = enable;
        break;
    case Qt::UI_FadeMenu:
        if (enable)
            QApplicationPrivate::animate_menu = true;
        QApplicationPrivate::fade_menu = enable;
        break;
    case Qt::UI_AnimateCombo:
        QApplicationPrivate::animate_combo = enable;
        break;
    case Qt::UI_AnimateTooltip:
        QApplicationPrivate::animate_tooltip = enable;
        break;
    case Qt::UI_FadeTooltip:
        if (enable)
            QApplicationPrivate::animate_tooltip = true;
        QApplicationPrivate::fade_tooltip = enable;
        break;
    case Qt::UI_AnimateToolBox:
        QApplicationPrivate::animate_toolbox = enable;
        break;
    default:
        QApplicationPrivate::animate_ui = enable;
        break;
    }
}

bool QApplication::isEffectEnabled(Qt::UIEffect effect)
{
    if (QColormap::instance().depth() < 16 || !QApplicationPrivate::animate_ui)
        return false;

    switch(effect) {
    case Qt::UI_AnimateMenu:
        return QApplicationPrivate::animate_menu;
    case Qt::UI_FadeMenu:
        return QApplicationPrivate::fade_menu;
    case Qt::UI_AnimateCombo:
        return QApplicationPrivate::animate_combo;
    case Qt::UI_AnimateTooltip:
        return QApplicationPrivate::animate_tooltip;
    case Qt::UI_FadeTooltip:
        return QApplicationPrivate::fade_tooltip;
    case Qt::UI_AnimateToolBox:
        return QApplicationPrivate::animate_toolbox;
    default:
        return QApplicationPrivate::animate_ui;
    }
}

#ifndef QT_NO_CURSOR
void QApplication::setOverrideCursor(const QCursor &cursor)
{
    qApp->d_func()->cursor_list.prepend(cursor);
    qt_qpa_set_cursor(0, false);
}

void QApplication::restoreOverrideCursor()
{
    if (qApp->d_func()->cursor_list.isEmpty())
        return;
    qApp->d_func()->cursor_list.removeFirst();
    qt_qpa_set_cursor(0, false);
}

#endif// QT_NO_CURSOR

QWidget *QApplication::topLevelAt(const QPoint &pos)
{
    QPlatformIntegration *pi = QApplicationPrivate::platformIntegration();

    QList<QPlatformScreen *> screens = pi->screens();
    QList<QPlatformScreen *>::const_iterator screen = screens.constBegin();
    QList<QPlatformScreen *>::const_iterator end = screens.constEnd();

    // The first screen in a virtual environment should know about all top levels
    if (pi->isVirtualDesktop()) {
        QWidget *w = (*screen)->topLevelAt(pos);
        return w;
    }

    while (screen != end) {
        if ((*screen)->geometry().contains(pos))
            return (*screen)->topLevelAt(pos);
        ++screen;
    }
    return 0;
}

void QApplication::beep()
{
}

void QApplication::alert(QWidget *, int)
{
}

/*!
    \internal
*/
QPlatformNativeInterface *QApplication::platformNativeInterface()
{
    QPlatformIntegration *pi = QApplicationPrivate::platformIntegration();
    return pi->nativeInterface();
}

static void init_platform(const QString &name, const QString &platformPluginPath)
{
    QApplicationPrivate::platform_integration = new QMinimalIntegration;
}


static void cleanup_platform()
{
    delete QApplicationPrivate::platform_integration;
    QApplicationPrivate::platform_integration = 0;
}

static void init_plugins(const QList<QByteArray> pluginList)
{
    for (int i = 0; i < pluginList.count(); ++i) {
        QByteArray pluginSpec = pluginList.at(i);
        qDebug() << "init_plugins" << i << pluginSpec;
        int colonPos = pluginSpec.indexOf(':');
        QObject *plugin;
        if (colonPos < 0)
            plugin = QGenericPluginFactory::create(QLatin1String(pluginSpec), QString());
        else
            plugin = QGenericPluginFactory::create(QLatin1String(pluginSpec.mid(0, colonPos)),
                                                   QLatin1String(pluginSpec.mid(colonPos+1)));
        qDebug() << "	created" << plugin;
    }
}

#ifndef QT_NO_QWS_INPUTMETHODS
class QDummyInputContext : public QInputContext
{
public:
    explicit QDummyInputContext(QObject* parent = 0) : QInputContext(parent) {}
    ~QDummyInputContext() {}
    QString identifierName() { return QString(); }
    QString language() { return QString(); }

    void reset() {}
    bool isComposing() const { return false; }

};
#endif // QT_NO_QWS_INPUTMETHODS

void qt_init(QApplicationPrivate *priv, int type)
{
    Q_UNUSED(type);

    qApp->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    char *p;
    char **argv = priv->argv;
    int argc = priv->argc;

    if (argv && *argv) { //apparently, we allow people to pass 0 on the other platforms
        p = strrchr(argv[0], '/');
        appName = QString::fromLocal8Bit(p ? p + 1 : argv[0]);
    }

    QList<QByteArray> pluginList;
    QString platformPluginPath = QLatin1String(qgetenv("QT_QPA_PLATFORM_PLUGIN_PATH"));
    QByteArray platformName;
#ifdef QT_QPA_DEFAULT_PLATFORM_NAME
    platformName = QT_QPA_DEFAULT_PLATFORM_NAME;
#endif
    QByteArray platformNameEnv = qgetenv("QT_QPA_PLATFORM");
    if (!platformNameEnv.isEmpty()) {
        platformName = platformNameEnv;
    }

    // Get command line params

    int j = argc ? 1 : 0;
    for (int i=1; i<argc; i++) {
        if (argv[i] && *argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        QByteArray arg = argv[i];
        if (arg == "-fn" || arg == "-font") {
            if (++i < argc)
                appFont = QString::fromLocal8Bit(argv[i]);
        } else if (arg == "-platformpluginpath") {
            if (++i < argc)
                platformPluginPath = QLatin1String(argv[i]);
        } else if (arg == "-platform") {
            if (++i < argc)
                platformName = argv[i];
        } else if (arg == "-plugin") {
            if (++i < argc)
                pluginList << argv[i];
        } else {
            argv[j++] = argv[i];
        }
    }

    if (j < priv->argc) {
        priv->argv[j] = 0;
        priv->argc = j;
    }

#if 0
    QByteArray pluginEnv = qgetenv("QT_QPA_PLUGINS");
    if (!pluginEnv.isEmpty()) {
        pluginList.append(pluginEnv.split(';'));
    }
#endif

    init_platform(QLatin1String(platformName), platformPluginPath);
    init_plugins(pluginList);

    QColormap::initialize();
    QFont::initialize();
#ifndef QT_NO_CURSOR
//    QCursorData::initialize();
#endif

    qApp->setObjectName(appName);

#ifndef QT_NO_QWS_INPUTMETHODS
        qApp->setInputContext(new QDummyInputContext(qApp));
#endif
}

void qt_cleanup()
{
    cleanup_platform();

    QPixmapCache::clear();
#ifndef QT_NO_CURSOR
    QCursorData::cleanup();
#endif
    QFont::cleanup();
    QColormap::cleanup();
    delete QApplicationPrivate::inputContext;
    QApplicationPrivate::inputContext = 0;

    QApplicationPrivate::active_window = 0; //### this should not be necessary
}


#ifdef QT3_SUPPORT
void QApplication::setMainWidget(QWidget *mainWidget)
{
    QApplicationPrivate::main_widget = mainWidget;
    if (QApplicationPrivate::main_widget && windowIcon().isNull()
        && QApplicationPrivate::main_widget->testAttribute(Qt::WA_SetWindowIcon))
        setWindowIcon(QApplicationPrivate::main_widget->windowIcon());
}
#endif

void QApplicationPrivate::processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e)
{
    static QWeakPointer<QWidget> implicit_mouse_grabber;

    QEvent::Type type;
    // move first
    Qt::MouseButtons stateChange = e->buttons ^ buttons;
    if (e->globalPos != QPoint(qt_last_x, qt_last_y) && (stateChange != Qt::NoButton)) {
        QWindowSystemInterfacePrivate::MouseEvent * newMouseEvent =
                new QWindowSystemInterfacePrivate::MouseEvent(e->widget.data(), e->timestamp, e->localPos, e->globalPos, e->buttons);
        QWindowSystemInterfacePrivate::windowSystemEventQueue.prepend(newMouseEvent); // just in case the move triggers a new event loop
        stateChange = Qt::NoButton;
    }

    QWidget * tlw = e->widget.data();

    QPoint localPoint = e->localPos;
    QPoint globalPoint = e->globalPos;
    QWidget *mouseWindow = tlw;

    Qt::MouseButton button = Qt::NoButton;


    if (qt_last_x != globalPoint.x() || qt_last_y != globalPoint.y()) {
        type = QEvent::MouseMove;
        qt_last_x = globalPoint.x();
        qt_last_y = globalPoint.y();
        if (qAbs(globalPoint.x() - mousePressX) > mouse_double_click_distance||
            qAbs(globalPoint.y() - mousePressY) > mouse_double_click_distance)
            mousePressButton = Qt::NoButton;
    }
    else { // check to see if a new button has been pressed/released
        for (int check = Qt::LeftButton;
             check <= Qt::XButton2;
             check = check << 1) {
            if (check & stateChange) {
                button = Qt::MouseButton(check);
                break;
            }
        }
        if (button == Qt::NoButton) {
            // Ignore mouse events that don't change the current state
            return;
        }
        buttons = e->buttons;
        if (button & e->buttons) {
            if ((e->timestamp - mousePressTime) < static_cast<ulong>(QApplication::doubleClickInterval()) && button == mousePressButton) {
                type = QEvent::MouseButtonDblClick;
                mousePressButton = Qt::NoButton;
            }
            else {
                type = QEvent::MouseButtonPress;
                mousePressTime = e->timestamp;
                mousePressButton = button;
                mousePressX = qt_last_x;
                mousePressY = qt_last_y;
            }
        }
        else
            type = QEvent::MouseButtonRelease;
    }

    if (self->inPopupMode()) {
        //popup mouse handling is magical...
        mouseWindow = qApp->activePopupWidget();

        implicit_mouse_grabber.clear();
        //### how should popup mode and implicit mouse grab interact?

    } else if (tlw && app_do_modal && !qt_try_modal(tlw, QEvent::MouseButtonRelease) ) {
        //even if we're blocked by modality, we should deliver the mouse release event..
        //### this code is not completely correct: multiple buttons can be pressed simultaneously
        if (!(implicit_mouse_grabber && buttons == Qt::NoButton)) {
            //qDebug() << "modal blocked mouse event to" << tlw;
            return;
        }
    }

    // find the tlw if we didn't get it from the plugin
    if (!mouseWindow) {
        mouseWindow = QApplication::topLevelAt(globalPoint);
    }

    if (!mouseWindow && !implicit_mouse_grabber)
        mouseWindow = QApplication::desktop();

    if (mouseWindow && mouseWindow != tlw) {
        //we did not get a sensible localPoint from the window system, so let's calculate it
        localPoint = mouseWindow->mapFromGlobal(globalPoint);
    }

    // which child should have it?
    QWidget *mouseWidget = mouseWindow;
    if (mouseWindow) {
        QWidget *w =  mouseWindow->childAt(localPoint);
        if (w) {
            mouseWidget = w;
        }
    }

    //handle implicit mouse grab
    if (type == QEvent::MouseButtonPress && !implicit_mouse_grabber) {
        implicit_mouse_grabber = mouseWidget;

        Q_ASSERT(mouseWindow);
        mouseWindow->activateWindow(); //focus
    } else if (implicit_mouse_grabber) {
        mouseWidget = implicit_mouse_grabber.data();
        mouseWindow = mouseWidget->window();
        if (mouseWindow != tlw)
            localPoint = mouseWindow->mapFromGlobal(globalPoint);
    }

    Q_ASSERT(mouseWidget);

    //localPoint is local to mouseWindow, but it needs to be local to mouseWidget
    localPoint = mouseWidget->mapFrom(mouseWindow, localPoint);

    if (buttons == Qt::NoButton) {
        //qDebug() << "resetting mouse grabber";
        implicit_mouse_grabber.clear();
    }

    if (mouseWidget != qt_last_mouse_receiver) {
        dispatchEnterLeave(mouseWidget, qt_last_mouse_receiver);
        qt_last_mouse_receiver = mouseWidget;
    }

    // Remember, we might enter a modal event loop when sending the event,
    // so think carefully before adding code below this point.


    QMouseEvent ev(type, localPoint, globalPoint, button, buttons, QApplication::keyboardModifiers());

    QList<QWeakPointer<QPlatformCursor> > cursors = QPlatformCursorPrivate::getInstances();
    foreach (QWeakPointer<QPlatformCursor> cursor, cursors) {
        if (cursor)
            cursor.data()->pointerEvent(ev);
    }

    // qDebug() << "sending mouse event" << ev.type() << localPoint << globalPoint << ev.button() << ev.buttons() << mouseWidget << "mouse grabber" << implicit_mouse_grabber;

    int oldOpenPopupCount = openPopupCount;
    QApplication::sendSpontaneousEvent(mouseWidget, &ev);

#ifndef QT_NO_CONTEXTMENU
    if (type == QEvent::MouseButtonPress && button == Qt::RightButton && (openPopupCount == oldOpenPopupCount)) {
        QContextMenuEvent e(QContextMenuEvent::Mouse, localPoint, globalPoint, QApplication::keyboardModifiers());
        QApplication::sendSpontaneousEvent(mouseWidget, &e);
    }
#endif // QT_NO_CONTEXTMENU
}

void QApplicationPrivate::processPlatformPanelEvent(QWindowSystemInterfacePrivate::PlatformPanelEvent *e)
{
    if (!e->widget)
        return;

    if (app_do_modal && !qt_try_modal(e->widget.data(), QEvent::PlatformPanel)) {
        // a modal window is blocking this window, don't allow events through
        return;
    }

    QEvent ev(QEvent::PlatformPanel);
    QApplication::sendSpontaneousEvent(e->widget.data(), &ev);
}

//### there's a lot of duplicated logic here -- refactoring required!

void QApplicationPrivate::processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e)
{

    if (!e->widget)
        return;

//    QPoint localPoint = ev.pos();
    QPoint globalPoint = e->globalPos;
//    bool trustLocalPoint = !!tlw; //is there something the local point can be local to?
    QWidget *mouseWidget;

    qt_last_x = globalPoint.x();
    qt_last_y = globalPoint.y();

     QWidget *mouseWindow = e->widget.data();

     // find the tlw if we didn't get it from the plugin
     if (!mouseWindow) {
         mouseWindow = QApplication::topLevelAt(globalPoint);
     }

     if (!mouseWindow)
         return;

     mouseWidget = mouseWindow;

     if (app_do_modal && !qt_try_modal(mouseWindow, QEvent::Wheel) ) {
         qDebug() << "modal blocked wheel event" << mouseWindow;
         return;
     }
     QPoint p = mouseWindow->mapFromGlobal(globalPoint);
     QWidget *w = mouseWindow->childAt(p);
     if (w) {
         mouseWidget = w;
         p = mouseWidget->mapFromGlobal(globalPoint);
     }

     QWheelEvent ev(p, globalPoint, e->delta, buttons, QApplication::keyboardModifiers(),
                   e->orient);
     QApplication::sendSpontaneousEvent(mouseWidget, &ev);
}



// Remember, Qt convention is:  keyboard state is state *before*

void QApplicationPrivate::processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e)
{
    QWidget *focusW = 0;
    if (self->inPopupMode()) {
        QWidget *popupW = qApp->activePopupWidget();
        focusW = popupW->focusWidget() ? popupW->focusWidget() : popupW;
    }
    if (!focusW)
        focusW = QApplication::focusWidget();
    if (!focusW) {
        focusW = e->widget.data();
    }
    if (!focusW)
        focusW = QApplication::activeWindow();

    //qDebug() << "handleKeyEvent" << hex << e->key() << e->modifiers() << e->text() << "widget" << focusW;

    if (!focusW)
        return;
    if (app_do_modal && !qt_try_modal(focusW, e->keyType))
        return;

    if (e->nativeScanCode || e->nativeVirtualKey || e->nativeModifiers) {
        QKeyEventEx ev(e->keyType, e->key, e->modifiers, e->unicode, e->repeat, e->repeatCount,
                       e->nativeScanCode, e->nativeVirtualKey, e->nativeModifiers);
        QApplication::sendSpontaneousEvent(focusW, &ev);
    } else {
        QKeyEvent ev(e->keyType, e->key, e->modifiers, e->unicode, e->repeat, e->repeatCount);
        QApplication::sendSpontaneousEvent(focusW, &ev);
    }
}

void QApplicationPrivate::processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e)
{
    if (!e->enter)
        return;

    QApplicationPrivate::dispatchEnterLeave(e->enter.data(),0);
    qt_last_mouse_receiver = e->enter.data();
}

void QApplicationPrivate::processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e)
{
    if (!e->leave)
        return;

    QApplicationPrivate::dispatchEnterLeave(0,qt_last_mouse_receiver);

    if (e->leave.data() && !e->leave.data()->isAncestorOf(qt_last_mouse_receiver)) //(???) this should not happen
        QApplicationPrivate::dispatchEnterLeave(0, e->leave.data());
    qt_last_mouse_receiver = 0;

}

void QApplicationPrivate::processActivatedEvent(QWindowSystemInterfacePrivate::ActivatedWindowEvent *e)
{
    QApplication::setActiveWindow(e->activated.data());
}

void QApplicationPrivate::processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e)
{
    if (e->tlw.isNull())
       return;
    QWidget *tlw = e->tlw.data();
    if (!tlw->isWindow())
        return; //geo of native child widgets is controlled by lighthouse
                //so we already have sent the events; besides this new rect
                //is not mapped to parent

    QRect newRect = e->newGeometry;
    QRect cr(tlw->geometry());
    bool isResize = cr.size() != newRect.size();
    bool isMove = cr.topLeft() != newRect.topLeft();
    tlw->data->crect = newRect;
    if (isResize) {
        QResizeEvent e(tlw->data->crect.size(), cr.size());
        QApplication::sendSpontaneousEvent(tlw, &e);
        tlw->update();
    }

    if (isMove) {
        //### frame geometry
        QMoveEvent e(tlw->data->crect.topLeft(), cr.topLeft());
        QApplication::sendSpontaneousEvent(tlw, &e);
    }
}

void QApplicationPrivate::processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e)
{
    if (e->topLevel.isNull()) {
        //qDebug() << "QApplicationPrivate::processCloseEvent NULL";
        return;
    }
    e->topLevel.data()->d_func()->close_helper(QWidgetPrivate::CloseWithSpontaneousEvent);
}

void QApplicationPrivate::processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *e)
{
    translateRawTouchEvent(e->widget.data(), e->devType, e->points);
}

void QApplicationPrivate::reportScreenCount(QWindowSystemInterfacePrivate::ScreenCountEvent *e)
{
    // This operation only makes sense after the QApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    QApplication::desktop()->d_func()->updateScreenList();
    // signal anything listening for creation or deletion of screens
    QDesktopWidget *desktop = QApplication::desktop();
    emit desktop->screenCountChanged(e->count);
}

void QApplicationPrivate::reportGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *e)
{
    // This operation only makes sense after the QApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    QApplication::desktop()->d_func()->updateScreenList();

    // signal anything listening for screen geometry changes
    QDesktopWidget *desktop = QApplication::desktop();
    emit desktop->resized(e->index);

    // make sure maximized and fullscreen windows are updated
    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = list.size() - 1; i >= 0; --i) {
        QWidget *w = list.at(i);
        if (w->isFullScreen())
            w->d_func()->setFullScreenSize_helper();
        else if (w->isMaximized())
            w->d_func()->setMaxWindowState_helper();
    }
}

void QApplicationPrivate::reportAvailableGeometryChange(
        QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *e)
{
    // This operation only makes sense after the QApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    QApplication::desktop()->d_func()->updateScreenList();

    // signal anything listening for screen geometry changes
    QDesktopWidget *desktop = QApplication::desktop();
    emit desktop->workAreaResized(e->index);

    // make sure maximized and fullscreen windows are updated
    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = list.size() - 1; i >= 0; --i) {
        QWidget *w = list.at(i);
        if (w->isFullScreen())
            w->d_func()->setFullScreenSize_helper();
        else if (w->isMaximized())
            w->d_func()->setMaxWindowState_helper();
    }
}

void QApplicationPrivate::reportLocaleChange()
{
    QApplication::sendSpontaneousEvent( qApp, new QEvent( QEvent::LocaleChange ) );
}

QT_END_NAMESPACE

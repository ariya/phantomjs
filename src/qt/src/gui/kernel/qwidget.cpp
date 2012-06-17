/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qapplication.h"
#include "qapplication_p.h"
#include "qbrush.h"
#include "qcursor.h"
#include "qdesktopwidget.h"
#include "qevent.h"
#include "qhash.h"
#include "qlayout.h"
#include "qmenu.h"
#include "qmetaobject.h"
#include "qpixmap.h"
#include "qpointer.h"
#include "qstack.h"
#include "qstyle.h"
#include "qstylefactory.h"
#include "qvariant.h"
#include "qwidget.h"
#include "qstyleoption.h"
#ifndef QT_NO_ACCESSIBILITY
# include "qaccessible.h"
#endif
#if defined(Q_WS_WIN)
# include "qt_windows.h"
#endif
#ifdef Q_WS_MAC
# include "qt_mac_p.h"
# include "qt_cocoa_helpers_mac_p.h"
# include "qmainwindow.h"
# include "qtoolbar.h"
# include <private/qmainwindowlayout_p.h>
#endif
#if defined(Q_WS_QWS)
# include "qwsdisplay_qws.h"
# include "qwsmanager_qws.h"
# include "qpaintengine.h" // for PorterDuff
# include "private/qwindowsurface_qws_p.h"
#endif
#if defined(Q_WS_QPA)
#include "qplatformwindow_qpa.h"
#endif
#include "qpainter.h"
#include "qtooltip.h"
#include "qwhatsthis.h"
#include "qdebug.h"
#include "private/qstylesheetstyle_p.h"
#include "private/qstyle_p.h"
#include "private/qinputcontext_p.h"
#include "qfileinfo.h"
#include "private/qsoftkeymanager_p.h"

#if defined (Q_WS_WIN)
# include <private/qwininputcontext_p.h>
#endif

#if defined(Q_WS_X11)
# include <private/qpaintengine_x11_p.h>
# include "qx11info_x11.h"
#endif

#include <private/qgraphicseffect_p.h>
#include <private/qwindowsurface_p.h>
#include <private/qbackingstore_p.h>
#ifdef Q_WS_MAC
# include <private/qpaintengine_mac_p.h>
#endif
#include <private/qpaintengine_raster_p.h>

#if defined(Q_OS_SYMBIAN)
#include "private/qt_s60_p.h"
#endif

#include "qwidget_p.h"
#include "qaction_p.h"
#include "qlayout_p.h"
#include "QtGui/qgraphicsproxywidget.h"
#include "QtGui/qgraphicsscene.h"
#include "private/qgraphicsproxywidget_p.h"
#include "QtGui/qabstractscrollarea.h"
#include "private/qabstractscrollarea_p.h"
#include "private/qevent_p.h"

#include "private/qgraphicssystem_p.h"
#include "private/qgesturemanager_p.h"

#ifdef QT_KEYPAD_NAVIGATION
#include "qtabwidget.h" // Needed in inTabWidget()
#endif // QT_KEYPAD_NAVIGATION

#ifdef Q_WS_S60
#include <aknappui.h>
#endif

// widget/widget data creation count
//#define QWIDGET_EXTRA_DEBUG
//#define ALIEN_DEBUG

QT_BEGIN_NAMESPACE

#if !defined(Q_WS_QWS)
static bool qt_enable_backingstore = true;
#endif
#ifdef Q_WS_X11
// for compatibility with Qt 4.0
Q_GUI_EXPORT void qt_x11_set_global_double_buffer(bool enable)
{
    qt_enable_backingstore = enable;
}
#endif

#if defined(QT_MAC_USE_COCOA)
bool qt_mac_clearDirtyOnWidgetInsideDrawWidget = false;
#endif

static inline bool qRectIntersects(const QRect &r1, const QRect &r2)
{
    return (qMax(r1.left(), r2.left()) <= qMin(r1.right(), r2.right()) &&
            qMax(r1.top(), r2.top()) <= qMin(r1.bottom(), r2.bottom()));
}

static inline bool hasBackingStoreSupport()
{
#ifdef Q_WS_MAC
    return QApplicationPrivate::graphicsSystem() != 0;
#else
    return true;
#endif
}

#ifdef Q_WS_MAC
#  define QT_NO_PAINT_DEBUG
#endif

extern bool qt_sendSpontaneousEvent(QObject*, QEvent*); // qapplication.cpp
extern QDesktopWidget *qt_desktopWidget; // qapplication.cpp

/*!
    \internal
    \class QWidgetBackingStoreTracker
    \brief Class which allows tracking of which widgets are using a given backing store

    QWidgetBackingStoreTracker is a thin wrapper around a QWidgetBackingStore pointer,
    which maintains a list of the QWidgets which are currently using the backing
    store.  This list is modified via the registerWidget and unregisterWidget functions.
 */

QWidgetBackingStoreTracker::QWidgetBackingStoreTracker()
    :   m_ptr(0)
{

}

QWidgetBackingStoreTracker::~QWidgetBackingStoreTracker()
{
    delete m_ptr;
}

/*!
    \internal
    Destroy the contained QWidgetBackingStore, if not null, and clear the list of
    widgets using the backing store, then create a new QWidgetBackingStore, providing
    the QWidget.
 */
void QWidgetBackingStoreTracker::create(QWidget *widget)
{
    destroy();
    m_ptr = new QWidgetBackingStore(widget);
}

/*!
    \internal
    Destroy the contained QWidgetBackingStore, if not null, and clear the list of
    widgets using the backing store.
 */
void QWidgetBackingStoreTracker::destroy()
{
    delete m_ptr;
    m_ptr = 0;
    m_widgets.clear();
}

/*!
    \internal
    Add the widget to the list of widgets currently using the backing store.
    If the widget was already in the list, this function is a no-op.
 */
void QWidgetBackingStoreTracker::registerWidget(QWidget *w)
{
    Q_ASSERT(m_ptr);
    Q_ASSERT(w->internalWinId());
    Q_ASSERT(qt_widget_private(w)->maybeBackingStore() == m_ptr);
    m_widgets.insert(w);
}

/*!
    \internal
    Remove the widget from the list of widgets currently using the backing store.
    If the widget was in the list, and removing it causes the list to be empty,
    the backing store is deleted.
    If the widget was not in the list, this function is a no-op.
 */
void QWidgetBackingStoreTracker::unregisterWidget(QWidget *w)
{
    if (m_widgets.remove(w) && m_widgets.isEmpty()) {
        delete m_ptr;
        m_ptr = 0;
    }
}

/*!
    \internal
    Recursively remove widget and all of its descendents.
 */
void QWidgetBackingStoreTracker::unregisterWidgetSubtree(QWidget *widget)
{
    unregisterWidget(widget);
    foreach (QObject *child, widget->children())
        if (QWidget *childWidget = qobject_cast<QWidget *>(child))
            unregisterWidgetSubtree(childWidget);
}

QWidgetPrivate::QWidgetPrivate(int version)
    : QObjectPrivate(version)
      , extra(0)
      , focus_next(0)
      , focus_prev(0)
      , focus_child(0)
      , layout(0)
      , needsFlush(0)
      , redirectDev(0)
      , widgetItem(0)
      , extraPaintEngine(0)
      , polished(0)
      , graphicsEffect(0)
#if !defined(QT_NO_IM)
      , imHints(Qt::ImhNone)
#endif
      , inheritedFontResolveMask(0)
      , inheritedPaletteResolveMask(0)
      , leftmargin(0)
      , topmargin(0)
      , rightmargin(0)
      , bottommargin(0)
      , leftLayoutItemMargin(0)
      , topLayoutItemMargin(0)
      , rightLayoutItemMargin(0)
      , bottomLayoutItemMargin(0)
      , hd(0)
      , size_policy(QSizePolicy::Preferred, QSizePolicy::Preferred)
      , fg_role(QPalette::NoRole)
      , bg_role(QPalette::NoRole)
      , dirtyOpaqueChildren(1)
      , isOpaque(0)
      , inDirtyList(0)
      , isScrolled(0)
      , isMoved(0)
      , isGLWidget(0)
      , usesDoubleBufferedGLContext(0)
#ifndef QT_NO_IM
      , inheritsInputMethodHints(0)
#endif
      , inSetParent(0)
#if defined(Q_WS_X11)
      , picture(0)
#elif defined(Q_WS_WIN)
      , noPaintOnScreen(0)
  #ifndef QT_NO_GESTURES
      , nativeGesturePanEnabled(0)
  #endif
#elif defined(Q_WS_MAC)
      , needWindowChange(0)
      , window_event(0)
      , qd_hd(0)
#elif defined(Q_OS_SYMBIAN)
      , symbianScreenNumber(0)
      , fixNativeOrientationCalled(false)
      , isGLGlobalShareWidget(0)
#endif
{
    if (!qApp) {
        qFatal("QWidget: Must construct a QApplication before a QPaintDevice");
        return;
    }

    if (version != QObjectPrivateVersion)
        qFatal("Cannot mix incompatible Qt libraries");

    isWidget = true;
    memset(high_attributes, 0, sizeof(high_attributes));
#if QT_MAC_USE_COCOA
    drawRectOriginalAdded = false;
    originalDrawMethod = true;
    changeMethods = false;
    isInUnifiedToolbar = false;
    unifiedSurface = 0;
    toolbar_ancestor = 0;
    flushRequested = false;
    touchEventsEnabled = false;
#endif // QT_MAC_USE_COCOA
#ifdef QWIDGET_EXTRA_DEBUG
    static int count = 0;
    qDebug() << "widgets" << ++count;
#endif
}


QWidgetPrivate::~QWidgetPrivate()
{
#ifdef Q_OS_SYMBIAN
    _q_cleanupWinIds();
#endif

    if (widgetItem)
        widgetItem->wid = 0;

    if (extra)
        deleteExtra();

#ifndef QT_NO_GRAPHICSEFFECT
    delete graphicsEffect;
#endif //QT_NO_GRAPHICSEFFECT
}

class QDummyWindowSurface : public QWindowSurface
{
public:
    QDummyWindowSurface(QWidget *window) : QWindowSurface(window) {}
    QPaintDevice *paintDevice() { return window(); }
    void flush(QWidget *, const QRegion &, const QPoint &) {}
};

QWindowSurface *QWidgetPrivate::createDefaultWindowSurface()
{
    Q_Q(QWidget);

    QWindowSurface *surface;
#ifndef QT_NO_PROPERTIES
    if (q->property("_q_DummyWindowSurface").toBool()) {
        surface = new QDummyWindowSurface(q);
    } else
#endif
    {
        if (QApplicationPrivate::graphicsSystem())
            surface = QApplicationPrivate::graphicsSystem()->createWindowSurface(q);
        else
            surface = createDefaultWindowSurface_sys();
    }

    return surface;
}

/*!
    \internal
*/
void QWidgetPrivate::scrollChildren(int dx, int dy)
{
    Q_Q(QWidget);
    if (q->children().size() > 0) {        // scroll children
        QPoint pd(dx, dy);
        QObjectList childObjects = q->children();
        for (int i = 0; i < childObjects.size(); ++i) { // move all children
            QWidget *w = qobject_cast<QWidget*>(childObjects.at(i));
            if (w && !w->isWindow()) {
                QPoint oldp = w->pos();
                QRect  r(w->pos() + pd, w->size());
                w->data->crect = r;
#ifndef Q_WS_QWS
                if (w->testAttribute(Qt::WA_WState_Created))
                    w->d_func()->setWSGeometry();
#endif
                w->d_func()->setDirtyOpaqueRegion();
                QMoveEvent e(r.topLeft(), oldp);
                QApplication::sendEvent(w, &e);
            }
        }
    }
}

QInputContext *QWidgetPrivate::assignedInputContext() const
{
#ifndef QT_NO_IM
    const QWidget *widget = q_func();
    while (widget) {
        if (QInputContext *qic = widget->d_func()->ic)
            return qic;
        widget = widget->parentWidget();
    }
#endif
    return 0;
}

QInputContext *QWidgetPrivate::inputContext() const
{
#ifndef QT_NO_IM
    if (QInputContext *qic = assignedInputContext())
        return qic;
    return qApp->inputContext();
#else
    return 0;
#endif
}

/*!
    This function returns the QInputContext for this widget. By
    default the input context is inherited from the widgets
    parent. For toplevels it is inherited from QApplication.

    You can override this and set a special input context for this
    widget by using the setInputContext() method.

    \sa setInputContext()
*/
QInputContext *QWidget::inputContext()
{
    Q_D(QWidget);
    if (!testAttribute(Qt::WA_InputMethodEnabled))
        return 0;

    return d->inputContext();
}

/*!
  This function sets the input context \a context
  on this widget.

  Qt takes ownership of the given input \a context.

  \sa inputContext()
*/
void QWidget::setInputContext(QInputContext *context)
{
    Q_D(QWidget);
    if (!testAttribute(Qt::WA_InputMethodEnabled))
        return;
#ifndef QT_NO_IM
    if (context == d->ic)
        return;
    if (d->ic)
        delete d->ic;
    d->ic = context;
    if (d->ic)
        d->ic->setParent(this);
#endif
}


/*!
    \obsolete

    This function can be called on the widget that currently has focus
    to reset the input method operating on it.

    This function is providing for convenience, instead you should use
    \l{QInputContext::}{reset()} on the input context that was
    returned by inputContext().

    \sa QInputContext, inputContext(), QInputContext::reset()
*/
void QWidget::resetInputContext()
{
    if (!hasFocus())
        return;
#ifndef QT_NO_IM
    QInputContext *qic = this->inputContext();
    if(qic)
        qic->reset();
#endif // QT_NO_IM
}

#ifdef QT_KEYPAD_NAVIGATION
QPointer<QWidget> QWidgetPrivate::editingWidget;

/*!
    Returns true if this widget currently has edit focus; otherwise false.

    This feature is only available in Qt for Embedded Linux.

    \sa setEditFocus(), QApplication::keypadNavigationEnabled()
*/
bool QWidget::hasEditFocus() const
{
    const QWidget* w = this;
    while (w->d_func()->extra && w->d_func()->extra->focus_proxy)
        w = w->d_func()->extra->focus_proxy;
    return QWidgetPrivate::editingWidget == w;
}

/*!
    \fn void QWidget::setEditFocus(bool enable)

    If \a enable is true, make this widget have edit focus, in which
    case Qt::Key_Up and Qt::Key_Down will be delivered to the widget
    normally; otherwise, Qt::Key_Up and Qt::Key_Down are used to
    change focus.

    This feature is only available in Qt for Embedded Linux and Qt
    for Symbian.

    \sa hasEditFocus(), QApplication::keypadNavigationEnabled()
*/
void QWidget::setEditFocus(bool on)
{
    QWidget *f = this;
    while (f->d_func()->extra && f->d_func()->extra->focus_proxy)
        f = f->d_func()->extra->focus_proxy;

    if (QWidgetPrivate::editingWidget && QWidgetPrivate::editingWidget != f)
        QWidgetPrivate::editingWidget->setEditFocus(false);

    if (on && !f->hasFocus())
        f->setFocus();

    if ((!on && !QWidgetPrivate::editingWidget)
        || (on && QWidgetPrivate::editingWidget == f)) {
        return;
    }

    if (!on && QWidgetPrivate::editingWidget == f) {
        QWidgetPrivate::editingWidget = 0;
        QEvent event(QEvent::LeaveEditFocus);
        QApplication::sendEvent(f, &event);
        QApplication::sendEvent(f->style(), &event);
    } else if (on) {
        QWidgetPrivate::editingWidget = f;
        QEvent event(QEvent::EnterEditFocus);
        QApplication::sendEvent(f, &event);
        QApplication::sendEvent(f->style(), &event);
    }
}
#endif

/*!
    \property QWidget::autoFillBackground
    \brief whether the widget background is filled automatically
    \since 4.1

    If enabled, this property will cause Qt to fill the background of the
    widget before invoking the paint event. The color used is defined by the
    QPalette::Window color role from the widget's \l{QPalette}{palette}.

    In addition, Windows are always filled with QPalette::Window, unless the
    WA_OpaquePaintEvent or WA_NoSystemBackground attributes are set.

    This property cannot be turned off (i.e., set to false) if a widget's
    parent has a static gradient for its background.

    \warning Use this property with caution in conjunction with
    \l{Qt Style Sheets}. When a widget has a style sheet with a valid
    background or a border-image, this property is automatically disabled.

    By default, this property is false.

    \sa Qt::WA_OpaquePaintEvent, Qt::WA_NoSystemBackground,
    {QWidget#Transparency and Double Buffering}{Transparency and Double Buffering}
*/
bool QWidget::autoFillBackground() const
{
    Q_D(const QWidget);
    return d->extra && d->extra->autoFillBackground;
}

void QWidget::setAutoFillBackground(bool enabled)
{
    Q_D(QWidget);
    if (!d->extra)
        d->createExtra();
    if (d->extra->autoFillBackground == enabled)
        return;

    d->extra->autoFillBackground = enabled;
    d->updateIsOpaque();
    update();
    d->updateIsOpaque();
}

/*!
    \class QWidget
    \brief The QWidget class is the base class of all user interface objects.

    \ingroup basicwidgets

    The widget is the atom of the user interface: it receives mouse, keyboard
    and other events from the window system, and paints a representation of
    itself on the screen. Every widget is rectangular, and they are sorted in a
    Z-order. A widget is clipped by its parent and by the widgets in front of
    it.

    A widget that is not embedded in a parent widget is called a window.
    Usually, windows have a frame and a title bar, although it is also possible
    to create windows without such decoration using suitable
    \l{Qt::WindowFlags}{window flags}). In Qt, QMainWindow and the various
    subclasses of QDialog are the most common window types.

    Every widget's constructor accepts one or two standard arguments:

    \list 1
        \i  \c{QWidget *parent = 0} is the parent of the new widget. If it is 0
            (the default), the new widget will be a window. If not, it will be
            a child of \e parent, and be constrained by \e parent's geometry
            (unless you specify Qt::Window as window flag).
        \i  \c{Qt::WindowFlags f = 0} (where available) sets the window flags;
            the default is suitable for almost all widgets, but to get, for
            example, a window without a window system frame, you must use
            special flags.
    \endlist

    QWidget has many member functions, but some of them have little direct
    functionality; for example, QWidget has a font property, but never uses
    this itself. There are many subclasses which provide real functionality,
    such as QLabel, QPushButton, QListWidget, and QTabWidget.


    \section1 Top-Level and Child Widgets

    A widget without a parent widget is always an independent window (top-level
    widget). For these widgets, setWindowTitle() and setWindowIcon() set the
    title bar and icon respectively.

    Non-window widgets are child widgets, displayed within their parent
    widgets. Most widgets in Qt are mainly useful as child widgets. For
    example, it is possible to display a button as a top-level window, but most
    people prefer to put their buttons inside other widgets, such as QDialog.

    \image parent-child-widgets.png A parent widget containing various child widgets.

    The diagram above shows a QGroupBox widget being used to hold various child
    widgets in a layout provided by QGridLayout. The QLabel child widgets have
    been outlined to indicate their full sizes.

    If you want to use a QWidget to hold child widgets you will usually want to
    add a layout to the parent QWidget. See \l{Layout Management} for more
    information.


    \section1 Composite Widgets

    When a widget is used as a container to group a number of child widgets, it
    is known as a composite widget. These can be created by constructing a
    widget with the required visual properties - a QFrame, for example - and
    adding child widgets to it, usually managed by a layout. The above diagram
    shows such a composite widget that was created using \l{Qt Designer}.

    Composite widgets can also be created by subclassing a standard widget,
    such as QWidget or QFrame, and adding the necessary layout and child
    widgets in the constructor of the subclass. Many of the \l{Qt Examples}
    {examples provided with Qt} use this approach, and it is also covered in
    the Qt \l{Tutorials}.


    \section1 Custom Widgets and Painting

    Since QWidget is a subclass of QPaintDevice, subclasses can be used to
    display custom content that is composed using a series of painting
    operations with an instance of the QPainter class. This approach contrasts
    with the canvas-style approach used by the \l{Graphics View}
    {Graphics View Framework} where items are added to a scene by the
    application and are rendered by the framework itself.

    Each widget performs all painting operations from within its paintEvent()
    function. This is called whenever the widget needs to be redrawn, either
    as a result of some external change or when requested by the application.

    The \l{widgets/analogclock}{Analog Clock example} shows how a simple widget
    can handle paint events.


    \section1 Size Hints and Size Policies

    When implementing a new widget, it is almost always useful to reimplement
    sizeHint() to provide a reasonable default size for the widget and to set
    the correct size policy with setSizePolicy().

    By default, composite widgets which do not provide a size hint will be
    sized according to the space requirements of their child widgets.

    The size policy lets you supply good default behavior for the layout
    management system, so that other widgets can contain and manage yours
    easily. The default size policy indicates that the size hint represents
    the preferred size of the widget, and this is often good enough for many
    widgets.

    \note The size of top-level widgets are constrained to 2/3 of the desktop's
    height and width. You can resize() the widget manually if these bounds are
    inadequate.


    \section1 Events

    Widgets respond to events that are typically caused by user actions. Qt
    delivers events to widgets by calling specific event handler functions with
    instances of QEvent subclasses containing information about each event.

    If your widget only contains child widgets, you probably do not need to
    implement any event handlers. If you want to detect a mouse click in a
    child widget call the child's underMouse() function inside the widget's
    mousePressEvent().

    The \l{widgets/scribble}{Scribble example} implements a wider set of
    events to handle mouse movement, button presses, and window resizing.

    You will need to supply the behavior and content for your own widgets, but
    here is a brief overview of the events that are relevant to QWidget,
    starting with the most common ones:

    \list
        \i  paintEvent() is called whenever the widget needs to be repainted.
            Every widget displaying custom content must implement it. Painting
            using a QPainter can only take place in a paintEvent() or a
            function called by a paintEvent().
        \i  resizeEvent() is called when the widget has been resized.
        \i  mousePressEvent() is called when a mouse button is pressed while
            the mouse cursor is inside the widget, or when the widget has
            grabbed the mouse using grabMouse(). Pressing the mouse without
            releasing it is effectively the same as calling grabMouse().
        \i  mouseReleaseEvent() is called when a mouse button is released. A
            widget receives mouse release events when it has received the
            corresponding mouse press event. This means that if the user
            presses the mouse inside \e your widget, then drags the mouse
            somewhere else before releasing the mouse button, \e your widget
            receives the release event. There is one exception: if a popup menu
            appears while the mouse button is held down, this popup immediately
            steals the mouse events.
        \i  mouseDoubleClickEvent() is called when the user double-clicks in
            the widget. If the user double-clicks, the widget receives a mouse
            press event, a mouse release event and finally this event instead
            of a second mouse press event. (Some mouse move events may also be
            received if the mouse is not held steady during this operation.) It
            is \e{not possible} to distinguish a click from a double-click
            until the second click arrives. (This is one reason why most GUI
            books recommend that double-clicks be an extension of
            single-clicks, rather than trigger a different action.)
    \endlist

    Widgets that accept keyboard input need to reimplement a few more event
    handlers:

    \list
        \i  keyPressEvent() is called whenever a key is pressed, and again when
            a key has been held down long enough for it to auto-repeat. The
            \key Tab and \key Shift+Tab keys are only passed to the widget if
            they are not used by the focus-change mechanisms. To force those
            keys to be processed by your widget, you must reimplement
            QWidget::event().
        \i  focusInEvent() is called when the widget gains keyboard focus
            (assuming you have called setFocusPolicy()). Well-behaved widgets
            indicate that they own the keyboard focus in a clear but discreet
            way.
        \i  focusOutEvent() is called when the widget loses keyboard focus.
    \endlist

    You may be required to also reimplement some of the less common event
    handlers:

    \list
        \i  mouseMoveEvent() is called whenever the mouse moves while a mouse
            button is held down. This can be useful during drag and drop
            operations. If you call \l{setMouseTracking()}{setMouseTracking}(true),
            you get mouse move events even when no buttons are held down.
            (See also the \l{Drag and Drop} guide.)
        \i  keyReleaseEvent() is called whenever a key is released and while it
            is held down (if the key is auto-repeating). In that case, the
            widget will receive a pair of key release and key press event for
            every repeat. The \key Tab and \key Shift+Tab keys are only passed
            to the widget if they are not used by the focus-change mechanisms.
            To force those keys to be processed by your widget, you must
            reimplement QWidget::event().
        \i  wheelEvent() is called whenever the user turns the mouse wheel
            while the widget has the focus.
        \i  enterEvent() is called when the mouse enters the widget's screen
            space. (This excludes screen space owned by any of the widget's
            children.)
        \i  leaveEvent() is called when the mouse leaves the widget's screen
            space. If the mouse enters a child widget it will not cause a
            leaveEvent().
        \i  moveEvent() is called when the widget has been moved relative to
            its parent.
        \i  closeEvent() is called when the user closes the widget (or when
            close() is called).
    \endlist

    There are also some rather obscure events described in the documentation
    for QEvent::Type. To handle these events, you need to reimplement event()
    directly.

    The default implementation of event() handles \key Tab and \key Shift+Tab
    (to move the keyboard focus), and passes on most of the other events to
    one of the more specialized handlers above.

    Events and the mechanism used to deliver them are covered in 
    \l{The Event System}.

    \section1 Groups of Functions and Properties

    \table
    \header \i Context \i Functions and Properties

    \row \i Window functions \i
        show(),
        hide(),
        raise(),
        lower(),
        close().

    \row \i Top-level windows \i
        \l windowModified, \l windowTitle, \l windowIcon, \l windowIconText,
        \l isActiveWindow, activateWindow(), \l minimized, showMinimized(),
        \l maximized, showMaximized(), \l fullScreen, showFullScreen(),
        showNormal().

    \row \i Window contents \i
        update(),
        repaint(),
        scroll().

    \row \i Geometry \i
        \l pos, x(), y(), \l rect, \l size, width(), height(), move(), resize(),
        \l sizePolicy, sizeHint(), minimumSizeHint(),
        updateGeometry(), layout(),
        \l frameGeometry, \l geometry, \l childrenRect, \l childrenRegion,
        adjustSize(),
        mapFromGlobal(), mapToGlobal(),
        mapFromParent(), mapToParent(),
        \l maximumSize, \l minimumSize, \l sizeIncrement,
        \l baseSize, setFixedSize()

    \row \i Mode \i
        \l visible, isVisibleTo(),
        \l enabled, isEnabledTo(),
        \l modal,
        isWindow(),
        \l mouseTracking,
        \l updatesEnabled,
        visibleRegion().

    \row \i Look and feel \i
        style(),
        setStyle(),
        \l styleSheet,
        \l cursor,
        \l font,
        \l palette,
        backgroundRole(), setBackgroundRole(),
        fontInfo(), fontMetrics().

    \row \i Keyboard focus functions \i
        \l focus, \l focusPolicy,
        setFocus(), clearFocus(), setTabOrder(), setFocusProxy(),
        focusNextChild(), focusPreviousChild().

    \row \i Mouse and keyboard grabbing \i
        grabMouse(), releaseMouse(),
        grabKeyboard(), releaseKeyboard(),
        mouseGrabber(), keyboardGrabber().

    \row \i Event handlers \i
        event(),
        mousePressEvent(),
        mouseReleaseEvent(),
        mouseDoubleClickEvent(),
        mouseMoveEvent(),
        keyPressEvent(),
        keyReleaseEvent(),
        focusInEvent(),
        focusOutEvent(),
        wheelEvent(),
        enterEvent(),
        leaveEvent(),
        paintEvent(),
        moveEvent(),
        resizeEvent(),
        closeEvent(),
        dragEnterEvent(),
        dragMoveEvent(),
        dragLeaveEvent(),
        dropEvent(),
        childEvent(),
        showEvent(),
        hideEvent(),
        customEvent().
        changeEvent(),

    \row \i System functions \i
        parentWidget(), window(), setParent(), winId(),
        find(), metric().

    \row \i Interactive help \i
        setToolTip(), setWhatsThis()

    \endtable


    \section1 Widget Style Sheets

    In addition to the standard widget styles for each platform, widgets can
    also be styled according to rules specified in a \l{styleSheet}
    {style sheet}. This feature enables you to customize the appearance of
    specific widgets to provide visual cues to users about their purpose. For
    example, a button could be styled in a particular way to indicate that it
    performs a destructive action.

    The use of widget style sheets is described in more detail in the
    \l{Qt Style Sheets} document.


    \section1 Transparency and Double Buffering

    Since Qt 4.0, QWidget automatically double-buffers its painting, so there
    is no need to write double-buffering code in paintEvent() to avoid
    flicker.

    Since Qt 4.1, the Qt::WA_ContentsPropagated widget attribute has been
    deprecated. Instead, the contents of parent widgets are propagated by
    default to each of their children as long as Qt::WA_PaintOnScreen is not
    set. Custom widgets can be written to take advantage of this feature by
    updating irregular regions (to create non-rectangular child widgets), or
    painting with colors that have less than full alpha component. The
    following diagram shows how attributes and properties of a custom widget
    can be fine-tuned to achieve different effects.

    \image propagation-custom.png

    In the above diagram, a semi-transparent rectangular child widget with an
    area removed is constructed and added to a parent widget (a QLabel showing
    a pixmap). Then, different properties and widget attributes are set to
    achieve different effects:

    \list
        \i  The left widget has no additional properties or widget attributes
            set. This default state suits most custom widgets using
            transparency, are irregularly-shaped, or do not paint over their
            entire area with an opaque brush.
        \i  The center widget has the \l autoFillBackground property set. This
            property is used with custom widgets that rely on the widget to
            supply a default background, and do not paint over their entire
            area with an opaque brush.
        \i  The right widget has the Qt::WA_OpaquePaintEvent widget attribute
            set. This indicates that the widget will paint over its entire area
            with opaque colors. The widget's area will initially be
            \e{uninitialized}, represented in the diagram with a red diagonal
            grid pattern that shines through the overpainted area. The
            Qt::WA_OpaquePaintArea attribute is useful for widgets that need to
            paint their own specialized contents quickly and do not need a
            default filled background.
    \endlist

    To rapidly update custom widgets with simple background colors, such as
    real-time plotting or graphing widgets, it is better to define a suitable
    background color (using setBackgroundRole() with the
    QPalette::Window role), set the \l autoFillBackground property, and only
    implement the necessary drawing functionality in the widget's paintEvent().

    To rapidly update custom widgets that constantly paint over their entire
    areas with opaque content, e.g., video streaming widgets, it is better to
    set the widget's Qt::WA_OpaquePaintEvent, avoiding any unnecessary overhead
    associated with repainting the widget's background.

    If a widget has both the Qt::WA_OpaquePaintEvent widget attribute \e{and}
    the \l autoFillBackground property set, the Qt::WA_OpaquePaintEvent
    attribute takes precedence. Depending on your requirements, you should
    choose either one of them.

    Since Qt 4.1, the contents of parent widgets are also propagated to
    standard Qt widgets. This can lead to some unexpected results if the
    parent widget is decorated in a non-standard way, as shown in the diagram
    below.

    \image propagation-standard.png

    The scope for customizing the painting behavior of standard Qt widgets,
    without resorting to subclassing, is slightly less than that possible for
    custom widgets. Usually, the desired appearance of a standard widget can be
    achieved by setting its \l autoFillBackground property.


    \section1 Creating Translucent Windows

    Since Qt 4.5, it has been possible to create windows with translucent regions
    on window systems that support compositing.

    To enable this feature in a top-level widget, set its Qt::WA_TranslucentBackground
    attribute with setAttribute() and ensure that its background is painted with
    non-opaque colors in the regions you want to be partially transparent.

    Platform notes:

    \list
    \o X11: This feature relies on the use of an X server that supports ARGB visuals
    and a compositing window manager.
    \o Windows: The widget needs to have the Qt::FramelessWindowHint window flag set
    for the translucency to work.
    \endlist


    \section1 Native Widgets vs Alien Widgets

    Introduced in Qt 4.4, alien widgets are widgets unknown to the windowing
    system. They do not have a native window handle associated with them. This
    feature significantly speeds up widget painting, resizing, and removes flicker.

    Should you require the old behavior with native windows, you can choose
    one of the following options:

    \list 1
        \i  Use the \c{QT_USE_NATIVE_WINDOWS=1} in your environment.
        \i  Set the Qt::AA_NativeWindows attribute on your application. All
            widgets will be native widgets.
        \i  Set the Qt::WA_NativeWindow attribute on widgets: The widget itself
            and all of its ancestors will become native (unless
            Qt::WA_DontCreateNativeAncestors is set).
        \i  Call QWidget::winId to enforce a native window (this implies 3).
        \i  Set the Qt::WA_PaintOnScreen attribute to enforce a native window
            (this implies 3).
    \endlist

    \sa QEvent, QPainter, QGridLayout, QBoxLayout

    \section1 Softkeys

    Since Qt 4.6, Softkeys are usually physical keys on a device that have a corresponding label or
    other visual representation on the screen that is generally located next to its
    physical counterpart. They are most often found on mobile phone platforms. In
    modern touch based user interfaces it is also possible to have softkeys that do
    not correspond to any physical keys. Softkeys differ from other onscreen labels
    in that they are contextual.

    In Qt, contextual softkeys are added to a widget by calling addAction() and
    passing a \c QAction with a softkey role set on it. When the widget
    containing the softkey actions has focus, its softkeys should appear in
    the user interface. Softkeys are discovered by traversing the widget
    hierarchy so it is possible to define a single set of softkeys that are
    present at all times by calling addAction() for a given top level widget.

    On some platforms, this concept overlaps with \c QMenuBar such that if no
    other softkeys are found and the top level widget is a QMainWindow containing
    a QMenuBar, the menubar actions may appear on one of the softkeys.

    Note: Currently softkeys are only supported on the Symbian Platform.

    \sa addAction(), QAction, QMenuBar

*/

QWidgetMapper *QWidgetPrivate::mapper = 0;          // widget with wid
QWidgetSet *QWidgetPrivate::allWidgets = 0;         // widgets with no wid


/*****************************************************************************
  QWidget utility functions
 *****************************************************************************/

QRegion qt_dirtyRegion(QWidget *widget)
{
    if (!widget)
        return QRegion();

    QWidgetBackingStore *bs = qt_widget_private(widget)->maybeBackingStore();
    if (!bs)
        return QRegion();

    return bs->dirtyRegion(widget);
}

/*****************************************************************************
  QWidget member functions
 *****************************************************************************/

/*
    Widget state flags:
  \list
  \i Qt::WA_WState_Created The widget has a valid winId().
  \i Qt::WA_WState_Visible The widget is currently visible.
  \i Qt::WA_WState_Hidden The widget is hidden, i.e. it won't
  become visible unless you call show() on it. Qt::WA_WState_Hidden
  implies !Qt::WA_WState_Visible.
  \i Qt::WA_WState_CompressKeys Compress keyboard events.
  \i Qt::WA_WState_BlockUpdates Repaints and updates are disabled.
  \i Qt::WA_WState_InPaintEvent Currently processing a paint event.
  \i Qt::WA_WState_Reparented The widget has been reparented.
  \i Qt::WA_WState_ConfigPending A configuration (resize/move) event is pending.
  \i Qt::WA_WState_DND (Deprecated) The widget supports drag and drop, see setAcceptDrops().
  \endlist
*/

struct QWidgetExceptionCleaner
{
    /* this cleans up when the constructor throws an exception */
    static inline void cleanup(QWidget *that, QWidgetPrivate *d)
    {
#ifdef QT_NO_EXCEPTIONS
        Q_UNUSED(that);
        Q_UNUSED(d);
#else
        QWidgetPrivate::allWidgets->remove(that);
        if (d->focus_next != that) {
            if (d->focus_next)
                d->focus_next->d_func()->focus_prev = d->focus_prev;
            if (d->focus_prev)
                d->focus_prev->d_func()->focus_next = d->focus_next;
        }
#endif
    }
};

/*!
    Constructs a widget which is a child of \a parent, with  widget
    flags set to \a f.

    If \a parent is 0, the new widget becomes a window. If
    \a parent is another widget, this widget becomes a child window
    inside \a parent. The new widget is deleted when its \a parent is
    deleted.

    The widget flags argument, \a f, is normally 0, but it can be set
    to customize the frame of a window (i.e. \a
    parent must be 0). To customize the frame, use a value composed
    from the bitwise OR of any of the \l{Qt::WindowFlags}{window flags}.

    If you add a child widget to an already visible widget you must
    explicitly show the child to make it visible.

    Note that the X11 version of Qt may not be able to deliver all
    combinations of style flags on all systems. This is because on
    X11, Qt can only ask the window manager, and the window manager
    can override the application's settings. On Windows, Qt can set
    whatever flags you want.

    \sa windowFlags
*/
QWidget::QWidget(QWidget *parent, Qt::WindowFlags f)
    : QObject(*new QWidgetPrivate, 0), QPaintDevice()
{
    QT_TRY {
        d_func()->init(parent, f);
    } QT_CATCH(...) {
        QWidgetExceptionCleaner::cleanup(this, d_func());
        QT_RETHROW;
    }
}

#ifdef QT3_SUPPORT
/*!
    \overload
    \obsolete
 */
QWidget::QWidget(QWidget *parent, const char *name, Qt::WindowFlags f)
    : QObject(*new QWidgetPrivate, 0), QPaintDevice()
{
    QT_TRY {
        d_func()->init(parent , f);
        setObjectName(QString::fromAscii(name));
    } QT_CATCH(...) {
        QWidgetExceptionCleaner::cleanup(this, d_func());
        QT_RETHROW;
    }
}
#endif

/*! \internal
*/
QWidget::QWidget(QWidgetPrivate &dd, QWidget* parent, Qt::WindowFlags f)
    : QObject(dd, 0), QPaintDevice()
{
    Q_D(QWidget);
    QT_TRY {
        d->init(parent, f);
    } QT_CATCH(...) {
        QWidgetExceptionCleaner::cleanup(this, d_func());
        QT_RETHROW;
    }
}

/*!
    \internal
*/
int QWidget::devType() const
{
    return QInternal::Widget;
}


//### w is a "this" ptr, passed as a param because QWorkspace needs special logic
void QWidgetPrivate::adjustFlags(Qt::WindowFlags &flags, QWidget *w)
{
    bool customize =  (flags & (Qt::CustomizeWindowHint
            | Qt::FramelessWindowHint
            | Qt::WindowTitleHint
            | Qt::WindowSystemMenuHint
            | Qt::WindowMinimizeButtonHint
            | Qt::WindowMaximizeButtonHint
            | Qt::WindowCloseButtonHint
            | Qt::WindowContextHelpButtonHint));

    uint type = (flags & Qt::WindowType_Mask);

    if ((type == Qt::Widget || type == Qt::SubWindow) && w && !w->parent()) {
        type = Qt::Window;
        flags |= Qt::Window;
    }

    if (flags & Qt::CustomizeWindowHint) {
        // modify window flags to make them consistent.
        // Only enable this on non-Mac platforms. Since the old way of doing this would
        // interpret WindowSystemMenuHint as a close button and we can't change that behavior
        // we can't just add this in.
#ifndef Q_WS_MAC
        if (flags & (Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::WindowContextHelpButtonHint)) {
            flags |= Qt::WindowSystemMenuHint;
#else
        if (flags & (Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint
                     | Qt::WindowSystemMenuHint)) {
#endif
            flags |= Qt::WindowTitleHint;
            flags &= ~Qt::FramelessWindowHint;
        }
    } else if (customize && !(flags & Qt::FramelessWindowHint)) {
        // if any of the window hints that affect the titlebar are set
        // and the window is supposed to have frame, we add a titlebar
        // and system menu by default.
        flags |= Qt::WindowSystemMenuHint;
        flags |= Qt::WindowTitleHint;
    }
    if (customize)
        ; // don't modify window flags if the user explicitly set them.
    else if (type == Qt::Dialog || type == Qt::Sheet)
#ifndef Q_WS_WINCE
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint;
#else
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
#endif
    else if (type == Qt::Tool)
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
    else
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint;


}

void QWidgetPrivate::init(QWidget *parentWidget, Qt::WindowFlags f)
{
    Q_Q(QWidget);
    if (QApplication::type() == QApplication::Tty)
        qFatal("QWidget: Cannot create a QWidget when no GUI is being used");

    Q_ASSERT(allWidgets);
    if (allWidgets)
        allWidgets->insert(q);

    QWidget *desktopWidget = 0;
    if (parentWidget && parentWidget->windowType() == Qt::Desktop) {
        desktopWidget = parentWidget;
        parentWidget = 0;
    }

    q->data = &data;

#ifndef QT_NO_THREAD
    if (!parent) {
        Q_ASSERT_X(q->thread() == qApp->thread(), "QWidget",
                   "Widgets must be created in the GUI thread.");
    }
#endif

#if defined(Q_WS_X11)
    if (desktopWidget) {
        // make sure the widget is created on the same screen as the
        // programmer specified desktop widget
        xinfo = desktopWidget->d_func()->xinfo;
    }
#elif defined(Q_OS_SYMBIAN)
    if (desktopWidget) {
        symbianScreenNumber = qt_widget_private(desktopWidget)->symbianScreenNumber;
    }
#elif defined(Q_WS_QPA)
    if (desktopWidget) {
        int screen = desktopWidget->d_func()->topData()->screenIndex;
        topData()->screenIndex = screen;
        QPlatformIntegration *platform = QApplicationPrivate::platformIntegration();
        platform->moveToScreen(q, screen);
    }
#else
    Q_UNUSED(desktopWidget);
#endif

    data.fstrut_dirty = true;

    data.winid = 0;
    data.widget_attributes = 0;
    data.window_flags = f;
    data.window_state = 0;
    data.focus_policy = 0;
    data.context_menu_policy = Qt::DefaultContextMenu;
    data.window_modality = Qt::NonModal;

    data.sizehint_forced = 0;
    data.is_closing = 0;
    data.in_show = 0;
    data.in_set_window_state = 0;
    data.in_destructor = false;

    // Widgets with Qt::MSWindowsOwnDC (typically QGLWidget) must have a window handle.
    if (f & Qt::MSWindowsOwnDC)
        q->setAttribute(Qt::WA_NativeWindow);

//#ifdef Q_WS_MAC
//    q->setAttribute(Qt::WA_NativeWindow);
//#endif

    q->setAttribute(Qt::WA_QuitOnClose); // might be cleared in adjustQuitOnCloseAttribute()
    adjustQuitOnCloseAttribute();

    q->setAttribute(Qt::WA_WState_Hidden);

    //give potential windows a bigger "pre-initial" size; create_sys() will give them a new size later
#ifdef Q_OS_SYMBIAN
    if (isGLWidget) {
        // Don't waste GPU mem for unnecessary large egl surface until resized by application
        data.crect = QRect(0,0,1,1);
    } else {
        data.crect = parentWidget ? QRect(0,0,100,30) : QRect(0,0,360,640);
    }
#else
    data.crect = parentWidget ? QRect(0,0,100,30) : QRect(0,0,640,480);
#endif

    focus_next = focus_prev = q;

    if ((f & Qt::WindowType_Mask) == Qt::Desktop)
        q->create();
    else if (parentWidget)
        q->setParent(parentWidget, data.window_flags);
    else {
        adjustFlags(data.window_flags, q);
        resolveLayoutDirection();
        // opaque system background?
        const QBrush &background = q->palette().brush(QPalette::Window);
        setOpaque(q->isWindow() && background.style() != Qt::NoBrush && background.isOpaque());
    }
    data.fnt = QFont(data.fnt, q);
#if defined(Q_WS_X11)
    data.fnt.x11SetScreen(xinfo.screen());
#endif // Q_WS_X11

    q->setAttribute(Qt::WA_PendingMoveEvent);
    q->setAttribute(Qt::WA_PendingResizeEvent);

    if (++QWidgetPrivate::instanceCounter > QWidgetPrivate::maxInstances)
        QWidgetPrivate::maxInstances = QWidgetPrivate::instanceCounter;

    if (QApplicationPrivate::app_compile_version < 0x040200
        || QApplicationPrivate::testAttribute(Qt::AA_ImmediateWidgetCreation))
        q->create();


    QEvent e(QEvent::Create);
    QApplication::sendEvent(q, &e);
    QApplication::postEvent(q, new QEvent(QEvent::PolishRequest));

    extraPaintEngine = 0;
}



void QWidgetPrivate::createRecursively()
{
    Q_Q(QWidget);
    q->create(0, true, true);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (child && !child->isHidden() && !child->isWindow() && !child->testAttribute(Qt::WA_WState_Created))
            child->d_func()->createRecursively();
    }
}




/*!
    Creates a new widget window if \a window is 0, otherwise sets the
    widget's window to \a window.

    Initializes the window (sets the geometry etc.) if \a
    initializeWindow is true. If \a initializeWindow is false, no
    initialization is performed. This parameter only makes sense if \a
    window is a valid window.

    Destroys the old window if \a destroyOldWindow is true. If \a
    destroyOldWindow is false, you are responsible for destroying the
    window yourself (using platform native code).

    The QWidget constructor calls create(0,true,true) to create a
    window for this widget.
*/

void QWidget::create(WId window, bool initializeWindow, bool destroyOldWindow)
{
    Q_D(QWidget);
    if (testAttribute(Qt::WA_WState_Created) && window == 0 && internalWinId())
        return;

    if (d->data.in_destructor)
        return;

    Qt::WindowType type = windowType();
    Qt::WindowFlags &flags = data->window_flags;

    if ((type == Qt::Widget || type == Qt::SubWindow) && !parentWidget()) {
        type = Qt::Window;
        flags |= Qt::Window;
    }

#ifndef Q_WS_QPA
    if (QWidget *parent = parentWidget()) {
        if (type & Qt::Window) {
            if (!parent->testAttribute(Qt::WA_WState_Created))
                parent->createWinId();
        } else if (testAttribute(Qt::WA_NativeWindow) && !parent->internalWinId()
                   && !testAttribute(Qt::WA_DontCreateNativeAncestors)) {
            // We're about to create a native child widget that doesn't have a native parent;
            // enforce a native handle for the parent unless the Qt::WA_DontCreateNativeAncestors
            // attribute is set.
            d->createWinId(window);
            // Nothing more to do.
            Q_ASSERT(testAttribute(Qt::WA_WState_Created));
            Q_ASSERT(internalWinId());
            return;
        }
    }
#endif //Q_WS_QPA

#ifdef QT3_SUPPORT
    if (flags & Qt::WStaticContents)
        setAttribute(Qt::WA_StaticContents);
    if (flags & Qt::WDestructiveClose)
        setAttribute(Qt::WA_DeleteOnClose);
    if (flags & Qt::WShowModal)
        setWindowModality(Qt::ApplicationModal);
    if (flags & Qt::WMouseNoMask)
        setAttribute(Qt::WA_MouseNoMask);
    if (flags & Qt::WGroupLeader)
        setAttribute(Qt::WA_GroupLeader);
    if (flags & Qt::WNoMousePropagation)
        setAttribute(Qt::WA_NoMousePropagation);
#endif

    static int paintOnScreenEnv = -1;
    if (paintOnScreenEnv == -1)
        paintOnScreenEnv = qgetenv("QT_ONSCREEN_PAINT").toInt() > 0 ? 1 : 0;
    if (paintOnScreenEnv == 1)
        setAttribute(Qt::WA_PaintOnScreen);

    if (QApplicationPrivate::testAttribute(Qt::AA_NativeWindows))
        setAttribute(Qt::WA_NativeWindow);

#ifdef ALIEN_DEBUG
    qDebug() << "QWidget::create:" << this << "parent:" << parentWidget()
             << "Alien?" << !testAttribute(Qt::WA_NativeWindow);
#endif

#if defined (Q_WS_WIN) && !defined(QT_NO_DRAGANDDROP)
    // Unregister the dropsite (if already registered) before we
    // re-create the widget with a native window.
    if (testAttribute(Qt::WA_WState_Created) && !internalWinId() && testAttribute(Qt::WA_NativeWindow)
            && d->extra && d->extra->dropTarget) {
        d->registerDropSite(false);
    }
#endif // defined (Q_WS_WIN) && !defined(QT_NO_DRAGANDDROP)

    d->updateIsOpaque();

    setAttribute(Qt::WA_WState_Created);                        // set created flag
    d->create_sys(window, initializeWindow, destroyOldWindow);

    // a real toplevel window needs a backing store
    if (isWindow() && windowType() != Qt::Desktop) {
        d->topData()->backingStore.destroy();
        if (hasBackingStoreSupport())
            d->topData()->backingStore.create(this);
    }

    d->setModal_sys();

    if (!isWindow() && parentWidget() && parentWidget()->testAttribute(Qt::WA_DropSiteRegistered))
        setAttribute(Qt::WA_DropSiteRegistered, true);

#ifdef QT_EVAL
    extern void qt_eval_init_widget(QWidget *w);
    qt_eval_init_widget(this);
#endif

    // need to force the resting of the icon after changing parents
    if (testAttribute(Qt::WA_SetWindowIcon))
        d->setWindowIcon_sys(true);
    if (isWindow() && !d->topData()->iconText.isEmpty())
        d->setWindowIconText_helper(d->topData()->iconText);
    if (isWindow() && !d->topData()->caption.isEmpty())
        d->setWindowTitle_helper(d->topData()->caption);
    if (windowType() != Qt::Desktop) {
        d->updateSystemBackground();

        if (isWindow() && !testAttribute(Qt::WA_SetWindowIcon))
            d->setWindowIcon_sys();
    }
}

/*!
    Destroys the widget.

    All this widget's children are deleted first. The application
    exits if this widget is the main widget.
*/

QWidget::~QWidget()
{
    Q_D(QWidget);
    d->data.in_destructor = true;

#if defined (QT_CHECK_STATE)
    if (paintingActive())
        qWarning("QWidget: %s (%s) deleted while being painted", className(), name());
#endif

#ifndef QT_NO_GESTURES
    foreach (Qt::GestureType type, d->gestureContext.keys())
        ungrabGesture(type);
#endif

    // force acceptDrops false before winId is destroyed.
    d->registerDropSite(false);

#ifndef QT_NO_ACTION
    // remove all actions from this widget
    for (int i = 0; i < d->actions.size(); ++i) {
        QActionPrivate *apriv = d->actions.at(i)->d_func();
        apriv->widgets.removeAll(this);
    }
    d->actions.clear();
#endif

#ifndef QT_NO_SHORTCUT
    // Remove all shortcuts grabbed by this
    // widget, unless application is closing
    if (!QApplicationPrivate::is_app_closing && testAttribute(Qt::WA_GrabbedShortcut))
        qApp->d_func()->shortcutMap.removeShortcut(0, this, QKeySequence());
#endif

    // delete layout while we still are a valid widget
    delete d->layout;
    d->layout = 0;
    // Remove myself from focus list

    Q_ASSERT(d->focus_next->d_func()->focus_prev == this);
    Q_ASSERT(d->focus_prev->d_func()->focus_next == this);

    if (d->focus_next != this) {
        d->focus_next->d_func()->focus_prev = d->focus_prev;
        d->focus_prev->d_func()->focus_next = d->focus_next;
        d->focus_next = d->focus_prev = 0;
    }

#ifdef QT3_SUPPORT
    if (QApplicationPrivate::main_widget == this) {        // reset main widget
        QApplicationPrivate::main_widget = 0;
        QApplication::quit();
    }
#endif

    QT_TRY {
        clearFocus();
    } QT_CATCH(...) {
        // swallow this problem because we are in a destructor
    }

    d->setDirtyOpaqueRegion();

    if (isWindow() && isVisible() && internalWinId()) {
        QT_TRY {
            d->close_helper(QWidgetPrivate::CloseNoEvent);
        } QT_CATCH(...) {
            // if we're out of memory, at least hide the window.
            QT_TRY {
                hide();
            } QT_CATCH(...) {
                // and if that also doesn't work, then give up
            }
        }
    }

#if defined(Q_WS_WIN) || defined(Q_WS_X11)|| defined(Q_WS_MAC)
    else if (!internalWinId() && isVisible()) {
        qApp->d_func()->sendSyntheticEnterLeave(this);
    }
#elif defined(Q_WS_QWS) || defined(Q_WS_QPA)
    else if (isVisible()) {
        qApp->d_func()->sendSyntheticEnterLeave(this);
    }
#endif

#ifdef Q_OS_SYMBIAN
    if (d->extra && d->extra->topextra && d->extra->topextra->backingStore) {
        // Okay, we are about to destroy the top-level window that owns
        // the backing store. Make sure we delete the backing store right away
        // before the window handle is invalid. This is important because
        // the backing store will delete its window surface, which may or may
        // not have a reference to this widget that will be used later to
        // notify the window it no longer has a surface.
        d->extra->topextra->backingStore.destroy();
    }
#endif
    if (QWidgetBackingStore *bs = d->maybeBackingStore()) {
        bs->removeDirtyWidget(this);
        if (testAttribute(Qt::WA_StaticContents))
            bs->removeStaticWidget(this);
    }

    delete d->needsFlush;
    d->needsFlush = 0;

    // set all QPointers for this object to zero
    if (d->hasGuards)
        QObjectPrivate::clearGuards(this);

    if (d->declarativeData) {
        QAbstractDeclarativeData::destroyed(d->declarativeData, this);
        d->declarativeData = 0;                 // don't activate again in ~QObject
    }

#ifdef QT_MAC_USE_COCOA
    // QCocoaView holds a pointer back to this widget. Clear it now
    // to make sure it's not followed later on. The lifetime of the
    // QCocoaView might exceed the lifetime of this widget in cases
    // where Cocoa itself holds references to it.
    extern void qt_mac_clearCocoaViewQWidgetPointers(QWidget *);
    qt_mac_clearCocoaViewQWidgetPointers(this);
#endif

    if (!d->children.isEmpty())
        d->deleteChildren();

#ifndef QT_NO_ACCESSIBILITY
    QAccessible::updateAccessibility(this, 0, QAccessible::ObjectDestroyed);
#endif

    QApplication::removePostedEvents(this);

    QT_TRY {
        destroy();                                        // platform-dependent cleanup
    } QT_CATCH(...) {
        // if this fails we can't do anything about it but at least we are not allowed to throw.
    }
    --QWidgetPrivate::instanceCounter;

    if (QWidgetPrivate::allWidgets) // might have been deleted by ~QApplication
        QWidgetPrivate::allWidgets->remove(this);

    QT_TRY {
        QEvent e(QEvent::Destroy);
        QCoreApplication::sendEvent(this, &e);
    } QT_CATCH(const std::exception&) {
        // if this fails we can't do anything about it but at least we are not allowed to throw.
    }
}

int QWidgetPrivate::instanceCounter = 0;  // Current number of widget instances
int QWidgetPrivate::maxInstances = 0;     // Maximum number of widget instances

void QWidgetPrivate::setWinId(WId id)                // set widget identifier
{
    Q_Q(QWidget);
    // the user might create a widget with Qt::Desktop window
    // attribute (or create another QDesktopWidget instance), which
    // will have the same windowid (the root window id) as the
    // qt_desktopWidget. We should not add the second desktop widget
    // to the mapper.
    bool userDesktopWidget = qt_desktopWidget != 0 && qt_desktopWidget != q && q->windowType() == Qt::Desktop;
    if (mapper && data.winid && !userDesktopWidget) {
        mapper->remove(data.winid);
    }

    const WId oldWinId = data.winid;

    data.winid = id;
#if defined(Q_WS_X11)
    hd = id; // X11: hd == ident
#endif
    if (mapper && id && !userDesktopWidget) {
        mapper->insert(data.winid, q);
    }

    if(oldWinId != id) {
        QEvent e(QEvent::WinIdChange);
        QCoreApplication::sendEvent(q, &e);
    }
}

void QWidgetPrivate::createTLExtra()
{
    if (!extra)
        createExtra();
    if (!extra->topextra) {
        QTLWExtra* x = extra->topextra = new QTLWExtra;
        x->icon = 0;
        x->iconPixmap = 0;
        x->windowSurface = 0;
        x->sharedPainter = 0;
        x->incw = x->inch = 0;
        x->basew = x->baseh = 0;
        x->frameStrut.setCoords(0, 0, 0, 0);
        x->normalGeometry = QRect(0,0,-1,-1);
        x->savedFlags = 0;
        x->opacity = 255;
        x->posFromMove = false;
        x->sizeAdjusted = false;
        x->inTopLevelResize = false;
        x->inRepaint = false;
        x->embedded = 0;
#ifdef Q_WS_MAC
#ifdef QT_MAC_USE_COCOA
        x->wasMaximized = false;
#endif // QT_MAC_USE_COCOA
#endif // Q_WS_MAC
        createTLSysExtra();
#ifdef QWIDGET_EXTRA_DEBUG
        static int count = 0;
        qDebug() << "tlextra" << ++count;
#endif
#if defined(Q_WS_QPA)
        x->platformWindow = 0;
        x->platformWindowFormat = QPlatformWindowFormat::defaultFormat();
        x->screenIndex = 0;
#endif
    }
}

/*!
  \internal
  Creates the widget extra data.
*/

void QWidgetPrivate::createExtra()
{
    if (!extra) {                                // if not exists
        extra = new QWExtra;
        extra->glContext = 0;
        extra->topextra = 0;
#ifndef QT_NO_GRAPHICSVIEW
        extra->proxyWidget = 0;
#endif
#ifndef QT_NO_CURSOR
        extra->curs = 0;
#endif
        extra->minw = 0;
        extra->minh = 0;
        extra->maxw = QWIDGETSIZE_MAX;
        extra->maxh = QWIDGETSIZE_MAX;
        extra->customDpiX = 0;
        extra->customDpiY = 0;
        extra->explicitMinSize = 0;
        extra->explicitMaxSize = 0;
        extra->autoFillBackground = 0;
        extra->nativeChildrenForced = 0;
        extra->inRenderWithPainter = 0;
        extra->hasMask = 0;
        createSysExtra();
#ifdef QWIDGET_EXTRA_DEBUG
        static int count = 0;
        qDebug() << "extra" << ++count;
#endif
    }
}


/*!
  \internal
  Deletes the widget extra data.
*/

void QWidgetPrivate::deleteExtra()
{
    if (extra) {                                // if exists
#ifndef QT_NO_CURSOR
        delete extra->curs;
#endif
        deleteSysExtra();
#ifndef QT_NO_STYLE_STYLESHEET
        // dereference the stylesheet style
        if (QStyleSheetStyle *proxy = qobject_cast<QStyleSheetStyle *>(extra->style))
            proxy->deref();
#endif
        if (extra->topextra) {
            deleteTLSysExtra();
            extra->topextra->backingStore.destroy();
            delete extra->topextra->icon;
            delete extra->topextra->iconPixmap;
#if defined(Q_WS_QWS) && !defined(QT_NO_QWS_MANAGER)
            delete extra->topextra->qwsManager;
#endif
            delete extra->topextra->windowSurface;
            delete extra->topextra;
        }
        delete extra;
        // extra->xic destroyed in QWidget::destroy()
        extra = 0;
    }
}

/*
  Returns true if there are widgets above this which overlap with
  \a rect, which is in parent's coordinate system (same as crect).
*/

bool QWidgetPrivate::isOverlapped(const QRect &rect) const
{
    Q_Q(const QWidget);

    const QWidget *w = q;
    QRect r = rect;
    while (w) {
        if (w->isWindow())
            return false;
        QWidgetPrivate *pd = w->parentWidget()->d_func();
        bool above = false;
        for (int i = 0; i < pd->children.size(); ++i) {
            QWidget *sibling = qobject_cast<QWidget *>(pd->children.at(i));
            if (!sibling || !sibling->isVisible() || sibling->isWindow())
                continue;
            if (!above) {
                above = (sibling == w);
                continue;
            }

            if (qRectIntersects(sibling->d_func()->effectiveRectFor(sibling->data->crect), r)) {
                const QWExtra *siblingExtra = sibling->d_func()->extra;
                if (siblingExtra && siblingExtra->hasMask && !sibling->d_func()->graphicsEffect
                    && !siblingExtra->mask.translated(sibling->data->crect.topLeft()).intersects(r)) {
                    continue;
                }
                return true;
            }
        }
        w = w->parentWidget();
        r.translate(pd->data.crect.topLeft());
    }
    return false;
}

void QWidgetPrivate::syncBackingStore()
{
    if (paintOnScreen()) {
        repaint_sys(dirty);
        dirty = QRegion();
    } else if (QWidgetBackingStore *bs = maybeBackingStore()) {
        bs->sync();
    }
}

void QWidgetPrivate::syncBackingStore(const QRegion &region)
{
    if (paintOnScreen())
        repaint_sys(region);
    else if (QWidgetBackingStore *bs = maybeBackingStore()) {
        bs->sync(q_func(), region);
    }
}

void QWidgetPrivate::setUpdatesEnabled_helper(bool enable)
{
    Q_Q(QWidget);

    if (enable && !q->isWindow() && q->parentWidget() && !q->parentWidget()->updatesEnabled())
        return; // nothing we can do

    if (enable != q->testAttribute(Qt::WA_UpdatesDisabled))
        return; // nothing to do

    q->setAttribute(Qt::WA_UpdatesDisabled, !enable);
    if (enable)
        q->update();

    Qt::WidgetAttribute attribute = enable ? Qt::WA_ForceUpdatesDisabled : Qt::WA_UpdatesDisabled;
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && !w->isWindow() && !w->testAttribute(attribute))
            w->d_func()->setUpdatesEnabled_helper(enable);
    }
}

/*!
    \internal

    Propagate this widget's palette to all children, except style sheet
    widgets, and windows that don't enable window propagation (palettes don't
    normally propagate to windows).
*/
void QWidgetPrivate::propagatePaletteChange()
{
    Q_Q(QWidget);
    // Propagate a new inherited mask to all children.
#ifndef QT_NO_GRAPHICSVIEW
    if (!q->parentWidget() && extra && extra->proxyWidget) {
        QGraphicsProxyWidget *p = extra->proxyWidget;
        inheritedPaletteResolveMask = p->d_func()->inheritedPaletteResolveMask | p->palette().resolve();
    } else
#endif //QT_NO_GRAPHICSVIEW
        if (q->isWindow() && !q->testAttribute(Qt::WA_WindowPropagation)) {
        inheritedPaletteResolveMask = 0;
    }
    int mask = data.pal.resolve() | inheritedPaletteResolveMask;

    QEvent pc(QEvent::PaletteChange);
    QApplication::sendEvent(q, &pc);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget*>(children.at(i));
        if (w && !w->testAttribute(Qt::WA_StyleSheet)
            && (!w->isWindow() || w->testAttribute(Qt::WA_WindowPropagation))) {
            QWidgetPrivate *wd = w->d_func();
            wd->inheritedPaletteResolveMask = mask;
            wd->resolvePalette();
        }
    }
#if defined(QT3_SUPPORT)
    q->paletteChange(q->palette()); // compatibility
#endif
}

/*
  Returns the widget's clipping rectangle.
*/
QRect QWidgetPrivate::clipRect() const
{
    Q_Q(const QWidget);
    const QWidget * w = q;
    if (!w->isVisible())
        return QRect();
    QRect r = effectiveRectFor(q->rect());
    int ox = 0;
    int oy = 0;
    while (w
            && w->isVisible()
            && !w->isWindow()
            && w->parentWidget()) {
        ox -= w->x();
        oy -= w->y();
        w = w->parentWidget();
        r &= QRect(ox, oy, w->width(), w->height());
    }
    return r;
}

/*
  Returns the widget's clipping region (without siblings).
*/
QRegion QWidgetPrivate::clipRegion() const
{
    Q_Q(const QWidget);
    if (!q->isVisible())
        return QRegion();
    QRegion r(q->rect());
    const QWidget * w = q;
    const QWidget *ignoreUpTo;
    int ox = 0;
    int oy = 0;
    while (w
           && w->isVisible()
           && !w->isWindow()
           && w->parentWidget()) {
        ox -= w->x();
        oy -= w->y();
        ignoreUpTo = w;
        w = w->parentWidget();
        r &= QRegion(ox, oy, w->width(), w->height());

        int i = 0;
        while(w->d_func()->children.at(i++) != static_cast<const QObject *>(ignoreUpTo))
            ;
        for ( ; i < w->d_func()->children.size(); ++i) {
            if(QWidget *sibling = qobject_cast<QWidget *>(w->d_func()->children.at(i))) {
                if(sibling->isVisible() && !sibling->isWindow()) {
                    QRect siblingRect(ox+sibling->x(), oy+sibling->y(),
                                      sibling->width(), sibling->height());
                    if (qRectIntersects(siblingRect, q->rect()))
                        r -= QRegion(siblingRect);
                }
            }
        }
    }
    return r;
}

#ifndef QT_NO_GRAPHICSEFFECT
void QWidgetPrivate::invalidateGraphicsEffectsRecursively()
{
    Q_Q(QWidget);
    QWidget *w = q;
    do {
        if (w->graphicsEffect()) {
            QWidgetEffectSourcePrivate *sourced =
                static_cast<QWidgetEffectSourcePrivate *>(w->graphicsEffect()->source()->d_func());
            if (!sourced->updateDueToGraphicsEffect)
                w->graphicsEffect()->source()->d_func()->invalidateCache();
        }
        w = w->parentWidget();
    } while (w);
}
#endif //QT_NO_GRAPHICSEFFECT

void QWidgetPrivate::setDirtyOpaqueRegion()
{
    Q_Q(QWidget);

    dirtyOpaqueChildren = true;

#ifndef QT_NO_GRAPHICSEFFECT
    invalidateGraphicsEffectsRecursively();
#endif //QT_NO_GRAPHICSEFFECT

    if (q->isWindow())
        return;

    QWidget *parent = q->parentWidget();
    if (!parent)
        return;

    // TODO: instead of setting dirtyflag, manipulate the dirtyregion directly?
    QWidgetPrivate *pd = parent->d_func();
    if (!pd->dirtyOpaqueChildren)
        pd->setDirtyOpaqueRegion();
}

const QRegion &QWidgetPrivate::getOpaqueChildren() const
{
    if (!dirtyOpaqueChildren)
        return opaqueChildren;

    QWidgetPrivate *that = const_cast<QWidgetPrivate*>(this);
    that->opaqueChildren = QRegion();

    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (!child || !child->isVisible() || child->isWindow())
            continue;

        const QPoint offset = child->geometry().topLeft();
        QWidgetPrivate *childd = child->d_func();
        QRegion r = childd->isOpaque ? child->rect() : childd->getOpaqueChildren();
        if (childd->extra && childd->extra->hasMask)
            r &= childd->extra->mask;
        if (r.isEmpty())
            continue;
        r.translate(offset);
        that->opaqueChildren += r;
    }

    that->opaqueChildren &= q_func()->rect();
    that->dirtyOpaqueChildren = false;

    return that->opaqueChildren;
}

void QWidgetPrivate::subtractOpaqueChildren(QRegion &source, const QRect &clipRect) const
{
    if (children.isEmpty() || clipRect.isEmpty())
        return;

    const QRegion &r = getOpaqueChildren();
    if (!r.isEmpty())
        source -= (r & clipRect);
}

//subtract any relatives that are higher up than me --- this is too expensive !!!
void QWidgetPrivate::subtractOpaqueSiblings(QRegion &sourceRegion, bool *hasDirtySiblingsAbove,
                                            bool alsoNonOpaque) const
{
    Q_Q(const QWidget);
    static int disableSubtractOpaqueSiblings = qgetenv("QT_NO_SUBTRACTOPAQUESIBLINGS").toInt();
    if (disableSubtractOpaqueSiblings || q->isWindow())
        return;

#ifdef QT_MAC_USE_COCOA
    if (q->d_func()->isInUnifiedToolbar)
        return;
#endif // QT_MAC_USE_COCOA

    QRect clipBoundingRect;
    bool dirtyClipBoundingRect = true;

    QRegion parentClip;
    bool dirtyParentClip = true;

    QPoint parentOffset = data.crect.topLeft();

    const QWidget *w = q;

    while (w) {
        if (w->isWindow())
            break;
        QWidgetPrivate *pd = w->parentWidget()->d_func();
        const int myIndex = pd->children.indexOf(const_cast<QWidget *>(w));
        const QRect widgetGeometry = w->d_func()->effectiveRectFor(w->data->crect);
        for (int i = myIndex + 1; i < pd->children.size(); ++i) {
            QWidget *sibling = qobject_cast<QWidget *>(pd->children.at(i));
            if (!sibling || !sibling->isVisible() || sibling->isWindow())
                continue;

            const QRect siblingGeometry = sibling->d_func()->effectiveRectFor(sibling->data->crect);
            if (!qRectIntersects(siblingGeometry, widgetGeometry))
                continue;

            if (dirtyClipBoundingRect) {
                clipBoundingRect = sourceRegion.boundingRect();
                dirtyClipBoundingRect = false;
            }

            if (!qRectIntersects(siblingGeometry, clipBoundingRect.translated(parentOffset)))
                continue;

            if (dirtyParentClip) {
                parentClip = sourceRegion.translated(parentOffset);
                dirtyParentClip = false;
            }

            const QPoint siblingPos(sibling->data->crect.topLeft());
            const QRect siblingClipRect(sibling->d_func()->clipRect());
            QRegion siblingDirty(parentClip);
            siblingDirty &= (siblingClipRect.translated(siblingPos));
            const bool hasMask = sibling->d_func()->extra && sibling->d_func()->extra->hasMask
                                 && !sibling->d_func()->graphicsEffect;
            if (hasMask)
                siblingDirty &= sibling->d_func()->extra->mask.translated(siblingPos);
            if (siblingDirty.isEmpty())
                continue;

            if (sibling->d_func()->isOpaque || alsoNonOpaque) {
                if (hasMask) {
                    siblingDirty.translate(-parentOffset);
                    sourceRegion -= siblingDirty;
                } else {
                    sourceRegion -= siblingGeometry.translated(-parentOffset);
                }
            } else {
                if (hasDirtySiblingsAbove)
                    *hasDirtySiblingsAbove = true;
                if (sibling->d_func()->children.isEmpty())
                    continue;
                QRegion opaqueSiblingChildren(sibling->d_func()->getOpaqueChildren());
                opaqueSiblingChildren.translate(-parentOffset + siblingPos);
                sourceRegion -= opaqueSiblingChildren;
            }
            if (sourceRegion.isEmpty())
                return;

            dirtyClipBoundingRect = true;
            dirtyParentClip = true;
        }

        w = w->parentWidget();
        parentOffset += pd->data.crect.topLeft();
        dirtyParentClip = true;
    }
}

void QWidgetPrivate::clipToEffectiveMask(QRegion &region) const
{
    Q_Q(const QWidget);

    const QWidget *w = q;
    QPoint offset;

#ifndef QT_NO_GRAPHICSEFFECT
    if (graphicsEffect) {
        w = q->parentWidget();
        offset -= data.crect.topLeft();
    }
#endif //QT_NO_GRAPHICSEFFECT

    while (w) {
        const QWidgetPrivate *wd = w->d_func();
        if (wd->extra && wd->extra->hasMask)
            region &= (w != q) ? wd->extra->mask.translated(offset) : wd->extra->mask;
        if (w->isWindow())
            return;
        offset -= wd->data.crect.topLeft();
        w = w->parentWidget();
    }
}

bool QWidgetPrivate::paintOnScreen() const
{
#if defined(Q_WS_QWS)
    return false;
#elif  defined(QT_NO_BACKINGSTORE)
    return true;
#else
    Q_Q(const QWidget);
    if (q->testAttribute(Qt::WA_PaintOnScreen)
            || (!q->isWindow() && q->window()->testAttribute(Qt::WA_PaintOnScreen))) {
        return true;
    }

    return !qt_enable_backingstore;
#endif
}

void QWidgetPrivate::updateIsOpaque()
{
    // hw: todo: only needed if opacity actually changed
    setDirtyOpaqueRegion();

#ifndef QT_NO_GRAPHICSEFFECT
    if (graphicsEffect) {
        // ### We should probably add QGraphicsEffect::isOpaque at some point.
        setOpaque(false);
        return;
    }
#endif //QT_NO_GRAPHICSEFFECT

    Q_Q(QWidget);
#ifdef Q_WS_X11
    if (q->testAttribute(Qt::WA_X11OpenGLOverlay)) {
        setOpaque(false);
        return;
    }
#endif

#ifdef Q_WS_S60
    if (q->testAttribute(Qt::WA_TranslucentBackground)) {
        if (q->windowType() & Qt::Dialog || q->windowType() & Qt::Popup) {
            if (S60->avkonComponentsSupportTransparency) {
                setOpaque(false);
                return;
            }
        } else {
            setOpaque(false);
            return;
        }
    }
#endif

    if (q->testAttribute(Qt::WA_OpaquePaintEvent) || q->testAttribute(Qt::WA_PaintOnScreen)) {
        setOpaque(true);
        return;
    }

    const QPalette &pal = q->palette();

    if (q->autoFillBackground()) {
        const QBrush &autoFillBrush = pal.brush(q->backgroundRole());
        if (autoFillBrush.style() != Qt::NoBrush && autoFillBrush.isOpaque()) {
            setOpaque(true);
            return;
        }
    }

    if (q->isWindow() && !q->testAttribute(Qt::WA_NoSystemBackground)) {
#ifdef Q_WS_S60
        setOpaque(true);
        return;
#else
        const QBrush &windowBrush = q->palette().brush(QPalette::Window);
        if (windowBrush.style() != Qt::NoBrush && windowBrush.isOpaque()) {
            setOpaque(true);
            return;
        }
#endif
    }
    setOpaque(false);
}

void QWidgetPrivate::setOpaque(bool opaque)
{
    if (isOpaque == opaque)
        return;
    isOpaque = opaque;
#ifdef Q_WS_MAC
    macUpdateIsOpaque();
#endif
#ifdef Q_WS_X11
    x11UpdateIsOpaque();
#endif
#ifdef Q_WS_WIN
    winUpdateIsOpaque();
#endif
#ifdef Q_OS_SYMBIAN
    s60UpdateIsOpaque();
#endif
}

void QWidgetPrivate::updateIsTranslucent()
{
#ifdef Q_WS_MAC
    macUpdateIsOpaque();
#endif
#ifdef Q_WS_X11
    x11UpdateIsOpaque();
#endif
#ifdef Q_WS_WIN
    winUpdateIsOpaque();
#endif
#ifdef Q_OS_SYMBIAN
    s60UpdateIsOpaque();
#endif
}

/*!
    \fn void QPixmap::fill(const QWidget *widget, const QPoint &offset)

    Fills the pixmap with the \a widget's background color or pixmap
    according to the given offset.

    The QPoint \a offset defines a point in widget coordinates to
    which the pixmap's top-left pixel will be mapped to. This is only
    significant if the widget has a background pixmap; otherwise the
    pixmap will simply be filled with the background color of the
    widget.
*/

void QPixmap::fill( const QWidget *widget, const QPoint &off )
{
    QPainter p(this);
    p.translate(-off);
    widget->d_func()->paintBackground(&p, QRect(off, size()));
}

static inline void fillRegion(QPainter *painter, const QRegion &rgn, const QBrush &brush)
{
    Q_ASSERT(painter);

    if (brush.style() == Qt::TexturePattern) {
#ifdef Q_WS_MAC
        // Optimize pattern filling on mac by using HITheme directly
        // when filling with the standard widget background.
        // Defined in qmacstyle_mac.cpp
        extern void qt_mac_fill_background(QPainter *painter, const QRegion &rgn, const QBrush &brush);
        qt_mac_fill_background(painter, rgn, brush);
#else
#if !defined(QT_NO_STYLE_S60)
        // Defined in qs60style.cpp
        extern bool qt_s60_fill_background(QPainter *painter, const QRegion &rgn, const QBrush &brush);
        if (!qt_s60_fill_background(painter, rgn, brush))
#endif // !defined(QT_NO_STYLE_S60)
        {
            const QRect rect(rgn.boundingRect());
            painter->setClipRegion(rgn);
            painter->drawTiledPixmap(rect, brush.texture(), rect.topLeft());
        }
#endif // Q_WS_MAC

    } else if (brush.gradient()
               && brush.gradient()->coordinateMode() == QGradient::ObjectBoundingMode) {
        painter->save();
        painter->setClipRegion(rgn);
        painter->fillRect(0, 0, painter->device()->width(), painter->device()->height(), brush);
        painter->restore();
    } else {
        const QVector<QRect> &rects = rgn.rects();
        for (int i = 0; i < rects.size(); ++i)
            painter->fillRect(rects.at(i), brush);
    }
}

void QWidgetPrivate::paintBackground(QPainter *painter, const QRegion &rgn, int flags) const
{
    Q_Q(const QWidget);

#ifndef QT_NO_SCROLLAREA
    bool resetBrushOrigin = false;
    QPointF oldBrushOrigin;
    //If we are painting the viewport of a scrollarea, we must apply an offset to the brush in case we are drawing a texture
    QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(parent);
    if (scrollArea && scrollArea->viewport() == q) {
        QObjectData *scrollPrivate = static_cast<QWidget *>(scrollArea)->d_ptr.data();
        QAbstractScrollAreaPrivate *priv = static_cast<QAbstractScrollAreaPrivate *>(scrollPrivate);
        oldBrushOrigin = painter->brushOrigin();
        resetBrushOrigin = true;
        painter->setBrushOrigin(-priv->contentsOffset());

    }
#endif // QT_NO_SCROLLAREA

    const QBrush autoFillBrush = q->palette().brush(q->backgroundRole());

    if ((flags & DrawAsRoot) && !(q->autoFillBackground() && autoFillBrush.isOpaque())) {
        const QBrush bg = q->palette().brush(QPalette::Window);
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
        if (!(flags & DontSetCompositionMode)) {
            //copy alpha straight in
            QPainter::CompositionMode oldMode = painter->compositionMode();
            painter->setCompositionMode(QPainter::CompositionMode_Source);
            fillRegion(painter, rgn, bg);
            painter->setCompositionMode(oldMode);
        } else {
            fillRegion(painter, rgn, bg);
        }
#else
        fillRegion(painter, rgn, bg);
#endif
    }

    if (q->autoFillBackground())
        fillRegion(painter, rgn, autoFillBrush);

    if (q->testAttribute(Qt::WA_StyledBackground)) {
        painter->setClipRegion(rgn);
        QStyleOption opt;
        opt.initFrom(q);
        q->style()->drawPrimitive(QStyle::PE_Widget, &opt, painter, q);
    }

#ifndef QT_NO_SCROLLAREA
    if (resetBrushOrigin)
        painter->setBrushOrigin(oldBrushOrigin);
#endif // QT_NO_SCROLLAREA
}

/*
  \internal
  This function is called when a widget is hidden or destroyed.
  It resets some application global pointers that should only refer active,
  visible widgets.
*/

#ifdef Q_WS_MAC
    extern QPointer<QWidget> qt_button_down;
#else
    extern QWidget *qt_button_down;
#endif

void QWidgetPrivate::deactivateWidgetCleanup()
{
    Q_Q(QWidget);
    // If this was the active application window, reset it
    if (QApplication::activeWindow() == q)
        QApplication::setActiveWindow(0);
    // If the is the active mouse press widget, reset it
    if (q == qt_button_down)
        qt_button_down = 0;
}


/*!
    Returns a pointer to the widget with window identifer/handle \a
    id.

    The window identifier type depends on the underlying window
    system, see \c qwindowdefs.h for the actual definition. If there
    is no widget with this identifier, 0 is returned.
*/

QWidget *QWidget::find(WId id)
{
    return QWidgetPrivate::mapper ? QWidgetPrivate::mapper->value(id, 0) : 0;
}



/*!
    \fn WId QWidget::internalWinId() const
    \internal
    Returns the window system identifier of the widget, or 0 if the widget is not created yet.

*/

/*!
    \fn WId QWidget::winId() const

    Returns the window system identifier of the widget.

    Portable in principle, but if you use it you are probably about to
    do something non-portable. Be careful.

    If a widget is non-native (alien) and winId() is invoked on it, that widget
    will be provided a native handle.

    On Mac OS X, the type returned depends on which framework Qt was linked
    against. If Qt is using Carbon, the {WId} is actually an HIViewRef. If Qt
    is using Cocoa, {WId} is a pointer to an NSView.

    This value may change at run-time. An event with type QEvent::WinIdChange
    will be sent to the widget following a change in window system identifier.

    \sa find()
*/
WId QWidget::winId() const
{
    if (!testAttribute(Qt::WA_WState_Created) || !internalWinId()) {
#ifdef ALIEN_DEBUG
        qDebug() << "QWidget::winId: creating native window for" << this;
#endif
        QWidget *that = const_cast<QWidget*>(this);
#ifndef Q_WS_QPA
        that->setAttribute(Qt::WA_NativeWindow);
#endif
        that->d_func()->createWinId();
        return that->data->winid;
    }
    return data->winid;
}


void QWidgetPrivate::createWinId(WId winid)
{
    Q_Q(QWidget);

#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::createWinId for" << q << winid;
#endif
    const bool forceNativeWindow = q->testAttribute(Qt::WA_NativeWindow);
    if (!q->testAttribute(Qt::WA_WState_Created) || (forceNativeWindow && !q->internalWinId())) {
#ifndef Q_WS_QPA
        if (!q->isWindow()) {
            QWidget *parent = q->parentWidget();
            QWidgetPrivate *pd = parent->d_func();
            if (forceNativeWindow && !q->testAttribute(Qt::WA_DontCreateNativeAncestors))
                parent->setAttribute(Qt::WA_NativeWindow);
            if (!parent->internalWinId()) {
                pd->createWinId();
            }

            for (int i = 0; i < pd->children.size(); ++i) {
                QWidget *w = qobject_cast<QWidget *>(pd->children.at(i));
                if (w && !w->isWindow() && (!w->testAttribute(Qt::WA_WState_Created)
                                            || (!w->internalWinId() && w->testAttribute(Qt::WA_NativeWindow)))) {
                    if (w!=q) {
                        w->create();
                    } else {
                        w->create(winid);
                        // if the window has already been created, we
                        // need to raise it to its proper stacking position
                        if (winid)
                            w->raise();
                    }
                }
            }
        } else {
            q->create();
        }
#else
        Q_UNUSED(winid);
        q->create();
#endif //Q_WS_QPA

    }
}


/*!
\internal
Ensures that the widget has a window system identifier, i.e. that it is known to the windowing system.

*/

void QWidget::createWinId()
{
    Q_D(QWidget);
#ifdef ALIEN_DEBUG
    qDebug()  << "QWidget::createWinId" << this;
#endif
//    qWarning("QWidget::createWinId is obsolete, please fix your code.");
    d->createWinId();
}

/*!
    \since 4.4

    Returns the effective window system identifier of the widget, i.e. the
    native parent's window system identifier.

    If the widget is native, this function returns the native widget ID.
    Otherwise, the window ID of the first native parent widget, i.e., the
    top-level widget that contains this widget, is returned.

    \note We recommend that you do not store this value as it is likely to
    change at run-time.

    \sa nativeParentWidget()
*/
WId QWidget::effectiveWinId() const
{
    WId id = internalWinId();
    if (id || !testAttribute(Qt::WA_WState_Created))
        return id;
    QWidget *realParent = nativeParentWidget();
    if (!realParent && d_func()->inSetParent) {
        // In transitional state. This is really just a workaround. The real problem
        // is that QWidgetPrivate::setParent_sys (platform specific code) first sets
        // the window id to 0 (setWinId(0)) before it sets the Qt::WA_WState_Created
        // attribute to false. The correct way is to do it the other way around, and
        // in that case the Qt::WA_WState_Created logic above will kick in and
        // return 0 whenever the widget is in a transitional state. However, changing
        // the original logic for all platforms is far more intrusive and might
        // break existing applications.
        // Note: The widget can only be in a transitional state when changing its
        // parent -- everything else is an internal error -- hence explicitly checking
        // against 'inSetParent' rather than doing an unconditional return whenever
        // 'realParent' is 0 (which may cause strange artifacts and headache later).
        return 0;
    }
    // This widget *must* have a native parent widget.
    Q_ASSERT(realParent);
    Q_ASSERT(realParent->internalWinId());
    return realParent->internalWinId();
}

#ifndef QT_NO_STYLE_STYLESHEET

/*!
    \property QWidget::styleSheet
    \brief the widget's style sheet
    \since 4.2

    The style sheet contains a textual description of customizations to the
    widget's style, as described in the \l{Qt Style Sheets} document.

    Since Qt 4.5, Qt style sheets fully supports Mac OS X.

    \warning Qt style sheets are currently not supported for custom QStyle
    subclasses. We plan to address this in some future release.

    \sa setStyle(), QApplication::styleSheet, {Qt Style Sheets}
*/
QString QWidget::styleSheet() const
{
    Q_D(const QWidget);
    if (!d->extra)
        return QString();
    return d->extra->styleSheet;
}

void QWidget::setStyleSheet(const QString& styleSheet)
{
    Q_D(QWidget);
    d->createExtra();

    QStyleSheetStyle *proxy = qobject_cast<QStyleSheetStyle *>(d->extra->style);
    d->extra->styleSheet = styleSheet;
    if (styleSheet.isEmpty()) { // stylesheet removed
        if (!proxy)
            return;

        d->inheritStyle();
        return;
    }

    if (proxy) { // style sheet update
        proxy->repolish(this);
        return;
    }

    if (testAttribute(Qt::WA_SetStyle)) {
        d->setStyle_helper(new QStyleSheetStyle(d->extra->style), true);
    } else {
        d->setStyle_helper(new QStyleSheetStyle(0), true);
    }
}

#endif // QT_NO_STYLE_STYLESHEET

/*!
    \sa QWidget::setStyle(), QApplication::setStyle(), QApplication::style()
*/

QStyle *QWidget::style() const
{
    Q_D(const QWidget);

    if (d->extra && d->extra->style)
        return d->extra->style;
    return QApplication::style();
}

/*!
    Sets the widget's GUI style to \a style. The ownership of the style
    object is not transferred.

    If no style is set, the widget uses the application's style,
    QApplication::style() instead.

    Setting a widget's style has no effect on existing or future child
    widgets.

    \warning This function is particularly useful for demonstration
    purposes, where you want to show Qt's styling capabilities. Real
    applications should avoid it and use one consistent GUI style
    instead.

    \warning Qt style sheets are currently not supported for custom QStyle
    subclasses. We plan to address this in some future release.

    \sa style(), QStyle, QApplication::style(), QApplication::setStyle()
*/

void QWidget::setStyle(QStyle *style)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_SetStyle, style != 0);
    d->createExtra();
#ifndef QT_NO_STYLE_STYLESHEET
    if (QStyleSheetStyle *proxy = qobject_cast<QStyleSheetStyle *>(style)) {
        //if for some reason someone try to set a QStyleSheetStyle, ref it
        //(this may happen for exemple in QButtonDialogBox which propagates its style)
        proxy->ref();
        d->setStyle_helper(style, false);
    } else if (qobject_cast<QStyleSheetStyle *>(d->extra->style) || !qApp->styleSheet().isEmpty()) {
        // if we have an application stylesheet or have a proxy already, propagate
        d->setStyle_helper(new QStyleSheetStyle(style), true);
    } else
#endif
        d->setStyle_helper(style, false);
}

void QWidgetPrivate::setStyle_helper(QStyle *newStyle, bool propagate, bool
#ifdef Q_WS_MAC
        metalHack
#endif
        )
{
    Q_Q(QWidget);
    QStyle *oldStyle  = q->style();
#ifndef QT_NO_STYLE_STYLESHEET
    QWeakPointer<QStyle> origStyle;
#endif

#ifdef Q_WS_MAC
    // the metalhack boolean allows Qt/Mac to do a proper re-polish depending
    // on how the Qt::WA_MacBrushedMetal attribute is set. It is only ever
    // set when changing that attribute and passes the widget's CURRENT style.
    // therefore no need to do a reassignment.
    if (!metalHack)
#endif
    {
        createExtra();

#ifndef QT_NO_STYLE_STYLESHEET
        origStyle = extra->style.data();
#endif
        extra->style = newStyle;
    }

    // repolish
    if (q->windowType() != Qt::Desktop) {
        if (polished) {
            oldStyle->unpolish(q);
#ifdef Q_WS_MAC
            if (metalHack)
                macUpdateMetalAttribute();
#endif
            q->style()->polish(q);
#ifdef Q_WS_MAC
        } else if (metalHack) {
            macUpdateMetalAttribute();
#endif
        }
    }

    if (propagate) {
        for (int i = 0; i < children.size(); ++i) {
            QWidget *c = qobject_cast<QWidget*>(children.at(i));
            if (c)
                c->d_func()->inheritStyle();
        }
    }

#ifndef QT_NO_STYLE_STYLESHEET
    if (!qobject_cast<QStyleSheetStyle*>(newStyle)) {
        if (const QStyleSheetStyle* cssStyle = qobject_cast<QStyleSheetStyle*>(origStyle.data())) {
            cssStyle->clearWidgetFont(q);
        }
    }
#endif

    QEvent e(QEvent::StyleChange);
    QApplication::sendEvent(q, &e);
#ifdef QT3_SUPPORT
    q->styleChange(*oldStyle);
#endif

#ifndef QT_NO_STYLE_STYLESHEET
    // dereference the old stylesheet style
    if (QStyleSheetStyle *proxy = qobject_cast<QStyleSheetStyle *>(origStyle.data()))
        proxy->deref();
#endif
}

// Inherits style from the current parent and propagates it as necessary
void QWidgetPrivate::inheritStyle()
{
#ifndef QT_NO_STYLE_STYLESHEET
    Q_Q(QWidget);

    QStyleSheetStyle *proxy = extra ? qobject_cast<QStyleSheetStyle *>(extra->style) : 0;

    if (!q->styleSheet().isEmpty()) {
        Q_ASSERT(proxy);
        proxy->repolish(q);
        return;
    }

    QStyle *origStyle = proxy ? proxy->base : (extra ? (QStyle*)extra->style : 0);
    QWidget *parent = q->parentWidget();
    QStyle *parentStyle = (parent && parent->d_func()->extra) ? (QStyle*)parent->d_func()->extra->style : 0;
    // If we have stylesheet on app or parent has stylesheet style, we need
    // to be running a proxy
    if (!qApp->styleSheet().isEmpty() || qobject_cast<QStyleSheetStyle *>(parentStyle)) {
        QStyle *newStyle = parentStyle;
        if (q->testAttribute(Qt::WA_SetStyle))
            newStyle = new QStyleSheetStyle(origStyle);
        else if (QStyleSheetStyle *newProxy = qobject_cast<QStyleSheetStyle *>(parentStyle))
            newProxy->ref();

        setStyle_helper(newStyle, true);
        return;
    }

    // So, we have no stylesheet on parent/app and we have an empty stylesheet
    // we just need our original style back
    if (origStyle == (extra ? (QStyle*)extra->style : 0)) // is it any different?
        return;

    // We could have inherited the proxy from our parent (which has a custom style)
    // In such a case we need to start following the application style (i.e revert
    // the propagation behavior of QStyleSheetStyle)
    if (!q->testAttribute(Qt::WA_SetStyle))
        origStyle = 0;

    setStyle_helper(origStyle, true);
#endif // QT_NO_STYLE_STYLESHEET
}

#ifdef QT3_SUPPORT
/*!
    \overload

    Sets the widget's GUI style to \a style using the QStyleFactory.
*/
QStyle* QWidget::setStyle(const QString &style)
{
    QStyle *s = QStyleFactory::create(style);
    setStyle(s);
    return s;
}
#endif

/*!
    \fn bool QWidget::isWindow() const

    Returns true if the widget is an independent window, otherwise
    returns false.

    A window is a widget that isn't visually the child of any other
    widget and that usually has a frame and a
    \l{QWidget::setWindowTitle()}{window title}.

    A window can have a \l{QWidget::parentWidget()}{parent widget}.
    It will then be grouped with its parent and deleted when the
    parent is deleted, minimized when the parent is minimized etc. If
    supported by the window manager, it will also have a common
    taskbar entry with its parent.

    QDialog and QMainWindow widgets are by default windows, even if a
    parent widget is specified in the constructor. This behavior is
    specified by the Qt::Window flag.

    \sa window(), isModal(), parentWidget()
*/

/*!
    \property QWidget::modal
    \brief whether the widget is a modal widget

    This property only makes sense for windows. A modal widget
    prevents widgets in all other windows from getting any input.

    By default, this property is false.

    \sa isWindow(), windowModality, QDialog
*/

/*!
    \property QWidget::windowModality
    \brief which windows are blocked by the modal widget
    \since 4.1

    This property only makes sense for windows. A modal widget
    prevents widgets in other windows from getting input. The value of
    this property controls which windows are blocked when the widget
    is visible. Changing this property while the window is visible has
    no effect; you must hide() the widget first, then show() it again.

    By default, this property is Qt::NonModal.

    \sa isWindow(), QWidget::modal, QDialog
*/

Qt::WindowModality QWidget::windowModality() const
{
    return static_cast<Qt::WindowModality>(data->window_modality);
}

void QWidget::setWindowModality(Qt::WindowModality windowModality)
{
    data->window_modality = windowModality;
    // setModal_sys() will be called by setAttribute()
    setAttribute(Qt::WA_ShowModal, (data->window_modality != Qt::NonModal));
    setAttribute(Qt::WA_SetWindowModality, true);
}

/*!
    \fn bool QWidget::underMouse() const

    Returns true if the widget is under the mouse cursor; otherwise
    returns false.

    This value is not updated properly during drag and drop
    operations.

    \sa enterEvent(), leaveEvent()
*/

/*!
    \property QWidget::minimized
    \brief whether this widget is minimized (iconified)

    This property is only relevant for windows.

    By default, this property is false.

    \sa showMinimized(), visible, show(), hide(), showNormal(), maximized
*/
bool QWidget::isMinimized() const
{ return data->window_state & Qt::WindowMinimized; }

/*!
    Shows the widget minimized, as an icon.

    Calling this function only affects \l{isWindow()}{windows}.

    \sa showNormal(), showMaximized(), show(), hide(), isVisible(),
        isMinimized()
*/
void QWidget::showMinimized()
{
    bool isMin = isMinimized();
    if (isMin && isVisible())
        return;

    ensurePolished();
#ifdef QT3_SUPPORT
    if (parent())
        QApplication::sendPostedEvents(parent(), QEvent::ChildInserted);
#endif

    if (!isMin)
        setWindowState((windowState() & ~Qt::WindowActive) | Qt::WindowMinimized);
    show();
}

/*!
    \property QWidget::maximized
    \brief whether this widget is maximized

    This property is only relevant for windows.

    \note Due to limitations on some window systems, this does not always
    report the expected results (e.g., if the user on X11 maximizes the
    window via the window manager, Qt has no way of distinguishing this
    from any other resize). This is expected to improve as window manager
    protocols evolve.

    By default, this property is false.

    \sa windowState(), showMaximized(), visible, show(), hide(), showNormal(), minimized
*/
bool QWidget::isMaximized() const
{ return data->window_state & Qt::WindowMaximized; }



/*!
    Returns the current window state. The window state is a OR'ed
    combination of Qt::WindowState: Qt::WindowMinimized,
    Qt::WindowMaximized, Qt::WindowFullScreen, and Qt::WindowActive.

  \sa Qt::WindowState setWindowState()
 */
Qt::WindowStates QWidget::windowState() const
{
    return Qt::WindowStates(data->window_state);
}

/*!\internal

   The function sets the window state on child widgets similar to
   setWindowState(). The difference is that the window state changed
   event has the isOverride() flag set. It exists mainly to keep
   Q3Workspace working.
 */
void QWidget::overrideWindowState(Qt::WindowStates newstate)
{
    QWindowStateChangeEvent e(Qt::WindowStates(data->window_state), true);
    data->window_state  = newstate;
    QApplication::sendEvent(this, &e);
}

/*!
    \fn void QWidget::setWindowState(Qt::WindowStates windowState)

    Sets the window state to \a windowState. The window state is a OR'ed
    combination of Qt::WindowState: Qt::WindowMinimized,
    Qt::WindowMaximized, Qt::WindowFullScreen, and Qt::WindowActive.

    If the window is not visible (i.e. isVisible() returns false), the
    window state will take effect when show() is called. For visible
    windows, the change is immediate. For example, to toggle between
    full-screen and normal mode, use the following code:

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 0

    In order to restore and activate a minimized window (while
    preserving its maximized and/or full-screen state), use the following:

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 1

    Calling this function will hide the widget. You must call show() to make
    the widget visible again.

    \note On some window systems Qt::WindowActive is not immediate, and may be
    ignored in certain cases.

    When the window state changes, the widget receives a changeEvent()
    of type QEvent::WindowStateChange.

    \sa Qt::WindowState windowState()
*/

/*!
    \property QWidget::fullScreen
    \brief whether the widget is shown in full screen mode

    A widget in full screen mode occupies the whole screen area and does not
    display window decorations, such as a title bar.

    By default, this property is false.

    \sa windowState(), minimized, maximized
*/
bool QWidget::isFullScreen() const
{ return data->window_state & Qt::WindowFullScreen; }

/*!
    Shows the widget in full-screen mode.

    Calling this function only affects \l{isWindow()}{windows}.

    To return from full-screen mode, call showNormal().

    Full-screen mode works fine under Windows, but has certain
    problems under X. These problems are due to limitations of the
    ICCCM protocol that specifies the communication between X11
    clients and the window manager. ICCCM simply does not understand
    the concept of non-decorated full-screen windows. Therefore, the
    best we can do is to request a borderless window and place and
    resize it to fill the entire screen. Depending on the window
    manager, this may or may not work. The borderless window is
    requested using MOTIF hints, which are at least partially
    supported by virtually all modern window managers.

    An alternative would be to bypass the window manager entirely and
    create a window with the Qt::X11BypassWindowManagerHint flag. This
    has other severe problems though, like totally broken keyboard focus
    and very strange effects on desktop changes or when the user raises
    other windows.

    X11 window managers that follow modern post-ICCCM specifications
    support full-screen mode properly.

    \sa showNormal(), showMaximized(), show(), hide(), isVisible()
*/
void QWidget::showFullScreen()
{
#ifdef Q_WS_MAC
    // If the unified toolbar is enabled, we have to disable it before going fullscreen.
    QMainWindow *mainWindow = qobject_cast<QMainWindow*>(this);
    if (mainWindow && mainWindow->unifiedTitleAndToolBarOnMac()) {
        mainWindow->setUnifiedTitleAndToolBarOnMac(false);
        QMainWindowLayout *mainLayout = qobject_cast<QMainWindowLayout*>(mainWindow->layout());
        mainLayout->activateUnifiedToolbarAfterFullScreen = true;
    }
#endif // Q_WS_MAC
    ensurePolished();
#ifdef QT3_SUPPORT
    if (parent())
        QApplication::sendPostedEvents(parent(), QEvent::ChildInserted);
#endif

    setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowMaximized))
                   | Qt::WindowFullScreen);
    show();
    activateWindow();
}

/*!
    Shows the widget maximized.

    Calling this function only affects \l{isWindow()}{windows}.

    On X11, this function may not work properly with certain window
    managers. See the \l{Window Geometry} documentation for an explanation.

    \sa setWindowState(), showNormal(), showMinimized(), show(), hide(), isVisible()
*/
void QWidget::showMaximized()
{
    ensurePolished();
#ifdef QT3_SUPPORT
    if (parent())
        QApplication::sendPostedEvents(parent(), QEvent::ChildInserted);
#endif

    setWindowState((windowState() & ~(Qt::WindowMinimized | Qt::WindowFullScreen))
                   | Qt::WindowMaximized);
#ifdef Q_WS_MAC
    // If the unified toolbar was enabled before going fullscreen, we have to enable it back.
    QMainWindow *mainWindow = qobject_cast<QMainWindow*>(this);
    if (mainWindow)
    {
        QMainWindowLayout *mainLayout = qobject_cast<QMainWindowLayout*>(mainWindow->layout());
        if (mainLayout->activateUnifiedToolbarAfterFullScreen) {
            mainWindow->setUnifiedTitleAndToolBarOnMac(true);
            mainLayout->activateUnifiedToolbarAfterFullScreen = false;
        }
    }
#endif // Q_WS_MAC
    show();
}

/*!
    Restores the widget after it has been maximized or minimized.

    Calling this function only affects \l{isWindow()}{windows}.

    \sa setWindowState(), showMinimized(), showMaximized(), show(), hide(), isVisible()
*/
void QWidget::showNormal()
{
    ensurePolished();
#ifdef QT3_SUPPORT
    if (parent())
        QApplication::sendPostedEvents(parent(), QEvent::ChildInserted);
#endif

    setWindowState(windowState() & ~(Qt::WindowMinimized
                                     | Qt::WindowMaximized
                                     | Qt::WindowFullScreen));
#ifdef Q_WS_MAC
    // If the unified toolbar was enabled before going fullscreen, we have to enable it back.
    QMainWindow *mainWindow = qobject_cast<QMainWindow*>(this);
    if (mainWindow)
    {
        QMainWindowLayout *mainLayout = qobject_cast<QMainWindowLayout*>(mainWindow->layout());
        if (mainLayout->activateUnifiedToolbarAfterFullScreen) {
            mainWindow->setUnifiedTitleAndToolBarOnMac(true);
            mainLayout->activateUnifiedToolbarAfterFullScreen = false;
        }
    }
#endif // Q_WS_MAC
    show();
}

/*!
    Returns true if this widget would become enabled if \a ancestor is
    enabled; otherwise returns false.



    This is the case if neither the widget itself nor every parent up
    to but excluding \a ancestor has been explicitly disabled.

    isEnabledTo(0) is equivalent to isEnabled().

    \sa setEnabled() enabled
*/

bool QWidget::isEnabledTo(QWidget* ancestor) const
{
    const QWidget * w = this;
    while (!w->testAttribute(Qt::WA_ForceDisabled)
            && !w->isWindow()
            && w->parentWidget()
            && w->parentWidget() != ancestor)
        w = w->parentWidget();
    return !w->testAttribute(Qt::WA_ForceDisabled);
}

#ifndef QT_NO_ACTION
/*!
    Appends the action \a action to this widget's list of actions.

    All QWidgets have a list of \l{QAction}s, however they can be
    represented graphically in many different ways. The default use of
    the QAction list (as returned by actions()) is to create a context
    QMenu.

    A QWidget should only have one of each action and adding an action
    it already has will not cause the same action to be in the widget twice.

    The ownership of \a action is not transferred to this QWidget.

    \sa removeAction(), insertAction(), actions(), QMenu
*/
void QWidget::addAction(QAction *action)
{
    insertAction(0, action);
}

/*!
    Appends the actions \a actions to this widget's list of actions.

    \sa removeAction(), QMenu, addAction()
*/
void QWidget::addActions(QList<QAction*> actions)
{
    for(int i = 0; i < actions.count(); i++)
        insertAction(0, actions.at(i));
}

/*!
    Inserts the action \a action to this widget's list of actions,
    before the action \a before. It appends the action if \a before is 0 or
    \a before is not a valid action for this widget.

    A QWidget should only have one of each action.

    \sa removeAction(), addAction(), QMenu, contextMenuPolicy, actions()
*/
void QWidget::insertAction(QAction *before, QAction *action)
{
    if(!action) {
        qWarning("QWidget::insertAction: Attempt to insert null action");
        return;
    }

    Q_D(QWidget);
    if(d->actions.contains(action))
        removeAction(action);

    int pos = d->actions.indexOf(before);
    if (pos < 0) {
        before = 0;
        pos = d->actions.size();
    }
    d->actions.insert(pos, action);

    QActionPrivate *apriv = action->d_func();
    apriv->widgets.append(this);

    QActionEvent e(QEvent::ActionAdded, action, before);
    QApplication::sendEvent(this, &e);
}

/*!
    Inserts the actions \a actions to this widget's list of actions,
    before the action \a before. It appends the action if \a before is 0 or
    \a before is not a valid action for this widget.

    A QWidget can have at most one of each action.

    \sa removeAction(), QMenu, insertAction(), contextMenuPolicy
*/
void QWidget::insertActions(QAction *before, QList<QAction*> actions)
{
    for(int i = 0; i < actions.count(); ++i)
        insertAction(before, actions.at(i));
}

/*!
    Removes the action \a action from this widget's list of actions.
    \sa insertAction(), actions(), insertAction()
*/
void QWidget::removeAction(QAction *action)
{
    if (!action)
        return;

    Q_D(QWidget);

    QActionPrivate *apriv = action->d_func();
    apriv->widgets.removeAll(this);

    if (d->actions.removeAll(action)) {
        QActionEvent e(QEvent::ActionRemoved, action);
        QApplication::sendEvent(this, &e);
    }
}

/*!
    Returns the (possibly empty) list of this widget's actions.

    \sa contextMenuPolicy, insertAction(), removeAction()
*/
QList<QAction*> QWidget::actions() const
{
    Q_D(const QWidget);
    return d->actions;
}
#endif // QT_NO_ACTION

/*!
  \fn bool QWidget::isEnabledToTLW() const
  \obsolete

  This function is deprecated. It is equivalent to isEnabled()
*/

/*!
    \property QWidget::enabled
    \brief whether the widget is enabled

    In general an enabled widget handles keyboard and mouse events; a disabled
    widget does not. An exception is made with \l{QAbstractButton}.

    Some widgets display themselves differently when they are
    disabled. For example a button might draw its label grayed out. If
    your widget needs to know when it becomes enabled or disabled, you
    can use the changeEvent() with type QEvent::EnabledChange.

    Disabling a widget implicitly disables all its children. Enabling
    respectively enables all child widgets unless they have been
    explicitly disabled.

    By default, this property is true.

    \sa isEnabledTo(), QKeyEvent, QMouseEvent, changeEvent()
*/
void QWidget::setEnabled(bool enable)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_ForceDisabled, !enable);
    d->setEnabled_helper(enable);
}

void QWidgetPrivate::setEnabled_helper(bool enable)
{
    Q_Q(QWidget);

    if (enable && !q->isWindow() && q->parentWidget() && !q->parentWidget()->isEnabled())
        return; // nothing we can do

    if (enable != q->testAttribute(Qt::WA_Disabled))
        return; // nothing to do

    q->setAttribute(Qt::WA_Disabled, !enable);
    updateSystemBackground();

    if (!enable && q->window()->focusWidget() == q) {
        bool parentIsEnabled = (!q->parentWidget() || q->parentWidget()->isEnabled());
        if (!parentIsEnabled || !q->focusNextChild())
            q->clearFocus();
    }

    Qt::WidgetAttribute attribute = enable ? Qt::WA_ForceDisabled : Qt::WA_Disabled;
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && !w->testAttribute(attribute))
            w->d_func()->setEnabled_helper(enable);
    }
#if defined(Q_WS_X11)
    if (q->testAttribute(Qt::WA_SetCursor) || q->isWindow()) {
        // enforce the windows behavior of clearing the cursor on
        // disabled widgets
        qt_x11_enforce_cursor(q);
    }
#endif
#if defined(Q_WS_MAC)
    setEnabled_helper_sys(enable);
#endif
#ifndef QT_NO_IM
    if (q->testAttribute(Qt::WA_InputMethodEnabled) && q->hasFocus()) {
        QWidget *focusWidget = effectiveFocusWidget();
        QInputContext *qic = focusWidget->d_func()->inputContext();
        if (enable) {
            if (focusWidget->testAttribute(Qt::WA_InputMethodEnabled))
                qic->setFocusWidget(focusWidget);
        } else {
            qic->reset();
            qic->setFocusWidget(0);
        }
    }
#endif //QT_NO_IM
    QEvent e(QEvent::EnabledChange);
    QApplication::sendEvent(q, &e);
#ifdef QT3_SUPPORT
    q->enabledChange(!enable); // compatibility
#endif
}

/*!
    \property QWidget::acceptDrops
    \brief whether drop events are enabled for this widget

    Setting this property to true announces to the system that this
    widget \e may be able to accept drop events.

    If the widget is the desktop (windowType() == Qt::Desktop), this may
    fail if another application is using the desktop; you can call
    acceptDrops() to test if this occurs.

    \warning Do not modify this property in a drag and drop event handler.

    By default, this property is false.

    \sa {Drag and Drop}
*/
bool QWidget::acceptDrops() const
{
    return testAttribute(Qt::WA_AcceptDrops);
}

void QWidget::setAcceptDrops(bool on)
{
    setAttribute(Qt::WA_AcceptDrops, on);

}

/*!
    \fn void QWidget::enabledChange(bool)

    \internal
    \obsolete
*/

/*!
    \fn void QWidget::paletteChange(const QPalette &)

    \internal
    \obsolete
*/

/*!
    \fn void QWidget::fontChange(const QFont &)

    \internal
    \obsolete
*/

/*!
    \fn void QWidget::windowActivationChange(bool)

    \internal
    \obsolete
*/

/*!
    \fn void QWidget::languageChange()

    \obsolete
*/

/*!
    \fn void QWidget::styleChange(QStyle& style)

    \internal
    \obsolete
*/

/*!
    Disables widget input events if \a disable is true; otherwise
    enables input events.

    See the \l enabled documentation for more information.

    \sa isEnabledTo(), QKeyEvent, QMouseEvent, changeEvent()
*/
void QWidget::setDisabled(bool disable)
{
    setEnabled(!disable);
}

/*!
    \property QWidget::frameGeometry
    \brief geometry of the widget relative to its parent including any
    window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa geometry() x() y() pos()
*/
QRect QWidget::frameGeometry() const
{
    Q_D(const QWidget);
    if (isWindow() && ! (windowType() == Qt::Popup)) {
        QRect fs = d->frameStrut();
        return QRect(data->crect.x() - fs.left(),
                     data->crect.y() - fs.top(),
                     data->crect.width() + fs.left() + fs.right(),
                     data->crect.height() + fs.top() + fs.bottom());
    }
    return data->crect;
}

/*!
    \property QWidget::x

    \brief the x coordinate of the widget relative to its parent including
    any window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property has a value of 0.

    \sa frameGeometry, y, pos
*/
int QWidget::x() const
{
    Q_D(const QWidget);
    if (isWindow() && ! (windowType() == Qt::Popup))
        return data->crect.x() - d->frameStrut().left();
    return data->crect.x();
}

/*!
    \property QWidget::y
    \brief the y coordinate of the widget relative to its parent and
    including any window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property has a value of 0.

    \sa frameGeometry, x, pos
*/
int QWidget::y() const
{
    Q_D(const QWidget);
    if (isWindow() && ! (windowType() == Qt::Popup))
        return data->crect.y() - d->frameStrut().top();
    return data->crect.y();
}

/*!
    \property QWidget::pos
    \brief the position of the widget within its parent widget

    If the widget is a window, the position is that of the widget on
    the desktop, including its frame.

    When changing the position, the widget, if visible, receives a
    move event (moveEvent()) immediately. If the widget is not
    currently visible, it is guaranteed to receive an event before it
    is shown.

    By default, this property contains a position that refers to the
    origin.

    \warning Calling move() or setGeometry() inside moveEvent() can
    lead to infinite recursion.

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    \sa frameGeometry, size x(), y()
*/
QPoint QWidget::pos() const
{
    Q_D(const QWidget);
    if (isWindow() && ! (windowType() == Qt::Popup)) {
        QRect fs = d->frameStrut();
        return QPoint(data->crect.x() - fs.left(), data->crect.y() - fs.top());
    }
    return data->crect.topLeft();
}

/*!
    \property QWidget::geometry
    \brief the geometry of the widget relative to its parent and
    excluding the window frame

    When changing the geometry, the widget, if visible, receives a
    move event (moveEvent()) and/or a resize event (resizeEvent())
    immediately. If the widget is not currently visible, it is
    guaranteed to receive appropriate events before it is shown.

    The size component is adjusted if it lies outside the range
    defined by minimumSize() and maximumSize().

    \warning Calling setGeometry() inside resizeEvent() or moveEvent()
    can lead to infinite recursion.

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa frameGeometry(), rect(), move(), resize(), moveEvent(),
        resizeEvent(), minimumSize(), maximumSize()
*/

/*!
    \property QWidget::normalGeometry

    \brief the geometry of the widget as it will appear when shown as
    a normal (not maximized or full screen) top-level widget

    For child widgets this property always holds an empty rectangle.

    By default, this property contains an empty rectangle.

    \sa QWidget::windowState(), QWidget::geometry
*/

/*!
    \property QWidget::size
    \brief the size of the widget excluding any window frame

    If the widget is visible when it is being resized, it receives a resize event
    (resizeEvent()) immediately. If the widget is not currently
    visible, it is guaranteed to receive an event before it is shown.

    The size is adjusted if it lies outside the range defined by
    minimumSize() and maximumSize().

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \warning Calling resize() or setGeometry() inside resizeEvent() can
    lead to infinite recursion.

    \note Setting the size to \c{QSize(0, 0)} will cause the widget to not
    appear on screen. This also applies to windows.

    \sa pos, geometry, minimumSize, maximumSize, resizeEvent(), adjustSize()
*/

/*!
    \property QWidget::width
    \brief the width of the widget excluding any window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    \note Do not use this function to find the width of a screen on
    a \l{QDesktopWidget}{multiple screen desktop}. Read
    \l{QDesktopWidget#Screen Geometry}{this note} for details.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa geometry, height, size
*/

/*!
    \property QWidget::height
    \brief the height of the widget excluding any window frame

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    \note Do not use this function to find the height of a screen
    on a \l{QDesktopWidget}{multiple screen desktop}. Read
    \l{QDesktopWidget#Screen Geometry}{this note} for details.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa geometry, width, size
*/

/*!
    \property QWidget::rect
    \brief the internal geometry of the widget excluding any window
    frame

    The rect property equals QRect(0, 0, width(), height()).

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    By default, this property contains a value that depends on the user's
    platform and screen geometry.

    \sa size
*/


QRect QWidget::normalGeometry() const
{
    Q_D(const QWidget);
    if (!d->extra || !d->extra->topextra)
        return QRect();

    if (!isMaximized() && !isFullScreen())
        return geometry();

    return d->topData()->normalGeometry;
}


/*!
    \property QWidget::childrenRect
    \brief the bounding rectangle of the widget's children

    Hidden children are excluded.

    By default, for a widget with no children, this property contains a
    rectangle with zero width and height located at the origin.

    \sa childrenRegion() geometry()
*/

QRect QWidget::childrenRect() const
{
    Q_D(const QWidget);
    QRect r(0, 0, 0, 0);
    for (int i = 0; i < d->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
        if (w && !w->isWindow() && !w->isHidden())
            r |= w->geometry();
    }
    return r;
}

/*!
    \property QWidget::childrenRegion
    \brief the combined region occupied by the widget's children

    Hidden children are excluded.

    By default, for a widget with no children, this property contains an
    empty region.

    \sa childrenRect() geometry() mask()
*/

QRegion QWidget::childrenRegion() const
{
    Q_D(const QWidget);
    QRegion r;
    for (int i = 0; i < d->children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
        if (w && !w->isWindow() && !w->isHidden()) {
            QRegion mask = w->mask();
            if (mask.isEmpty())
                r |= w->geometry();
            else
                r |= mask.translated(w->pos());
        }
    }
    return r;
}


/*!
    \property QWidget::minimumSize
    \brief the widget's minimum size

    The widget cannot be resized to a smaller size than the minimum
    widget size. The widget's size is forced to the minimum size if
    the current size is smaller.

    The minimum size set by this function will override the minimum size
    defined by QLayout. In order to unset the minimum size, use a
    value of \c{QSize(0, 0)}.

    By default, this property contains a size with zero width and height.

    \sa minimumWidth, minimumHeight, maximumSize, sizeIncrement
*/

QSize QWidget::minimumSize() const
{
    Q_D(const QWidget);
    return d->extra ? QSize(d->extra->minw, d->extra->minh) : QSize(0, 0);
}

/*!
    \property QWidget::maximumSize
    \brief the widget's maximum size in pixels

    The widget cannot be resized to a larger size than the maximum
    widget size.

    By default, this property contains a size in which both width and height
    have values of 16777215.

    \note The definition of the \c QWIDGETSIZE_MAX macro limits the maximum size
    of widgets.

    \sa maximumWidth, maximumHeight, minimumSize, sizeIncrement
*/

QSize QWidget::maximumSize() const
{
    Q_D(const QWidget);
    return d->extra ? QSize(d->extra->maxw, d->extra->maxh)
                 : QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}


/*!
    \property QWidget::minimumWidth
    \brief the widget's minimum width in pixels

    This property corresponds to the width held by the \l minimumSize property.

    By default, this property has a value of 0.

    \sa minimumSize, minimumHeight
*/

/*!
    \property QWidget::minimumHeight
    \brief the widget's minimum height in pixels

    This property corresponds to the height held by the \l minimumSize property.

    By default, this property has a value of 0.

    \sa minimumSize, minimumWidth
*/

/*!
    \property QWidget::maximumWidth
    \brief the widget's maximum width in pixels

    This property corresponds to the width held by the \l maximumSize property.

    By default, this property contains a value of 16777215.

    \note The definition of the \c QWIDGETSIZE_MAX macro limits the maximum size
    of widgets.

    \sa maximumSize, maximumHeight
*/

/*!
    \property QWidget::maximumHeight
    \brief the widget's maximum height in pixels

    This property corresponds to the height held by the \l maximumSize property.

    By default, this property contains a value of 16777215.

    \note The definition of the \c QWIDGETSIZE_MAX macro limits the maximum size
    of widgets.

    \sa maximumSize, maximumWidth
*/

/*!
    \property QWidget::sizeIncrement
    \brief the size increment of the widget

    When the user resizes the window, the size will move in steps of
    sizeIncrement().width() pixels horizontally and
    sizeIncrement.height() pixels vertically, with baseSize() as the
    basis. Preferred widget sizes are for non-negative integers \e i
    and \e j:
    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 2

    Note that while you can set the size increment for all widgets, it
    only affects windows.

    By default, this property contains a size with zero width and height.

    \warning The size increment has no effect under Windows, and may
    be disregarded by the window manager on X11.

    \sa size, minimumSize, maximumSize
*/
QSize QWidget::sizeIncrement() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra)
        ? QSize(d->extra->topextra->incw, d->extra->topextra->inch)
        : QSize(0, 0);
}

/*!
    \property QWidget::baseSize
    \brief the base size of the widget

    The base size is used to calculate a proper widget size if the
    widget defines sizeIncrement().

    By default, for a newly-created widget, this property contains a size with
    zero width and height.

    \sa setSizeIncrement()
*/

QSize QWidget::baseSize() const
{
    Q_D(const QWidget);
    return (d->extra != 0 && d->extra->topextra != 0)
        ? QSize(d->extra->topextra->basew, d->extra->topextra->baseh)
        : QSize(0, 0);
}

bool QWidgetPrivate::setMinimumSize_helper(int &minw, int &minh)
{
    Q_Q(QWidget);

#ifdef Q_WS_QWS
    if (q->isWindow()) {
        const QRect maxWindowRect = QApplication::desktop()->availableGeometry(QApplication::desktop()->screenNumber(q));
        if (!maxWindowRect.isEmpty()) {
            // ### This is really just a work-around. Layout shouldn't be
            // asking for minimum sizes bigger than the screen.
            if (minw > maxWindowRect.width())
                minw = maxWindowRect.width();
            if (minh > maxWindowRect.height())
                minh = maxWindowRect.height();
        }
    }
#endif
    int mw = minw, mh = minh;
    if (mw == QWIDGETSIZE_MAX)
        mw = 0;
    if (mh == QWIDGETSIZE_MAX)
        mh = 0;
    if (minw > QWIDGETSIZE_MAX || minh > QWIDGETSIZE_MAX) {
        qWarning("QWidget::setMinimumSize: (%s/%s) "
                "The largest allowed size is (%d,%d)",
                 q->objectName().toLocal8Bit().data(), q->metaObject()->className(), QWIDGETSIZE_MAX,
                QWIDGETSIZE_MAX);
        minw = mw = qMin<int>(minw, QWIDGETSIZE_MAX);
        minh = mh = qMin<int>(minh, QWIDGETSIZE_MAX);
    }
    if (minw < 0 || minh < 0) {
        qWarning("QWidget::setMinimumSize: (%s/%s) Negative sizes (%d,%d) "
                "are not possible",
                q->objectName().toLocal8Bit().data(), q->metaObject()->className(), minw, minh);
        minw = mw = qMax(minw, 0);
        minh = mh = qMax(minh, 0);
    }
    createExtra();
    if (extra->minw == mw && extra->minh == mh)
        return false;
    extra->minw = mw;
    extra->minh = mh;
    extra->explicitMinSize = (mw ? Qt::Horizontal : 0) | (mh ? Qt::Vertical : 0);
    return true;
}

/*!
    \overload

    This function corresponds to setMinimumSize(QSize(minw, minh)).
    Sets the minimum width to \a minw and the minimum height to \a
    minh.
*/

void QWidget::setMinimumSize(int minw, int minh)
{
    Q_D(QWidget);
    if (!d->setMinimumSize_helper(minw, minh))
        return;

    if (isWindow())
        d->setConstraints_sys();
    if (minw > width() || minh > height()) {
        bool resized = testAttribute(Qt::WA_Resized);
        bool maximized = isMaximized();
        resize(qMax(minw,width()), qMax(minh,height()));
        setAttribute(Qt::WA_Resized, resized); //not a user resize
        if (maximized)
            data->window_state = data->window_state | Qt::WindowMaximized;
    }
#ifndef QT_NO_GRAPHICSVIEW
    if (d->extra) {
        if (d->extra->proxyWidget)
            d->extra->proxyWidget->setMinimumSize(minw, minh);
    }
#endif
    d->updateGeometry_helper(d->extra->minw == d->extra->maxw && d->extra->minh == d->extra->maxh);
}

bool QWidgetPrivate::setMaximumSize_helper(int &maxw, int &maxh)
{
    Q_Q(QWidget);
    if (maxw > QWIDGETSIZE_MAX || maxh > QWIDGETSIZE_MAX) {
        qWarning("QWidget::setMaximumSize: (%s/%s) "
                "The largest allowed size is (%d,%d)",
                 q->objectName().toLocal8Bit().data(), q->metaObject()->className(), QWIDGETSIZE_MAX,
                QWIDGETSIZE_MAX);
        maxw = qMin<int>(maxw, QWIDGETSIZE_MAX);
        maxh = qMin<int>(maxh, QWIDGETSIZE_MAX);
    }
    if (maxw < 0 || maxh < 0) {
        qWarning("QWidget::setMaximumSize: (%s/%s) Negative sizes (%d,%d) "
                "are not possible",
                q->objectName().toLocal8Bit().data(), q->metaObject()->className(), maxw, maxh);
        maxw = qMax(maxw, 0);
        maxh = qMax(maxh, 0);
    }
    createExtra();
    if (extra->maxw == maxw && extra->maxh == maxh)
        return false;
    extra->maxw = maxw;
    extra->maxh = maxh;
    extra->explicitMaxSize = (maxw != QWIDGETSIZE_MAX ? Qt::Horizontal : 0) |
                             (maxh != QWIDGETSIZE_MAX ? Qt::Vertical : 0);
    return true;
}

/*!
    \overload

    This function corresponds to setMaximumSize(QSize(\a maxw, \a
    maxh)). Sets the maximum width to \a maxw and the maximum height
    to \a maxh.
*/
void QWidget::setMaximumSize(int maxw, int maxh)
{
    Q_D(QWidget);
    if (!d->setMaximumSize_helper(maxw, maxh))
        return;

    if (isWindow())
        d->setConstraints_sys();
    if (maxw < width() || maxh < height()) {
        bool resized = testAttribute(Qt::WA_Resized);
        resize(qMin(maxw,width()), qMin(maxh,height()));
        setAttribute(Qt::WA_Resized, resized); //not a user resize
    }

#ifndef QT_NO_GRAPHICSVIEW
    if (d->extra) {
        if (d->extra->proxyWidget)
            d->extra->proxyWidget->setMaximumSize(maxw, maxh);
    }
#endif

    d->updateGeometry_helper(d->extra->minw == d->extra->maxw && d->extra->minh == d->extra->maxh);
}

/*!
    \overload

    Sets the x (width) size increment to \a w and the y (height) size
    increment to \a h.
*/
void QWidget::setSizeIncrement(int w, int h)
{
    Q_D(QWidget);
    d->createTLExtra();
    QTLWExtra* x = d->topData();
    if (x->incw == w && x->inch == h)
        return;
    x->incw = w;
    x->inch = h;
    if (isWindow())
        d->setConstraints_sys();
}

/*!
    \overload

    This corresponds to setBaseSize(QSize(\a basew, \a baseh)). Sets
    the widgets base size to width \a basew and height \a baseh.
*/
void QWidget::setBaseSize(int basew, int baseh)
{
    Q_D(QWidget);
    d->createTLExtra();
    QTLWExtra* x = d->topData();
    if (x->basew == basew && x->baseh == baseh)
        return;
    x->basew = basew;
    x->baseh = baseh;
    if (isWindow())
        d->setConstraints_sys();
}

/*!
    Sets both the minimum and maximum sizes of the widget to \a s,
    thereby preventing it from ever growing or shrinking.

    This will override the default size constraints set by QLayout.

    To remove constraints, set the size to QWIDGETSIZE_MAX.

    Alternatively, if you want the widget to have a
    fixed size based on its contents, you can call
    QLayout::setSizeConstraint(QLayout::SetFixedSize);

    \sa maximumSize, minimumSize
*/

void QWidget::setFixedSize(const QSize & s)
{
    setFixedSize(s.width(), s.height());
}


/*!
    \fn void QWidget::setFixedSize(int w, int h)
    \overload

    Sets the width of the widget to \a w and the height to \a h.
*/

void QWidget::setFixedSize(int w, int h)
{
    Q_D(QWidget);
#ifdef Q_WS_QWS
    // temporary fix for 4.3.x.
    // Should move the embedded spesific contraints in setMinimumSize_helper into QLayout
    int tmpW = w;
    int tmpH = h;
    bool minSizeSet = d->setMinimumSize_helper(tmpW, tmpH);
#else
    bool minSizeSet = d->setMinimumSize_helper(w, h);
#endif
    bool maxSizeSet = d->setMaximumSize_helper(w, h);
    if (!minSizeSet && !maxSizeSet)
        return;

    if (isWindow())
        d->setConstraints_sys();
    else
        d->updateGeometry_helper(true);

    if (w != QWIDGETSIZE_MAX || h != QWIDGETSIZE_MAX)
        resize(w, h);
}

void QWidget::setMinimumWidth(int w)
{
    Q_D(QWidget);
    d->createExtra();
    uint expl = d->extra->explicitMinSize | (w ? Qt::Horizontal : 0);
    setMinimumSize(w, minimumSize().height());
    d->extra->explicitMinSize = expl;
}

void QWidget::setMinimumHeight(int h)
{
    Q_D(QWidget);
    d->createExtra();
    uint expl = d->extra->explicitMinSize | (h ? Qt::Vertical : 0);
    setMinimumSize(minimumSize().width(), h);
    d->extra->explicitMinSize = expl;
}

void QWidget::setMaximumWidth(int w)
{
    Q_D(QWidget);
    d->createExtra();
    uint expl = d->extra->explicitMaxSize | (w == QWIDGETSIZE_MAX ? 0 : Qt::Horizontal);
    setMaximumSize(w, maximumSize().height());
    d->extra->explicitMaxSize = expl;
}

void QWidget::setMaximumHeight(int h)
{
    Q_D(QWidget);
    d->createExtra();
    uint expl = d->extra->explicitMaxSize | (h == QWIDGETSIZE_MAX ? 0 : Qt::Vertical);
    setMaximumSize(maximumSize().width(), h);
    d->extra->explicitMaxSize = expl;
}

/*!
    Sets both the minimum and maximum width of the widget to \a w
    without changing the heights. Provided for convenience.

    \sa sizeHint() minimumSize() maximumSize() setFixedSize()
*/

void QWidget::setFixedWidth(int w)
{
    Q_D(QWidget);
    d->createExtra();
    uint explMin = d->extra->explicitMinSize | Qt::Horizontal;
    uint explMax = d->extra->explicitMaxSize | Qt::Horizontal;
    setMinimumSize(w, minimumSize().height());
    setMaximumSize(w, maximumSize().height());
    d->extra->explicitMinSize = explMin;
    d->extra->explicitMaxSize = explMax;
}


/*!
    Sets both the minimum and maximum heights of the widget to \a h
    without changing the widths. Provided for convenience.

    \sa sizeHint() minimumSize() maximumSize() setFixedSize()
*/

void QWidget::setFixedHeight(int h)
{
    Q_D(QWidget);
    d->createExtra();
    uint explMin = d->extra->explicitMinSize | Qt::Vertical;
    uint explMax = d->extra->explicitMaxSize | Qt::Vertical;
    setMinimumSize(minimumSize().width(), h);
    setMaximumSize(maximumSize().width(), h);
    d->extra->explicitMinSize = explMin;
    d->extra->explicitMaxSize = explMax;
}


/*!
    Translates the widget coordinate \a pos to the coordinate system
    of \a parent. The \a parent must not be 0 and must be a parent
    of the calling widget.

    \sa mapFrom() mapToParent() mapToGlobal() underMouse()
*/

QPoint QWidget::mapTo(QWidget * parent, const QPoint & pos) const
{
    QPoint p = pos;
    if (parent) {
        const QWidget * w = this;
        while (w != parent) {
            Q_ASSERT_X(w, "QWidget::mapTo(QWidget *parent, const QPoint &pos)",
                       "parent must be in parent hierarchy");
            p = w->mapToParent(p);
            w = w->parentWidget();
        }
    }
    return p;
}


/*!
    Translates the widget coordinate \a pos from the coordinate system
    of \a parent to this widget's coordinate system. The \a parent
    must not be 0 and must be a parent of the calling widget.

    \sa mapTo() mapFromParent() mapFromGlobal() underMouse()
*/

QPoint QWidget::mapFrom(QWidget * parent, const QPoint & pos) const
{
    QPoint p(pos);
    if (parent) {
        const QWidget * w = this;
        while (w != parent) {
            Q_ASSERT_X(w, "QWidget::mapFrom(QWidget *parent, const QPoint &pos)",
                       "parent must be in parent hierarchy");

            p = w->mapFromParent(p);
            w = w->parentWidget();
        }
    }
    return p;
}


/*!
    Translates the widget coordinate \a pos to a coordinate in the
    parent widget.

    Same as mapToGlobal() if the widget has no parent.

    \sa mapFromParent() mapTo() mapToGlobal() underMouse()
*/

QPoint QWidget::mapToParent(const QPoint &pos) const
{
    return pos + data->crect.topLeft();
}

/*!
    Translates the parent widget coordinate \a pos to widget
    coordinates.

    Same as mapFromGlobal() if the widget has no parent.

    \sa mapToParent() mapFrom() mapFromGlobal() underMouse()
*/

QPoint QWidget::mapFromParent(const QPoint &pos) const
{
    return pos - data->crect.topLeft();
}


/*!
    Returns the window for this widget, i.e. the next ancestor widget
    that has (or could have) a window-system frame.

    If the widget is a window, the widget itself is returned.

    Typical usage is changing the window title:

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 3

    \sa isWindow()
*/

QWidget *QWidget::window() const
{
    QWidget *w = (QWidget *)this;
    QWidget *p = w->parentWidget();
    while (!w->isWindow() && p) {
        w = p;
        p = p->parentWidget();
    }
    return w;
}

/*!
    \since 4.4

    Returns the native parent for this widget, i.e. the next ancestor widget
    that has a system identifier, or 0 if it does not have any native parent.

    \sa effectiveWinId()
*/
QWidget *QWidget::nativeParentWidget() const
{
    QWidget *parent = parentWidget();
    while (parent && !parent->internalWinId())
        parent = parent->parentWidget();
    return parent;
}

/*! \fn QWidget *QWidget::topLevelWidget() const
    \obsolete

    Use window() instead.
*/

#ifdef QT3_SUPPORT
/*!
    Returns the color role used for painting the widget's background.

    Use QPalette(backgroundRole(()) instead.
*/
Qt::BackgroundMode QWidget::backgroundMode() const
{
    if (testAttribute(Qt::WA_NoSystemBackground))
        return Qt::NoBackground;
    switch(backgroundRole()) {
    case QPalette::WindowText:
        return Qt::PaletteForeground;
    case QPalette::Button:
        return Qt::PaletteButton;
    case QPalette::Light:
        return Qt::PaletteLight;
    case QPalette::Midlight:
        return Qt::PaletteMidlight;
    case QPalette::Dark:
        return Qt::PaletteDark;
    case QPalette::Mid:
        return Qt::PaletteMid;
    case QPalette::Text:
        return Qt::PaletteText;
    case QPalette::BrightText:
        return Qt::PaletteBrightText;
    case QPalette::Base:
        return Qt::PaletteBase;
    case QPalette::Window:
        return Qt::PaletteBackground;
    case QPalette::Shadow:
        return Qt::PaletteShadow;
    case QPalette::Highlight:
        return Qt::PaletteHighlight;
    case QPalette::HighlightedText:
        return Qt::PaletteHighlightedText;
    case QPalette::ButtonText:
        return Qt::PaletteButtonText;
    case QPalette::Link:
        return Qt::PaletteLink;
    case QPalette::LinkVisited:
        return Qt::PaletteLinkVisited;
    default:
        break;
    }
    return Qt::NoBackground;
}

/*!
    \fn void QWidget::setBackgroundMode(Qt::BackgroundMode
    widgetBackground, Qt::BackgroundMode paletteBackground)

    Sets the color role used for painting the widget's background to
    background mode \a widgetBackground. The \a paletteBackground mode
    parameter is ignored.
*/
void QWidget::setBackgroundMode(Qt::BackgroundMode m, Qt::BackgroundMode)
{
    Q_D(QWidget);
    if(m == Qt::NoBackground) {
        setAttribute(Qt::WA_NoSystemBackground, true);
        return;
    }
    setAttribute(Qt::WA_NoSystemBackground, false);
    d->fg_role = QPalette::NoRole;
    QPalette::ColorRole role = d->bg_role;
    switch(m) {
    case Qt::FixedColor:
    case Qt::FixedPixmap:
        break;
    case Qt::PaletteForeground:
        role = QPalette::WindowText;
        break;
    case Qt::PaletteButton:
        role = QPalette::Button;
        break;
    case Qt::PaletteLight:
        role = QPalette::Light;
        break;
    case Qt::PaletteMidlight:
        role = QPalette::Midlight;
        break;
    case Qt::PaletteDark:
        role = QPalette::Dark;
        break;
    case Qt::PaletteMid:
        role = QPalette::Mid;
        break;
    case Qt::PaletteText:
        role = QPalette::Text;
        break;
    case Qt::PaletteBrightText:
        role = QPalette::BrightText;
        break;
    case Qt::PaletteBase:
        role = QPalette::Base;
        break;
    case Qt::PaletteBackground:
        role = QPalette::Window;
        break;
    case Qt::PaletteShadow:
        role = QPalette::Shadow;
        break;
    case Qt::PaletteHighlight:
        role = QPalette::Highlight;
        break;
    case Qt::PaletteHighlightedText:
        role = QPalette::HighlightedText;
        break;
    case Qt::PaletteButtonText:
        role = QPalette::ButtonText;
        break;
    case Qt::PaletteLink:
        role = QPalette::Link;
        break;
    case Qt::PaletteLinkVisited:
        role = QPalette::LinkVisited;
        break;
    case Qt::X11ParentRelative:
        d->fg_role = role = QPalette::NoRole;
    default:
        break;
    }
    setBackgroundRole(role);
}

/*!
    The widget mapper is no longer part of the public API.
*/
QT3_SUPPORT QWidgetMapper *QWidget::wmapper() { return QWidgetPrivate::mapper; }

#endif


/*!
  Returns the background role of the widget.

  The background role defines the brush from the widget's \l palette that
  is used to render the background.

  If no explicit background role is set, the widget inherts its parent
  widget's background role.

  \sa setBackgroundRole(), foregroundRole()
 */
QPalette::ColorRole QWidget::backgroundRole() const
{

    const QWidget *w = this;
    do {
        QPalette::ColorRole role = w->d_func()->bg_role;
        if (role != QPalette::NoRole)
            return role;
        if (w->isWindow() || w->windowType() == Qt::SubWindow)
            break;
        w = w->parentWidget();
    } while (w);
    return QPalette::Window;
}

/*!
  Sets the background role of the widget to \a role.

  The background role defines the brush from the widget's \l palette that
  is used to render the background.

  If \a role is QPalette::NoRole, then the widget inherits its
  parent's background role.

  Note that styles are free to choose any color from the palette.
  You can modify the palette or set a style sheet if you don't
  achieve the result you want with setBackgroundRole().

  \sa backgroundRole(), foregroundRole()
 */

void QWidget::setBackgroundRole(QPalette::ColorRole role)
{
    Q_D(QWidget);
    d->bg_role = role;
    d->updateSystemBackground();
    d->propagatePaletteChange();
    d->updateIsOpaque();
}

/*!
  Returns the foreground role.

  The foreground role defines the color from the widget's \l palette that
  is used to draw the foreground.

  If no explicit foreground role is set, the function returns a role
  that contrasts with the background role.

  \sa setForegroundRole(), backgroundRole()
 */
QPalette::ColorRole QWidget::foregroundRole() const
{
    Q_D(const QWidget);
    QPalette::ColorRole rl = QPalette::ColorRole(d->fg_role);
    if (rl != QPalette::NoRole)
        return rl;
    QPalette::ColorRole role = QPalette::WindowText;
    switch (backgroundRole()) {
    case QPalette::Button:
        role = QPalette::ButtonText;
        break;
    case QPalette::Base:
        role = QPalette::Text;
        break;
    case QPalette::Dark:
    case QPalette::Shadow:
        role = QPalette::Light;
        break;
    case QPalette::Highlight:
        role = QPalette::HighlightedText;
        break;
    case QPalette::ToolTipBase:
        role = QPalette::ToolTipText;
        break;
    default:
        ;
    }
    return role;
}

/*!
  Sets the foreground role of the widget to \a role.

  The foreground role defines the color from the widget's \l palette that
  is used to draw the foreground.

  If \a role is QPalette::NoRole, the widget uses a foreground role
  that contrasts with the background role.

  Note that styles are free to choose any color from the palette.
  You can modify the palette or set a style sheet if you don't
  achieve the result you want with setForegroundRole().

  \sa foregroundRole(), backgroundRole()
 */
void QWidget::setForegroundRole(QPalette::ColorRole role)
{
    Q_D(QWidget);
    d->fg_role = role;
    d->updateSystemBackground();
    d->propagatePaletteChange();
}

/*!
    \property QWidget::palette
    \brief the widget's palette

    This property describes the widget's palette. The palette is used by the
    widget's style when rendering standard components, and is available as a
    means to ensure that custom widgets can maintain consistency with the
    native platform's look and feel. It's common that different platforms, or
    different styles, have different palettes.

    When you assign a new palette to a widget, the color roles from this
    palette are combined with the widget's default palette to form the
    widget's final palette. The palette entry for the widget's background role
    is used to fill the widget's background (see QWidget::autoFillBackground),
    and the foreground role initializes QPainter's pen.

    The default depends on the system environment. QApplication maintains a
    system/theme palette which serves as a default for all widgets.  There may
    also be special palette defaults for certain types of widgets (e.g., on
    Windows XP and Vista, all classes that derive from QMenuBar have a special
    default palette). You can also define default palettes for widgets
    yourself by passing a custom palette and the name of a widget to
    QApplication::setPalette(). Finally, the style always has the option of
    polishing the palette as it's assigned (see QStyle::polish()).

    QWidget propagates explicit palette roles from parent to child. If you
    assign a brush or color to a specific role on a palette and assign that
    palette to a widget, that role will propagate to all the widget's
    children, overriding any system defaults for that role. Note that palettes
    by default don't propagate to windows (see isWindow()) unless the
    Qt::WA_WindowPropagation attribute is enabled.

    QWidget's palette propagation is similar to its font propagation.

    The current style, which is used to render the content of all standard Qt
    widgets, is free to choose colors and brushes from the widget palette, or
    in some cases, to ignore the palette (partially, or completely). In
    particular, certain styles like GTK style, Mac style, Windows XP, and
    Vista style, depend on third party APIs to render the content of widgets,
    and these styles typically do not follow the palette. Because of this,
    assigning roles to a widget's palette is not guaranteed to change the
    appearance of the widget. Instead, you may choose to apply a \l
    styleSheet. You can refer to our Knowledge Base article
    \l{http://qt.nokia.com/developer/knowledgebase/22}{here} for more
    information.

    \warning Do not use this function in conjunction with \l{Qt Style Sheets}.
    When using style sheets, the palette of a widget can be customized using
    the "color", "background-color", "selection-color",
    "selection-background-color" and "alternate-background-color".

    \sa QApplication::palette(), QWidget::font()
*/
const QPalette &QWidget::palette() const
{
    if (!isEnabled()) {
        data->pal.setCurrentColorGroup(QPalette::Disabled);
    } else if ((!isVisible() || isActiveWindow())
#if defined(Q_OS_WIN) && !defined(Q_WS_WINCE)
        && !QApplicationPrivate::isBlockedByModal(const_cast<QWidget *>(this))
#endif
        ) {
        data->pal.setCurrentColorGroup(QPalette::Active);
    } else {
#ifdef Q_WS_MAC
        extern bool qt_mac_can_clickThrough(const QWidget *); //qwidget_mac.cpp
        if (qt_mac_can_clickThrough(this))
            data->pal.setCurrentColorGroup(QPalette::Active);
        else
#endif
            data->pal.setCurrentColorGroup(QPalette::Inactive);
    }
    return data->pal;
}

void QWidget::setPalette(const QPalette &palette)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_SetPalette, palette.resolve() != 0);

    // Determine which palette is inherited from this widget's ancestors and
    // QApplication::palette, resolve this against \a palette (attributes from
    // the inherited palette are copied over this widget's palette). Then
    // propagate this palette to this widget's children.
    QPalette naturalPalette = d->naturalWidgetPalette(d->inheritedPaletteResolveMask);
    QPalette resolvedPalette = palette.resolve(naturalPalette);
    d->setPalette_helper(resolvedPalette);
}

/*!
    \internal

    Returns the palette that the widget \a w inherits from its ancestors and
    QApplication::palette. \a inheritedMask is the combination of the widget's
    ancestors palette request masks (i.e., which attributes from the parent
    widget's palette are implicitly imposed on this widget by the user). Note
    that this font does not take into account the palette set on \a w itself.
*/
QPalette QWidgetPrivate::naturalWidgetPalette(uint inheritedMask) const
{
    Q_Q(const QWidget);
    QPalette naturalPalette = QApplication::palette(q);
    if (!q->testAttribute(Qt::WA_StyleSheet)
        && (!q->isWindow() || q->testAttribute(Qt::WA_WindowPropagation)
#ifndef QT_NO_GRAPHICSVIEW
            || (extra && extra->proxyWidget)
#endif //QT_NO_GRAPHICSVIEW
            )) {
        if (QWidget *p = q->parentWidget()) {
            if (!p->testAttribute(Qt::WA_StyleSheet)) {
                if (!naturalPalette.isCopyOf(QApplication::palette())) {
                    QPalette inheritedPalette = p->palette();
                    inheritedPalette.resolve(inheritedMask);
                    naturalPalette = inheritedPalette.resolve(naturalPalette);
                } else {
                    naturalPalette = p->palette();
                }
            }
        }
#ifndef QT_NO_GRAPHICSVIEW
        else if (extra && extra->proxyWidget) {
            QPalette inheritedPalette = extra->proxyWidget->palette();
            inheritedPalette.resolve(inheritedMask);
            naturalPalette = inheritedPalette.resolve(naturalPalette);
        }
#endif //QT_NO_GRAPHICSVIEW
    }
    naturalPalette.resolve(0);
    return naturalPalette;
}
/*!
    \internal

    Determine which palette is inherited from this widget's ancestors and
    QApplication::palette, resolve this against this widget's palette
    (attributes from the inherited palette are copied over this widget's
    palette). Then propagate this palette to this widget's children.
*/
void QWidgetPrivate::resolvePalette()
{
    QPalette naturalPalette = naturalWidgetPalette(inheritedPaletteResolveMask);
    QPalette resolvedPalette = data.pal.resolve(naturalPalette);
    setPalette_helper(resolvedPalette);
}

void QWidgetPrivate::setPalette_helper(const QPalette &palette)
{
    Q_Q(QWidget);
    if (data.pal == palette && data.pal.resolve() == palette.resolve())
        return;
    data.pal = palette;
    updateSystemBackground();
    propagatePaletteChange();
    updateIsOpaque();
    q->update();
    updateIsOpaque();
}

/*!
    \property QWidget::font
    \brief the font currently set for the widget

    This property describes the widget's requested font. The font is used by
    the widget's style when rendering standard components, and is available as
    a means to ensure that custom widgets can maintain consistency with the
    native platform's look and feel. It's common that different platforms, or
    different styles, define different fonts for an application.

    When you assign a new font to a widget, the properties from this font are
    combined with the widget's default font to form the widget's final
    font. You can call fontInfo() to get a copy of the widget's final
    font. The final font is also used to initialize QPainter's font.

    The default depends on the system environment. QApplication maintains a
    system/theme font which serves as a default for all widgets.  There may
    also be special font defaults for certain types of widgets. You can also
    define default fonts for widgets yourself by passing a custom font and the
    name of a widget to QApplication::setFont(). Finally, the font is matched
    against Qt's font database to find the best match.

    QWidget propagates explicit font properties from parent to child. If you
    change a specific property on a font and assign that font to a widget,
    that property will propagate to all the widget's children, overriding any
    system defaults for that property. Note that fonts by default don't
    propagate to windows (see isWindow()) unless the Qt::WA_WindowPropagation
    attribute is enabled.

    QWidget's font propagation is similar to its palette propagation.

    The current style, which is used to render the content of all standard Qt
    widgets, is free to choose to use the widget font, or in some cases, to
    ignore it (partially, or completely). In particular, certain styles like
    GTK style, Mac style, Windows XP, and Vista style, apply special
    modifications to the widget font to match the platform's native look and
    feel. Because of this, assigning properties to a widget's font is not
    guaranteed to change the appearance of the widget. Instead, you may choose
    to apply a \l styleSheet.

    \note If \l{Qt Style Sheets} are used on the same widget as setFont(),
    style sheets will take precedence if the settings conflict.

    \sa fontInfo(), fontMetrics()
*/

void QWidget::setFont(const QFont &font)
{
    Q_D(QWidget);

#ifndef QT_NO_STYLE_STYLESHEET
    const QStyleSheetStyle* style;
    if (d->extra && (style = qobject_cast<const QStyleSheetStyle*>(d->extra->style))) {
        style->saveWidgetFont(this, font);
    }
#endif

    setAttribute(Qt::WA_SetFont, font.resolve() != 0);

    // Determine which font is inherited from this widget's ancestors and
    // QApplication::font, resolve this against \a font (attributes from the
    // inherited font are copied over). Then propagate this font to this
    // widget's children.
    QFont naturalFont = d->naturalWidgetFont(d->inheritedFontResolveMask);
    QFont resolvedFont = font.resolve(naturalFont);
    d->setFont_helper(resolvedFont);
}

/*
    \internal

    Returns the font that the widget \a w inherits from its ancestors and
    QApplication::font. \a inheritedMask is the combination of the widget's
    ancestors font request masks (i.e., which attributes from the parent
    widget's font are implicitly imposed on this widget by the user). Note
    that this font does not take into account the font set on \a w itself.

    ### Stylesheet has a different font propagation mechanism. When a stylesheet
        is applied, fonts are not propagated anymore
*/
QFont QWidgetPrivate::naturalWidgetFont(uint inheritedMask) const
{
    Q_Q(const QWidget);
    QFont naturalFont = QApplication::font(q);
    if (!q->testAttribute(Qt::WA_StyleSheet)
        && (!q->isWindow() || q->testAttribute(Qt::WA_WindowPropagation)
#ifndef QT_NO_GRAPHICSVIEW
            || (extra && extra->proxyWidget)
#endif //QT_NO_GRAPHICSVIEW
            )) {
        if (QWidget *p = q->parentWidget()) {
            if (!p->testAttribute(Qt::WA_StyleSheet)) {
                if (!naturalFont.isCopyOf(QApplication::font())) {
                    QFont inheritedFont = p->font();
                    inheritedFont.resolve(inheritedMask);
                    naturalFont = inheritedFont.resolve(naturalFont);
                } else {
                    naturalFont = p->font();
                }
            }
        }
#ifndef QT_NO_GRAPHICSVIEW
        else if (extra && extra->proxyWidget) {
            QFont inheritedFont = extra->proxyWidget->font();
            inheritedFont.resolve(inheritedMask);
            naturalFont = inheritedFont.resolve(naturalFont);
        }
#endif //QT_NO_GRAPHICSVIEW
    }
    naturalFont.resolve(0);
    return naturalFont;
}

/*!
    \internal

    Determine which font is implicitly imposed on this widget by its ancestors
    and QApplication::font, resolve this against its own font (attributes from
    the implicit font are copied over). Then propagate this font to this
    widget's children.
*/
void QWidgetPrivate::resolveFont()
{
    QFont naturalFont = naturalWidgetFont(inheritedFontResolveMask);
    QFont resolvedFont = data.fnt.resolve(naturalFont);
    setFont_helper(resolvedFont);
}

/*!
    \internal

    Assign \a font to this widget, and propagate it to all children, except
    style sheet widgets (handled differently) and windows that don't enable
    window propagation.  \a implicitMask is the union of all ancestor widgets'
    font request masks, and determines which attributes from this widget's
    font should propagate.
*/
void QWidgetPrivate::updateFont(const QFont &font)
{
    Q_Q(QWidget);
#ifndef QT_NO_STYLE_STYLESHEET
    const QStyleSheetStyle* cssStyle;
    cssStyle = extra ? qobject_cast<const QStyleSheetStyle*>(extra->style) : 0;
#endif

#ifdef QT3_SUPPORT
    QFont old = data.fnt;
#endif
    data.fnt = QFont(font, q);
#if defined(Q_WS_X11)
    // make sure the font set on this widget is associated with the correct screen
    data.fnt.x11SetScreen(xinfo.screen());
#endif
    // Combine new mask with natural mask and propagate to children.
#ifndef QT_NO_GRAPHICSVIEW
    if (!q->parentWidget() && extra && extra->proxyWidget) {
        QGraphicsProxyWidget *p = extra->proxyWidget;
        inheritedFontResolveMask = p->d_func()->inheritedFontResolveMask | p->font().resolve();
    } else
#endif //QT_NO_GRAPHICSVIEW
    if (q->isWindow() && !q->testAttribute(Qt::WA_WindowPropagation)) {
        inheritedFontResolveMask = 0;
    }
    uint newMask = data.fnt.resolve() | inheritedFontResolveMask;

    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget*>(children.at(i));
        if (w) {
            if (0) {
#ifndef QT_NO_STYLE_STYLESHEET
            } else if (w->testAttribute(Qt::WA_StyleSheet)) {
                // Style sheets follow a different font propagation scheme.
                if (cssStyle)
                    cssStyle->updateStyleSheetFont(w);
#endif
            } else if ((!w->isWindow() || w->testAttribute(Qt::WA_WindowPropagation))) {
                // Propagate font changes.
                QWidgetPrivate *wd = w->d_func();
                wd->inheritedFontResolveMask = newMask;
                wd->resolveFont();
            }
        }
    }

#ifndef QT_NO_STYLE_STYLESHEET
    if (cssStyle) {
        cssStyle->updateStyleSheetFont(q);
    }
#endif

    QEvent e(QEvent::FontChange);
    QApplication::sendEvent(q, &e);
#ifdef QT3_SUPPORT
    q->fontChange(old);
#endif
}

void QWidgetPrivate::setLayoutDirection_helper(Qt::LayoutDirection direction)
{
    Q_Q(QWidget);

    if ( (direction == Qt::RightToLeft) == q->testAttribute(Qt::WA_RightToLeft))
        return;
    q->setAttribute(Qt::WA_RightToLeft, (direction == Qt::RightToLeft));
    if (!children.isEmpty()) {
        for (int i = 0; i < children.size(); ++i) {
            QWidget *w = qobject_cast<QWidget*>(children.at(i));
            if (w && !w->isWindow() && !w->testAttribute(Qt::WA_SetLayoutDirection))
                w->d_func()->setLayoutDirection_helper(direction);
        }
    }
    QEvent e(QEvent::LayoutDirectionChange);
    QApplication::sendEvent(q, &e);
}

void QWidgetPrivate::resolveLayoutDirection()
{
    Q_Q(const QWidget);
    if (!q->testAttribute(Qt::WA_SetLayoutDirection))
        setLayoutDirection_helper(q->isWindow() ? QApplication::layoutDirection() : q->parentWidget()->layoutDirection());
}

/*!
    \property QWidget::layoutDirection

    \brief the layout direction for this widget

    By default, this property is set to Qt::LeftToRight.

    When the layout direction is set on a widget, it will propagate to
    the widget's children, but not to a child that is a window and not
    to a child for which setLayoutDirection() has been explicitly
    called. Also, child widgets added \e after setLayoutDirection()
    has been called for the parent do not inherit the parent's layout
    direction.

    This method no longer affects text layout direction since Qt 4.7.

    \sa QApplication::layoutDirection
*/
void QWidget::setLayoutDirection(Qt::LayoutDirection direction)
{
    Q_D(QWidget);

    if (direction == Qt::LayoutDirectionAuto) {
        unsetLayoutDirection();
        return;
    }

    setAttribute(Qt::WA_SetLayoutDirection);
    d->setLayoutDirection_helper(direction);
}

Qt::LayoutDirection QWidget::layoutDirection() const
{
    return testAttribute(Qt::WA_RightToLeft) ? Qt::RightToLeft : Qt::LeftToRight;
}

void QWidget::unsetLayoutDirection()
{
    Q_D(QWidget);
    setAttribute(Qt::WA_SetLayoutDirection, false);
    d->resolveLayoutDirection();
}

/*!
    \fn QFontMetrics QWidget::fontMetrics() const

    Returns the font metrics for the widget's current font.
    Equivalent to QFontMetrics(widget->font()).

    \sa font(), fontInfo(), setFont()
*/

/*!
    \fn QFontInfo QWidget::fontInfo() const

    Returns the font info for the widget's current font.
    Equivalent to QFontInto(widget->font()).

    \sa font(), fontMetrics(), setFont()
*/


/*!
    \property QWidget::cursor
    \brief the cursor shape for this widget

    The mouse cursor will assume this shape when it's over this
    widget. See the \link Qt::CursorShape list of predefined cursor
    objects\endlink for a range of useful shapes.

    An editor widget might use an I-beam cursor:
    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 6

    If no cursor has been set, or after a call to unsetCursor(), the
    parent's cursor is used.

    By default, this property contains a cursor with the Qt::ArrowCursor
    shape.

    Some underlying window implementations will reset the cursor if it
    leaves a widget even if the mouse is grabbed. If you want to have
    a cursor set for all widgets, even when outside the window, consider
    QApplication::setOverrideCursor().

    \sa QApplication::setOverrideCursor()
*/

#ifndef QT_NO_CURSOR
QCursor QWidget::cursor() const
{
    Q_D(const QWidget);
    if (testAttribute(Qt::WA_SetCursor))
        return (d->extra && d->extra->curs)
            ? *d->extra->curs
            : QCursor(Qt::ArrowCursor);
    if (isWindow() || !parentWidget())
        return QCursor(Qt::ArrowCursor);
    return parentWidget()->cursor();
}

void QWidget::setCursor(const QCursor &cursor)
{
    Q_D(QWidget);
// On Mac we must set the cursor even if it is the ArrowCursor.
#if !defined(Q_WS_MAC) && !defined(Q_WS_QWS)
    if (cursor.shape() != Qt::ArrowCursor
        || (d->extra && d->extra->curs))
#endif
    {
        d->createExtra();
        QCursor *newCursor = new QCursor(cursor);
        delete d->extra->curs;
        d->extra->curs = newCursor;
    }
    setAttribute(Qt::WA_SetCursor);
    d->setCursor_sys(cursor);

    QEvent event(QEvent::CursorChange);
    QApplication::sendEvent(this, &event);
}

void QWidget::unsetCursor()
{
    Q_D(QWidget);
    if (d->extra) {
        delete d->extra->curs;
        d->extra->curs = 0;
    }
    if (!isWindow())
        setAttribute(Qt::WA_SetCursor, false);
    d->unsetCursor_sys();

    QEvent event(QEvent::CursorChange);
    QApplication::sendEvent(this, &event);
}

#endif

/*!
    \enum QWidget::RenderFlag

    This enum describes how to render the widget when calling QWidget::render().

    \value DrawWindowBackground If you enable this option, the widget's background
    is rendered into the target even if autoFillBackground is not set. By default,
    this option is enabled.

    \value DrawChildren If you enable this option, the widget's children
    are rendered recursively into the target. By default, this option is enabled.

    \value IgnoreMask If you enable this option, the widget's QWidget::mask()
    is ignored when rendering into the target. By default, this option is disabled.

    \since 4.3
*/

/*!
    \since 4.3

    Renders the \a sourceRegion of this widget into the \a target
    using \a renderFlags to determine how to render. Rendering
    starts at \a targetOffset in the \a target. For example:

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 7

    If \a sourceRegion is a null region, this function will use QWidget::rect() as
    the region, i.e. the entire widget.

    Ensure that you call QPainter::end() for the \a target device's
    active painter (if any) before rendering. For example:

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 8

    \note To obtain the contents of an OpenGL widget, use QGLWidget::grabFrameBuffer()
    or QGLWidget::renderPixmap() instead.
*/
void QWidget::render(QPaintDevice *target, const QPoint &targetOffset,
                     const QRegion &sourceRegion, RenderFlags renderFlags)
{
    d_func()->render(target, targetOffset, sourceRegion, renderFlags, false);
}

/*!
    \overload

    Renders the widget into the \a painter's QPainter::device().

    Transformations and settings applied to the \a painter will be used
    when rendering.

    \note The \a painter must be active. On Mac OS X the widget will be
    rendered into a QPixmap and then drawn by the \a painter.

    \sa QPainter::device()
*/
void QWidget::render(QPainter *painter, const QPoint &targetOffset,
                     const QRegion &sourceRegion, RenderFlags renderFlags)
{
    if (!painter) {
        qWarning("QWidget::render: Null pointer to painter");
        return;
    }

    if (!painter->isActive()) {
        qWarning("QWidget::render: Cannot render with an inactive painter");
        return;
    }

    const qreal opacity = painter->opacity();
    if (qFuzzyIsNull(opacity))
        return; // Fully transparent.

    Q_D(QWidget);
    const bool inRenderWithPainter = d->extra && d->extra->inRenderWithPainter;
    const QRegion toBePainted = !inRenderWithPainter ? d->prepareToRender(sourceRegion, renderFlags)
                                                     : sourceRegion;
    if (toBePainted.isEmpty())
        return;

    if (!d->extra)
        d->createExtra();
    d->extra->inRenderWithPainter = true;

#ifdef Q_WS_MAC
    d->render_helper(painter, targetOffset, toBePainted, renderFlags);
#else
    QPaintEngine *engine = painter->paintEngine();
    Q_ASSERT(engine);
    QPaintEnginePrivate *enginePriv = engine->d_func();
    Q_ASSERT(enginePriv);
    QPaintDevice *target = engine->paintDevice();
    Q_ASSERT(target);

    // Render via a pixmap when dealing with non-opaque painters or printers.
    if (!inRenderWithPainter && (opacity < 1.0 || (target->devType() == QInternal::Printer))) {
        d->render_helper(painter, targetOffset, toBePainted, renderFlags);
        d->extra->inRenderWithPainter = false;
        return;
    }

    // Set new shared painter.
    QPainter *oldPainter = d->sharedPainter();
    d->setSharedPainter(painter);

    // Save current system clip, viewport and transform,
    const QTransform oldTransform = enginePriv->systemTransform;
    const QRegion oldSystemClip = enginePriv->systemClip;
    const QRegion oldSystemViewport = enginePriv->systemViewport;

    // This ensures that all painting triggered by render() is clipped to the current engine clip.
    if (painter->hasClipping()) {
        const QRegion painterClip = painter->deviceTransform().map(painter->clipRegion());
        enginePriv->setSystemViewport(oldSystemClip.isEmpty() ? painterClip : oldSystemClip & painterClip);
    } else {
        enginePriv->setSystemViewport(oldSystemClip);
    }

    render(target, targetOffset, toBePainted, renderFlags);

    // Restore system clip, viewport and transform.
    enginePriv->systemClip = oldSystemClip;
    enginePriv->setSystemViewport(oldSystemViewport);
    enginePriv->setSystemTransform(oldTransform);

    // Restore shared painter.
    d->setSharedPainter(oldPainter);
#endif

    d->extra->inRenderWithPainter = false;
}

/*!
    \brief The graphicsEffect function returns a pointer to the
    widget's graphics effect.

    If the widget has no graphics effect, 0 is returned.

    \since 4.6

    \sa setGraphicsEffect()
*/
#ifndef QT_NO_GRAPHICSEFFECT
QGraphicsEffect *QWidget::graphicsEffect() const
{
    Q_D(const QWidget);
    return d->graphicsEffect;
}
#endif //QT_NO_GRAPHICSEFFECT

/*!

  \brief The setGraphicsEffect function is for setting the widget's graphics effect.

    Sets \a effect as the widget's effect. If there already is an effect installed
    on this widget, QWidget will delete the existing effect before installing
    the new \a effect.

    If \a effect is the installed on a different widget, setGraphicsEffect() will remove
    the effect from the widget and install it on this widget.

    QWidget takes ownership of \a effect.

    \note This function will apply the effect on itself and all its children.

    \since 4.6

    \sa graphicsEffect()
*/
#ifndef QT_NO_GRAPHICSEFFECT
void QWidget::setGraphicsEffect(QGraphicsEffect *effect)
{
    Q_D(QWidget);
    if (d->graphicsEffect == effect)
        return;

    if (d->graphicsEffect) {
        d->invalidateBuffer(rect());
        delete d->graphicsEffect;
        d->graphicsEffect = 0;
    }

    if (effect) {
        // Set new effect.
        QGraphicsEffectSourcePrivate *sourced = new QWidgetEffectSourcePrivate(this);
        QGraphicsEffectSource *source = new QGraphicsEffectSource(*sourced);
        d->graphicsEffect = effect;
        effect->d_func()->setGraphicsEffectSource(source);
        update();
    }

    d->updateIsOpaque();
}
#endif //QT_NO_GRAPHICSEFFECT

bool QWidgetPrivate::isAboutToShow() const
{
    if (data.in_show)
        return true;

    Q_Q(const QWidget);
    if (q->isHidden())
        return false;

    // The widget will be shown if any of its ancestors are about to show.
    QWidget *parent = q->parentWidget();
    return parent ? parent->d_func()->isAboutToShow() : false;
}

QRegion QWidgetPrivate::prepareToRender(const QRegion &region, QWidget::RenderFlags renderFlags)
{
    Q_Q(QWidget);
    const bool isVisible = q->isVisible();

    // Make sure the widget is laid out correctly.
    if (!isVisible && !isAboutToShow()) {
        QWidget *topLevel = q->window();
        (void)topLevel->d_func()->topData(); // Make sure we at least have top-data.
        topLevel->ensurePolished();

        // Invalidate the layout of hidden ancestors (incl. myself) and pretend
        // they're not explicitly hidden.
        QWidget *widget = q;
        QWidgetList hiddenWidgets;
        while (widget) {
            if (widget->isHidden()) {
                widget->setAttribute(Qt::WA_WState_Hidden, false);
                hiddenWidgets.append(widget);
                if (!widget->isWindow() && widget->parentWidget()->d_func()->layout)
                    widget->d_func()->updateGeometry_helper(true);
            }
            widget = widget->parentWidget();
        }

        // Activate top-level layout.
        if (topLevel->d_func()->layout)
            topLevel->d_func()->layout->activate();

        // Adjust size if necessary.
        QTLWExtra *topLevelExtra = topLevel->d_func()->maybeTopData();
        if (topLevelExtra && !topLevelExtra->sizeAdjusted
            && !topLevel->testAttribute(Qt::WA_Resized)) {
            topLevel->adjustSize();
            topLevel->setAttribute(Qt::WA_Resized, false);
        }

        // Activate child layouts.
        topLevel->d_func()->activateChildLayoutsRecursively();

        // We're not cheating with WA_WState_Hidden anymore.
        for (int i = 0; i < hiddenWidgets.size(); ++i) {
            QWidget *widget = hiddenWidgets.at(i);
            widget->setAttribute(Qt::WA_WState_Hidden);
            if (!widget->isWindow() && widget->parentWidget()->d_func()->layout)
                widget->parentWidget()->d_func()->layout->invalidate();
        }
    } else if (isVisible) {
        q->window()->d_func()->sendPendingMoveAndResizeEvents(true, true);
    }

    // Calculate the region to be painted.
    QRegion toBePainted = !region.isEmpty() ? region : QRegion(q->rect());
    if (!(renderFlags & QWidget::IgnoreMask) && extra && extra->hasMask)
        toBePainted &= extra->mask;
    return toBePainted;
}

void QWidgetPrivate::render_helper(QPainter *painter, const QPoint &targetOffset, const QRegion &toBePainted,
                                   QWidget::RenderFlags renderFlags)
{
    Q_ASSERT(painter);
    Q_ASSERT(!toBePainted.isEmpty());

    Q_Q(QWidget);
#ifndef Q_WS_MAC
    const QTransform originalTransform = painter->worldTransform();
    const bool useDeviceCoordinates = originalTransform.isScaling();
    if (!useDeviceCoordinates) {
#endif
        // Render via a pixmap.
        const QRect rect = toBePainted.boundingRect();
        const QSize size = rect.size();
        if (size.isNull())
            return;

        QPixmap pixmap(size);
        if (!(renderFlags & QWidget::DrawWindowBackground) || !isOpaque)
            pixmap.fill(Qt::transparent);
        q->render(&pixmap, QPoint(), toBePainted, renderFlags);

        const bool restore = !(painter->renderHints() & QPainter::SmoothPixmapTransform);
        painter->setRenderHints(QPainter::SmoothPixmapTransform, true);

        painter->drawPixmap(targetOffset, pixmap);

        if (restore)
            painter->setRenderHints(QPainter::SmoothPixmapTransform, false);

#ifndef Q_WS_MAC
    } else {
        // Render via a pixmap in device coordinates (to avoid pixmap scaling).
        QTransform transform = originalTransform;
        transform.translate(targetOffset.x(), targetOffset.y());

        QPaintDevice *device = painter->device();
        Q_ASSERT(device);

        // Calculate device rect.
        const QRectF rect(toBePainted.boundingRect());
        QRect deviceRect = transform.mapRect(QRectF(0, 0, rect.width(), rect.height())).toAlignedRect();
        deviceRect &= QRect(0, 0, device->width(), device->height());

        QPixmap pixmap(deviceRect.size());
        pixmap.fill(Qt::transparent);

        // Create a pixmap device coordinate painter.
        QPainter pixmapPainter(&pixmap);
        pixmapPainter.setRenderHints(painter->renderHints());
        transform *= QTransform::fromTranslate(-deviceRect.x(), -deviceRect.y());
        pixmapPainter.setTransform(transform);

        q->render(&pixmapPainter, QPoint(), toBePainted, renderFlags);
        pixmapPainter.end();

        // And then draw the pixmap.
        painter->setTransform(QTransform());
        painter->drawPixmap(deviceRect.topLeft(), pixmap);
        painter->setTransform(originalTransform);
    }
#endif
}

void QWidgetPrivate::drawWidget(QPaintDevice *pdev, const QRegion &rgn, const QPoint &offset, int flags,
                                QPainter *sharedPainter, QWidgetBackingStore *backingStore)
{
    if (rgn.isEmpty())
        return;

#if defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
    if (qt_mac_clearDirtyOnWidgetInsideDrawWidget)
        dirtyOnWidget = QRegion();

    // We disable the rendering of QToolBar in the backingStore if
    // it's supposed to be in the unified toolbar on Mac OS X.
    if (backingStore && isInUnifiedToolbar)
        return;
#endif // Q_WS_MAC && QT_MAC_USE_COCOA


    Q_Q(QWidget);
#ifndef QT_NO_GRAPHICSEFFECT
    if (graphicsEffect && graphicsEffect->isEnabled()) {
        QGraphicsEffectSource *source = graphicsEffect->d_func()->source;
        QWidgetEffectSourcePrivate *sourced = static_cast<QWidgetEffectSourcePrivate *>
                                                         (source->d_func());
        if (!sourced->context) {
            QWidgetPaintContext context(pdev, rgn, offset, flags, sharedPainter, backingStore);
            sourced->context = &context;
            if (!sharedPainter) {
                QPaintEngine *paintEngine = pdev->paintEngine();
                paintEngine->d_func()->systemClip = rgn.translated(offset);
                QPainter p(pdev);
                p.translate(offset);
                context.painter = &p;
                graphicsEffect->draw(&p);
                paintEngine->d_func()->systemClip = QRegion();
            } else {
                context.painter = sharedPainter;
                if (sharedPainter->worldTransform() != sourced->lastEffectTransform) {
                    sourced->invalidateCache();
                    sourced->lastEffectTransform = sharedPainter->worldTransform();
                }
                sharedPainter->save();
                sharedPainter->translate(offset);
                graphicsEffect->draw(sharedPainter);
                sharedPainter->restore();
            }
            sourced->context = 0;
            return;
        }
    }
#endif //QT_NO_GRAFFICSEFFECT

    const bool asRoot = flags & DrawAsRoot;
    const bool alsoOnScreen = flags & DrawPaintOnScreen;
    const bool recursive = flags & DrawRecursive;
    const bool alsoInvisible = flags & DrawInvisible;

    Q_ASSERT(sharedPainter ? sharedPainter->isActive() : true);

    QRegion toBePainted(rgn);
    if (asRoot && !alsoInvisible)
        toBePainted &= clipRect(); //(rgn & visibleRegion());
    if (!(flags & DontSubtractOpaqueChildren))
        subtractOpaqueChildren(toBePainted, q->rect());

    if (!toBePainted.isEmpty()) {
        bool onScreen = paintOnScreen();
        if (!onScreen || alsoOnScreen) {
            //update the "in paint event" flag
            if (q->testAttribute(Qt::WA_WState_InPaintEvent))
                qWarning("QWidget::repaint: Recursive repaint detected");
            q->setAttribute(Qt::WA_WState_InPaintEvent);

            //clip away the new area
#ifndef QT_NO_PAINT_DEBUG
            bool flushed = QWidgetBackingStore::flushPaint(q, toBePainted);
#endif
            QPaintEngine *paintEngine = pdev->paintEngine();
            if (paintEngine) {
                setRedirected(pdev, -offset);

#ifdef Q_WS_MAC
                // (Alien support) Special case for Mac when redirecting: If the paint device
                // is of the Widget type we need to set WA_WState_InPaintEvent since painting
                // outside the paint event is not supported on QWidgets. The attributeis
                // restored further down.
                if (pdev->devType() == QInternal::Widget)
                    static_cast<QWidget *>(pdev)->setAttribute(Qt::WA_WState_InPaintEvent);

#endif
                if (sharedPainter)
                    paintEngine->d_func()->systemClip = toBePainted;
                else
                    paintEngine->d_func()->systemRect = q->data->crect;

                //paint the background
                if ((asRoot || q->autoFillBackground() || onScreen || q->testAttribute(Qt::WA_StyledBackground))
                    && !q->testAttribute(Qt::WA_OpaquePaintEvent) && !q->testAttribute(Qt::WA_NoSystemBackground)) {
                    QPainter p(q);
                    paintBackground(&p, toBePainted, (asRoot || onScreen) ? flags | DrawAsRoot : 0);
                }

                if (!sharedPainter)
                    paintEngine->d_func()->systemClip = toBePainted.translated(offset);

                if (!onScreen && !asRoot && !isOpaque && q->testAttribute(Qt::WA_TintedBackground)) {
                    QPainter p(q);
                    QColor tint = q->palette().window().color();
                    tint.setAlphaF(qreal(.6));
                    p.fillRect(toBePainted.boundingRect(), tint);
                }
            }

#if 0
            qDebug() << "painting" << q << "opaque ==" << isOpaque();
            qDebug() << "clipping to" << toBePainted << "location == " << offset
                     << "geometry ==" << QRect(q->mapTo(q->window(), QPoint(0, 0)), q->size());
#endif

            //actually send the paint event
            QPaintEvent e(toBePainted);
            QCoreApplication::sendSpontaneousEvent(q, &e);
#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
            if (backingStore && !onScreen && !asRoot && (q->internalWinId() || !q->nativeParentWidget()->isWindow()))
                backingStore->markDirtyOnScreen(toBePainted, q, offset);
#endif

            //restore
            if (paintEngine) {
#ifdef Q_WS_MAC
                if (pdev->devType() == QInternal::Widget)
                    static_cast<QWidget *>(pdev)->setAttribute(Qt::WA_WState_InPaintEvent, false);
#endif
                restoreRedirected();
                if (!sharedPainter)
                    paintEngine->d_func()->systemRect = QRect();
                else
                    paintEngine->d_func()->currentClipWidget = 0;
                paintEngine->d_func()->systemClip = QRegion();
            }
            q->setAttribute(Qt::WA_WState_InPaintEvent, false);
            if (q->paintingActive() && !q->testAttribute(Qt::WA_PaintOutsidePaintEvent))
                qWarning("QWidget::repaint: It is dangerous to leave painters active on a widget outside of the PaintEvent");

            if (paintEngine && paintEngine->autoDestruct()) {
                delete paintEngine;
            }

#ifndef QT_NO_PAINT_DEBUG
            if (flushed)
                QWidgetBackingStore::unflushPaint(q, toBePainted);
#endif
        } else if (q->isWindow()) {
            QPaintEngine *engine = pdev->paintEngine();
            if (engine) {
                QPainter p(pdev);
                p.setClipRegion(toBePainted);
                const QBrush bg = q->palette().brush(QPalette::Window);
                if (bg.style() == Qt::TexturePattern)
                    p.drawTiledPixmap(q->rect(), bg.texture());
                else
                    p.fillRect(q->rect(), bg);

                if (engine->autoDestruct())
                    delete engine;
            }
        }
    }

    if (recursive && !children.isEmpty()) {
        paintSiblingsRecursive(pdev, children, children.size() - 1, rgn, offset, flags & ~DrawAsRoot
#ifdef Q_BACKINGSTORE_SUBSURFACES
                                , q->windowSurface()
#endif
                                , sharedPainter, backingStore);
    }
}

void QWidgetPrivate::render(QPaintDevice *target, const QPoint &targetOffset,
                            const QRegion &sourceRegion, QWidget::RenderFlags renderFlags,
                            bool readyToRender)
{
    if (!target) {
        qWarning("QWidget::render: null pointer to paint device");
        return;
    }

    const bool inRenderWithPainter = extra && extra->inRenderWithPainter;
    QRegion paintRegion = !inRenderWithPainter && !readyToRender
                          ? prepareToRender(sourceRegion, renderFlags)
                          : sourceRegion;
    if (paintRegion.isEmpty())
        return;

#ifndef Q_WS_MAC
    QPainter *oldSharedPainter = inRenderWithPainter ? sharedPainter() : 0;

    // Use the target's shared painter if set (typically set when doing
    // "other->render(widget);" in the widget's paintEvent.
    if (target->devType() == QInternal::Widget) {
        QWidgetPrivate *targetPrivate = static_cast<QWidget *>(target)->d_func();
        if (targetPrivate->extra && targetPrivate->extra->inRenderWithPainter) {
            QPainter *targetPainter = targetPrivate->sharedPainter();
            if (targetPainter && targetPainter->isActive())
                setSharedPainter(targetPainter);
        }
    }
#endif

    // Use the target's redirected device if set and adjust offset and paint
    // region accordingly. This is typically the case when people call render
    // from the paintEvent.
    QPoint offset = targetOffset;
    offset -= paintRegion.boundingRect().topLeft();
    QPoint redirectionOffset;
    QPaintDevice *redirected = 0;

    if (target->devType() == QInternal::Widget)
        redirected = static_cast<QWidget *>(target)->d_func()->redirected(&redirectionOffset);
    if (!redirected)
        redirected = QPainter::redirected(target, &redirectionOffset);

    if (redirected) {
        target = redirected;
        offset -= redirectionOffset;
    }

    if (!inRenderWithPainter) { // Clip handled by shared painter (in qpainter.cpp).
        if (QPaintEngine *targetEngine = target->paintEngine()) {
            const QRegion targetSystemClip = targetEngine->systemClip();
            if (!targetSystemClip.isEmpty())
                paintRegion &= targetSystemClip.translated(-offset);
        }
    }

    // Set backingstore flags.
    int flags = DrawPaintOnScreen | DrawInvisible;
    if (renderFlags & QWidget::DrawWindowBackground)
        flags |= DrawAsRoot;

    if (renderFlags & QWidget::DrawChildren)
        flags |= DrawRecursive;
    else
        flags |= DontSubtractOpaqueChildren;

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
    flags |= DontSetCompositionMode;
#endif

    if (target->devType() == QInternal::Printer) {
        QPainter p(target);
        render_helper(&p, targetOffset, paintRegion, renderFlags);
        return;
    }

#ifndef Q_WS_MAC
    // Render via backingstore.
    drawWidget(target, paintRegion, offset, flags, sharedPainter());

    // Restore shared painter.
    if (oldSharedPainter)
        setSharedPainter(oldSharedPainter);
#else
    // Render via backingstore (no shared painter).
    drawWidget(target, paintRegion, offset, flags, 0);
#endif
}

void QWidgetPrivate::paintSiblingsRecursive(QPaintDevice *pdev, const QObjectList& siblings, int index, const QRegion &rgn,
                                            const QPoint &offset, int flags
#ifdef Q_BACKINGSTORE_SUBSURFACES
                                            , const QWindowSurface *currentSurface
#endif
                                            , QPainter *sharedPainter, QWidgetBackingStore *backingStore)
{
    QWidget *w = 0;
    QRect boundingRect;
    bool dirtyBoundingRect = true;
    const bool exludeOpaqueChildren = (flags & DontDrawOpaqueChildren);
    const bool excludeNativeChildren = (flags & DontDrawNativeChildren);

    do {
        QWidget *x =  qobject_cast<QWidget*>(siblings.at(index));
        if (x && !(exludeOpaqueChildren && x->d_func()->isOpaque) && !x->isHidden() && !x->isWindow()
            && !(excludeNativeChildren && x->internalWinId())) {
            if (dirtyBoundingRect) {
                boundingRect = rgn.boundingRect();
                dirtyBoundingRect = false;
            }

            if (qRectIntersects(boundingRect, x->d_func()->effectiveRectFor(x->data->crect))) {
#ifdef Q_BACKINGSTORE_SUBSURFACES
                if (x->windowSurface() == currentSurface)
#endif
                {
                    w = x;
                    break;
                }
            }
        }
        --index;
    } while (index >= 0);

    if (!w)
        return;

    QWidgetPrivate *wd = w->d_func();
    const QPoint widgetPos(w->data->crect.topLeft());
    const bool hasMask = wd->extra && wd->extra->hasMask && !wd->graphicsEffect;
    if (index > 0) {
        QRegion wr(rgn);
        if (wd->isOpaque)
            wr -= hasMask ? wd->extra->mask.translated(widgetPos) : w->data->crect;
        paintSiblingsRecursive(pdev, siblings, --index, wr, offset, flags
#ifdef Q_BACKINGSTORE_SUBSURFACES
                               , currentSurface
#endif
                               , sharedPainter, backingStore);
    }

    if (w->updatesEnabled()
#ifndef QT_NO_GRAPHICSVIEW
            && (!w->d_func()->extra || !w->d_func()->extra->proxyWidget)
#endif //QT_NO_GRAPHICSVIEW
       ) {
        QRegion wRegion(rgn);
        wRegion &= wd->effectiveRectFor(w->data->crect);
        wRegion.translate(-widgetPos);
        if (hasMask)
            wRegion &= wd->extra->mask;
        wd->drawWidget(pdev, wRegion, offset + widgetPos, flags, sharedPainter, backingStore);
    }
}

#ifndef QT_NO_GRAPHICSEFFECT
QRectF QWidgetEffectSourcePrivate::boundingRect(Qt::CoordinateSystem system) const
{
    if (system != Qt::DeviceCoordinates)
        return m_widget->rect();

    if (!context) {
        // Device coordinates without context not yet supported.
        qWarning("QGraphicsEffectSource::boundingRect: Not yet implemented, lacking device context");
        return QRectF();
    }

    return context->painter->worldTransform().mapRect(m_widget->rect());
}

void QWidgetEffectSourcePrivate::draw(QPainter *painter)
{
    if (!context || context->painter != painter) {
        m_widget->render(painter);
        return;
    }

    // The region saved in the context is neither clipped to the rect
    // nor the mask, so we have to clip it here before calling drawWidget.
    QRegion toBePainted = context->rgn;
    toBePainted &= m_widget->rect();
    QWidgetPrivate *wd = qt_widget_private(m_widget);
    if (wd->extra && wd->extra->hasMask)
        toBePainted &= wd->extra->mask;

    wd->drawWidget(context->pdev, toBePainted, context->offset, context->flags,
                   context->sharedPainter, context->backingStore);
}

QPixmap QWidgetEffectSourcePrivate::pixmap(Qt::CoordinateSystem system, QPoint *offset,
                                           QGraphicsEffect::PixmapPadMode mode) const
{
    const bool deviceCoordinates = (system == Qt::DeviceCoordinates);
    if (!context && deviceCoordinates) {
        // Device coordinates without context not yet supported.
        qWarning("QGraphicsEffectSource::pixmap: Not yet implemented, lacking device context");
        return QPixmap();
    }

    QPoint pixmapOffset;
    QRectF sourceRect = m_widget->rect();

    if (deviceCoordinates) {
        const QTransform &painterTransform = context->painter->worldTransform();
        sourceRect = painterTransform.mapRect(sourceRect);
        pixmapOffset = painterTransform.map(pixmapOffset);
    }

    QRect effectRect;

    if (mode == QGraphicsEffect::PadToEffectiveBoundingRect)
        effectRect = m_widget->graphicsEffect()->boundingRectFor(sourceRect).toAlignedRect();
    else if (mode == QGraphicsEffect::PadToTransparentBorder)
        effectRect = sourceRect.adjusted(-1, -1, 1, 1).toAlignedRect();
    else
        effectRect = sourceRect.toAlignedRect();

    if (offset)
        *offset = effectRect.topLeft();

    pixmapOffset -= effectRect.topLeft();

    QPixmap pixmap(effectRect.size());
    pixmap.fill(Qt::transparent);
    m_widget->render(&pixmap, pixmapOffset, QRegion(), QWidget::DrawChildren);
    return pixmap;
}
#endif //QT_NO_GRAPHICSEFFECT

#ifndef QT_NO_GRAPHICSVIEW
/*!
    \internal

    Finds the nearest widget embedded in a graphics proxy widget along the chain formed by this
    widget and its ancestors. The search starts at \a origin (inclusive).
    If successful, the function returns the proxy that embeds the widget, or 0 if no embedded
    widget was found.
*/
QGraphicsProxyWidget * QWidgetPrivate::nearestGraphicsProxyWidget(const QWidget *origin)
{
    if (origin) {
        QWExtra *extra = origin->d_func()->extra;
        if (extra && extra->proxyWidget)
            return extra->proxyWidget;
        return nearestGraphicsProxyWidget(origin->parentWidget());
    }
    return 0;
}
#endif

/*!
    \property QWidget::locale
    \brief the widget's locale
    \since 4.3

    As long as no special locale has been set, this is either
    the parent's locale or (if this widget is a top level widget),
    the default locale.

    If the widget displays dates or numbers, these should be formatted
    using the widget's locale.

    \sa QLocale QLocale::setDefault()
*/

void QWidgetPrivate::setLocale_helper(const QLocale &loc, bool forceUpdate)
{
    Q_Q(QWidget);
    if (locale == loc && !forceUpdate)
        return;

    locale = loc;

    if (!children.isEmpty()) {
        for (int i = 0; i < children.size(); ++i) {
            QWidget *w = qobject_cast<QWidget*>(children.at(i));
            if (!w)
                continue;
            if (w->testAttribute(Qt::WA_SetLocale))
                continue;
            if (w->isWindow() && !w->testAttribute(Qt::WA_WindowPropagation))
                continue;
            w->d_func()->setLocale_helper(loc, forceUpdate);
        }
    }
    QEvent e(QEvent::LocaleChange);
    QApplication::sendEvent(q, &e);
}

void QWidget::setLocale(const QLocale &locale)
{
    Q_D(QWidget);

    setAttribute(Qt::WA_SetLocale);
    d->setLocale_helper(locale);
}

QLocale QWidget::locale() const
{
    Q_D(const QWidget);

    return d->locale;
}

void QWidgetPrivate::resolveLocale()
{
    Q_Q(const QWidget);

    if (!q->testAttribute(Qt::WA_SetLocale)) {
        setLocale_helper(q->isWindow()
                            ? QLocale()
                            : q->parentWidget()->locale());
    }
}

void QWidget::unsetLocale()
{
    Q_D(QWidget);
    setAttribute(Qt::WA_SetLocale, false);
    d->resolveLocale();
}

static QString constructWindowTitleFromFilePath(const QString &filePath)
{
    QFileInfo fi(filePath);
    QString windowTitle = fi.fileName() + QLatin1String("[*]");
#ifndef Q_WS_MAC
    QString appName = QApplication::applicationName();
    if (!appName.isEmpty())
        windowTitle += QLatin1Char(' ') + QChar(0x2014) + QLatin1Char(' ') + appName;
#endif
    return windowTitle;
}

/*!
    \property QWidget::windowTitle
    \brief the window title (caption)

    This property only makes sense for top-level widgets, such as
    windows and dialogs. If no caption has been set, the title is based of the
    \l windowFilePath. If neither of these is set, then the title is
    an empty string.

    If you use the \l windowModified mechanism, the window title must
    contain a "[*]" placeholder, which indicates where the '*' should
    appear. Normally, it should appear right after the file name
    (e.g., "document1.txt[*] - Text Editor"). If the \l
    windowModified property is false (the default), the placeholder
    is simply removed.

    \sa windowIcon, windowIconText, windowModified, windowFilePath
*/
QString QWidget::windowTitle() const
{
    Q_D(const QWidget);
    if (d->extra && d->extra->topextra) {
        if (!d->extra->topextra->caption.isEmpty())
            return d->extra->topextra->caption;
        if (!d->extra->topextra->filePath.isEmpty())
            return constructWindowTitleFromFilePath(d->extra->topextra->filePath);
    }
    return QString();
}

/*!
    Returns a modified window title with the [*] place holder
    replaced according to the rules described in QWidget::setWindowTitle

    This function assumes that "[*]" can be quoted by another
    "[*]", so it will replace two place holders by one and
    a single last one by either "*" or nothing depending on
    the modified flag.

    \internal
*/
QString qt_setWindowTitle_helperHelper(const QString &title, const QWidget *widget)
{
    Q_ASSERT(widget);

#ifdef QT_EVAL
    extern QString qt_eval_adapt_window_title(const QString &title);
    QString cap = qt_eval_adapt_window_title(title);
#else
    QString cap = title;
#endif

    if (cap.isEmpty())
        return cap;

    QLatin1String placeHolder("[*]");
    int placeHolderLength = 3; // QLatin1String doesn't have length()

    int index = cap.indexOf(placeHolder);

    // here the magic begins
    while (index != -1) {
        index += placeHolderLength;
        int count = 1;
        while (cap.indexOf(placeHolder, index) == index) {
            ++count;
            index += placeHolderLength;
        }

        if (count%2) { // odd number of [*] -> replace last one
            int lastIndex = cap.lastIndexOf(placeHolder, index - 1);
            if (widget->isWindowModified()
             && widget->style()->styleHint(QStyle::SH_TitleBar_ModifyNotification, 0, widget))
                cap.replace(lastIndex, 3, QWidget::tr("*"));
            else
                cap.remove(lastIndex, 3);
        }

        index = cap.indexOf(placeHolder, index);
    }

    cap.replace(QLatin1String("[*][*]"), placeHolder);

    return cap;
}

void QWidgetPrivate::setWindowTitle_helper(const QString &title)
{
    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_WState_Created))
        setWindowTitle_sys(qt_setWindowTitle_helperHelper(title, q));
}

void QWidgetPrivate::setWindowIconText_helper(const QString &title)
{
    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_WState_Created))
        setWindowIconText_sys(qt_setWindowTitle_helperHelper(title, q));
}

void QWidget::setWindowIconText(const QString &iconText)
{
    if (QWidget::windowIconText() == iconText)
        return;

    Q_D(QWidget);
    d->topData()->iconText = iconText;
    d->setWindowIconText_helper(iconText);

    QEvent e(QEvent::IconTextChange);
    QApplication::sendEvent(this, &e);
}

void QWidget::setWindowTitle(const QString &title)
{
    if (QWidget::windowTitle() == title && !title.isEmpty() && !title.isNull())
        return;

    Q_D(QWidget);
    d->topData()->caption = title;
    d->setWindowTitle_helper(title);

    QEvent e(QEvent::WindowTitleChange);
    QApplication::sendEvent(this, &e);
}


/*!
    \property QWidget::windowIcon
    \brief the widget's icon

    This property only makes sense for windows. If no icon
    has been set, windowIcon() returns the application icon
    (QApplication::windowIcon()).

    \sa windowIconText, windowTitle
*/
QIcon QWidget::windowIcon() const
{
    const QWidget *w = this;
    while (w) {
        const QWidgetPrivate *d = w->d_func();
        if (d->extra && d->extra->topextra && d->extra->topextra->icon)
            return *d->extra->topextra->icon;
        w = w->parentWidget();
    }
    return QApplication::windowIcon();
}

void QWidgetPrivate::setWindowIcon_helper()
{
    QEvent e(QEvent::WindowIconChange);
    QApplication::sendEvent(q_func(), &e);
    for (int i = 0; i < children.size(); ++i) {
        QWidget *w = qobject_cast<QWidget *>(children.at(i));
        if (w && !w->isWindow())
            QApplication::sendEvent(w, &e);
    }
}

void QWidget::setWindowIcon(const QIcon &icon)
{
    Q_D(QWidget);

    setAttribute(Qt::WA_SetWindowIcon, !icon.isNull());
    d->createTLExtra();

    if (!d->extra->topextra->icon)
        d->extra->topextra->icon = new QIcon();
    *d->extra->topextra->icon = icon;

    delete d->extra->topextra->iconPixmap;
    d->extra->topextra->iconPixmap = 0;

    d->setWindowIcon_sys();
    d->setWindowIcon_helper();
}


/*!
    \property QWidget::windowIconText
    \brief the widget's icon text

    This property only makes sense for windows. If no icon
    text has been set, this functions returns an empty string.

    \sa windowIcon, windowTitle
*/

QString QWidget::windowIconText() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra) ? d->extra->topextra->iconText : QString();
}

/*!
    \property QWidget::windowFilePath
    \since 4.4
    \brief the file path associated with a widget

    This property only makes sense for windows. It associates a file path with
    a window. If you set the file path, but have not set the window title, Qt
    sets the window title to contain a string created using the following
    components.

    On Mac OS X:

    \list
    \o The file name of the specified path, obtained using QFileInfo::fileName().
    \endlist

    On Windows and X11:

    \list
    \o The file name of the specified path, obtained using QFileInfo::fileName().
    \o An optional \c{*} character, if the \l windowModified property is set.
    \o The \c{0x2014} unicode character, padded either side by spaces.
    \o The application name, obtained from the application's
    \l{QCoreApplication::}{applicationName} property.
    \endlist

    If the window title is set at any point, then the window title takes precedence and
    will be shown instead of the file path string.

    Additionally, on Mac OS X, this has an added benefit that it sets the
    \l{http://developer.apple.com/documentation/UserExperience/Conceptual/OSXHIGuidelines/XHIGWindows/chapter_17_section_3.html}{proxy icon}
    for the window, assuming that the file path exists.

    If no file path is set, this property contains an empty string.

    By default, this property contains an empty string.

    \sa windowTitle, windowIcon
*/

QString QWidget::windowFilePath() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra) ? d->extra->topextra->filePath : QString();
}

void QWidget::setWindowFilePath(const QString &filePath)
{
    if (filePath == windowFilePath())
        return;

    Q_D(QWidget);

    d->createTLExtra();
    d->extra->topextra->filePath = filePath;
    d->setWindowFilePath_helper(filePath);
}

void QWidgetPrivate::setWindowFilePath_helper(const QString &filePath)
{
    if (extra->topextra && extra->topextra->caption.isEmpty()) {
#ifdef Q_WS_MAC
        setWindowTitle_helper(QFileInfo(filePath).fileName());
#else
        Q_Q(QWidget);
        Q_UNUSED(filePath);
        setWindowTitle_helper(q->windowTitle());
#endif
    }
#ifdef Q_WS_MAC
    setWindowFilePath_sys(filePath);
#endif
}

/*!
    Returns the window's role, or an empty string.

    \sa windowIcon, windowTitle
*/

QString QWidget::windowRole() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra) ? d->extra->topextra->role : QString();
}

/*!
    Sets the window's role to \a role. This only makes sense for
    windows on X11.
*/
void QWidget::setWindowRole(const QString &role)
{
#if defined(Q_WS_X11)
    Q_D(QWidget);
    d->topData()->role = role;
    d->setWindowRole();
#else
    Q_UNUSED(role)
#endif
}

/*!
    \property QWidget::mouseTracking
    \brief whether mouse tracking is enabled for the widget

    If mouse tracking is disabled (the default), the widget only
    receives mouse move events when at least one mouse button is
    pressed while the mouse is being moved.

    If mouse tracking is enabled, the widget receives mouse move
    events even if no buttons are pressed.

    \sa mouseMoveEvent()
*/


/*!
    Sets the widget's focus proxy to widget \a w. If \a w is 0, the
    function resets this widget to have no focus proxy.

    Some widgets can "have focus", but create a child widget, such as
    QLineEdit, to actually handle the focus. In this case, the widget
    can set the line edit to be its focus proxy.

    setFocusProxy() sets the widget which will actually get focus when
    "this widget" gets it. If there is a focus proxy, setFocus() and
    hasFocus() operate on the focus proxy.

    \sa focusProxy()
*/

void QWidget::setFocusProxy(QWidget * w)
{
    Q_D(QWidget);
    if (!w && !d->extra)
        return;

    for (QWidget* fp  = w; fp; fp = fp->focusProxy()) {
        if (fp == this) {
            qWarning("QWidget: %s (%s) already in focus proxy chain", metaObject()->className(), objectName().toLocal8Bit().constData());
            return;
        }
    }

    d->createExtra();
    d->extra->focus_proxy = w;
}


/*!
    Returns the focus proxy, or 0 if there is no focus proxy.

    \sa setFocusProxy()
*/

QWidget * QWidget::focusProxy() const
{
    Q_D(const QWidget);
    return d->extra ? (QWidget *)d->extra->focus_proxy : 0;
}


/*!
    \property QWidget::focus
    \brief whether this widget (or its focus proxy) has the keyboard
    input focus

    By default, this property is false.

    \note Obtaining the value of this property for a widget is effectively equivalent
    to checking whether QApplication::focusWidget() refers to the widget.

    \sa setFocus(), clearFocus(), setFocusPolicy(), QApplication::focusWidget()
*/
bool QWidget::hasFocus() const
{
    const QWidget* w = this;
    while (w->d_func()->extra && w->d_func()->extra->focus_proxy)
        w = w->d_func()->extra->focus_proxy;
    if (QWidget *window = w->window()) {
#ifndef QT_NO_GRAPHICSVIEW
        QWExtra *e = window->d_func()->extra;
        if (e && e->proxyWidget && e->proxyWidget->hasFocus() && window->focusWidget() == w)
            return true;
#endif
    }
    return (QApplication::focusWidget() == w);
}

/*!
    Gives the keyboard input focus to this widget (or its focus
    proxy) if this widget or one of its parents is the \link
    isActiveWindow() active window\endlink. The \a reason argument will
    be passed into any focus event sent from this function, it is used
    to give an explanation of what caused the widget to get focus.
    If the window is not active, the widget will be given the focus when
    the window becomes active.

    First, a focus out event is sent to the focus widget (if any) to
    tell it that it is about to lose the focus. Then a focus in event
    is sent to this widget to tell it that it just received the focus.
    (Nothing happens if the focus in and focus out widgets are the
    same.)

    \note On embedded platforms, setFocus() will not cause an input panel
    to be opened by the input method. If you want this to happen, you
    have to send a QEvent::RequestSoftwareInputPanel event to the
    widget yourself.

    setFocus() gives focus to a widget regardless of its focus policy,
    but does not clear any keyboard grab (see grabKeyboard()).

    Be aware that if the widget is hidden, it will not accept focus
    until it is shown.

    \warning If you call setFocus() in a function which may itself be
    called from focusOutEvent() or focusInEvent(), you may get an
    infinite recursion.

    \sa hasFocus(), clearFocus(), focusInEvent(), focusOutEvent(),
    setFocusPolicy(), focusWidget(), QApplication::focusWidget(), grabKeyboard(),
    grabMouse(), {Keyboard Focus}, QEvent::RequestSoftwareInputPanel
*/

void QWidget::setFocus(Qt::FocusReason reason)
{
    if (!isEnabled())
        return;

    QWidget *f = this;
    while (f->d_func()->extra && f->d_func()->extra->focus_proxy)
        f = f->d_func()->extra->focus_proxy;

    if (QApplication::focusWidget() == f
#if defined(Q_WS_WIN)
        && GetFocus() == f->internalWinId()
#endif
       )
        return;

#ifndef QT_NO_GRAPHICSVIEW
    QWidget *previousProxyFocus = 0;
    if (QWExtra *topData = window()->d_func()->extra) {
        if (topData->proxyWidget && topData->proxyWidget->hasFocus()) {
            previousProxyFocus = topData->proxyWidget->widget()->focusWidget();
            if (previousProxyFocus && previousProxyFocus->focusProxy())
                previousProxyFocus = previousProxyFocus->focusProxy();
            if (previousProxyFocus == this && !topData->proxyWidget->d_func()->proxyIsGivingFocus)
                return;
        }
    }
#endif

    QWidget *w = f;
    if (isHidden()) {
        while (w && w->isHidden()) {
            w->d_func()->focus_child = f;
            w = w->isWindow() ? 0 : w->parentWidget();
        }
    } else {
        while (w) {
            w->d_func()->focus_child = f;
            w = w->isWindow() ? 0 : w->parentWidget();
        }
    }

#ifndef QT_NO_GRAPHICSVIEW
    // Update proxy state
    if (QWExtra *topData = window()->d_func()->extra) {
        if (topData->proxyWidget && !topData->proxyWidget->hasFocus()) {
            topData->proxyWidget->d_func()->focusFromWidgetToProxy = 1;
            topData->proxyWidget->setFocus(reason);
            topData->proxyWidget->d_func()->focusFromWidgetToProxy = 0;
        }
    }
#endif

    if (f->isActiveWindow()) {
        QApplicationPrivate::setFocusWidget(f, reason);
#ifndef QT_NO_ACCESSIBILITY
# ifdef Q_OS_WIN
        // The negation of the condition in setFocus_sys
        if (!(testAttribute(Qt::WA_WState_Created) && window()->windowType() != Qt::Popup && internalWinId()))
            //setFocusWidget will already post a focus event for us (that the AT client receives) on Windows
# endif
# ifdef  Q_OS_UNIX
        // menus update the focus manually and this would create bogus events
        if (!(f->inherits("QMenuBar") || f->inherits("QMenu") || f->inherits("QMenuItem")))
# endif
            QAccessible::updateAccessibility(f, 0, QAccessible::Focus);
#endif
#ifndef QT_NO_GRAPHICSVIEW
        if (QWExtra *topData = window()->d_func()->extra) {
            if (topData->proxyWidget) {
                if (previousProxyFocus && previousProxyFocus != f) {
                    // Send event to self
                    QFocusEvent event(QEvent::FocusOut, reason);
                    QPointer<QWidget> that = previousProxyFocus;
                    QApplication::sendEvent(previousProxyFocus, &event);
                    if (that)
                        QApplication::sendEvent(that->style(), &event);
                }
                if (!isHidden()) {
#ifndef QT_NO_GRAPHICSVIEW
                    // Update proxy state
                    if (QWExtra *topData = window()->d_func()->extra)
                        if (topData->proxyWidget && topData->proxyWidget->hasFocus())
                            topData->proxyWidget->d_func()->updateProxyInputMethodAcceptanceFromWidget();
#endif
                    // Send event to self
                    QFocusEvent event(QEvent::FocusIn, reason);
                    QPointer<QWidget> that = f;
                    QApplication::sendEvent(f, &event);
                    if (that)
                        QApplication::sendEvent(that->style(), &event);
                }
            }
        }
#endif
    }
}

/*!
    \fn void QWidget::setFocus()
    \overload

    Gives the keyboard input focus to this widget (or its focus
    proxy) if this widget or one of its parents is the
    \l{isActiveWindow()}{active window}.
*/

/*!
    Takes keyboard input focus from the widget.

    If the widget has active focus, a \link focusOutEvent() focus out
    event\endlink is sent to this widget to tell it that it is about
    to lose the focus.

    This widget must enable focus setting in order to get the keyboard
    input focus, i.e. it must call setFocusPolicy().

    \sa hasFocus(), setFocus(), focusInEvent(), focusOutEvent(),
    setFocusPolicy(), QApplication::focusWidget()
*/

void QWidget::clearFocus()
{
    QWidget *w = this;
    while (w) {
        if (w->d_func()->focus_child == this)
            w->d_func()->focus_child = 0;
        w = w->parentWidget();
    }
#ifndef QT_NO_GRAPHICSVIEW
    QWExtra *topData = d_func()->extra;
    if (topData && topData->proxyWidget)
        topData->proxyWidget->clearFocus();
#endif

    if (hasFocus()) {
        // Update proxy state
        QApplicationPrivate::setFocusWidget(0, Qt::OtherFocusReason);
#if defined(Q_WS_WIN)
        if (!(windowType() == Qt::Popup) && GetFocus() == internalWinId())
            SetFocus(0);
        else
#endif
        {
#ifndef QT_NO_ACCESSIBILITY
            QAccessible::updateAccessibility(this, 0, QAccessible::Focus);
#endif
        }
    }
}


/*!
    \fn bool QWidget::focusNextChild()

    Finds a new widget to give the keyboard focus to, as appropriate
    for \key Tab, and returns true if it can find a new widget, or
    false if it can't.

    \sa focusPreviousChild()
*/

/*!
    \fn bool QWidget::focusPreviousChild()

    Finds a new widget to give the keyboard focus to, as appropriate
    for \key Shift+Tab, and returns true if it can find a new widget,
    or false if it can't.

    \sa focusNextChild()
*/

/*!
    Finds a new widget to give the keyboard focus to, as appropriate
    for Tab and Shift+Tab, and returns true if it can find a new
    widget, or false if it can't.

    If \a next is true, this function searches forward, if \a next
    is false, it searches backward.

    Sometimes, you will want to reimplement this function. For
    example, a web browser might reimplement it to move its "current
    active link" forward or backward, and call
    focusNextPrevChild() only when it reaches the last or
    first link on the "page".

    Child widgets call focusNextPrevChild() on their parent widgets,
    but only the window that contains the child widgets decides where
    to redirect focus. By reimplementing this function for an object,
    you thus gain control of focus traversal for all child widgets.

    \sa focusNextChild(), focusPreviousChild()
*/

bool QWidget::focusNextPrevChild(bool next)
{
    Q_D(QWidget);
    QWidget* p = parentWidget();
    bool isSubWindow = (windowType() == Qt::SubWindow);
    if (!isWindow() && !isSubWindow && p)
        return p->focusNextPrevChild(next);
#ifndef QT_NO_GRAPHICSVIEW
    if (d->extra && d->extra->proxyWidget)
        return d->extra->proxyWidget->focusNextPrevChild(next);
#endif
    QWidget *w = QApplicationPrivate::focusNextPrevChild_helper(this, next);
    if (!w) return false;

    w->setFocus(next ? Qt::TabFocusReason : Qt::BacktabFocusReason);
    return true;
}

/*!
    Returns the last child of this widget that setFocus had been
    called on.  For top level widgets this is the widget that will get
    focus in case this window gets activated

    This is not the same as QApplication::focusWidget(), which returns
    the focus widget in the currently active window.
*/

QWidget *QWidget::focusWidget() const
{
    return const_cast<QWidget *>(d_func()->focus_child);
}

/*!
    Returns the next widget in this widget's focus chain.

    \sa previousInFocusChain()
*/
QWidget *QWidget::nextInFocusChain() const
{
    return const_cast<QWidget *>(d_func()->focus_next);
}

/*!
    \brief The previousInFocusChain function returns the previous
    widget in this widget's focus chain.

    \sa nextInFocusChain()

    \since 4.6
*/
QWidget *QWidget::previousInFocusChain() const
{
    return const_cast<QWidget *>(d_func()->focus_prev);
}

/*!
    \property QWidget::isActiveWindow
    \brief whether this widget's window is the active window

    The active window is the window that contains the widget that has
    keyboard focus (The window may still have focus if it has no
    widgets or none of its widgets accepts keyboard focus).

    When popup windows are visible, this property is true for both the
    active window \e and for the popup.

    By default, this property is false.

    \sa activateWindow(), QApplication::activeWindow()
*/
bool QWidget::isActiveWindow() const
{
    QWidget *tlw = window();
    if(tlw == QApplication::activeWindow() || (isVisible() && (tlw->windowType() == Qt::Popup)))
        return true;

#ifndef QT_NO_GRAPHICSVIEW
    if (QWExtra *tlwExtra = tlw->d_func()->extra) {
        if (isVisible() && tlwExtra->proxyWidget)
            return tlwExtra->proxyWidget->isActiveWindow();
    }
#endif

#ifdef Q_WS_MAC
    extern bool qt_mac_is_macdrawer(const QWidget *); //qwidget_mac.cpp
    if(qt_mac_is_macdrawer(tlw) &&
       tlw->parentWidget() && tlw->parentWidget()->isActiveWindow())
        return true;

    extern bool qt_mac_insideKeyWindow(const QWidget *); //qwidget_mac.cpp
    if (QApplication::testAttribute(Qt::AA_MacPluginApplication) && qt_mac_insideKeyWindow(tlw))
        return true;
#endif
    if(style()->styleHint(QStyle::SH_Widget_ShareActivation, 0, this)) {
        if(tlw->windowType() == Qt::Tool &&
           !tlw->isModal() &&
           (!tlw->parentWidget() || tlw->parentWidget()->isActiveWindow()))
           return true;
        QWidget *w = QApplication::activeWindow();
        while(w && tlw->windowType() == Qt::Tool &&
              !w->isModal() && w->parentWidget()) {
            w = w->parentWidget()->window();
            if(w == tlw)
                return true;
        }
    }
#if defined(Q_WS_WIN32)
    HWND active = GetActiveWindow();
    if (!tlw->testAttribute(Qt::WA_WState_Created))
        return false;
    return active == tlw->internalWinId() || ::IsChild(active, tlw->internalWinId());
#else
    return false;
#endif
}

/*!
    Puts the \a second widget after the \a first widget in the focus order.

    Note that since the tab order of the \a second widget is changed, you
    should order a chain like this:

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 9

    \e not like this:

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 10

    If \a first or \a second has a focus proxy, setTabOrder()
    correctly substitutes the proxy.

    \sa setFocusPolicy(), setFocusProxy(), {Keyboard Focus}
*/
void QWidget::setTabOrder(QWidget* first, QWidget *second)
{
    if (!first || !second || first->focusPolicy() == Qt::NoFocus || second->focusPolicy() == Qt::NoFocus)
        return;

    if (first->window() != second->window()) {
        qWarning("QWidget::setTabOrder: 'first' and 'second' must be in the same window");
        return;
    }

    QWidget *fp = first->focusProxy();
    if (fp) {
        // If first is redirected, set first to the last child of first
        // that can take keyboard focus so that second is inserted after
        // that last child, and the focus order within first is (more
        // likely to be) preserved.
        QList<QWidget *> l = first->findChildren<QWidget *>();
        for (int i = l.size()-1; i >= 0; --i) {
            QWidget * next = l.at(i);
            if (next->window() == fp->window()) {
                fp = next;
                if (fp->focusPolicy() != Qt::NoFocus)
                    break;
            }
        }
        first = fp;
    }

    if (fp == second)
        return;

    if (QWidget *sp = second->focusProxy())
        second = sp;

//    QWidget *fp = first->d_func()->focus_prev;
    QWidget *fn = first->d_func()->focus_next;

    if (fn == second || first == second)
        return;

    QWidget *sp = second->d_func()->focus_prev;
    QWidget *sn = second->d_func()->focus_next;

    fn->d_func()->focus_prev = second;
    first->d_func()->focus_next = second;

    second->d_func()->focus_next = fn;
    second->d_func()->focus_prev = first;

    sp->d_func()->focus_next = sn;
    sn->d_func()->focus_prev = sp;


    Q_ASSERT(first->d_func()->focus_next->d_func()->focus_prev == first);
    Q_ASSERT(first->d_func()->focus_prev->d_func()->focus_next == first);

    Q_ASSERT(second->d_func()->focus_next->d_func()->focus_prev == second);
    Q_ASSERT(second->d_func()->focus_prev->d_func()->focus_next == second);
}

/*!\internal

  Moves the relevant subwidgets of this widget from the \a oldtlw's
  tab chain to that of the new parent, if there's anything to move and
  we're really moving

  This function is called from QWidget::reparent() *after* the widget
  has been reparented.

  \sa reparent()
*/

void QWidgetPrivate::reparentFocusWidgets(QWidget * oldtlw)
{
    Q_Q(QWidget);
    if (oldtlw == q->window())
        return; // nothing to do

    if(focus_child)
        focus_child->clearFocus();

    // separate the focus chain into new (children of myself) and old (the rest)
    QWidget *firstOld = 0;
    //QWidget *firstNew = q; //invariant
    QWidget *o = 0; // last in the old list
    QWidget *n = q; // last in the new list

    bool prevWasNew = true;
    QWidget *w = focus_next;

    //Note: for efficiency, we do not maintain the list invariant inside the loop
    //we append items to the relevant list, and we optimize by not changing pointers
    //when subsequent items are going into the same list.
    while (w  != q) {
        bool currentIsNew =  q->isAncestorOf(w);
        if (currentIsNew) {
            if (!prevWasNew) {
                //prev was old -- append to new list
                n->d_func()->focus_next = w;
                w->d_func()->focus_prev = n;
            }
            n = w;
        } else {
            if (prevWasNew) {
                //prev was new -- append to old list, if there is one
                if (o) {
                    o->d_func()->focus_next = w;
                    w->d_func()->focus_prev = o;
                } else {
                    // "create" the old list
                    firstOld = w;
                }
            }
            o = w;
        }
        w = w->d_func()->focus_next;
        prevWasNew = currentIsNew;
    }

    //repair the old list:
    if (firstOld) {
        o->d_func()->focus_next = firstOld;
        firstOld->d_func()->focus_prev = o;
    }

    if (!q->isWindow()) {
        QWidget *topLevel = q->window();
        //insert new chain into toplevel's chain

        QWidget *prev = topLevel->d_func()->focus_prev;

        topLevel->d_func()->focus_prev = n;
        prev->d_func()->focus_next = q;

        focus_prev = prev;
        n->d_func()->focus_next = topLevel;
    } else {
        //repair the new list
            n->d_func()->focus_next = q;
            focus_prev = n;
    }

}

/*!\internal

  Measures the shortest distance from a point to a rect.

  This function is called from QDesktopwidget::screen(QPoint) to find the
  closest screen for a point.
  In directional KeypadNavigation, it is called to find the closest
  widget to the current focus widget center.
*/
int QWidgetPrivate::pointToRect(const QPoint &p, const QRect &r)
{
    int dx = 0;
    int dy = 0;
    if (p.x() < r.left())
        dx = r.left() - p.x();
    else if (p.x() > r.right())
        dx = p.x() - r.right();
    if (p.y() < r.top())
        dy = r.top() - p.y();
    else if (p.y() > r.bottom())
        dy = p.y() - r.bottom();
    return dx + dy;
}

/*!
    \property QWidget::frameSize
    \brief the size of the widget including any window frame

    By default, this property contains a value that depends on the user's
    platform and screen geometry.
*/
QSize QWidget::frameSize() const
{
    Q_D(const QWidget);
    if (isWindow() && !(windowType() == Qt::Popup)) {
        QRect fs = d->frameStrut();
        return QSize(data->crect.width() + fs.left() + fs.right(),
                      data->crect.height() + fs.top() + fs.bottom());
    }
    return data->crect.size();
}

/*! \fn void QWidget::move(int x, int y)

    \overload

    This corresponds to move(QPoint(\a x, \a y)).
*/

void QWidget::move(const QPoint &p)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_Moved);
    if (isWindow())
        d->topData()->posFromMove = true;
    if (testAttribute(Qt::WA_WState_Created)) {
        d->setGeometry_sys(p.x() + geometry().x() - QWidget::x(),
                       p.y() + geometry().y() - QWidget::y(),
                       width(), height(), true);
        d->setDirtyOpaqueRegion();
    } else {
        data->crect.moveTopLeft(p); // no frame yet
        setAttribute(Qt::WA_PendingMoveEvent);
    }
}

/*! \fn void QWidget::resize(int w, int h)
    \overload

    This corresponds to resize(QSize(\a w, \a h)).
*/

void QWidget::resize(const QSize &s)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_Resized);
    if (testAttribute(Qt::WA_WState_Created)) {
        d->setGeometry_sys(geometry().x(), geometry().y(), s.width(), s.height(), false);
        d->setDirtyOpaqueRegion();
    } else {
        data->crect.setSize(s.boundedTo(maximumSize()).expandedTo(minimumSize()));
        setAttribute(Qt::WA_PendingResizeEvent);
    }
}

void QWidget::setGeometry(const QRect &r)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_Resized);
    setAttribute(Qt::WA_Moved);
    if (isWindow())
        d->topData()->posFromMove = false;
    if (testAttribute(Qt::WA_WState_Created)) {
        d->setGeometry_sys(r.x(), r.y(), r.width(), r.height(), true);
        d->setDirtyOpaqueRegion();
    } else {
        data->crect.setTopLeft(r.topLeft());
        data->crect.setSize(r.size().boundedTo(maximumSize()).expandedTo(minimumSize()));
        setAttribute(Qt::WA_PendingMoveEvent);
        setAttribute(Qt::WA_PendingResizeEvent);
    }
}

/*!
    \since 4.2
    Saves the current geometry and state for top-level widgets.

    To save the geometry when the window closes, you can
    implement a close event like this:

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 11

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    Use QMainWindow::saveState() to save the geometry and the state of
    toolbars and dock widgets.

    \sa restoreGeometry(), QMainWindow::saveState(), QMainWindow::restoreState()
*/
QByteArray QWidget::saveGeometry() const
{
#ifdef QT_MAC_USE_COCOA
    // We check if the window was maximized during this invocation. If so, we need to record the
    // starting position as 0,0.
    Q_D(const QWidget);
    QRect newFramePosition = frameGeometry();
    QRect newNormalPosition = normalGeometry();
    if(d->topData()->wasMaximized && !(windowState() & Qt::WindowMaximized)) {
        // Change the starting position
        newFramePosition.moveTo(0, 0);
        newNormalPosition.moveTo(0, 0);
    }
#endif // QT_MAC_USE_COCOA
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_0);
    const quint32 magicNumber = 0x1D9D0CB;
    quint16 majorVersion = 1;
    quint16 minorVersion = 0;
    stream << magicNumber
           << majorVersion
           << minorVersion
#ifdef QT_MAC_USE_COCOA
           << newFramePosition
           << newNormalPosition
#else
           << frameGeometry()
           << normalGeometry()
#endif // QT_MAC_USE_COCOA
           << qint32(QApplication::desktop()->screenNumber(this))
           << quint8(windowState() & Qt::WindowMaximized)
           << quint8(windowState() & Qt::WindowFullScreen);
    return array;
}

/*!
    \since 4.2

    Restores the geometry and state top-level widgets stored in the
    byte array \a geometry. Returns true on success; otherwise
    returns false.

    If the restored geometry is off-screen, it will be modified to be
    inside the available screen geometry.

    To restore geometry saved using QSettings, you can use code like
    this:

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 12

    See the \l{Window Geometry} documentation for an overview of geometry
    issues with windows.

    Use QMainWindow::restoreState() to restore the geometry and the
    state of toolbars and dock widgets.

    \sa saveGeometry(), QSettings, QMainWindow::saveState(), QMainWindow::restoreState()
*/
bool QWidget::restoreGeometry(const QByteArray &geometry)
{
    if (geometry.size() < 4)
        return false;
    QDataStream stream(geometry);
    stream.setVersion(QDataStream::Qt_4_0);

    const quint32 magicNumber = 0x1D9D0CB;
    quint32 storedMagicNumber;
    stream >> storedMagicNumber;
    if (storedMagicNumber != magicNumber)
        return false;

    const quint16 currentMajorVersion = 1;
    quint16 majorVersion = 0;
    quint16 minorVersion = 0;

    stream >> majorVersion >> minorVersion;

    if (majorVersion != currentMajorVersion)
        return false;
    // (Allow all minor versions.)

    QRect restoredFrameGeometry;
     QRect restoredNormalGeometry;
    qint32 restoredScreenNumber;
    quint8 maximized;
    quint8 fullScreen;

    stream >> restoredFrameGeometry
           >> restoredNormalGeometry
           >> restoredScreenNumber
           >> maximized
           >> fullScreen;

    const int frameHeight = 20;
    if (!restoredFrameGeometry.isValid())
        restoredFrameGeometry = QRect(QPoint(0,0), sizeHint());

    if (!restoredNormalGeometry.isValid())
        restoredNormalGeometry = QRect(QPoint(0, frameHeight), sizeHint());
    if (!restoredNormalGeometry.isValid()) {
        // use the widget's adjustedSize if the sizeHint() doesn't help
        restoredNormalGeometry.setSize(restoredNormalGeometry
                                       .size()
                                       .expandedTo(d_func()->adjustedSize()));
    }

    const QDesktopWidget * const desktop = QApplication::desktop();
    if (restoredScreenNumber >= desktop->numScreens())
        restoredScreenNumber = desktop->primaryScreen();

    const QRect availableGeometry = desktop->availableGeometry(restoredScreenNumber);

    // Modify the restored geometry if we are about to restore to coordinates
    // that would make the window "lost". This happens if:
    // - The restored geometry is completely oustside the available geometry
    // - The title bar is outside the available geometry.
    // - (Mac only) The window is higher than the available geometry. It must
    //   be possible to bring the size grip on screen by moving the window.
#ifdef Q_WS_MAC
    restoredFrameGeometry.setHeight(qMin(restoredFrameGeometry.height(), availableGeometry.height()));
    restoredNormalGeometry.setHeight(qMin(restoredNormalGeometry.height(), availableGeometry.height() - frameHeight));
#endif

    if (!restoredFrameGeometry.intersects(availableGeometry)) {
        restoredFrameGeometry.moveBottom(qMin(restoredFrameGeometry.bottom(), availableGeometry.bottom()));
        restoredFrameGeometry.moveLeft(qMax(restoredFrameGeometry.left(), availableGeometry.left()));
        restoredFrameGeometry.moveRight(qMin(restoredFrameGeometry.right(), availableGeometry.right()));
    }
    restoredFrameGeometry.moveTop(qMax(restoredFrameGeometry.top(), availableGeometry.top()));

    if (!restoredNormalGeometry.intersects(availableGeometry)) {
        restoredNormalGeometry.moveBottom(qMin(restoredNormalGeometry.bottom(), availableGeometry.bottom()));
        restoredNormalGeometry.moveLeft(qMax(restoredNormalGeometry.left(), availableGeometry.left()));
        restoredNormalGeometry.moveRight(qMin(restoredNormalGeometry.right(), availableGeometry.right()));
    }
    restoredNormalGeometry.moveTop(qMax(restoredNormalGeometry.top(), availableGeometry.top() + frameHeight));

    if (maximized || fullScreen) {
        // set geomerty before setting the window state to make
        // sure the window is maximized to the right screen.
        // Skip on windows: the window is restored into a broken
        // half-maximized state.
#ifndef Q_WS_WIN
        setGeometry(restoredNormalGeometry);
#endif
        Qt::WindowStates ws = windowState();
        if (maximized)
            ws |= Qt::WindowMaximized;
        if (fullScreen)
            ws |= Qt::WindowFullScreen;
       setWindowState(ws);
       d_func()->topData()->normalGeometry = restoredNormalGeometry;
    } else {
        QPoint offset;
#ifdef Q_WS_X11
        if (isFullScreen())
            offset = d_func()->topData()->fullScreenOffset;
#endif
        setWindowState(windowState() & ~(Qt::WindowMaximized | Qt::WindowFullScreen));
        move(restoredFrameGeometry.topLeft() + offset);
        resize(restoredNormalGeometry.size());
    }
    return true;
}

/*!\fn void QWidget::setGeometry(int x, int y, int w, int h)
    \overload

    This corresponds to setGeometry(QRect(\a x, \a y, \a w, \a h)).
*/

/*!
  Sets the margins around the contents of the widget to have the sizes
  \a left, \a top, \a right, and \a bottom. The margins are used by
  the layout system, and may be used by subclasses to specify the area
  to draw in (e.g. excluding the frame).

  Changing the margins will trigger a resizeEvent().

  \sa contentsRect(), getContentsMargins()
*/
void QWidget::setContentsMargins(int left, int top, int right, int bottom)
{
    Q_D(QWidget);
    if (left == d->leftmargin && top == d->topmargin
         && right == d->rightmargin && bottom == d->bottommargin)
        return;
    d->leftmargin = left;
    d->topmargin = top;
    d->rightmargin = right;
    d->bottommargin = bottom;

    if (QLayout *l=d->layout)
        l->update(); //force activate; will do updateGeometry
    else
        updateGeometry();

    // ### Qt 5: compat, remove
    if (isVisible()) {
        update();
        QResizeEvent e(data->crect.size(), data->crect.size());
        QApplication::sendEvent(this, &e);
    } else {
        setAttribute(Qt::WA_PendingResizeEvent, true);
    }

    QEvent e(QEvent::ContentsRectChange);
    QApplication::sendEvent(this, &e);
}

/*!
  \overload
  \since 4.6

  \brief The setContentsMargins function sets the margins around the
  widget's contents.

  Sets the margins around the contents of the widget to have the
  sizes determined by \a margins. The margins are
  used by the layout system, and may be used by subclasses to
  specify the area to draw in (e.g. excluding the frame).

  Changing the margins will trigger a resizeEvent().

  \sa contentsRect(), getContentsMargins()
*/
void QWidget::setContentsMargins(const QMargins &margins)
{
    setContentsMargins(margins.left(), margins.top(),
                       margins.right(), margins.bottom());
}

/*!
  Returns the widget's contents margins for \a left, \a top, \a
  right, and \a bottom.

  \sa setContentsMargins(), contentsRect()
 */
void QWidget::getContentsMargins(int *left, int *top, int *right, int *bottom) const
{
    Q_D(const QWidget);
    if (left)
        *left = d->leftmargin;
    if (top)
        *top = d->topmargin;
    if (right)
        *right = d->rightmargin;
    if (bottom)
        *bottom = d->bottommargin;
}

/*!
  \since 4.6

  \brief The contentsMargins function returns the widget's contents margins.

  \sa getContentsMargins(), setContentsMargins(), contentsRect()
 */
QMargins QWidget::contentsMargins() const
{
    Q_D(const QWidget);
    return QMargins(d->leftmargin, d->topmargin, d->rightmargin, d->bottommargin);
}


/*!
    Returns the area inside the widget's margins.

    \sa setContentsMargins(), getContentsMargins()
*/
QRect QWidget::contentsRect() const
{
    Q_D(const QWidget);
    return QRect(QPoint(d->leftmargin, d->topmargin),
                 QPoint(data->crect.width() - 1 - d->rightmargin,
                        data->crect.height() - 1 - d->bottommargin));

}



/*!
  \fn void QWidget::customContextMenuRequested(const QPoint &pos)

  This signal is emitted when the widget's \l contextMenuPolicy is
  Qt::CustomContextMenu, and the user has requested a context menu on
  the widget. The position \a pos is the position of the context menu
  event that the widget receives. Normally this is in widget
  coordinates. The exception to this rule is QAbstractScrollArea and
  its subclasses that map the context menu event to coordinates of the
  \link QAbstractScrollArea::viewport() viewport() \endlink .


  \sa mapToGlobal() QMenu contextMenuPolicy
*/


/*!
    \property QWidget::contextMenuPolicy
    \brief how the widget shows a context menu

    The default value of this property is Qt::DefaultContextMenu,
    which means the contextMenuEvent() handler is called. Other values
    are Qt::NoContextMenu, Qt::PreventContextMenu,
    Qt::ActionsContextMenu, and Qt::CustomContextMenu. With
    Qt::CustomContextMenu, the signal customContextMenuRequested() is
    emitted.

    \sa contextMenuEvent(), customContextMenuRequested(), actions()
*/

Qt::ContextMenuPolicy QWidget::contextMenuPolicy() const
{
    return (Qt::ContextMenuPolicy)data->context_menu_policy;
}

void QWidget::setContextMenuPolicy(Qt::ContextMenuPolicy policy)
{
    data->context_menu_policy = (uint) policy;
}

/*!
    \property QWidget::focusPolicy
    \brief the way the widget accepts keyboard focus

    The policy is Qt::TabFocus if the widget accepts keyboard
    focus by tabbing, Qt::ClickFocus if the widget accepts
    focus by clicking, Qt::StrongFocus if it accepts both, and
    Qt::NoFocus (the default) if it does not accept focus at
    all.

    You must enable keyboard focus for a widget if it processes
    keyboard events. This is normally done from the widget's
    constructor. For instance, the QLineEdit constructor calls
    setFocusPolicy(Qt::StrongFocus).

    If the widget has a focus proxy, then the focus policy will
    be propagated to it.

    \sa focusInEvent(), focusOutEvent(), keyPressEvent(), keyReleaseEvent(), enabled
*/


Qt::FocusPolicy QWidget::focusPolicy() const
{
    return (Qt::FocusPolicy)data->focus_policy;
}

void QWidget::setFocusPolicy(Qt::FocusPolicy policy)
{
    data->focus_policy = (uint) policy;
    Q_D(QWidget);
    if (d->extra && d->extra->focus_proxy)
        d->extra->focus_proxy->setFocusPolicy(policy);
}

/*!
    \property QWidget::updatesEnabled
    \brief whether updates are enabled

    An updates enabled widget receives paint events and has a system
    background; a disabled widget does not. This also implies that
    calling update() and repaint() has no effect if updates are
    disabled.

    By default, this property is true.

    setUpdatesEnabled() is normally used to disable updates for a
    short period of time, for instance to avoid screen flicker during
    large changes. In Qt, widgets normally do not generate screen
    flicker, but on X11 the server might erase regions on the screen
    when widgets get hidden before they can be replaced by other
    widgets. Disabling updates solves this.

    Example:
    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 13

    Disabling a widget implicitly disables all its children. Enabling a widget
    enables all child widgets \e except top-level widgets or those that
    have been explicitly disabled. Re-enabling updates implicitly calls
    update() on the widget.

    \sa paintEvent()
*/
void QWidget::setUpdatesEnabled(bool enable)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_ForceUpdatesDisabled, !enable);
    d->setUpdatesEnabled_helper(enable);
}

/*!  \fn void QWidget::show()

    Shows the widget and its child widgets. This function is
    equivalent to setVisible(true).

    \sa raise(), showEvent(), hide(), setVisible(), showMinimized(), showMaximized(),
    showNormal(), isVisible()
*/


/*! \internal

   Makes the widget visible in the isVisible() meaning of the word.
   It is only called for toplevels or widgets with visible parents.
 */
void QWidgetPrivate::show_recursive()
{
    Q_Q(QWidget);
    // polish if necessary

    if (!q->testAttribute(Qt::WA_WState_Created))
        createRecursively();
    q->ensurePolished();

#ifdef QT3_SUPPORT
    if(sendChildEvents)
        QApplication::sendPostedEvents(q, QEvent::ChildInserted);
#endif
    if (!q->isWindow() && q->parentWidget()->d_func()->layout && !q->parentWidget()->data->in_show)
        q->parentWidget()->d_func()->layout->activate();
    // activate our layout before we and our children become visible
    if (layout)
        layout->activate();

    show_helper();
}

void QWidgetPrivate::sendPendingMoveAndResizeEvents(bool recursive, bool disableUpdates)
{
    Q_Q(QWidget);

    disableUpdates = disableUpdates && q->updatesEnabled();
    if (disableUpdates)
        q->setAttribute(Qt::WA_UpdatesDisabled);

    if (q->testAttribute(Qt::WA_PendingMoveEvent)) {
        QMoveEvent e(data.crect.topLeft(), data.crect.topLeft());
        QApplication::sendEvent(q, &e);
        q->setAttribute(Qt::WA_PendingMoveEvent, false);
    }

    if (q->testAttribute(Qt::WA_PendingResizeEvent)) {
        QResizeEvent e(data.crect.size(), QSize());
        QApplication::sendEvent(q, &e);
        q->setAttribute(Qt::WA_PendingResizeEvent, false);
    }

    if (disableUpdates)
        q->setAttribute(Qt::WA_UpdatesDisabled, false);

    if (!recursive)
        return;

    for (int i = 0; i < children.size(); ++i) {
        if (QWidget *child = qobject_cast<QWidget *>(children.at(i)))
            child->d_func()->sendPendingMoveAndResizeEvents(recursive, disableUpdates);
    }
}

void QWidgetPrivate::activateChildLayoutsRecursively()
{
    sendPendingMoveAndResizeEvents(false, true);

    for (int i = 0; i < children.size(); ++i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (!child || child->isHidden() || child->isWindow())
            continue;

        child->ensurePolished();

        // Activate child's layout
        QWidgetPrivate *childPrivate = child->d_func();
        if (childPrivate->layout)
            childPrivate->layout->activate();

        // Pretend we're visible.
        const bool wasVisible = child->isVisible();
        if (!wasVisible)
            child->setAttribute(Qt::WA_WState_Visible);

        // Do the same for all my children.
        childPrivate->activateChildLayoutsRecursively();

        // We're not cheating anymore.
        if (!wasVisible)
            child->setAttribute(Qt::WA_WState_Visible, false);
    }
}

void QWidgetPrivate::show_helper()
{
    Q_Q(QWidget);
    data.in_show = true; // qws optimization
    // make sure we receive pending move and resize events
    sendPendingMoveAndResizeEvents();

    // become visible before showing all children
    q->setAttribute(Qt::WA_WState_Visible);

    // finally show all children recursively
    showChildren(false);

#ifdef QT3_SUPPORT
    if (q->parentWidget() && sendChildEvents)
        QApplication::sendPostedEvents(q->parentWidget(),
                                        QEvent::ChildInserted);
#endif


    // popup handling: new popups and tools need to be raised, and
    // existing popups must be closed. Also propagate the current
    // windows's KeyboardFocusChange status.
    if (q->isWindow()) {
        if ((q->windowType() == Qt::Tool) || (q->windowType() == Qt::Popup) || q->windowType() == Qt::ToolTip) {
            q->raise();
            if (q->parentWidget() && q->parentWidget()->window()->testAttribute(Qt::WA_KeyboardFocusChange))
                q->setAttribute(Qt::WA_KeyboardFocusChange);
        } else {
            while (QApplication::activePopupWidget()) {
                if (!QApplication::activePopupWidget()->close())
                    break;
            }
        }
    }

    // Automatic embedding of child windows of widgets already embedded into
    // QGraphicsProxyWidget when they are shown the first time.
    bool isEmbedded = false;
#ifndef QT_NO_GRAPHICSVIEW
    if (q->isWindow()) {
        isEmbedded = q->graphicsProxyWidget() ? true : false;
        if (!isEmbedded && !bypassGraphicsProxyWidget(q)) {
            QGraphicsProxyWidget *ancestorProxy = nearestGraphicsProxyWidget(q->parentWidget());
            if (ancestorProxy) {
                isEmbedded = true;
                ancestorProxy->d_func()->embedSubWindow(q);
            }
        }
    }
#else
    Q_UNUSED(isEmbedded);
#endif

    // On Windows, show the popup now so that our own focus handling
    // stores the correct old focus widget even if it's stolen in the
    // showevent
#if defined(Q_WS_WIN) || defined(Q_WS_MAC) || defined(Q_OS_SYMBIAN)
    if (!isEmbedded && q->windowType() == Qt::Popup)
        qApp->d_func()->openPopup(q);
#endif

    // send the show event before showing the window
    QShowEvent showEvent;
    QApplication::sendEvent(q, &showEvent);

    if (!isEmbedded && q->isModal() && q->isWindow())
        // QApplicationPrivate::enterModal *before* show, otherwise the initial
        // stacking might be wrong
        QApplicationPrivate::enterModal(q);


    show_sys();

#if !defined(Q_WS_WIN) && !defined(Q_WS_MAC) && !defined(Q_OS_SYMBIAN)
    if (!isEmbedded && q->windowType() == Qt::Popup)
        qApp->d_func()->openPopup(q);
#endif

#ifndef QT_NO_ACCESSIBILITY
    if (q->windowType() != Qt::ToolTip)     // Tooltips are read aloud twice in MS narrator.
        QAccessible::updateAccessibility(q, 0, QAccessible::ObjectShow);
#endif

    if (QApplicationPrivate::hidden_focus_widget == q) {
        QApplicationPrivate::hidden_focus_widget = 0;
        q->setFocus(Qt::OtherFocusReason);
    }

    // Process events when showing a Qt::SplashScreen widget before the event loop
    // is spinnning; otherwise it might not show up on particular platforms.
    // This makes QSplashScreen behave the same on all platforms.
    if (!qApp->d_func()->in_exec && q->windowType() == Qt::SplashScreen)
        QApplication::processEvents();

    data.in_show = false;  // reset qws optimization
}

/*! \fn void QWidget::hide()

    Hides the widget. This function is equivalent to
    setVisible(false).


    \note If you are working with QDialog or its subclasses and you invoke
    the show() function after this function, the dialog will be displayed in
    its original position.

    \sa hideEvent(), isHidden(), show(), setVisible(), isVisible(), close()
*/

/*!\internal
 */
void QWidgetPrivate::hide_helper()
{
    Q_Q(QWidget);

    bool isEmbedded = false;
#if !defined QT_NO_GRAPHICSVIEW
    isEmbedded = q->isWindow() && !bypassGraphicsProxyWidget(q) && nearestGraphicsProxyWidget(q->parentWidget()) != 0;
#else
    Q_UNUSED(isEmbedded);
#endif

    if (!isEmbedded && (q->windowType() == Qt::Popup))
        qApp->d_func()->closePopup(q);

    // Move test modal here.  Otherwise, a modal dialog could get
    // destroyed and we lose all access to its parent because we haven't
    // left modality.  (Eg. modal Progress Dialog)
    if (!isEmbedded && q->isModal() && q->isWindow())
        QApplicationPrivate::leaveModal(q);

#if defined(Q_WS_WIN)
    if (q->isWindow() && !(q->windowType() == Qt::Popup) && q->parentWidget()
        && !q->parentWidget()->isHidden() && q->isActiveWindow())
        q->parentWidget()->activateWindow();        // Activate parent
#endif

    q->setAttribute(Qt::WA_Mapped, false);
    hide_sys();

    bool wasVisible = q->testAttribute(Qt::WA_WState_Visible);

    if (wasVisible) {
        q->setAttribute(Qt::WA_WState_Visible, false);

    }

    QHideEvent hideEvent;
    QApplication::sendEvent(q, &hideEvent);
    hideChildren(false);

    // next bit tries to move the focus if the focus widget is now
    // hidden.
    if (wasVisible) {
#if defined(Q_WS_WIN) || defined(Q_WS_X11) || defined (Q_WS_QWS) || defined(Q_WS_MAC) || defined(Q_WS_QPA)
        qApp->d_func()->sendSyntheticEnterLeave(q);
#endif

        QWidget *fw = QApplication::focusWidget();
        while (fw &&  !fw->isWindow()) {
            if (fw == q) {
                q->focusNextPrevChild(true);
                break;
            }
            fw = fw->parentWidget();
        }
    }

    if (QWidgetBackingStore *bs = maybeBackingStore())
        bs->removeDirtyWidget(q);

#ifndef QT_NO_ACCESSIBILITY
    if (wasVisible)
        QAccessible::updateAccessibility(q, 0, QAccessible::ObjectHide);
#endif
}

/*!
    \fn bool QWidget::isHidden() const

    Returns true if the widget is hidden, otherwise returns false.

    A hidden widget will only become visible when show() is called on
    it. It will not be automatically shown when the parent is shown.

    To check visibility, use !isVisible() instead (notice the exclamation mark).

    isHidden() implies !isVisible(), but a widget can be not visible
    and not hidden at the same time. This is the case for widgets that are children of
    widgets that are not visible.


    Widgets are hidden if:
    \list
        \o they were created as independent windows,
        \o they were created as children of visible widgets,
        \o hide() or setVisible(false) was called.
    \endlist
*/


void QWidget::setVisible(bool visible)
{
    if (visible) { // show
        if (testAttribute(Qt::WA_WState_ExplicitShowHide) && !testAttribute(Qt::WA_WState_Hidden))
            return;

        Q_D(QWidget);

        // Designer uses a trick to make grabWidget work without showing
        if (!isWindow() && parentWidget() && parentWidget()->isVisible()
            && !parentWidget()->testAttribute(Qt::WA_WState_Created))
            parentWidget()->window()->d_func()->createRecursively();

        //we have to at least create toplevels before applyX11SpecificCommandLineArguments
        //but not children of non-visible parents
        QWidget *pw = parentWidget();
        if (!testAttribute(Qt::WA_WState_Created)
            && (isWindow() || pw->testAttribute(Qt::WA_WState_Created))) {
            create();
        }

#if defined(Q_WS_X11)
        if (windowType() == Qt::Window)
            QApplicationPrivate::applyX11SpecificCommandLineArguments(this);
#elif defined(Q_WS_QWS)
        if (windowType() == Qt::Window)
            QApplicationPrivate::applyQWSSpecificCommandLineArguments(this);
#endif

        bool wasResized = testAttribute(Qt::WA_Resized);
        Qt::WindowStates initialWindowState = windowState();

        // polish if necessary
        ensurePolished();

        // remember that show was called explicitly
        setAttribute(Qt::WA_WState_ExplicitShowHide);
        // whether we need to inform the parent widget immediately
        bool needUpdateGeometry = !isWindow() && testAttribute(Qt::WA_WState_Hidden);
        // we are no longer hidden
        setAttribute(Qt::WA_WState_Hidden, false);

        if (needUpdateGeometry)
            d->updateGeometry_helper(true);

#ifdef QT3_SUPPORT
        QApplication::sendPostedEvents(this, QEvent::ChildInserted);
#endif
        // activate our layout before we and our children become visible
        if (d->layout)
            d->layout->activate();

        if (!isWindow()) {
            QWidget *parent = parentWidget();
            while (parent && parent->isVisible() && parent->d_func()->layout  && !parent->data->in_show) {
                parent->d_func()->layout->activate();
                if (parent->isWindow())
                    break;
                parent = parent->parentWidget();
            }
            if (parent)
                parent->d_func()->setDirtyOpaqueRegion();
        }

        // adjust size if necessary
        if (!wasResized
            && (isWindow() || !parentWidget()->d_func()->layout))  {
            if (isWindow()) {
                adjustSize();
                if (windowState() != initialWindowState)
                    setWindowState(initialWindowState);
            } else {
                adjustSize();
            }
            setAttribute(Qt::WA_Resized, false);
        }

        setAttribute(Qt::WA_KeyboardFocusChange, false);

        if (isWindow() || parentWidget()->isVisible()) {
            // remove posted quit events when showing a new window
            QCoreApplication::removePostedEvents(qApp, QEvent::Quit);

            d->show_helper();

#if defined(Q_WS_WIN) || defined(Q_WS_X11) || defined (Q_WS_QWS) || defined(Q_WS_MAC) || defined(Q_WS_QPA)
            qApp->d_func()->sendSyntheticEnterLeave(this);
#endif
        }

        QEvent showToParentEvent(QEvent::ShowToParent);
        QApplication::sendEvent(this, &showToParentEvent);
    } else { // hide
        if (testAttribute(Qt::WA_WState_ExplicitShowHide) && testAttribute(Qt::WA_WState_Hidden))
            return;
#if defined(Q_WS_WIN)
        // reset WS_DISABLED style in a Blocked window
        if(isWindow() && testAttribute(Qt::WA_WState_Created)
           && QApplicationPrivate::isBlockedByModal(this))
        {
            LONG dwStyle = GetWindowLong(winId(), GWL_STYLE);
            dwStyle &= ~WS_DISABLED;
            SetWindowLong(winId(), GWL_STYLE, dwStyle);
        }
#endif
        if (QApplicationPrivate::hidden_focus_widget == this)
            QApplicationPrivate::hidden_focus_widget = 0;

        Q_D(QWidget);

        // hw: The test on getOpaqueRegion() needs to be more intelligent
        // currently it doesn't work if the widget is hidden (the region will
        // be clipped). The real check should be testing the cached region
        // (and dirty flag) directly.
        if (!isWindow() && parentWidget()) // && !d->getOpaqueRegion().isEmpty())
            parentWidget()->d_func()->setDirtyOpaqueRegion();

        setAttribute(Qt::WA_WState_Hidden);
        setAttribute(Qt::WA_WState_ExplicitShowHide);
        if (testAttribute(Qt::WA_WState_Created))
            d->hide_helper();

        // invalidate layout similar to updateGeometry()
        if (!isWindow() && parentWidget()) {
            if (parentWidget()->d_func()->layout)
                parentWidget()->d_func()->layout->invalidate();
            else if (parentWidget()->isVisible())
                QApplication::postEvent(parentWidget(), new QEvent(QEvent::LayoutRequest));
        }

        QEvent hideToParentEvent(QEvent::HideToParent);
        QApplication::sendEvent(this, &hideToParentEvent);
    }
}

/*!\fn void QWidget::setHidden(bool hidden)

    Convenience function, equivalent to setVisible(!\a hidden).
*/

/*!\fn void QWidget::setShown(bool shown)

    Use setVisible(\a shown) instead.
*/


void QWidgetPrivate::_q_showIfNotHidden()
{
    Q_Q(QWidget);
    if ( !(q->isHidden() && q->testAttribute(Qt::WA_WState_ExplicitShowHide)) )
        q->setVisible(true);
}

void QWidgetPrivate::showChildren(bool spontaneous)
{
    QList<QObject*> childList = children;
    for (int i = 0; i < childList.size(); ++i) {
        QWidget *widget = qobject_cast<QWidget*>(childList.at(i));
        if (!widget
            || widget->isWindow()
            || widget->testAttribute(Qt::WA_WState_Hidden))
            continue;
        if (spontaneous) {
            widget->setAttribute(Qt::WA_Mapped);
            widget->d_func()->showChildren(true);
            QShowEvent e;
            QApplication::sendSpontaneousEvent(widget, &e);
        } else {
            if (widget->testAttribute(Qt::WA_WState_ExplicitShowHide))
                widget->d_func()->show_recursive();
            else
                widget->show();
        }
    }
}

void QWidgetPrivate::hideChildren(bool spontaneous)
{
    QList<QObject*> childList = children;
    for (int i = 0; i < childList.size(); ++i) {
        QWidget *widget = qobject_cast<QWidget*>(childList.at(i));
        if (!widget || widget->isWindow() || widget->testAttribute(Qt::WA_WState_Hidden))
            continue;
#ifdef QT_MAC_USE_COCOA
        // Before doing anything we need to make sure that we don't leave anything in a non-consistent state.
        // When hiding a widget we need to make sure that no mouse_down events are active, because
        // the mouse_up event will never be received by a hidden widget or one of its descendants.
        // The solution is simple, before going through with this we check if there are any mouse_down events in
        // progress, if so we check if it is related to this widget or not. If so, we just reset the mouse_down and
        // then we continue.
        // In X11 and Windows we send a mouse_release event, however we don't do that here because we were already
        // ignoring that from before. I.e. Carbon did not send the mouse release event, so we will not send the
        // mouse release event. There are two ways to interpret this:
        // 1. If we don't send the mouse release event, the widget might get into an inconsistent state, i.e. it
        // might be waiting for a release event that will never arrive.
        // 2. If we send the mouse release event, then the widget might decide to trigger an action that is not
        // supposed to trigger because it is not visible.
        if(widget == qt_button_down)
            qt_button_down = 0;
#endif // QT_MAC_USE_COCOA
        if (spontaneous)
            widget->setAttribute(Qt::WA_Mapped, false);
        else
            widget->setAttribute(Qt::WA_WState_Visible, false);
        widget->d_func()->hideChildren(spontaneous);
        QHideEvent e;
        if (spontaneous) {
            QApplication::sendSpontaneousEvent(widget, &e);
        } else {
            QApplication::sendEvent(widget, &e);
            if (widget->internalWinId()
                && widget->testAttribute(Qt::WA_DontCreateNativeAncestors)) {
                // hide_sys() on an ancestor won't have any affect on this
                // widget, so it needs an explicit hide_sys() of its own
                widget->d_func()->hide_sys();
            }
        }
#if defined(Q_WS_WIN) || defined(Q_WS_X11) || defined (Q_WS_QWS) || defined(Q_WS_MAC) || defined(Q_WS_QPA)
        qApp->d_func()->sendSyntheticEnterLeave(widget);
#endif
#ifndef QT_NO_ACCESSIBILITY
        if (!spontaneous)
            QAccessible::updateAccessibility(widget, 0, QAccessible::ObjectHide);
#endif
    }
}

bool QWidgetPrivate::close_helper(CloseMode mode)
{
    if (data.is_closing)
        return true;

    Q_Q(QWidget);
    data.is_closing = 1;

    QPointer<QWidget> that = q;
    QPointer<QWidget> parentWidget = q->parentWidget();

#ifdef QT3_SUPPORT
    bool isMain = (QApplicationPrivate::main_widget == q);
#endif
    bool quitOnClose = q->testAttribute(Qt::WA_QuitOnClose);
    if (mode != CloseNoEvent) {
        QCloseEvent e;
        if (mode == CloseWithSpontaneousEvent)
            QApplication::sendSpontaneousEvent(q, &e);
        else
            QApplication::sendEvent(q, &e);
        if (!that.isNull() && !e.isAccepted()) {
            data.is_closing = 0;
            return false;
        }
    }

    if (!that.isNull() && !q->isHidden())
        q->hide();

#ifdef QT3_SUPPORT
    if (isMain)
        QApplication::quit();
#endif
    // Attempt to close the application only if this has WA_QuitOnClose set and a non-visible parent
    quitOnClose = quitOnClose && (parentWidget.isNull() || !parentWidget->isVisible());

    if (quitOnClose) {
        /* if there is no non-withdrawn primary window left (except
           the ones without QuitOnClose), we emit the lastWindowClosed
           signal */
        QWidgetList list = QApplication::topLevelWidgets();
        bool lastWindowClosed = true;
        for (int i = 0; i < list.size(); ++i) {
            QWidget *w = list.at(i);
            if (!w->isVisible() || w->parentWidget() || !w->testAttribute(Qt::WA_QuitOnClose))
                continue;
            lastWindowClosed = false;
            break;
        }
        if (lastWindowClosed)
            QApplicationPrivate::emitLastWindowClosed();
    }

    if (!that.isNull()) {
        data.is_closing = 0;
        if (q->testAttribute(Qt::WA_DeleteOnClose)) {
            q->setAttribute(Qt::WA_DeleteOnClose, false);
            q->deleteLater();
        }
    }
    return true;
}


/*!
    Closes this widget. Returns true if the widget was closed;
    otherwise returns false.

    First it sends the widget a QCloseEvent. The widget is \link
    hide() hidden\endlink if it \link QCloseEvent::accept()
    accepts\endlink the close event. If it \link QCloseEvent::ignore()
    ignores\endlink the event, nothing happens. The default
    implementation of QWidget::closeEvent() accepts the close event.

    If the widget has the Qt::WA_DeleteOnClose flag, the widget
    is also deleted. A close events is delivered to the widget no
    matter if the widget is visible or not.

    The \l QApplication::lastWindowClosed() signal is emitted when the
    last visible primary window (i.e. window with no parent) with the
    Qt::WA_QuitOnClose attribute set is closed. By default this
    attribute is set for all widgets except transient windows such as
    splash screens, tool windows, and popup menus.

*/

bool QWidget::close()
{
    return d_func()->close_helper(QWidgetPrivate::CloseWithEvent);
}

/*!
    \property QWidget::visible
    \brief whether the widget is visible

    Calling setVisible(true) or show() sets the widget to visible
    status if all its parent widgets up to the window are visible. If
    an ancestor is not visible, the widget won't become visible until
    all its ancestors are shown. If its size or position has changed,
    Qt guarantees that a widget gets move and resize events just
    before it is shown. If the widget has not been resized yet, Qt
    will adjust the widget's size to a useful default using
    adjustSize().

    Calling setVisible(false) or hide() hides a widget explicitly. An
    explicitly hidden widget will never become visible, even if all
    its ancestors become visible, unless you show it.

    A widget receives show and hide events when its visibility status
    changes. Between a hide and a show event, there is no need to
    waste CPU cycles preparing or displaying information to the user.
    A video application, for example, might simply stop generating new
    frames.

    A widget that happens to be obscured by other windows on the
    screen is considered to be visible. The same applies to iconified
    windows and windows that exist on another virtual
    desktop (on platforms that support this concept). A widget
    receives spontaneous show and hide events when its mapping status
    is changed by the window system, e.g. a spontaneous hide event
    when the user minimizes the window, and a spontaneous show event
    when the window is restored again.

    You almost never have to reimplement the setVisible() function. If
    you need to change some settings before a widget is shown, use
    showEvent() instead. If you need to do some delayed initialization
    use the Polish event delivered to the event() function.

    \sa show(), hide(), isHidden(), isVisibleTo(), isMinimized(),
    showEvent(), hideEvent()
*/


/*!
    Returns true if this widget would become visible if \a ancestor is
    shown; otherwise returns false.

    The true case occurs if neither the widget itself nor any parent
    up to but excluding \a ancestor has been explicitly hidden.

    This function will still return true if the widget is obscured by
    other windows on the screen, but could be physically visible if it
    or they were to be moved.

    isVisibleTo(0) is identical to isVisible().

    \sa show() hide() isVisible()
*/

bool QWidget::isVisibleTo(QWidget* ancestor) const
{
    if (!ancestor)
        return isVisible();
    const QWidget * w = this;
    while (!w->isHidden()
            && !w->isWindow()
            && w->parentWidget()
            && w->parentWidget() != ancestor)
        w = w->parentWidget();
    return !w->isHidden();
}

#ifdef QT3_SUPPORT
/*!
    Use visibleRegion() instead.
*/
QRect QWidget::visibleRect() const
{
    return d_func()->clipRect();
}
#endif

/*!
    Returns the unobscured region where paint events can occur.

    For visible widgets, this is an approximation of the area not
    covered by other widgets; otherwise, this is an empty region.

    The repaint() function calls this function if necessary, so in
    general you do not need to call it.

*/
QRegion QWidget::visibleRegion() const
{
    Q_D(const QWidget);

    QRect clipRect = d->clipRect();
    if (clipRect.isEmpty())
        return QRegion();
    QRegion r(clipRect);
    d->subtractOpaqueChildren(r, clipRect);
    d->subtractOpaqueSiblings(r);
#ifdef Q_WS_QWS
    const QWSWindowSurface *surface = static_cast<const QWSWindowSurface*>(windowSurface());
    if (surface) {
        const QPoint offset = mapTo(surface->window(), QPoint());
        r &= surface->clipRegion().translated(-offset);
    }
#endif
    return r;
}


QSize QWidgetPrivate::adjustedSize() const
{
    Q_Q(const QWidget);

    QSize s = q->sizeHint();

    if (q->isWindow()) {
        Qt::Orientations exp;
        if (layout) {
            if (layout->hasHeightForWidth())
                s.setHeight(layout->totalHeightForWidth(s.width()));
            exp = layout->expandingDirections();
        } else
        {
            if (q->sizePolicy().hasHeightForWidth())
                s.setHeight(q->heightForWidth(s.width()));
            exp = q->sizePolicy().expandingDirections();
        }
        if (exp & Qt::Horizontal)
            s.setWidth(qMax(s.width(), 200));
        if (exp & Qt::Vertical)
            s.setHeight(qMax(s.height(), 100));
#if defined(Q_WS_X11)
        QRect screen = QApplication::desktop()->screenGeometry(q->x11Info().screen());
#else // all others
        QRect screen = QApplication::desktop()->screenGeometry(q->pos());
#endif
#if defined (Q_WS_WINCE) || defined (Q_OS_SYMBIAN)
        s.setWidth(qMin(s.width(), screen.width()));
        s.setHeight(qMin(s.height(), screen.height()));
#else
        s.setWidth(qMin(s.width(), screen.width()*2/3));
        s.setHeight(qMin(s.height(), screen.height()*2/3));
#endif
        if (QTLWExtra *extra = maybeTopData())
            extra->sizeAdjusted = true;
    }

    if (!s.isValid()) {
        QRect r = q->childrenRect(); // get children rectangle
        if (r.isNull())
            return s;
        s = r.size() + QSize(2 * r.x(), 2 * r.y());
    }

    return s;
}

/*!
    Adjusts the size of the widget to fit its contents.

    This function uses sizeHint() if it is valid, i.e., the size hint's width
    and height are \>= 0. Otherwise, it sets the size to the children
    rectangle that covers all child widgets (the union of all child widget
    rectangles).

    For windows, the screen size is also taken into account. If the sizeHint()
    is less than (200, 100) and the size policy is \l{QSizePolicy::Expanding}
    {expanding}, the window will be at least (200, 100). The maximum size of
    a window is 2/3 of the screen's width and height.

    \sa sizeHint(), childrenRect()
*/

void QWidget::adjustSize()
{
    Q_D(QWidget);
    ensurePolished();
    QSize s = d->adjustedSize();

    if (d->layout)
        d->layout->activate();

    if (s.isValid())
        resize(s);
}


/*!
    \property QWidget::sizeHint
    \brief the recommended size for the widget

    If the value of this property is an invalid size, no size is
    recommended.

    The default implementation of sizeHint() returns an invalid size
    if there is no layout for this widget, and returns the layout's
    preferred size otherwise.

    \sa QSize::isValid(), minimumSizeHint(), sizePolicy(),
    setMinimumSize(), updateGeometry()
*/

QSize QWidget::sizeHint() const
{
    Q_D(const QWidget);
    if (d->layout)
        return d->layout->totalSizeHint();
    return QSize(-1, -1);
}

/*!
    \property QWidget::minimumSizeHint
    \brief the recommended minimum size for the widget

    If the value of this property is an invalid size, no minimum size
    is recommended.

    The default implementation of minimumSizeHint() returns an invalid
    size if there is no layout for this widget, and returns the
    layout's minimum size otherwise. Most built-in widgets reimplement
    minimumSizeHint().

    \l QLayout will never resize a widget to a size smaller than the
    minimum size hint unless minimumSize() is set or the size policy is
    set to QSizePolicy::Ignore. If minimumSize() is set, the minimum
    size hint will be ignored.

    \sa QSize::isValid(), resize(), setMinimumSize(), sizePolicy()
*/
QSize QWidget::minimumSizeHint() const
{
    Q_D(const QWidget);
    if (d->layout)
        return d->layout->totalMinimumSize();
    return QSize(-1, -1);
}


/*!
    \fn QWidget *QWidget::parentWidget() const

    Returns the parent of this widget, or 0 if it does not have any
    parent widget.
*/


/*!
    Returns true if this widget is a parent, (or grandparent and so on
    to any level), of the given \a child, and both widgets are within
    the same window; otherwise returns false.
*/

bool QWidget::isAncestorOf(const QWidget *child) const
{
    while (child) {
        if (child == this)
            return true;
        if (child->isWindow())
            return false;
        child = child->parentWidget();
    }
    return false;
}

#if defined(Q_WS_WIN)
inline void setDisabledStyle(QWidget *w, bool setStyle)
{
    // set/reset WS_DISABLED style.
    if(w && w->isWindow() && w->isVisible() && w->isEnabled()) {
        LONG dwStyle = GetWindowLong(w->winId(), GWL_STYLE);
        LONG newStyle = dwStyle;
        if (setStyle)
            newStyle |= WS_DISABLED;
        else
            newStyle &= ~WS_DISABLED;
        if (newStyle != dwStyle) {
            SetWindowLong(w->winId(), GWL_STYLE, newStyle);
            // we might need to repaint in some situations (eg. menu)
            w->repaint();
        }
    }
}
#endif

/*****************************************************************************
  QWidget event handling
 *****************************************************************************/

/*!
    This is the main event handler; it handles event \a event. You can
    reimplement this function in a subclass, but we recommend using
    one of the specialized event handlers instead.

    Key press and release events are treated differently from other
    events. event() checks for Tab and Shift+Tab and tries to move the
    focus appropriately. If there is no widget to move the focus to
    (or the key press is not Tab or Shift+Tab), event() calls
    keyPressEvent().

    Mouse and tablet event handling is also slightly special: only
    when the widget is \l enabled, event() will call the specialized
    handlers such as mousePressEvent(); otherwise it will discard the
    event.

    This function returns true if the event was recognized, otherwise
    it returns false.  If the recognized event was accepted (see \l
    QEvent::accepted), any further processing such as event
    propagation to the parent widget stops.

    \sa closeEvent(), focusInEvent(), focusOutEvent(), enterEvent(),
    keyPressEvent(), keyReleaseEvent(), leaveEvent(),
    mouseDoubleClickEvent(), mouseMoveEvent(), mousePressEvent(),
    mouseReleaseEvent(), moveEvent(), paintEvent(), resizeEvent(),
    QObject::event(), QObject::timerEvent()
*/

bool QWidget::event(QEvent *event)
{
    Q_D(QWidget);

    // ignore mouse events when disabled
    if (!isEnabled()) {
        switch(event->type()) {
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        case QEvent::TabletMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        case QEvent::ContextMenu:
#ifndef QT_NO_WHEELEVENT
        case QEvent::Wheel:
#endif
            return false;
        default:
            break;
        }
    }
    switch (event->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonPress:
        // Don't reset input context here. Whether reset or not is
        // a responsibility of input method. reset() will be
        // called by mouseHandler() of input method if necessary
        // via mousePressEvent() of text widgets.
#if 0
        resetInputContext();
#endif
        mousePressEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent((QMouseEvent*)event);
        break;

    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent((QMouseEvent*)event);
        break;
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        wheelEvent((QWheelEvent*)event);
        break;
#endif
#ifndef QT_NO_TABLETEVENT
    case QEvent::TabletMove:
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
        tabletEvent((QTabletEvent*)event);
        break;
#endif
#ifdef QT3_SUPPORT
    case QEvent::Accel:
        event->ignore();
        return false;
#endif
    case QEvent::KeyPress: {
        QKeyEvent *k = (QKeyEvent *)event;
        bool res = false;
        if (!(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {  //### Add MetaModifier?
            if (k->key() == Qt::Key_Backtab
                || (k->key() == Qt::Key_Tab && (k->modifiers() & Qt::ShiftModifier)))
                res = focusNextPrevChild(false);
            else if (k->key() == Qt::Key_Tab)
                res = focusNextPrevChild(true);
            if (res)
                break;
        }
        keyPressEvent(k);
#ifdef QT_KEYPAD_NAVIGATION
        if (!k->isAccepted() && QApplication::keypadNavigationEnabled()
            && !(k->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier))) {
            if (QApplication::navigationMode() == Qt::NavigationModeKeypadTabOrder) {
                if (k->key() == Qt::Key_Up)
                    res = focusNextPrevChild(false);
                else if (k->key() == Qt::Key_Down)
                    res = focusNextPrevChild(true);
            } else if (QApplication::navigationMode() == Qt::NavigationModeKeypadDirectional) {
                if (k->key() == Qt::Key_Up)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionNorth);
                else if (k->key() == Qt::Key_Right)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionEast);
                else if (k->key() == Qt::Key_Down)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionSouth);
                else if (k->key() == Qt::Key_Left)
                    res = QWidgetPrivate::navigateToDirection(QWidgetPrivate::DirectionWest);
            }
            if (res) {
                k->accept();
                break;
            }
        }
#endif
#ifndef QT_NO_WHATSTHIS
        if (!k->isAccepted()
            && k->modifiers() & Qt::ShiftModifier && k->key() == Qt::Key_F1
            && d->whatsThis.size()) {
            QWhatsThis::showText(mapToGlobal(inputMethodQuery(Qt::ImMicroFocus).toRect().center()), d->whatsThis, this);
            k->accept();
        }
#endif
    }
        break;

    case QEvent::KeyRelease:
        keyReleaseEvent((QKeyEvent*)event);
        // fall through
    case QEvent::ShortcutOverride:
        break;

    case QEvent::InputMethod:
        inputMethodEvent((QInputMethodEvent *) event);
        break;

    case QEvent::PolishRequest:
        ensurePolished();
        break;

    case QEvent::Polish: {
        style()->polish(this);
        setAttribute(Qt::WA_WState_Polished);
        if (!QApplication::font(this).isCopyOf(QApplication::font()))
            d->resolveFont();
        if (!QApplication::palette(this).isCopyOf(QApplication::palette()))
            d->resolvePalette();
#ifdef QT3_SUPPORT
        if(d->sendChildEvents)
            QApplication::sendPostedEvents(this, QEvent::ChildInserted);
#endif
    }
        break;

    case QEvent::ApplicationWindowIconChange:
        if (isWindow() && !testAttribute(Qt::WA_SetWindowIcon)) {
            d->setWindowIcon_sys();
            d->setWindowIcon_helper();
        }
        break;
    case QEvent::FocusIn:
#ifdef QT_SOFTKEYS_ENABLED
        QSoftKeyManager::updateSoftKeys();
#endif
        focusInEvent((QFocusEvent*)event);
        break;

    case QEvent::FocusOut:
        focusOutEvent((QFocusEvent*)event);
        break;

    case QEvent::Enter:
#ifndef QT_NO_STATUSTIP
        if (d->statusTip.size()) {
            QStatusTipEvent tip(d->statusTip);
            QApplication::sendEvent(const_cast<QWidget *>(this), &tip);
        }
#endif
        enterEvent(event);
        break;

    case QEvent::Leave:
#ifndef QT_NO_STATUSTIP
        if (d->statusTip.size()) {
            QString empty;
            QStatusTipEvent tip(empty);
            QApplication::sendEvent(const_cast<QWidget *>(this), &tip);
        }
#endif
        leaveEvent(event);
        break;

    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
        update();
        break;

    case QEvent::Paint:
        // At this point the event has to be delivered, regardless
        // whether the widget isVisible() or not because it
        // already went through the filters
        paintEvent((QPaintEvent*)event);
        break;

    case QEvent::Move:
        moveEvent((QMoveEvent*)event);
        break;

    case QEvent::Resize:
        resizeEvent((QResizeEvent*)event);
        break;

    case QEvent::Close:
        closeEvent((QCloseEvent *)event);
        break;

#ifndef QT_NO_CONTEXTMENU
    case QEvent::ContextMenu:
        switch (data->context_menu_policy) {
        case Qt::PreventContextMenu:
            break;
        case Qt::DefaultContextMenu:
            contextMenuEvent(static_cast<QContextMenuEvent *>(event));
            break;
        case Qt::CustomContextMenu:
            emit customContextMenuRequested(static_cast<QContextMenuEvent *>(event)->pos());
            break;
#ifndef QT_NO_MENU
        case Qt::ActionsContextMenu:
            if (d->actions.count()) {
                QMenu::exec(d->actions, static_cast<QContextMenuEvent *>(event)->globalPos(),
                            0, this);
                break;
            }
            // fall through
#endif
        default:
            event->ignore();
            break;
        }
        break;
#endif // QT_NO_CONTEXTMENU

#ifndef QT_NO_DRAGANDDROP
    case QEvent::Drop:
        dropEvent((QDropEvent*) event);
        break;

    case QEvent::DragEnter:
        dragEnterEvent((QDragEnterEvent*) event);
        break;

    case QEvent::DragMove:
        dragMoveEvent((QDragMoveEvent*) event);
        break;

    case QEvent::DragLeave:
        dragLeaveEvent((QDragLeaveEvent*) event);
        break;
#endif

    case QEvent::Show:
        showEvent((QShowEvent*) event);
        break;

    case QEvent::Hide:
        hideEvent((QHideEvent*) event);
        break;

    case QEvent::ShowWindowRequest:
        if (!isHidden())
            d->show_sys();
        break;

    case QEvent::ApplicationFontChange:
        d->resolveFont();
        break;
    case QEvent::ApplicationPaletteChange:
        if (!(windowType() == Qt::Desktop))
            d->resolvePalette();
        break;

    case QEvent::ToolBarChange:
    case QEvent::ActivationChange:
    case QEvent::EnabledChange:
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::PaletteChange:
    case QEvent::WindowTitleChange:
    case QEvent::IconTextChange:
    case QEvent::ModifiedChange:
    case QEvent::MouseTrackingChange:
    case QEvent::ParentChange:
    case QEvent::WindowStateChange:
    case QEvent::LocaleChange:
    case QEvent::MacSizeChange:
    case QEvent::ContentsRectChange:
        changeEvent(event);
        break;

    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate: {
#ifdef QT3_SUPPORT
        windowActivationChange(event->type() != QEvent::WindowActivate);
#endif
        if (isVisible() && !palette().isEqual(QPalette::Active, QPalette::Inactive))
            update();
        QList<QObject*> childList = d->children;
        for (int i = 0; i < childList.size(); ++i) {
            QWidget *w = qobject_cast<QWidget *>(childList.at(i));
            if (w && w->isVisible() && !w->isWindow())
                QApplication::sendEvent(w, event);
        }

#ifdef QT_SOFTKEYS_ENABLED
        if (isWindow())
            QSoftKeyManager::updateSoftKeys();
#endif

        break; }

    case QEvent::LanguageChange:
#ifdef QT3_SUPPORT
        languageChange();
#endif
        changeEvent(event);
        {
            QList<QObject*> childList = d->children;
            for (int i = 0; i < childList.size(); ++i) {
                QObject *o = childList.at(i);
                if (o)
                    QApplication::sendEvent(o, event);
            }
        }
        update();
        break;

    case QEvent::ApplicationLayoutDirectionChange:
        d->resolveLayoutDirection();
        break;

    case QEvent::LayoutDirectionChange:
        if (d->layout)
            d->layout->invalidate();
        update();
        changeEvent(event);
        break;
    case QEvent::UpdateRequest:
        d->syncBackingStore();
        break;
    case QEvent::UpdateLater:
        update(static_cast<QUpdateLaterEvent*>(event)->region());
        break;

    case QEvent::WindowBlocked:
    case QEvent::WindowUnblocked:
        {
            QList<QObject*> childList = d->children;
            for (int i = 0; i < childList.size(); ++i) {
                QObject *o = childList.at(i);
                if (o && o != QApplication::activeModalWidget()) {
                    if (qobject_cast<QWidget *>(o) && static_cast<QWidget *>(o)->isWindow()) {
                        // do not forward the event to child windows,
                        // QApplication does this for us
                        continue;
                    }
                    QApplication::sendEvent(o, event);
                }
            }
#if defined(Q_WS_WIN)
            setDisabledStyle(this, (event->type() == QEvent::WindowBlocked));
#endif
        }
        break;
#ifndef QT_NO_TOOLTIP
    case QEvent::ToolTip:
        if (!d->toolTip.isEmpty())
            QToolTip::showText(static_cast<QHelpEvent*>(event)->globalPos(), d->toolTip, this);
        else
            event->ignore();
        break;
#endif
#ifndef QT_NO_WHATSTHIS
    case QEvent::WhatsThis:
        if (d->whatsThis.size())
            QWhatsThis::showText(static_cast<QHelpEvent *>(event)->globalPos(), d->whatsThis, this);
        else
            event->ignore();
        break;
    case QEvent::QueryWhatsThis:
        if (d->whatsThis.isEmpty())
            event->ignore();
        break;
#endif
#ifndef QT_NO_ACCESSIBILITY
    case QEvent::AccessibilityDescription:
    case QEvent::AccessibilityHelp: {
        QAccessibleEvent *ev = static_cast<QAccessibleEvent *>(event);
        if (ev->child())
            return false;
        switch (ev->type()) {
#ifndef QT_NO_TOOLTIP
        case QEvent::AccessibilityDescription:
            ev->setValue(d->toolTip);
            break;
#endif
#ifndef QT_NO_WHATSTHIS
        case QEvent::AccessibilityHelp:
            ev->setValue(d->whatsThis);
            break;
#endif
        default:
            return false;
        }
        break; }
#endif
    case QEvent::EmbeddingControl:
        d->topData()->frameStrut.setCoords(0 ,0, 0, 0);
        data->fstrut_dirty = false;
#if defined(Q_WS_WIN) || defined(Q_WS_X11)
        d->topData()->embedded = 1;
#endif
        break;
#ifndef QT_NO_ACTION
    case QEvent::ActionAdded:
    case QEvent::ActionRemoved:
    case QEvent::ActionChanged:
#ifdef QT_SOFTKEYS_ENABLED
        QSoftKeyManager::updateSoftKeys();
#endif
        actionEvent((QActionEvent*)event);
        break;
#endif

    case QEvent::KeyboardLayoutChange:
        {
            changeEvent(event);

            // inform children of the change
            QList<QObject*> childList = d->children;
            for (int i = 0; i < childList.size(); ++i) {
                QWidget *w = qobject_cast<QWidget *>(childList.at(i));
                if (w && w->isVisible() && !w->isWindow())
                    QApplication::sendEvent(w, event);
            }
            break;
        }
#ifdef Q_WS_MAC
    case QEvent::MacGLWindowChange:
        d->needWindowChange = false;
        break;
#endif
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    {
#ifndef Q_WS_MAC
        QTouchEvent *touchEvent = static_cast<QTouchEvent *>(event);
        const QTouchEvent::TouchPoint &touchPoint = touchEvent->touchPoints().first();
        if (touchPoint.isPrimary() || touchEvent->deviceType() == QTouchEvent::TouchPad)
            break;

        // fake a mouse event!
        QEvent::Type eventType = QEvent::None;
        switch (touchEvent->type()) {
        case QEvent::TouchBegin:
            eventType = QEvent::MouseButtonPress;
            break;
        case QEvent::TouchUpdate:
            eventType = QEvent::MouseMove;
            break;
        case QEvent::TouchEnd:
            eventType = QEvent::MouseButtonRelease;
            break;
        default:
            Q_ASSERT(!true);
            break;
        }
        if (eventType == QEvent::None)
            break;

        QMouseEvent mouseEvent(eventType,
                               touchPoint.pos().toPoint(),
                               touchPoint.screenPos().toPoint(),
                               Qt::LeftButton,
                               Qt::LeftButton,
                               touchEvent->modifiers());
        (void) QApplication::sendEvent(this, &mouseEvent);
#endif // Q_WS_MAC
        break;
    }
#ifndef QT_NO_GESTURES
    case QEvent::Gesture:
        event->ignore();
        break;
#endif
#ifndef QT_NO_PROPERTIES
    case QEvent::DynamicPropertyChange: {
        const QByteArray &propName = static_cast<QDynamicPropertyChangeEvent *>(event)->propertyName();
        if (!qstrncmp(propName, "_q_customDpi", 12) && propName.length() == 13) {
            uint value = property(propName.constData()).toUInt();
            if (!d->extra)
                d->createExtra();
            const char axis = propName.at(12);
            if (axis == 'X')
                d->extra->customDpiX = value;
            else if (axis == 'Y')
                d->extra->customDpiY = value;
            d->updateFont(d->data.fnt);
        }
        // fall through
    }
#endif
    default:
        return QObject::event(event);
    }
    return true;
}

/*!
  This event handler can be reimplemented to handle state changes.

  The state being changed in this event can be retrieved through the \a event
  supplied.

  Change events include: QEvent::ToolBarChange,
  QEvent::ActivationChange, QEvent::EnabledChange, QEvent::FontChange,
  QEvent::StyleChange, QEvent::PaletteChange,
  QEvent::WindowTitleChange, QEvent::IconTextChange,
  QEvent::ModifiedChange, QEvent::MouseTrackingChange,
  QEvent::ParentChange, QEvent::WindowStateChange,
  QEvent::LanguageChange, QEvent::LocaleChange,
  QEvent::LayoutDirectionChange.

*/
void QWidget::changeEvent(QEvent * event)
{
    switch(event->type()) {
    case QEvent::EnabledChange:
        update();
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::updateAccessibility(this, 0, QAccessible::StateChanged);
#endif
        break;

    case QEvent::FontChange:
    case QEvent::StyleChange: {
        Q_D(QWidget);
        update();
        updateGeometry();
        if (d->layout)
            d->layout->invalidate();
#ifdef Q_WS_QWS
        if (isWindow())
            d->data.fstrut_dirty = true;
#endif
        break;
    }

    case QEvent::PaletteChange:
        update();
        break;

#ifdef Q_WS_MAC
    case QEvent::MacSizeChange:
        updateGeometry();
        break;
    case QEvent::ToolTipChange:
    case QEvent::MouseTrackingChange:
        qt_mac_update_mouseTracking(this);
        break;
#endif

    default:
        break;
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive mouse move events for the widget.

    If mouse tracking is switched off, mouse move events only occur if
    a mouse button is pressed while the mouse is being moved. If mouse
    tracking is switched on, mouse move events occur even if no mouse
    button is pressed.

    QMouseEvent::pos() reports the position of the mouse cursor,
    relative to this widget. For press and release events, the
    position is usually the same as the position of the last mouse
    move event, but it might be different if the user's hand shakes.
    This is a feature of the underlying window system, not Qt.

    If you want to show a tooltip immediately, while the mouse is
    moving (e.g., to get the mouse coordinates with QMouseEvent::pos()
    and show them as a tooltip), you must first enable mouse tracking
    as described above. Then, to ensure that the tooltip is updated
    immediately, you must call QToolTip::showText() instead of
    setToolTip() in your implementation of mouseMoveEvent().

    \sa setMouseTracking(), mousePressEvent(), mouseReleaseEvent(),
    mouseDoubleClickEvent(), event(), QMouseEvent, {Scribble Example}
*/

void QWidget::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive mouse press events for the widget.

    If you create new widgets in the mousePressEvent() the
    mouseReleaseEvent() may not end up where you expect, depending on
    the underlying window system (or X11 window manager), the widgets'
    location and maybe more.

    The default implementation implements the closing of popup widgets
    when you click outside the window. For other widget types it does
    nothing.

    \sa mouseReleaseEvent(), mouseDoubleClickEvent(),
    mouseMoveEvent(), event(), QMouseEvent, {Scribble Example}
*/

void QWidget::mousePressEvent(QMouseEvent *event)
{
    event->ignore();
    if ((windowType() == Qt::Popup)) {
        event->accept();
        QWidget* w;
        while ((w = QApplication::activePopupWidget()) && w != this){
            w->close();
            if (QApplication::activePopupWidget() == w) // widget does not want to disappear
                w->hide(); // hide at least
        }
        if (!rect().contains(event->pos())){
            close();
        }
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive mouse release events for the widget.

    \sa mousePressEvent(), mouseDoubleClickEvent(),
    mouseMoveEvent(), event(), QMouseEvent, {Scribble Example}
*/

void QWidget::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive mouse double click events for the widget.

    The default implementation generates a normal mouse press event.

    \note The widget will also receive mouse press and mouse release
    events in addition to the double click event. It is up to the
    developer to ensure that the application interprets these events
    correctly.

    \sa mousePressEvent(), mouseReleaseEvent() mouseMoveEvent(),
    event(), QMouseEvent
*/

void QWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    mousePressEvent(event);                        // try mouse press event
}

#ifndef QT_NO_WHEELEVENT
/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive wheel events for the widget.

    If you reimplement this handler, it is very important that you
    \link QWheelEvent ignore()\endlink the event if you do not handle
    it, so that the widget's parent can interpret it.

    The default implementation ignores the event.

    \sa QWheelEvent::ignore(), QWheelEvent::accept(), event(),
    QWheelEvent
*/

void QWidget::wheelEvent(QWheelEvent *event)
{
    event->ignore();
}
#endif // QT_NO_WHEELEVENT

#ifndef QT_NO_TABLETEVENT
/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive tablet events for the widget.

    If you reimplement this handler, it is very important that you
    \link QTabletEvent ignore()\endlink the event if you do not handle
    it, so that the widget's parent can interpret it.

    The default implementation ignores the event.

    \sa QTabletEvent::ignore(), QTabletEvent::accept(), event(),
    QTabletEvent
*/

void QWidget::tabletEvent(QTabletEvent *event)
{
    event->ignore();
}
#endif // QT_NO_TABLETEVENT

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive key press events for the widget.

    A widget must call setFocusPolicy() to accept focus initially and
    have focus in order to receive a key press event.

    If you reimplement this handler, it is very important that you
    call the base class implementation if you do not act upon the key.

    The default implementation closes popup widgets if the user
    presses Esc. Otherwise the event is ignored, so that the widget's
    parent can interpret it.

    Note that QKeyEvent starts with isAccepted() == true, so you do not
    need to call QKeyEvent::accept() - just do not call the base class
    implementation if you act upon the key.

    \sa keyReleaseEvent(), setFocusPolicy(),
    focusInEvent(), focusOutEvent(), event(), QKeyEvent, {Tetrix Example}
*/

void QWidget::keyPressEvent(QKeyEvent *event)
{
    if ((windowType() == Qt::Popup) && event->key() == Qt::Key_Escape) {
        event->accept();
        close();
    } else {
        event->ignore();
    }
}

/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive key release events for the widget.

    A widget must \link setFocusPolicy() accept focus\endlink
    initially and \link hasFocus() have focus\endlink in order to
    receive a key release event.

    If you reimplement this handler, it is very important that you
    call the base class implementation if you do not act upon the key.

    The default implementation ignores the event, so that the widget's
    parent can interpret it.

    Note that QKeyEvent starts with isAccepted() == true, so you do not
    need to call QKeyEvent::accept() - just do not call the base class
    implementation if you act upon the key.

    \sa keyPressEvent(), QKeyEvent::ignore(), setFocusPolicy(),
    focusInEvent(), focusOutEvent(), event(), QKeyEvent
*/

void QWidget::keyReleaseEvent(QKeyEvent *event)
{
    event->ignore();
}

/*!
    \fn void QWidget::focusInEvent(QFocusEvent *event)

    This event handler can be reimplemented in a subclass to receive
    keyboard focus events (focus received) for the widget. The event
    is passed in the \a event parameter

    A widget normally must setFocusPolicy() to something other than
    Qt::NoFocus in order to receive focus events. (Note that the
    application programmer can call setFocus() on any widget, even
    those that do not normally accept focus.)

    The default implementation updates the widget (except for windows
    that do not specify a focusPolicy()).

    \sa focusOutEvent(), setFocusPolicy(), keyPressEvent(),
    keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusInEvent(QFocusEvent *)
{
    if (focusPolicy() != Qt::NoFocus || !isWindow()) {
        update();
    }
}

/*!
    \fn void QWidget::focusOutEvent(QFocusEvent *event)

    This event handler can be reimplemented in a subclass to receive
    keyboard focus events (focus lost) for the widget. The events is
    passed in the \a event parameter.

    A widget normally must setFocusPolicy() to something other than
    Qt::NoFocus in order to receive focus events. (Note that the
    application programmer can call setFocus() on any widget, even
    those that do not normally accept focus.)

    The default implementation updates the widget (except for windows
    that do not specify a focusPolicy()).

    \sa focusInEvent(), setFocusPolicy(), keyPressEvent(),
    keyReleaseEvent(), event(), QFocusEvent
*/

void QWidget::focusOutEvent(QFocusEvent *)
{
    if (focusPolicy() != Qt::NoFocus || !isWindow())
        update();
}

/*!
    \fn void QWidget::enterEvent(QEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget enter events which are passed in the \a event parameter.

    An event is sent to the widget when the mouse cursor enters the
    widget.

    \sa leaveEvent(), mouseMoveEvent(), event()
*/

void QWidget::enterEvent(QEvent *)
{
}

/*!
    \fn void QWidget::leaveEvent(QEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget leave events which are passed in the \a event parameter.

    A leave event is sent to the widget when the mouse cursor leaves
    the widget.

    \sa enterEvent(), mouseMoveEvent(), event()
*/

void QWidget::leaveEvent(QEvent *)
{
}

/*!
    \fn void QWidget::paintEvent(QPaintEvent *event)

    This event handler can be reimplemented in a subclass to receive paint
    events passed in \a event.

    A paint event is a request to repaint all or part of a widget. It can
    happen for one of the following reasons:

    \list
        \o repaint() or update() was invoked,
        \o the widget was obscured and has now been uncovered, or
        \o many other reasons.
    \endlist

    Many widgets can simply repaint their entire surface when asked to, but
    some slow widgets need to optimize by painting only the requested region:
    QPaintEvent::region(). This speed optimization does not change the result,
    as painting is clipped to that region during event processing. QListView
    and QTableView do this, for example.

    Qt also tries to speed up painting by merging multiple paint events into
    one. When update() is called several times or the window system sends
    several paint events, Qt merges these events into one event with a larger
    region (see QRegion::united()). The repaint() function does not permit this
    optimization, so we suggest using update() whenever possible.

    When the paint event occurs, the update region has normally been erased, so
    you are painting on the widget's background.

    The background can be set using setBackgroundRole() and setPalette().

    Since Qt 4.0, QWidget automatically double-buffers its painting, so there
    is no need to write double-buffering code in paintEvent() to avoid flicker.

    \bold{Note for the X11 platform}: It is possible to toggle global double
    buffering by calling \c qt_x11_set_global_double_buffer(). For example,

    \snippet doc/src/snippets/code/src_gui_kernel_qwidget.cpp 14

    \note Generally, you should refrain from calling update() or repaint()
    \bold{inside} a paintEvent(). For example, calling update() or repaint() on
    children inside a paintevent() results in undefined behavior; the child may
    or may not get a paint event.

    \warning If you are using a custom paint engine without Qt's backingstore,
    Qt::WA_PaintOnScreen must be set. Otherwise, QWidget::paintEngine() will
    never be called; the backingstore will be used instead.

    \sa event(), repaint(), update(), QPainter, QPixmap, QPaintEvent,
    {Analog Clock Example}
*/

void QWidget::paintEvent(QPaintEvent *)
{
}


/*!
    \fn void QWidget::moveEvent(QMoveEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget move events which are passed in the \a event parameter.
    When the widget receives this event, it is already at the new
    position.

    The old position is accessible through QMoveEvent::oldPos().

    \sa resizeEvent(), event(), move(), QMoveEvent
*/

void QWidget::moveEvent(QMoveEvent *)
{
}


/*!
    This event handler can be reimplemented in a subclass to receive
    widget resize events which are passed in the \a event parameter.
    When resizeEvent() is called, the widget already has its new
    geometry. The old size is accessible through
    QResizeEvent::oldSize().

    The widget will be erased and receive a paint event immediately
    after processing the resize event. No drawing need be (or should
    be) done inside this handler.


    \sa moveEvent(), event(), resize(), QResizeEvent, paintEvent(),
        {Scribble Example}
*/

void QWidget::resizeEvent(QResizeEvent * /* event */)
{
}

#ifndef QT_NO_ACTION
/*!
    \fn void QWidget::actionEvent(QActionEvent *event)

    This event handler is called with the given \a event whenever the
    widget's actions are changed.

    \sa addAction(), insertAction(), removeAction(), actions(), QActionEvent
*/
void QWidget::actionEvent(QActionEvent *)
{

}
#endif

/*!
    This event handler is called with the given \a event when Qt receives a window
    close request for a top-level widget from the window system.

    By default, the event is accepted and the widget is closed. You can reimplement
    this function to change the way the widget responds to window close requests.
    For example, you can prevent the window from closing by calling \l{QEvent::}{ignore()}
    on all events.

    Main window applications typically use reimplementations of this function to check
    whether the user's work has been saved and ask for permission before closing.
    For example, the \l{Application Example} uses a helper function to determine whether
    or not to close the window:

    \snippet mainwindows/application/mainwindow.cpp 3
    \snippet mainwindows/application/mainwindow.cpp 4

    \sa event(), hide(), close(), QCloseEvent, {Application Example}
*/

void QWidget::closeEvent(QCloseEvent *event)
{
    event->accept();
}

#ifndef QT_NO_CONTEXTMENU
/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive widget context menu events.

    The handler is called when the widget's \l contextMenuPolicy is
    Qt::DefaultContextMenu.

    The default implementation ignores the context event.
    See the \l QContextMenuEvent documentation for more details.

    \sa event(), QContextMenuEvent customContextMenuRequested()
*/

void QWidget::contextMenuEvent(QContextMenuEvent *event)
{
    event->ignore();
}
#endif // QT_NO_CONTEXTMENU


/*!
    This event handler, for event \a event, can be reimplemented in a
    subclass to receive Input Method composition events. This handler
    is called when the state of the input method changes.

    Note that when creating custom text editing widgets, the
    Qt::WA_InputMethodEnabled window attribute must be set explicitly
    (using the setAttribute() function) in order to receive input
    method events.

    The default implementation calls event->ignore(), which rejects the
    Input Method event. See the \l QInputMethodEvent documentation for more
    details.

    \sa event(), QInputMethodEvent
*/
void QWidget::inputMethodEvent(QInputMethodEvent *event)
{
    event->ignore();
}

/*!
    This method is only relevant for input widgets. It is used by the
    input method to query a set of properties of the widget to be
    able to support complex input method operations as support for
    surrounding text and reconversions.

    \a query specifies which property is queried.

    \sa inputMethodEvent(), QInputMethodEvent, QInputContext, inputMethodHints
*/
QVariant QWidget::inputMethodQuery(Qt::InputMethodQuery query) const
{
    switch(query) {
    case Qt::ImMicroFocus:
        return QRect(width()/2, 0, 1, height());
    case Qt::ImFont:
        return font();
    case Qt::ImAnchorPosition:
        // Fallback.
        return inputMethodQuery(Qt::ImCursorPosition);
    default:
        return QVariant();
    }
}

/*!
    \property QWidget::inputMethodHints
    \brief What input method specific hints the widget has.

    This is only relevant for input widgets. It is used by
    the input method to retrieve hints as to how the input method
    should operate. For example, if the Qt::ImhFormattedNumbersOnly flag
    is set, the input method may change its visual components to reflect
    that only numbers can be entered.

    \note The flags are only hints, so the particular input method
          implementation is free to ignore them. If you want to be
          sure that a certain type of characters are entered,
          you should also set a QValidator on the widget.

    The default value is Qt::ImhNone.

    \since 4.6

    \sa inputMethodQuery(), QInputContext
*/
Qt::InputMethodHints QWidget::inputMethodHints() const
{
#ifndef QT_NO_IM
    const QWidgetPrivate *priv = d_func();
    while (priv->inheritsInputMethodHints) {
        priv = priv->q_func()->parentWidget()->d_func();
        Q_ASSERT(priv);
    }
    return priv->imHints;
#else //QT_NO_IM
    return 0;
#endif //QT_NO_IM
}

void QWidget::setInputMethodHints(Qt::InputMethodHints hints)
{
#ifndef QT_NO_IM
    Q_D(QWidget);
    d->imHints = hints;
    // Optimization to update input context only it has already been created.
    if (d->ic || qApp->d_func()->inputContext) {
        QInputContext *ic = inputContext();
        if (ic)
            ic->update();
    }
#endif //QT_NO_IM
}


#ifndef QT_NO_DRAGANDDROP

/*!
    \fn void QWidget::dragEnterEvent(QDragEnterEvent *event)

    This event handler is called when a drag is in progress and the
    mouse enters this widget. The event is passed in the \a event parameter.

    If the event is ignored, the widget won't receive any \l{dragMoveEvent()}{drag
    move events}.

    See the \link dnd.html Drag-and-drop documentation\endlink for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDragEnterEvent
*/
void QWidget::dragEnterEvent(QDragEnterEvent *)
{
}

/*!
    \fn void QWidget::dragMoveEvent(QDragMoveEvent *event)

    This event handler is called if a drag is in progress, and when
    any of the following conditions occur: the cursor enters this widget,
    the cursor moves within this widget, or a modifier key is pressed on
    the keyboard while this widget has the focus. The event is passed
    in the \a event parameter.

    See the \link dnd.html Drag-and-drop documentation\endlink for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDragMoveEvent
*/
void QWidget::dragMoveEvent(QDragMoveEvent *)
{
}

/*!
    \fn void QWidget::dragLeaveEvent(QDragLeaveEvent *event)

    This event handler is called when a drag is in progress and the
    mouse leaves this widget. The event is passed in the \a event
    parameter.

    See the \link dnd.html Drag-and-drop documentation\endlink for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDragLeaveEvent
*/
void QWidget::dragLeaveEvent(QDragLeaveEvent *)
{
}

/*!
    \fn void QWidget::dropEvent(QDropEvent *event)

    This event handler is called when the drag is dropped on this
    widget. The event is passed in the \a event parameter.

    See the \link dnd.html Drag-and-drop documentation\endlink for an
    overview of how to provide drag-and-drop in your application.

    \sa QDrag, QDropEvent
*/
void QWidget::dropEvent(QDropEvent *)
{
}

#endif // QT_NO_DRAGANDDROP

/*!
    \fn void QWidget::showEvent(QShowEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget show events which are passed in the \a event parameter.

    Non-spontaneous show events are sent to widgets immediately
    before they are shown. The spontaneous show events of windows are
    delivered afterwards.

    Note: A widget receives spontaneous show and hide events when its
    mapping status is changed by the window system, e.g. a spontaneous
    hide event when the user minimizes the window, and a spontaneous
    show event when the window is restored again. After receiving a
    spontaneous hide event, a widget is still considered visible in
    the sense of isVisible().

    \sa visible, event(), QShowEvent
*/
void QWidget::showEvent(QShowEvent *)
{
}

/*!
    \fn void QWidget::hideEvent(QHideEvent *event)

    This event handler can be reimplemented in a subclass to receive
    widget hide events. The event is passed in the \a event parameter.

    Hide events are sent to widgets immediately after they have been
    hidden.

    Note: A widget receives spontaneous show and hide events when its
    mapping status is changed by the window system, e.g. a spontaneous
    hide event when the user minimizes the window, and a spontaneous
    show event when the window is restored again. After receiving a
    spontaneous hide event, a widget is still considered visible in
    the sense of isVisible().

    \sa visible, event(), QHideEvent
*/
void QWidget::hideEvent(QHideEvent *)
{
}

/*
    \fn QWidget::x11Event(MSG *)

    This special event handler can be reimplemented in a subclass to receive
    native X11 events.

    In your reimplementation of this function, if you want to stop Qt from
    handling the event, return true. If you return false, this native event
    is passed back to Qt, which translates it into a Qt event and sends it to
    the widget.

    \note Events are only delivered to this event handler if the widget is
    native.

    \warning This function is not portable.

    \sa QApplication::x11EventFilter(), QWidget::winId()
*/


#if defined(Q_WS_MAC)

/*!
    \fn bool QWidget::macEvent(EventHandlerCallRef caller, EventRef event)

    This special event handler can be reimplemented in a subclass to
    receive native Macintosh events.

    The parameters are a bit different depending if Qt is build against Carbon
    or Cocoa.  In Carbon, \a caller and \a event are the corresponding
    EventHandlerCallRef and EventRef that correspond to the Carbon event
    handlers that are installed. In Cocoa, \a caller is always 0 and the
    EventRef is the EventRef generated from the NSEvent.

    In your reimplementation of this function, if you want to stop the
    event being handled by Qt, return true. If you return false, this
    native event is passed back to Qt, which translates the event into
    a Qt event and sends it to the widget.

    \warning This function is not portable.

    \warning This function was not called inside of Qt until Qt 4.4.
    If you need compatibility with earlier versions of Qt, consider QApplication::macEventFilter() instead.

    \sa QApplication::macEventFilter()
*/

bool QWidget::macEvent(EventHandlerCallRef, EventRef)
{
    return false;
}

#endif
#if defined(Q_WS_WIN)

/*!
    This special event handler can be reimplemented in a subclass to
    receive native Windows events which are passed in the \a message
    parameter.

    In your reimplementation of this function, if you want to stop the
    event being handled by Qt, return true and set \a result to the value
    that the window procedure should return. If you return false, this
    native event is passed back to Qt, which translates the event into
    a Qt event and sends it to the widget.

    \warning This function is not portable.

    \sa QApplication::winEventFilter()
*/
bool QWidget::winEvent(MSG *message, long *result)
{
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}

#endif
#if defined(Q_WS_X11)

/*!
    \fn bool QWidget::x11Event(XEvent *event)

    This special event handler can be reimplemented in a subclass to receive
    native X11 events passed in the \a event parameter.

    In your reimplementation of this function, if you want to stop Qt from
    handling the event, return true. If you return false, this native event
    is passed back to Qt, which translates it into a Qt event and sends it to
    the widget.

    \note Events are only delivered to this event handler if the widget is
    native.

    \warning This function is not portable.

    \sa QApplication::x11EventFilter(), QWidget::winId()
*/
bool QWidget::x11Event(XEvent *)
{
    return false;
}

#endif
#if defined(Q_WS_QWS)

/*!
    \fn bool QWidget::qwsEvent(QWSEvent *event)

    This special event handler can be reimplemented in a subclass to
    receive native Qt for Embedded Linux events which are passed in the
    \a event parameter.

    In your reimplementation of this function, if you want to stop the
    event being handled by Qt, return true. If you return false, this
    native event is passed back to Qt, which translates the event into
    a Qt event and sends it to the widget.

    \warning This function is not portable.

    \sa QApplication::qwsEventFilter()
*/
bool QWidget::qwsEvent(QWSEvent *)
{
    return false;
}

#endif


/*!
    Ensures that the widget has been polished by QStyle (i.e., has a
    proper font and palette).

    QWidget calls this function after it has been fully constructed
    but before it is shown the very first time. You can call this
    function if you want to ensure that the widget is polished before
    doing an operation, e.g., the correct font size might be needed in
    the widget's sizeHint() reimplementation. Note that this function
    \e is called from the default implementation of sizeHint().

    Polishing is useful for final initialization that must happen after
    all constructors (from base classes as well as from subclasses)
    have been called.

    If you need to change some settings when a widget is polished,
    reimplement event() and handle the QEvent::Polish event type.

    \bold{Note:} The function is declared const so that it can be called from
    other const functions (e.g., sizeHint()).

    \sa event()
*/
void QWidget::ensurePolished() const
{
    Q_D(const QWidget);

    const QMetaObject *m = metaObject();
    if (m == d->polished)
        return;
    d->polished = m;

    QEvent e(QEvent::Polish);
    QCoreApplication::sendEvent(const_cast<QWidget *>(this), &e);

    // polish children after 'this'
    QList<QObject*> children = d->children;
    for (int i = 0; i < children.size(); ++i) {
        QObject *o = children.at(i);
        if(!o->isWidgetType())
            continue;
        if (QWidget *w = qobject_cast<QWidget *>(o))
            w->ensurePolished();
    }

    if (d->parent && d->sendChildEvents) {
        QChildEvent e(QEvent::ChildPolished, const_cast<QWidget *>(this));
        QCoreApplication::sendEvent(d->parent, &e);
    }
}

/*!
    Returns the mask currently set on a widget. If no mask is set the
    return value will be an empty region.

    \sa setMask(), clearMask(), QRegion::isEmpty(), {Shaped Clock Example}
*/
QRegion QWidget::mask() const
{
    Q_D(const QWidget);
    return d->extra ? d->extra->mask : QRegion();
}

/*!
    Returns the layout manager that is installed on this widget, or 0
    if no layout manager is installed.

    The layout manager sets the geometry of the widget's children
    that have been added to the layout.

    \sa setLayout(), sizePolicy(), {Layout Management}
*/
QLayout *QWidget::layout() const
{
    return d_func()->layout;
}


/*!
    \fn void QWidget::setLayout(QLayout *layout)

    Sets the layout manager for this widget to \a layout.

    If there already is a layout manager installed on this widget,
    QWidget won't let you install another. You must first delete the
    existing layout manager (returned by layout()) before you can
    call setLayout() with the new layout.

    If \a layout is the layout manger on a different widget, setLayout()
    will reparent the layout and make it the layout manager for this widget.

    Example:

    \snippet examples/uitools/textfinder/textfinder.cpp 3b

    An alternative to calling this function is to pass this widget to
    the layout's constructor.

    The QWidget will take ownership of \a layout.

    \sa layout(), {Layout Management}
*/

void QWidget::setLayout(QLayout *l)
{
    if (!l) {
        qWarning("QWidget::setLayout: Cannot set layout to 0");
        return;
    }
    if (layout()) {
        if (layout() != l)
            qWarning("QWidget::setLayout: Attempting to set QLayout \"%s\" on %s \"%s\", which already has a"
                     " layout", l->objectName().toLocal8Bit().data(), metaObject()->className(),
                     objectName().toLocal8Bit().data());
        return;
    }

    QObject *oldParent = l->parent();
    if (oldParent && oldParent != this) {
        if (oldParent->isWidgetType()) {
            // Steal the layout off a widget parent. Takes effect when
            // morphing laid-out container widgets in Designer.
            QWidget *oldParentWidget = static_cast<QWidget *>(oldParent);
            oldParentWidget->takeLayout();
        } else {
            qWarning("QWidget::setLayout: Attempting to set QLayout \"%s\" on %s \"%s\", when the QLayout already has a parent",
                     l->objectName().toLocal8Bit().data(), metaObject()->className(),
                     objectName().toLocal8Bit().data());
            return;
        }
    }

    Q_D(QWidget);
    l->d_func()->topLevel = true;
    d->layout = l;
    if (oldParent != this) {
        l->setParent(this);
        l->d_func()->reparentChildWidgets(this);
        l->invalidate();
    }

    if (isWindow() && d->maybeTopData())
        d->topData()->sizeAdjusted = false;
}

/*!
    \fn QLayout *QWidget::takeLayout()

    Remove the layout from the widget.
    \since 4.5
*/

QLayout *QWidget::takeLayout()
{
    Q_D(QWidget);
    QLayout *l =  layout();
    if (!l)
        return 0;
    d->layout = 0;
    l->setParent(0);
    return l;
}

/*!
    \property QWidget::sizePolicy
    \brief the default layout behavior of the widget

    If there is a QLayout that manages this widget's children, the
    size policy specified by that layout is used. If there is no such
    QLayout, the result of this function is used.

    The default policy is Preferred/Preferred, which means that the
    widget can be freely resized, but prefers to be the size
    sizeHint() returns. Button-like widgets set the size policy to
    specify that they may stretch horizontally, but are fixed
    vertically. The same applies to lineedit controls (such as
    QLineEdit, QSpinBox or an editable QComboBox) and other
    horizontally orientated widgets (such as QProgressBar).
    QToolButton's are normally square, so they allow growth in both
    directions. Widgets that support different directions (such as
    QSlider, QScrollBar or QHeader) specify stretching in the
    respective direction only. Widgets that can provide scroll bars
    (usually subclasses of QScrollArea) tend to specify that they can
    use additional space, and that they can make do with less than
    sizeHint().

    \sa sizeHint() QLayout QSizePolicy updateGeometry()
*/
QSizePolicy QWidget::sizePolicy() const
{
    Q_D(const QWidget);
    return d->size_policy;
}

void QWidget::setSizePolicy(QSizePolicy policy)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_WState_OwnSizePolicy);
    if (policy == d->size_policy)
        return;
    d->size_policy = policy;

#ifndef QT_NO_GRAPHICSVIEW
    if (QWExtra *extra = d->extra) {
        if (extra->proxyWidget)
            extra->proxyWidget->setSizePolicy(policy);
    }
#endif

    updateGeometry();

    if (isWindow() && d->maybeTopData())
        d->topData()->sizeAdjusted = false;
}

/*!
    \fn void QWidget::setSizePolicy(QSizePolicy::Policy horizontal, QSizePolicy::Policy vertical)
    \overload

    Sets the size policy of the widget to \a horizontal and \a
    vertical, with standard stretch and no height-for-width.

    \sa QSizePolicy::QSizePolicy()
*/

/*!
    Returns the preferred height for this widget, given the width \a w.

    If this widget has a layout, the default implementation returns
    the layout's preferred height.  if there is no layout, the default
    implementation returns -1 indicating that the preferred height
    does not depend on the width.
*/

int QWidget::heightForWidth(int w) const
{
    if (layout() && layout()->hasHeightForWidth())
        return layout()->totalHeightForWidth(w);
    return -1;
}


/*!
    \internal

    *virtual private*

    This is a bit hackish, but ideally we would have created a virtual function
    in the public API (however, too late...) so that subclasses could reimplement 
    their own function.
    Instead we add a virtual function to QWidgetPrivate.
    ### Qt5: move to public class and make virtual
*/ 
bool QWidgetPrivate::hasHeightForWidth() const
{
    return layout ? layout->hasHeightForWidth() : size_policy.hasHeightForWidth();
}

/*!
    \fn QWidget *QWidget::childAt(int x, int y) const

    Returns the visible child widget at the position (\a{x}, \a{y})
    in the widget's coordinate system. If there is no visible child
    widget at the specified position, the function returns 0.
*/

/*!
    \overload

    Returns the visible child widget at point \a p in the widget's own
    coordinate system.
*/

QWidget *QWidget::childAt(const QPoint &p) const
{
    return d_func()->childAt_helper(p, false);
}

QWidget *QWidgetPrivate::childAt_helper(const QPoint &p, bool ignoreChildrenInDestructor) const
{
    if (children.isEmpty())
        return 0;

#ifdef Q_WS_MAC
    Q_Q(const QWidget);
    // Unified tool bars on the Mac require special handling since they live outside
    // QMainWindow's geometry(). See commit: 35667fd45ada49269a5987c235fdedfc43e92bb8
    bool includeFrame = q->isWindow() && qobject_cast<const QMainWindow *>(q)
                        && static_cast<const QMainWindow *>(q)->unifiedTitleAndToolBarOnMac();
    if (includeFrame)
        return childAtRecursiveHelper(p, ignoreChildrenInDestructor, includeFrame);
#endif

    if (!pointInsideRectAndMask(p))
        return 0;
    return childAtRecursiveHelper(p, ignoreChildrenInDestructor);
}

QWidget *QWidgetPrivate::childAtRecursiveHelper(const QPoint &p, bool ignoreChildrenInDestructor, bool includeFrame) const
{
#ifndef Q_WS_MAC
    Q_UNUSED(includeFrame);
#endif
    for (int i = children.size() - 1; i >= 0; --i) {
        QWidget *child = qobject_cast<QWidget *>(children.at(i));
        if (!child || child->isWindow() || child->isHidden() || child->testAttribute(Qt::WA_TransparentForMouseEvents)
            || (ignoreChildrenInDestructor && child->data->in_destructor)) {
            continue;
        }

        // Map the point 'p' from parent coordinates to child coordinates.
        QPoint childPoint = p;
#ifdef Q_WS_MAC
        // 'includeFrame' is true if the child's parent is a top-level QMainWindow with an unified tool bar.
        // An unified tool bar on the Mac lives outside QMainWindow's geometry(), so a normal
        // QWidget::mapFromParent won't do the trick.
        if (includeFrame && qobject_cast<QToolBar *>(child))
            childPoint = qt_mac_nativeMapFromParent(child, p);
        else
#endif
        childPoint -= child->data->crect.topLeft();

        // Check if the point hits the child.
        if (!child->d_func()->pointInsideRectAndMask(childPoint))
            continue;

        // Do the same for the child's descendants.
        if (QWidget *w = child->d_func()->childAtRecursiveHelper(childPoint, ignoreChildrenInDestructor))
            return w;

        // We have found our target; namely the child at position 'p'.
        return child;
    }
    return 0;
}

void QWidgetPrivate::updateGeometry_helper(bool forceUpdate)
{
    Q_Q(QWidget);
    if (widgetItem)
        widgetItem->invalidateSizeCache();
    QWidget *parent;
    if (forceUpdate || !extra || extra->minw != extra->maxw || extra->minh != extra->maxh) {
        if (!q->isWindow() && !q->isHidden() && (parent = q->parentWidget())) {
            if (parent->d_func()->layout)
                parent->d_func()->layout->invalidate();
            else if (parent->isVisible())
                QApplication::postEvent(parent, new QEvent(QEvent::LayoutRequest));
        }
    }
}

/*!
    Notifies the layout system that this widget has changed and may
    need to change geometry.

    Call this function if the sizeHint() or sizePolicy() have changed.

    For explicitly hidden widgets, updateGeometry() is a no-op. The
    layout system will be notified as soon as the widget is shown.
*/

void QWidget::updateGeometry()
{
    Q_D(QWidget);
    d->updateGeometry_helper(false);
}

/*! \property QWidget::windowFlags

    Window flags are a combination of a type (e.g. Qt::Dialog) and
    zero or more hints to the window system (e.g.
    Qt::FramelessWindowHint).

    If the widget had type Qt::Widget or Qt::SubWindow and becomes a
    window (Qt::Window, Qt::Dialog, etc.), it is put at position (0,
    0) on the desktop. If the widget is a window and becomes a
    Qt::Widget or Qt::SubWindow, it is put at position (0, 0)
    relative to its parent widget.

    \note This function calls setParent() when changing the flags for
    a window, causing the widget to be hidden. You must call show() to make
    the widget visible again..

    \sa windowType(), {Window Flags Example}
*/
void QWidget::setWindowFlags(Qt::WindowFlags flags)
{
    if (data->window_flags == flags)
        return;

    Q_D(QWidget);

    if ((data->window_flags | flags) & Qt::Window) {
        // the old type was a window and/or the new type is a window
        QPoint oldPos = pos();
        bool visible = isVisible();
        setParent(parentWidget(), flags);

        // if both types are windows or neither of them are, we restore
        // the old position
        if (!((data->window_flags ^ flags) & Qt::Window)
            && (visible || testAttribute(Qt::WA_Moved))) {
            move(oldPos);
        }
        // for backward-compatibility we change Qt::WA_QuitOnClose attribute value only when the window was recreated.
        d->adjustQuitOnCloseAttribute();
    } else {
        data->window_flags = flags;
    }
}

/*!
    Sets the window flags for the widget to \a flags,
    \e without telling the window system.

    \warning Do not call this function unless you really know what
    you're doing.

    \sa setWindowFlags()
*/
void QWidget::overrideWindowFlags(Qt::WindowFlags flags)
{
    data->window_flags = flags;
}

/*!
    \fn Qt::WindowType QWidget::windowType() const

    Returns the window type of this widget. This is identical to
    windowFlags() & Qt::WindowType_Mask.

    \sa windowFlags
*/

/*!
    Sets the parent of the widget to \a parent, and resets the window
    flags. The widget is moved to position (0, 0) in its new parent.

    If the new parent widget is in a different window, the
    reparented widget and its children are appended to the end of the
    \l{setFocusPolicy()}{tab chain} of the new parent
    widget, in the same internal order as before. If one of the moved
    widgets had keyboard focus, setParent() calls clearFocus() for that
    widget.

    If the new parent widget is in the same window as the
    old parent, setting the parent doesn't change the tab order or
    keyboard focus.

    If the "new" parent widget is the old parent widget, this function
    does nothing.

    \note The widget becomes invisible as part of changing its parent,
    even if it was previously visible. You must call show() to make the
    widget visible again.

    \warning It is very unlikely that you will ever need this
    function. If you have a widget that changes its content
    dynamically, it is far easier to use \l QStackedWidget.

    \sa setWindowFlags()
*/
void QWidget::setParent(QWidget *parent)
{
    if (parent == parentWidget())
        return;
    setParent((QWidget*)parent, windowFlags() & ~Qt::WindowType_Mask);
}

/*!
    \overload

    This function also takes widget flags, \a f as an argument.
*/

void QWidget::setParent(QWidget *parent, Qt::WindowFlags f)
{
    Q_D(QWidget);
    d->inSetParent = true;
    bool resized = testAttribute(Qt::WA_Resized);
    bool wasCreated = testAttribute(Qt::WA_WState_Created);
    QWidget *oldtlw = window();

    QWidget *desktopWidget = 0;
    if (parent && parent->windowType() == Qt::Desktop)
        desktopWidget = parent;
    bool newParent = (parent != parentWidget()) || !wasCreated || desktopWidget;

#if defined(Q_WS_X11) || defined(Q_WS_WIN) || defined(Q_WS_MAC) || defined(Q_OS_SYMBIAN)
    if (newParent && parent && !desktopWidget) {
        if (testAttribute(Qt::WA_NativeWindow) && !qApp->testAttribute(Qt::AA_DontCreateNativeWidgetSiblings)
#if defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
            // On Mac, toolbars inside the unified title bar will never overlap with
            // siblings in the content view. So we skip enforce native siblings in that case
            && !d->isInUnifiedToolbar && parentWidget() && parentWidget()->isWindow()
#endif // Q_WS_MAC && QT_MAC_USE_COCOA
        )
            parent->d_func()->enforceNativeChildren();
        else if (parent->d_func()->nativeChildrenForced() || parent->testAttribute(Qt::WA_PaintOnScreen))
            setAttribute(Qt::WA_NativeWindow);
    }
#endif

    if (wasCreated) {
        if (!testAttribute(Qt::WA_WState_Hidden)) {
            hide();
            setAttribute(Qt::WA_WState_ExplicitShowHide, false);
        }
        if (newParent) {
            QEvent e(QEvent::ParentAboutToChange);
            QApplication::sendEvent(this, &e);
        }
    }
    if (newParent && isAncestorOf(focusWidget()))
        focusWidget()->clearFocus();

    QTLWExtra *oldTopExtra = window()->d_func()->maybeTopData();
    QWidgetBackingStoreTracker *oldBsTracker = oldTopExtra ? &oldTopExtra->backingStore : 0;

    d->setParent_sys(parent, f);

    QTLWExtra *topExtra = window()->d_func()->maybeTopData();
    QWidgetBackingStoreTracker *bsTracker = topExtra ? &topExtra->backingStore : 0;
    if (oldBsTracker && oldBsTracker != bsTracker)
        oldBsTracker->unregisterWidgetSubtree(this);

    if (desktopWidget)
        parent = 0;

#ifdef Q_BACKINGSTORE_SUBSURFACES
    QTLWExtra *extra = d->maybeTopData();
    QWindowSurface *windowSurface = (extra ? extra->windowSurface : 0);
    if (newParent && windowSurface) {
        QWidgetBackingStore *oldBs = oldtlw->d_func()->maybeBackingStore();
        if (oldBs)
            oldBs->subSurfaces.removeAll(windowSurface);

        if (parent) {
            QWidgetBackingStore *newBs = parent->d_func()->maybeBackingStore();
            if (newBs)
                newBs->subSurfaces.append(windowSurface);
        }
    }
#endif

    if (QWidgetBackingStore *oldBs = oldtlw->d_func()->maybeBackingStore()) {
        if (newParent)
            oldBs->removeDirtyWidget(this);
        // Move the widget and all its static children from
        // the old backing store to the new one.
        oldBs->moveStaticWidgets(this);
    }

    if ((QApplicationPrivate::app_compile_version < 0x040200
         || QApplicationPrivate::testAttribute(Qt::AA_ImmediateWidgetCreation))
        && !testAttribute(Qt::WA_WState_Created))
        create();

    d->reparentFocusWidgets(oldtlw);
    setAttribute(Qt::WA_Resized, resized);
    if (!testAttribute(Qt::WA_StyleSheet)
        && (!parent || !parent->testAttribute(Qt::WA_StyleSheet))) {
        d->resolveFont();
        d->resolvePalette();
    }
    d->resolveLayoutDirection();
    d->resolveLocale();

    // Note: GL widgets under WGL or EGL will always need a ParentChange
    // event to handle recreation/rebinding of the GL context, hence the
    // (f & Qt::MSWindowsOwnDC) clause (which is set on QGLWidgets on all
    // platforms).
    if (newParent
#if defined(Q_WS_WIN) || defined(QT_OPENGL_ES)
        || (f & Qt::MSWindowsOwnDC)
#endif
        ) {
        // propagate enabled updates enabled state to non-windows
        if (!isWindow()) {
            if (!testAttribute(Qt::WA_ForceDisabled))
                d->setEnabled_helper(parent ? parent->isEnabled() : true);
            if (!testAttribute(Qt::WA_ForceUpdatesDisabled))
                d->setUpdatesEnabled_helper(parent ? parent->updatesEnabled() : true);
        }
        d->inheritStyle();

        // send and post remaining QObject events
        if (parent && d->sendChildEvents) {
            QChildEvent e(QEvent::ChildAdded, this);
            QApplication::sendEvent(parent, &e);
#ifdef QT3_SUPPORT
            if (parent->d_func()->pendingChildInsertedEvents.isEmpty()) {
                QApplication::postEvent(parent,
                                        new QEvent(QEvent::ChildInsertedRequest),
                                        Qt::HighEventPriority);
            }
            parent->d_func()->pendingChildInsertedEvents.append(this);
#endif
        }

//### already hidden above ---> must probably do something smart on the mac
// #ifdef Q_WS_MAC
//             extern bool qt_mac_is_macdrawer(const QWidget *); //qwidget_mac.cpp
//             if(!qt_mac_is_macdrawer(q)) //special case
//                 q->setAttribute(Qt::WA_WState_Hidden);
// #else
//             q->setAttribute(Qt::WA_WState_Hidden);
//#endif

        if (parent && d->sendChildEvents && d->polished) {
            QChildEvent e(QEvent::ChildPolished, this);
            QCoreApplication::sendEvent(parent, &e);
        }

        QEvent e(QEvent::ParentChange);
        QApplication::sendEvent(this, &e);
    }

    if (!wasCreated) {
        if (isWindow() || parentWidget()->isVisible())
            setAttribute(Qt::WA_WState_Hidden, true);
        else if (!testAttribute(Qt::WA_WState_ExplicitShowHide))
            setAttribute(Qt::WA_WState_Hidden, false);
    }

    d->updateIsOpaque();

#ifndef QT_NO_GRAPHICSVIEW
    // Embed the widget into a proxy if the parent is embedded.
    // ### Doesn't handle reparenting out of an embedded widget.
    if (oldtlw->graphicsProxyWidget()) {
        if (QGraphicsProxyWidget *ancestorProxy = d->nearestGraphicsProxyWidget(oldtlw))
            ancestorProxy->d_func()->unembedSubWindow(this);
    }
    if (isWindow() && parent && !graphicsProxyWidget() && !bypassGraphicsProxyWidget(this)) {
        if (QGraphicsProxyWidget *ancestorProxy = d->nearestGraphicsProxyWidget(parent))
            ancestorProxy->d_func()->embedSubWindow(this);
    }
#endif

    d->inSetParent = false;
}

/*!
    Scrolls the widget including its children \a dx pixels to the
    right and \a dy downward. Both \a dx and \a dy may be negative.

    After scrolling, the widgets will receive paint events for
    the areas that need to be repainted. For widgets that Qt knows to
    be opaque, this is only the newly exposed parts.
    For example, if an opaque widget is scrolled 8 pixels to the left,
    only an 8-pixel wide stripe at the right edge needs updating.

    Since widgets propagate the contents of their parents by default,
    you need to set the \l autoFillBackground property, or use
    setAttribute() to set the Qt::WA_OpaquePaintEvent attribute, to make
    a widget opaque.

    For widgets that use contents propagation, a scroll will cause an
    update of the entire scroll area.

    \sa {Transparency and Double Buffering}
*/

void QWidget::scroll(int dx, int dy)
{
    if ((!updatesEnabled() && children().size() == 0) || !isVisible())
        return;
    if (dx == 0 && dy == 0)
        return;
    Q_D(QWidget);
#ifndef QT_NO_GRAPHICSVIEW
    if (QGraphicsProxyWidget *proxy = QWidgetPrivate::nearestGraphicsProxyWidget(this)) {
        // Graphics View maintains its own dirty region as a list of rects;
        // until we can connect item updates directly to the view, we must
        // separately add a translated dirty region.
        if (!d->dirty.isEmpty()) {
            foreach (const QRect &rect, (d->dirty.translated(dx, dy)).rects())
                proxy->update(rect);
        }
        proxy->scroll(dx, dy, proxy->subWidgetRect(this));
        return;
    }
#endif
    d->setDirtyOpaqueRegion();
    d->scroll_sys(dx, dy);
}

/*!
    \overload

    This version only scrolls \a r and does not move the children of
    the widget.

    If \a r is empty or invalid, the result is undefined.

    \sa QScrollArea
*/
void QWidget::scroll(int dx, int dy, const QRect &r)
{

    if ((!updatesEnabled() && children().size() == 0) || !isVisible())
        return;
    if (dx == 0 && dy == 0)
        return;
    Q_D(QWidget);
#ifndef QT_NO_GRAPHICSVIEW
    if (QGraphicsProxyWidget *proxy = QWidgetPrivate::nearestGraphicsProxyWidget(this)) {
        // Graphics View maintains its own dirty region as a list of rects;
        // until we can connect item updates directly to the view, we must
        // separately add a translated dirty region.
        if (!d->dirty.isEmpty()) {
            foreach (const QRect &rect, (d->dirty.translated(dx, dy) & r).rects())
                proxy->update(rect);
        }
        proxy->scroll(dx, dy, r.translated(proxy->subWidgetRect(this).topLeft().toPoint()));
        return;
    }
#endif
    d->scroll_sys(dx, dy, r);
}

/*!
    Repaints the widget directly by calling paintEvent() immediately,
    unless updates are disabled or the widget is hidden.

    We suggest only using repaint() if you need an immediate repaint,
    for example during animation. In almost all circumstances update()
    is better, as it permits Qt to optimize for speed and minimize
    flicker.

    \warning If you call repaint() in a function which may itself be
    called from paintEvent(), you may get infinite recursion. The
    update() function never causes recursion.

    \sa update(), paintEvent(), setUpdatesEnabled()
*/

void QWidget::repaint()
{
    repaint(rect());
}

/*! \overload

    This version repaints a rectangle (\a x, \a y, \a w, \a h) inside
    the widget.

    If \a w is negative, it is replaced with \c{width() - x}, and if
    \a h is negative, it is replaced width \c{height() - y}.
*/
void QWidget::repaint(int x, int y, int w, int h)
{
    if (x > data->crect.width() || y > data->crect.height())
        return;

    if (w < 0)
        w = data->crect.width()  - x;
    if (h < 0)
        h = data->crect.height() - y;

    repaint(QRect(x, y, w, h));
}

/*! \overload

    This version repaints a rectangle \a rect inside the widget.
*/
void QWidget::repaint(const QRect &rect)
{
    Q_D(QWidget);

    if (testAttribute(Qt::WA_WState_ConfigPending)) {
        update(rect);
        return;
    }

    if (!isVisible() || !updatesEnabled() || rect.isEmpty())
        return;

    if (hasBackingStoreSupport()) {
#ifdef QT_MAC_USE_COCOA
        if (qt_widget_private(this)->isInUnifiedToolbar) {
            qt_widget_private(this)->unifiedSurface->renderToolbar(this, true);
            return;
        }
#endif // QT_MAC_USE_COCOA
        QTLWExtra *tlwExtra = window()->d_func()->maybeTopData();
        if (tlwExtra && !tlwExtra->inTopLevelResize && tlwExtra->backingStore) {
            tlwExtra->inRepaint = true;
            tlwExtra->backingStore->markDirty(rect, this, true);
            tlwExtra->inRepaint = false;
        }
    } else {
        d->repaint_sys(rect);
    }
}

/*!
    \overload

    This version repaints a region \a rgn inside the widget.
*/
void QWidget::repaint(const QRegion &rgn)
{
    Q_D(QWidget);

    if (testAttribute(Qt::WA_WState_ConfigPending)) {
        update(rgn);
        return;
    }

    if (!isVisible() || !updatesEnabled() || rgn.isEmpty())
        return;

    if (hasBackingStoreSupport()) {
#ifdef QT_MAC_USE_COCOA
        if (qt_widget_private(this)->isInUnifiedToolbar) {
            qt_widget_private(this)->unifiedSurface->renderToolbar(this, true);
            return;
        }
#endif // QT_MAC_USE_COCOA
        QTLWExtra *tlwExtra = window()->d_func()->maybeTopData();
        if (tlwExtra && !tlwExtra->inTopLevelResize && tlwExtra->backingStore) {
            tlwExtra->inRepaint = true;
            tlwExtra->backingStore->markDirty(rgn, this, true);
            tlwExtra->inRepaint = false;
        }
    } else {
        d->repaint_sys(rgn);
    }
}

/*!
    Updates the widget unless updates are disabled or the widget is
    hidden.

    This function does not cause an immediate repaint; instead it
    schedules a paint event for processing when Qt returns to the main
    event loop. This permits Qt to optimize for more speed and less
    flicker than a call to repaint() does.

    Calling update() several times normally results in just one
    paintEvent() call.

    Qt normally erases the widget's area before the paintEvent() call.
    If the Qt::WA_OpaquePaintEvent widget attribute is set, the widget is
    responsible for painting all its pixels with an opaque color.

    \sa repaint() paintEvent(), setUpdatesEnabled(), {Analog Clock Example}
*/
void QWidget::update()
{
    update(rect());
}

/*! \fn void QWidget::update(int x, int y, int w, int h)
    \overload

    This version updates a rectangle (\a x, \a y, \a w, \a h) inside
    the widget.
*/

/*!
    \overload

    This version updates a rectangle \a rect inside the widget.
*/
void QWidget::update(const QRect &rect)
{
    if (!isVisible() || !updatesEnabled() || rect.isEmpty())
        return;

    if (testAttribute(Qt::WA_WState_InPaintEvent)) {
        QApplication::postEvent(this, new QUpdateLaterEvent(rect));
        return;
    }

    if (hasBackingStoreSupport()) {
#ifdef QT_MAC_USE_COCOA
        if (qt_widget_private(this)->isInUnifiedToolbar) {
            qt_widget_private(this)->unifiedSurface->renderToolbar(this, true);
            return;
        }
#endif // QT_MAC_USE_COCOA
        QTLWExtra *tlwExtra = window()->d_func()->maybeTopData();
        if (tlwExtra && !tlwExtra->inTopLevelResize && tlwExtra->backingStore)
            tlwExtra->backingStore->markDirty(rect, this);
    } else {
        d_func()->repaint_sys(rect);
    }
}

/*!
    \overload

    This version repaints a region \a rgn inside the widget.
*/
void QWidget::update(const QRegion &rgn)
{
    if (!isVisible() || !updatesEnabled() || rgn.isEmpty())
        return;

    if (testAttribute(Qt::WA_WState_InPaintEvent)) {
        QApplication::postEvent(this, new QUpdateLaterEvent(rgn));
        return;
    }

    if (hasBackingStoreSupport()) {
#ifdef QT_MAC_USE_COCOA
        if (qt_widget_private(this)->isInUnifiedToolbar) {
            qt_widget_private(this)->unifiedSurface->renderToolbar(this, true);
            return;
        }
#endif // QT_MAC_USE_COCOA
        QTLWExtra *tlwExtra = window()->d_func()->maybeTopData();
        if (tlwExtra && !tlwExtra->inTopLevelResize && tlwExtra->backingStore)
            tlwExtra->backingStore->markDirty(rgn, this);
    } else {
        d_func()->repaint_sys(rgn);
    }
}

#ifdef QT3_SUPPORT
/*!
    Clear the rectangle at point (\a x, \a y) of width \a w and height
    \a h.

    \warning This is best done in a paintEvent().
*/
void QWidget::erase_helper(int x, int y, int w, int h)
{
    if (testAttribute(Qt::WA_NoSystemBackground) || testAttribute(Qt::WA_UpdatesDisabled) ||  !testAttribute(Qt::WA_WState_Visible))
        return;
    if (w < 0)
        w = data->crect.width()  - x;
    if (h < 0)
        h = data->crect.height() - y;
    if (w != 0 && h != 0) {
        QPainter p(this);
        p.eraseRect(QRect(x, y, w, h));
    }
}

/*!
    \overload

    Clear the given region, \a rgn.

    Drawing may only take place in a QPaintEvent. Overload
    paintEvent() to do your erasing and call update() to schedule a
    replaint whenever necessary. See also QPainter.
*/
void QWidget::erase(const QRegion& rgn)
{
    if (testAttribute(Qt::WA_NoSystemBackground) || testAttribute(Qt::WA_UpdatesDisabled) || !testAttribute(Qt::WA_WState_Visible))
        return;

    QPainter p(this);
    p.setClipRegion(rgn);
    p.eraseRect(rgn.boundingRect());
}

void QWidget::drawText_helper(int x, int y, const QString &str)
{
    if(!testAttribute(Qt::WA_WState_Visible))
        return;
    QPainter paint(this);
    paint.drawText(x, y, str);
}


/*!
    Closes the widget.

    Use the no-argument overload instead.
*/
bool QWidget::close(bool alsoDelete)
{
    QPointer<QWidget> that = this;
    bool accepted = close();
    if (alsoDelete && accepted && that)
        deleteLater();
    return accepted;
}

void QWidget::setIcon(const QPixmap &i)
{
    setWindowIcon(i);
}

/*!
    Return's the widget's icon.

    Use windowIcon() instead.
*/
const QPixmap *QWidget::icon() const
{
    Q_D(const QWidget);
    return (d->extra && d->extra->topextra) ? d->extra->topextra->iconPixmap : 0;
}

#endif // QT3_SUPPORT

 /*!
  \internal

  This just sets the corresponding attribute bit to 1 or 0
 */
static void setAttribute_internal(Qt::WidgetAttribute attribute, bool on, QWidgetData *data,
                                  QWidgetPrivate *d)
{
    if (attribute < int(8*sizeof(uint))) {
        if (on)
            data->widget_attributes |= (1<<attribute);
        else
            data->widget_attributes &= ~(1<<attribute);
    } else {
        const int x = attribute - 8*sizeof(uint);
        const int int_off = x / (8*sizeof(uint));
        if (on)
            d->high_attributes[int_off] |= (1<<(x-(int_off*8*sizeof(uint))));
        else
            d->high_attributes[int_off] &= ~(1<<(x-(int_off*8*sizeof(uint))));
    }
}

/*!
    Sets the attribute \a attribute on this widget if \a on is true;
    otherwise clears the attribute.

    \sa testAttribute()
*/
void QWidget::setAttribute(Qt::WidgetAttribute attribute, bool on)
{
    if (testAttribute(attribute) == on)
        return;

    Q_D(QWidget);
    Q_ASSERT_X(sizeof(d->high_attributes)*8 >= (Qt::WA_AttributeCount - sizeof(uint)*8),
               "QWidget::setAttribute(WidgetAttribute, bool)",
               "QWidgetPrivate::high_attributes[] too small to contain all attributes in WidgetAttribute");
#ifdef Q_WS_WIN
    // ### Don't use PaintOnScreen+paintEngine() to do native painting in 5.0
    if (attribute == Qt::WA_PaintOnScreen && on && !inherits("QGLWidget")) {
        // see qwidget_win.cpp, ::paintEngine for details
        paintEngine();
        if (d->noPaintOnScreen)
            return;
    }
#endif

    setAttribute_internal(attribute, on, data, d);

    switch (attribute) {

#ifndef QT_NO_DRAGANDDROP
    case Qt::WA_AcceptDrops:  {
        if (on && !testAttribute(Qt::WA_DropSiteRegistered))
            setAttribute(Qt::WA_DropSiteRegistered, true);
        else if (!on && (isWindow() || !parentWidget() || !parentWidget()->testAttribute(Qt::WA_DropSiteRegistered)))
            setAttribute(Qt::WA_DropSiteRegistered, false);
        QEvent e(QEvent::AcceptDropsChange);
        QApplication::sendEvent(this, &e);
        break;
    }
    case Qt::WA_DropSiteRegistered:  {
        d->registerDropSite(on);
        for (int i = 0; i < d->children.size(); ++i) {
            QWidget *w = qobject_cast<QWidget *>(d->children.at(i));
            if (w && !w->isWindow() && !w->testAttribute(Qt::WA_AcceptDrops) && w->testAttribute(Qt::WA_DropSiteRegistered) != on)
                w->setAttribute(Qt::WA_DropSiteRegistered, on);
        }
        break;
    }
#endif

    case Qt::WA_NoChildEventsForParent:
        d->sendChildEvents = !on;
        break;
    case Qt::WA_NoChildEventsFromChildren:
        d->receiveChildEvents = !on;
        break;
    case Qt::WA_MacBrushedMetal:
#ifdef Q_WS_MAC
        d->setStyle_helper(style(), false, true);  // Make sure things get unpolished/polished correctly.
        // fall through since changing the metal attribute affects the opaque size grip.
    case Qt::WA_MacOpaqueSizeGrip:
        d->macUpdateOpaqueSizeGrip();
        break;
    case Qt::WA_MacShowFocusRect:
        if (hasFocus()) {
            clearFocus();
            setFocus();
        }
        break;
    case Qt::WA_Hover:
        qt_mac_update_mouseTracking(this);
        break;
#endif
    case Qt::WA_MacAlwaysShowToolWindow:
#ifdef Q_WS_MAC
        d->macUpdateHideOnSuspend();
#endif
        break;
    case Qt::WA_MacNormalSize:
    case Qt::WA_MacSmallSize:
    case Qt::WA_MacMiniSize:
#ifdef Q_WS_MAC
        {
            // We can only have one of these set at a time
            const Qt::WidgetAttribute MacSizes[] = { Qt::WA_MacNormalSize, Qt::WA_MacSmallSize,
                                                     Qt::WA_MacMiniSize };
            for (int i = 0; i < 3; ++i) {
                if (MacSizes[i] != attribute)
                    setAttribute_internal(MacSizes[i], false, data, d);
            }
            d->macUpdateSizeAttribute();
        }
#endif
        break;
    case Qt::WA_ShowModal:
        if (!on) {
            if (isVisible())
                QApplicationPrivate::leaveModal(this);
            // reset modality type to Modeless when clearing WA_ShowModal
            data->window_modality = Qt::NonModal;
        } else if (data->window_modality == Qt::NonModal) {
            // determine the modality type if it hasn't been set prior
            // to setting WA_ShowModal. set the default to WindowModal
            // if we are the child of a group leader; otherwise use
            // ApplicationModal.
            QWidget *w = parentWidget();
            if (w)
                w = w->window();
            while (w && !w->testAttribute(Qt::WA_GroupLeader)) {
                w = w->parentWidget();
                if (w)
                    w = w->window();
            }
            data->window_modality = (w && w->testAttribute(Qt::WA_GroupLeader))
                                    ? Qt::WindowModal
                                    : Qt::ApplicationModal;
            // Some window managers does not allow us to enter modal after the
            // window is showing. Therefore, to be consistent, we cannot call
            // QApplicationPrivate::enterModal(this) here. The window must be
            // hidden before changing modality.
        }
        if (testAttribute(Qt::WA_WState_Created)) {
            // don't call setModal_sys() before create_sys()
            d->setModal_sys();
        }
        break;
    case Qt::WA_MouseTracking: {
        QEvent e(QEvent::MouseTrackingChange);
        QApplication::sendEvent(this, &e);
        break; }
    case Qt::WA_NativeWindow: {
#ifndef QT_NO_IM
        QWidget *focusWidget = d->effectiveFocusWidget();
        QInputContext *ic = 0;
        if (on && !internalWinId() && hasFocus()
            && focusWidget->testAttribute(Qt::WA_InputMethodEnabled)) {
            ic = focusWidget->d_func()->inputContext();
            if (ic) {
                ic->reset();
                ic->setFocusWidget(0);
            }
        }
        if (!qApp->testAttribute(Qt::AA_DontCreateNativeWidgetSiblings) && parentWidget()
#if defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
            // On Mac, toolbars inside the unified title bar will never overlap with
            // siblings in the content view. So we skip enforce native siblings in that case
            && !d->isInUnifiedToolbar && parentWidget()->isWindow()
#endif // Q_WS_MAC && QT_MAC_USE_COCOA
        )
            parentWidget()->d_func()->enforceNativeChildren();
        if (on && !internalWinId() && testAttribute(Qt::WA_WState_Created))
            d->createWinId();
        if (ic && isEnabled() && focusWidget->isEnabled()
            && focusWidget->testAttribute(Qt::WA_InputMethodEnabled)) {
            ic->setFocusWidget(focusWidget);
        }
#endif //QT_NO_IM
        break;
    }
    case Qt::WA_PaintOnScreen:
        d->updateIsOpaque();
#if defined(Q_WS_WIN) || defined(Q_WS_X11) || defined(Q_WS_MAC) || defined(Q_OS_SYMBIAN)
        // Recreate the widget if it's already created as an alien widget and
        // WA_PaintOnScreen is enabled. Paint on screen widgets must have win id.
        // So must their children.
        if (on) {
            setAttribute(Qt::WA_NativeWindow);
            d->enforceNativeChildren();
        }
#endif
        // fall through
    case Qt::WA_OpaquePaintEvent:
        d->updateIsOpaque();
        break;
    case Qt::WA_NoSystemBackground:
        d->updateIsOpaque();
        // fall through...
    case Qt::WA_UpdatesDisabled:
        d->updateSystemBackground();
        break;
    case Qt::WA_TransparentForMouseEvents:
#ifdef Q_WS_MAC
        d->macUpdateIgnoreMouseEvents();
#endif
        break;
    case Qt::WA_InputMethodEnabled: {
#ifndef QT_NO_IM
        QWidget *focusWidget = d->effectiveFocusWidget();
        QInputContext *ic = focusWidget->d_func()->assignedInputContext();
        if (!ic && (!on || hasFocus()))
            ic = focusWidget->d_func()->inputContext();
        if (ic) {
            if (on && hasFocus() && ic->focusWidget() != focusWidget && isEnabled()
                && focusWidget->testAttribute(Qt::WA_InputMethodEnabled)) {
                ic->setFocusWidget(focusWidget);
            } else if (!on && ic->focusWidget() == focusWidget) {
                ic->reset();
                ic->setFocusWidget(0);
            }
        }
#endif //QT_NO_IM
        break;
    }
    case Qt::WA_WindowPropagation:
        d->resolvePalette();
        d->resolveFont();
        d->resolveLocale();
        break;
#ifdef Q_WS_X11
    case Qt::WA_NoX11EventCompression:
        if (!d->extra)
            d->createExtra();
        d->extra->compress_events = on;
        break;
    case Qt::WA_X11OpenGLOverlay:
        d->updateIsOpaque();
        break;
    case Qt::WA_X11DoNotAcceptFocus:
        if (testAttribute(Qt::WA_WState_Created))
            d->updateX11AcceptFocus();
        break;
#endif
    case Qt::WA_DontShowOnScreen: {
        if (on && isVisible()) {
            // Make sure we keep the current state and only hide the widget
            // from the desktop. show_sys will only update platform specific
            // attributes at this point.
            d->hide_sys();
#ifdef Q_WS_QWS
            // Release the region for this window from qws if the widget has
            // been shown before the attribute was set.
            if (QWSWindowSurface *surface = static_cast<QWSWindowSurface *>(windowSurface())) {
                QWidget::qwsDisplay()->requestRegion(surface->winId(), surface->key(),
                                                     surface->permanentState(), QRegion());
            }
#endif
            d->show_sys();
        }
        break;
    }

#ifdef Q_WS_X11
    case Qt::WA_X11NetWmWindowTypeDesktop:
    case Qt::WA_X11NetWmWindowTypeDock:
    case Qt::WA_X11NetWmWindowTypeToolBar:
    case Qt::WA_X11NetWmWindowTypeMenu:
    case Qt::WA_X11NetWmWindowTypeUtility:
    case Qt::WA_X11NetWmWindowTypeSplash:
    case Qt::WA_X11NetWmWindowTypeDialog:
    case Qt::WA_X11NetWmWindowTypeDropDownMenu:
    case Qt::WA_X11NetWmWindowTypePopupMenu:
    case Qt::WA_X11NetWmWindowTypeToolTip:
    case Qt::WA_X11NetWmWindowTypeNotification:
    case Qt::WA_X11NetWmWindowTypeCombo:
    case Qt::WA_X11NetWmWindowTypeDND:
        if (testAttribute(Qt::WA_WState_Created))
            d->setNetWmWindowTypes();
        break;
#endif

    case Qt::WA_StaticContents:
        if (QWidgetBackingStore *bs = d->maybeBackingStore()) {
            if (on)
                bs->addStaticWidget(this);
            else
                bs->removeStaticWidget(this);
        }
        break;
    case Qt::WA_TranslucentBackground:
#if defined(Q_OS_SYMBIAN)
        setAttribute(Qt::WA_NoSystemBackground, on);
#else
        if (on) {
            setAttribute(Qt::WA_NoSystemBackground);
            d->updateIsTranslucent();
        }
#endif
        break;
    case Qt::WA_AcceptTouchEvents:
#if defined(Q_WS_WIN) || defined(Q_WS_MAC) || defined(Q_OS_SYMBIAN)
        if (on)
            d->registerTouchWindow();
#endif
        break;
    case Qt::WA_LockPortraitOrientation:
    case Qt::WA_LockLandscapeOrientation:
    case Qt::WA_AutoOrientation: {
        const Qt::WidgetAttribute orientations[3] = {
            Qt::WA_LockPortraitOrientation,
            Qt::WA_LockLandscapeOrientation,
            Qt::WA_AutoOrientation
        };

        if (on) {
            // We can only have one of these set at a time
            for (int i = 0; i < 3; ++i) {
                if (orientations[i] != attribute)
                    setAttribute_internal(orientations[i], false, data, d);
            }
        }

#ifdef Q_WS_S60
        CAknAppUiBase* appUi = static_cast<CAknAppUiBase*>(CEikonEnv::Static()->EikAppUi());
        const CAknAppUiBase::TAppUiOrientation s60orientations[] = {
            CAknAppUiBase::EAppUiOrientationPortrait,
            CAknAppUiBase::EAppUiOrientationLandscape,
            CAknAppUiBase::EAppUiOrientationAutomatic
        };
        CAknAppUiBase::TAppUiOrientation s60orientation = CAknAppUiBase::EAppUiOrientationUnspecified;
        for (int i = 0; i < 3; ++i) {
            if (testAttribute(orientations[i])) {
                s60orientation = s60orientations[i];
                break;
            }
        }
        QT_TRAP_THROWING(appUi->SetOrientationL(s60orientation));
        S60->orientationSet = true;
        QSymbianControl *window = static_cast<QSymbianControl *>(internalWinId());
        if (window)
            window->ensureFixNativeOrientation();
#endif
        break;
    }
    default:
        break;
    }
}

/*! \fn bool QWidget::testAttribute(Qt::WidgetAttribute attribute) const

  Returns true if attribute \a attribute is set on this widget;
  otherwise returns false.

  \sa setAttribute()
 */
bool QWidget::testAttribute_helper(Qt::WidgetAttribute attribute) const
{
    Q_D(const QWidget);
    const int x = attribute - 8*sizeof(uint);
    const int int_off = x / (8*sizeof(uint));
    return (d->high_attributes[int_off] & (1<<(x-(int_off*8*sizeof(uint)))));
}

/*!
  \property QWidget::windowOpacity

  \brief The level of opacity for the window.

  The valid range of opacity is from 1.0 (completely opaque) to
  0.0 (completely transparent).

  By default the value of this property is 1.0.

  This feature is available on Embedded Linux, Mac OS X, Windows,
  and X11 platforms that support the Composite extension.

  This feature is not available on Windows CE.

  Note that under X11 you need to have a composite manager running,
  and the X11 specific _NET_WM_WINDOW_OPACITY atom needs to be
  supported by the window manager you are using.

  \warning Changing this property from opaque to transparent might issue a
  paint event that needs to be processed before the window is displayed
  correctly. This affects mainly the use of QPixmap::grabWindow(). Also note
  that semi-transparent windows update and resize significantly slower than
  opaque windows.

  \sa setMask()
*/
qreal QWidget::windowOpacity() const
{
    Q_D(const QWidget);
    return (isWindow() && d->maybeTopData()) ? d->maybeTopData()->opacity / qreal(255.) : qreal(1.0);
}

void QWidget::setWindowOpacity(qreal opacity)
{
    Q_D(QWidget);
    if (!isWindow())
        return;

    opacity = qBound(qreal(0.0), opacity, qreal(1.0));
    QTLWExtra *extra = d->topData();
    extra->opacity = uint(opacity * 255);
    setAttribute(Qt::WA_WState_WindowOpacitySet);

#ifndef Q_WS_QWS
    if (!testAttribute(Qt::WA_WState_Created))
        return;
#endif

#ifndef QT_NO_GRAPHICSVIEW
    if (QGraphicsProxyWidget *proxy = graphicsProxyWidget()) {
        // Avoid invalidating the cache if set.
        if (proxy->cacheMode() == QGraphicsItem::NoCache)
            proxy->update();
        else if (QGraphicsScene *scene = proxy->scene())
            scene->update(proxy->sceneBoundingRect());
        return;
    }
#endif

    d->setWindowOpacity_sys(opacity);
}

/*!
    \property QWidget::windowModified
    \brief whether the document shown in the window has unsaved changes

    A modified window is a window whose content has changed but has
    not been saved to disk. This flag will have different effects
    varied by the platform. On Mac OS X the close button will have a
    modified look; on other platforms, the window title will have an
    '*' (asterisk).

    The window title must contain a "[*]" placeholder, which
    indicates where the '*' should appear. Normally, it should appear
    right after the file name (e.g., "document1.txt[*] - Text
    Editor"). If the window isn't modified, the placeholder is simply
    removed.

    Note that if a widget is set as modified, all its ancestors will
    also be set as modified. However, if you call \c
    {setWindowModified(false)} on a widget, this will not propagate to
    its parent because other children of the parent might have been
    modified.

    \sa windowTitle, {Application Example}, {SDI Example}, {MDI Example}
*/
bool QWidget::isWindowModified() const
{
    return testAttribute(Qt::WA_WindowModified);
}

void QWidget::setWindowModified(bool mod)
{
    Q_D(QWidget);
    setAttribute(Qt::WA_WindowModified, mod);

#ifndef Q_WS_MAC
    if (!windowTitle().contains(QLatin1String("[*]")) && mod)
        qWarning("QWidget::setWindowModified: The window title does not contain a '[*]' placeholder");
#endif
    d->setWindowTitle_helper(windowTitle());
    d->setWindowIconText_helper(windowIconText());
#ifdef Q_WS_MAC
    d->setWindowModified_sys(mod);
#endif

    QEvent e(QEvent::ModifiedChange);
    QApplication::sendEvent(this, &e);
}

#ifndef QT_NO_TOOLTIP
/*!
  \property QWidget::toolTip

  \brief the widget's tooltip

  Note that by default tooltips are only shown for widgets that are
  children of the active window. You can change this behavior by
  setting the attribute Qt::WA_AlwaysShowToolTips on the \e window,
  not on the widget with the tooltip.

  If you want to control a tooltip's behavior, you can intercept the
  event() function and catch the QEvent::ToolTip event (e.g., if you
  want to customize the area for which the tooltip should be shown).

  By default, this property contains an empty string.

  \sa QToolTip statusTip whatsThis
*/
void QWidget::setToolTip(const QString &s)
{
    Q_D(QWidget);
    d->toolTip = s;

    QEvent event(QEvent::ToolTipChange);
    QApplication::sendEvent(this, &event);
}

QString QWidget::toolTip() const
{
    Q_D(const QWidget);
    return d->toolTip;
}
#endif // QT_NO_TOOLTIP


#ifndef QT_NO_STATUSTIP
/*!
  \property QWidget::statusTip
  \brief the widget's status tip

  By default, this property contains an empty string.

  \sa toolTip whatsThis
*/
void QWidget::setStatusTip(const QString &s)
{
    Q_D(QWidget);
    d->statusTip = s;
}

QString QWidget::statusTip() const
{
    Q_D(const QWidget);
    return d->statusTip;
}
#endif // QT_NO_STATUSTIP

#ifndef QT_NO_WHATSTHIS
/*!
  \property QWidget::whatsThis

  \brief the widget's What's This help text.

  By default, this property contains an empty string.

  \sa QWhatsThis QWidget::toolTip QWidget::statusTip
*/
void QWidget::setWhatsThis(const QString &s)
{
    Q_D(QWidget);
    d->whatsThis = s;
}

QString QWidget::whatsThis() const
{
    Q_D(const QWidget);
    return d->whatsThis;
}
#endif // QT_NO_WHATSTHIS

#ifndef QT_NO_ACCESSIBILITY
/*!
  \property QWidget::accessibleName

  \brief the widget's name as seen by assistive technologies

  This property is used by accessible clients to identify, find, or announce
  the widget for accessible clients.

  By default, this property contains an empty string.

  \sa QAccessibleInterface::text()
*/
void QWidget::setAccessibleName(const QString &name)
{
    Q_D(QWidget);
    d->accessibleName = name;
    QAccessible::updateAccessibility(this, 0, QAccessible::NameChanged);
}

QString QWidget::accessibleName() const
{
    Q_D(const QWidget);
    return d->accessibleName;
}

/*!
  \property QWidget::accessibleDescription

  \brief the widget's description as seen by assistive technologies

  By default, this property contains an empty string.

  \sa QAccessibleInterface::text()
*/
void QWidget::setAccessibleDescription(const QString &description)
{
    Q_D(QWidget);
    d->accessibleDescription = description;
    QAccessible::updateAccessibility(this, 0, QAccessible::DescriptionChanged);
}

QString QWidget::accessibleDescription() const
{
    Q_D(const QWidget);
    return d->accessibleDescription;
}
#endif // QT_NO_ACCESSIBILITY

#ifndef QT_NO_SHORTCUT
/*!
    Adds a shortcut to Qt's shortcut system that watches for the given
    \a key sequence in the given \a context. If the \a context is
    Qt::ApplicationShortcut, the shortcut applies to the application as a
    whole. Otherwise, it is either local to this widget, Qt::WidgetShortcut,
    or to the window itself, Qt::WindowShortcut.

    If the same \a key sequence has been grabbed by several widgets,
    when the \a key sequence occurs a QEvent::Shortcut event is sent
    to all the widgets to which it applies in a non-deterministic
    order, but with the ``ambiguous'' flag set to true.

    \warning You should not normally need to use this function;
    instead create \l{QAction}s with the shortcut key sequences you
    require (if you also want equivalent menu options and toolbar
    buttons), or create \l{QShortcut}s if you just need key sequences.
    Both QAction and QShortcut handle all the event filtering for you,
    and provide signals which are triggered when the user triggers the
    key sequence, so are much easier to use than this low-level
    function.

    \sa releaseShortcut() setShortcutEnabled()
*/
int QWidget::grabShortcut(const QKeySequence &key, Qt::ShortcutContext context)
{
    Q_ASSERT(qApp);
    if (key.isEmpty())
        return 0;
    setAttribute(Qt::WA_GrabbedShortcut);
    return qApp->d_func()->shortcutMap.addShortcut(this, key, context);
}

/*!
    Removes the shortcut with the given \a id from Qt's shortcut
    system. The widget will no longer receive QEvent::Shortcut events
    for the shortcut's key sequence (unless it has other shortcuts
    with the same key sequence).

    \warning You should not normally need to use this function since
    Qt's shortcut system removes shortcuts automatically when their
    parent widget is destroyed. It is best to use QAction or
    QShortcut to handle shortcuts, since they are easier to use than
    this low-level function. Note also that this is an expensive
    operation.

    \sa grabShortcut() setShortcutEnabled()
*/
void QWidget::releaseShortcut(int id)
{
    Q_ASSERT(qApp);
    if (id)
        qApp->d_func()->shortcutMap.removeShortcut(id, this, 0);
}

/*!
    If \a enable is true, the shortcut with the given \a id is
    enabled; otherwise the shortcut is disabled.

    \warning You should not normally need to use this function since
    Qt's shortcut system enables/disables shortcuts automatically as
    widgets become hidden/visible and gain or lose focus. It is best
    to use QAction or QShortcut to handle shortcuts, since they are
    easier to use than this low-level function.

    \sa grabShortcut() releaseShortcut()
*/
void QWidget::setShortcutEnabled(int id, bool enable)
{
    Q_ASSERT(qApp);
    if (id)
        qApp->d_func()->shortcutMap.setShortcutEnabled(enable, id, this, 0);
}

/*!
    \since 4.2

    If \a enable is true, auto repeat of the shortcut with the
    given \a id is enabled; otherwise it is disabled.

    \sa grabShortcut() releaseShortcut()
*/
void QWidget::setShortcutAutoRepeat(int id, bool enable)
{
    Q_ASSERT(qApp);
    if (id)
        qApp->d_func()->shortcutMap.setShortcutAutoRepeat(enable, id, this, 0);
}
#endif // QT_NO_SHORTCUT
/*!
    Updates the widget's micro focus.

    \sa QInputContext
*/
void QWidget::updateMicroFocus()
{
#if !defined(QT_NO_IM) && (defined(Q_WS_X11) || defined(Q_WS_QWS) || defined(Q_OS_SYMBIAN))
    Q_D(QWidget);
    // and optimization to update input context only it has already been created.
    if (d->assignedInputContext() || qApp->d_func()->inputContext) {
        QInputContext *ic = inputContext();
        if (ic)
            ic->update();
    }
#endif
#ifndef QT_NO_ACCESSIBILITY
    if (isVisible()) {
        // ##### is this correct
        QAccessible::updateAccessibility(this, 0, QAccessible::StateChanged);
    }
#endif
}


#if defined (Q_WS_WIN)
/*!
    Returns the window system handle of the widget, for low-level
    access. Using this function is not portable.

    An HDC acquired with getDC() has to be released with releaseDC().

    \warning Using this function is not portable.
*/
HDC QWidget::getDC() const
{
    Q_D(const QWidget);
    if (d->hd)
        return (HDC) d->hd;
    return GetDC(winId());
}

/*!
    Releases the HDC \a hdc acquired by a previous call to getDC().

    \warning Using this function is not portable.
*/
void QWidget::releaseDC(HDC hdc) const
{
    Q_D(const QWidget);
    // If its the widgets own dc, it will be released elsewhere. If
    // its a different HDC we release it and issue a warning if it
    // fails.
    if (hdc != d->hd && !ReleaseDC(winId(), hdc))
        qErrnoWarning("QWidget::releaseDC(): failed to release HDC");
}
#else
/*!
    Returns the window system handle of the widget, for low-level
    access. Using this function is not portable.

    The HANDLE type varies with platform; see \c qwindowdefs.h for
    details.
*/
Qt::HANDLE QWidget::handle() const
{
    Q_D(const QWidget);
    if (!internalWinId() && testAttribute(Qt::WA_WState_Created))
        (void)winId(); // enforce native window
    return d->hd;
}
#endif


/*!
    Raises this widget to the top of the parent widget's stack.

    After this call the widget will be visually in front of any
    overlapping sibling widgets.

    \note When using activateWindow(), you can call this function to
    ensure that the window is stacked on top.

    \sa lower(), stackUnder()
*/

void QWidget::raise()
{
    Q_D(QWidget);
    if (!isWindow()) {
        QWidget *p = parentWidget();
        const int parentChildCount = p->d_func()->children.size();
        if (parentChildCount < 2)
            return;
        const int from = p->d_func()->children.indexOf(this);
        Q_ASSERT(from >= 0);
        // Do nothing if the widget is already in correct stacking order _and_ created.
        if (from != parentChildCount -1)
            p->d_func()->children.move(from, parentChildCount - 1);
        if (!testAttribute(Qt::WA_WState_Created) && p->testAttribute(Qt::WA_WState_Created))
            create();
        else if (from == parentChildCount - 1)
            return;

        QRegion region(rect());
        d->subtractOpaqueSiblings(region);
        d->invalidateBuffer(region);
    }
    if (testAttribute(Qt::WA_WState_Created))
        d->raise_sys();

    QEvent e(QEvent::ZOrderChange);
    QApplication::sendEvent(this, &e);
}

/*!
    Lowers the widget to the bottom of the parent widget's stack.

    After this call the widget will be visually behind (and therefore
    obscured by) any overlapping sibling widgets.

    \sa raise(), stackUnder()
*/

void QWidget::lower()
{
    Q_D(QWidget);
    if (!isWindow()) {
        QWidget *p = parentWidget();
        const int parentChildCount = p->d_func()->children.size();
        if (parentChildCount < 2)
            return;
        const int from = p->d_func()->children.indexOf(this);
        Q_ASSERT(from >= 0);
        // Do nothing if the widget is already in correct stacking order _and_ created.
        if (from != 0)
            p->d_func()->children.move(from, 0);
        if (!testAttribute(Qt::WA_WState_Created) && p->testAttribute(Qt::WA_WState_Created))
            create();
        else if (from == 0)
            return;
    }
    if (testAttribute(Qt::WA_WState_Created))
        d->lower_sys();

    QEvent e(QEvent::ZOrderChange);
    QApplication::sendEvent(this, &e);
}


/*!
    Places the widget under \a w in the parent widget's stack.

    To make this work, the widget itself and \a w must be siblings.

    \sa raise(), lower()
*/
void QWidget::stackUnder(QWidget* w)
{
    Q_D(QWidget);
    QWidget *p = parentWidget();
    if (!w || isWindow() || p != w->parentWidget() || this == w)
        return;
    if (p) {
        int from = p->d_func()->children.indexOf(this);
        int to = p->d_func()->children.indexOf(w);
        Q_ASSERT(from >= 0);
        Q_ASSERT(to >= 0);
        if (from < to)
            --to;
        // Do nothing if the widget is already in correct stacking order _and_ created.
        if (from != to)
            p->d_func()->children.move(from, to);
        if (!testAttribute(Qt::WA_WState_Created) && p->testAttribute(Qt::WA_WState_Created))
            create();
        else if (from == to)
            return;
    }
    if (testAttribute(Qt::WA_WState_Created))
        d->stackUnder_sys(w);

    QEvent e(QEvent::ZOrderChange);
    QApplication::sendEvent(this, &e);
}

void QWidget::styleChange(QStyle&) { }
void QWidget::enabledChange(bool) { }  // compat
void QWidget::paletteChange(const QPalette &) { }  // compat
void QWidget::fontChange(const QFont &) { }  // compat
void QWidget::windowActivationChange(bool) { }  // compat
void QWidget::languageChange() { }  // compat


/*!
    \enum QWidget::BackgroundOrigin

    \compat

    \value WidgetOrigin
    \value ParentOrigin
    \value WindowOrigin
    \value AncestorOrigin

*/

/*!
    \fn bool QWidget::isVisibleToTLW() const

    Use isVisible() instead.
*/

/*!
    \fn void QWidget::iconify()

    Use showMinimized() instead.
*/

/*!
    \fn void QWidget::constPolish() const

    Use ensurePolished() instead.
*/

/*!
    \fn void QWidget::reparent(QWidget *parent, Qt::WindowFlags f, const QPoint &p, bool showIt)

    Use setParent() to change the parent or the widget's widget flags;
    use move() to move the widget, and use show() to show the widget.
*/

/*!
    \fn void QWidget::reparent(QWidget *parent, const QPoint &p, bool showIt)

    Use setParent() to change the parent; use move() to move the
    widget, and use show() to show the widget.
*/

/*!
    \fn void QWidget::recreate(QWidget *parent, Qt::WindowFlags f, const QPoint & p, bool showIt)

    Use setParent() to change the parent or the widget's widget flags;
    use move() to move the widget, and use show() to show the widget.
*/

/*!
    \fn bool QWidget::hasMouse() const

    Use testAttribute(Qt::WA_UnderMouse) instead.
*/

/*!
    \fn bool QWidget::ownCursor() const

    Use testAttribute(Qt::WA_SetCursor) instead.
*/

/*!
    \fn bool QWidget::ownFont() const

    Use testAttribute(Qt::WA_SetFont) instead.
*/

/*!
    \fn void QWidget::unsetFont()

    Use setFont(QFont()) instead.
*/

/*!
    \fn bool QWidget::ownPalette() const

    Use testAttribute(Qt::WA_SetPalette) instead.
*/

/*!
    \fn void QWidget::unsetPalette()

    Use setPalette(QPalette()) instead.
*/

/*!
    \fn void QWidget::setEraseColor(const QColor &color)

    Use the palette instead.

    \oldcode
    widget->setEraseColor(color);
    \newcode
    QPalette palette;
    palette.setColor(widget->backgroundRole(), color);
    widget->setPalette(palette);
    \endcode
*/

/*!
    \fn void QWidget::setErasePixmap(const QPixmap &pixmap)

    Use the palette instead.

    \oldcode
    widget->setErasePixmap(pixmap);
    \newcode
    QPalette palette;
    palette.setBrush(widget->backgroundRole(), QBrush(pixmap));
    widget->setPalette(palette);
    \endcode
*/

/*!
    \fn void QWidget::setPaletteForegroundColor(const QColor &color)

    Use the palette directly.

    \oldcode
    widget->setPaletteForegroundColor(color);
    \newcode
    QPalette palette;
    palette.setColor(widget->foregroundRole(), color);
    widget->setPalette(palette);
    \endcode
*/

/*!
    \fn void QWidget::setPaletteBackgroundColor(const QColor &color)

    Use the palette directly.

    \oldcode
    widget->setPaletteBackgroundColor(color);
    \newcode
    QPalette palette;
    palette.setColor(widget->backgroundRole(), color);
    widget->setPalette(palette);
    \endcode
*/

/*!
    \fn void QWidget::setPaletteBackgroundPixmap(const QPixmap &pixmap)

    Use the palette directly.

    \oldcode
    widget->setPaletteBackgroundPixmap(pixmap);
    \newcode
    QPalette palette;
    palette.setBrush(widget->backgroundRole(), QBrush(pixmap));
    widget->setPalette(palette);
    \endcode
*/

/*!
    \fn void QWidget::setBackgroundPixmap(const QPixmap &pixmap)

    Use the palette instead.

    \oldcode
    widget->setBackgroundPixmap(pixmap);
    \newcode
    QPalette palette;
    palette.setBrush(widget->backgroundRole(), QBrush(pixmap));
    widget->setPalette(palette);
    \endcode
*/

/*!
    \fn void QWidget::setBackgroundColor(const QColor &color)

    Use the palette instead.

    \oldcode
    widget->setBackgroundColor(color);
    \newcode
    QPalette palette;
    palette.setColor(widget->backgroundRole(), color);
    widget->setPalette(palette);
    \endcode
*/

/*!
    \fn QColorGroup QWidget::colorGroup() const

    Use QColorGroup(palette()) instead.
*/

/*!
    \fn QWidget *QWidget::parentWidget(bool sameWindow) const

    Use the no-argument overload instead.
*/

/*!
    \fn void QWidget::setKeyCompression(bool b)

    Use setAttribute(Qt::WA_KeyCompression, b) instead.
*/

/*!
    \fn void QWidget::setFont(const QFont &f, bool b)

    Use the single-argument overload instead.
*/

/*!
    \fn void QWidget::setPalette(const QPalette &p, bool b)

    Use the single-argument overload instead.
*/

/*!
    \fn void QWidget::setBackgroundOrigin(BackgroundOrigin background)

    \obsolete
*/

/*!
    \fn BackgroundOrigin QWidget::backgroundOrigin() const

    \obsolete

    Always returns \c WindowOrigin.
*/

/*!
    \fn QPoint QWidget::backgroundOffset() const

    \obsolete

    Always returns QPoint().
*/

/*!
    \fn void QWidget::repaint(bool b)

    The boolean parameter \a b is ignored. Use the no-argument overload instead.
*/

/*!
    \fn void QWidget::repaint(int x, int y, int w, int h, bool b)

    The boolean parameter \a b is ignored. Use the four-argument overload instead.
*/

/*!
    \fn void QWidget::repaint(const QRect &r, bool b)

    The boolean parameter \a b is ignored. Use the single rect-argument overload instead.
*/

/*!
    \fn void QWidget::repaint(const QRegion &rgn, bool b)

    The boolean parameter \a b is ignored. Use the single region-argument overload instead.
*/

/*!
    \fn void QWidget::erase()

    Drawing may only take place in a QPaintEvent. Overload
    paintEvent() to do your erasing and call update() to schedule a
    replaint whenever necessary. See also QPainter.
*/

/*!
    \fn void QWidget::erase(int x, int y, int w, int h)

    Drawing may only take place in a QPaintEvent. Overload
    paintEvent() to do your erasing and call update() to schedule a
    replaint whenever necessary. See also QPainter.
*/

/*!
    \fn void QWidget::erase(const QRect &rect)

    Drawing may only take place in a QPaintEvent. Overload
    paintEvent() to do your erasing and call update() to schedule a
    replaint whenever necessary. See also QPainter.
*/

/*!
    \fn void QWidget::drawText(const QPoint &p, const QString &s)

    Drawing may only take place in a QPaintEvent. Overload
    paintEvent() to do your drawing and call update() to schedule a
    replaint whenever necessary. See also QPainter.
*/

/*!
    \fn void QWidget::drawText(int x, int y, const QString &s)

    Drawing may only take place in a QPaintEvent. Overload
    paintEvent() to do your drawing and call update() to schedule a
    replaint whenever necessary. See also QPainter.
*/

/*!
    \fn QWidget *QWidget::childAt(const QPoint &p, bool includeThis) const

    Use the single point argument overload instead.
*/

/*!
    \fn void QWidget::setCaption(const QString &c)

    Use setWindowTitle() instead.
*/

/*!
    \fn void QWidget::setIcon(const QPixmap &i)

    Use setWindowIcon() instead.
*/

/*!
    \fn void QWidget::setIconText(const QString &it)

    Use setWindowIconText() instead.
*/

/*!
    \fn QString QWidget::caption() const

    Use windowTitle() instead.
*/

/*!
    \fn QString QWidget::iconText() const

    Use windowIconText() instead.
*/

/*!
    \fn bool QWidget::isTopLevel() const
    \obsolete

    Use isWindow() instead.
*/

/*!
    \fn bool QWidget::isRightToLeft() const
    \internal
*/

/*!
    \fn bool QWidget::isLeftToRight() const
    \internal
*/

/*!
    \fn void QWidget::setInputMethodEnabled(bool enabled)

    Use setAttribute(Qt::WA_InputMethodEnabled, \a enabled) instead.
*/

/*!
    \fn bool QWidget::isInputMethodEnabled() const

    Use testAttribute(Qt::WA_InputMethodEnabled) instead.
*/

/*!
    \fn void QWidget::setActiveWindow()

    Use activateWindow() instead.
*/

/*!
    \fn bool QWidget::isShown() const

    Use !isHidden() instead (notice the exclamation mark), or use isVisible() to check whether the widget is visible.
*/

/*!
    \fn bool QWidget::isDialog() const

    Use windowType() == Qt::Dialog instead.
*/

/*!
    \fn bool QWidget::isPopup() const

    Use windowType() == Qt::Popup instead.
*/

/*!
    \fn bool QWidget::isDesktop() const

    Use windowType() == Qt::Desktop instead.
*/

/*!
    \fn void QWidget::polish()

    Use ensurePolished() instead.
*/

/*!
    \fn QWidget *QWidget::childAt(int x, int y, bool includeThis) const

    Use the childAt() overload that doesn't have an \a includeThis parameter.

    \oldcode
        return widget->childAt(x, y, true);
    \newcode
        QWidget *child = widget->childAt(x, y, true);
        if (child)
            return child;
        if (widget->rect().contains(x, y))
            return widget;
    \endcode
*/

/*!
    \fn void QWidget::setSizePolicy(QSizePolicy::Policy hor, QSizePolicy::Policy ver, bool hfw)
    \compat

    Use the \l sizePolicy property and heightForWidth() function instead.
*/

/*!
    \fn bool QWidget::isUpdatesEnabled() const
    \compat

    Use the \l updatesEnabled property instead.
*/

/*!
     \macro QWIDGETSIZE_MAX
     \relates QWidget

     Defines the maximum size for a QWidget object.

     The largest allowed size for a widget is QSize(QWIDGETSIZE_MAX,
     QWIDGETSIZE_MAX), i.e. QSize (16777215,16777215).

     \sa QWidget::setMaximumSize()
*/

/*!
    \fn QWidget::setupUi(QWidget *widget)

    Sets up the user interface for the specified \a widget.

    \note This function is available with widgets that derive from user
    interface descriptions created using \l{uic}.

    \sa {Using a Designer UI File in Your Application}
*/

QRect QWidgetPrivate::frameStrut() const
{
    Q_Q(const QWidget);
    if (!q->isWindow() || (q->windowType() == Qt::Desktop) || q->testAttribute(Qt::WA_DontShowOnScreen)) {
        // x2 = x1 + w - 1, so w/h = 1
        return QRect(0, 0, 1, 1);
    }

    if (data.fstrut_dirty
#ifndef Q_WS_WIN
        // ### Fix properly for 4.3
        && q->isVisible()
#endif
        && q->testAttribute(Qt::WA_WState_Created))
        const_cast<QWidgetPrivate *>(this)->updateFrameStrut();

    return maybeTopData() ? maybeTopData()->frameStrut : QRect();
}

#ifdef QT_KEYPAD_NAVIGATION
/*!
    \internal

    Changes the focus  from the current focusWidget to a widget in
    the \a direction.

    Returns true, if there was a widget in that direction
*/
bool QWidgetPrivate::navigateToDirection(Direction direction)
{
    QWidget *targetWidget = widgetInNavigationDirection(direction);
    if (targetWidget)
        targetWidget->setFocus();
    return (targetWidget != 0);
}

/*!
    \internal

    Searches for a widget that is positioned in the \a direction, starting
    from the current focusWidget.

    Returns the pointer to a found widget or 0, if there was no widget in
    that direction.
*/
QWidget *QWidgetPrivate::widgetInNavigationDirection(Direction direction)
{
    const QWidget *sourceWidget = QApplication::focusWidget();
    if (!sourceWidget)
        return 0;
    const QRect sourceRect = sourceWidget->rect().translated(sourceWidget->mapToGlobal(QPoint()));
    const int sourceX =
            (direction == DirectionNorth || direction == DirectionSouth) ?
                (sourceRect.left() + (sourceRect.right() - sourceRect.left()) / 2)
                :(direction == DirectionEast ? sourceRect.right() : sourceRect.left());
    const int sourceY =
            (direction == DirectionEast || direction == DirectionWest) ?
                (sourceRect.top() + (sourceRect.bottom() - sourceRect.top()) / 2)
                :(direction == DirectionSouth ? sourceRect.bottom() : sourceRect.top());
    const QPoint sourcePoint(sourceX, sourceY);
    const QPoint sourceCenter = sourceRect.center();
    const QWidget *sourceWindow = sourceWidget->window();

    QWidget *targetWidget = 0;
    int shortestDistance = INT_MAX;
    foreach(QWidget *targetCandidate, QApplication::allWidgets()) {

        const QRect targetCandidateRect = targetCandidate->rect().translated(targetCandidate->mapToGlobal(QPoint()));

        // For focus proxies, the child widget handling the focus can have keypad navigation focus,
        // but the owner of the proxy cannot.
        // Additionally, empty widgets should be ignored.
        if (targetCandidate->focusProxy() || targetCandidateRect.isEmpty())
            continue;

        // Only navigate to a target widget that...
        if (       targetCandidate != sourceWidget
                   // ...takes the focus,
                && targetCandidate->focusPolicy() & Qt::TabFocus
                   // ...is above if DirectionNorth,
                && !(direction == DirectionNorth && targetCandidateRect.bottom() > sourceRect.top())
                   // ...is on the right if DirectionEast,
                && !(direction == DirectionEast  && targetCandidateRect.left()   < sourceRect.right())
                   // ...is below if DirectionSouth,
                && !(direction == DirectionSouth && targetCandidateRect.top()    < sourceRect.bottom())
                   // ...is on the left if DirectionWest,
                && !(direction == DirectionWest  && targetCandidateRect.right()  > sourceRect.left())
                   // ...is enabled,
                && targetCandidate->isEnabled()
                   // ...is visible,
                && targetCandidate->isVisible()
                   // ...is in the same window,
                && targetCandidate->window() == sourceWindow) {
            const int targetCandidateDistance = pointToRect(sourcePoint, targetCandidateRect);
            if (targetCandidateDistance < shortestDistance) {
                shortestDistance = targetCandidateDistance;
                targetWidget = targetCandidate;
            }
        }
    }
    return targetWidget;
}

/*!
    \internal

    Tells us if it there is currently a reachable widget by keypad navigation in
    a certain \a orientation.
    If no navigation is possible, occurring key events in that \a orientation may
    be used to interact with the value in the focused widget, even though it
    currently has not the editFocus.

    \sa QWidgetPrivate::widgetInNavigationDirection(), QWidget::hasEditFocus()
*/
bool QWidgetPrivate::canKeypadNavigate(Qt::Orientation orientation)
{
    return orientation == Qt::Horizontal?
            (QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionEast)
                    || QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionWest))
            :(QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionNorth)
                    || QWidgetPrivate::widgetInNavigationDirection(QWidgetPrivate::DirectionSouth));
}
/*!
    \internal

    Checks, if the \a widget is inside a QTabWidget. If is is inside
    one, left/right key events will be used to switch between tabs in keypad
    navigation. If there is no QTabWidget, the horizontal key events can be used
to
    interact with the value in the focused widget, even though it currently has
    not the editFocus.

    \sa QWidget::hasEditFocus()
*/
bool QWidgetPrivate::inTabWidget(QWidget *widget)
{
    for (QWidget *tabWidget = widget; tabWidget; tabWidget = tabWidget->parentWidget())
        if (qobject_cast<const QTabWidget*>(tabWidget))
            return true;
    return false;
}
#endif

/*!
    \preliminary
    \since 4.2
    \obsolete

    Sets the window surface to be the \a surface specified.
    The QWidget takes will ownership of the \a surface.
    widget itself is deleted.
*/
void QWidget::setWindowSurface(QWindowSurface *surface)
{
    // ### createWinId() ??

#ifndef Q_BACKINGSTORE_SUBSURFACES
    if (!isTopLevel())
        return;
#endif

    Q_D(QWidget);

    QTLWExtra *topData = d->topData();
    if (topData->windowSurface == surface)
        return;

    QWindowSurface *oldSurface = topData->windowSurface;
    delete topData->windowSurface;
    topData->windowSurface = surface;

    QWidgetBackingStore *bs = d->maybeBackingStore();
    if (!bs)
        return;

    if (isTopLevel()) {
        if (bs->windowSurface != oldSurface && bs->windowSurface != surface)
            delete bs->windowSurface;
        bs->windowSurface = surface;
    }
#ifdef Q_BACKINGSTORE_SUBSURFACES
    else {
        bs->subSurfaces.append(surface);
    }
    bs->subSurfaces.removeOne(oldSurface);
#endif
}

/*!
    \preliminary
    \since 4.2

    Returns the QWindowSurface this widget will be drawn into.
*/
QWindowSurface *QWidget::windowSurface() const
{
    Q_D(const QWidget);
    QTLWExtra *extra = d->maybeTopData();
    if (extra && extra->windowSurface)
        return extra->windowSurface;

    QWidgetBackingStore *bs = d->maybeBackingStore();

#ifdef Q_BACKINGSTORE_SUBSURFACES
    if (bs && bs->subSurfaces.isEmpty())
        return bs->windowSurface;

    if (!isTopLevel()) {
        const QWidget *w = parentWidget();
        while (w) {
            QTLWExtra *extra = w->d_func()->maybeTopData();
            if (extra && extra->windowSurface)
                return extra->windowSurface;
            if (w->isTopLevel())
                break;
            w = w->parentWidget();
        }
    }
#endif // Q_BACKINGSTORE_SUBSURFACES

    return bs ? bs->windowSurface : 0;
}

void QWidgetPrivate::getLayoutItemMargins(int *left, int *top, int *right, int *bottom) const
{
    if (left)
        *left = (int)leftLayoutItemMargin;
    if (top)
        *top = (int)topLayoutItemMargin;
    if (right)
        *right = (int)rightLayoutItemMargin;
    if (bottom)
        *bottom = (int)bottomLayoutItemMargin;
}

void QWidgetPrivate::setLayoutItemMargins(int left, int top, int right, int bottom)
{
    if (leftLayoutItemMargin == left
        && topLayoutItemMargin == top
        && rightLayoutItemMargin == right
        && bottomLayoutItemMargin == bottom)
        return;

    Q_Q(QWidget);
    leftLayoutItemMargin = (signed char)left;
    topLayoutItemMargin = (signed char)top;
    rightLayoutItemMargin = (signed char)right;
    bottomLayoutItemMargin = (signed char)bottom;
    q->updateGeometry();
}

void QWidgetPrivate::setLayoutItemMargins(QStyle::SubElement element, const QStyleOption *opt)
{
    Q_Q(QWidget);
    QStyleOption myOpt;
    if (!opt) {
        myOpt.initFrom(q);
        myOpt.rect.setRect(0, 0, 32768, 32768);     // arbitrary
        opt = &myOpt;
    }

    QRect liRect = q->style()->subElementRect(element, opt, q);
    if (liRect.isValid()) {
        leftLayoutItemMargin = (signed char)(opt->rect.left() - liRect.left());
        topLayoutItemMargin = (signed char)(opt->rect.top() - liRect.top());
        rightLayoutItemMargin = (signed char)(liRect.right() - opt->rect.right());
        bottomLayoutItemMargin = (signed char)(liRect.bottom() - opt->rect.bottom());
    } else {
        leftLayoutItemMargin = 0;
        topLayoutItemMargin = 0;
        rightLayoutItemMargin = 0;
        bottomLayoutItemMargin = 0;
    }
}
// resets the Qt::WA_QuitOnClose attribute to the default value for transient widgets.
void QWidgetPrivate::adjustQuitOnCloseAttribute()
{
    Q_Q(QWidget);

    if (!q->parentWidget()) {
        Qt::WindowType type = q->windowType();
        if (type == Qt::Widget || type == Qt::SubWindow)
            type = Qt::Window;
        if (type != Qt::Widget && type != Qt::Window && type != Qt::Dialog)
            q->setAttribute(Qt::WA_QuitOnClose, false);
    }
}



Q_GUI_EXPORT QWidgetData *qt_qwidget_data(QWidget *widget)
{
    return widget->data;
}

Q_GUI_EXPORT QWidgetPrivate *qt_widget_private(QWidget *widget)
{
    return widget->d_func();
}


#ifndef QT_NO_GRAPHICSVIEW
/*!
   \since 4.5

   Returns the proxy widget for the corresponding embedded widget in a graphics
   view; otherwise returns 0.

   \sa QGraphicsProxyWidget::createProxyForChildWidget(),
       QGraphicsScene::addWidget()
 */
QGraphicsProxyWidget *QWidget::graphicsProxyWidget() const
{
    Q_D(const QWidget);
    if (d->extra) {
        return d->extra->proxyWidget;
    }
    return 0;
}
#endif


/*!
    \typedef QWidgetList
    \relates QWidget

    Synonym for QList<QWidget *>.
*/

#ifndef QT_NO_GESTURES
/*!
    Subscribes the widget to a given \a gesture with specific \a flags.

    \sa ungrabGesture(), QGestureEvent
    \since 4.6
*/
void QWidget::grabGesture(Qt::GestureType gesture, Qt::GestureFlags flags)
{
    Q_D(QWidget);
    d->gestureContext.insert(gesture, flags);
    (void)QGestureManager::instance(); // create a gesture manager
}

/*!
    Unsubscribes the widget from a given \a gesture type

    \sa grabGesture(), QGestureEvent
    \since 4.6
*/
void QWidget::ungrabGesture(Qt::GestureType gesture)
{
    Q_D(QWidget);
    if (d->gestureContext.remove(gesture)) {
        if (QGestureManager *manager = QGestureManager::instance())
            manager->cleanupCachedGestures(this, gesture);
    }
}
#endif // QT_NO_GESTURES

/*!
    \typedef WId
    \relates QWidget

    Platform dependent window identifier.
*/

/*!
    \fn void QWidget::destroy(bool destroyWindow, bool destroySubWindows)

    Frees up window system resources. Destroys the widget window if \a
    destroyWindow is true.

    destroy() calls itself recursively for all the child widgets,
    passing \a destroySubWindows for the \a destroyWindow parameter.
    To have more control over destruction of subwidgets, destroy
    subwidgets selectively first.

    This function is usually called from the QWidget destructor.
*/

/*!
    \fn QPaintEngine *QWidget::paintEngine() const

    Returns the widget's paint engine.

    Note that this function should not be called explicitly by the
    user, since it's meant for reimplementation purposes only. The
    function is called by Qt internally, and the default
    implementation may not always return a valid pointer.
*/

/*!
    \fn QPoint QWidget::mapToGlobal(const QPoint &pos) const

    Translates the widget coordinate \a pos to global screen
    coordinates. For example, \c{mapToGlobal(QPoint(0,0))} would give
    the global coordinates of the top-left pixel of the widget.

    \sa mapFromGlobal() mapTo() mapToParent()
*/

/*!
    \fn QPoint QWidget::mapFromGlobal(const QPoint &pos) const

    Translates the global screen coordinate \a pos to widget
    coordinates.

    \sa mapToGlobal() mapFrom() mapFromParent()
*/

/*!
    \fn void QWidget::grabMouse()

    Grabs the mouse input.

    This widget receives all mouse events until releaseMouse() is
    called; other widgets get no mouse events at all. Keyboard
    events are not affected. Use grabKeyboard() if you want to grab
    that.

    \warning Bugs in mouse-grabbing applications very often lock the
    terminal. Use this function with extreme caution, and consider
    using the \c -nograb command line option while debugging.

    It is almost never necessary to grab the mouse when using Qt, as
    Qt grabs and releases it sensibly. In particular, Qt grabs the
    mouse when a mouse button is pressed and keeps it until the last
    button is released.

    \note Only visible widgets can grab mouse input. If isVisible()
    returns false for a widget, that widget cannot call grabMouse().

    \note \bold{(Mac OS X developers)} For \e Cocoa, calling
    grabMouse() on a widget only works when the mouse is inside the
    frame of that widget.  For \e Carbon, it works outside the widget's
    frame as well, like for Windows and X11.

    \sa releaseMouse() grabKeyboard() releaseKeyboard()
*/

/*!
    \fn void QWidget::grabMouse(const QCursor &cursor)
    \overload grabMouse()

    Grabs the mouse input and changes the cursor shape.

    The cursor will assume shape \a cursor (for as long as the mouse
    focus is grabbed) and this widget will be the only one to receive
    mouse events until releaseMouse() is called().

    \warning Grabbing the mouse might lock the terminal.

    \note \bold{(Mac OS X developers)} See the note in QWidget::grabMouse().

    \sa releaseMouse(), grabKeyboard(), releaseKeyboard(), setCursor()
*/

/*!
    \fn void QWidget::releaseMouse()

    Releases the mouse grab.

    \sa grabMouse(), grabKeyboard(), releaseKeyboard()
*/

/*!
    \fn void QWidget::grabKeyboard()

    Grabs the keyboard input.

    This widget receives all keyboard events until releaseKeyboard()
    is called; other widgets get no keyboard events at all. Mouse
    events are not affected. Use grabMouse() if you want to grab that.

    The focus widget is not affected, except that it doesn't receive
    any keyboard events. setFocus() moves the focus as usual, but the
    new focus widget receives keyboard events only after
    releaseKeyboard() is called.

    If a different widget is currently grabbing keyboard input, that
    widget's grab is released first.

    \sa releaseKeyboard() grabMouse() releaseMouse() focusWidget()
*/

/*!
    \fn void QWidget::releaseKeyboard()

    Releases the keyboard grab.

    \sa grabKeyboard(), grabMouse(), releaseMouse()
*/

/*!
    \fn QWidget *QWidget::mouseGrabber()

    Returns the widget that is currently grabbing the mouse input.

    If no widget in this application is currently grabbing the mouse,
    0 is returned.

    \sa grabMouse(), keyboardGrabber()
*/

/*!
    \fn QWidget *QWidget::keyboardGrabber()

    Returns the widget that is currently grabbing the keyboard input.

    If no widget in this application is currently grabbing the
    keyboard, 0 is returned.

    \sa grabMouse(), mouseGrabber()
*/

/*!
    \fn void QWidget::activateWindow()

    Sets the top-level widget containing this widget to be the active
    window.

    An active window is a visible top-level window that has the
    keyboard input focus.

    This function performs the same operation as clicking the mouse on
    the title bar of a top-level window. On X11, the result depends on
    the Window Manager. If you want to ensure that the window is
    stacked on top as well you should also call raise(). Note that the
    window must be visible, otherwise activateWindow() has no effect.

    On Windows, if you are calling this when the application is not
    currently the active one then it will not make it the active
    window.  It will change the color of the taskbar entry to indicate
    that the window has changed in some way. This is because Microsoft
    does not allow an application to interrupt what the user is currently
    doing in another application.

    \sa isActiveWindow(), window(), show()
*/

/*!
    \fn int QWidget::metric(PaintDeviceMetric m) const

    Internal implementation of the virtual QPaintDevice::metric()
    function.

    \a m is the metric to get.
*/

/*!
    \fn void QWidget::setMask(const QRegion &region)
    \overload

    Causes only the parts of the widget which overlap \a region to be
    visible. If the region includes pixels outside the rect() of the
    widget, window system controls in that area may or may not be
    visible, depending on the platform.

    Note that this effect can be slow if the region is particularly
    complex.

    \sa windowOpacity
*/
void QWidget::setMask(const QRegion &newMask)
{
    Q_D(QWidget);

    d->createExtra();
    if (newMask == d->extra->mask)
        return;

#ifndef QT_NO_BACKINGSTORE
    const QRegion oldMask(d->extra->mask);
#endif

    d->extra->mask = newMask;
    d->extra->hasMask = !newMask.isEmpty();

#ifndef QT_MAC_USE_COCOA
    if (!testAttribute(Qt::WA_WState_Created))
        return;
#endif

    d->setMask_sys(newMask);

#ifndef QT_NO_BACKINGSTORE
    if (!isVisible())
        return;

    if (!d->extra->hasMask) {
        // Mask was cleared; update newly exposed area.
        QRegion expose(rect());
        expose -= oldMask;
        if (!expose.isEmpty()) {
            d->setDirtyOpaqueRegion();
            update(expose);
        }
        return;
    }

    if (!isWindow()) {
        // Update newly exposed area on the parent widget.
        QRegion parentExpose(rect());
        parentExpose -= newMask;
        if (!parentExpose.isEmpty()) {
            d->setDirtyOpaqueRegion();
            parentExpose.translate(data->crect.topLeft());
            parentWidget()->update(parentExpose);
        }

        // Update newly exposed area on this widget
        if (!oldMask.isEmpty())
            update(newMask - oldMask);
    }
#endif
}

/*!
    \fn void QWidget::setMask(const QBitmap &bitmap)

    Causes only the pixels of the widget for which \a bitmap has a
    corresponding 1 bit to be visible. If the region includes pixels
    outside the rect() of the widget, window system controls in that
    area may or may not be visible, depending on the platform.

    Note that this effect can be slow if the region is particularly
    complex.

    The following code shows how an image with an alpha channel can be
    used to generate a mask for a widget:

    \snippet doc/src/snippets/widget-mask/main.cpp 0

    The label shown by this code is masked using the image it contains,
    giving the appearance that an irregularly-shaped image is being drawn
    directly onto the screen.

    Masked widgets receive mouse events only on their visible
    portions.

    \sa clearMask(), windowOpacity(), {Shaped Clock Example}
*/
void QWidget::setMask(const QBitmap &bitmap)
{
    setMask(QRegion(bitmap));
}

/*!
    \fn void QWidget::clearMask()

    Removes any mask set by setMask().

    \sa setMask()
*/
void QWidget::clearMask()
{
    setMask(QRegion());
}

/*! \fn const QX11Info &QWidget::x11Info() const
    Returns information about the configuration of the X display used to display
    the widget.

    \warning This function is only available on X11.
*/

/*! \fn Qt::HANDLE QWidget::x11PictureHandle() const
    Returns the X11 Picture handle of the widget for XRender
    support. Use of this function is not portable. This function will
    return 0 if XRender support is not compiled into Qt, if the
    XRender extension is not supported on the X11 display, or if the
    handle could not be created.
*/

#ifdef Q_OS_SYMBIAN
void QWidgetPrivate::_q_cleanupWinIds()
{
    foreach (WId wid, widCleanupList)
        delete wid;
    widCleanupList.clear();
}
#endif

#if QT_MAC_USE_COCOA
void QWidgetPrivate::syncUnifiedMode() {
    // The whole purpose of this method is to keep the unifiedToolbar in sync.
    // That means making sure we either exchange the drawing methods or we let
    // the toolbar know that it does not require to draw the baseline.
    Q_Q(QWidget);
    // This function makes sense only if this is a top level
    if(!q->isWindow())
        return;
    OSWindowRef window = qt_mac_window_for(q);
    if(changeMethods) {
        // Ok, we are in documentMode.
        if(originalDrawMethod)
            qt_mac_replaceDrawRect(window, this);
    } else {
        if(!originalDrawMethod)
            qt_mac_replaceDrawRectOriginal(window, this);
    }
}

#endif // QT_MAC_USE_COCOA

QT_END_NAMESPACE

#include "moc_qwidget.cpp"


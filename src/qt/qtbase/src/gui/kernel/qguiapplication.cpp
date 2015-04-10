/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qguiapplication.h"

#include "private/qguiapplication_p.h"
#include <qpa/qplatformintegrationfactory_p.h>
#include "private/qevent_p.h"
#include "qfont.h"
#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformwindow.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformtheme.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformdrag.h>

#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QVariant>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/private/qabstracteventdispatcher_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/private/qthread_p.h>
#include <QtCore/qdir.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qnumeric.h>
#include <QtDebug>
#ifndef QT_NO_ACCESSIBILITY
#include "qaccessible.h"
#endif
#include <qpalette.h>
#include <qscreen.h>
#include "qsessionmanager.h"
#include <private/qscreen_p.h>
#include <private/qdrawhelper_p.h>

#include <QtGui/qgenericpluginfactory.h>
#include <QtGui/qstylehints.h>
#include <QtGui/qinputmethod.h>
#include <QtGui/qpixmapcache.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatforminputcontext_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include "private/qwindow_p.h"
#include "private/qcursor_p.h"

#include "private/qdnd_p.h"
#include <qpa/qplatformthemefactory_p.h>

#ifndef QT_NO_CURSOR
#include <qpa/qplatformcursor.h>
#endif

#include <QtGui/QPixmap>

#ifndef QT_NO_CLIPBOARD
#include <QtGui/QClipboard>
#endif

#if defined(Q_OS_MAC)
#  include "private/qcore_mac_p.h"
#elif defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
#  include <QtCore/qt_windows.h>
#  include <QtCore/QLibraryInfo>
#endif // Q_OS_WIN && !Q_OS_WINCE

#include <ctype.h>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT bool qt_is_gui_used = true;

Qt::MouseButtons QGuiApplicationPrivate::mouse_buttons = Qt::NoButton;
Qt::KeyboardModifiers QGuiApplicationPrivate::modifier_buttons = Qt::NoModifier;

QPointF QGuiApplicationPrivate::lastCursorPosition(qInf(), qInf());

bool QGuiApplicationPrivate::tabletState = false;
QWindow *QGuiApplicationPrivate::tabletPressTarget = 0;
QWindow *QGuiApplicationPrivate::currentMouseWindow = 0;

Qt::ApplicationState QGuiApplicationPrivate::applicationState = Qt::ApplicationInactive;

QPlatformIntegration *QGuiApplicationPrivate::platform_integration = 0;
QPlatformTheme *QGuiApplicationPrivate::platform_theme = 0;

QList<QObject *> QGuiApplicationPrivate::generic_plugin_list;

enum ApplicationResourceFlags
{
    ApplicationPaletteExplicitlySet = 0x1,
    ApplicationFontExplicitlySet = 0x2
};

static unsigned applicationResourceFlags = 0;

QIcon *QGuiApplicationPrivate::app_icon = 0;

QString *QGuiApplicationPrivate::platform_name = 0;
QString *QGuiApplicationPrivate::displayName = 0;

QPalette *QGuiApplicationPrivate::app_pal = 0;        // default application palette

Qt::MouseButtons QGuiApplicationPrivate::buttons = Qt::NoButton;
ulong QGuiApplicationPrivate::mousePressTime = 0;
Qt::MouseButton QGuiApplicationPrivate::mousePressButton = Qt::NoButton;
int QGuiApplicationPrivate::mousePressX = 0;
int QGuiApplicationPrivate::mousePressY = 0;
int QGuiApplicationPrivate::mouse_double_click_distance = -1;

static Qt::LayoutDirection layout_direction = Qt::LeftToRight;
static bool force_reverse = false;

QGuiApplicationPrivate *QGuiApplicationPrivate::self = 0;
QTouchDevice *QGuiApplicationPrivate::m_fakeTouchDevice = 0;
int QGuiApplicationPrivate::m_fakeMouseSourcePointId = 0;

#ifndef QT_NO_CLIPBOARD
QClipboard *QGuiApplicationPrivate::qt_clipboard = 0;
#endif

QList<QScreen *> QGuiApplicationPrivate::screen_list;

QWindowList QGuiApplicationPrivate::window_list;
QWindow *QGuiApplicationPrivate::focus_window = 0;

static QBasicMutex applicationFontMutex;
QFont *QGuiApplicationPrivate::app_font = 0;
bool QGuiApplicationPrivate::obey_desktop_settings = true;
bool QGuiApplicationPrivate::noGrab = false;

static qreal fontSmoothingGamma = 1.7;

extern void qRegisterGuiVariant();
extern void qInitDrawhelperAsm();
extern void qInitImageConversions();

static bool qt_detectRTLLanguage()
{
    return force_reverse ^
        (QCoreApplication::tr("QT_LAYOUT_DIRECTION",
                         "Translate this string to the string 'LTR' in left-to-right"
                         " languages or to 'RTL' in right-to-left languages (such as Hebrew"
                         " and Arabic) to get proper widget layout.") == QLatin1String("RTL"));
}

static void initPalette()
{
    if (!QGuiApplicationPrivate::app_pal)
        if (const QPalette *themePalette = QGuiApplicationPrivate::platformTheme()->palette())
            QGuiApplicationPrivate::app_pal = new QPalette(*themePalette);
    if (!QGuiApplicationPrivate::app_pal)
        QGuiApplicationPrivate::app_pal = new QPalette(Qt::gray);
}

static inline void clearPalette()
{
    delete QGuiApplicationPrivate::app_pal;
    QGuiApplicationPrivate::app_pal = 0;
}

static void initFontUnlocked()
{
    if (!QGuiApplicationPrivate::app_font) {
        if (const QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
            if (const QFont *font = theme->font(QPlatformTheme::SystemFont))
                QGuiApplicationPrivate::app_font = new QFont(*font);
    }
    if (!QGuiApplicationPrivate::app_font)
        QGuiApplicationPrivate::app_font =
            new QFont(QGuiApplicationPrivate::platformIntegration()->fontDatabase()->defaultFont());
}

static inline void clearFontUnlocked()
{
    delete QGuiApplicationPrivate::app_font;
    QGuiApplicationPrivate::app_font = 0;
}

static inline bool isPopupWindow(const QWindow *w)
{
    return (w->flags() & Qt::WindowType_Mask) == Qt::Popup;
}

// Geometry specification for top level windows following the convention of the
// -geometry command line arguments in X11 (see XParseGeometry).
struct QWindowGeometrySpecification
{
    QWindowGeometrySpecification() : corner(Qt::TopLeftCorner), xOffset(-1), yOffset(-1), width(-1), height(-1) {}
    static QWindowGeometrySpecification fromArgument(const QByteArray &a);
    QRect apply(const QRect &windowGeometry, const QSize &windowMinimumSize, const QSize &windowMaximumSize, const QRect &availableGeometry) const;
    inline QRect apply(const QRect &windowGeometry, const QWindow *window) const
    { return apply(windowGeometry, window->minimumSize(), window->maximumSize(), window->screen()->virtualGeometry()); }

    Qt::Corner corner;
    int xOffset;
    int yOffset;
    int width;
    int height;
};

// Parse a token of a X11 geometry specification "200x100+10-20".
static inline int nextGeometryToken(const QByteArray &a, int &pos, char *op)
{
    *op = 0;
    const int size = a.size();
    if (pos >= size)
        return -1;

    *op = a.at(pos);
    if (*op == '+' || *op == '-' || *op == 'x')
        pos++;
    else if (isdigit(*op))
        *op = 'x'; // If it starts with a digit, it is supposed to be a width specification.
    else
        return -1;

    const int numberPos = pos;
    for ( ; pos < size && isdigit(a.at(pos)); ++pos) ;

    bool ok;
    const int result = a.mid(numberPos, pos - numberPos).toInt(&ok);
    return ok ? result : -1;
}

QWindowGeometrySpecification QWindowGeometrySpecification::fromArgument(const QByteArray &a)
{
    QWindowGeometrySpecification result;
    int pos = 0;
    for (int i = 0; i < 4; ++i) {
        char op;
        const int value = nextGeometryToken(a, pos, &op);
        if (value < 0)
            break;
        switch (op) {
        case 'x':
            (result.width >= 0 ? result.height : result.width) = value;
            break;
        case '+':
        case '-':
            if (result.xOffset >= 0) {
                result.yOffset = value;
                if (op == '-')
                    result.corner = result.corner == Qt::TopRightCorner ? Qt::BottomRightCorner : Qt::BottomLeftCorner;
            } else {
                result.xOffset = value;
                if (op == '-')
                    result.corner = Qt::TopRightCorner;
            }
        }
    }
    return result;
}

QRect QWindowGeometrySpecification::apply(const QRect &windowGeometry, const QSize &windowMinimumSize, const QSize &windowMaximumSize, const QRect &availableGeometry) const
{
    QRect result = windowGeometry;
    if (width >= 0 || height >= 0) {
        QSize size = windowGeometry.size();
        if (width >= 0)
            size.setWidth(qBound(windowMinimumSize.width(), width, windowMaximumSize.width()));
        if (height >= 0)
            size.setHeight(qBound(windowMinimumSize.height(), height, windowMaximumSize.height()));
        result.setSize(size);
    }
    if (xOffset >= 0 || yOffset >= 0) {
        QPoint topLeft = windowGeometry.topLeft();
        if (xOffset >= 0) {
            topLeft.setX(corner == Qt::TopLeftCorner || corner == Qt::BottomLeftCorner ?
                         xOffset :
                         qMax(availableGeometry.right() - result.width() - xOffset, availableGeometry.left()));
        }
        if (yOffset >= 0) {
            topLeft.setY(corner == Qt::TopLeftCorner || corner == Qt::TopRightCorner ?
                         yOffset :
                         qMax(availableGeometry.bottom() - result.height() - yOffset, availableGeometry.top()));
        }
        result.moveTopLeft(topLeft);
    }
    return result;
}

static QWindowGeometrySpecification windowGeometrySpecification;

/*!
    \class QGuiApplication
    \brief The QGuiApplication class manages the GUI application's control
    flow and main settings.

    \inmodule QtGui
    \since 5.0

    QGuiApplication contains the main event loop, where all events from the window
    system and other sources are processed and dispatched. It also handles the
    application's initialization and finalization, and provides session management.
    In addition, QGuiApplication handles most of the system-wide and application-wide
    settings.

    For any GUI application using Qt, there is precisely \b one QGuiApplication
    object no matter whether the application has 0, 1, 2 or more windows at
    any given time. For non-GUI Qt applications, use QCoreApplication instead,
    as it does not depend on the Qt GUI module. For QWidget based Qt applications,
    use QApplication instead, as it provides some functionality needed for creating
    QWidget instances.

    The QGuiApplication object is accessible through the instance() function, which
    returns a pointer equivalent to the global \l qApp pointer.

    QGuiApplication's main areas of responsibility are:
        \list
            \li  It initializes the application with the user's desktop settings,
                such as palette(), font() and styleHints(). It keeps
                track of these properties in case the user changes the desktop
                globally, for example, through some kind of control panel.

            \li  It performs event handling, meaning that it receives events
                from the underlying window system and dispatches them to the
                relevant widgets. You can send your own events to windows by
                using sendEvent() and postEvent().

            \li  It parses common command line arguments and sets its internal
                state accordingly. See the \l{QGuiApplication::QGuiApplication()}
                {constructor documentation} below for more details.

            \li  It provides localization of strings that are visible to the
                user via translate().

            \li  It provides some magical objects like the clipboard().

            \li  It knows about the application's windows. You can ask which
                window is at a certain position using topLevelAt(), get a list of
                topLevelWindows(), etc.

            \li  It manages the application's mouse cursor handling, see
                setOverrideCursor()

            \li  It provides support for sophisticated \l{Session Management}
                {session management}. This makes it possible for applications
                to terminate gracefully when the user logs out, to cancel a
                shutdown process if termination isn't possible and even to
                preserve the entire application's state for a future session.
                See isSessionRestored(), sessionId() and commitDataRequest() and
                saveStateRequest() for details.
        \endlist

    Since the QGuiApplication object does so much initialization, it \e{must} be
    created before any other objects related to the user interface are created.
    QGuiApplication also deals with common command line arguments. Hence, it is
    usually a good idea to create it \e before any interpretation or
    modification of \c argv is done in the application itself.

    \table
    \header
        \li{2,1} Groups of functions

        \row
        \li  System settings
        \li  desktopSettingsAware(),
            setDesktopSettingsAware(),
            styleHints(),
            palette(),
            setPalette(),
            font(),
            setFont().

        \row
        \li  Event handling
        \li  exec(),
            processEvents(),
            exit(),
            quit().
            sendEvent(),
            postEvent(),
            sendPostedEvents(),
            removePostedEvents(),
            hasPendingEvents(),
            notify().

        \row
        \li  Windows
        \li  allWindows(),
            topLevelWindows(),
            focusWindow(),
            clipboard(),
            topLevelAt().

        \row
        \li  Advanced cursor handling
        \li  overrideCursor(),
            setOverrideCursor(),
            restoreOverrideCursor().

        \row
        \li  Session management
        \li  isSessionRestored(),
            sessionId(),
            commitDataRequest(),
            saveStateRequest().

        \row
        \li  Miscellaneous
        \li  startingUp(),
            closingDown(),
            type().
    \endtable

    \sa QCoreApplication, QAbstractEventDispatcher, QEventLoop
*/

/*!
    Initializes the window system and constructs an application object with
    \a argc command line arguments in \a argv.

    \warning The data referred to by \a argc and \a argv must stay valid for
    the entire lifetime of the QGuiApplication object. In addition, \a argc must
    be greater than zero and \a argv must contain at least one valid character
    string.

    The global \c qApp pointer refers to this application object. Only one
    application object should be created.

    This application object must be constructed before any \l{QPaintDevice}
    {paint devices} (including pixmaps, bitmaps etc.).

    \note \a argc and \a argv might be changed as Qt removes command line
    arguments that it recognizes.

    \section1 Supported Command Line Options

    All Qt programs automatically support the following command line options:
    \list

        \li \c{-platform} \e {platformName[:options]}, specifies the
            \l{Qt Platform Abstraction} (QPA) plugin.

            Overridden by the \c QT_QPA_PLATFORM environment variable.
        \li \c{-platformpluginpath} \e path, specifies the path to platform
            plugins.

            Overridden by the \c QT_QPA_PLATFORM_PLUGIN_PATH environment
            variable.

        \li \c{-platformtheme} \e platformTheme, specifies the platform theme.

            Overridden by the \c QT_QPA_PLATFORMTHEME environment variable.
        \li \c{-qmljsdebugger=}, activates the QML/JS debugger with a specified port.
            The value must be of format \c{port:1234}\e{[,block]}, where
            \e block is optional
            and will make the application wait until a debugger connects to it.
        \li \c {-qwindowgeometry} \e geometry, specifies window geometry for
            the main window using the X11-syntax. For example:
            \c {-qwindowgeometry 100x100+50+50}
        \li \c{-reverse}, sets the application's layout direction to
            Qt::RightToLeft
        \li \c{-session} \e session, restores the application from an earlier
            \l{Session Management}{session}.
        \li  -qwindowgeometry, sets the geometry of the first window
        \li  -qwindowtitle, sets the title of the first window
    \endlist

    The following standard command line options are available for X11:

    \list
        \li \c {-display} \e {hostname:screen_number}, switches displays on X11.
        \li \c {-geometry} \e geometry, same as \c {-qwindowgeometry}.
    \endlist

    \section1 Platform-Specific Arguments

    You can specify platform-specific arguments for the \c{-platform} option.
    Place them after the platform plugin name following a colon as a
    comma-separated list. For example,
    \c{-platform windows:dialogs=xp,fontengine=freetype}.

    The following parameters are available for \c {-platform windows}:

    \list
        \li \c {dialogs=[xp|none]}, \c xp uses XP-style native dialogs and
            \c none disables them.
        \li \c {fontengine=freetype}, uses the FreeType font engine.
    \endlist

    For more information about the platform-specific arguments available for
    embedded Linux platforms, see \l{Qt for Embedded Linux}.

    \sa arguments() QGuiApplication::platformName
*/
#ifdef Q_QDOC
QGuiApplication::QGuiApplication(int &argc, char **argv)
#else
QGuiApplication::QGuiApplication(int &argc, char **argv, int flags)
#endif
    : QCoreApplication(*new QGuiApplicationPrivate(argc, argv, flags))
{
    d_func()->init();

    QCoreApplicationPrivate::eventDispatcher->startingUp();
}

/*!
    \internal
*/
QGuiApplication::QGuiApplication(QGuiApplicationPrivate &p)
    : QCoreApplication(p)
{
    d_func()->init(); }

/*!
    Destructs the application.
*/
QGuiApplication::~QGuiApplication()
{
    Q_D(QGuiApplication);

    d->eventDispatcher->closingDown();
    d->eventDispatcher = 0;

#ifndef QT_NO_CLIPBOARD
    delete QGuiApplicationPrivate::qt_clipboard;
    QGuiApplicationPrivate::qt_clipboard = 0;
#endif

#ifndef QT_NO_SESSIONMANAGER
    delete d->session_manager;
    d->session_manager = 0;
#endif //QT_NO_SESSIONMANAGER

    clearPalette();

#ifndef QT_NO_CURSOR
    d->cursor_list.clear();
#endif

    delete QGuiApplicationPrivate::app_icon;
    QGuiApplicationPrivate::app_icon = 0;
    delete QGuiApplicationPrivate::platform_name;
    QGuiApplicationPrivate::platform_name = 0;
    delete QGuiApplicationPrivate::displayName;
    QGuiApplicationPrivate::displayName = 0;
}

QGuiApplicationPrivate::QGuiApplicationPrivate(int &argc, char **argv, int flags)
    : QCoreApplicationPrivate(argc, argv, flags),
      styleHints(0),
      inputMethod(0),
      lastTouchType(QEvent::TouchEnd)
{
    self = this;
    application_type = QCoreApplicationPrivate::Gui;
#ifndef QT_NO_SESSIONMANAGER
    is_session_restored = false;
    is_saving_session = false;
#endif
}

/*!
    \property QGuiApplication::applicationDisplayName
    \brief the user-visible name of this application
    \since 5.0

    This name is shown to the user, for instance in window titles.
    It can be translated, if necessary.

    If not set, the application display name defaults to the application name.

    \sa applicationName
*/
void QGuiApplication::setApplicationDisplayName(const QString &name)
{
    if (!QGuiApplicationPrivate::displayName)
        QGuiApplicationPrivate::displayName = new QString;
    *QGuiApplicationPrivate::displayName = name;
}

QString QGuiApplication::applicationDisplayName()
{
    return QGuiApplicationPrivate::displayName ? *QGuiApplicationPrivate::displayName : applicationName();
}

/*!
    Returns the most recently shown modal window. If no modal windows are
    visible, this function returns zero.

    A modal window is a window which has its
    \l{QWindow::modality}{modality} property set to Qt::WindowModal
    or Qt::ApplicationModal. A modal window must be closed before the user can
    continue with other parts of the program.

    Modal window are organized in a stack. This function returns the modal
    window at the top of the stack.

    \sa Qt::WindowModality, QWindow::setModality()
*/
QWindow *QGuiApplication::modalWindow()
{
    if (QGuiApplicationPrivate::self->modalWindowList.isEmpty())
        return 0;
    return QGuiApplicationPrivate::self->modalWindowList.first();
}

static void updateBlockedStatusRecursion(QWindow *window, bool shouldBeBlocked)
{
    QWindowPrivate *p = qt_window_private(window);
    if (p->blockedByModalWindow != shouldBeBlocked) {
        p->blockedByModalWindow = shouldBeBlocked;
        QEvent e(shouldBeBlocked ? QEvent::WindowBlocked : QEvent::WindowUnblocked);
        QGuiApplication::sendEvent(window, &e);
        foreach (QObject *c, window->children())
            if (c->isWindowType())
                updateBlockedStatusRecursion(static_cast<QWindow *>(c), shouldBeBlocked);
    }
}

void QGuiApplicationPrivate::updateBlockedStatus(QWindow *window)
{
    bool shouldBeBlocked = false;
    if (!isPopupWindow(window) && !self->modalWindowList.isEmpty())
        shouldBeBlocked = self->isWindowBlocked(window);
    updateBlockedStatusRecursion(window, shouldBeBlocked);
}

void QGuiApplicationPrivate::showModalWindow(QWindow *modal)
{
    self->modalWindowList.prepend(modal);

    // Send leave for currently entered window if it should be blocked
    if (currentMouseWindow && !isPopupWindow(currentMouseWindow)) {
        bool shouldBeBlocked = self->isWindowBlocked(currentMouseWindow);
        if (shouldBeBlocked) {
            // Remove the new window from modalWindowList temporarily so leave can go through
            self->modalWindowList.removeFirst();
            QEvent e(QEvent::Leave);
            QGuiApplication::sendEvent(currentMouseWindow, &e);
            currentMouseWindow = 0;
            self->modalWindowList.prepend(modal);
        }
    }

    QWindowList windows = QGuiApplication::topLevelWindows();
    for (int i = 0; i < windows.count(); ++i) {
        QWindow *window = windows.at(i);
        if (!window->d_func()->blockedByModalWindow)
            updateBlockedStatus(window);
    }

    updateBlockedStatus(modal);
}

void QGuiApplicationPrivate::hideModalWindow(QWindow *window)
{
    self->modalWindowList.removeAll(window);

    QWindowList windows = QGuiApplication::topLevelWindows();
    for (int i = 0; i < windows.count(); ++i) {
        QWindow *window = windows.at(i);
        if (window->d_func()->blockedByModalWindow)
            updateBlockedStatus(window);
    }
}

/*
    Returns \c true if \a window is blocked by a modal window. If \a
    blockingWindow is non-zero, *blockingWindow will be set to the blocking
    window (or to zero if \a window is not blocked).
*/
bool QGuiApplicationPrivate::isWindowBlocked(QWindow *window, QWindow **blockingWindow) const
{
    QWindow *unused = 0;
    if (!blockingWindow)
        blockingWindow = &unused;

    if (modalWindowList.isEmpty()) {
        *blockingWindow = 0;
        return false;
    }

    for (int i = 0; i < modalWindowList.count(); ++i) {
        QWindow *modalWindow = modalWindowList.at(i);

        {
            // check if the modal window is our window or a (transient) parent of our window
            QWindow *w = window;
            while (w) {
                if (w == modalWindow) {
                    *blockingWindow = 0;
                    return false;
                }
                QWindow *p = w->parent();
                if (!p)
                    p = w->transientParent();
                w = p;
            }
        }

        Qt::WindowModality windowModality = modalWindow->modality();
        switch (windowModality) {
        case Qt::ApplicationModal:
        {
            if (modalWindow != window) {
                *blockingWindow = modalWindow;
                return true;
            }
            break;
        }
        case Qt::WindowModal:
        {
            QWindow *w = window;
            do {
                QWindow *m = modalWindow;
                do {
                    if (m == w) {
                        *blockingWindow = m;
                        return true;
                    }
                    QWindow *p = m->parent();
                    if (!p)
                        p = m->transientParent();
                    m = p;
                } while (m);
                QWindow *p = w->parent();
                if (!p)
                    p = w->transientParent();
                w = p;
            } while (w);
            break;
        }
        default:
            Q_ASSERT_X(false, "QGuiApplication", "internal error, a modal widget cannot be modeless");
            break;
        }
    }
    *blockingWindow = 0;
    return false;
}

bool QGuiApplicationPrivate::synthesizeMouseFromTouchEventsEnabled()
{
    return QCoreApplication::testAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents)
            && QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::SynthesizeMouseFromTouchEvents).toBool();
}

/*!
    Returns the QWindow that receives events tied to focus,
    such as key events.
*/
QWindow *QGuiApplication::focusWindow()
{
    return QGuiApplicationPrivate::focus_window;
}

/*!
    \fn QGuiApplication::focusObjectChanged(QObject *focusObject)

    This signal is emitted when final receiver of events tied to focus is changed.
    \a focusObject is the new receiver.

    \sa focusObject()
*/

/*!
    \fn QGuiApplication::focusWindowChanged(QWindow *focusWindow)

    This signal is emitted when the focused window changes.
    \a focusWindow is the new focused window.

    \sa focusWindow()
*/

/*!
    Returns the QObject in currently active window that will be final receiver of events
    tied to focus, such as key events.
 */
QObject *QGuiApplication::focusObject()
{
    if (focusWindow())
        return focusWindow()->focusObject();
    return 0;
}

/*!
    \fn QGuiApplication::allWindows()

    Returns a list of all the windows in the application.

    The list is empty if there are no windows.

    \sa topLevelWindows()
 */
QWindowList QGuiApplication::allWindows()
{
    return QGuiApplicationPrivate::window_list;
}

/*!
    \fn QGuiApplication::topLevelWindows()

    Returns a list of the top-level windows in the application.

    \sa allWindows()
 */
QWindowList QGuiApplication::topLevelWindows()
{
    const QWindowList &list = QGuiApplicationPrivate::window_list;
    QWindowList topLevelWindows;
    for (int i = 0; i < list.size(); i++) {
        if (!list.at(i)->parent() && list.at(i)->type() != Qt::Desktop) {
            // Top windows of embedded QAxServers do not have QWindow parents,
            // but they are not true top level windows, so do not include them.
            const bool embedded = list.at(i)->handle() && list.at(i)->handle()->isEmbedded(0);
            if (!embedded)
                topLevelWindows.prepend(list.at(i));
        }
    }
    return topLevelWindows;
}

/*!
    Returns the primary (or default) screen of the application.

    This will be the screen where QWindows are shown, unless otherwise specified.
*/
QScreen *QGuiApplication::primaryScreen()
{
    if (QGuiApplicationPrivate::screen_list.isEmpty())
        return 0;
    return QGuiApplicationPrivate::screen_list.at(0);
}

/*!
    Returns a list of all the screens associated with the
    windowing system the application is connected to.
*/
QList<QScreen *> QGuiApplication::screens()
{
    return QGuiApplicationPrivate::screen_list;
}

/*!
    \fn void QGuiApplication::screenAdded(QScreen *screen)

    This signal is emitted whenever a new screen \a screen has been added to the system.

    \sa screens(), primaryScreen()
*/


/*!
    Returns the highest screen device pixel ratio found on
    the system. This is the ratio between physical pixels and
    device-independent pixels.

    Use this function only when you don't know which window you are targeting.
    If you do know the target window, use QWindow::devicePixelRatio() instead.

    \sa QWindow::devicePixelRatio()
*/
qreal QGuiApplication::devicePixelRatio() const
{
    // Cache topDevicePixelRatio, iterate through the screen list once only.
    static qreal topDevicePixelRatio = 0.0;
    if (!qFuzzyIsNull(topDevicePixelRatio)) {
        return topDevicePixelRatio;
    }

    topDevicePixelRatio = 1.0; // make sure we never return 0.
    foreach (QScreen *screen, QGuiApplicationPrivate::screen_list) {
        topDevicePixelRatio = qMax(topDevicePixelRatio, screen->devicePixelRatio());
    }

    return topDevicePixelRatio;
}

/*!
    Returns the top level window at the given position \a pos, if any.
*/
QWindow *QGuiApplication::topLevelAt(const QPoint &pos)
{
    QList<QScreen *> screens = QGuiApplication::screens();
    QList<QScreen *>::const_iterator screen = screens.constBegin();
    QList<QScreen *>::const_iterator end = screens.constEnd();

    while (screen != end) {
        if ((*screen)->geometry().contains(pos))
            return (*screen)->handle()->topLevelAt(pos);
        ++screen;
    }
    return 0;
}

/*!
    \property QGuiApplication::platformName
    \brief The name of the underlying platform plugin.

    The QPA platform plugins are located in \c {qtbase\src\plugins\platforms}.
    At the time of writing, the following platform plugin names are supported:

    \list
        \li \c android
        \li \c cocoa is a platform plugin for Mac OS X.
        \li \c directfb
        \li \c eglfs is a platform plugin for running Qt5 applications on top of
            EGL and  OpenGL ES 2.0 without an actual windowing system (like X11
            or Wayland). For more information, see \l{EGLFS}.
        \li \c ios
        \li \c kms is an experimental platform plugin using kernel modesetting
            and \l{http://dri.freedesktop.org/wiki/DRM}{DRM} (Direct Rendering
            Manager).
        \li \c linuxfb writes directly to the framebuffer. For more information,
            see \l{LinuxFB}.
        \li \c minimal is provided as an examples for developers who want to
            write their own platform plugins. However, you can use the plugin to
            run GUI applications in environments without a GUI, such as servers.
        \li \c minimalegl is an example plugin.
        \li \c offscreen
        \li \c openwfd
        \li \c qnx
        \li \c windows
        \li \c xcb is the X11 plugin used on regular desktop Linux platforms.
    \endlist

    For more information about the platform plugins for embedded Linux devices,
    see \l{Qt for Embedded Linux}.
*/

QString QGuiApplication::platformName()
{
    return QGuiApplicationPrivate::platform_name ?
           *QGuiApplicationPrivate::platform_name : QString();
}

static void init_platform(const QString &pluginArgument, const QString &platformPluginPath, const QString &platformThemeName, int &argc, char **argv)
{
    // Split into platform name and arguments
    QStringList arguments = pluginArgument.split(QLatin1Char(':'));
    const QString name = arguments.takeFirst().toLower();
    QString argumentsKey = name;
    argumentsKey[0] = argumentsKey.at(0).toUpper();
    arguments.append(QLibraryInfo::platformPluginArguments(argumentsKey));

   // Create the platform integration.
    QGuiApplicationPrivate::platform_integration = QPlatformIntegrationFactory::create(name, arguments, argc, argv, platformPluginPath);
    if (QGuiApplicationPrivate::platform_integration) {
        QGuiApplicationPrivate::platform_name = new QString(name);
    } else {
        QStringList keys = QPlatformIntegrationFactory::keys(platformPluginPath);

        QString fatalMessage
                = QStringLiteral("This application failed to start because it could not find or load the Qt platform plugin \"%1\".\n\n").arg(name);
        if (!keys.isEmpty()) {
            fatalMessage += QStringLiteral("Available platform plugins are: %1.\n\n").arg(
                        keys.join(QStringLiteral(", ")));
        }
        fatalMessage += QStringLiteral("Reinstalling the application may fix this problem.");
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
        // Windows: Display message box unless it is a console application
        // or debug build showing an assert box.
        if (!QLibraryInfo::isDebugBuild() && !GetConsoleWindow())
            MessageBox(0, (LPCTSTR)fatalMessage.utf16(), (LPCTSTR)(QCoreApplication::applicationName().utf16()), MB_OK | MB_ICONERROR);
#endif // Q_OS_WIN && !Q_OS_WINCE && !Q_OS_WINRT
        qFatal("%s", qPrintable(fatalMessage));
        return;
    }

    // Create the platform theme:

    // 1) Fetch the platform name from the environment if present.
    QStringList themeNames;
    if (!platformThemeName.isEmpty())
        themeNames.append(platformThemeName);

    // 2) Ask the platform integration for a list of theme names
    themeNames += QGuiApplicationPrivate::platform_integration->themeNames();
    // 3) Look for a theme plugin.
    foreach (const QString &themeName, themeNames) {
        QGuiApplicationPrivate::platform_theme = QPlatformThemeFactory::create(themeName, platformPluginPath);
        if (QGuiApplicationPrivate::platform_theme)
            break;
    }

    // 4) If no theme plugin was found ask the platform integration to
    // create a theme
    if (!QGuiApplicationPrivate::platform_theme) {
        foreach (const QString &themeName, themeNames) {
            QGuiApplicationPrivate::platform_theme = QGuiApplicationPrivate::platform_integration->createPlatformTheme(themeName);
            if (QGuiApplicationPrivate::platform_theme)
                break;
        }
        // No error message; not having a theme plugin is allowed.
    }

    // 5) Fall back on the built-in "null" platform theme.
    if (!QGuiApplicationPrivate::platform_theme)
        QGuiApplicationPrivate::platform_theme = new QPlatformTheme;

#ifndef QT_NO_PROPERTIES
    // Set arguments as dynamic properties on the native interface as
    // boolean 'foo' or strings: 'foo=bar'
    if (!arguments.isEmpty()) {
        if (QObject *nativeInterface = QGuiApplicationPrivate::platform_integration->nativeInterface()) {
            foreach (const QString &argument, arguments) {
                const int equalsPos = argument.indexOf(QLatin1Char('='));
                const QByteArray name =
                    equalsPos != -1 ? argument.left(equalsPos).toUtf8() : argument.toUtf8();
                const QVariant value =
                    equalsPos != -1 ? QVariant(argument.mid(equalsPos + 1)) : QVariant(true);
                nativeInterface->setProperty(name.constData(), value);
            }
        }
    }
#endif

    fontSmoothingGamma = QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::FontSmoothingGamma).toReal();
}

static void init_plugins(const QList<QByteArray> &pluginList)
{
    for (int i = 0; i < pluginList.count(); ++i) {
        QByteArray pluginSpec = pluginList.at(i);
        int colonPos = pluginSpec.indexOf(':');
        QObject *plugin;
        if (colonPos < 0)
            plugin = QGenericPluginFactory::create(QLatin1String(pluginSpec), QString());
        else
            plugin = QGenericPluginFactory::create(QLatin1String(pluginSpec.mid(0, colonPos)),
                                                   QLatin1String(pluginSpec.mid(colonPos+1)));
        if (plugin)
            QGuiApplicationPrivate::generic_plugin_list.append(plugin);
    }
}

void QGuiApplicationPrivate::createPlatformIntegration()
{
    // Use the Qt menus by default. Platform plugins that
    // want to enable a native menu implementation can clear
    // this flag.
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);

    // Load the platform integration
    QString platformPluginPath = QLatin1String(qgetenv("QT_QPA_PLATFORM_PLUGIN_PATH"));


    QByteArray platformName;
#ifdef QT_QPA_DEFAULT_PLATFORM_NAME
    platformName = QT_QPA_DEFAULT_PLATFORM_NAME;
#endif
    QByteArray platformNameEnv = qgetenv("QT_QPA_PLATFORM");
    if (!platformNameEnv.isEmpty()) {
        platformName = platformNameEnv;
    }

    QString platformThemeName = QString::fromLocal8Bit(qgetenv("QT_QPA_PLATFORMTHEME"));

    // Get command line params

    int j = argc ? 1 : 0;
    for (int i=1; i<argc; i++) {
        if (argv[i] && *argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        QByteArray arg = argv[i];
        if (arg.startsWith("--"))
            arg.remove(0, 1);
        if (arg == "-platformpluginpath") {
            if (++i < argc)
                platformPluginPath = QLatin1String(argv[i]);
        } else if (arg == "-platform") {
            if (++i < argc)
                platformName = argv[i];
        } else if (arg == "-platformtheme") {
            if (++i < argc)
                platformThemeName = QString::fromLocal8Bit(argv[i]);
        } else if (arg == "-qwindowgeometry" || (platformName == "xcb" && arg == "-geometry")) {
            if (++i < argc)
                windowGeometrySpecification = QWindowGeometrySpecification::fromArgument(argv[i]);
        } else if (arg == "-qwindowtitle" || (platformName == "xcb" && arg == "-title")) {
            if (++i < argc)
                firstWindowTitle = QString::fromLocal8Bit(argv[i]);
        } else {
            argv[j++] = argv[i];
        }
    }

    if (j < argc) {
        argv[j] = 0;
        argc = j;
    }

    init_platform(QLatin1String(platformName), platformPluginPath, platformThemeName, argc, argv);

}

/*!
    Called from QCoreApplication::init()

    Responsible for creating an event dispatcher when QCoreApplication
    decides that it needs one (because a custom one has not been set).
*/
void QGuiApplicationPrivate::createEventDispatcher()
{
    Q_ASSERT(!eventDispatcher);

    if (platform_integration == 0)
        createPlatformIntegration();

    // The platform integration should not mess with the event dispatcher
    Q_ASSERT(!eventDispatcher);

    eventDispatcher = platform_integration->createEventDispatcher();
}

void QGuiApplicationPrivate::eventDispatcherReady()
{
    if (platform_integration == 0)
        createPlatformIntegration();

    platform_integration->initialize();
}

#if defined(QT_DEBUG) && defined(Q_OS_LINUX)
// Find out if our parent process is gdb by looking at the 'exe' symlink under /proc.
static bool runningUnderDebugger()
{
    const QFileInfo parentProcExe(QStringLiteral("/proc/") + QString::number(getppid()) + QStringLiteral("/exe"));
    return parentProcExe.isSymLink() && parentProcExe.symLinkTarget().endsWith(QStringLiteral("/gdb"));
}
#endif

void QGuiApplicationPrivate::init()
{
    QCoreApplicationPrivate::is_app_running = false; // Starting up.

    bool doGrabUnderDebugger = false;
    QList<QByteArray> pluginList;
    // Get command line params
#ifndef QT_NO_SESSIONMANAGER
    QString session_id;
    QString session_key;
#endif
    int j = argc ? 1 : 0;
    for (int i=1; i<argc; i++) {
        if (argv[i] && *argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        QByteArray arg = argv[i];
        if (arg.startsWith("--"))
            arg.remove(0, 1);
        if (arg == "-plugin") {
            if (++i < argc)
                pluginList << argv[i];
        } else if (arg == "-reverse") {
            force_reverse = true;
            QGuiApplication::setLayoutDirection(Qt::RightToLeft);
#ifdef Q_OS_MAC
        } else if (arg.startsWith("-psn_")) {
            // eat "-psn_xxxx" on Mac, which is passed when starting an app from Finder.
            // special hack to change working directory (for an app bundle) when running from finder
            if (QDir::currentPath() == QLatin1String("/")) {
                QCFType<CFURLRef> bundleURL(CFBundleCopyBundleURL(CFBundleGetMainBundle()));
                QString qbundlePath = QCFString(CFURLCopyFileSystemPath(bundleURL,
                                                                        kCFURLPOSIXPathStyle));
                if (qbundlePath.endsWith(QLatin1String(".app")))
                    QDir::setCurrent(qbundlePath.section(QLatin1Char('/'), 0, -2));
            }
#endif
        } else if (arg == "-nograb") {
            QGuiApplicationPrivate::noGrab = true;
        } else if (arg == "-dograb") {
            doGrabUnderDebugger = true;
#ifndef QT_NO_SESSIONMANAGER
        } else if (arg == "-session" && i < argc-1) {
            ++i;
            if (argv[i] && *argv[i]) {
                session_id = QString::fromLatin1(argv[i]);
                int p = session_id.indexOf(QLatin1Char('_'));
                if (p >= 0) {
                    session_key = session_id.mid(p +1);
                    session_id = session_id.left(p);
                }
                is_session_restored = true;
            }
#endif
        } else {
            argv[j++] = argv[i];
        }
    }

    if (j < argc) {
        argv[j] = 0;
        argc = j;
    }

#if defined(QT_DEBUG) && defined(Q_OS_LINUX)
    if (!doGrabUnderDebugger && !QGuiApplicationPrivate::noGrab && runningUnderDebugger()) {
        QGuiApplicationPrivate::noGrab = true;
        qDebug("Qt: gdb: -nograb added to command-line options.\n"
               "\t Use the -dograb option to enforce grabbing.");
    }
#else
    Q_UNUSED(doGrabUnderDebugger)
#endif

    // Load environment exported generic plugins
    foreach (const QByteArray &plugin, qgetenv("QT_QPA_GENERIC_PLUGINS").split(','))
        pluginList << plugin;

    if (platform_integration == 0)
        createPlatformIntegration();

    // Set up which span functions should be used in raster engine...
    qInitDrawhelperAsm();
    // and QImage conversion functions
    qInitImageConversions();

    initPalette();
    QFont::initialize();

    mouse_double_click_distance = platformTheme()->themeHint(QPlatformTheme::MouseDoubleClickDistance).toInt();

#ifndef QT_NO_CURSOR
    QCursorData::initialize();
#endif

    // trigger registering of QVariant's GUI types
    qRegisterGuiVariant();

    QWindowSystemInterfacePrivate::eventTime.start();

    is_app_running = true;
    init_plugins(pluginList);
    QWindowSystemInterface::flushWindowSystemEvents();

#ifndef QT_NO_SESSIONMANAGER
    Q_Q(QGuiApplication);

    // connect to the session manager
    session_manager = new QSessionManager(q, session_id, session_key);
#endif

}

extern void qt_cleanupFontDatabase();

QGuiApplicationPrivate::~QGuiApplicationPrivate()
{
    is_app_closing = true;
    is_app_running = false;

    for (int i = 0; i < generic_plugin_list.count(); ++i)
        delete generic_plugin_list.at(i);
    generic_plugin_list.clear();

    clearFontUnlocked();

    QFont::cleanup();

#ifndef QT_NO_CURSOR
    QCursorData::cleanup();
#endif

    layout_direction = Qt::LeftToRight;

    cleanupThreadData();

    delete styleHints;
    delete inputMethod;

    qt_cleanupFontDatabase();

    QPixmapCache::clear();

    delete platform_theme;
    platform_theme = 0;
    delete platform_integration;
    platform_integration = 0;
    delete m_gammaTables.load();

    window_list.clear();
}

#if 0
#ifndef QT_NO_CURSOR
QCursor *overrideCursor();
void setOverrideCursor(const QCursor &);
void changeOverrideCursor(const QCursor &);
void restoreOverrideCursor();
#endif

static QFont font();
static QFont font(const QWidget*);
static QFont font(const char *className);
static void setFont(const QFont &, const char* className = 0);
static QFontMetrics fontMetrics();

#ifndef QT_NO_CLIPBOARD
static QClipboard *clipboard();
#endif
#endif

/*!
    Returns the current state of the modifier keys on the keyboard. The current
    state is updated sychronously as the event queue is emptied of events that
    will spontaneously change the keyboard state (QEvent::KeyPress and
    QEvent::KeyRelease events).

    It should be noted this may not reflect the actual keys held on the input
    device at the time of calling but rather the modifiers as last reported in
    one of the above events. If no keys are being held Qt::NoModifier is
    returned.

    \sa mouseButtons(), queryKeyboardModifiers()
*/
Qt::KeyboardModifiers QGuiApplication::keyboardModifiers()
{
    return QGuiApplicationPrivate::modifier_buttons;
}

/*!
    \fn Qt::KeyboardModifiers QGuiApplication::queryKeyboardModifiers()

    Queries and returns the state of the modifier keys on the keyboard.
    Unlike keyboardModifiers, this method returns the actual keys held
    on the input device at the time of calling the method.

    It does not rely on the keypress events having been received by this
    process, which makes it possible to check the modifiers while moving
    a window, for instance. Note that in most cases, you should use
    keyboardModifiers(), which is faster and more accurate since it contains
    the state of the modifiers as they were when the currently processed
    event was received.

    \sa keyboardModifiers()
*/
Qt::KeyboardModifiers QGuiApplication::queryKeyboardModifiers()
{
    QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    return pi->queryKeyboardModifiers();
}

/*!
    Returns the current state of the buttons on the mouse. The current state is
    updated syncronously as the event queue is emptied of events that will
    spontaneously change the mouse state (QEvent::MouseButtonPress and
    QEvent::MouseButtonRelease events).

    It should be noted this may not reflect the actual buttons held on the
    input device at the time of calling but rather the mouse buttons as last
    reported in one of the above events. If no mouse buttons are being held
    Qt::NoButton is returned.

    \sa keyboardModifiers()
*/
Qt::MouseButtons QGuiApplication::mouseButtons()
{
    return QGuiApplicationPrivate::mouse_buttons;
}

/*!
    Returns the platform's native interface, for platform specific
    functionality.
*/
QPlatformNativeInterface *QGuiApplication::platformNativeInterface()
{
    QPlatformIntegration *pi = QGuiApplicationPrivate::platformIntegration();
    return pi ? pi->nativeInterface() : 0;
}

/*!
    Enters the main event loop and waits until exit() is called, and then
    returns the value that was set to exit() (which is 0 if exit() is called
    via quit()).

    It is necessary to call this function to start event handling. The main
    event loop receives events from the window system and dispatches these to
    the application widgets.

    Generally, no user interaction can take place before calling exec().

    To make your application perform idle processing, e.g., executing a special
    function whenever there are no pending events, use a QTimer with 0 timeout.
    More advanced idle processing schemes can be achieved using processEvents().

    We recommend that you connect clean-up code to the
    \l{QCoreApplication::}{aboutToQuit()} signal, instead of putting it in your
    application's \c{main()} function. This is because, on some platforms, the
    QApplication::exec() call may not return.

    \sa quitOnLastWindowClosed, quit(), exit(), processEvents(),
        QCoreApplication::exec()
*/
int QGuiApplication::exec()
{
#ifndef QT_NO_ACCESSIBILITY
    QAccessible::setRootObject(qApp);
#endif
    return QCoreApplication::exec();
}

/*! \reimp
*/
bool QGuiApplication::notify(QObject *object, QEvent *event)
{
#ifndef QT_NO_SHORTCUT
    if (event->type() == QEvent::KeyPress) {
        // Try looking for a Shortcut before sending key events
        QWindow *w = qobject_cast<QWindow *>(object);
        QObject *focus = w ? w->focusObject() : 0;
        if (!focus)
            focus = object;
        if (QGuiApplicationPrivate::instance()->shortcutMap.tryShortcutEvent(focus, static_cast<QKeyEvent *>(event)))
            return true;
    }
#endif

    if (object->isWindowType())
        QGuiApplicationPrivate::sendQWindowEventToQPlatformWindow(static_cast<QWindow *>(object), event);
    return QCoreApplication::notify(object, event);
}

/*! \reimp
*/
bool QGuiApplication::event(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange) {
        setLayoutDirection(qt_detectRTLLanguage()?Qt::RightToLeft:Qt::LeftToRight);
    }
    return QCoreApplication::event(e);
}

/*!
    \internal
*/
bool QGuiApplication::compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents)
{
    return QCoreApplication::compressEvent(event, receiver, postedEvents);
}

void QGuiApplicationPrivate::sendQWindowEventToQPlatformWindow(QWindow *window, QEvent *event)
{
    if (!window)
        return;
    QPlatformWindow *platformWindow = window->handle();
    if (!platformWindow)
        return;
    // spontaneous events come from the platform integration already, we don't need to send the events back
    if (event->spontaneous())
        return;
    // let the platform window do any handling it needs to as well
    platformWindow->windowEvent(event);
}

bool QGuiApplicationPrivate::processNativeEvent(QWindow *window, const QByteArray &eventType, void *message, long *result)
{
    return window->nativeEvent(eventType, message, result);
}

void QGuiApplicationPrivate::processWindowSystemEvent(QWindowSystemInterfacePrivate::WindowSystemEvent *e)
{
    switch(e->type) {
    case QWindowSystemInterfacePrivate::FrameStrutMouse:
    case QWindowSystemInterfacePrivate::Mouse:
        QGuiApplicationPrivate::processMouseEvent(static_cast<QWindowSystemInterfacePrivate::MouseEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Wheel:
        QGuiApplicationPrivate::processWheelEvent(static_cast<QWindowSystemInterfacePrivate::WheelEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Key:
        QGuiApplicationPrivate::processKeyEvent(static_cast<QWindowSystemInterfacePrivate::KeyEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Touch:
        QGuiApplicationPrivate::processTouchEvent(static_cast<QWindowSystemInterfacePrivate::TouchEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::GeometryChange:
        QGuiApplicationPrivate::processGeometryChangeEvent(static_cast<QWindowSystemInterfacePrivate::GeometryChangeEvent*>(e));
        break;
    case QWindowSystemInterfacePrivate::Enter:
        QGuiApplicationPrivate::processEnterEvent(static_cast<QWindowSystemInterfacePrivate::EnterEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Leave:
        QGuiApplicationPrivate::processLeaveEvent(static_cast<QWindowSystemInterfacePrivate::LeaveEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ActivatedWindow:
        QGuiApplicationPrivate::processActivatedEvent(static_cast<QWindowSystemInterfacePrivate::ActivatedWindowEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::WindowStateChanged:
        QGuiApplicationPrivate::processWindowStateChangedEvent(static_cast<QWindowSystemInterfacePrivate::WindowStateChangedEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::WindowScreenChanged:
        QGuiApplicationPrivate::processWindowScreenChangedEvent(static_cast<QWindowSystemInterfacePrivate::WindowScreenChangedEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ApplicationStateChanged:
        QGuiApplicationPrivate::setApplicationState(static_cast<QWindowSystemInterfacePrivate::ApplicationStateChangedEvent *>(e)->newState);
        break;
    case QWindowSystemInterfacePrivate::FlushEvents:
        QWindowSystemInterface::deferredFlushWindowSystemEvents();
        break;
    case QWindowSystemInterfacePrivate::Close:
        QGuiApplicationPrivate::processCloseEvent(
                static_cast<QWindowSystemInterfacePrivate::CloseEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenOrientation:
        QGuiApplicationPrivate::reportScreenOrientationChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenOrientationEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenGeometry:
        QGuiApplicationPrivate::reportGeometryChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenGeometryEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenAvailableGeometry:
        QGuiApplicationPrivate::reportAvailableGeometryChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInch:
        QGuiApplicationPrivate::reportLogicalDotsPerInchChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ScreenRefreshRate:
        QGuiApplicationPrivate::reportRefreshRateChange(
                static_cast<QWindowSystemInterfacePrivate::ScreenRefreshRateEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::ThemeChange:
        QGuiApplicationPrivate::processThemeChanged(
                    static_cast<QWindowSystemInterfacePrivate::ThemeChangeEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Expose:
        QGuiApplicationPrivate::processExposeEvent(static_cast<QWindowSystemInterfacePrivate::ExposeEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::Tablet:
        QGuiApplicationPrivate::processTabletEvent(
                    static_cast<QWindowSystemInterfacePrivate::TabletEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::TabletEnterProximity:
        QGuiApplicationPrivate::processTabletEnterProximityEvent(
                    static_cast<QWindowSystemInterfacePrivate::TabletEnterProximityEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::TabletLeaveProximity:
        QGuiApplicationPrivate::processTabletLeaveProximityEvent(
                    static_cast<QWindowSystemInterfacePrivate::TabletLeaveProximityEvent *>(e));
        break;
#ifndef QT_NO_GESTURES
    case QWindowSystemInterfacePrivate::Gesture:
        QGuiApplicationPrivate::processGestureEvent(
                    static_cast<QWindowSystemInterfacePrivate::GestureEvent *>(e));
        break;
#endif
    case QWindowSystemInterfacePrivate::PlatformPanel:
        QGuiApplicationPrivate::processPlatformPanelEvent(
                    static_cast<QWindowSystemInterfacePrivate::PlatformPanelEvent *>(e));
        break;
    case QWindowSystemInterfacePrivate::FileOpen:
        QGuiApplicationPrivate::processFileOpenEvent(
                    static_cast<QWindowSystemInterfacePrivate::FileOpenEvent *>(e));
        break;
#ifndef QT_NO_CONTEXTMENU
        case QWindowSystemInterfacePrivate::ContextMenu:
        QGuiApplicationPrivate::processContextMenuEvent(
                    static_cast<QWindowSystemInterfacePrivate::ContextMenuEvent *>(e));
        break;
#endif
    case QWindowSystemInterfacePrivate::EnterWhatsThisMode:
        QGuiApplication::postEvent(QGuiApplication::instance(), new QEvent(QEvent::EnterWhatsThisMode));
        break;
    default:
        qWarning() << "Unknown user input event type:" << e->type;
        break;
    }
}

void QGuiApplicationPrivate::processMouseEvent(QWindowSystemInterfacePrivate::MouseEvent *e)
{
    QEvent::Type type;
    Qt::MouseButtons stateChange = e->buttons ^ buttons;
    if (e->globalPos != QGuiApplicationPrivate::lastCursorPosition && (stateChange != Qt::NoButton)) {
        // A mouse event should not change both position and buttons at the same time. Instead we
        // should first send a move event followed by a button changed event. Since this is not the case
        // with the current event, we fake a move-only event that we recurse and process first. This
        // will update the global mouse position and cause the second event to be a button only event.
        QWindowSystemInterfacePrivate::MouseEvent moveEvent(e->window.data(),
                e->timestamp, e->type, e->localPos, e->globalPos, buttons, e->modifiers);
        processMouseEvent(&moveEvent);
        Q_ASSERT(e->globalPos == QGuiApplicationPrivate::lastCursorPosition);
        // continue with processing mouse button change event
    }

    QWindow *window = e->window.data();
    modifier_buttons = e->modifiers;

    QPointF localPoint = e->localPos;
    QPointF globalPoint = e->globalPos;

    if (e->nullWindow) {
        window = QGuiApplication::topLevelAt(globalPoint.toPoint());
        if (window) {
            QPointF delta = globalPoint - globalPoint.toPoint();
            localPoint = window->mapFromGlobal(globalPoint.toPoint()) + delta;
        }
    }

    Qt::MouseButton button = Qt::NoButton;
    bool doubleClick = false;
    const bool frameStrut = e->type == QWindowSystemInterfacePrivate::FrameStrutMouse;

    if (QGuiApplicationPrivate::lastCursorPosition != globalPoint) {
        type = frameStrut ? QEvent::NonClientAreaMouseMove : QEvent::MouseMove;
        QGuiApplicationPrivate::lastCursorPosition = globalPoint;
        if (qAbs(globalPoint.x() - mousePressX) > mouse_double_click_distance||
            qAbs(globalPoint.y() - mousePressY) > mouse_double_click_distance)
            mousePressButton = Qt::NoButton;
    } else { // Check to see if a new button has been pressed/released.
        for (int check = Qt::LeftButton;
            check <= int(Qt::MaxMouseButton);
             check = check << 1) {
            if (check & stateChange) {
                button = Qt::MouseButton(check);
                break;
            }
        }
        if (button == Qt::NoButton) {
            // Ignore mouse events that don't change the current state.
            return;
        }
        mouse_buttons = buttons = e->buttons;
        if (button & e->buttons) {
            ulong doubleClickInterval = static_cast<ulong>(qApp->styleHints()->mouseDoubleClickInterval());
            doubleClick = e->timestamp - mousePressTime < doubleClickInterval && button == mousePressButton;
            type = frameStrut ? QEvent::NonClientAreaMouseButtonPress : QEvent::MouseButtonPress;
            mousePressTime = e->timestamp;
            mousePressButton = button;
            const QPoint point = QGuiApplicationPrivate::lastCursorPosition.toPoint();
            mousePressX = point.x();
            mousePressY = point.y();
        } else {
            type = frameStrut ? QEvent::NonClientAreaMouseButtonRelease : QEvent::MouseButtonRelease;
        }
    }

    if (!window)
        return;

    QMouseEvent ev(type, localPoint, localPoint, globalPoint, button, buttons, e->modifiers);
    ev.setTimestamp(e->timestamp);
    setMouseEventSource(&ev, e->source);
#ifndef QT_NO_CURSOR
    if (!e->synthetic) {
        if (const QScreen *screen = window->screen())
            if (QPlatformCursor *cursor = screen->handle()->cursor())
                cursor->pointerEvent(ev);
    }
#endif

    if (window->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow mouse events through
        return;
    }

    if (doubleClick && (ev.type() == QEvent::MouseButtonPress)) {
        // QtBUG-25831, used to suppress delivery in qwidgetwindow.cpp
        setMouseEventFlags(&ev, ev.flags() | Qt::MouseEventCreatedDoubleClick);
    }

    QGuiApplication::sendSpontaneousEvent(window, &ev);
    if (!e->synthetic && !ev.isAccepted()
        && !frameStrut
        && qApp->testAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents)) {
        if (!m_fakeTouchDevice) {
            m_fakeTouchDevice = new QTouchDevice;
            QWindowSystemInterface::registerTouchDevice(m_fakeTouchDevice);
        }
        QList<QWindowSystemInterface::TouchPoint> points;
        QWindowSystemInterface::TouchPoint point;
        point.id = 1;
        point.area = QRectF(globalPoint.x() - 2, globalPoint.y() - 2, 4, 4);

        // only translate left button related events to
        // avoid strange touch event sequences when several
        // buttons are pressed
        if (type == QEvent::MouseButtonPress && button == Qt::LeftButton) {
            point.state = Qt::TouchPointPressed;
        } else if (type == QEvent::MouseButtonRelease && button == Qt::LeftButton) {
            point.state = Qt::TouchPointReleased;
        } else if (type == QEvent::MouseMove && (buttons & Qt::LeftButton)) {
            point.state = Qt::TouchPointMoved;
        } else {
            return;
        }

        points << point;

        QEvent::Type type;
        QList<QTouchEvent::TouchPoint> touchPoints = QWindowSystemInterfacePrivate::convertTouchPoints(points, &type);

        QWindowSystemInterfacePrivate::TouchEvent fake(window, e->timestamp, type, m_fakeTouchDevice, touchPoints, e->modifiers);
        fake.synthetic = true;
        processTouchEvent(&fake);
    }
    if (doubleClick) {
        mousePressButton = Qt::NoButton;
        if (!e->window.isNull()) { // QTBUG-36364, check if window closed in response to press
            const QEvent::Type doubleClickType = frameStrut ? QEvent::NonClientAreaMouseButtonDblClick : QEvent::MouseButtonDblClick;
            QMouseEvent dblClickEvent(doubleClickType, localPoint, localPoint, globalPoint,
                                      button, buttons, e->modifiers);
            dblClickEvent.setTimestamp(e->timestamp);
            setMouseEventSource(&dblClickEvent, e->source);
            QGuiApplication::sendSpontaneousEvent(window, &dblClickEvent);
        }
    }
}

void QGuiApplicationPrivate::processWheelEvent(QWindowSystemInterfacePrivate::WheelEvent *e)
{
#ifndef QT_NO_WHEELEVENT
    QWindow *window = e->window.data();
    QPointF globalPoint = e->globalPos;
    QPointF localPoint = e->localPos;

    if (e->nullWindow) {
        window = QGuiApplication::topLevelAt(globalPoint.toPoint());
        if (window) {
            QPointF delta = globalPoint - globalPoint.toPoint();
            localPoint = window->mapFromGlobal(globalPoint.toPoint()) + delta;
        }
    }

    if (!window)
        return;

    QGuiApplicationPrivate::lastCursorPosition = globalPoint;
    modifier_buttons = e->modifiers;

    if (window->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow wheel events through
        return;
    }

     QWheelEvent ev(localPoint, globalPoint, e->pixelDelta, e->angleDelta, e->qt4Delta, e->qt4Orientation, buttons, e->modifiers, e->phase);
     ev.setTimestamp(e->timestamp);
     QGuiApplication::sendSpontaneousEvent(window, &ev);
#endif /* ifndef QT_NO_WHEELEVENT */
}

// Remember, Qt convention is:  keyboard state is state *before*

void QGuiApplicationPrivate::processKeyEvent(QWindowSystemInterfacePrivate::KeyEvent *e)
{
    QWindow *window = e->window.data();
    modifier_buttons = e->modifiers;
    if (e->nullWindow
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
           || e->key == Qt::Key_Back || e->key == Qt::Key_Menu
#endif
            ) {
        window = QGuiApplication::focusWindow();
    }

    QKeyEvent ev(e->keyType, e->key, e->modifiers,
                 e->nativeScanCode, e->nativeVirtualKey, e->nativeModifiers,
                 e->unicode, e->repeat, e->repeatCount);
    ev.setTimestamp(e->timestamp);

    // only deliver key events when we have a window, and no modal window is blocking this window

    if (window && !window->d_func()->blockedByModalWindow)
        QGuiApplication::sendSpontaneousEvent(window, &ev);
#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
    else
        ev.setAccepted(false);

    static bool backKeyPressAccepted = false;
    if (e->keyType == QEvent::KeyPress) {
        backKeyPressAccepted = e->key == Qt::Key_Back && ev.isAccepted();
    } else if (e->keyType == QEvent::KeyRelease && e->key == Qt::Key_Back && !backKeyPressAccepted && !ev.isAccepted()) {
        if (!window)
            qApp->quit();
        else
            QWindowSystemInterface::handleCloseEvent(window);
    }
#endif
}

void QGuiApplicationPrivate::processEnterEvent(QWindowSystemInterfacePrivate::EnterEvent *e)
{
    if (!e->enter)
        return;
    if (e->enter.data()->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow enter events through
        return;
    }

    currentMouseWindow = e->enter;

    QEnterEvent event(e->localPos, e->localPos, e->globalPos);
    QCoreApplication::sendSpontaneousEvent(e->enter.data(), &event);
}

void QGuiApplicationPrivate::processLeaveEvent(QWindowSystemInterfacePrivate::LeaveEvent *e)
{
    if (!e->leave)
        return;
    if (e->leave.data()->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow leave events through
        return;
    }

    currentMouseWindow = 0;

    QEvent event(QEvent::Leave);
    QCoreApplication::sendSpontaneousEvent(e->leave.data(), &event);
}

void QGuiApplicationPrivate::processActivatedEvent(QWindowSystemInterfacePrivate::ActivatedWindowEvent *e)
{
    QWindow *previous = QGuiApplicationPrivate::focus_window;
    QWindow *newFocus = e->activated.data();

    if (previous == newFocus)
        return;

    if (newFocus)
        if (QPlatformWindow *platformWindow = newFocus->handle())
            if (platformWindow->isAlertState())
                platformWindow->setAlertState(false);

    QObject *previousFocusObject = previous ? previous->focusObject() : 0;

    if (previous) {
        QFocusEvent focusAboutToChange(QEvent::FocusAboutToChange);
        QCoreApplication::sendSpontaneousEvent(previous, &focusAboutToChange);
    }

    QGuiApplicationPrivate::focus_window = newFocus;
    if (!qApp)
        return;

    if (previous) {
        Qt::FocusReason r = e->reason;
        if ((r == Qt::OtherFocusReason || r == Qt::ActiveWindowFocusReason) &&
                newFocus && (newFocus->flags() & Qt::Popup) == Qt::Popup)
            r = Qt::PopupFocusReason;
        QFocusEvent focusOut(QEvent::FocusOut, r);
        QCoreApplication::sendSpontaneousEvent(previous, &focusOut);
        QObject::disconnect(previous, SIGNAL(focusObjectChanged(QObject*)),
                            qApp, SLOT(_q_updateFocusObject(QObject*)));
    } else if (!platformIntegration()->hasCapability(QPlatformIntegration::ApplicationState)) {
        setApplicationState(Qt::ApplicationActive);
    }

    if (QGuiApplicationPrivate::focus_window) {
        Qt::FocusReason r = e->reason;
        if ((r == Qt::OtherFocusReason || r == Qt::ActiveWindowFocusReason) &&
                previous && (previous->flags() & Qt::Popup) == Qt::Popup)
            r = Qt::PopupFocusReason;
        QFocusEvent focusIn(QEvent::FocusIn, r);
        QCoreApplication::sendSpontaneousEvent(QGuiApplicationPrivate::focus_window, &focusIn);
        QObject::connect(QGuiApplicationPrivate::focus_window, SIGNAL(focusObjectChanged(QObject*)),
                         qApp, SLOT(_q_updateFocusObject(QObject*)));
    } else if (!platformIntegration()->hasCapability(QPlatformIntegration::ApplicationState)) {
        setApplicationState(Qt::ApplicationInactive);
    }

    if (self) {
        self->notifyActiveWindowChange(previous);

        if (previousFocusObject != qApp->focusObject())
            self->_q_updateFocusObject(qApp->focusObject());
    }

    emit qApp->focusWindowChanged(newFocus);
    if (previous)
        emit previous->activeChanged();
    if (newFocus)
        emit newFocus->activeChanged();
}

void QGuiApplicationPrivate::processWindowStateChangedEvent(QWindowSystemInterfacePrivate::WindowStateChangedEvent *wse)
{
    if (QWindow *window  = wse->window.data()) {
        QWindowStateChangeEvent e(window->windowState());
        window->d_func()->windowState = wse->newState;
        QGuiApplication::sendSpontaneousEvent(window, &e);
    }
}

void QGuiApplicationPrivate::processWindowScreenChangedEvent(QWindowSystemInterfacePrivate::WindowScreenChangedEvent *wse)
{
    if (QWindow *window  = wse->window.data()) {
        if (QScreen *screen = wse->screen.data())
            window->d_func()->setScreen(screen, false /* recreate */);
        else // Fall back to default behavior, and try to find some appropriate screen
            window->setScreen(0);
    }
}

void QGuiApplicationPrivate::processThemeChanged(QWindowSystemInterfacePrivate::ThemeChangeEvent *tce)
{
    if (self)
        self->notifyThemeChanged();
    if (QWindow *window  = tce->window.data()) {
        QEvent e(QEvent::ThemeChange);
        QGuiApplication::sendSpontaneousEvent(window, &e);
    }
}

void QGuiApplicationPrivate::processGeometryChangeEvent(QWindowSystemInterfacePrivate::GeometryChangeEvent *e)
{
    if (e->tlw.isNull())
       return;

    QWindow *window = e->tlw.data();
    if (!window)
        return;

    QRect newRect = e->newGeometry;
    QRect oldRect = e->oldGeometry.isNull() ? window->d_func()->geometry : e->oldGeometry;

    bool isResize = oldRect.size() != newRect.size();
    bool isMove = oldRect.topLeft() != newRect.topLeft();

    window->d_func()->geometry = newRect;

    if (isResize || window->d_func()->resizeEventPending) {
        QResizeEvent e(newRect.size(), oldRect.size());
        QGuiApplication::sendSpontaneousEvent(window, &e);

        window->d_func()->resizeEventPending = false;

        if (oldRect.width() != newRect.width())
            window->widthChanged(newRect.width());
        if (oldRect.height() != newRect.height())
            window->heightChanged(newRect.height());
    }

    if (isMove) {
        //### frame geometry
        QMoveEvent e(newRect.topLeft(), oldRect.topLeft());
        QGuiApplication::sendSpontaneousEvent(window, &e);

        if (oldRect.x() != newRect.x())
            window->xChanged(newRect.x());
        if (oldRect.y() != newRect.y())
            window->yChanged(newRect.y());
    }
}

void QGuiApplicationPrivate::processCloseEvent(QWindowSystemInterfacePrivate::CloseEvent *e)
{
    if (e->window.isNull())
        return;
    if (e->window.data()->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow close events through
        return;
    }

    QCloseEvent event;
    QGuiApplication::sendSpontaneousEvent(e->window.data(), &event);
    if (e->accepted) {
        *(e->accepted) = event.isAccepted();
    }
}

void QGuiApplicationPrivate::processFileOpenEvent(QWindowSystemInterfacePrivate::FileOpenEvent *e)
{
    if (e->url.isEmpty())
        return;

    QFileOpenEvent event(e->url);
    QGuiApplication::sendSpontaneousEvent(qApp, &event);
}

void QGuiApplicationPrivate::processTabletEvent(QWindowSystemInterfacePrivate::TabletEvent *e)
{
#ifndef QT_NO_TABLETEVENT
    QEvent::Type type = QEvent::TabletMove;
    if (e->down != tabletState) {
        type = e->down ? QEvent::TabletPress : QEvent::TabletRelease;
        tabletState = e->down;
    }

    QWindow *window = e->window.data();
    modifier_buttons = e->modifiers;

    bool localValid = true;
    // If window is null, pick one based on the global position and make sure all
    // subsequent events up to the release are delivered to that same window.
    // If window is given, just send to that.
    if (type == QEvent::TabletPress) {
        if (e->nullWindow) {
            window = QGuiApplication::topLevelAt(e->global.toPoint());
            localValid = false;
        }
        if (!window)
            return;
        tabletPressTarget = window;
    } else {
        if (e->nullWindow) {
            window = tabletPressTarget;
            localValid = false;
        }
        if (type == QEvent::TabletRelease)
            tabletPressTarget = 0;
        if (!window)
            return;
    }
    QPointF local = e->local;
    if (!localValid) {
        QPointF delta = e->global - e->global.toPoint();
        local = window->mapFromGlobal(e->global.toPoint()) + delta;
    }
    QTabletEvent ev(type, local, e->global,
                    e->device, e->pointerType, e->pressure, e->xTilt, e->yTilt,
                    e->tangentialPressure, e->rotation, e->z,
                    e->modifiers, e->uid);
    ev.setTimestamp(e->timestamp);
    QGuiApplication::sendSpontaneousEvent(window, &ev);
#else
    Q_UNUSED(e)
#endif
}

void QGuiApplicationPrivate::processTabletEnterProximityEvent(QWindowSystemInterfacePrivate::TabletEnterProximityEvent *e)
{
#ifndef QT_NO_TABLETEVENT
    QTabletEvent ev(QEvent::TabletEnterProximity, QPointF(), QPointF(),
                    e->device, e->pointerType, 0, 0, 0,
                    0, 0, 0,
                    Qt::NoModifier, e->uid);
    ev.setTimestamp(e->timestamp);
    QGuiApplication::sendSpontaneousEvent(qGuiApp, &ev);
#else
    Q_UNUSED(e)
#endif
}

void QGuiApplicationPrivate::processTabletLeaveProximityEvent(QWindowSystemInterfacePrivate::TabletLeaveProximityEvent *e)
{
#ifndef QT_NO_TABLETEVENT
    QTabletEvent ev(QEvent::TabletLeaveProximity, QPointF(), QPointF(),
                    e->device, e->pointerType, 0, 0, 0,
                    0, 0, 0,
                    Qt::NoModifier, e->uid);
    ev.setTimestamp(e->timestamp);
    QGuiApplication::sendSpontaneousEvent(qGuiApp, &ev);
#else
    Q_UNUSED(e)
#endif
}

#ifndef QT_NO_GESTURES
void QGuiApplicationPrivate::processGestureEvent(QWindowSystemInterfacePrivate::GestureEvent *e)
{
    QNativeGestureEvent ev(e->type, e->pos, e->pos, e->globalPos, e->realValue, e->sequenceId, e->intValue);
    ev.setTimestamp(e->timestamp);
    QGuiApplication::sendSpontaneousEvent(e->window, &ev);
}
#endif // QT_NO_GESTURES

void QGuiApplicationPrivate::processPlatformPanelEvent(QWindowSystemInterfacePrivate::PlatformPanelEvent *e)
{
    if (!e->window)
        return;

    if (e->window->d_func()->blockedByModalWindow) {
        // a modal window is blocking this window, don't allow events through
        return;
    }

    QEvent ev(QEvent::PlatformPanel);
    QGuiApplication::sendSpontaneousEvent(e->window.data(), &ev);
}

#ifndef QT_NO_CONTEXTMENU
void QGuiApplicationPrivate::processContextMenuEvent(QWindowSystemInterfacePrivate::ContextMenuEvent *e)
{
    // Widgets do not care about mouse triggered context menu events. Also, do not forward event
    // to a window blocked by a modal window.
    if (!e->window || e->mouseTriggered || e->window->d_func()->blockedByModalWindow)
        return;

    QContextMenuEvent ev(QContextMenuEvent::Keyboard, e->pos, e->globalPos, e->modifiers);
    QGuiApplication::sendSpontaneousEvent(e->window.data(), &ev);
}
#endif

Q_GUI_EXPORT uint qHash(const QGuiApplicationPrivate::ActiveTouchPointsKey &k)
{
    return qHash(k.device) + k.touchPointId;
}

Q_GUI_EXPORT bool operator==(const QGuiApplicationPrivate::ActiveTouchPointsKey &a,
                             const QGuiApplicationPrivate::ActiveTouchPointsKey &b)
{
    return a.device == b.device
            && a.touchPointId == b.touchPointId;
}

void QGuiApplicationPrivate::processTouchEvent(QWindowSystemInterfacePrivate::TouchEvent *e)
{
    QGuiApplicationPrivate *d = self;
    modifier_buttons = e->modifiers;

    if (e->touchType == QEvent::TouchCancel) {
        // The touch sequence has been canceled (e.g. by the compositor).
        // Send the TouchCancel to all windows with active touches and clean up.
        QTouchEvent touchEvent(QEvent::TouchCancel, e->device, e->modifiers);
        touchEvent.setTimestamp(e->timestamp);
        QHash<ActiveTouchPointsKey, ActiveTouchPointsValue>::const_iterator it
                = self->activeTouchPoints.constBegin(), ite = self->activeTouchPoints.constEnd();
        QSet<QWindow *> windowsNeedingCancel;
        while (it != ite) {
            QWindow *w = it->window.data();
            if (w)
                windowsNeedingCancel.insert(w);
            ++it;
        }
        for (QSet<QWindow *>::const_iterator winIt = windowsNeedingCancel.constBegin(),
             winItEnd = windowsNeedingCancel.constEnd(); winIt != winItEnd; ++winIt) {
            touchEvent.setWindow(*winIt);
            QGuiApplication::sendSpontaneousEvent(*winIt, &touchEvent);
        }
        if (!self->synthesizedMousePoints.isEmpty() && !e->synthetic) {
            for (QHash<QWindow *, SynthesizedMouseData>::const_iterator synthIt = self->synthesizedMousePoints.constBegin(),
                 synthItEnd = self->synthesizedMousePoints.constEnd(); synthIt != synthItEnd; ++synthIt) {
                if (!synthIt->window)
                    continue;
                QWindowSystemInterfacePrivate::MouseEvent fake(synthIt->window.data(),
                                                               e->timestamp,
                                                               synthIt->pos,
                                                               synthIt->screenPos,
                                                               Qt::NoButton,
                                                               e->modifiers,
                                                               Qt::MouseEventSynthesizedByQt);
                fake.synthetic = true;
                processMouseEvent(&fake);
            }
            self->synthesizedMousePoints.clear();
        }
        self->activeTouchPoints.clear();
        self->lastTouchType = e->touchType;
        return;
    }

    // Prevent sending ill-formed event sequences: Cancel can only be followed by a Begin.
    if (self->lastTouchType == QEvent::TouchCancel && e->touchType != QEvent::TouchBegin)
        return;

    self->lastTouchType = e->touchType;

    QWindow *window = e->window.data();
    typedef QPair<Qt::TouchPointStates, QList<QTouchEvent::TouchPoint> > StatesAndTouchPoints;
    QHash<QWindow *, StatesAndTouchPoints> windowsNeedingEvents;

    for (int i = 0; i < e->points.count(); ++i) {
        QTouchEvent::TouchPoint touchPoint = e->points.at(i);
        // explicitly detach from the original touch point that we got, so even
        // if the touchpoint structs are reused, we will make a copy that we'll
        // deliver to the user (which might want to store the struct for later use).
        touchPoint.d = touchPoint.d->detach();

        // update state
        QPointer<QWindow> w;
        QTouchEvent::TouchPoint previousTouchPoint;
        ActiveTouchPointsKey touchInfoKey(e->device, touchPoint.id());
        ActiveTouchPointsValue &touchInfo = d->activeTouchPoints[touchInfoKey];
        switch (touchPoint.state()) {
        case Qt::TouchPointPressed:
            if (e->device->type() == QTouchDevice::TouchPad) {
                // on touch-pads, send all touch points to the same widget
                w = d->activeTouchPoints.isEmpty()
                    ? QPointer<QWindow>()
                    : d->activeTouchPoints.constBegin().value().window;
            }

            if (!w) {
                // determine which window this event will go to
                if (!window)
                    window = QGuiApplication::topLevelAt(touchPoint.screenPos().toPoint());
                if (!window)
                    continue;
                w = window;
            }

            touchInfo.window = w;
            touchPoint.d->startScreenPos = touchPoint.screenPos();
            touchPoint.d->lastScreenPos = touchPoint.screenPos();
            touchPoint.d->startNormalizedPos = touchPoint.normalizedPos();
            touchPoint.d->lastNormalizedPos = touchPoint.normalizedPos();
            if (touchPoint.pressure() < qreal(0.))
                touchPoint.d->pressure = qreal(1.);

            touchInfo.touchPoint = touchPoint;
            break;

        case Qt::TouchPointReleased:
            w = touchInfo.window;
            if (!w)
                continue;

            previousTouchPoint = touchInfo.touchPoint;
            touchPoint.d->startScreenPos = previousTouchPoint.startScreenPos();
            touchPoint.d->lastScreenPos = previousTouchPoint.screenPos();
            touchPoint.d->startPos = previousTouchPoint.startPos();
            touchPoint.d->lastPos = previousTouchPoint.pos();
            touchPoint.d->startNormalizedPos = previousTouchPoint.startNormalizedPos();
            touchPoint.d->lastNormalizedPos = previousTouchPoint.normalizedPos();
            if (touchPoint.pressure() < qreal(0.))
                touchPoint.d->pressure = qreal(0.);

            break;

        default:
            w = touchInfo.window;
            if (!w)
                continue;

            previousTouchPoint = touchInfo.touchPoint;
            touchPoint.d->startScreenPos = previousTouchPoint.startScreenPos();
            touchPoint.d->lastScreenPos = previousTouchPoint.screenPos();
            touchPoint.d->startPos = previousTouchPoint.startPos();
            touchPoint.d->lastPos = previousTouchPoint.pos();
            touchPoint.d->startNormalizedPos = previousTouchPoint.startNormalizedPos();
            touchPoint.d->lastNormalizedPos = previousTouchPoint.normalizedPos();
            if (touchPoint.pressure() < qreal(0.))
                touchPoint.d->pressure = qreal(1.);

            // Stationary points might not be delivered down to the receiving item
            // and get their position transformed, keep the old values instead.
            if (touchPoint.state() != Qt::TouchPointStationary)
                touchInfo.touchPoint = touchPoint;
            break;
        }

        Q_ASSERT(w.data() != 0);

        // make the *scene* functions return the same as the *screen* functions
        touchPoint.d->sceneRect = touchPoint.screenRect();
        touchPoint.d->startScenePos = touchPoint.startScreenPos();
        touchPoint.d->lastScenePos = touchPoint.lastScreenPos();

        StatesAndTouchPoints &maskAndPoints = windowsNeedingEvents[w.data()];
        maskAndPoints.first |= touchPoint.state();
        maskAndPoints.second.append(touchPoint);
    }

    if (windowsNeedingEvents.isEmpty())
        return;

    QHash<QWindow *, StatesAndTouchPoints>::ConstIterator it = windowsNeedingEvents.constBegin();
    const QHash<QWindow *, StatesAndTouchPoints>::ConstIterator end = windowsNeedingEvents.constEnd();
    for (; it != end; ++it) {
        QWindow *w = it.key();

        QEvent::Type eventType;
        switch (it.value().first) {
        case Qt::TouchPointPressed:
            eventType = QEvent::TouchBegin;
            break;
        case Qt::TouchPointReleased:
            eventType = QEvent::TouchEnd;
            break;
        case Qt::TouchPointStationary:
            // don't send the event if nothing changed
            continue;
        default:
            eventType = QEvent::TouchUpdate;
            break;
        }

        if (w->d_func()->blockedByModalWindow) {
            // a modal window is blocking this window, don't allow touch events through

            // QTBUG-37371 temporary fix; TODO: revisit in 5.4 when we have a forwarding solution
            if (eventType == QEvent::TouchEnd) {
                // but don't leave dangling state: e.g.
                // QQuickWindowPrivate::itemForTouchPointId needs to be cleared.
                QTouchEvent touchEvent(QEvent::TouchCancel,
                                       e->device,
                                       e->modifiers);
                touchEvent.setTimestamp(e->timestamp);
                touchEvent.setWindow(w);
                QGuiApplication::sendSpontaneousEvent(w, &touchEvent);
            }
            continue;
        }

        QTouchEvent touchEvent(eventType,
                               e->device,
                               e->modifiers,
                               it.value().first,
                               it.value().second);
        touchEvent.setTimestamp(e->timestamp);
        touchEvent.setWindow(w);

        const int pointCount = touchEvent.touchPoints().count();
        for (int i = 0; i < pointCount; ++i) {
            QTouchEvent::TouchPoint &touchPoint = touchEvent._touchPoints[i];

            // preserve the sub-pixel resolution
            QRectF rect = touchPoint.screenRect();
            const QPointF screenPos = rect.center();
            const QPointF delta = screenPos - screenPos.toPoint();

            rect.moveCenter(w->mapFromGlobal(screenPos.toPoint()) + delta);
            touchPoint.d->rect = rect;
            if (touchPoint.state() == Qt::TouchPointPressed) {
                touchPoint.d->startPos = w->mapFromGlobal(touchPoint.startScreenPos().toPoint()) + delta;
                touchPoint.d->lastPos = w->mapFromGlobal(touchPoint.lastScreenPos().toPoint()) + delta;
            }
        }

        QGuiApplication::sendSpontaneousEvent(w, &touchEvent);
        if (!e->synthetic && !touchEvent.isAccepted() && synthesizeMouseFromTouchEventsEnabled()) {
            // exclude touchpads as those generate their own mouse events
            if (touchEvent.device()->type() != QTouchDevice::TouchPad) {
                Qt::MouseButtons b = eventType == QEvent::TouchEnd ? Qt::NoButton : Qt::LeftButton;
                if (b == Qt::NoButton)
                    self->synthesizedMousePoints.clear();

                QList<QTouchEvent::TouchPoint> touchPoints = touchEvent.touchPoints();
                if (eventType == QEvent::TouchBegin)
                    m_fakeMouseSourcePointId = touchPoints.first().id();

                for (int i = 0; i < touchPoints.count(); ++i) {
                    const QTouchEvent::TouchPoint &touchPoint = touchPoints.at(i);
                    if (touchPoint.id() == m_fakeMouseSourcePointId) {
                        if (b != Qt::NoButton)
                            self->synthesizedMousePoints.insert(w, SynthesizedMouseData(
                                                                    touchPoint.pos(), touchPoint.screenPos(), w));
                        QWindowSystemInterfacePrivate::MouseEvent fake(w, e->timestamp,
                                                                       touchPoint.pos(),
                                                                       touchPoint.screenPos(),
                                                                       b, e->modifiers);
                        fake.synthetic = true;
                        processMouseEvent(&fake);
                        break;
                    }
                }
            }
        }
    }

    // Remove released points from the hash table only after the event is
    // delivered. When the receiver is a widget, QApplication will access
    // activeTouchPoints during delivery and therefore nothing can be removed
    // before sending the event.
    for (int i = 0; i < e->points.count(); ++i) {
        QTouchEvent::TouchPoint touchPoint = e->points.at(i);
        if (touchPoint.state() == Qt::TouchPointReleased)
            d->activeTouchPoints.remove(ActiveTouchPointsKey(e->device, touchPoint.id()));
    }
}

void QGuiApplicationPrivate::reportScreenOrientationChange(QWindowSystemInterfacePrivate::ScreenOrientationEvent *e)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    if (!e->screen)
        return;

    QScreen *s = e->screen.data();
    s->d_func()->orientation = e->orientation;

    updateFilteredScreenOrientation(s);
}

void QGuiApplicationPrivate::updateFilteredScreenOrientation(QScreen *s)
{
    Qt::ScreenOrientation o = s->d_func()->orientation;
    if (o == Qt::PrimaryOrientation)
        o = s->primaryOrientation();
    o = Qt::ScreenOrientation(o & s->orientationUpdateMask());
    if (o == Qt::PrimaryOrientation)
        return;
    if (o == s->d_func()->filteredOrientation)
        return;
    s->d_func()->filteredOrientation = o;
    reportScreenOrientationChange(s);
}

void QGuiApplicationPrivate::reportScreenOrientationChange(QScreen *s)
{
    emit s->orientationChanged(s->orientation());

    QScreenOrientationChangeEvent event(s, s->orientation());
    QCoreApplication::sendEvent(QCoreApplication::instance(), &event);
}

void QGuiApplicationPrivate::reportGeometryChange(QWindowSystemInterfacePrivate::ScreenGeometryEvent *e)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    if (!e->screen)
        return;

    QScreen *s = e->screen.data();
    s->d_func()->geometry = e->geometry;

    Qt::ScreenOrientation primaryOrientation = s->primaryOrientation();
    s->d_func()->updatePrimaryOrientation();

    emit s->geometryChanged(s->geometry());
    emit s->physicalSizeChanged(s->physicalSize());
    emit s->physicalDotsPerInchChanged(s->physicalDotsPerInch());
    emit s->logicalDotsPerInchChanged(s->logicalDotsPerInch());
    foreach (QScreen* sibling, s->virtualSiblings())
        emit sibling->virtualGeometryChanged(sibling->virtualGeometry());

    if (s->primaryOrientation() != primaryOrientation)
        emit s->primaryOrientationChanged(s->primaryOrientation());

    if (s->d_func()->orientation == Qt::PrimaryOrientation)
        updateFilteredScreenOrientation(s);
}

void QGuiApplicationPrivate::reportAvailableGeometryChange(
        QWindowSystemInterfacePrivate::ScreenAvailableGeometryEvent *e)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    if (!e->screen)
        return;

    QScreen *s = e->screen.data();
    s->d_func()->availableGeometry = e->availableGeometry;

    foreach (QScreen* sibling, s->virtualSiblings())
        emit sibling->virtualGeometryChanged(sibling->virtualGeometry());
}

void QGuiApplicationPrivate::reportLogicalDotsPerInchChange(QWindowSystemInterfacePrivate::ScreenLogicalDotsPerInchEvent *e)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    if (!e->screen)
        return;

    QScreen *s = e->screen.data();
    s->d_func()->logicalDpi = QDpi(e->dpiX, e->dpiY);

    emit s->logicalDotsPerInchChanged(s->logicalDotsPerInch());
}

void QGuiApplicationPrivate::reportRefreshRateChange(QWindowSystemInterfacePrivate::ScreenRefreshRateEvent *e)
{
    // This operation only makes sense after the QGuiApplication constructor runs
    if (QCoreApplication::startingUp())
        return;

    if (!e->screen)
        return;

    QScreen *s = e->screen.data();
    s->d_func()->refreshRate = e->rate;

    emit s->refreshRateChanged(s->refreshRate());
}

void QGuiApplicationPrivate::processExposeEvent(QWindowSystemInterfacePrivate::ExposeEvent *e)
{
    if (!e->exposed)
        return;

    QWindow *window = e->exposed.data();
    QWindowPrivate *p = qt_window_private(window);

    if (!p->receivedExpose) {
        if (p->resizeEventPending) {
            // as a convenience for plugins, send a resize event before the first expose event if they haven't done so
            // window->geometry() should have a valid size as soon as a handle exists.
            QResizeEvent e(window->geometry().size(), p->geometry.size());
            QGuiApplication::sendSpontaneousEvent(window, &e);

            p->resizeEventPending = false;
        }

        p->receivedExpose = true;
    }

    p->exposed = e->isExposed;

    QExposeEvent exposeEvent(e->region);
    QCoreApplication::sendSpontaneousEvent(window, &exposeEvent);
}

#ifndef QT_NO_DRAGANDDROP

QPlatformDragQtResponse QGuiApplicationPrivate::processDrag(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions)
{
    static QPointer<QWindow> currentDragWindow;
    static Qt::DropAction lastAcceptedDropAction = Qt::IgnoreAction;
    QPlatformDrag *platformDrag = platformIntegration()->drag();
    if (!platformDrag) {
        lastAcceptedDropAction = Qt::IgnoreAction;
        return QPlatformDragQtResponse(false, lastAcceptedDropAction, QRect());
    }

    if (!dropData) {
        if (currentDragWindow.data() == w)
            currentDragWindow = 0;
        QDragLeaveEvent e;
        QGuiApplication::sendEvent(w, &e);
        lastAcceptedDropAction = Qt::IgnoreAction;
        return QPlatformDragQtResponse(false, lastAcceptedDropAction, QRect());
    }
    QDragMoveEvent me(p, supportedActions, dropData,
                      QGuiApplication::mouseButtons(), QGuiApplication::keyboardModifiers());

    if (w != currentDragWindow) {
        lastAcceptedDropAction = Qt::IgnoreAction;
        if (currentDragWindow) {
            QDragLeaveEvent e;
            QGuiApplication::sendEvent(currentDragWindow, &e);
        }
        currentDragWindow = w;
        QDragEnterEvent e(p, supportedActions, dropData,
                          QGuiApplication::mouseButtons(), QGuiApplication::keyboardModifiers());
        QGuiApplication::sendEvent(w, &e);
        if (e.isAccepted() && e.dropAction() != Qt::IgnoreAction)
            lastAcceptedDropAction = e.dropAction();
    }

    // Handling 'DragEnter' should suffice for the application.
    if (lastAcceptedDropAction != Qt::IgnoreAction
        && (supportedActions & lastAcceptedDropAction)) {
        me.setDropAction(lastAcceptedDropAction);
        me.accept();
    }
    QGuiApplication::sendEvent(w, &me);
    lastAcceptedDropAction = me.isAccepted() ?
                             me.dropAction() :  Qt::IgnoreAction;
    return QPlatformDragQtResponse(me.isAccepted(), lastAcceptedDropAction, me.answerRect());
}

QPlatformDropQtResponse QGuiApplicationPrivate::processDrop(QWindow *w, const QMimeData *dropData, const QPoint &p, Qt::DropActions supportedActions)
{
    QDropEvent de(p, supportedActions, dropData,
                  QGuiApplication::mouseButtons(), QGuiApplication::keyboardModifiers());
    QGuiApplication::sendEvent(w, &de);

    Qt::DropAction acceptedAction = de.isAccepted() ? de.dropAction() : Qt::IgnoreAction;
    QPlatformDropQtResponse response(de.isAccepted(),acceptedAction);
    return response;
}

#endif // QT_NO_DRAGANDDROP

#ifndef QT_NO_CLIPBOARD
/*!
    Returns the object for interacting with the clipboard.
*/
QClipboard * QGuiApplication::clipboard()
{
    if (QGuiApplicationPrivate::qt_clipboard == 0) {
        if (!qApp) {
            qWarning("QGuiApplication: Must construct a QGuiApplication before accessing a QClipboard");
            return 0;
        }
        QGuiApplicationPrivate::qt_clipboard = new QClipboard(0);
    }
    return QGuiApplicationPrivate::qt_clipboard;
}
#endif

/*!
    Returns the default application palette.

    \sa setPalette()
*/

QPalette QGuiApplication::palette()
{
    initPalette();
    return *QGuiApplicationPrivate::app_pal;
}

/*!
    Changes the default application palette to \a pal.

    \sa palette()
*/
void QGuiApplication::setPalette(const QPalette &pal)
{
    if (QGuiApplicationPrivate::app_pal && pal.isCopyOf(*QGuiApplicationPrivate::app_pal))
        return;
    if (!QGuiApplicationPrivate::app_pal)
        QGuiApplicationPrivate::app_pal = new QPalette(pal);
    else
        *QGuiApplicationPrivate::app_pal = pal;
    applicationResourceFlags |= ApplicationPaletteExplicitlySet;
}

QRect QGuiApplicationPrivate::applyWindowGeometrySpecification(const QRect &windowGeometry, const QWindow *window)
{
    return windowGeometrySpecification.apply(windowGeometry, window);
}

/*!
    Returns the default application font.

    \sa setFont()
*/
QFont QGuiApplication::font()
{
    Q_ASSERT_X(QGuiApplicationPrivate::self, "QGuiApplication::font()", "no QGuiApplication instance");
    QMutexLocker locker(&applicationFontMutex);
    initFontUnlocked();
    return *QGuiApplicationPrivate::app_font;
}

/*!
    Changes the default application font to \a font.

    \sa font()
*/
void QGuiApplication::setFont(const QFont &font)
{
    QMutexLocker locker(&applicationFontMutex);
    if (!QGuiApplicationPrivate::app_font)
        QGuiApplicationPrivate::app_font = new QFont(font);
    else
        *QGuiApplicationPrivate::app_font = font;
    applicationResourceFlags |= ApplicationFontExplicitlySet;
}

/*!
    \fn bool QGuiApplication::isRightToLeft()

    Returns \c true if the application's layout direction is
    Qt::RightToLeft; otherwise returns \c false.

    \sa layoutDirection(), isLeftToRight()
*/

/*!
    \fn bool QGuiApplication::isLeftToRight()

    Returns \c true if the application's layout direction is
    Qt::LeftToRight; otherwise returns \c false.

    \sa layoutDirection(), isRightToLeft()
*/

void QGuiApplicationPrivate::notifyLayoutDirectionChange()
{
    const QWindowList list = QGuiApplication::topLevelWindows();
    for (int i = 0; i < list.size(); ++i) {
        QEvent ev(QEvent::ApplicationLayoutDirectionChange);
        QCoreApplication::sendEvent(list.at(i), &ev);
    }
}

void QGuiApplicationPrivate::notifyActiveWindowChange(QWindow *)
{
}

/*!
    \property QGuiApplication::windowIcon
    \brief the default window icon

    \sa QWindow::setIcon(), {Setting the Application Icon}
*/
QIcon QGuiApplication::windowIcon()
{
    return QGuiApplicationPrivate::app_icon ? *QGuiApplicationPrivate::app_icon : QIcon();
}

void QGuiApplication::setWindowIcon(const QIcon &icon)
{
    if (!QGuiApplicationPrivate::app_icon)
        QGuiApplicationPrivate::app_icon = new QIcon();
    *QGuiApplicationPrivate::app_icon = icon;
    if (QGuiApplicationPrivate::is_app_running && !QGuiApplicationPrivate::is_app_closing)
        QGuiApplicationPrivate::self->notifyWindowIconChanged();
}

void QGuiApplicationPrivate::notifyWindowIconChanged()
{
    QEvent ev(QEvent::ApplicationWindowIconChange);
    const QWindowList list = QGuiApplication::topLevelWindows();
    for (int i = 0; i < list.size(); ++i)
        QCoreApplication::sendEvent(list.at(i), &ev);
}



/*!
    \property QGuiApplication::quitOnLastWindowClosed

    \brief whether the application implicitly quits when the last window is
    closed.

    The default is \c true.

    If this property is \c true, the applications quits when the last visible
    primary window (i.e. window with no parent) is closed.

    \sa quit(), QWindow::close()
 */

void QGuiApplication::setQuitOnLastWindowClosed(bool quit)
{
    QCoreApplication::setQuitLockEnabled(quit);
}



bool QGuiApplication::quitOnLastWindowClosed()
{
    return QCoreApplication::isQuitLockEnabled();
}


/*!
    \fn void QGuiApplication::lastWindowClosed()

    This signal is emitted from exec() when the last visible
    primary window (i.e. window with no parent) is closed.

    By default, QGuiApplication quits after this signal is emitted. This feature
    can be turned off by setting \l quitOnLastWindowClosed to \c false.

    \sa QWindow::close(), QWindow::isTopLevel()
*/

void QGuiApplicationPrivate::emitLastWindowClosed()
{
    if (qGuiApp && qGuiApp->d_func()->in_exec) {
        emit qGuiApp->lastWindowClosed();
    }
}

bool QGuiApplicationPrivate::shouldQuit()
{
    const QWindowList processedWindows;
    return shouldQuitInternal(processedWindows);
}

bool QGuiApplicationPrivate::shouldQuitInternal(const QWindowList &processedWindows)
{
    /* if there is no visible top-level window left, we allow the quit */
    QWindowList list = QGuiApplication::topLevelWindows();
    for (int i = 0; i < list.size(); ++i) {
        QWindow *w = list.at(i);
        if (processedWindows.contains(w))
            continue;
        if (w->isVisible() && !w->transientParent())
            return false;
    }
    return true;
}

bool QGuiApplicationPrivate::tryCloseAllWindows()
{
    return tryCloseRemainingWindows(QWindowList());
}

bool QGuiApplicationPrivate::tryCloseRemainingWindows(QWindowList processedWindows)
{
    QWindowList list = QGuiApplication::topLevelWindows();
    for (int i = 0; i < list.size(); ++i) {
        QWindow *w = list.at(i);
        if (w->isVisible() && !processedWindows.contains(w)) {
            if (!w->close())
                return false;
            processedWindows.append(w);
            list = QGuiApplication::topLevelWindows();
            i = -1;
        }
    }
    return true;
}

/*!
    \since 5.2
    \fn Qt::ApplicationState QGuiApplication::applicationState()


    Returns the current state of the application.

    You can react to application state changes to perform actions such as
    stopping/resuming CPU-intensive tasks, freeing/loading resources or
    saving/restoring application data.
 */

Qt::ApplicationState QGuiApplication::applicationState()
{
    return QGuiApplicationPrivate::applicationState;
}

/*!
    \since 5.2
    \fn void QGuiApplication::applicationStateChanged(Qt::ApplicationState state)

    This signal is emitted when the \a state of the application changes.

    \sa applicationState()
*/

void QGuiApplicationPrivate::setApplicationState(Qt::ApplicationState state)
{
    if (applicationState == state)
        return;

    applicationState = state;

    switch (state) {
    case Qt::ApplicationActive: {
        QEvent appActivate(QEvent::ApplicationActivate);
        QCoreApplication::sendSpontaneousEvent(qApp, &appActivate);
        break; }
    case Qt::ApplicationInactive: {
        QEvent appDeactivate(QEvent::ApplicationDeactivate);
        QCoreApplication::sendSpontaneousEvent(qApp, &appDeactivate);
        break; }
    default:
        break;
    }

    QApplicationStateChangeEvent event(applicationState);
    QCoreApplication::sendSpontaneousEvent(qApp, &event);

    emit qApp->applicationStateChanged(applicationState);
}

/*!
    \since 4.2
    \fn void QGuiApplication::commitDataRequest(QSessionManager &manager)

    This signal deals with \l{Session Management}{session management}. It is
    emitted when the QSessionManager wants the application to commit all its
    data.

    Usually this means saving all open files, after getting permission from
    the user. Furthermore you may want to provide a means by which the user
    can cancel the shutdown.

    You should not exit the application within this signal. Instead,
    the session manager may or may not do this afterwards, depending on the
    context.

    \warning Within this signal, no user interaction is possible, \e
    unless you ask the \a manager for explicit permission. See
    QSessionManager::allowsInteraction() and
    QSessionManager::allowsErrorInteraction() for details and example
    usage.

    \note You should use Qt::DirectConnection when connecting to this signal.

    \sa isSessionRestored(), sessionId(), saveStateRequest(), {Session Management}
*/

/*!
    \since 4.2
    \fn void QGuiApplication::saveStateRequest(QSessionManager &manager)

    This signal deals with \l{Session Management}{session management}. It is
    invoked when the \l{QSessionManager}{session manager} wants the application
    to preserve its state for a future session.

    For example, a text editor would create a temporary file that includes the
    current contents of its edit buffers, the location of the cursor and other
    aspects of the current editing session.

    You should never exit the application within this signal. Instead, the
    session manager may or may not do this afterwards, depending on the
    context. Futhermore, most session managers will very likely request a saved
    state immediately after the application has been started. This permits the
    session manager to learn about the application's restart policy.

    \warning Within this signal, no user interaction is possible, \e
    unless you ask the \a manager for explicit permission. See
    QSessionManager::allowsInteraction() and
    QSessionManager::allowsErrorInteraction() for details.

    \note You should use Qt::DirectConnection when connecting to this signal.

    \sa isSessionRestored(), sessionId(), commitDataRequest(), {Session Management}
*/

/*!
    \fn bool QGuiApplication::isSessionRestored() const

    Returns \c true if the application has been restored from an earlier
    \l{Session Management}{session}; otherwise returns \c false.

    \sa sessionId(), commitDataRequest(), saveStateRequest()
*/

/*!
    \since 5.0
    \fn bool QGuiApplication::isSavingSession() const

    Returns \c true if the application is currently saving the
    \l{Session Management}{session}; otherwise returns \c false.

    This is \c true when commitDataRequest() and saveStateRequest() are emitted,
    but also when the windows are closed afterwards by session management.

    \sa sessionId(), commitDataRequest(), saveStateRequest()
*/

/*!
    \fn QString QGuiApplication::sessionId() const

    Returns the current \l{Session Management}{session's} identifier.

    If the application has been restored from an earlier session, this
    identifier is the same as it was in that previous session. The session
    identifier is guaranteed to be unique both for different applications
    and for different instances of the same application.

    \sa isSessionRestored(), sessionKey(), commitDataRequest(), saveStateRequest()
*/

/*!
    \fn QString QGuiApplication::sessionKey() const

    Returns the session key in the current \l{Session Management}{session}.

    If the application has been restored from an earlier session, this key is
    the same as it was when the previous session ended.

    The session key changes every time the session is saved. If the shutdown process
    is cancelled, another session key will be used when shutting down again.

    \sa isSessionRestored(), sessionId(), commitDataRequest(), saveStateRequest()
*/
#ifndef QT_NO_SESSIONMANAGER
bool QGuiApplication::isSessionRestored() const
{
    Q_D(const QGuiApplication);
    return d->is_session_restored;
}

QString QGuiApplication::sessionId() const
{
    Q_D(const QGuiApplication);
    return d->session_manager->sessionId();
}

QString QGuiApplication::sessionKey() const
{
    Q_D(const QGuiApplication);
    return d->session_manager->sessionKey();
}

bool QGuiApplication::isSavingSession() const
{
    Q_D(const QGuiApplication);
    return d->is_saving_session;
}

/*!
    \since 5.2

    Function that can be used to sync Qt state with the Window Systems state.

    This function will first empty Qts events by calling QCoreApplication::processEvents(),
    then the platform plugin will sync up with the windowsystem, and finally Qts events
    will be delived by another call to QCoreApplication::processEvents();

    This function is timeconsuming and its use is discouraged.
*/
void QGuiApplication::sync()
{
    QCoreApplication::processEvents();
    if (QGuiApplicationPrivate::platform_integration
            && QGuiApplicationPrivate::platform_integration->hasCapability(QPlatformIntegration::SyncState)) {
        QGuiApplicationPrivate::platform_integration->sync();
        QCoreApplication::processEvents();
        QWindowSystemInterface::flushWindowSystemEvents();
    }
}

void QGuiApplicationPrivate::commitData()
{
    Q_Q(QGuiApplication);
    is_saving_session = true;
    emit q->commitDataRequest(*session_manager);
    if (session_manager->allowsInteraction() && !tryCloseAllWindows())
        session_manager->cancel();
    is_saving_session = false;
}


void QGuiApplicationPrivate::saveState()
{
    Q_Q(QGuiApplication);
    is_saving_session = true;
    emit q->saveStateRequest(*session_manager);
    is_saving_session = false;
}
#endif //QT_NO_SESSIONMANAGER

/*!
    \property QGuiApplication::layoutDirection
    \brief the default layout direction for this application

    On system start-up, the default layout direction depends on the
    application's language.

    \sa QWidget::layoutDirection, isLeftToRight(), isRightToLeft()
 */

void QGuiApplication::setLayoutDirection(Qt::LayoutDirection direction)
{
    if (layout_direction == direction || direction == Qt::LayoutDirectionAuto)
        return;

    layout_direction = direction;

    QGuiApplicationPrivate::self->notifyLayoutDirectionChange();
}

Qt::LayoutDirection QGuiApplication::layoutDirection()
{
    return layout_direction;
}

/*!
    \fn QCursor *QGuiApplication::overrideCursor()

    Returns the active application override cursor.

    This function returns 0 if no application cursor has been defined (i.e. the
    internal cursor stack is empty).

    \sa setOverrideCursor(), restoreOverrideCursor()
*/
#ifndef QT_NO_CURSOR
QCursor *QGuiApplication::overrideCursor()
{
    return qGuiApp->d_func()->cursor_list.isEmpty() ? 0 : &qGuiApp->d_func()->cursor_list.first();
}

/*!
    Changes the currently active application override cursor to \a cursor.

    This function has no effect if setOverrideCursor() was not called.

    \sa setOverrideCursor(), overrideCursor(), restoreOverrideCursor(),
    QWidget::setCursor()
 */
void QGuiApplication::changeOverrideCursor(const QCursor &cursor)
{
    if (qGuiApp->d_func()->cursor_list.isEmpty())
        return;
    qGuiApp->d_func()->cursor_list.removeFirst();
    setOverrideCursor(cursor);
}
#endif


#ifndef QT_NO_CURSOR
static inline void applyCursor(QWindow *w, QCursor c)
{
    if (const QScreen *screen = w->screen())
        if (QPlatformCursor *cursor = screen->handle()->cursor())
            cursor->changeCursor(&c, w);
}

static inline void unsetCursor(QWindow *w)
{
    if (const QScreen *screen = w->screen())
        if (QPlatformCursor *cursor = screen->handle()->cursor())
            cursor->changeCursor(0, w);
}

static inline void applyCursor(const QList<QWindow *> &l, const QCursor &c)
{
    for (int i = 0; i < l.size(); ++i) {
        QWindow *w = l.at(i);
        if (w->handle() && w->type() != Qt::Desktop)
            applyCursor(w, c);
    }
}

static inline void applyWindowCursor(const QList<QWindow *> &l)
{
    for (int i = 0; i < l.size(); ++i) {
        QWindow *w = l.at(i);
        if (w->handle() && w->type() != Qt::Desktop) {
            if (qt_window_private(w)->hasCursor) {
                applyCursor(w, w->cursor());
            } else {
                unsetCursor(w);
            }
        }
    }
}

/*!
    \fn void QGuiApplication::setOverrideCursor(const QCursor &cursor)

    Sets the application override cursor to \a cursor.

    Application override cursors are intended for showing the user that the
    application is in a special state, for example during an operation that
    might take some time.

    This cursor will be displayed in all the application's widgets until
    restoreOverrideCursor() or another setOverrideCursor() is called.

    Application cursors are stored on an internal stack. setOverrideCursor()
    pushes the cursor onto the stack, and restoreOverrideCursor() pops the
    active cursor off the stack. changeOverrideCursor() changes the curently
    active application override cursor.

    Every setOverrideCursor() must eventually be followed by a corresponding
    restoreOverrideCursor(), otherwise the stack will never be emptied.

    Example:
    \snippet code/src_gui_kernel_qguiapplication_x11.cpp 0

    \sa overrideCursor(), restoreOverrideCursor(), changeOverrideCursor(),
    QWidget::setCursor()
*/
void QGuiApplication::setOverrideCursor(const QCursor &cursor)
{
    qGuiApp->d_func()->cursor_list.prepend(cursor);
    applyCursor(QGuiApplicationPrivate::window_list, cursor);
}

/*!
    \fn void QGuiApplication::restoreOverrideCursor()

    Undoes the last setOverrideCursor().

    If setOverrideCursor() has been called twice, calling
    restoreOverrideCursor() will activate the first cursor set. Calling this
    function a second time restores the original widgets' cursors.

    \sa setOverrideCursor(), overrideCursor()
*/
void QGuiApplication::restoreOverrideCursor()
{
    if (qGuiApp->d_func()->cursor_list.isEmpty())
        return;
    qGuiApp->d_func()->cursor_list.removeFirst();
    if (qGuiApp->d_func()->cursor_list.size() > 0) {
        QCursor c(qGuiApp->d_func()->cursor_list.value(0));
        applyCursor(QGuiApplicationPrivate::window_list, c);
    } else {
        applyWindowCursor(QGuiApplicationPrivate::window_list);
    }
}
#endif// QT_NO_CURSOR

/*!
  Returns the application's style hints.

  The style hints encapsulate a set of platform dependent properties
  such as double click intervals, full width selection and others.

  The hints can be used to integrate tighter with the underlying platform.

  \sa QStyleHints
  */
QStyleHints *QGuiApplication::styleHints()
{
    if (!qGuiApp->d_func()->styleHints)
        qGuiApp->d_func()->styleHints = new QStyleHints();
    return qGuiApp->d_func()->styleHints;
}

/*!
    Sets whether Qt should use the system's standard colors, fonts, etc., to
    \a on. By default, this is \c true.

    This function must be called before creating the QGuiApplication object, like
    this:

    \snippet code/src_gui_kernel_qguiapplication.cpp 0

    \sa desktopSettingsAware()
*/
void QGuiApplication::setDesktopSettingsAware(bool on)
{
    QGuiApplicationPrivate::obey_desktop_settings = on;
}

/*!
    Returns \c true if Qt is set to use the system's standard colors, fonts, etc.;
    otherwise returns \c false. The default is \c true.

    \sa setDesktopSettingsAware()
*/
bool QGuiApplication::desktopSettingsAware()
{
    return QGuiApplicationPrivate::obey_desktop_settings;
}

/*!
  returns the input method.

  The input method returns properties about the state and position of
  the virtual keyboard. It also provides information about the position of the
  current focused input element.

  \sa QInputMethod
  */
QInputMethod *QGuiApplication::inputMethod()
{
    if (!qGuiApp->d_func()->inputMethod)
        qGuiApp->d_func()->inputMethod = new QInputMethod();
    return qGuiApp->d_func()->inputMethod;
}

/*!
    \fn void QGuiApplication::fontDatabaseChanged()

    This signal is emitted when application fonts are loaded or removed.

    \sa QFontDatabase::addApplicationFont(),
    QFontDatabase::addApplicationFontFromData(),
    QFontDatabase::removeAllApplicationFonts(),
    QFontDatabase::removeApplicationFont()
*/

QPixmap QGuiApplicationPrivate::getPixmapCursor(Qt::CursorShape cshape)
{
    Q_UNUSED(cshape);
    return QPixmap();
}

void QGuiApplicationPrivate::notifyThemeChanged()
{
    if (!(applicationResourceFlags & ApplicationPaletteExplicitlySet)) {
        clearPalette();
        initPalette();
    }
    if (!(applicationResourceFlags & ApplicationFontExplicitlySet)) {
        QMutexLocker locker(&applicationFontMutex);
        clearFontUnlocked();
        initFontUnlocked();
    }
}

#ifndef QT_NO_DRAGANDDROP
void QGuiApplicationPrivate::notifyDragStarted(const QDrag *drag)
{
    Q_UNUSED(drag)

}
#endif

const QDrawHelperGammaTables *QGuiApplicationPrivate::gammaTables()
{
    QDrawHelperGammaTables *result = m_gammaTables.load();
    if (!result){
        QDrawHelperGammaTables *tables = new QDrawHelperGammaTables(fontSmoothingGamma);
        if (!m_gammaTables.testAndSetRelease(0, tables))
            delete tables;
        result = m_gammaTables.load();
    }
    return result;
}

void QGuiApplicationPrivate::_q_updateFocusObject(QObject *object)
{
    Q_Q(QGuiApplication);

    bool enabled = false;
    if (object) {
        QInputMethodQueryEvent query(Qt::ImEnabled);
        QGuiApplication::sendEvent(object, &query);
        enabled = query.value(Qt::ImEnabled).toBool();
    }

    QPlatformInputContextPrivate::setInputMethodAccepted(enabled);
    QPlatformInputContext *inputContext = platformIntegration()->inputContext();
    if (inputContext)
        inputContext->setFocusObject(object);
    emit q->focusObjectChanged(object);
}

enum {
    MouseCapsMask = 0xFF,
    MouseSourceMaskDst = 0xFF00,
    MouseSourceMaskSrc = MouseCapsMask,
    MouseSourceShift = 8,
    MouseFlagsCapsMask = 0xFF0000,
    MouseFlagsShift = 16
};

int QGuiApplicationPrivate::mouseEventCaps(QMouseEvent *event)
{
    return event->caps & MouseCapsMask;
}

QVector2D QGuiApplicationPrivate::mouseEventVelocity(QMouseEvent *event)
{
    return event->velocity;
}

void QGuiApplicationPrivate::setMouseEventCapsAndVelocity(QMouseEvent *event, int caps, const QVector2D &velocity)
{
    Q_ASSERT(caps <= MouseCapsMask);
    event->caps &= ~MouseCapsMask;
    event->caps |= caps & MouseCapsMask;
    event->velocity = velocity;
}

Qt::MouseEventSource QGuiApplicationPrivate::mouseEventSource(const QMouseEvent *event)
{
    return Qt::MouseEventSource((event->caps & MouseSourceMaskDst) >> MouseSourceShift);
}

void QGuiApplicationPrivate::setMouseEventSource(QMouseEvent *event, Qt::MouseEventSource source)
{
    // Mouse event synthesization status is encoded in the caps field because
    // QTouchDevice::CapabilityFlag uses only 6 bits from it.
    int value = source;
    Q_ASSERT(value <= MouseSourceMaskSrc);
    event->caps &= ~MouseSourceMaskDst;
    event->caps |= (value & MouseSourceMaskSrc) << MouseSourceShift;
}

Qt::MouseEventFlags QGuiApplicationPrivate::mouseEventFlags(const QMouseEvent *event)
{
    return Qt::MouseEventFlags((event->caps & MouseFlagsCapsMask) >> MouseFlagsShift);
}

void QGuiApplicationPrivate::setMouseEventFlags(QMouseEvent *event, Qt::MouseEventFlags flags)
{
    // use the 0x00FF0000 byte from caps (containing up to 7 mouse event flags)
    unsigned int value = flags;
    Q_ASSERT(value <= Qt::MouseEventFlagMask);
    event->caps &= ~MouseFlagsCapsMask;
    event->caps |= (value & Qt::MouseEventFlagMask) << MouseFlagsShift;
}

#include "moc_qguiapplication.cpp"

QT_END_NAMESPACE

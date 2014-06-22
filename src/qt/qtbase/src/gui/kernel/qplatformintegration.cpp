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

#include "qplatformintegration.h"

#include <qpa/qplatformfontdatabase.h>
#include <qpa/qplatformclipboard.h>
#include <qpa/qplatformaccessibility.h>
#include <qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qpixmap_raster_p.h>
#include <qpa/qplatformscreen_p.h>
#include <private/qdnd_p.h>
#include <private/qsimpledrag_p.h>

#ifndef QT_NO_SESSIONMANAGER
# include <qpa/qplatformsessionmanager.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    Accessor for the platform integration's fontdatabase.

    Default implementation returns a default QPlatformFontDatabase.

    \sa QPlatformFontDatabase
*/
QPlatformFontDatabase *QPlatformIntegration::fontDatabase() const
{
    static QPlatformFontDatabase *db = 0;
    if (!db) {
        db = new QPlatformFontDatabase;
    }
    return db;
}

/*!
    Accessor for the platform integration's clipboard.

    Default implementation returns a default QPlatformClipboard.

    \sa QPlatformClipboard

*/

#ifndef QT_NO_CLIPBOARD

QPlatformClipboard *QPlatformIntegration::clipboard() const
{
    static QPlatformClipboard *clipboard = 0;
    if (!clipboard) {
        clipboard = new QPlatformClipboard;
    }
    return clipboard;
}

#endif

#ifndef QT_NO_DRAGANDDROP
/*!
    Accessor for the platform integration's drag object.

    Default implementation returns 0, implying no drag and drop support.

*/
QPlatformDrag *QPlatformIntegration::drag() const
{
    static QSimpleDrag *drag = 0;
    if (!drag) {
        drag = new QSimpleDrag;
    }
    return drag;
}
#endif

QPlatformNativeInterface * QPlatformIntegration::nativeInterface() const
{
    return 0;
}

QPlatformServices *QPlatformIntegration::services() const
{
    return 0;
}

/*!
    \class QPlatformIntegration
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa
    \brief The QPlatformIntegration class is the entry for WindowSystem specific functionality.

    QPlatformIntegration is the single entry point for windowsystem specific functionality when
    using the QPA platform. It has factory functions for creating platform specific pixmaps and
    windows. The class also controls the font subsystem.

    QPlatformIntegration is a singleton class which gets instantiated in the QGuiApplication
    constructor. The QPlatformIntegration instance do not have ownership of objects it creates in
    functions where the name starts with create. However, functions which don't have a name
    starting with create acts as accessors to member variables.

    It is not trivial to create or build a platform plugin outside of the Qt source tree. Therefore
    the recommended approach for making new platform plugin is to copy an existing plugin inside
    the QTSRCTREE/src/plugins/platform and develop the plugin inside the source tree.

    The minimal platform integration is the smallest platform integration it is possible to make,
    which makes it an ideal starting point for new plugins. For a slightly more advanced plugin,
    consider reviewing the directfb plugin, or the testlite plugin.
*/

/*!
    \fn QPlatformPixmap *QPlatformIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const

    Factory function for QPlatformPixmap. PixelType can be either PixmapType or BitmapType.
    \sa QPlatformPixmap
*/

/*!
    \fn QPlatformWindow *QPlatformIntegration::createPlatformWindow(QWindow *window) const

    Factory function for QPlatformWindow. The \a window parameter is a pointer to the top level
    window which the QPlatformWindow is supposed to be created for.

    All top level windows have to have a QPlatformWindow, and it will be created when the
    QPlatformWindow is set to be visible for the first time. If the top level window's flags are
    changed, or if the top level window's QPlatformWindowFormat is changed, then the top level
    window's QPlatformWindow is deleted and a new one is created.

    In the constructor, of the QPlatformWindow, the window flags, state, title and geometry
    of the \a window should be applied to the underlying window. If the resulting flags or state
    differs, the resulting values should be set on the \a window using QWindow::setWindowFlags()
    or QWindow::setWindowState(), respectively.

    \sa QPlatformWindow, QPlatformWindowFormat
    \sa createPlatformBackingStore()
*/

/*!
    \fn QPlatformBackingStore *QPlatformIntegration::createPlatformBackingStore(QWindow *window) const

    Factory function for QPlatformBackingStore. The QWindow parameter is a pointer to the
    top level widget(tlw) the window surface is created for. A QPlatformWindow is always created
    before the QPlatformBackingStore for tlw where the widget also requires a backing store.

    \sa QBackingStore
    \sa createPlatformWindow()
*/

/*!
    \enum QPlatformIntegration::Capability

    Capabilities are used to determing specific features of a platform integration

    \value ThreadedPixmaps The platform uses a pixmap implementation that is reentrant
    and can be used from multiple threads, like the raster paint engine and QImage based
    pixmaps.

    \value OpenGL The platform supports OpenGL

    \value ThreadedOpenGL The platform supports using OpenGL outside the GUI thread.

    \value SharedGraphicsCache The platform supports a shared graphics cache

    \value BufferQueueingOpenGL The OpenGL implementation on the platform will queue
    up buffers when swapBuffers() is called and block only when its buffer pipeline
    is full, rather than block immediately.

    \value MultipleWindows The platform supports multiple QWindows, i.e. does some kind
    of compositing either client or server side. Some platforms might only support a
    single fullscreen window.

    \value ApplicationState The platform handles the application state explicitly.
    This means that QEvent::ApplicationActivate and QEvent::ApplicationDeativate
    will not be posted automatically. Instead, the platform must handle application
    state explicitly by using QWindowSystemInterface::handleApplicationStateChanged().
    If not set, application state will follow window activation, which is the normal
    behavior for desktop platforms.

    \value ForeignWindows The platform allows creating QWindows which represent
    native windows created by other processes or anyway created by using native
    libraries.

    \value NonFullScreenWindows The platform supports top-level windows which do not
    fill the screen. The default implementation returns \c true. Returning false for
    this will cause all windows, including dialogs and popups, to be resized to fill the
    screen.

    \value WindowManagement The platform is based on a system that performs window
    management.  This includes the typical desktop platforms. Can be set to false on
    platforms where no window management is available, meaning for example that windows
    are never repositioned by the window manager. The default implementation returns \c true.

    \value AllGLFunctionsQueryable The QOpenGLContext backend provided by the platform is
    able to return function pointers from getProcAddress() even for standard OpenGL
    functions, for example OpenGL 1 functions like glClear() or glDrawArrays(). This is
    important because the OpenGL specifications do not require this ability from the
    getProcAddress implementations of the windowing system interfaces (EGL, WGL, GLX). The
    platform plugins may however choose to enhance the behavior in the backend
    implementation for QOpenGLContext::getProcAddress() and support returning a function
    pointer also for the standard, non-extension functions. This capability is a
    prerequisite for dynamic OpenGL loading.
 */

/*!

    \fn QAbstractEventDispatcher *QPlatformIntegration::createEventDispatcher() const = 0

    Factory function for the GUI event dispatcher. The platform plugin should create
    and return a QAbstractEventDispatcher subclass when this function is called.

    If the platform plugin for some reason creates the event dispatcher outside of
    this function (for example in the constructor), it needs to handle the case
    where this function is never called, ensuring that the event dispatcher is
    still deleted at some point (typically in the destructor).

    Note that the platform plugin should never explicitly set the event dispatcher
    itself, using QCoreApplication::setEventDispatcher(), but let QCoreApplication
    decide when and which event dispatcher to create.

    \since 5.2
*/

bool QPlatformIntegration::hasCapability(Capability cap) const
{
    return cap == NonFullScreenWindows || cap == NativeWidgets || cap == WindowManagement;
}

QPlatformPixmap *QPlatformIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    return new QRasterPlatformPixmap(type);
}

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QPlatformIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    Q_UNUSED(context);
    qWarning("This plugin does not support createPlatformOpenGLContext!");
    return 0;
}
#endif

/*!
   Factory function for QPlatformSharedGraphicsCache. This function will return 0 if the platform
   integration does not support any shared graphics cache mechanism for the given \a cacheId.
*/
QPlatformSharedGraphicsCache *QPlatformIntegration::createPlatformSharedGraphicsCache(const char *cacheId) const
{
    qWarning("This plugin does not support createPlatformSharedGraphicsBuffer for cacheId: %s!",
             cacheId);
    return 0;
}

/*!
   Factory function for QPaintEngine. This function will return 0 if the platform
   integration does not support creating any paint engine the given \a paintDevice.
*/
QPaintEngine *QPlatformIntegration::createImagePaintEngine(QPaintDevice *paintDevice) const
{
    Q_UNUSED(paintDevice)
    return 0;
}

/*!
  Performs initialization steps that depend on having an event dispatcher
  available. Called after the event dispatcher has been created.

  Tasks that require an event dispatcher, for example creating socket notifiers, cannot be
  performed in the constructor. Instead, they should be performed here. The default
  implementation does nothing.
*/
void QPlatformIntegration::initialize()
{
}

/*!
  Returns the platforms input context.

  The default implementation returns 0, implying no input method support.
*/
QPlatformInputContext *QPlatformIntegration::inputContext() const
{
    return 0;
}

#ifndef QT_NO_ACCESSIBILITY

/*!
  Returns the platforms accessibility.

  The default implementation returns 0, implying no accessibility support.
*/
QPlatformAccessibility *QPlatformIntegration::accessibility() const
{
    return 0;
}

#endif

QVariant QPlatformIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case CursorFlashTime:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::CursorFlashTime);
    case KeyboardInputInterval:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::KeyboardInputInterval);
    case KeyboardAutoRepeatRate:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::KeyboardAutoRepeatRate);
    case MouseDoubleClickInterval:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::MouseDoubleClickInterval);
    case StartDragDistance:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::StartDragDistance);
    case StartDragTime:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::StartDragTime);
    case ShowIsFullScreen:
        return false;
    case ShowIsMaximized:
        return false;
    case PasswordMaskDelay:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::PasswordMaskDelay);
    case PasswordMaskCharacter:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::PasswordMaskCharacter);
    case FontSmoothingGamma:
        return qreal(1.7);
    case StartDragVelocity:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::StartDragVelocity);
    case UseRtlExtensions:
        return QVariant(false);
    case SynthesizeMouseFromTouchEvents:
        return true;
    case SetFocusOnTouchRelease:
        return QVariant(false);
    case MousePressAndHoldInterval:
        return QPlatformTheme::defaultThemeHint(QPlatformTheme::MousePressAndHoldInterval);
    }

    return 0;
}

Qt::WindowState QPlatformIntegration::defaultWindowState(Qt::WindowFlags flags) const
{
    // Leave popup-windows as is
    if (flags & Qt::Popup & ~Qt::Window)
        return Qt::WindowNoState;

    if (styleHint(QPlatformIntegration::ShowIsFullScreen).toBool())
        return Qt::WindowFullScreen;
    else if (styleHint(QPlatformIntegration::ShowIsMaximized).toBool())
        return Qt::WindowMaximized;

    return Qt::WindowNoState;
}

Qt::KeyboardModifiers QPlatformIntegration::queryKeyboardModifiers() const
{
    return QGuiApplication::keyboardModifiers();
}

/*!
  Should be used to obtain a list of possible shortcuts for the given key
  event. As that needs system functionality it cannot be done in qkeymapper.

  One example for more than 1 possibility is the key combination of Shift+5.
  That one might trigger a shortcut which is set as "Shift+5" as well as one
  using %. These combinations depend on the currently set keyboard layout
  which cannot be obtained by Qt functionality.
*/
QList<int> QPlatformIntegration::possibleKeys(const QKeyEvent *) const
{
    return QList<int>();
}

/*!
  Should be called by the implementation whenever a new screen is added.

  The first screen added will be the primary screen, used for default-created
  windows, GL contexts, and other resources unless otherwise specified.

  This adds the screen to QGuiApplication::screens(), and emits the
  QGuiApplication::screenAdded() signal.

  The screen is automatically removed when the QPlatformScreen is destroyed.
*/
void QPlatformIntegration::screenAdded(QPlatformScreen *ps)
{
    QScreen *screen = new QScreen(ps);
    ps->d_func()->screen = screen;
    QGuiApplicationPrivate::screen_list << screen;
    emit qGuiApp->screenAdded(screen);
}

QStringList QPlatformIntegration::themeNames() const
{
    return QStringList();
}

class QPlatformTheme *QPlatformIntegration::createPlatformTheme(const QString &name) const
{
    Q_UNUSED(name)
    return new QPlatformTheme;
}

/*!
   Factory function for QOffscreenSurface. An offscreen surface will typically be implemented with a
   pixel buffer (pbuffer). If the platform doesn't support offscreen surfaces, an invisible window
   will be used by QOffscreenSurface instead.
*/
QPlatformOffscreenSurface *QPlatformIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    Q_UNUSED(surface)
    return 0;
}

#ifndef QT_NO_SESSIONMANAGER
/*!
   \since 5.2

   Factory function for QPlatformSessionManager. The default QPlatformSessionManager provides the same
   functionality as the QSessionManager.
*/
QPlatformSessionManager *QPlatformIntegration::createPlatformSessionManager(const QString &id, const QString &key) const
{
    return new QPlatformSessionManager(id, key);
}
#endif

/*!
   \since 5.2

   Function to sync the platform integrations state with the window system.

   This is often implemented as a roundtrip from the platformintegration to the window system.

   This function should not call QWindowSystemInterface::flushWindowSystemEvents() or
   QCoreApplication::processEvents()
*/
void QPlatformIntegration::sync()
{
}

#ifndef QT_NO_OPENGL
/*!
  Platform integration function for querying the OpenGL implementation type.

  Used only when dynamic OpenGL implementation loading is enabled.

  Subclasses should reimplement this function and return a value based on
  the OpenGL implementation they have chosen to load.

  \note The return value does not indicate or limit the types of
  contexts that can be created by a given implementation. For example
  a desktop OpenGL implementation may be capable of creating OpenGL
  ES-compatible contexts too.

  \sa QOpenGLContext::openGLModuleType(), QOpenGLContext::isOpenGLES()

  \since 5.3
 */
QOpenGLContext::OpenGLModuleType QPlatformIntegration::openGLModuleType()
{
    qWarning("This plugin does not support dynamic OpenGL loading!");
    return QOpenGLContext::LibGL;
}
#endif

QT_END_NAMESPACE

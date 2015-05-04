/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsintegration.h"
#include "qwindowsscaling.h"
#include "qwindowswindow.h"
#include "qwindowscontext.h"
#include "qwindowsopenglcontext.h"

#include "qwindowsscreen.h"
#include "qwindowstheme.h"
#include "qwindowsservices.h"
#ifndef QT_NO_FREETYPE
#  include "qwindowsfontdatabase_ft.h"
#endif
#include "qwindowsfontdatabase.h"
#include "qwindowsguieventdispatcher.h"
#ifndef QT_NO_CLIPBOARD
#  include "qwindowsclipboard.h"
#  ifndef QT_NO_DRAGANDDROP
#    include "qwindowsdrag.h"
#  endif
#endif
#include "qwindowsinputcontext.h"
#include "qwindowskeymapper.h"
#ifndef QT_NO_ACCESSIBILITY
#  include "accessible/qwindowsaccessibility.h"
#endif

#include <qpa/qplatformnativeinterface.h>
#include <qpa/qwindowsysteminterface.h>
#if !defined(Q_OS_WINCE) && !defined(QT_NO_SESSIONMANAGER)
#  include "qwindowssessionmanager.h"
#endif
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatforminputcontextfactory_p.h>

#include <QtCore/private/qeventdispatcher_win_p.h>
#include <QtCore/QDebug>
#include <QtCore/QVariant>

#include <limits.h>

#if defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_DYNAMIC)
#  include "qwindowseglcontext.h"
#endif
#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
#  include "qwindowsglcontext.h"
#endif

#ifndef Q_OS_WINCE
#  include "qwindowsopengltester.h"
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsIntegration
    \brief QPlatformIntegration implementation for Windows.
    \internal

    \section1 Programming Considerations

    The platform plugin should run on Desktop Windows from Windows XP onwards
    and Windows Embedded.

    It should compile with:
    \list
    \li Microsoft Visual Studio 2008 or later (using the Microsoft Windows SDK,
        (\c Q_CC_MSVC).
    \li Stock \l{http://mingw.org/}{MinGW} (\c Q_CC_MINGW).
        This version ships with headers that are missing a lot of WinAPI.
    \li MinGW distributions using GCC 4.7 or higher and a recent MinGW-w64 runtime API,
        such as \l{http://tdm-gcc.tdragon.net/}{TDM-GCC}, or
        \l{http://mingwbuilds.sourceforge.net/}{MinGW-builds}
        (\c Q_CC_MINGW and \c __MINGW64_VERSION_MAJOR indicating the version).
        MinGW-w64 provides more complete headers (compared to stock MinGW from mingw.org),
        including a considerable part of the Windows SDK.
    \li Visual Studio 2008 for Windows Embedded (\c Q_OS_WINCE).
    \endlist

    The file \c qtwindows_additional.h contains defines and declarations that
    are missing in MinGW. When encountering missing declarations, it should
    be added there so that \c #ifdefs for MinGW can be avoided. Similarly,
    \c qplatformfunctions_wince.h contains defines and declarations for
    Windows Embedded.

    When using a function from the WinAPI, the minimum supported Windows version
    and Windows Embedded support should be checked. If the function is not supported
    on Windows XP or is not present in the MinGW-headers, it should be dynamically
    resolved. For this purpose, QWindowsContext has static structs like
    QWindowsUser32DLL and QWindowsShell32DLL. All function pointers should go to
    these structs to avoid lookups in several places.

    \ingroup qt-lighthouse-win
*/

struct QWindowsIntegrationPrivate
{
    explicit QWindowsIntegrationPrivate(const QStringList &paramList);
    ~QWindowsIntegrationPrivate();

    unsigned m_options;
    QWindowsContext m_context;
    QPlatformFontDatabase *m_fontDatabase;
#ifndef QT_NO_CLIPBOARD
    QWindowsClipboard m_clipboard;
#  ifndef QT_NO_DRAGANDDROP
    QWindowsDrag m_drag;
#  endif
#endif
#ifndef QT_NO_OPENGL
    QSharedPointer<QWindowsStaticOpenGLContext> m_staticOpenGLContext;
#endif // QT_NO_OPENGL
    QScopedPointer<QPlatformInputContext> m_inputContext;
#ifndef QT_NO_ACCESSIBILITY
    QWindowsAccessibility m_accessibility;
#endif
    QWindowsServices m_services;
};

template <typename IntType>
bool parseIntOption(const QString &parameter,const QLatin1String &option,
                    IntType minimumValue, IntType maximumValue, IntType *target)
{
    const int valueLength = parameter.size() - option.size() - 1;
    if (valueLength < 1 || !parameter.startsWith(option) || parameter.at(option.size()) != QLatin1Char('='))
        return false;
    bool ok;
    const QStringRef valueRef = parameter.rightRef(valueLength);
    const int value = valueRef.toInt(&ok);
    if (ok) {
        if (value >= minimumValue && value <= maximumValue)
            *target = static_cast<IntType>(value);
        else {
            qWarning() << "Value" << value << "for option" << option << "out of range"
                << minimumValue << ".." << maximumValue;
        }
    } else {
        qWarning() << "Invalid value" << valueRef << "for option" << option;
    }
    return true;
}

static inline unsigned parseOptions(const QStringList &paramList,
                                    int *tabletAbsoluteRange,
                                    QtWindows::ProcessDpiAwareness *dpiAwareness)
{
    unsigned options = 0;
    foreach (const QString &param, paramList) {
        if (param.startsWith(QLatin1String("fontengine="))) {
            if (param.endsWith(QLatin1String("freetype"))) {
                options |= QWindowsIntegration::FontDatabaseFreeType;
            } else if (param.endsWith(QLatin1String("native"))) {
                options |= QWindowsIntegration::FontDatabaseNative;
            }
        } else if (param.startsWith(QLatin1String("dialogs="))) {
            if (param.endsWith(QLatin1String("xp"))) {
                options |= QWindowsIntegration::XpNativeDialogs;
            } else if (param.endsWith(QLatin1String("none"))) {
                options |= QWindowsIntegration::NoNativeDialogs;
            }
        } else if (param == QLatin1String("gl=gdi")) {
            options |= QWindowsIntegration::DisableArb;
        } else if (param == QLatin1String("nomousefromtouch")) {
            options |= QWindowsIntegration::DontPassOsMouseEventsSynthesizedFromTouch;
        } else if (parseIntOption(param, QLatin1String("verbose"), 0, INT_MAX, &QWindowsContext::verbose)
            || parseIntOption(param, QLatin1String("tabletabsoluterange"), 0, INT_MAX, tabletAbsoluteRange)
            || parseIntOption(param, QLatin1String("dpiawareness"), QtWindows::ProcessDpiUnaware, QtWindows::ProcessPerMonitorDpiAware, dpiAwareness)) {
        } else {
            qWarning() << "Unknown option" << param;
        }
    }
    return options;
}

QWindowsIntegrationPrivate::QWindowsIntegrationPrivate(const QStringList &paramList)
    : m_options(0)
    , m_fontDatabase(0)
{
    static bool dpiAwarenessSet = false;
    int tabletAbsoluteRange = -1;
    // Default to per-monitor awareness to avoid being scaled when monitors with different DPI
    // are connected to Windows 8.1
    QtWindows::ProcessDpiAwareness dpiAwareness = QtWindows::ProcessPerMonitorDpiAware;
    m_options = parseOptions(paramList, &tabletAbsoluteRange, &dpiAwareness);
    if (tabletAbsoluteRange >= 0)
        m_context.setTabletAbsoluteRange(tabletAbsoluteRange);
    if (!dpiAwarenessSet) { // Set only once in case of repeated instantiations of QGuiApplication.
        m_context.setProcessDpiAwareness(dpiAwareness);
        dpiAwarenessSet = true;
    }
    // Determine suitable scale factor, don't mix Windows and Qt scaling
    if (dpiAwareness != QtWindows::ProcessDpiUnaware)
        QWindowsScaling::setFactor(QWindowsScaling::determineUiScaleFactor());
    qCDebug(lcQpaWindows)
        << __FUNCTION__ << "DpiAwareness=" << dpiAwareness <<",Scaling="
        << QWindowsScaling::factor();
}

QWindowsIntegrationPrivate::~QWindowsIntegrationPrivate()
{
    if (m_fontDatabase)
        delete m_fontDatabase;
}

QWindowsIntegration::QWindowsIntegration(const QStringList &paramList) :
    d(new QWindowsIntegrationPrivate(paramList))
{
#ifndef QT_NO_CLIPBOARD
    d->m_clipboard.registerViewer();
#endif
    d->m_context.screenManager().handleScreenChanges();
}

QWindowsIntegration::~QWindowsIntegration()
{
}

void QWindowsIntegration::initialize()
{
    if (QPlatformInputContext *pluginContext = QPlatformInputContextFactory::create())
        d->m_inputContext.reset(pluginContext);
    else
        d->m_inputContext.reset(new QWindowsInputContext);
}

bool QWindowsIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
        return true;
#ifndef QT_NO_OPENGL
    case OpenGL:
        return true;
    case ThreadedOpenGL:
        if (const QWindowsStaticOpenGLContext *glContext = QWindowsIntegration::staticOpenGLContext())
            return glContext->supportsThreadedOpenGL();
        return false;
#endif // !QT_NO_OPENGL
    case WindowMasks:
        return true;
    case MultipleWindows:
        return true;
    case ForeignWindows:
        return true;
    case RasterGLSurface:
        return true;
    case AllGLFunctionsQueryable:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
    return false;
}

QWindowsWindowData QWindowsIntegration::createWindowData(QWindow *window) const
{
    QWindowsWindowData requested;
    requested.flags = window->flags();
    requested.geometry = QWindowsScaling::mapToNative(window->geometry());
    // Apply custom margins (see  QWindowsWindow::setCustomMargins())).
    const QVariant customMarginsV = window->property("_q_windowsCustomMargins");
    if (customMarginsV.isValid())
        requested.customMargins = qvariant_cast<QMargins>(customMarginsV);

    QWindowsWindowData obtained = QWindowsWindowData::create(window, requested, window->title());
    qCDebug(lcQpaWindows).nospace()
        << __FUNCTION__ << '<' << window
        << "\n    Requested: " << requested.geometry << "frame incl.: "
        << QWindowsGeometryHint::positionIncludesFrame(window)
        << " Flags=" << QWindowsWindow::debugWindowFlags(requested.flags)
        << "\n    Obtained : " << obtained.geometry << " Margins "<< obtained.frame
        << " Flags=" << QWindowsWindow::debugWindowFlags(obtained.flags)
        << " Handle=" << obtained.hwnd << '\n';

    if (obtained.hwnd) {
        if (requested.flags != obtained.flags)
            window->setFlags(obtained.flags);
        // Trigger geometry change signals of QWindow.
        if ((obtained.flags & Qt::Desktop) != Qt::Desktop && requested.geometry != obtained.geometry)
            QWindowSystemInterface::handleGeometryChange(window, QWindowsScaling::mapFromNative(obtained.geometry));
    }

    return obtained;
}

QPlatformWindow *QWindowsIntegration::createPlatformWindow(QWindow *window) const
{
    QWindowsWindowData data = createWindowData(window);
    return data.hwnd ? new QWindowsWindow(window, data)
                     : Q_NULLPTR;
}

#ifndef QT_NO_OPENGL

QWindowsStaticOpenGLContext *QWindowsStaticOpenGLContext::doCreate()
{
#if defined(QT_OPENGL_DYNAMIC)
    QWindowsOpenGLTester::Renderer requestedRenderer = QWindowsOpenGLTester::requestedRenderer();
    switch (requestedRenderer) {
    case QWindowsOpenGLTester::DesktopGl:
        if (QWindowsStaticOpenGLContext *glCtx = QOpenGLStaticContext::create())
            return glCtx;
        qCWarning(lcQpaGl, "System OpenGL failed. Falling back to Software OpenGL.");
        return QOpenGLStaticContext::create(true);
    // If ANGLE is requested, use it, don't try anything else.
    case QWindowsOpenGLTester::AngleRendererD3d9:
    case QWindowsOpenGLTester::AngleRendererD3d11:
    case QWindowsOpenGLTester::AngleRendererD3d11Warp:
        return QWindowsEGLStaticContext::create(requestedRenderer);
    case QWindowsOpenGLTester::Gles:
        return QWindowsEGLStaticContext::create(QWindowsOpenGLTester::supportedGlesRenderers());
    case QWindowsOpenGLTester::SoftwareRasterizer:
        if (QWindowsStaticOpenGLContext *swCtx = QOpenGLStaticContext::create(true))
            return swCtx;
        qCWarning(lcQpaGl, "Software OpenGL failed. Falling back to system OpenGL.");
        if (QWindowsOpenGLTester::supportedRenderers() & QWindowsOpenGLTester::DesktopGl)
            return QOpenGLStaticContext::create();
        return Q_NULLPTR;
    default:
        break;
    }

    const QWindowsOpenGLTester::Renderers supportedRenderers = QWindowsOpenGLTester::supportedRenderers();
    if (supportedRenderers & QWindowsOpenGLTester::DesktopGl) {
        if (QWindowsStaticOpenGLContext *glCtx = QOpenGLStaticContext::create())
            return glCtx;
    }
    if (QWindowsOpenGLTester::Renderers glesRenderers = supportedRenderers & QWindowsOpenGLTester::GlesMask) {
        if (QWindowsEGLStaticContext *eglCtx = QWindowsEGLStaticContext::create(glesRenderers))
            return eglCtx;
    }
    return QOpenGLStaticContext::create(true);
#elif defined(QT_OPENGL_ES_2)
    QWindowsOpenGLTester::Renderers glesRenderers = QWindowsOpenGLTester::requestedGlesRenderer();
    if (glesRenderers == QWindowsOpenGLTester::InvalidRenderer)
        glesRenderers = QWindowsOpenGLTester::supportedGlesRenderers();
    return QWindowsEGLStaticContext::create(glesRenderers);
#elif !defined(QT_NO_OPENGL)
    return QOpenGLStaticContext::create();
#endif
}

QWindowsStaticOpenGLContext *QWindowsStaticOpenGLContext::create()
{
    return QWindowsStaticOpenGLContext::doCreate();
}

QPlatformOpenGLContext *QWindowsIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    qCDebug(lcQpaGl) << __FUNCTION__ << context->format();
    if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext()) {
        QScopedPointer<QWindowsOpenGLContext> result(staticOpenGLContext->createContext(context));
        if (result->isValid())
            return result.take();
    }
    return 0;
}

QOpenGLContext::OpenGLModuleType QWindowsIntegration::openGLModuleType()
{
#if defined(QT_OPENGL_ES_2)
    return QOpenGLContext::LibGLES;
#elif !defined(QT_OPENGL_DYNAMIC)
    return QOpenGLContext::LibGL;
#else
    if (const QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
        return staticOpenGLContext->moduleType();
    return QOpenGLContext::LibGL;
#endif
}

QWindowsStaticOpenGLContext *QWindowsIntegration::staticOpenGLContext()
{
    QWindowsIntegrationPrivate *d = QWindowsIntegration::instance()->d.data();
    if (d->m_staticOpenGLContext.isNull())
        d->m_staticOpenGLContext = QSharedPointer<QWindowsStaticOpenGLContext>(QWindowsStaticOpenGLContext::create());
    return d->m_staticOpenGLContext.data();
}
#endif // !QT_NO_OPENGL

/* Workaround for QTBUG-24205: In 'Auto', pick the FreeType engine for
 * QML2 applications. */

#ifdef Q_OS_WINCE
// It's not easy to detect if we are running a QML application
// Let's try to do so by checking if the Qt Quick module is loaded.
inline bool isQMLApplication()
{
    // check if the Qt Quick module is loaded
#ifdef _DEBUG
    HMODULE handle = GetModuleHandle(L"Qt5Quick" QT_LIBINFIX L"d.dll");
#else
    HMODULE handle = GetModuleHandle(L"Qt5Quick" QT_LIBINFIX L".dll");
#endif
    return (handle != NULL);
}
#endif

QPlatformFontDatabase *QWindowsIntegration::fontDatabase() const
{
    if (!d->m_fontDatabase) {
#ifdef QT_NO_FREETYPE
        d->m_fontDatabase = new QWindowsFontDatabase();
#else // QT_NO_FREETYPE
        if (d->m_options & QWindowsIntegration::FontDatabaseFreeType) {
            d->m_fontDatabase = new QWindowsFontDatabaseFT;
        } else if (d->m_options & QWindowsIntegration::FontDatabaseNative){
            d->m_fontDatabase = new QWindowsFontDatabase;
        } else {
#ifndef Q_OS_WINCE
            d->m_fontDatabase = new QWindowsFontDatabase;
#else
            if (isQMLApplication()) {
                qCDebug(lcQpaFonts) << "QML application detected, using FreeType rendering";
                d->m_fontDatabase = new QWindowsFontDatabaseFT;
            }
            else
                d->m_fontDatabase = new QWindowsFontDatabase;
#endif
        }
#endif // QT_NO_FREETYPE
    }
    return d->m_fontDatabase;
}

#ifdef SPI_GETKEYBOARDSPEED
static inline int keyBoardAutoRepeatRateMS()
{
  DWORD time = 0;
  if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &time, 0))
      return time ? 1000 / static_cast<int>(time) : 500;
  return 30;
}
#endif

QVariant QWindowsIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    switch (hint) {
    case QPlatformIntegration::CursorFlashTime:
        if (const unsigned timeMS = GetCaretBlinkTime())
            return QVariant(int(timeMS) * 2);
        break;
#ifdef SPI_GETKEYBOARDSPEED
    case KeyboardAutoRepeatRate:
        return QVariant(keyBoardAutoRepeatRateMS());
#endif
    case QPlatformIntegration::StartDragTime:
    case QPlatformIntegration::StartDragDistance:
    case QPlatformIntegration::KeyboardInputInterval:
    case QPlatformIntegration::ShowIsFullScreen:
    case QPlatformIntegration::PasswordMaskDelay:
    case QPlatformIntegration::StartDragVelocity:
        break; // Not implemented
    case QPlatformIntegration::FontSmoothingGamma:
        return QVariant(QWindowsFontDatabase::fontSmoothingGamma());
    case QPlatformIntegration::MouseDoubleClickInterval:
        if (const int ms = GetDoubleClickTime())
            return QVariant(ms);
        break;
    case QPlatformIntegration::UseRtlExtensions:
        return QVariant(d->m_context.useRTLExtensions());
    case QPlatformIntegration::SynthesizeMouseFromTouchEvents:
#ifdef Q_OS_WINCE
        // We do not want Qt to synthesize mouse events as Windows also does that.
       return false;
#else // Q_OS_WINCE
       return QVariant(bool(d->m_options & DontPassOsMouseEventsSynthesizedFromTouch));
#endif // !Q_OS_WINCE
    default:
        break;
    }
    return QPlatformIntegration::styleHint(hint);
}

Qt::KeyboardModifiers QWindowsIntegration::queryKeyboardModifiers() const
{
    return QWindowsKeyMapper::queryKeyboardModifiers();
}

QList<int> QWindowsIntegration::possibleKeys(const QKeyEvent *e) const
{
    return d->m_context.possibleKeys(e);
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard * QWindowsIntegration::clipboard() const
{
    return &d->m_clipboard;
}
#  ifndef QT_NO_DRAGANDDROP
QPlatformDrag *QWindowsIntegration::drag() const
{
    return &d->m_drag;
}
#  endif // !QT_NO_DRAGANDDROP
#endif // !QT_NO_CLIPBOARD

QPlatformInputContext * QWindowsIntegration::inputContext() const
{
    return d->m_inputContext.data();
}

#ifndef QT_NO_ACCESSIBILITY
QPlatformAccessibility *QWindowsIntegration::accessibility() const
{
    return &d->m_accessibility;
}
#endif

QWindowsIntegration *QWindowsIntegration::instance()
{
    return static_cast<QWindowsIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

unsigned QWindowsIntegration::options() const
{
    return d->m_options;
}

#if !defined(Q_OS_WINCE) && !defined(QT_NO_SESSIONMANAGER)
QPlatformSessionManager *QWindowsIntegration::createPlatformSessionManager(const QString &id, const QString &key) const
{
    return new QWindowsSessionManager(id, key);
}
#endif

QAbstractEventDispatcher * QWindowsIntegration::createEventDispatcher() const
{
    return new QWindowsGuiEventDispatcher;
}

QStringList QWindowsIntegration::themeNames() const
{
    return QStringList(QLatin1String(QWindowsTheme::name));
}

QPlatformTheme *QWindowsIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String(QWindowsTheme::name))
        return new QWindowsTheme;
    return QPlatformIntegration::createPlatformTheme(name);
}

QPlatformServices *QWindowsIntegration::services() const
{
    return &d->m_services;
}

QT_END_NAMESPACE

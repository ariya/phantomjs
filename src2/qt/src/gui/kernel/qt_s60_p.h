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

#ifndef QT_S60_P_H
#define QT_S60_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qwindowdefs.h"
#include "private/qcore_symbian_p.h"
#include "qhash.h"
#include "qpoint.h"
#include "QtGui/qfont.h"
#include "QtGui/qimage.h"
#include "QtGui/qevent.h"
#include "qpointer.h"
#include "qapplication.h"
#include "qelapsedtimer.h"
#include "QtCore/qthreadstorage.h"
#include "qwidget_p.h"
#include <w32std.h>
#include <coecntrl.h>
#include <eikenv.h>
#include <eikappui.h>

#ifdef Q_WS_S60
#include <AknUtils.h>               // AknLayoutUtils
#include <avkon.hrh>                // EEikStatusPaneUidTitle
#include <akntitle.h>               // CAknTitlePane
#include <akncontext.h>             // CAknContextPane
#include <eikspane.h>               // CEikStatusPane
#include <AknPopupFader.h>          // MAknFadedComponent and TAknPopupFader
#include <bitstd.h>                 // EGraphicsOrientation constants
#ifdef QT_SYMBIAN_HAVE_AKNTRANSEFFECT_H
#include <gfxtranseffect/gfxtranseffect.h> // BeginFullScreen
#include <akntranseffect.h> // BeginFullScreen
#endif
#endif

QT_BEGIN_NAMESPACE

// Application internal HandleResourceChangeL events,
// system events seems to start with 0x10
const TInt KInternalStatusPaneChange = 0x50000000;

// For BeginFullScreen().
const TUint KQtAppExitFlag = 0x400;

static const int qt_symbian_max_screens = 4;

//this macro exists because EColor16MAP enum value doesn't exist in Symbian OS 9.2
#define Q_SYMBIAN_ECOLOR16MAP TDisplayMode(13)

class QSymbianTypeFaceExtras;
typedef QHash<QString, const QSymbianTypeFaceExtras *> QSymbianTypeFaceExtrasHash;
typedef void (*QThreadLocalReleaseFunc)();

#ifdef COE_GROUPED_POINTER_EVENT_VERSION
class CCoeEventData;
#endif

class Q_AUTOTEST_EXPORT QS60ThreadLocalData
{
public:
    QS60ThreadLocalData();
    ~QS60ThreadLocalData();
    bool usingCONEinstances;
    RWsSession wsSession;
    CWsScreenDevice *screenDevice;
    QSymbianTypeFaceExtrasHash fontData;
    QVector<QThreadLocalReleaseFunc> releaseFuncs;
};

class QS60Data
{
public:
    QS60Data();
    QThreadStorage<QS60ThreadLocalData *> tls;
    TUid uid;
    int screenDepth;
    QPoint lastCursorPos;
    QPoint lastPointerEventPos;
    QPointer<QWidget> lastPointerEventTarget;
    QPointer<QWidget> mousePressTarget;
    int screenWidthInPixels;
    int screenHeightInPixels;
    int screenWidthInTwips;
    int screenHeightInTwips;
    int defaultDpiX;
    int defaultDpiY;
    WId curWin;
    enum PressedKeys {
        Select = 0x1,
        Right = 0x2,
        Down = 0x4,
        Left = 0x8,
        Up = 0x10,
        LeftUp = 0x20,
        RightUp = 0x40,
        RightDown = 0x80,
        LeftDown = 0x100
    };
    int virtualMousePressedKeys; // of the above type, but avoids casting problems
    int virtualMouseAccelDX;
    int virtualMouseAccelDY;
    QElapsedTimer virtualMouseAccelTimeout;
    int virtualMouseMaxAccel;
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
    int brokenPointerCursors : 1;
#endif
    int hasTouchscreen : 1;
    int mouseInteractionEnabled : 1;
    int virtualMouseRequired : 1;
    int qtOwnsS60Environment : 1;
    int supportsPremultipliedAlpha : 1;
    int avkonComponentsSupportTransparency : 1;
    int menuBeingConstructed : 1;
    int orientationSet : 1;
    int partial_keyboard : 1;
    int partial_keyboardAutoTranslation : 1;
    int partialKeyboardOpen : 1;
    int handleStatusPaneResizeNotifications : 1;
    int screenFurnitureFullyCreated : 1;
    int beginFullScreenCalled : 1;
    int endFullScreenCalled : 1;
    int eglSurfaceCreationError : 1;
    QApplication::QS60MainApplicationFactory s60ApplicationFactory; // typedef'ed pointer type
    QPointer<QWidget> splitViewLastWidget;

    static CEikButtonGroupContainer *cba;

    enum ScanCodeState {
        Unpressed,
        KeyDown,
        KeyDownAndKey
    };
    QHash<TInt, ScanCodeState> scanCodeStates;

    static inline void updateScreenSize();
    inline RWsSession& wsSession();
    static inline int screenCount();
    static inline RWindowGroup& windowGroup();
    static inline RWindowGroup& windowGroup(const QWidget *widget);
    static inline RWindowGroup& windowGroup(int screenNumber);
    inline CWsScreenDevice* screenDevice();
    inline CWsScreenDevice* screenDevice(const QWidget *widget);
    inline CWsScreenDevice* screenDevice(int screenNumber);
    static inline int screenNumberForWidget(const QWidget *widget);
    inline QSymbianTypeFaceExtrasHash& fontData();
    inline void addThreadLocalReleaseFunc(QThreadLocalReleaseFunc func);
    static inline CCoeAppUi* appUi();
    static inline CEikMenuBar* menuBar();
#ifdef Q_WS_S60
    static inline CEikStatusPane* statusPane();
    static inline CCoeControl* statusPaneSubPane(TInt aPaneId);
    static inline CAknTitlePane* titlePane();
    static inline CAknContextPane* contextPane();
    static inline CEikButtonGroupContainer* buttonGroupContainer();
    static inline void setButtonGroupContainer(CEikButtonGroupContainer* newCba);
    static void setStatusPaneAndButtonGroupVisibility(bool statusPaneVisible, bool buttonGroupVisible);
    static bool setRecursiveDecorationsVisibility(QWidget *window, Qt::WindowStates newState);
    static void createStatusPaneAndCBA();
#endif
    static void controlVisibilityChanged(CCoeControl *control, bool visible);
    static TRect clientRect();

    TTrapHandler *s60InstalledTrapHandler;

    int screenWidthInPixelsForScreen[qt_symbian_max_screens];
    int screenHeightInPixelsForScreen[qt_symbian_max_screens];
    int screenWidthInTwipsForScreen[qt_symbian_max_screens];
    int screenHeightInTwipsForScreen[qt_symbian_max_screens];

    int nativeScreenWidthInPixels;
    int nativeScreenHeightInPixels;

    enum ScreenRotation {
        ScreenRotation0, // portrait (or the native orientation)
        ScreenRotation90, // typically DisplayLeftUp landscape
        ScreenRotation180, // not used
        ScreenRotation270 // DisplayRightUp landscape when 3-way orientation is supported
    };
    ScreenRotation screenRotation;

    int editorFlags;
};

Q_AUTOTEST_EXPORT QS60Data* qGlobalS60Data();
#define S60 qGlobalS60Data()

class QAbstractLongTapObserver
{
public:
    virtual void HandleLongTapEventL( const TPoint& aPenEventLocation,
                                      const TPoint& aPenEventScreenLocation ) = 0;
};
class QLongTapTimer;


class QSymbianControl : public CCoeControl, public QAbstractLongTapObserver
#ifdef Q_WS_S60
, public MAknFadedComponent, public MEikStatusPaneObserver
#endif
{
public:
    DECLARE_TYPE_ID(0x51740000) // Fun fact: the two first values are "Qt" in ASCII.

public:
    QSymbianControl(QWidget *w);
    void ConstructL(bool isWindowOwning = false, bool desktop = false);
    ~QSymbianControl();
    void HandleResourceChange(int resourceType);
    void HandlePointerEventL(const TPointerEvent& aPointerEvent);
    TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
#if !defined(QT_NO_IM) && defined(Q_WS_S60)
    TCoeInputCapabilities InputCapabilities() const;
#endif
    TTypeUid::Ptr MopSupplyObject(TTypeUid id);

    inline QWidget* widget() const { return qwidget; }
    void setWidget(QWidget *w);
    void sendInputEvent(QWidget *widget, QInputEvent *inputEvent);
    void setIgnoreFocusChanged(bool enabled) { m_ignoreFocusChanged = enabled; }
    void CancelLongTapTimer();

    void setFocusSafely(bool focus);

    bool isControlActive();

    void ensureFixNativeOrientation();
    enum TTranslationType { ETranslatePixelCenter, ETranslatePixelEdge };
    QPoint translatePointForFixedNativeOrientation(const TPoint &pointerEventPos, TTranslationType translationType) const;
    TRect translateRectForFixedNativeOrientation(const TRect &controlRect) const;

#ifdef Q_WS_S60
    void FadeBehindPopup(bool fade){ popupFader.FadeBehindPopup( this, this, fade); }
    void HandleStatusPaneSizeChange();

protected: // from MAknFadedComponent
    TInt CountFadedComponents() {return 1;}
    CCoeControl* FadedComponent(TInt /*aIndex*/) {return this;}
#else
    // #warning No fallback implementation for QSymbianControl::FadeBehindPopup
    void FadeBehindPopup(bool /*fade*/){ }
#endif

protected:
    void Draw(const TRect& aRect) const;
    void SizeChanged();
    void PositionChanged();
    void FocusChanged(TDrawNow aDrawNow);

protected:
    void qwidgetResize_helper(const QSize &newSize);

private:
    void HandlePointerEvent(const TPointerEvent& aPointerEvent);
    TKeyResponse OfferKeyEvent(const TKeyEvent& aKeyEvent,TEventCode aType);
    TKeyResponse sendSymbianKeyEvent(const TKeyEvent &keyEvent, QEvent::Type type);
    TKeyResponse sendKeyEvent(QWidget *widget, QKeyEvent *keyEvent);
    TKeyResponse handleVirtualMouse(const TKeyEvent& keyEvent,TEventCode type);
    bool sendMouseEvent(QWidget *widget, QMouseEvent *mEvent);
    void sendMouseEvent(
            QWidget *receiver,
            QEvent::Type type,
            const QPoint &globalPos,
            Qt::MouseButton button,
            Qt::KeyboardModifiers modifiers);
    struct TouchEventParams
    {
        TouchEventParams();
        TouchEventParams(int pointerNumber, TPointerEvent::TType type, QPointF screenPos, qreal pressure);
        int pointerNumber;
        TPointerEvent::TType type;
        QPointF screenPos;
        qreal pressure;
    };
    void processTouchEvents(const QVector<TouchEventParams> &touches);
    void HandleLongTapEventL( const TPoint& aPenEventLocation, const TPoint& aPenEventScreenLocation );
#ifdef QT_SYMBIAN_SUPPORTS_ADVANCED_POINTER
#ifdef COE_GROUPED_POINTER_EVENT_VERSION
    void translateMultiEventPointerEvent(const CCoeEventData &eventData );
#endif
    void translateAdvancedPointerEvent(const TAdvancedPointerEvent *event);
    TouchEventParams TouchEventFromAdvancedPointerEvent(const TAdvancedPointerEvent *event);
#endif
    bool isSplitViewWidget(QWidget *widget);
    bool hasFocusedAndVisibleChild(QWidget *parentWidget);
    void doDraw(const TRect& aRect) const;

public:
    void handleClientAreaChange();

private:
    static QSymbianControl *lastFocusedControl;

private:
    QWidget *qwidget;
    QLongTapTimer* m_longTapDetector;
    QElapsedTimer m_doubleClickTimer;
    bool m_ignoreFocusChanged : 1;
    bool m_symbianPopupIsOpen : 1;

#ifdef Q_WS_S60
    // Fader object used to fade everything except this menu and the CBA.
    TAknPopupFader popupFader;
#endif

    bool m_inExternalScreenOverride : 1;
    bool m_lastStatusPaneVisibility : 1;
};

inline QS60Data::QS60Data()
: uid(TUid::Null()),
  screenDepth(0),
  screenWidthInPixels(0),
  screenHeightInPixels(0),
  screenWidthInTwips(0),
  screenHeightInTwips(0),
  defaultDpiX(0),
  defaultDpiY(0),
  curWin(0),
  virtualMousePressedKeys(0),
  virtualMouseAccelDX(0),
  virtualMouseAccelDY(0),
  virtualMouseMaxAccel(0),
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
  brokenPointerCursors(0),
#endif
  hasTouchscreen(0),
  mouseInteractionEnabled(0),
  virtualMouseRequired(0),
  qtOwnsS60Environment(0),
  supportsPremultipliedAlpha(0),
  avkonComponentsSupportTransparency(0),
  menuBeingConstructed(0),
  orientationSet(0),
  partial_keyboard(0),
  partial_keyboardAutoTranslation(1),
  partialKeyboardOpen(0),
  handleStatusPaneResizeNotifications(1),
  screenFurnitureFullyCreated(0),
  beginFullScreenCalled(0),
  endFullScreenCalled(0),
  eglSurfaceCreationError(0),
  s60ApplicationFactory(0),
  s60InstalledTrapHandler(0),
  editorFlags(0)
{
}

inline void QS60Data::updateScreenSize()
{
    CWsScreenDevice *dev = S60->screenDevice();
    int screenModeCount = dev->NumScreenModes();
    int mode = dev->CurrentScreenMode();
    TPixelsTwipsAndRotation params;
    dev->GetScreenModeSizeAndRotation(mode, params);
    S60->screenWidthInPixels = params.iPixelSize.iWidth;
    S60->screenHeightInPixels = params.iPixelSize.iHeight;
    S60->screenWidthInTwips = params.iTwipsSize.iWidth;
    S60->screenHeightInTwips = params.iTwipsSize.iHeight;

    S60->virtualMouseMaxAccel = qMax(S60->screenHeightInPixels, S60->screenWidthInPixels) / 10;

    TReal inches = S60->screenHeightInTwips / (TReal)KTwipsPerInch;
    S60->defaultDpiY = S60->screenHeightInPixels / inches;
    inches = S60->screenWidthInTwips / (TReal)KTwipsPerInch;
    S60->defaultDpiX = S60->screenWidthInPixels / inches;

    switch (params.iRotation) {
    case CFbsBitGc::EGraphicsOrientationNormal:
        S60->screenRotation = ScreenRotation0;
        break;
    case CFbsBitGc::EGraphicsOrientationRotated90:
        S60->screenRotation = ScreenRotation90;
        break;
    case CFbsBitGc::EGraphicsOrientationRotated180:
        S60->screenRotation = ScreenRotation180;
        break;
    case CFbsBitGc::EGraphicsOrientationRotated270:
        S60->screenRotation = ScreenRotation270;
        break;
    default:
        S60->screenRotation = ScreenRotation0;
        break;
    }

    int screens = S60->screenCount();
    for (int i = 0; i < screens; ++i) {
        CWsScreenDevice *dev = S60->screenDevice(i);
        mode = dev->CurrentScreenMode();
        dev->GetScreenModeSizeAndRotation(mode, params);
        S60->screenWidthInPixelsForScreen[i] = params.iPixelSize.iWidth;
        S60->screenHeightInPixelsForScreen[i] = params.iPixelSize.iHeight;
        S60->screenWidthInTwipsForScreen[i] = params.iTwipsSize.iWidth;
        S60->screenHeightInTwipsForScreen[i] = params.iTwipsSize.iHeight;
    }

    // Look for a screen mode with rotation 0
    // in order to decide what the native orientation is.
    for (mode = 0; mode < screenModeCount; ++mode) {
        TPixelsAndRotation sizeAndRotation;
        dev->GetScreenModeSizeAndRotation(mode, sizeAndRotation);
        if (sizeAndRotation.iRotation == CFbsBitGc::EGraphicsOrientationNormal) {
            S60->nativeScreenWidthInPixels = sizeAndRotation.iPixelSize.iWidth;
            S60->nativeScreenHeightInPixels = sizeAndRotation.iPixelSize.iHeight;
            break;
        }
    }
}

inline RWsSession& QS60Data::wsSession()
{
    if(!tls.hasLocalData()) {
        tls.setLocalData(new QS60ThreadLocalData);
    }
    return tls.localData()->wsSession;
}

inline int QS60Data::screenCount()
{
#if defined(Q_SYMBIAN_SUPPORTS_MULTIPLE_SCREENS)
    CCoeEnv *env = CCoeEnv::Static();
    if (env) {
        return qMin(env->WsSession().NumberOfScreens(), qt_symbian_max_screens);
    }
#endif
    return 1;
}

inline RWindowGroup& QS60Data::windowGroup()
{
    return CCoeEnv::Static()->RootWin();
}

inline RWindowGroup& QS60Data::windowGroup(const QWidget *widget)
{
    return windowGroup(screenNumberForWidget(widget));
}

inline RWindowGroup& QS60Data::windowGroup(int screenNumber)
{
#if defined(Q_SYMBIAN_SUPPORTS_MULTIPLE_SCREENS)
    RWindowGroup *wg = CCoeEnv::Static()->RootWin(screenNumber);
    return wg ? *wg : windowGroup();
#else
    Q_UNUSED(screenNumber);
    return windowGroup();
#endif
}

inline CWsScreenDevice* QS60Data::screenDevice()
{
    if(!tls.hasLocalData()) {
        tls.setLocalData(new QS60ThreadLocalData);
    }
    return tls.localData()->screenDevice;
}

inline CWsScreenDevice* QS60Data::screenDevice(const QWidget *widget)
{
    return screenDevice(screenNumberForWidget(widget));
}

inline CWsScreenDevice* QS60Data::screenDevice(int screenNumber)
{
#if defined(Q_SYMBIAN_SUPPORTS_MULTIPLE_SCREENS)
    CCoeEnv *env = CCoeEnv::Static();
    if (env) {
        CWsScreenDevice *dev = env->ScreenDevice(screenNumber);
        return dev ? dev : screenDevice();
    } else {
        return screenDevice();
    }
#else
    return screenDevice();
#endif
}

inline int QS60Data::screenNumberForWidget(const QWidget *widget)
{
    if (!widget)
        return 0;
    const QWidget *w = widget;
    while (w->parentWidget())
        w = w->parentWidget();
    return qt_widget_private(const_cast<QWidget *>(w))->symbianScreenNumber;
}

inline QSymbianTypeFaceExtrasHash& QS60Data::fontData()
{
    if (!tls.hasLocalData()) {
        tls.setLocalData(new QS60ThreadLocalData);
    }
    return tls.localData()->fontData;
}

inline void QS60Data::addThreadLocalReleaseFunc(QThreadLocalReleaseFunc func)
{
    if (!tls.hasLocalData()) {
        tls.setLocalData(new QS60ThreadLocalData);
    }
    QS60ThreadLocalData *data = tls.localData();
    if (!data->releaseFuncs.contains(func))
        data->releaseFuncs.append(func);
}

inline CCoeAppUi* QS60Data::appUi()
{
    return CCoeEnv::Static()-> AppUi();
}

inline CEikMenuBar* QS60Data::menuBar()
{
    return CEikonEnv::Static()->AppUiFactory()->MenuBar();
}

#ifdef Q_WS_S60
inline CEikStatusPane* QS60Data::statusPane()
{
    return CEikonEnv::Static()->AppUiFactory()->StatusPane();
}

// Returns the application's status pane control, if not present returns NULL.
inline CCoeControl* QS60Data::statusPaneSubPane( TInt aPaneId )
{
    const TUid paneUid = { aPaneId };
    CEikStatusPane* statusPane = S60->statusPane();
    if (statusPane && statusPane->PaneCapabilities(paneUid).IsPresent()) {
        CCoeControl* control = NULL;
        // ControlL shouldn't leave because the pane is present
        TRAPD(err, control = statusPane->ControlL(paneUid));
        return err != KErrNone ? NULL : control;
    }
    return NULL;
}

// Returns the application's title pane, if not present returns NULL.
inline CAknTitlePane* QS60Data::titlePane()
{
    return static_cast<CAknTitlePane*>(S60->statusPaneSubPane(EEikStatusPaneUidTitle));
}

// Returns the application's title pane, if not present returns NULL.
inline CAknContextPane* QS60Data::contextPane()
{
    return static_cast<CAknContextPane*>(S60->statusPaneSubPane(EEikStatusPaneUidContext));
}

inline CEikButtonGroupContainer* QS60Data::buttonGroupContainer()
{
    return QS60Data::cba;
}

inline void QS60Data::setButtonGroupContainer(CEikButtonGroupContainer *newCba)
{
    QS60Data::cba = newCba;
}
#endif // Q_WS_S60

static inline QFont qt_TFontSpec2QFontL(const TFontSpec &fontSpec)
{
    return QFont(
        qt_TDesC2QString(fontSpec.iTypeface.iName),
        fontSpec.iHeight / KTwipsPerPoint,
        fontSpec.iFontStyle.StrokeWeight() == EStrokeWeightNormal ? QFont::Normal : QFont::Bold,
        fontSpec.iFontStyle.Posture() == EPostureItalic
    );
}

static inline QImage::Format qt_TDisplayMode2Format(TDisplayMode mode)
{
    QImage::Format format;
    switch(mode) {
    case EGray2:
        format = QImage::Format_MonoLSB;
        break;
    case EColor256:
    case EGray256:
        format = QImage::Format_Indexed8;
        break;
    case EColor4K:
        format = QImage::Format_RGB444;
        break;
    case EColor64K:
        format = QImage::Format_RGB16;
        break;
    case EColor16M:
        format = QImage::Format_RGB888;
        break;
    case EColor16MU:
        format = QImage::Format_RGB32;
        break;
    case EColor16MA:
        format = QImage::Format_ARGB32;
        break;
    case Q_SYMBIAN_ECOLOR16MAP:
        format = QImage::Format_ARGB32_Premultiplied;
        break;
    default:
        format = QImage::Format_Invalid;
        break;
    }
    return format;
}

#ifndef QT_NO_CURSOR
void qt_symbian_setWindowCursor(const QCursor &cursor, const CCoeControl* wid);
void qt_symbian_setWindowGroupCursor(const QCursor &cursor, RWindowTreeNode &node);
void qt_symbian_setGlobalCursor(const QCursor &cursor);
void qt_symbian_set_cursor_visible(bool visible);
bool qt_symbian_is_cursor_visible();
#endif

static inline bool qt_beginFullScreenEffect()
{
#if defined(Q_WS_S60) && defined(QT_SYMBIAN_HAVE_AKNTRANSEFFECT_H)
    // Only for post-S^3. On earlier versions the system transition effects
    // may not be able to capture the non-Avkon content, leading to confusing
    // looking effects, so just skip the whole thing.
    if (S60->beginFullScreenCalled || QSysInfo::s60Version() <= QSysInfo::SV_S60_5_2)
        return false;
    S60->beginFullScreenCalled = true;
    // For Avkon apps the app-exit effect is triggered from CAknAppUi::PrepareToExit().
    // That is good for Avkon apps, but in case of Qt the RWindows are destroyed earlier.
    // Therefore we call BeginFullScreen() ourselves.
    GfxTransEffect::BeginFullScreen(AknTransEffect::EApplicationExit,
        TRect(0, 0, 0, 0),
        AknTransEffect::EParameterType,
        AknTransEffect::GfxTransParam(S60->uid,
            AknTransEffect::TParameter::EAvkonCheck | KQtAppExitFlag));
    return true;
#else
    return false;
#endif
}

static inline void qt_abortFullScreenEffect()
{
#if defined(Q_WS_S60) && defined(QT_SYMBIAN_HAVE_AKNTRANSEFFECT_H)
    if (!S60->beginFullScreenCalled || QSysInfo::s60Version() <= QSysInfo::SV_S60_5_2)
        return;
    GfxTransEffect::AbortFullScreen();
    S60->beginFullScreenCalled = S60->endFullScreenCalled = false;
#endif
}

static inline void qt_endFullScreenEffect()
{
#if defined(Q_WS_S60) && defined(QT_SYMBIAN_HAVE_AKNTRANSEFFECT_H)
    if (S60->endFullScreenCalled || QSysInfo::s60Version() <= QSysInfo::SV_S60_5_2)
        return;
    S60->endFullScreenCalled = true;
    GfxTransEffect::EndFullScreen();
#endif
}

QT_END_NAMESPACE

#endif // QT_S60_P_H

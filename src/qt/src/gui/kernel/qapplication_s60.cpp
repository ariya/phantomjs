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

#include "qapplication_p.h"
#include "qsessionmanager.h"
#include "qevent.h"
#include "qsymbianevent.h"
#include "qeventdispatcher_s60_p.h"
#include "qwidget.h"
#include "qdesktopwidget.h"
#include "private/qbackingstore_p.h"
#include "qt_s60_p.h"
#include "private/qevent_p.h"
#include "qstring.h"
#include "qdebug.h"
#include "qimage.h"
#include "qcombobox.h"
#include "private/qkeymapper_p.h"
#include "private/qfont_p.h"
#ifndef QT_NO_STYLE_S60
#include "private/qs60style_p.h"
#endif
#include "private/qwindowsurface_s60_p.h"
#include "qpaintengine.h"
#include "private/qmenubar_p.h"
#include "private/qsoftkeymanager_p.h"
#ifdef QT_GRAPHICSSYSTEM_RUNTIME
#include "private/qgraphicssystem_runtime_p.h"
#endif
#include "private/qcursor_p.h"

#include "apgwgnam.h" // For CApaWindowGroupName
#include <mdaaudiotoneplayer.h>     // For CMdaAudioToneUtility

#if defined(Q_OS_SYMBIAN)
# include <private/qs60mainapplication_p.h>
# include <centralrepository.h>
# include "qs60mainappui.h"
# include "qinputcontext.h"
# include <private/qgraphicssystemex_symbian_p.h>
#endif

#if defined(Q_WS_S60)
# if !defined(QT_NO_IM)
#  include <private/qcoefepinputcontext_p.h>
# endif
#endif

#include "private/qstylesheetstyle_p.h"

#include <hal.h>
#include <hal_data.h>

#ifdef Q_SYMBIAN_TRANSITION_EFFECTS
#include <graphics/wstfxconst.h>
#endif

#ifdef COE_GROUPED_POINTER_EVENT_VERSION
#include <coeeventdata.h>
#endif

QT_BEGIN_NAMESPACE

// Goom Events through Window Server
static const int KGoomMemoryLowEvent = 0x10282DBF;
static const int KGoomMemoryGoodEvent = 0x20026790;
// Split view open/close events from AVKON
static const int KSplitViewOpenEvent = 0x2001E2C0;
static const int KSplitViewCloseEvent = 0x2001E2C1;

#if defined(QT_DEBUG)
static bool        appNoGrab        = false;        // Grabbing enabled
#endif
static bool        app_do_modal        = false;        // modal mode
Q_GLOBAL_STATIC(QS60Data, qt_s60Data);

extern bool qt_sendSpontaneousEvent(QObject*,QEvent*);
extern QWidgetList *qt_modal_stack;              // stack of modal widgets
extern QDesktopWidget *qt_desktopWidget; // qapplication.cpp

QWidget *qt_button_down = 0;                     // widget got last button-down

QSymbianControl *QSymbianControl::lastFocusedControl = 0;

static Qt::KeyboardModifiers app_keyboardModifiers = Qt::NoModifier;

QS60Data* qGlobalS60Data()
{
    return qt_s60Data();
}

#ifdef Q_WS_S60
void QS60Data::setStatusPaneAndButtonGroupVisibility(bool statusPaneVisible, bool buttonGroupVisible)
{
    bool buttonGroupVisibilityChanged = false;
    if (CEikButtonGroupContainer *const b = buttonGroupContainer()) {
        buttonGroupVisibilityChanged = (b->IsVisible() != buttonGroupVisible);
        b->MakeVisible(buttonGroupVisible);
    }
    bool statusPaneVisibilityChanged = false;
    if (CEikStatusPane *const s = statusPane()) {
        statusPaneVisibilityChanged = (s->IsVisible() != statusPaneVisible);
        s->MakeVisible(statusPaneVisible);
    }
    if (buttonGroupVisibilityChanged  || statusPaneVisibilityChanged) {
        const QSize size = qt_TRect2QRect(S60->clientRect()).size();
        const QSize oldSize; // note that QDesktopWidget::resizeEvent ignores the QResizeEvent contents
        QResizeEvent event(size, oldSize);
        QApplication::instance()->sendEvent(QApplication::desktop(), &event);
    }
    if (buttonGroupVisibilityChanged  && !statusPaneVisibilityChanged && QApplication::activeWindow())
        // Ensure that control rectangle is updated
        static_cast<QSymbianControl *>(QApplication::activeWindow()->winId())->handleClientAreaChange();
}

bool QS60Data::setRecursiveDecorationsVisibility(QWidget *window, Qt::WindowStates newState)
{
    // Show statusbar:
    //   Topmost parent: Show unless fullscreen/minimized.
    //   Child windows: Follow topmost parent, unless fullscreen, in which case do not show statusbar
    // Show CBA:
    //   Topmost parent: Show unless fullscreen/minimized.
    //     Exception: Show if fullscreen with Qt::WindowSoftkeysVisibleHint.
    //   Child windows:
    //     Minimized: Unclear if there is an use case for having focused minimized window at all.
    //       Always follow topmost parent just to be safe.
    //     Maximized and normal: follow topmost parent.
    //       Exception: If topmost parent is not showing CBA, show CBA if any softkey actions are
    //                  defined.
    //     Fullscreen: Show only if Qt::WindowSoftkeysVisibleHint set.

    Qt::WindowStates comparisonState = newState;
    QWidget *parentWindow = window->parentWidget();
    if (parentWindow) {
        while (parentWindow->parentWidget())
            parentWindow = parentWindow->parentWidget();
        comparisonState = parentWindow->windowState();
    } else {
        parentWindow = window;
    }

    bool decorationsVisible = !(comparisonState & (Qt::WindowFullScreen | Qt::WindowMinimized));
    const bool parentIsFullscreen = comparisonState & Qt::WindowFullScreen;
    const bool parentCbaVisibilityHint = parentWindow->windowFlags() & Qt::WindowSoftkeysVisibleHint;
    bool buttonGroupVisibility = (decorationsVisible || (parentIsFullscreen && parentCbaVisibilityHint));

    // Do extra checking for child windows
    if (window->parentWidget()) {
        if (newState & Qt::WindowFullScreen) {
            decorationsVisible = false;
            if (window->windowFlags() & Qt::WindowSoftkeysVisibleHint)
                buttonGroupVisibility = true;
            else
                buttonGroupVisibility = false;
        } else if (!(newState & Qt::WindowMinimized) && !buttonGroupVisibility) {
            for (int i = 0; i < window->actions().size(); ++i) {
                if (window->actions().at(i)->softKeyRole() != QAction::NoSoftKey) {
                    buttonGroupVisibility = true;
                    break;
                }
            }
        }
    }

    S60->setStatusPaneAndButtonGroupVisibility(decorationsVisible, buttonGroupVisibility);

    return decorationsVisible;
}
#endif

void QS60Data::createStatusPaneAndCBA()
{
    CEikAppUi *ui = static_cast<CEikAppUi *>(S60->appUi());
    MEikAppUiFactory *factory = CEikonEnv::Static()->AppUiFactory();
    QT_TRAP_THROWING(
        factory->CreateResourceIndependentFurnitureL(ui);
        CEikButtonGroupContainer *cba = CEikButtonGroupContainer::NewL(CEikButtonGroupContainer::ECba,
            CEikButtonGroupContainer::EHorizontal, ui, R_AVKON_SOFTKEYS_EMPTY_WITH_IDS);
        CEikButtonGroupContainer *oldCba = factory->SwapButtonGroup(cba);
        Q_ASSERT(!oldCba);
        S60->setButtonGroupContainer(cba);
        CEikMenuBar *menuBar = new(ELeave) CEikMenuBar;
        menuBar->ConstructL(ui, 0, R_AVKON_MENUPANE_EMPTY);
        menuBar->SetMenuType(CEikMenuBar::EMenuOptions);
        S60->appUi()->AddToStackL(menuBar, ECoeStackPriorityMenu, ECoeStackFlagRefusesFocus);
        CEikMenuBar *oldMenu = factory->SwapMenuBar(menuBar);
        Q_ASSERT(!oldMenu);
    )
    if (S60->statusPane()) {
        // Use QDesktopWidget as the status pane observer to proxy for the AppUi.
        // Can't use AppUi directly because it privately inherits from MEikStatusPaneObserver.
        QSymbianControl *desktopControl = static_cast<QSymbianControl *>(QApplication::desktop()->winId());
        S60->statusPane()->SetObserver(desktopControl);
    }
}

void QS60Data::controlVisibilityChanged(CCoeControl *control, bool visible)
{
    if (QWidgetPrivate::mapper && QWidgetPrivate::mapper->contains(control)) {
        QWidget *const widget = QWidgetPrivate::mapper->value(control);
        QWidget *const window = widget->window();
        if (QTLWExtra *topData = qt_widget_private(window)->maybeTopData()) {
            QWidgetBackingStoreTracker &backingStore = topData->backingStore;
            if (visible) {
                QApplicationPrivate *d = QApplicationPrivate::instance();
                d->emitAboutToUseGpuResources();

                // (Re)create the backing store and force repaint if we have no
                // backing store already, or EGL surface cration failed on last attempt.
                if (backingStore.data() && !S60->eglSurfaceCreationError) {
                    backingStore.registerWidget(widget);
                } else {
                    S60->eglSurfaceCreationError = false;
                    backingStore.create(window);
                    backingStore.registerWidget(widget);
                    qt_widget_private(widget)->invalidateBuffer(widget->rect());
                    widget->repaint();
                    if (S60->eglSurfaceCreationError)
                        backingStore.unregisterWidget(widget);
                }
            } else {
                QApplicationPrivate *d = QApplicationPrivate::instance();
                d->emitAboutToReleaseGpuResources();

                // In certain special scenarios we may get an ENotVisible event
                // without a previous EPartiallyVisible. The backingstore must
                // still be destroyed, hence the registerWidget() call below.
                if (backingStore.data() && widget->internalWinId()
                    && qt_widget_private(widget)->maybeBackingStore() == backingStore.data())
                    backingStore.registerWidget(widget);
                backingStore.unregisterWidget(widget);
                // In order to ensure that any resources used by the window surface
                // are immediately freed, we flush the WSERV command buffer.
                S60->wsSession().Flush();
            }
        }
    }
}

TRect QS60Data::clientRect()
{
    TRect r = static_cast<CEikAppUi*>(S60->appUi())->ClientRect();
    if (S60->partialKeyboardOpen && !QApplication::testAttribute(Qt::AA_S60DontConstructApplicationPanes)) {
        // Adjust client rect when splitview is open
        // We want it to take the client rect space as if the splitview keyboard was not there
        TRect statusPaneRect;
        TRect mainRect;
        AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EStatusPane, statusPaneRect);
        AknLayoutUtils::LayoutMetricsRect(AknLayoutUtils::EMainPane, mainRect);
        int clientAreaHeight = mainRect.Height();
        CEikStatusPane *const s = S60->statusPane();
        if (!(s && s->IsVisible()))
            clientAreaHeight += statusPaneRect.Height();
        r.SetHeight(clientAreaHeight);
    }
    return r;
}

bool qt_nograb()                                // application no-grab option
{
#if defined(QT_DEBUG)
    return appNoGrab;
#else
    return false;
#endif
}

// Modified from http://www3.symbian.com/faq.nsf/0/0F1464EE96E737E780256D5E00503DD1?OpenDocument
class QS60Beep : public CBase, public MMdaAudioToneObserver
{
public:
    static QS60Beep* NewL(TInt aFrequency,  TTimeIntervalMicroSeconds iDuration);
    void Play();
    ~QS60Beep();
private:
    void ConstructL(TInt aFrequency,  TTimeIntervalMicroSeconds iDuration);
    void MatoPrepareComplete(TInt aError);
    void MatoPlayComplete(TInt aError);
private:
    typedef enum
        {
        EBeepNotPrepared,
        EBeepPrepared,
        EBeepPlaying
        } TBeepState;
private:
    CMdaAudioToneUtility* iToneUtil;
    TBeepState iState;
    TInt iFrequency;
    TTimeIntervalMicroSeconds iDuration;
};

static QS60Beep* qt_S60Beep = 0;

QS60Beep::~QS60Beep()
{
    if (iToneUtil) {
        switch (iState) {
        case EBeepPlaying:
            iToneUtil->CancelPlay();
            break;
        case EBeepNotPrepared:
            iToneUtil->CancelPrepare();
            break;
        }
    }
    delete iToneUtil;
}

QS60Beep* QS60Beep::NewL(TInt aFrequency, TTimeIntervalMicroSeconds aDuration)
{
    QS60Beep* self = new (ELeave) QS60Beep();
    CleanupStack::PushL(self);
    self->ConstructL(aFrequency, aDuration);
    CleanupStack::Pop();
    return self;
}

void QS60Beep::ConstructL(TInt aFrequency, TTimeIntervalMicroSeconds aDuration)
{
    iToneUtil = CMdaAudioToneUtility::NewL(*this);
    iState = EBeepNotPrepared;
    iFrequency = aFrequency;
    iDuration = aDuration;
    iToneUtil->PrepareToPlayTone(iFrequency, iDuration);
}

void QS60Beep::Play()
{
    if (iState == EBeepPlaying) {
        iToneUtil->CancelPlay();
        iState = EBeepPrepared;
    }

    iToneUtil->Play();
    iState = EBeepPlaying;
}

void QS60Beep::MatoPrepareComplete(TInt aError)
{
    if (aError == KErrNone) {
        iState = EBeepPrepared;
        Play();
    }
}

void QS60Beep::MatoPlayComplete(TInt aError)
{
    Q_UNUSED(aError);
    iState = EBeepPrepared;
}


static Qt::KeyboardModifiers mapToQtModifiers(TUint s60Modifiers)
{
    Qt::KeyboardModifiers result = Qt::NoModifier;

    if (s60Modifiers & EModifierKeypad)
        result |= Qt::KeypadModifier;
    if (s60Modifiers & EModifierShift || s60Modifiers & EModifierLeftShift
            || s60Modifiers & EModifierRightShift)
        result |= Qt::ShiftModifier;
    if (s60Modifiers & EModifierCtrl || s60Modifiers & EModifierLeftCtrl
            || s60Modifiers & EModifierRightCtrl)
        result |= Qt::ControlModifier;
    if (s60Modifiers & EModifierAlt || s60Modifiers & EModifierLeftAlt
            || s60Modifiers & EModifierRightAlt)
        result |= Qt::AltModifier;

    return result;
}

static void mapS60MouseEventTypeToQt(QEvent::Type *type, Qt::MouseButton *button, const TPointerEvent *pEvent)
{
    switch (pEvent->iType) {
    case TPointerEvent::EButton1Down:
        *type = QEvent::MouseButtonPress;
        *button = Qt::LeftButton;
        break;
    case TPointerEvent::EButton1Up:
        *type = QEvent::MouseButtonRelease;
        *button = Qt::LeftButton;
        break;
    case TPointerEvent::EButton2Down:
        *type = QEvent::MouseButtonPress;
        *button = Qt::MidButton;
        break;
    case TPointerEvent::EButton2Up:
        *type = QEvent::MouseButtonRelease;
        *button = Qt::MidButton;
        break;
    case TPointerEvent::EButton3Down:
        *type = QEvent::MouseButtonPress;
        *button = Qt::RightButton;
        break;
    case TPointerEvent::EButton3Up:
        *type = QEvent::MouseButtonRelease;
        *button = Qt::RightButton;
        break;
    case TPointerEvent::EDrag:
        *type = QEvent::MouseMove;
        *button = Qt::NoButton;
        break;
    case TPointerEvent::EMove:
        // Qt makes no distinction between move and drag
        *type = QEvent::MouseMove;
        *button = Qt::NoButton;
        break;
    default:
        *type = QEvent::None;
        *button = Qt::NoButton;
        break;
    }
    if (pEvent->iModifiers & EModifierDoubleClick){
        *type = QEvent::MouseButtonDblClick;
    }

    if (*type == QEvent::MouseButtonPress || *type == QEvent::MouseButtonDblClick)
        QApplicationPrivate::mouse_buttons = QApplicationPrivate::mouse_buttons | (*button);
    else if (*type == QEvent::MouseButtonRelease)
        QApplicationPrivate::mouse_buttons = QApplicationPrivate::mouse_buttons &(~(*button));

    QApplicationPrivate::mouse_buttons = QApplicationPrivate::mouse_buttons & Qt::MouseButtonMask;
}

//### Can be replaced with CAknLongTapDetector if animation is required.
//NOTE: if CAknLongTapDetector is used make sure it gets variated out of 3.1 and 3.2,.
//also MLongTapObserver needs to be changed to MAknLongTapDetectorCallBack if CAknLongTapDetector is used.
class QLongTapTimer : public CTimer
{
public:
    static QLongTapTimer* NewL(QAbstractLongTapObserver *observer);
    QLongTapTimer(QAbstractLongTapObserver *observer);
    void ConstructL();
public:
    void PointerEventL(const TPointerEvent &event);
    void RunL();
protected:
private:
    QAbstractLongTapObserver *m_observer;
    TPointerEvent m_event;
    QPoint m_pressedCoordinates;
    int m_dragDistance;
};

QLongTapTimer* QLongTapTimer::NewL(QAbstractLongTapObserver *observer)
{
    QLongTapTimer* self = new QLongTapTimer(observer);
    self->ConstructL();
    return self;
}
void QLongTapTimer::ConstructL()
{
    CTimer::ConstructL();
}

QLongTapTimer::QLongTapTimer(QAbstractLongTapObserver *observer):CTimer(CActive::EPriorityHigh)
{
    m_observer = observer;
    m_dragDistance = qApp->startDragDistance();
    CActiveScheduler::Add(this);
}

void QLongTapTimer::PointerEventL(const TPointerEvent& event)
{
    if ( event.iType == TPointerEvent::EDrag || event.iType == TPointerEvent::EButtonRepeat)
    {
        QPoint diff(QPoint(event.iPosition.iX,event.iPosition.iY) - m_pressedCoordinates);
        if (diff.manhattanLength() < m_dragDistance)
            return;
    }
    Cancel();
    m_event = event;
    if (event.iType == TPointerEvent::EButton1Down)
    {
        m_pressedCoordinates = QPoint(event.iPosition.iX,event.iPosition.iY);
        // must be same as KLongTapDelay in aknlongtapdetector.h
        After(800000);
    }
}
void QLongTapTimer::RunL()
{
    if (m_observer)
        m_observer->HandleLongTapEventL(m_event.iPosition, m_event.iParentPosition);
}

QSymbianControl::QSymbianControl(QWidget *w)
    : CCoeControl()
    , qwidget(w)
    , m_longTapDetector(0)
    , m_ignoreFocusChanged(0)
    , m_symbianPopupIsOpen(0)
    , m_inExternalScreenOverride(false)
    , m_lastStatusPaneVisibility(0)
{
}

void QSymbianControl::ConstructL(bool isWindowOwning, bool desktop)
{
    if (!desktop)
    {
        if (isWindowOwning || !qwidget->parentWidget()
            || qwidget->parentWidget()->windowType() == Qt::Desktop) {
            RWindowGroup &wg(S60->windowGroup(qwidget));
            CreateWindowL(wg);
        } else {
            /**
             * TODO: in order to avoid creating windows for all ancestors of
             * this widget up to the root window, the parameter passed to
             * CreateWindowL should be
             * qwidget->parentWidget()->effectiveWinId().  However, if we do
             * this, then we need to take care of re-parenting when a window
             * is created for a widget between this one and the root window.
             */
            CreateWindowL(qwidget->parentWidget()->winId());
        }

        // Necessary in order to be able to track the activation status of
        // the control's window
        qwidget->d_func()->createExtra();

        if (!qwidget->d_func()->isGLGlobalShareWidget) {
            SetFocusing(true);
            m_longTapDetector = QLongTapTimer::NewL(this);
            m_doubleClickTimer.invalidate();

            DrawableWindow()->SetPointerGrab(ETrue);
        }
    }

#ifdef Q_SYMBIAN_TRANSITION_EFFECTS
    if (OwnsWindow()) {
        TTfxWindowPurpose windowPurpose(ETfxPurposeNone);
        switch (qwidget->windowType()) {
        case Qt::Dialog:
            windowPurpose = ETfxPurposeDialogWindow;
            break;
        case Qt::Popup:
            windowPurpose = ETfxPurposePopupWindow;
            break;
        case Qt::Tool:
            windowPurpose = ETfxPurposeToolWindow;
            break;
        case Qt::ToolTip:
            windowPurpose = ETfxPurposeToolTipWindow;
            break;
        case Qt::SplashScreen:
            windowPurpose = ETfxPurposeSplashScreenWindow;
            break;
        default:
            windowPurpose = (isWindowOwning || !qwidget->parentWidget() || qwidget->parentWidget()->windowType() == Qt::Desktop)
                            ? ETfxPurposeWindow : ETfxPurposeChildWindow;
            break;
        }
        Window().SetPurpose(windowPurpose);
    }
#endif
}

QSymbianControl::~QSymbianControl()
{
    if (!qwidget->d_func()->isGLGlobalShareWidget) { // GLGlobalShareWidget doesn't interact with scene
        // Ensure backing store is deleted before the top-level
        // window is destroyed
        QT_TRY {
            qt_widget_private(qwidget)->topData()->backingStore.destroy();
        } QT_CATCH(const std::exception&) {
            // ignore exceptions, nothing can be done
        }

        if (S60->curWin == this)
            S60->curWin = 0;
        if (!QApplicationPrivate::is_app_closing) {
            QT_TRY {
                setFocusSafely(false);
            } QT_CATCH(const std::exception&) {
                // ignore exceptions, nothing can be done
            }
        }
        S60->appUi()->RemoveFromStack(this);
        delete m_longTapDetector;
    }
}

void QSymbianControl::setWidget(QWidget *w)
{
    qwidget = w;
}

QPoint QSymbianControl::translatePointForFixedNativeOrientation(const TPoint &pointerEventPos, TTranslationType translationType) const
{
    QPoint pos(pointerEventPos.iX, pointerEventPos.iY);
    if (qwidget->d_func()->fixNativeOrientationCalled) {
        QSize wsize = qwidget->size(); // always same as the size in the native orientation
        TSize size = Size(); // depends on the current orientation
        // pixel center translations, eg touch points, should be reflected against the last pixel center, which is at size-1
        int offset = (translationType == ETranslatePixelCenter) ? 1 : 0;
        if (size.iWidth == wsize.height() && size.iHeight == wsize.width()) {
            qreal x = pos.x();
            qreal y = pos.y();
            if (S60->screenRotation == QS60Data::ScreenRotation90) {
                // DisplayRightUp
                pos.setX(size.iHeight - offset - y);
                pos.setY(x);
            } else if (S60->screenRotation == QS60Data::ScreenRotation270) {
                // DisplayLeftUp
                pos.setX(y);
                pos.setY(size.iWidth - offset - x);
            }
        }
    }
    return pos;
}

TRect QSymbianControl::translateRectForFixedNativeOrientation(const TRect &controlRect) const
{
    TRect rect = controlRect;
    if (qwidget->d_func()->fixNativeOrientationCalled) {
        QPoint a = translatePointForFixedNativeOrientation(rect.iTl, ETranslatePixelEdge);
        QPoint b = translatePointForFixedNativeOrientation(rect.iBr, ETranslatePixelEdge);
        if (a.x() < b.x()) {
            rect.iTl.iX = a.x();
            rect.iBr.iX = b.x();
        } else {
            rect.iTl.iX = b.x();
            rect.iBr.iX = a.x();
        }
        if (a.y() < b.y()) {
            rect.iTl.iY = a.y();
            rect.iBr.iY = b.y();
        } else {
            rect.iTl.iY = b.y();
            rect.iBr.iY = a.y();
        }
    }
    return rect;
}

void QSymbianControl::HandleLongTapEventL( const TPoint& aPenEventLocation, const TPoint& aPenEventScreenLocation )
{
    QWidget *alienWidget;
    QPoint widgetPos = translatePointForFixedNativeOrientation(aPenEventLocation, ETranslatePixelCenter);
    QPoint globalPos = translatePointForFixedNativeOrientation(aPenEventScreenLocation, ETranslatePixelCenter);
    alienWidget = qwidget->childAt(widgetPos);
    if (!alienWidget)
        alienWidget = qwidget;

#if !defined(QT_NO_CONTEXTMENU)
    QContextMenuEvent contextMenuEvent(QContextMenuEvent::Mouse, widgetPos, globalPos, Qt::NoModifier);
    qt_sendSpontaneousEvent(alienWidget, &contextMenuEvent);
#endif
}

#ifdef QT_SYMBIAN_SUPPORTS_ADVANCED_POINTER
#ifdef COE_GROUPED_POINTER_EVENT_VERSION
void QSymbianControl::translateMultiEventPointerEvent(const CCoeEventData &eventData )
{
    TUint count = eventData.Count();
    QVector<TouchEventParams> touches;
    touches.reserve(count);
    for (int i = 0; i < count; i++) {
        const TPointerEvent *pointerEvent = eventData.Pointer(i);
        const TAdvancedPointerEvent *advEvent = pointerEvent->AdvancedPointerEvent();
        if (advEvent)
            touches.push_back(TouchEventFromAdvancedPointerEvent(advEvent));
    }
    if (touches.size())
        processTouchEvents(touches);
}
#endif

void QSymbianControl::translateAdvancedPointerEvent(const TAdvancedPointerEvent *event)
{
    processTouchEvents(QVector<TouchEventParams>(1, TouchEventFromAdvancedPointerEvent(event)));
}

QSymbianControl::TouchEventParams QSymbianControl::TouchEventFromAdvancedPointerEvent(const TAdvancedPointerEvent *event)
{
    QApplicationPrivate *d = QApplicationPrivate::instance();
    QPointF screenPos = qwidget->mapToGlobal(translatePointForFixedNativeOrientation(event->iPosition, ETranslatePixelCenter));
    qreal pressure;
    if (d->pressureSupported
        && event->Pressure() > 0) //workaround for misconfigured HAL
        pressure = event->Pressure() / qreal(d->maxTouchPressure);
    else
        pressure = qreal(1.0);
    return TouchEventParams(event->PointerNumber(), event->iType, screenPos, pressure);
}
#endif

QSymbianControl::TouchEventParams::TouchEventParams()
{}

QSymbianControl::TouchEventParams::TouchEventParams(int pointerNumber, TPointerEvent::TType type, QPointF screenPos, qreal pressure)
    : pointerNumber(pointerNumber),
      type(type),
      screenPos(screenPos),
      pressure(pressure)
{}

void QSymbianControl::processTouchEvents(const QVector<TouchEventParams> &touches)
{
    QRect screenGeometry = qApp->desktop()->screenGeometry(qwidget);

    QApplicationPrivate *d = QApplicationPrivate::instance();

    // get the maximum pointer number
    int numUpdates = touches.size();
    int maxPointerNumber = 0;
    for (int i = 0; i < numUpdates; ++i) {
        const TouchEventParams &touch = touches[i];
        maxPointerNumber = qMax(maxPointerNumber, touch.pointerNumber);
    }

    // ensure there are sufficient touch events in the list,
    // touch events will be indexed by pointerNumber
    QList<QTouchEvent::TouchPoint> points = d->appAllTouchPoints;
    while (points.count() <= maxPointerNumber)
        points.append(QTouchEvent::TouchPoint(points.count()));

    // first set all active touch points to stationary
    for (int i = 0; i < points.count(); ++i) {
        QTouchEvent::TouchPoint &touchPoint = points[i];
        if (touchPoint.state() != Qt::TouchPointReleased) {
            touchPoint.setState(Qt::TouchPointStationary);
         }
    }

    // Add all info about moving or state changed touch points
    for (int i = 0; i < numUpdates; ++i) {
        const TouchEventParams &touch = touches[i];
        QTouchEvent::TouchPoint &touchPoint = points[touch.pointerNumber];
        Qt::TouchPointStates state;
        switch (touch.type) {
        case TPointerEvent::EButton1Down:
#ifdef QT_SYMBIAN_SUPPORTS_ADVANCED_POINTER
        case TPointerEvent::EEnterHighPressure:
#endif
            state = Qt::TouchPointPressed;
            break;
        case TPointerEvent::EButton1Up:
#ifdef QT_SYMBIAN_SUPPORTS_ADVANCED_POINTER
        case TPointerEvent::EExitCloseProximity:
#endif
            state = Qt::TouchPointReleased;
            break;
        case TPointerEvent::EDrag:
            state = Qt::TouchPointMoved;
            break;
        default:
            // how likely is this to happen?
            state = Qt::TouchPointStationary;
            break;
        }
        if (touch.pointerNumber == 0)
            state |= Qt::TouchPointPrimary;
        touchPoint.setState(state);

        touchPoint.setScreenPos(touch.screenPos);
        touchPoint.setNormalizedPos(QPointF(touch.screenPos.x() / screenGeometry.width(),
                                            touch.screenPos.y() / screenGeometry.height()));

        touchPoint.setPressure(touch.pressure);
    }

    // check the resulting state of all touch points
    Qt::TouchPointStates allStates = 0;
    for (int i = 0; i < points.count(); ++i) {
        QTouchEvent::TouchPoint &touchPoint = points[i];
        allStates |= touchPoint.state();
    }

    if ((allStates & Qt::TouchPointStateMask) == Qt::TouchPointReleased) {
        // all touch points released
        d->appAllTouchPoints.clear();
    } else {
        d->appAllTouchPoints = points;
    }

    QApplicationPrivate::translateRawTouchEvent(qwidget,
                                                QTouchEvent::TouchScreen,
                                                points);
}

void QSymbianControl::HandlePointerEventL(const TPointerEvent& pEvent)
{
#ifdef QT_SYMBIAN_SUPPORTS_ADVANCED_POINTER
#ifdef COE_GROUPED_POINTER_EVENT_VERSION
    if (pEvent.iType == TPointerEvent::EDataCCoeEventData) {
        // only advanced pointers can be data type pointer events
        const TAdvancedPointerEvent *advEvent = pEvent.AdvancedPointerEvent();
        if (!advEvent)
            return;
        const CCoeEventData& eventData = CCoeEventData::EventData(*advEvent);
        if (eventData.Type() == CWsEventWithData::EPointerEvent) {
            QT_TRYCATCH_LEAVING(translateMultiEventPointerEvent(eventData));
            // pointer 0 events and unnumbered events should also be handled as mouse events
            for (int i=0; i<eventData.Count(); i++) {
                const TPointerEvent *pointerEvent = eventData.Pointer(i);
                const TAdvancedPointerEvent *advEvent = pointerEvent->AdvancedPointerEvent();
                if (!advEvent || advEvent->PointerNumber() == 0) {
                    if (m_longTapDetector)
                        m_longTapDetector->PointerEventL(*pointerEvent);
                    QT_TRYCATCH_LEAVING(HandlePointerEvent(*pointerEvent));
                }
            }
            return;
        }
    }
#endif
    if (pEvent.IsAdvancedPointerEvent()) {
        const TAdvancedPointerEvent *advancedPointerEvent = pEvent.AdvancedPointerEvent();
        translateAdvancedPointerEvent(advancedPointerEvent);
        if (advancedPointerEvent->PointerNumber() != 0) {
            // only send mouse events for the first touch point
            return;
        }
    }
#endif
    if (m_longTapDetector)
        m_longTapDetector->PointerEventL(pEvent);
    QT_TRYCATCH_LEAVING(HandlePointerEvent(pEvent));
}

void QSymbianControl::HandlePointerEvent(const TPointerEvent& pEvent)
{
    QMouseEvent::Type type;
    Qt::MouseButton button;
    mapS60MouseEventTypeToQt(&type, &button, &pEvent);
    Qt::KeyboardModifiers modifiers = mapToQtModifiers(pEvent.iModifiers);
    app_keyboardModifiers = modifiers;

    QPoint widgetPos = translatePointForFixedNativeOrientation(pEvent.iPosition, ETranslatePixelCenter);
    TPoint controlScreenPos = PositionRelativeToScreen();
    QPoint globalPos = QPoint(controlScreenPos.iX, controlScreenPos.iY) + widgetPos;
    S60->lastCursorPos = globalPos;
    S60->lastPointerEventPos = widgetPos;

    QWidget *mouseGrabber = QWidget::mouseGrabber();

    QWidget *popupWidget = qApp->activePopupWidget();
    QWidget *popupReceiver = 0;
    if (popupWidget) {
        QWidget *popupChild = popupWidget->childAt(popupWidget->mapFromGlobal(globalPos));
        popupReceiver = popupChild ? popupChild : popupWidget;
    }

    if (mouseGrabber) {
        if (popupReceiver) {
            sendMouseEvent(popupReceiver, type, globalPos, button, modifiers);
        } else {
            sendMouseEvent(mouseGrabber, type, globalPos, button, modifiers);
        }
        // No Enter/Leave events in grabbing mode.
        return;
    }

    QWidget *widgetUnderPointer = qwidget->childAt(widgetPos);
    if (!widgetUnderPointer)
        widgetUnderPointer = qwidget;

    QApplicationPrivate::dispatchEnterLeave(widgetUnderPointer, S60->lastPointerEventTarget);
    S60->lastPointerEventTarget = widgetUnderPointer;

    QWidget *receiver;
    if (!popupReceiver && S60->mousePressTarget && type != QEvent::MouseButtonPress) {
        receiver = S60->mousePressTarget;
        if (type == QEvent::MouseButtonRelease)
            S60->mousePressTarget = 0;
    } else {
        receiver = popupReceiver ? popupReceiver : widgetUnderPointer;
        if (type == QEvent::MouseButtonPress)
            S60->mousePressTarget = receiver;
    }

#if !defined(QT_NO_CURSOR) && !defined(Q_SYMBIAN_FIXED_POINTER_CURSORS)
    if (S60->brokenPointerCursors)
        qt_symbian_move_cursor_sprite();
#endif

//Generate single touch event for S60 5.0 (has touchscreen, does not have advanced pointers)
#ifndef QT_SYMBIAN_SUPPORTS_ADVANCED_POINTER
    if (S60->hasTouchscreen) {
        processTouchEvents(QVector<TouchEventParams>(1, TouchEventParams(0, pEvent.iType, QPointF(globalPos), 1.0)));
    }
#endif

    sendMouseEvent(receiver, type, globalPos, button, modifiers);
}

#ifdef Q_WS_S60
void QSymbianControl::HandleStatusPaneSizeChange()
{
    QS60MainAppUi *s60AppUi = static_cast<QS60MainAppUi *>(S60->appUi());
    s60AppUi->HandleStatusPaneSizeChange();
}
#endif

void QSymbianControl::sendMouseEvent(
        QWidget *receiver,
        QEvent::Type type,
        const QPoint &globalPos,
        Qt::MouseButton button,
        Qt::KeyboardModifiers modifiers)
{
    Q_ASSERT(receiver);
    QMouseEvent mEvent(type, receiver->mapFromGlobal(globalPos), globalPos,
        button, QApplicationPrivate::mouse_buttons, modifiers);
    QEventDispatcherS60 *dispatcher;
    // It is theoretically possible for someone to install a different event dispatcher.
    if ((dispatcher = qobject_cast<QEventDispatcherS60 *>(receiver->d_func()->threadData->eventDispatcher)) != 0) {
        if (dispatcher->excludeUserInputEvents()) {
            dispatcher->saveInputEvent(this, receiver, new QMouseEvent(mEvent));
            return;
        }
    }

    sendMouseEvent(receiver, &mEvent);
}

bool QSymbianControl::sendMouseEvent(QWidget *widget, QMouseEvent *mEvent)
{
    return qt_sendSpontaneousEvent(widget, mEvent);
}

TKeyResponse QSymbianControl::OfferKeyEventL(const TKeyEvent& keyEvent, TEventCode type)
{
    TKeyResponse r = EKeyWasNotConsumed;
    QT_TRYCATCH_LEAVING(r = OfferKeyEvent(keyEvent, type));
    return r;
}

TKeyResponse QSymbianControl::OfferKeyEvent(const TKeyEvent& keyEvent, TEventCode type)
{
    /*
      S60 has a confusing way of delivering key events. There are three types of
      events: EEventKey, EEventKeyDown and EEventKeyUp. When a key is pressed,
      EEventKeyDown is first generated, followed by EEventKey. Then, when the key is
      released, EEventKeyUp is generated.
      However, it is possible that only the EEventKey is generated alone, typically
      in relation to virtual keyboards. In that case we need to take care to
      generate both press and release events in Qt, since applications expect that.
      We do this by having three states for each used scan code, depending on the
      events received. See the switch below for what happens in each state
      transition.
    */

    if (type != EEventKeyDown)
        if (handleVirtualMouse(keyEvent, type) == EKeyWasConsumed)
            return EKeyWasConsumed;

    TKeyResponse ret = EKeyWasNotConsumed;
#define GET_RETURN(x) (ret = ((x) == EKeyWasConsumed) ? EKeyWasConsumed : ret)

    // This top level switch corresponds to the states, and the inner switches
    // correspond to the transitions.
    QS60Data::ScanCodeState &scanCodeState = S60->scanCodeStates[keyEvent.iScanCode];
    switch (scanCodeState) {
    case QS60Data::Unpressed:
        switch (type) {
        case EEventKeyDown:
            scanCodeState = QS60Data::KeyDown;
            break;
        case EEventKey:
            GET_RETURN(sendSymbianKeyEvent(keyEvent, QEvent::KeyPress));
            GET_RETURN(sendSymbianKeyEvent(keyEvent, QEvent::KeyRelease));
            break;
        case EEventKeyUp:
            // No action.
            break;
        }
        break;
    case QS60Data::KeyDown:
        switch (type) {
        case EEventKeyDown:
            // This should never happen, just stay in this state to be safe.
            break;
        case EEventKey:
            GET_RETURN(sendSymbianKeyEvent(keyEvent, QEvent::KeyPress));
            scanCodeState = QS60Data::KeyDownAndKey;
            break;
        case EEventKeyUp:
            scanCodeState = QS60Data::Unpressed;
            break;
        }
        break;
    case QS60Data::KeyDownAndKey:
        switch (type) {
        case EEventKeyDown:
            // This should never happen, just stay in this state to be safe.
            break;
        case EEventKey:
            GET_RETURN(sendSymbianKeyEvent(keyEvent, QEvent::KeyRelease));
            GET_RETURN(sendSymbianKeyEvent(keyEvent, QEvent::KeyPress));
            break;
        case EEventKeyUp:
            GET_RETURN(sendSymbianKeyEvent(keyEvent, QEvent::KeyRelease));
            scanCodeState = QS60Data::Unpressed;
            break;
        }
        break;
    }
    return ret;

#undef GET_RETURN
}

TKeyResponse QSymbianControl::sendSymbianKeyEvent(const TKeyEvent &keyEvent, QEvent::Type type)
{
    // Because S60 does not generate keysyms for EKeyEventDown and EKeyEventUp
    // events, we need to cache the keysyms from the EKeyEvent events. This is what
    // resolveS60ScanCode does.
    TUint s60Keysym = QApplicationPrivate::resolveS60ScanCode(keyEvent.iScanCode,
            keyEvent.iCode);
    int keyCode;
    if (s60Keysym == EKeyNull){ //some key events have 0 in iCode, for them iScanCode should be used
        keyCode = qt_keymapper_private()->mapS60ScanCodesToQt(keyEvent.iScanCode);
    } else if (s60Keysym >= 0x20 && s60Keysym < ENonCharacterKeyBase) {
        // Normal characters keys.
        keyCode = s60Keysym;
    } else {
        // Special S60 keys.
        keyCode = qt_keymapper_private()->mapS60KeyToQt(s60Keysym);
    }

    Qt::KeyboardModifiers mods = mapToQtModifiers(keyEvent.iModifiers);

    TInt code = keyEvent.iCode;

    if (mods == Qt::ControlModifier) {
        //only support ctrl+a .. ctrl+z, 0x40 is the key value before Qt::Key_A
        if (code > 0 && code < 27)
            keyCode = 0x40 + code;
    }	
           
    QKeyEventEx qKeyEvent(type, keyCode, mods, qt_keymapper_private()->translateKeyEvent(keyCode, mods),
            (keyEvent.iRepeats != 0), 1, keyEvent.iScanCode, s60Keysym, keyEvent.iModifiers);
    QWidget *widget;
    widget = QWidget::keyboardGrabber();
    if (!widget) {
        if (QApplicationPrivate::popupWidgets != 0) {
            widget = QApplication::activePopupWidget()->focusWidget();
            if (!widget) {
                widget = QApplication::activePopupWidget();
            }
        } else {
            widget = QApplicationPrivate::focus_widget;
            if (!widget) {
                widget = qwidget;
            }
        }
    }

    QEventDispatcherS60 *dispatcher;
    // It is theoretically possible for someone to install a different event dispatcher.
    if ((dispatcher = qobject_cast<QEventDispatcherS60 *>(widget->d_func()->threadData->eventDispatcher)) != 0) {
        if (dispatcher->excludeUserInputEvents()) {
            dispatcher->saveInputEvent(this, widget, new QKeyEventEx(qKeyEvent));
            return EKeyWasConsumed;
        }
    }
    return sendKeyEvent(widget, &qKeyEvent);
}

TKeyResponse QSymbianControl::handleVirtualMouse(const TKeyEvent& keyEvent,TEventCode type)
{
#ifndef QT_NO_CURSOR
    if (S60->mouseInteractionEnabled && S60->virtualMouseRequired) {
        //translate keys to pointer
        if ((keyEvent.iScanCode >= EStdKeyLeftArrow && keyEvent.iScanCode <= EStdKeyDownArrow) ||
                (keyEvent.iScanCode >= EStdKeyDevice10 && keyEvent.iScanCode <= EStdKeyDevice13) ||
                keyEvent.iScanCode == EStdKeyDevice3) {
            QPoint pos = QCursor::pos();
            TPointerEvent fakeEvent;
            fakeEvent.iType = (TPointerEvent::TType)(-1);
            fakeEvent.iModifiers = keyEvent.iModifiers;
            TInt x = pos.x();
            TInt y = pos.y();
            if (type == EEventKeyUp) {
                S60->virtualMouseAccelTimeout.start();
                switch (keyEvent.iScanCode) {
                case EStdKeyLeftArrow:
                    S60->virtualMousePressedKeys &= ~QS60Data::Left;
                    break;
                case EStdKeyRightArrow:
                    S60->virtualMousePressedKeys &= ~QS60Data::Right;
                    break;
                case EStdKeyUpArrow:
                    S60->virtualMousePressedKeys &= ~QS60Data::Up;
                    break;
                case EStdKeyDownArrow:
                    S60->virtualMousePressedKeys &= ~QS60Data::Down;
                    break;
                // diagonal keys (named aliases don't exist in 3.1 SDK)
                case EStdKeyDevice10:
                    S60->virtualMousePressedKeys &= ~QS60Data::LeftUp;
                    break;
                case EStdKeyDevice11:
                    S60->virtualMousePressedKeys &= ~QS60Data::RightUp;
                    break;
                case EStdKeyDevice12:
                    S60->virtualMousePressedKeys &= ~QS60Data::RightDown;
                    break;
                case EStdKeyDevice13:
                    S60->virtualMousePressedKeys &= ~QS60Data::LeftDown;
                    break;
                case EStdKeyDevice3: //select
                    if (S60->virtualMousePressedKeys & QS60Data::Select)
                        fakeEvent.iType = TPointerEvent::EButton1Up;
                    S60->virtualMousePressedKeys &= ~QS60Data::Select;
                    break;
                }
            }
            else if (type == EEventKey) {
                int dx = 0;
                int dy = 0;
                if (keyEvent.iScanCode != EStdKeyDevice3) {
                    m_doubleClickTimer.invalidate();
                    //reset mouse accelleration after a short time with no moves
                    const int maxTimeBetweenKeyEventsMs = 500;
                    if (S60->virtualMouseAccelTimeout.isValid() &&
                            S60->virtualMouseAccelTimeout.hasExpired(maxTimeBetweenKeyEventsMs)) {
                        S60->virtualMouseAccelDX = 0;
                        S60->virtualMouseAccelDY = 0;
                    }
                    S60->virtualMouseAccelTimeout.invalidate();
                }
                switch (keyEvent.iScanCode) {
                case EStdKeyLeftArrow:
                    S60->virtualMousePressedKeys |= QS60Data::Left;
                    dx = -1;
                    fakeEvent.iType = TPointerEvent::EMove;
                    break;
                case EStdKeyRightArrow:
                    S60->virtualMousePressedKeys |= QS60Data::Right;
                    dx = 1;
                    fakeEvent.iType = TPointerEvent::EMove;
                    break;
                case EStdKeyUpArrow:
                    S60->virtualMousePressedKeys |= QS60Data::Up;
                    dy = -1;
                    fakeEvent.iType = TPointerEvent::EMove;
                    break;
                case EStdKeyDownArrow:
                    S60->virtualMousePressedKeys |= QS60Data::Down;
                    dy = 1;
                    fakeEvent.iType = TPointerEvent::EMove;
                    break;
                case EStdKeyDevice10:
                    S60->virtualMousePressedKeys |= QS60Data::LeftUp;
                    dx = -1;
                    dy = -1;
                    fakeEvent.iType = TPointerEvent::EMove;
                    break;
                case EStdKeyDevice11:
                    S60->virtualMousePressedKeys |= QS60Data::RightUp;
                    dx = 1;
                    dy = -1;
                    fakeEvent.iType = TPointerEvent::EMove;
                    break;
                case EStdKeyDevice12:
                    S60->virtualMousePressedKeys |= QS60Data::RightDown;
                    dx = 1;
                    dy = 1;
                    fakeEvent.iType = TPointerEvent::EMove;
                    break;
                case EStdKeyDevice13:
                    S60->virtualMousePressedKeys |= QS60Data::LeftDown;
                    dx = -1;
                    dy = 1;
                    fakeEvent.iType = TPointerEvent::EMove;
                    break;
                case EStdKeyDevice3:
                    // Platform bug. If you start pressing several keys simultaneously (for
                    // example for drag'n'drop), Symbian starts producing spurious up and
                    // down messages for some keys. Therefore, make sure we have a clean slate
                    // of pressed keys before starting a new button press.
                    if (S60->virtualMousePressedKeys & QS60Data::Select) {
                        return EKeyWasConsumed;
                    } else {
                        S60->virtualMousePressedKeys |= QS60Data::Select;
                        fakeEvent.iType = TPointerEvent::EButton1Down;
                        if (m_doubleClickTimer.isValid()
                                && !m_doubleClickTimer.hasExpired(QApplication::doubleClickInterval())) {
                            fakeEvent.iModifiers |= EModifierDoubleClick;
                            m_doubleClickTimer.invalidate();
                        } else {
                            m_doubleClickTimer.start();
                        }
                    }
                    break;
                }
                if (dx) {
                    int cdx = S60->virtualMouseAccelDX;
                    //reset accel on change of sign, else double accel
                    if (dx * cdx <= 0)
                        cdx = dx;
                    else
                        cdx *= 4;
                    //cap accelleration
                    if (dx * cdx > S60->virtualMouseMaxAccel)
                        cdx = dx * S60->virtualMouseMaxAccel;
                    //move mouse position
                    x += cdx;
                    S60->virtualMouseAccelDX = cdx;
                }

                if (dy) {
                    int cdy = S60->virtualMouseAccelDY;
                    if (dy * cdy <= 0)
                        cdy = dy;
                    else
                        cdy *= 4;
                    if (dy * cdy > S60->virtualMouseMaxAccel)
                        cdy = dy * S60->virtualMouseMaxAccel;
                    y += cdy;
                    S60->virtualMouseAccelDY = cdy;
                }
            }
            //clip to screen size (window server allows a sprite hotspot to be outside the screen)
            int screenNumber = S60->screenNumberForWidget(qwidget);
            if (x < 0)
                x = 0;
            else if (x >= S60->screenWidthInPixelsForScreen[screenNumber])
                x = S60->screenWidthInPixelsForScreen[screenNumber] - 1;
            if (y < 0)
                y = 0;
            else if (y >= S60->screenHeightInPixelsForScreen[screenNumber])
                y = S60->screenHeightInPixelsForScreen[screenNumber] - 1;
            TPoint epos(x, y);
            TPoint cpos = epos - PositionRelativeToScreen();
            fakeEvent.iPosition = cpos;
            fakeEvent.iParentPosition = epos;
            if(fakeEvent.iType != -1)
                HandlePointerEvent(fakeEvent);
            return EKeyWasConsumed;
        }
    }
#endif

    return EKeyWasNotConsumed;
}

void QSymbianControl::sendInputEvent(QWidget *widget, QInputEvent *inputEvent)
{
    switch (inputEvent->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
        sendKeyEvent(widget, static_cast<QKeyEvent *>(inputEvent));
        break;
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
        sendMouseEvent(widget, static_cast<QMouseEvent *>(inputEvent));
        break;
    default:
        // Shouldn't get here.
        Q_ASSERT_X(0 == 1, "QSymbianControl::sendInputEvent()", "inputEvent->type() is unknown");
        break;
    }
}

TKeyResponse QSymbianControl::sendKeyEvent(QWidget *widget, QKeyEvent *keyEvent)
{
#if !defined(QT_NO_IM) && defined(Q_WS_S60)
    if (widget && widget->isEnabled() && widget->testAttribute(Qt::WA_InputMethodEnabled)) {
        QInputContext *qic = widget->inputContext();
        if (qic && qic->filterEvent(keyEvent))
            return EKeyWasConsumed;
    }
#endif // !defined(QT_NO_IM) && defined(Q_OS_SYMBIAN)

    if (widget && qt_sendSpontaneousEvent(widget, keyEvent))
        if (keyEvent->isAccepted())
            return EKeyWasConsumed;

    return EKeyWasNotConsumed;
}

#if !defined(QT_NO_IM) && defined(Q_WS_S60)
TCoeInputCapabilities QSymbianControl::InputCapabilities() const
{
    QWidget *w = 0;

    if (qwidget->hasFocus())
        w = qwidget;
    else
        w = qwidget->focusWidget();

    QCoeFepInputContext *ic;
    if (w && w->isEnabled() && w->testAttribute(Qt::WA_InputMethodEnabled)
            && (ic = qobject_cast<QCoeFepInputContext *>(w->inputContext()))) {
        return ic->inputCapabilities();
    } else {
        return TCoeInputCapabilities(TCoeInputCapabilities::ENone, 0, 0);
    }
}
#endif

void QSymbianControl::Draw(const TRect& aRect) const
{
    int leaveCode = 0;
    int exceptionCode = 0;
    // Implementation of CCoeControl::Draw() must never leave or throw exception.
    // In native Symbian code this is considered a fatal error, and it causes
    // process termination.
    TRAP(leaveCode, QT_TRYCATCH_ERROR(exceptionCode, doDraw(aRect)));
    if (leaveCode)
        qWarning() << "QSymbianControl::doDraw leaved with code " << leaveCode;
    else if (exceptionCode)
        qWarning() << "QSymbianControl::doDraw threw exception with code " << exceptionCode;
}

void QSymbianControl::doDraw(const TRect& controlRect) const
{
    // Bail out immediately, if we don't have a drawing surface. Surface is attempted to be recreated
    // when this application becomes visible for the next time.
    if (S60->eglSurfaceCreationError) {
        qWarning() << "QSymbianControl::doDraw: EGL surface creation has failed, abort";
        return;
    }

    // Set flag to avoid calling DrawNow in window surface
    QWidget *window = qwidget->window();
    Q_ASSERT(window);
    QTLWExtra *topExtra = window->d_func()->maybeTopData();
    Q_ASSERT(topExtra);

    TRect wcontrolRect = translateRectForFixedNativeOrientation(controlRect);

    if (!topExtra->inExpose) {
        topExtra->inExpose = true;
        if (!qwidget->isWindow()) {
            // If we get here, then it means we have a native child window
            // Since no content should ever be painted to these windows, we
            // erase them with a transparent brush when they get an expose.
            CWindowGc &gc = SystemGc();
            gc.SetBrushColor(TRgb(0, 0, 0, 0));
            gc.Clear(controlRect);
        }
        QRect exposeRect = qt_TRect2QRect(wcontrolRect);
        qwidget->d_func()->syncBackingStore(exposeRect);
        topExtra->inExpose = false;
    }

    QWindowSurface *surface = qwidget->windowSurface();
    QPaintEngine *engine = surface ? surface->paintDevice()->paintEngine() : NULL;

    if (!engine)
        return;

    const bool sendNativePaintEvents = qwidget->d_func()->extraData()->receiveNativePaintEvents;
    if (sendNativePaintEvents) {
        const QRect r = qt_TRect2QRect(wcontrolRect);
        QMetaObject::invokeMethod(qwidget, "beginNativePaintEvent", Qt::DirectConnection, Q_ARG(QRect, r));
    }

    // Map source rectangle into coordinates of the backing store.
    const QPoint controlBase(controlRect.iTl.iX, controlRect.iTl.iY);
    const QPoint backingStoreBase = qwidget->mapTo(qwidget->window(), controlBase);
    const TRect backingStoreRect(TPoint(backingStoreBase.x(), backingStoreBase.y()), controlRect.Size());

    if (engine->type() == QPaintEngine::Raster) {
        QS60WindowSurface *s60Surface;
#ifdef QT_GRAPHICSSYSTEM_RUNTIME
        if (QApplicationPrivate::runtime_graphics_system) {
            QRuntimeWindowSurface *rtSurface =
                    static_cast<QRuntimeWindowSurface*>(qwidget->windowSurface());
            s60Surface = static_cast<QS60WindowSurface *>(rtSurface->m_windowSurface.data());
        } else
#endif
            s60Surface = static_cast<QS60WindowSurface *>(qwidget->windowSurface());

        CFbsBitmap *bitmap = s60Surface->symbianBitmap();
        CWindowGc &gc = SystemGc();

        QWExtra::NativePaintMode nativePaintMode = qwidget->d_func()->extraData()->nativePaintMode;
        if(qwidget->d_func()->paintOnScreen())
            nativePaintMode = QWExtra::Disable;

        switch(nativePaintMode) {
        case QWExtra::Disable:
            // Do nothing
            break;
        case QWExtra::Blit:
        case QWExtra::BlitWriteAlpha:
            if (qwidget->d_func()->isOpaque || nativePaintMode == QWExtra::BlitWriteAlpha)
                gc.SetDrawMode(CGraphicsContext::EDrawModeWriteAlpha);
            gc.BitBlt(controlRect.iTl, bitmap, backingStoreRect);
            break;
        case QWExtra::ZeroFill:
            if (Window().DisplayMode() == EColor16MA
                || Window().DisplayMode() == Q_SYMBIAN_ECOLOR16MAP) {
                gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
                gc.SetDrawMode(CGraphicsContext::EDrawModeWriteAlpha);
                gc.SetBrushColor(TRgb::Color16MA(0));
                gc.Clear(controlRect);
            } else {
                gc.SetBrushColor(TRgb(0x000000));
                gc.Clear(controlRect);
            };
            break;
        default:
            Q_ASSERT(false);
        }
    }

    if (sendNativePaintEvents) {
        const QRect r = qt_TRect2QRect(wcontrolRect);
        // The draw ops aren't actually sent to WSERV until the graphics
        // context is deactivated, which happens in the function calling
        // this one.  We therefore delay the delivery of endNativePaintEvent,
        // to ensure that drawing has completed by the time the widget
        // receives the event.  Note that, if the widget needs to ensure
        // that the draw ops have actually been executed into the output
        // framebuffer, a call to RWsSession::Flush is required in the
        // endNativePaintEvent implementation.
        QMetaObject::invokeMethod(qwidget, "endNativePaintEvent", Qt::QueuedConnection, Q_ARG(QRect, r));
    }
}

void QSymbianControl::qwidgetResize_helper(const QSize &newSize)
{
    QRect cr = qwidget->geometry();
    QSize oldSize(cr.size());
    cr.setSize(newSize);
    qwidget->data->crect = cr;
    if (qwidget->isVisible()) {
        QTLWExtra *tlwExtra = qwidget->d_func()->maybeTopData();
        bool slowResize = qgetenv("QT_SLOW_TOPLEVEL_RESIZE").toInt();
        if (!slowResize && tlwExtra)
            tlwExtra->inTopLevelResize = true;
        QResizeEvent e(newSize, oldSize);
        qt_sendSpontaneousEvent(qwidget, &e);
        if (!qwidget->testAttribute(Qt::WA_StaticContents))
            qwidget->d_func()->syncBackingStore();
        if (!slowResize && tlwExtra)
            tlwExtra->inTopLevelResize = false;
    } else {
        if (!qwidget->testAttribute(Qt::WA_PendingResizeEvent)) {
            QResizeEvent *e = new QResizeEvent(newSize, oldSize);
            QApplication::postEvent(qwidget, e);
        }
    }
}

void QSymbianControl::SizeChanged()
{
    CCoeControl::SizeChanged();

    // When FixNativeOrientation had been called, the RWindow/CCoeControl size
    // and the surface/QWidget size have nothing to do with each other.
    if (qwidget->d_func()->fixNativeOrientationCalled)
        return;

    QSize oldSize = qwidget->size();
    QSize newSize(Size().iWidth, Size().iHeight);

    if (oldSize != newSize) {
        // Enforce the proper size for fullscreen widgets on the secondary screen.
        const bool isFullscreen = qwidget->windowState() & Qt::WindowFullScreen;
        const int screenNumber = S60->screenNumberForWidget(qwidget);
        if (!m_inExternalScreenOverride && isFullscreen && screenNumber > 0) {
            int screenWidth = S60->screenWidthInPixelsForScreen[screenNumber];
            int screenHeight = S60->screenHeightInPixelsForScreen[screenNumber];
            TSize screenSize(screenWidth, screenHeight);
            if (screenWidth > 0 && screenHeight > 0 && screenSize != Size()) {
                m_inExternalScreenOverride = true;
                SetExtent(TPoint(0, 0), screenSize);
                return;
            }
        }

        qwidgetResize_helper(newSize);
    }

    m_inExternalScreenOverride = false;

    // CCoeControl::SetExtent calls SizeChanged, but does not call
    // PositionChanged, so we call it here to ensure that the widget's
    // position is updated.
    PositionChanged();
}

void QSymbianControl::PositionChanged()
{
    CCoeControl::PositionChanged();

    QPoint oldPos = qwidget->geometry().topLeft();
    QPoint newPos(Position().iX, Position().iY);

    if (oldPos != newPos) {
        QRect cr = qwidget->geometry();
        cr.moveTopLeft(newPos);
        qwidget->data->crect = cr;
        QTLWExtra *top = qwidget->d_func()->maybeTopData();
        if (top && (qwidget->windowState() & (~Qt::WindowActive)) == Qt::WindowNoState)
            top->normalGeometry.moveTopLeft(newPos);
        if (qwidget->isVisible()) {
            QMoveEvent e(newPos, oldPos);
            qt_sendSpontaneousEvent(qwidget, &e);
        } else {
            QMoveEvent * e = new QMoveEvent(newPos, oldPos);
            QApplication::postEvent(qwidget, e);
        }
    }
}

// Search recursively if there is a child widget that is both visible and focused.
bool QSymbianControl::hasFocusedAndVisibleChild(QWidget *parentWidget)
{
    for (int i = 0; i < parentWidget->children().size(); ++i) {
        QObject *object = parentWidget->children().at(i);
        if (object && object->isWidgetType()) {
            QWidget *w = static_cast<QWidget *>(object);
            WId winId = w->internalWinId();
            if (winId && winId->IsFocused() && winId->IsVisible())
                return true;
            if (hasFocusedAndVisibleChild(w))
                return true;
        }
    }
    return false;
}

void QSymbianControl::FocusChanged(TDrawNow /* aDrawNow */)
{
    QT_TRY {
        if (m_ignoreFocusChanged || (qwidget->windowType() & Qt::WindowType_Mask) == Qt::Desktop)
            return;

        // just in case
        if (qwidget->d_func()->isGLGlobalShareWidget)
            return;

#ifdef Q_WS_S60
        if (S60->splitViewLastWidget)
            return;
#endif

        // Popups never get focused, but still receive the FocusChanged when they are hidden.
        if (QApplicationPrivate::popupWidgets != 0
                || (qwidget->windowType() & Qt::Popup) == Qt::Popup)
            return;

        QWidget *parentWindow = qwidget->window();
        if (IsFocused() && IsVisible()) {
            if (m_symbianPopupIsOpen) {
                QWidget *fw = QApplication::focusWidget();
                if (fw) {
                    QFocusEvent event(QEvent::FocusIn, Qt::PopupFocusReason);
                    QCoreApplication::sendEvent(fw, &event);
                }
                m_symbianPopupIsOpen = false;
            }

            QApplication::setActiveWindow(qwidget->window());
            qwidget->d_func()->setWindowIcon_sys(true);
            qwidget->d_func()->setWindowTitle_sys(qwidget->windowTitle());
#ifdef Q_WS_S60
            if (parentWindow->isWindow())
                S60->setRecursiveDecorationsVisibility(parentWindow, parentWindow->windowState());
#endif
        } else {
            if (QApplication::activeWindow() == parentWindow && !hasFocusedAndVisibleChild(parentWindow)) {
                if (CCoeEnv::Static()->AppUi()->IsDisplayingMenuOrDialog() || S60->menuBeingConstructed) {
                    QWidget *fw = QApplication::focusWidget();
                    if (fw) {
                        QFocusEvent event(QEvent::FocusOut, Qt::PopupFocusReason);
                        QCoreApplication::sendEvent(fw, &event);
                    }
                    m_symbianPopupIsOpen = true;
                    return;
                }

                QApplication::setActiveWindow(0);
            }
        }
        // else { We don't touch the active window unless we were explicitly activated or deactivated }
    } QT_CATCH(const std::exception&) {
        // ignore errors
    }
}

void QSymbianControl::handleClientAreaChange()
{
    const bool cbaVisibilityHint = qwidget->windowFlags() & Qt::WindowSoftkeysVisibleHint;
    if (qwidget->isFullScreen() && !cbaVisibilityHint) {
        SetExtentToWholeScreen();
    } else if (qwidget->isMaximized() || (qwidget->isFullScreen() && cbaVisibilityHint)) {
        // Note that if there is S60->splitViewLastWidget, it means the resizing is done
        // by input context handling and we can use just default ClientRect.
        TRect r = (!S60->splitViewLastWidget) ? S60->clientRect() : static_cast<CEikAppUi*>(S60->appUi())->ClientRect();
        SetExtent(r.iTl, r.Size());
    } else if (!qwidget->isMinimized()) { // Normal geometry
        if (!qwidget->testAttribute(Qt::WA_Resized)) {
            qwidget->adjustSize();
            qwidget->setAttribute(Qt::WA_Resized, false); //not a user resize
        }
        if (!qwidget->testAttribute(Qt::WA_Moved) && qwidget->windowType() != Qt::Dialog) {
            TRect r = S60->clientRect();
            SetPosition(r.iTl);
            qwidget->setAttribute(Qt::WA_Moved, false); // not really an explicit position
        }
    }
}

bool QSymbianControl::isSplitViewWidget(QWidget *widget)
{
    bool returnValue = true;
    // Ignore events sent to non-active windows, not visible widgets and not parents of input widget
    // and non-Qt dialogs.
    if (!qwidget->isActiveWindow()
        || !qwidget->isVisible()
        || !qwidget->isAncestorOf(widget)
        || CCoeEnv::Static()->AppUi()->IsDisplayingMenuOrDialog()) {

        returnValue = false;
    }
    return returnValue;
}

void QSymbianControl::HandleResourceChange(int resourceType)
{
    switch (resourceType) {
    case KSplitViewCloseEvent: //intentional fall-through
    case KSplitViewOpenEvent: {
#if !defined(QT_NO_IM) && defined(Q_WS_S60)

        //Fetch widget getting the text input
        QWidget *widget = QWidget::keyboardGrabber();
        if (!widget) {
            if (QApplicationPrivate::popupWidgets) {
                widget = QApplication::activePopupWidget()->focusWidget();
                if (!widget) {
                    widget = QApplication::activePopupWidget();
                }
            } else {
                widget = QApplicationPrivate::focus_widget;
                if (!widget) {
                    widget = qwidget;
                }
            }
        }
        if (widget) {
            QCoeFepInputContext *ic = qobject_cast<QCoeFepInputContext *>(widget->inputContext());
            if (!ic) {
                ic = qobject_cast<QCoeFepInputContext *>(qApp->inputContext());
            }
            if (ic) {
                if (resourceType == KSplitViewCloseEvent) {
                    S60->partialKeyboardOpen = false;
                    ic->resetSplitViewWidget();
                } else {
                    S60->partialKeyboardOpen = true;
                    if (isSplitViewWidget(widget))
                        ic->ensureFocusWidgetVisible(widget);
                }
            }
        }
#endif // !defined(QT_NO_IM) && defined(Q_WS_S60)
    }
    break;
    case KInternalStatusPaneChange:
        // When status pane is not visible, only handle client area change if status pane was
        // previously visible, as size changes to hidden status pane should not affect
        // client area.
        if (S60->statusPane() && (S60->statusPane()->IsVisible() || m_lastStatusPaneVisibility)) {
            m_lastStatusPaneVisibility = S60->statusPane()->IsVisible();
            if (S60->handleStatusPaneResizeNotifications)
                handleClientAreaChange();
        }
        if (IsFocused() && IsVisible()) {
            qwidget->d_func()->setWindowIcon_sys(true);
            qwidget->d_func()->setWindowTitle_sys(qwidget->windowTitle());
        }
        break;
    case KUidValueCoeFontChangeEvent:
        // font change event
        break;
#ifdef Q_WS_S60
    case KEikDynamicLayoutVariantSwitch:
    {
#ifdef QT_SOFTKEYS_ENABLED
        // Update needed just in case softkeys contain icons
        QSoftKeyManager::updateSoftKeys();
#endif
        handleClientAreaChange();
        // Send resize event to trigger desktopwidget workAreaResized signal
        if (qt_desktopWidget) {
            QResizeEvent e(qt_desktopWidget->size(), qt_desktopWidget->size());
            QApplication::sendEvent(qt_desktopWidget, &e);
        }
        // Send resize event to dialogs so they can adjust their position if necessary.
        if (qwidget->windowType() & Qt::Dialog) {
            QResizeEvent e(qwidget->size(), qwidget->size());
            QApplication::sendEvent(qwidget, &e);
        }
        break;
    }
#endif
    default:
        break;
    }

    CCoeControl::HandleResourceChange(resourceType);

}
void QSymbianControl::CancelLongTapTimer()
{
    if (m_longTapDetector)
        m_longTapDetector->Cancel();
}

TTypeUid::Ptr QSymbianControl::MopSupplyObject(TTypeUid id)
{
    if (id.iUid == ETypeId)
        return id.MakePtr(this);

    return CCoeControl::MopSupplyObject(id);
}

void QSymbianControl::setFocusSafely(bool focus)
{
    if (qwidget->d_func()->isGLGlobalShareWidget)
        return;

    // The stack hack in here is very unfortunate, but it is the only way to ensure proper
    // focus in Symbian. If this is not executed, the control which happens to be on
    // the top of the stack may randomly be assigned focus by Symbian, for example
    // when creating new windows (specifically in CCoeAppUi::HandleStackChanged()).

    // Close any popups.
    CEikonEnv::Static()->EikAppUi()->StopDisplayingMenuBar();

    if (focus) {
        S60->appUi()->RemoveFromStack(this);
        // Symbian doesn't automatically remove focus from the last focused control, so we need to
        // remember it and clear focus ourselves.
        if (lastFocusedControl && lastFocusedControl != this)
            lastFocusedControl->SetFocus(false);
        QT_TRAP_THROWING(S60->appUi()->AddToStackL(this,
                ECoeStackPriorityDefault + 1, ECoeStackFlagStandard)); // Note the + 1
        lastFocusedControl = this;
        this->SetFocus(true);
    } else {
        S60->appUi()->RemoveFromStack(this);
        QT_TRAP_THROWING(S60->appUi()->AddToStackL(this,
                ECoeStackPriorityDefault, ECoeStackFlagStandard));
        if(this == lastFocusedControl)
            lastFocusedControl = 0;
        this->SetFocus(false);
    }
}

bool QSymbianControl::isControlActive()
{
    return IsActivated() ? true : false;
}

void QSymbianControl::ensureFixNativeOrientation()
{
#if defined(Q_SYMBIAN_SUPPORTS_FIXNATIVEORIENTATION)
    if (!qwidget->isWindow() || qwidget->windowType() == Qt::Desktop)
        return;
    if (S60->screenNumberForWidget(qwidget) > 0)
        return;
    const bool isFixed = qwidget->d_func()->fixNativeOrientationCalled;
    const bool isFixEnabled = qwidget->testAttribute(Qt::WA_SymbianNoSystemRotation);
    const bool isFullScreen = qwidget->windowState().testFlag(Qt::WindowFullScreen);
    if (isFullScreen && isFixEnabled) {
        const bool surfaceBasedGs =
            QApplicationPrivate::graphics_system_name == QLatin1String("openvg")
            || QApplicationPrivate::graphics_system_name == QLatin1String("opengl");
        if (!surfaceBasedGs)
            qwidget->setAttribute(Qt::WA_SymbianNoSystemRotation, false);
        if (!isFixed && surfaceBasedGs) {
            if (Window().FixNativeOrientation() == KErrNone) {
                qwidget->d_func()->fixNativeOrientationCalled = true;
                // The EGL window surface is now fixed to the native orientation
                // of the device, no matter what size we pass when creating it.
                // Enforce the same size for the QWidget too. For the underlying
                // CCoeControl and RWindow it is up to the system to resize them
                // when the standard auto-rotation mechanism is in use, we must not
                // change that behavior by forcing any size for those. In practice
                // this means that the QWidget and the underlying native control
                // dimensions will be out of sync when FixNativeOrientation was
                // called and the device is turned to the non-native (typically
                // landscape) orientation. The pointer event handling and certain
                // functions like Draw() will need to compensate for this.
                QSize newSize(S60->nativeScreenWidthInPixels, S60->nativeScreenHeightInPixels);
                if (qwidget->size() != newSize)
                    qwidgetResize_helper(newSize);
            } else {
                qwidget->setAttribute(Qt::WA_SymbianNoSystemRotation, false);
            }
        }
    } else if (isFixed) {
        qwidget->setAttribute(Qt::WA_SymbianNoSystemRotation, false);
        qwidget->d_func()->fixNativeOrientationCalled = false;
        qwidget->hide();
        qwidget->d_func()->create_sys(0, false, true);
        qwidget->show();
    }
#else
    qwidget->setAttribute(Qt::WA_SymbianNoSystemRotation, false);
#endif
}

/*!
    \typedef QApplication::QS60MainApplicationFactory
    \since 4.6

    This is a typedef for a pointer to a function with the following
    signature:

    \snippet doc/src/snippets/code/src_corelib_global_qglobal.cpp 47

    \sa QApplication::QApplication()
*/

/*!
    \since 4.6

    Creates an application using the application factory given in
    \a factory, and using \a argc command line arguments in \a argv.
    \a factory can be leaving, but the error will be converted to a
    standard exception.

    This function is only available on S60.
*/
QApplication::QApplication(QApplication::QS60MainApplicationFactory factory, int &argc, char **argv)
    : QCoreApplication(*new QApplicationPrivate(argc, argv, GuiClient, 0x040000))
{
    Q_D(QApplication);
    S60->s60ApplicationFactory = factory;
    d->construct();
}

QApplication::QApplication(QApplication::QS60MainApplicationFactory factory, int &argc, char **argv, int _internal)
    : QCoreApplication(*new QApplicationPrivate(argc, argv, GuiClient, _internal))
{
    Q_D(QApplication);
    S60->s60ApplicationFactory = factory;
    d->construct();
    QApplicationPrivate::app_compile_version = _internal;
}

void qt_init(QApplicationPrivate * /* priv */, int)
{
    if (!CCoeEnv::Static()) {
        // The S60 framework creates a new trap handler which will render any existing traps
        // invalid as long as it is active. This means that all code in main() that occurs after
        // the QApplication construction needs to be surrounded by a new trap, despite having
        // an outer one already. To avoid this, we save the original trap handler here, and set
        // it back after the S60 framework is constructed. Then we restore it right before the S60
        // framework destruction.
        TTrapHandler *origTrapHandler = User::TrapHandler();

        // The S60 framework has not been initialized. We need to do it.
        TApaApplicationFactory factory(S60->s60ApplicationFactory ?
                S60->s60ApplicationFactory : newS60Application);
        CApaCommandLine* commandLine = q_check_ptr(QCoreApplicationPrivate::symbianCommandLine());
        if (commandLine) {
            // After this construction, CEikonEnv will be available from CEikonEnv::Static().
            // (much like our qApp).
            CEikonEnv* coe = new CEikonEnv;
            //not using QT_TRAP_THROWING, because coe owns the cleanupstack so it can't be pushed there.
            TRAPD(err, coe->ConstructAppFromCommandLineL(factory, *commandLine));
            if(err != KErrNone) {
                qWarning() << "qt_init: Eikon application construct failed ("
                           << err
                           << "), maybe missing resource file on S60 3.1?";
                delete coe;
                qt_symbian_throwIfError(err);
            }
        }

        S60->s60InstalledTrapHandler = User::SetTrapHandler(origTrapHandler);

        S60->qtOwnsS60Environment = true;
    } else {
        S60->qtOwnsS60Environment = false;
    }

#ifdef QT_NO_DEBUG
    if (!qgetenv("QT_S60_AUTO_FLUSH_WSERV").isEmpty())
#endif
        S60->wsSession().SetAutoFlush(ETrue);

#ifdef Q_SYMBIAN_WINDOW_SIZE_CACHE
    TRAP_IGNORE(S60->wsSession().EnableWindowSizeCacheL());
#endif

    S60->updateScreenSize();


    TDisplayMode mode = S60->screenDevice()->DisplayMode();
    S60->screenDepth = TDisplayModeUtils::NumDisplayModeBitsPerPixel(mode);

    //NB: RWsSession::GetColorModeList tells you what window modes are supported,
    //not what bitmap formats.
    if(QSysInfo::symbianVersion() == QSysInfo::SV_9_2)
        S60->supportsPremultipliedAlpha = 0;
    else
        S60->supportsPremultipliedAlpha = 1;

    RProcess me;
    TSecureId securId = me.SecureId();
    S60->uid = securId.operator TUid();

#ifdef QT_SYMBIAN_HAVE_AKNTRANSEFFECT_H
    // Notify uiaccelerator, that we are qt application. This info is used for
    // decision making how startup effects are shown.
    GfxTransEffect::BeginFullScreen(AknTransEffect::ENone,
        TRect(0, 0, 0, 0),
        AknTransEffect::EParameterAvkonInternal,
        AknTransEffect::GfxTransParam(S60->uid, KQtAppExitFlag));
#endif
    // enable focus events - used to re-enable mouse after focus changed between mouse and non mouse app,
    // and for dimming behind modal windows
    S60->windowGroup().EnableFocusChangeEvents();

    //Check if mouse interaction is supported (either EMouse=1 in the HAL, or EMachineUID is one of the phones known to support this)
    const TInt KMachineUidSamsungI8510 = 0x2000C51E;
    // HAL::Get(HALData::EPen, TInt& result) may set 'result' to 1 on some 3.1 systems (e.g. N95).
    // But we know that S60 systems below 5.0 did not support touch.
    static const bool touchIsUnsupportedOnSystem =
        QSysInfo::s60Version() == QSysInfo::SV_S60_3_1
        || QSysInfo::s60Version() == QSysInfo::SV_S60_3_2;
    TInt machineUID;
    TInt mouse;
    TInt touch;
    TInt err;
    err = HAL::Get(HALData::EMouse, mouse);
    if (err != KErrNone)
        mouse = 0;
    err = HAL::Get(HALData::EMachineUid, machineUID);
    if (err != KErrNone)
        machineUID = 0;
    err = HAL::Get(HALData::EPen, touch);
    if (err != KErrNone || touchIsUnsupportedOnSystem)
        touch = 0;
#ifdef __WINS__
    if(QSysInfo::symbianVersion() <= QSysInfo::SV_9_4) {
        //for symbian SDK emulator, force values to match typical devices.
        mouse = 0;
        touch = touchIsUnsupportedOnSystem ? 0 : 1;
    }
#endif
    if (mouse || machineUID == KMachineUidSamsungI8510) {
        S60->hasTouchscreen = false;
        S60->virtualMouseRequired = false;
    }
    else if (!touch) {
        S60->hasTouchscreen = false;
        S60->virtualMouseRequired = true;
    }
    else {
        S60->hasTouchscreen = true;
        S60->virtualMouseRequired = false;
    }

    S60->avkonComponentsSupportTransparency = false;
    S60->menuBeingConstructed = false;

#ifdef Q_WS_S60
    TUid KCRUidAvkon = { 0x101F876E };
    TUint32 KAknAvkonTransparencyEnabled = 0x0000000D;

    CRepository* repository = 0;
    TRAP(err, repository = CRepository::NewL(KCRUidAvkon));

    if(err == KErrNone) {
        TInt value = 0;
        err = repository->Get(KAknAvkonTransparencyEnabled, value);
        if(err == KErrNone) {
            S60->avkonComponentsSupportTransparency = (value==1) ? true : false;
        }
    }
    delete repository;
    repository = 0;
#endif

    qt_keymapper_private()->updateInputLanguage();

#ifdef QT_KEYPAD_NAVIGATION
    if (touch) {
        QApplicationPrivate::navigationMode = Qt::NavigationModeNone;
    } else {
        QApplicationPrivate::navigationMode = Qt::NavigationModeKeypadDirectional;
    }
#endif

#ifndef QT_NO_CURSOR
    //Check if window server pointer cursors are supported or not
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
    //In generic binary, use the HAL and OS version
    //Any other known good phones should be added here.
    if (machineUID == KMachineUidSamsungI8510 || (QSysInfo::symbianVersion() != QSysInfo::SV_9_4
        && QSysInfo::symbianVersion() != QSysInfo::SV_9_3 && QSysInfo::symbianVersion()
        != QSysInfo::SV_9_2)) {
        S60->brokenPointerCursors = false;
        qt_symbian_setWindowGroupCursor(Qt::ArrowCursor, S60->windowGroup());
    }
    else
        S60->brokenPointerCursors = true;
#endif

    if (S60->mouseInteractionEnabled) {
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
        if (S60->brokenPointerCursors) {
            qt_symbian_set_pointer_sprite(Qt::ArrowCursor);
            qt_symbian_show_pointer_sprite();
        }
        else
#endif
            S60->wsSession().SetPointerCursorMode(EPointerCursorNormal);
    }
#endif

    QFont systemFont;
    systemFont.setFamily(systemFont.defaultFamily());
    QApplicationPrivate::setSystemFont(systemFont);

    QObject::connect(qApp, SIGNAL(aboutToQuit()), qApp, SLOT(_q_aboutToQuit()));

#ifdef Q_SYMBIAN_SEMITRANSPARENT_BG_SURFACE
    QApplicationPrivate::instance()->useTranslucentEGLSurfaces = true;

    if (QSymbianGraphicsSystemEx::hasBCM2727()) {
        // We have only 32MB GPU memory. Use raster surfaces
        // for transparent TLWs.
        QApplicationPrivate::instance()->useTranslucentEGLSurfaces = false;
    }

    if (QApplicationPrivate::graphics_system_name == QLatin1String("raster"))
        QApplicationPrivate::instance()->useTranslucentEGLSurfaces = false;
#else
    QApplicationPrivate::instance()->useTranslucentEGLSurfaces = false;
#endif
/*
 ### Commented out for now as parameter handling not needed in SOS(yet). Code below will break testlib with -o flag
    int argc = priv->argc;
    char **argv = priv->argv;

    // Get command line params
    int j = argc ? 1 : 0;
    for (int i=1; i<argc; i++) {
        if (argv[i] && *argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }

#if defined(QT_DEBUG)
        if (qstrcmp(argv[i], "-nograb") == 0)
            appNoGrab = !appNoGrab;
        else
#endif // QT_DEBUG
            ;
    }
*/

    // Register WId with the metatype system.  This is to enable
    // QWidgetPrivate::create_sys to used delayed slot invocation in order
    // to destroy WId objects during reparenting.
    qRegisterMetaType<WId>("WId");
}

#ifdef QT_NO_FREETYPE
extern void qt_cleanup_symbianFontDatabase(); // qfontdatabase_s60.cpp
#endif

/*****************************************************************************
  qt_cleanup() - cleans up when the application is finished
 *****************************************************************************/
void qt_cleanup()
{
    if(qt_S60Beep) {
        delete qt_S60Beep;
        qt_S60Beep = 0;
    }
    QFontCache::cleanup(); // Has to happen now, since QFontEngineS60 has FBS handles
    QPixmapCache::clear(); // Has to happen now, since QSymbianRasterPixmapData has FBS handles

#ifdef QT_NO_FREETYPE
    qt_cleanup_symbianFontDatabase();
#endif
// S60 structure and window server session are freed in eventdispatcher destructor as they are needed there

    // It's important that this happens here, before the event dispatcher gets
    // deleted, because the input context needs the event loop one last time before
    // it dies.
    delete QApplicationPrivate::inputContext;
    QApplicationPrivate::inputContext = 0;

    //Change mouse pointer back
    S60->wsSession().SetPointerCursorMode(EPointerCursorNone);

#ifdef Q_WS_S60
    // Clear CBA
    CEikonEnv::Static()->AppUiFactory()->SwapButtonGroup(0);
    delete S60->buttonGroupContainer();
    S60->setButtonGroupContainer(0);
#endif

    // Call EndFullScreen() to prevent confusing the system effect state machine.
    qt_endFullScreenEffect();

#ifndef QT_NO_CURSOR
    QCursorData::cleanup();
#endif

    if (S60->qtOwnsS60Environment) {
        // Restore the S60 framework trap handler. See qt_init().
        User::SetTrapHandler(S60->s60InstalledTrapHandler);

        CEikonEnv* coe = CEikonEnv::Static();
        coe->PrepareToExit();
        // The CEikonEnv itself is destroyed in here.
        coe->DestroyEnvironment();
    }
}

void QApplicationPrivate::initializeWidgetPaletteHash()
{
    // TODO: Implement QApplicationPrivate::initializeWidgetPaletteHash()
    // Possibly a task fot the S60Style guys
}

void QApplicationPrivate::createEventDispatcher()
{
    Q_Q(QApplication);
    eventDispatcher = new QEventDispatcherS60(q);
}

QString QApplicationPrivate::appName() const
{
    return QCoreApplicationPrivate::appName();
}

bool QApplicationPrivate::modalState()
{
    return app_do_modal;
}

void QApplicationPrivate::enterModal_sys(QWidget *widget)
{
#ifdef Q_SYMBIAN_TRANSITION_EFFECTS
    S60->wsSession().SendEffectCommand(ETfxCmdAppModalModeEnter);
#endif
    if (widget) {
        static_cast<QSymbianControl *>(widget->effectiveWinId())->FadeBehindPopup(ETrue);
        // Modal partial screen dialogs (like queries) capture pointer events.
        // ### FixMe: Add specialized behaviour for fullscreen modal dialogs
        widget->effectiveWinId()->SetGloballyCapturing(ETrue);
        widget->effectiveWinId()->SetPointerCapture(ETrue);
    }
    if (!qt_modal_stack)
        qt_modal_stack = new QWidgetList;
    qt_modal_stack->insert(0, widget);
    app_do_modal = true;
}

void QApplicationPrivate::leaveModal_sys(QWidget *widget)
{
#ifdef Q_SYMBIAN_TRANSITION_EFFECTS
    S60->wsSession().SendEffectCommand(ETfxCmdAppModalModeExit);
#endif
    if (widget) {
        static_cast<QSymbianControl *>(widget->effectiveWinId())->FadeBehindPopup(EFalse);
        // ### FixMe: Add specialized behaviour for fullscreen modal dialogs
        widget->effectiveWinId()->SetGloballyCapturing(EFalse);
        widget->effectiveWinId()->SetPointerCapture(EFalse);
    }
    if (qt_modal_stack && qt_modal_stack->removeAll(widget)) {
        if (qt_modal_stack->isEmpty()) {
            delete qt_modal_stack;
            qt_modal_stack = 0;
        }
    }
    app_do_modal = qt_modal_stack != 0;
}

void QApplicationPrivate::openPopup(QWidget *popup)
{
    if (popup && qobject_cast<QComboBox *>(popup->parentWidget()))
        static_cast<QSymbianControl *>(popup->effectiveWinId())->FadeBehindPopup(ETrue);

    if (!QApplicationPrivate::popupWidgets)
        QApplicationPrivate::popupWidgets = new QWidgetList;
    QApplicationPrivate::popupWidgets->append(popup);

    // Cancel focus widget pointer capture and long tap timer
    if (QApplication::focusWidget() && QApplication::focusWidget()->effectiveWinId()) {
        static_cast<QSymbianControl*>(QApplication::focusWidget()->effectiveWinId())->CancelLongTapTimer();
        QApplication::focusWidget()->effectiveWinId()->SetPointerCapture(false);
        }

    if (!qt_nograb()) {
        // Cancel pointer capture and long tap timer for earlier popup
        int popupCount = QApplicationPrivate::popupWidgets->count();
        if (popupCount > 1) {
            QWidget* prevPopup = QApplicationPrivate::popupWidgets->at(popupCount-2);
            static_cast<QSymbianControl*>(prevPopup->effectiveWinId())->CancelLongTapTimer();
            prevPopup->effectiveWinId()->SetPointerCapture(false);
        }

        // Enable pointer capture for this (topmost) popup
        Q_ASSERT(popup->testAttribute(Qt::WA_WState_Created));
        WId id = popup->effectiveWinId();
        id->SetPointerCapture(true);
    }

    // popups are not focus-handled by the window system (the first
    // popup grabbed the keyboard), so we have to do that manually: A
    // new popup gets the focus
    QWidget *fw = popup->focusWidget();
    if (fw) {
        fw->setFocus(Qt::PopupFocusReason);
    } else if (QApplicationPrivate::popupWidgets->count() == 1) { // this was the first popup
        fw = QApplication::focusWidget();
        if (fw) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            q_func()->sendEvent(fw, &e);
        }
    }
}

void QApplicationPrivate::closePopup(QWidget *popup)
{
    if (popup && qobject_cast<QComboBox *>(popup->parentWidget()))
        static_cast<QSymbianControl *>(popup->effectiveWinId())->FadeBehindPopup(EFalse);

    if (!QApplicationPrivate::popupWidgets)
        return;
    QApplicationPrivate::popupWidgets->removeAll(popup);

    // Cancel pointer capture and long tap for this popup
    WId id = popup->effectiveWinId();
    id->SetPointerCapture(false);
    static_cast<QSymbianControl*>(id)->CancelLongTapTimer();

    if (QApplicationPrivate::popupWidgets->isEmpty()) { // this was the last popup
        delete QApplicationPrivate::popupWidgets;
        QApplicationPrivate::popupWidgets = 0;
        if (!qt_nograb()) {                        // grabbing not disabled
            Q_ASSERT(popup->testAttribute(Qt::WA_WState_Created));
            if (QWidgetPrivate::mouseGrabber != 0)
                QWidgetPrivate::mouseGrabber->grabMouse();

            if (QWidgetPrivate::keyboardGrabber != 0)
                QWidgetPrivate::keyboardGrabber->grabKeyboard();

        QWidget *fw = QApplicationPrivate::active_window ? QApplicationPrivate::active_window->focusWidget()
              : q_func()->focusWidget();
          if (fw) {
              if(fw->window()->isModal()) // restore pointer capture for modal window
                  fw->effectiveWinId()->SetPointerCapture(true);

              if (fw != q_func()->focusWidget()) {
                  fw->setFocus(Qt::PopupFocusReason);
              } else {
                  QFocusEvent e(QEvent::FocusIn, Qt::PopupFocusReason);
                  q_func()->sendEvent(fw, &e);
              }
          }
        }
    } else {

        // popups are not focus-handled by the window system (the
        // first popup grabbed the keyboard), so we have to do that
        // manually: A popup was closed, so the previous popup gets
        // the focus.
        QWidget* aw = QApplicationPrivate::popupWidgets->last();
        if (QWidget *fw = QApplication::focusWidget()) {
            QFocusEvent e(QEvent::FocusOut, Qt::PopupFocusReason);
            q_func()->sendEvent(fw, &e);
        }

        // Enable pointer capture for previous popup
        if (aw) {
            aw->effectiveWinId()->SetPointerCapture(true);
        }
    }
}

QWidget * QApplication::topLevelAt(QPoint const& point)
{
    QWidget *found = 0;
    int lowestZ = INT_MAX;
    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; i < list.count(); ++i) {
        QWidget *widget = list.at(i);
        if (widget->isVisible() && !(widget->windowType() == Qt::Desktop)) {
            Q_ASSERT(widget->testAttribute(Qt::WA_WState_Created));
            if (widget->geometry().adjusted(0,0,1,1).contains(point)) {
                // At this point we know there is a Qt widget under the point.
                // Now we need to make sure it is the top most in the z-order.
                RDrawableWindow *const window = widget->effectiveWinId()->DrawableWindow();
                int z = window->OrdinalPosition();
                if (z < lowestZ) {
                    lowestZ = z;
                    found = widget;
                }
            }
        }
    }
    return found;
}

void QApplication::alert(QWidget * /* widget */, int /* duration */)
{
    // TODO: Implement QApplication::alert(QWidget *widget, int duration)
}

int QApplication::doubleClickInterval()
{
    TTimeIntervalMicroSeconds32 us;
    TInt distance;
    S60->wsSession().GetDoubleClickSettings(us, distance);
    return (us.Int() / 1000);
}

void QApplication::setDoubleClickInterval(int ms)
{
    TTimeIntervalMicroSeconds32 newUs( ms * 1000);
    TTimeIntervalMicroSeconds32 us;
    TInt distance;
    S60->wsSession().GetDoubleClickSettings(us, distance);
    if (us != newUs)
        S60->wsSession().SetDoubleClick(newUs, distance);
}

int QApplication::keyboardInputInterval()
{
    return QApplicationPrivate::keyboard_input_time;
}

void QApplication::setKeyboardInputInterval(int ms)
{
    QApplicationPrivate::keyboard_input_time = ms;
}

int QApplication::cursorFlashTime()
{
    return QApplicationPrivate::cursor_flash_time;
}

void QApplication::setCursorFlashTime(int msecs)
{
    QApplicationPrivate::cursor_flash_time = msecs;
}

void QApplication::beep()
{
    if (!qt_S60Beep) {
        TInt frequency = 880;
        TTimeIntervalMicroSeconds duration(500000);
        TRAP_IGNORE(qt_S60Beep=QS60Beep::NewL(frequency, duration));
    }
    if (qt_S60Beep)
        qt_S60Beep->Play();
}

static inline bool callSymbianEventFilters(const QSymbianEvent *event)
{
    long unused;
    return qApp->filterEvent(const_cast<QSymbianEvent *>(event), &unused);
}

/*!
    \warning This function is only available on Symbian.
    \since 4.6

    This function processes an individual Symbian event
    \a event. It returns 1 if the event was handled, 0 if
    the \a event was not handled, and -1 if the event was
    not handled because the event is not known to Qt.
 */

int QApplication::symbianProcessEvent(const QSymbianEvent *event)
{
    Q_D(QApplication);

    QScopedLoopLevelCounter counter(d->threadData);

    QT_TRY {
        if (d->eventDispatcher->filterEvent(const_cast<QSymbianEvent *>(event)))
            return 1;

        QWidget *w = qApp ? qApp->focusWidget() : 0;
        if (w) {
            QInputContext *ic = w->inputContext();
            if (ic && ic->symbianFilterEvent(w, event))
                return 1;
        }

        if (symbianEventFilter(event))
            return 1;
    } QT_CATCH(const std::exception& ex) {
        // don't allow an exception to stop exit command handling
        if (event->type() != QSymbianEvent::CommandEvent || event->command() != EEikCmdExit)
            QT_RETHROW;
    }

    switch (event->type()) {
    case QSymbianEvent::WindowServerEvent:
        return d->symbianProcessWsEvent(event);
    case QSymbianEvent::CommandEvent:
        return d->symbianHandleCommand(event);
    case QSymbianEvent::ResourceChangeEvent:
        return d->symbianResourceChange(event);
    default:
        return -1;
    }
}

int QApplicationPrivate::symbianProcessWsEvent(const QSymbianEvent *symbianEvent)
{
    // Qt event handling. Handle some events regardless of if the handle is in our
    // widget map or not.
    const TWsEvent *event = symbianEvent->windowServerEvent();
    CCoeControl* control = reinterpret_cast<CCoeControl*>(event->Handle());
    const bool controlInMap = QWidgetPrivate::mapper && QWidgetPrivate::mapper->contains(control);
    switch (event->Type()) {
    case EEventPointerEnter:
        if (controlInMap) {
            callSymbianEventFilters(symbianEvent);
            return 1; // Qt::Enter will be generated in HandlePointerL
        }
        break;
    case EEventPointerExit:
        if (controlInMap) {
            if (callSymbianEventFilters(symbianEvent))
                return 1;
            if (S60) {
                // mouseEvent outside our window, send leave event to last focused widget
                QMouseEvent mEvent(QEvent::Leave, S60->lastPointerEventPos, S60->lastCursorPos,
                    Qt::NoButton, QApplicationPrivate::mouse_buttons, Qt::NoModifier);
                if (S60->lastPointerEventTarget)
                    qt_sendSpontaneousEvent(S60->lastPointerEventTarget,&mEvent);
                S60->lastPointerEventTarget = 0;
            }
            return 1;
        }
        break;
    case EEventScreenDeviceChanged: // fallthrough
#if defined(Q_SYMBIAN_SUPPORTS_MULTIPLE_SCREENS)
    case EEventDisplayChanged:
#endif
        {
        if (callSymbianEventFilters(symbianEvent))
            return 1;
        if (S60)
            S60->updateScreenSize();
        if (qt_desktopWidget) {
            QSize oldSize = qt_desktopWidget->size();
            qt_desktopWidget->data->crect.setWidth(S60->screenWidthInPixels);
            qt_desktopWidget->data->crect.setHeight(S60->screenHeightInPixels);
            QResizeEvent e(qt_desktopWidget->size(), oldSize);
            QApplication::sendEvent(qt_desktopWidget, &e);
        }
        // Close non-native QMenus (that should act like context menus, i.e. close
        // automatically when the orientation changes).
        QMenu *activeMenu = qobject_cast<QMenu *>(QApplication::activePopupWidget());
        if (activeMenu)
            activeMenu->close();
        }
        return 0; // Propagate to CONE
    case EEventWindowVisibilityChanged:
        if (controlInMap) {
            if (callSymbianEventFilters(symbianEvent))
                return 1;
            const TWsVisibilityChangedEvent *visChangedEvent = event->VisibilityChanged();
            if (visChangedEvent->iFlags & TWsVisibilityChangedEvent::ENotVisible)
                S60->controlVisibilityChanged(control, false);
            else if (visChangedEvent->iFlags & TWsVisibilityChangedEvent::EPartiallyVisible)
                S60->controlVisibilityChanged(control, true);
            return 1;
        }
        break;
    case EEventFocusGained:
        if (callSymbianEventFilters(symbianEvent))
            return 1;
#ifndef QT_NO_CURSOR
        //re-enable mouse interaction
        if (S60->mouseInteractionEnabled) {
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
            if (S60->brokenPointerCursors)
                qt_symbian_show_pointer_sprite();
            else
#endif
                S60->wsSession().SetPointerCursorMode(EPointerCursorNormal);
        }
#endif
#ifdef QT_SOFTKEYS_ENABLED
        if (!CEikonEnv::Static()->EikAppUi()->IsDisplayingMenuOrDialog())
            QSoftKeyManager::updateSoftKeys();
#endif
        break;
    case EEventFocusLost:
        if (callSymbianEventFilters(symbianEvent))
            return 1;
#ifndef QT_NO_CURSOR
        //disable mouse as may be moving to application that does not support it
        if (S60->mouseInteractionEnabled) {
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
            if (S60->brokenPointerCursors)
                qt_symbian_hide_pointer_sprite();
            else
#endif
                S60->wsSession().SetPointerCursorMode(EPointerCursorNone);
        }
#endif
        break;
    case KGoomMemoryLowEvent:
#ifdef QT_DEBUG
        qDebug() << "QApplicationPrivate::symbianProcessWsEvent - KGoomMemoryLowEvent";
#endif
        if (callSymbianEventFilters(symbianEvent))
            return 1;
#ifdef QT_GRAPHICSSYSTEM_RUNTIME
        if(QApplicationPrivate::runtime_graphics_system) {
            bool switchToSwRendering(false);

            foreach (QWidget *w, QApplication::topLevelWidgets()) {
                if(w->d_func()->topData()->backingStore) {
                    switchToSwRendering = true;
                    break;
                }
            }

            if (switchToSwRendering) {
                QRuntimeGraphicsSystem *gs =
                   static_cast<QRuntimeGraphicsSystem*>(QApplicationPrivate::graphics_system);
                gs->setGraphicsSystem(QLatin1String("raster"));
            }
        }
#endif
        break;
    case KGoomMemoryGoodEvent:
#ifdef QT_DEBUG
        qDebug() << "QApplicationPrivate::symbianProcessWsEvent - KGoomMemoryGoodEvent";
#endif
        if (callSymbianEventFilters(symbianEvent))
            return 1;
#ifdef QT_GRAPHICSSYSTEM_RUNTIME
        if(QApplicationPrivate::runtime_graphics_system) {
            QRuntimeGraphicsSystem *gs =
                   static_cast<QRuntimeGraphicsSystem*>(QApplicationPrivate::graphics_system);
            gs->setGraphicsSystem(QLatin1String("openvg"));
        }
#endif
        break;
#ifdef Q_SYMBIAN_SUPPORTS_SURFACES
    case EEventUser:
        {
            // GOOM is looking for candidates to kill so indicate that we are
            // capable of cleaning up by handling this event
            TInt32 *data = reinterpret_cast<TInt32 *>(event->EventData());
            if (data[0] == EApaSystemEventShutdown && data[1] == KGoomMemoryLowEvent)
                return 1;
        }
        break;
#endif

#ifdef Q_WS_S60
    case KEikInputLanguageChange:
        qt_keymapper_private()->updateInputLanguage();
        break;
#endif

    default:
        break;
    }

    if (!controlInMap)
        return -1;

    return 0;
}

/*!
  \warning This virtual function is only available on Symbian.
  \since 4.6

  If you create an application that inherits QApplication and reimplement
  this function, you get direct access to events that the are received
  from Symbian. The events are passed in the \a event parameter.

  Return true if you want to stop the event from being processed. Return
  false for normal event dispatching. The default implementation returns
  false, and does nothing with \a event.
 */
bool QApplication::symbianEventFilter(const QSymbianEvent *event)
{
    Q_UNUSED(event);
    return false;
}

/*!
  \warning This function is only available on Symbian.
  \since 4.6

  Handles \a{command}s which are typically handled by
  CAknAppUi::HandleCommandL(). Qts Ui integration into Symbian is
  partially achieved by deriving from CAknAppUi. Currently, exit,
  menu and softkey commands are handled.

  \sa s60EventFilter(), s60ProcessEvent()
*/
int QApplicationPrivate::symbianHandleCommand(const QSymbianEvent *symbianEvent)
{
    Q_Q(QApplication);
    int ret = 0;

    if (callSymbianEventFilters(symbianEvent))
        return 1;

    int command = symbianEvent->command();

    switch (command) {
#ifdef Q_WS_S60
    case EAknSoftkeyExit: {
        QCloseEvent ev;
        QApplication::sendSpontaneousEvent(q, &ev);
        if (ev.isAccepted()) {
            q->quit();
            ret = 1;
        }
        break;
    }
#endif
    case EEikCmdExit:
        q->quit();
        ret = 1;
        break;
    default:
#ifdef Q_WS_S60
        bool handled = QSoftKeyManager::handleCommand(command);
        if (handled)
            ret = 1;
        else
            ret = QMenuBarPrivate::symbianCommands(command);
#endif
        break;
    }

    return ret;
}

/*!
  \warning This function is only available on Symbian.
  \since 4.6

  Handles the resource change specified by \a type.

  Currently, KEikDynamicLayoutVariantSwitch and
  KAknsMessageSkinChange are handled.
 */
int QApplicationPrivate::symbianResourceChange(const QSymbianEvent *symbianEvent)
{
    int ret = 0;

    int type = symbianEvent->resourceChangeType();

    switch (type) {
#ifdef Q_WS_S60
    case KEikDynamicLayoutVariantSwitch:
        {
        if (callSymbianEventFilters(symbianEvent))
            return 1;
        if (S60)
            S60->updateScreenSize();

#ifndef QT_NO_STYLE_S60
        QS60Style *s60Style = 0;

#ifndef QT_NO_STYLE_STYLESHEET
        QStyleSheetStyle *proxy = qobject_cast<QStyleSheetStyle*>(QApplication::style());
        if (proxy)
            s60Style = qobject_cast<QS60Style*>(proxy->baseStyle());
        else
#endif
            s60Style = qobject_cast<QS60Style*>(QApplication::style());

        if (s60Style) {
            s60Style->d_func()->handleDynamicLayoutVariantSwitch();
            ret = 1;
        }
#endif
        }
        break;

#ifndef QT_NO_STYLE_S60
    case KAknsMessageSkinChange:
        if (callSymbianEventFilters(symbianEvent))
            return 1;
        if (QS60Style *s60Style = qobject_cast<QS60Style*>(QApplication::style())) {
            s60Style->d_func()->handleSkinChange();
            ret = 1;
        }
        break;
#endif
#endif // Q_WS_S60
    default:
        break;
    }

    return ret;
}

void QApplicationPrivate::symbianHandleLiteModeStartup()
{
    if (QCoreApplication::arguments().contains(QLatin1String("--startup-lite"))) {
        if (!QApplication::testAttribute(Qt::AA_S60DontConstructApplicationPanes)
                && !S60->buttonGroupContainer() && !S60->statusPane()) {
            // hide and force this app to the background before creating screen furniture to avoid flickers
            CAknAppUi *appui = static_cast<CAknAppUi*>(CCoeEnv::Static()->AppUi());
            if (appui)
                appui->HideApplicationFromFSW(ETrue);
            CCoeEnv::Static()->RootWin().SetOrdinalPosition(-1);
            S60->createStatusPaneAndCBA();
            if (S60->statusPane()) {
                S60->setStatusPaneAndButtonGroupVisibility(false, false);
            }
        }
    }
}

#ifndef QT_NO_WHEELEVENT
int QApplication::wheelScrollLines()
{
    return QApplicationPrivate::wheel_scroll_lines;
}

void QApplication::setWheelScrollLines(int n)
{
    QApplicationPrivate::wheel_scroll_lines = n;
}
#endif //QT_NO_WHEELEVENT

bool QApplication::isEffectEnabled(Qt::UIEffect /* effect */)
{
    // TODO: Implement QApplication::isEffectEnabled(Qt::UIEffect effect)
    return false;
}

void QApplication::setEffectEnabled(Qt::UIEffect /* effect */, bool /* enable */)
{
    // TODO: Implement QApplication::setEffectEnabled(Qt::UIEffect effect, bool enable)
}

Qt::KeyboardModifiers QApplication::queryKeyboardModifiers()
{
    return app_keyboardModifiers;
}

TUint QApplicationPrivate::resolveS60ScanCode(TInt scanCode, TUint keysym)
{
    if (!scanCode)
        return keysym;

    QApplicationPrivate *d = QApplicationPrivate::instance();

    if (keysym) {
        // If keysym is specified, cache it.
        d->scanCodeCache.insert(scanCode, keysym);
        return keysym;
    } else {
        // If not, retrieve the cached version.
        return d->scanCodeCache[scanCode];
    }
}

void QApplicationPrivate::initializeMultitouch_sys()
{
#ifdef QT_SYMBIAN_SUPPORTS_ADVANCED_POINTER
    if (HAL::Get(HALData::EPointer3DPressureSupported, pressureSupported) != KErrNone)
        pressureSupported = 0;
    if (HAL::Get(HALData::EPointer3DMaxPressure, maxTouchPressure) != KErrNone)
        maxTouchPressure = KMaxTInt;
#else
    pressureSupported = 0;
    maxTouchPressure = KMaxTInt;
#endif
}

void QApplicationPrivate::cleanupMultitouch_sys()
{ }

#ifndef QT_NO_SESSIONMANAGER
QSessionManager::QSessionManager(QApplication * /* app */, QString & /* id */, QString& /* key */)
{

}

QSessionManager::~QSessionManager()
{

}

bool QSessionManager::allowsInteraction()
{
    return false;
}

void QSessionManager::cancel()
{

}
#endif //QT_NO_SESSIONMANAGER

#ifdef QT_KEYPAD_NAVIGATION
/*
 * Show/Hide the mouse cursor depending on phone type and chosen mode
 */
void QApplicationPrivate::setNavigationMode(Qt::NavigationMode mode)
{
#ifndef QT_NO_CURSOR
    const bool wasCursorOn = (QApplicationPrivate::navigationMode == Qt::NavigationModeCursorAuto
        && !S60->hasTouchscreen)
        || QApplicationPrivate::navigationMode == Qt::NavigationModeCursorForceVisible;
    const bool isCursorOn = (mode == Qt::NavigationModeCursorAuto
        && !S60->hasTouchscreen)
        || mode == Qt::NavigationModeCursorForceVisible;

    if (!wasCursorOn && isCursorOn) {
        //Show the cursor, when changing from another mode to cursor mode
        qt_symbian_set_cursor_visible(true);
    }
    else if (wasCursorOn && !isCursorOn) {
        //Hide the cursor, when leaving cursor mode
        qt_symbian_set_cursor_visible(false);
    }
#endif
    QApplicationPrivate::navigationMode = mode;
}
#endif

#ifndef QT_NO_CURSOR
/*****************************************************************************
 QApplication cursor stack
 *****************************************************************************/

void QApplication::setOverrideCursor(const QCursor &cursor)
{
    qApp->d_func()->cursor_list.prepend(cursor);
    qt_symbian_setGlobalCursor(cursor);
}

void QApplication::restoreOverrideCursor()
{
    if (qApp->d_func()->cursor_list.isEmpty())
        return;
    qApp->d_func()->cursor_list.removeFirst();

    if (!qApp->d_func()->cursor_list.isEmpty()) {
        qt_symbian_setGlobalCursor(qApp->d_func()->cursor_list.first());
    }
    else {
        //determine which widget has focus
        QWidget *w = QApplication::widgetAt(QCursor::pos());
#ifndef Q_SYMBIAN_FIXED_POINTER_CURSORS
        if (S60->brokenPointerCursors) {
            qt_symbian_set_pointer_sprite(w ? w->cursor() : Qt::ArrowCursor);
        }
        else
#endif
        {
            //because of the internals of window server, we need to force the cursor
            //to be set in all child windows too, otherwise when the cursor is over
            //the child window it may show a widget cursor or arrow cursor instead,
            //depending on construction order.
            QListIterator<WId> iter(QWidgetPrivate::mapper->uniqueKeys());
            while (iter.hasNext()) {
                CCoeControl *ctrl = iter.next();
                if(ctrl->OwnsWindow()) {
                    ctrl->DrawableWindow()->ClearPointerCursor();
                }
            }
            if (w)
                qt_symbian_setWindowCursor(w->cursor(), w->effectiveWinId());
            else
                qt_symbian_setWindowGroupCursor(Qt::ArrowCursor, S60->windowGroup());
        }
    }
}

#endif // QT_NO_CURSOR

void QApplicationPrivate::_q_aboutToQuit()
{
    qt_beginFullScreenEffect();

#ifdef Q_SYMBIAN_TRANSITION_EFFECTS
    // Send the shutdown tfx command
    S60->wsSession().SendEffectCommand(ETfxCmdAppShutDown);
#endif
}

void QApplicationPrivate::emitAboutToReleaseGpuResources()
{
#ifdef Q_SYMBIAN_SUPPORTS_SURFACES
    Q_Q(QApplication);
    QPointer<QApplication> guard(q);
    emit q->aboutToReleaseGpuResources();
#endif
}

void QApplicationPrivate::emitAboutToUseGpuResources()
{
#ifdef Q_SYMBIAN_SUPPORTS_SURFACES
    Q_Q(QApplication);
    QPointer<QApplication> guard(q);
    emit q->aboutToUseGpuResources();
#endif
}

QS60ThreadLocalData::QS60ThreadLocalData()
{
    CCoeEnv *env = CCoeEnv::Static();
    if (env) {
        //if this is the UI thread, share objects owned by CONE
        usingCONEinstances = true;
        wsSession = env->WsSession();
        screenDevice = env->ScreenDevice();
    }
    else {
        usingCONEinstances = false;
        qt_symbian_throwIfError(wsSession.Connect(qt_s60GetRFs()));
        screenDevice = new CWsScreenDevice(wsSession);
        screenDevice->Construct();
    }
}

QS60ThreadLocalData::~QS60ThreadLocalData()
{
    for (int i = 0; i < releaseFuncs.count(); ++i)
        releaseFuncs[i]();
    releaseFuncs.clear();
    if (!usingCONEinstances) {
        delete screenDevice;
        wsSession.Close();
    }
}

QT_END_NAMESPACE

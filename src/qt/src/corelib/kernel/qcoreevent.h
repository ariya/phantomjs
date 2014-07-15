/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QCOREEVENT_H
#define QCOREEVENT_H

#include <QtCore/qnamespace.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qobjectdefs.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QEventPrivate;
class Q_CORE_EXPORT QEvent           // event base class
{
    Q_GADGET
    QDOC_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)
    Q_ENUMS(Type)
public:
    enum Type {
        /*
          If you get a strange compiler error on the line with None,
          it's probably because you're also including X11 headers,
          which #define the symbol None. Put the X11 includes after
          the Qt includes to solve this problem.
        */
        None = 0,                               // invalid event
        Timer = 1,                              // timer event
        MouseButtonPress = 2,                   // mouse button pressed
        MouseButtonRelease = 3,                 // mouse button released
        MouseButtonDblClick = 4,                // mouse button double click
        MouseMove = 5,                          // mouse move
        KeyPress = 6,                           // key pressed
        KeyRelease = 7,                         // key released
        FocusIn = 8,                            // keyboard focus received
        FocusOut = 9,                           // keyboard focus lost
        Enter = 10,                             // mouse enters widget
        Leave = 11,                             // mouse leaves widget
        Paint = 12,                             // paint widget
        Move = 13,                              // move widget
        Resize = 14,                            // resize widget
        Create = 15,                            // after widget creation
        Destroy = 16,                           // during widget destruction
        Show = 17,                              // widget is shown
        Hide = 18,                              // widget is hidden
        Close = 19,                             // request to close widget
        Quit = 20,                              // request to quit application
        ParentChange = 21,                      // widget has been reparented
        ParentAboutToChange = 131,              // sent just before the parent change is done
#ifdef QT3_SUPPORT
        Reparent = ParentChange,
#endif
        ThreadChange = 22,                      // object has changed threads
        WindowActivate = 24,                    // window was activated
        WindowDeactivate = 25,                  // window was deactivated
        ShowToParent = 26,                      // widget is shown to parent
        HideToParent = 27,                      // widget is hidden to parent
        Wheel = 31,                             // wheel event
        WindowTitleChange = 33,                 // window title changed
        WindowIconChange = 34,                  // icon changed
        ApplicationWindowIconChange = 35,       // application icon changed
        ApplicationFontChange = 36,             // application font changed
        ApplicationLayoutDirectionChange = 37,  // application layout direction changed
        ApplicationPaletteChange = 38,          // application palette changed
        PaletteChange = 39,                     // widget palette changed
        Clipboard = 40,                         // internal clipboard event
        Speech = 42,                            // reserved for speech input
        MetaCall =  43,                         // meta call event
        SockAct = 50,                           // socket activation
        WinEventAct = 132,                      // win event activation
        DeferredDelete = 52,                    // deferred delete event
        DragEnter = 60,                         // drag moves into widget
        DragMove = 61,                          // drag moves in widget
        DragLeave = 62,                         // drag leaves or is cancelled
        Drop = 63,                              // actual drop
        DragResponse = 64,                      // drag accepted/rejected
        ChildAdded = 68,                        // new child widget
        ChildPolished = 69,                     // polished child widget
#ifdef QT3_SUPPORT
        ChildInsertedRequest = 67,              // send ChildInserted compatibility events to receiver
        ChildInserted = 70,                     // compatibility child inserted
        LayoutHint = 72,                        // compatibility relayout request
#endif
        ChildRemoved = 71,                      // deleted child widget
        ShowWindowRequest = 73,                 // widget's window should be mapped
        PolishRequest = 74,                     // widget should be polished
        Polish = 75,                            // widget is polished
        LayoutRequest = 76,                     // widget should be relayouted
        UpdateRequest = 77,                     // widget should be repainted
        UpdateLater = 78,                       // request update() later

        EmbeddingControl = 79,                  // ActiveX embedding
        ActivateControl = 80,                   // ActiveX activation
        DeactivateControl = 81,                 // ActiveX deactivation
        ContextMenu = 82,                       // context popup menu
        InputMethod = 83,                       // input method
        AccessibilityPrepare = 86,              // accessibility information is requested
        TabletMove = 87,                        // Wacom tablet event
        LocaleChange = 88,                      // the system locale changed
        LanguageChange = 89,                    // the application language changed
        LayoutDirectionChange = 90,             // the layout direction changed
        Style = 91,                             // internal style event
        TabletPress = 92,                       // tablet press
        TabletRelease = 93,                     // tablet release
        OkRequest = 94,                         // CE (Ok) button pressed
        HelpRequest = 95,                       // CE (?)  button pressed

        IconDrag = 96,                          // proxy icon dragged

        FontChange = 97,                        // font has changed
        EnabledChange = 98,                     // enabled state has changed
        ActivationChange = 99,                  // window activation has changed
        StyleChange = 100,                      // style has changed
        IconTextChange = 101,                   // icon text has changed
        ModifiedChange = 102,                   // modified state has changed
        MouseTrackingChange = 109,              // mouse tracking state has changed

        WindowBlocked = 103,                    // window is about to be blocked modally
        WindowUnblocked = 104,                  // windows modal blocking has ended
        WindowStateChange = 105,

        ToolTip = 110,
        WhatsThis = 111,
        StatusTip = 112,

        ActionChanged = 113,
        ActionAdded = 114,
        ActionRemoved = 115,

        FileOpen = 116,                         // file open request

        Shortcut = 117,                         // shortcut triggered
        ShortcutOverride = 51,                  // shortcut override request

#ifdef QT3_SUPPORT
        Accel = 30,                             // accelerator event
        AccelAvailable = 32,                    // accelerator available event
        AccelOverride = ShortcutOverride,       // accelerator override event
#endif

        WhatsThisClicked = 118,

#ifdef QT3_SUPPORT
        CaptionChange = WindowTitleChange,
        IconChange = WindowIconChange,
#endif
        ToolBarChange = 120,                    // toolbar visibility toggled

        ApplicationActivate = 121,              // application has been changed to active
        ApplicationActivated = ApplicationActivate, // deprecated
        ApplicationDeactivate = 122,            // application has been changed to inactive
        ApplicationDeactivated = ApplicationDeactivate, // deprecated

        QueryWhatsThis = 123,                   // query what's this widget help
        EnterWhatsThisMode = 124,
        LeaveWhatsThisMode = 125,

        ZOrderChange = 126,                     // child widget has had its z-order changed

        HoverEnter = 127,                       // mouse cursor enters a hover widget
        HoverLeave = 128,                       // mouse cursor leaves a hover widget
        HoverMove = 129,                        // mouse cursor move inside a hover widget

        AccessibilityHelp = 119,                // accessibility help text request
        AccessibilityDescription = 130,         // accessibility description text request

        // last event id used = 132

#ifdef QT_KEYPAD_NAVIGATION
        EnterEditFocus = 150,                   // enter edit mode in keypad navigation (Defined only with QT_KEYPAD_NAVIGATION)
        LeaveEditFocus = 151,                   // leave edit mode in keypad navigation (Defined only with QT_KEYPAD_NAVIGATION)
#endif
        AcceptDropsChange = 152,

        MenubarUpdated = 153,                    // Support event for Q3MainWindow, which needs to
                                                 // knwow when QMenubar is updated.

        ZeroTimerEvent = 154,                   // Used for Windows Zero timer events

        GraphicsSceneMouseMove = 155,           // GraphicsView
        GraphicsSceneMousePress = 156,
        GraphicsSceneMouseRelease = 157,
        GraphicsSceneMouseDoubleClick = 158,
        GraphicsSceneContextMenu = 159,
        GraphicsSceneHoverEnter = 160,
        GraphicsSceneHoverMove = 161,
        GraphicsSceneHoverLeave = 162,
        GraphicsSceneHelp = 163,
        GraphicsSceneDragEnter = 164,
        GraphicsSceneDragMove = 165,
        GraphicsSceneDragLeave = 166,
        GraphicsSceneDrop = 167,
        GraphicsSceneWheel = 168,

        KeyboardLayoutChange = 169,             // keyboard layout changed

        DynamicPropertyChange = 170,            // A dynamic property was changed through setProperty/property

        TabletEnterProximity = 171,
        TabletLeaveProximity = 172,

        NonClientAreaMouseMove = 173,
        NonClientAreaMouseButtonPress = 174,
        NonClientAreaMouseButtonRelease = 175,
        NonClientAreaMouseButtonDblClick = 176,

        MacSizeChange = 177,                    // when the Qt::WA_Mac{Normal,Small,Mini}Size changes

        ContentsRectChange = 178,               // sent by QWidget::setContentsMargins (internal)

        MacGLWindowChange = 179,                // Internal! the window of the GLWidget has changed

        FutureCallOut = 180,

        GraphicsSceneResize  = 181,
        GraphicsSceneMove  = 182,

        CursorChange = 183,
        ToolTipChange = 184,

        NetworkReplyUpdated = 185,              // Internal for QNetworkReply

        GrabMouse = 186,
        UngrabMouse = 187,
        GrabKeyboard = 188,
        UngrabKeyboard = 189,
        MacGLClearDrawable = 191,               // Internal Cocoa, the window has changed, so we must clear

        StateMachineSignal = 192,
        StateMachineWrapped = 193,

        TouchBegin = 194,
        TouchUpdate = 195,
        TouchEnd = 196,

#ifndef QT_NO_GESTURES
        NativeGesture = 197,                    // Internal for platform gesture support
#endif
        RequestSoftwareInputPanel = 199,
        CloseSoftwareInputPanel = 200,

        UpdateSoftKeys = 201,                   // Internal for compressing soft key updates

        WinIdChange = 203,
#ifndef QT_NO_GESTURES
        Gesture = 198,
        GestureOverride = 202,
#endif

        PlatformPanel = 212,

        // 512 reserved for Qt Jambi's MetaCall event
        // 513 reserved for Qt Jambi's DeleteOnMainThread event

        User = 1000,                            // first user event id
        MaxUser = 65535                         // last user event id
    };

    QEvent(Type type);
    virtual ~QEvent();
    inline Type type() const { return static_cast<Type>(t); }
    inline bool spontaneous() const { return spont; }

    inline void setAccepted(bool accepted) { m_accept = accepted; }
    inline bool isAccepted() const { return m_accept; }

    inline void accept() { m_accept = true; }
    inline void ignore() { m_accept = false; }

    static int registerEventType(int hint = -1);

protected:
    QEventPrivate *d;
    ushort t;

private:
    ushort posted : 1;
    ushort spont : 1;
    ushort m_accept : 1;
    ushort reserved : 13;

    friend class QCoreApplication;
    friend class QCoreApplicationPrivate;
    friend class QThreadData;
    friend class QApplication;
    friend class QApplicationPrivate;
    friend class Q3AccelManager;
    friend class QShortcutMap;
    friend class QETWidget;
    friend class QGraphicsView;
    friend class QGraphicsViewPrivate;
    friend class QGraphicsScene;
    friend class QGraphicsScenePrivate;
#ifndef QT_NO_GESTURES
    friend class QGestureManager;
#endif
};

class Q_CORE_EXPORT QTimerEvent : public QEvent
{
public:
    QTimerEvent( int timerId );
    ~QTimerEvent();
    int timerId() const { return id; }
protected:
    int id;
};

class QObject;

class Q_CORE_EXPORT QChildEvent : public QEvent
{
public:
    QChildEvent( Type type, QObject *child );
    ~QChildEvent();
    QObject *child() const { return c; }
    bool added() const { return type() == ChildAdded; }
#ifdef QT3_SUPPORT
    QT3_SUPPORT bool inserted() const { return type() == ChildInserted; }
#endif
    bool polished() const { return type() == ChildPolished; }
    bool removed() const { return type() == ChildRemoved; }
protected:
    QObject *c;
};

#ifdef QT3_SUPPORT
class Q_CORE_EXPORT QCustomEvent : public QEvent
{
public:
    QT3_SUPPORT_CONSTRUCTOR QCustomEvent(int type, void *data = 0);
    ~QCustomEvent();
    QT3_SUPPORT void *data()  const { return d; }
    QT3_SUPPORT void setData(void* aData) { d = reinterpret_cast<QEventPrivate *>(aData); }
};
#endif

class Q_CORE_EXPORT QDynamicPropertyChangeEvent : public QEvent
{
public:
    QDynamicPropertyChangeEvent(const QByteArray &name);
    ~QDynamicPropertyChangeEvent();

    inline QByteArray propertyName() const { return n; }

private:
    QByteArray n;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCOREEVENT_H

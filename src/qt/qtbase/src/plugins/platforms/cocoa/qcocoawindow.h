/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QCOCOAWINDOW_H
#define QCOCOAWINDOW_H

#include <Cocoa/Cocoa.h>

#include <qpa/qplatformwindow.h>
#include <QRect>

#include "qcocoaglcontext.h"
#include "qnsview.h"

QT_FORWARD_DECLARE_CLASS(QCocoaWindow)

@class QNSWindowHelper;

@protocol QNSWindowProtocol

@property (nonatomic, readonly) QNSWindowHelper *helper;

- (void)superSendEvent:(NSEvent *)theEvent;
- (void)closeAndRelease;

@end

typedef NSWindow<QNSWindowProtocol> QCocoaNSWindow;

@interface QNSWindowHelper : NSObject
{
    QCocoaNSWindow *_window;
    QCocoaWindow *_platformWindow;
    BOOL _grabbingMouse;
    BOOL _releaseOnMouseUp;
}

@property (nonatomic, readonly) QCocoaNSWindow *window;
@property (nonatomic, readonly) QCocoaWindow *platformWindow;
@property (nonatomic) BOOL grabbingMouse;
@property (nonatomic) BOOL releaseOnMouseUp;

- (id)initWithNSWindow:(QCocoaNSWindow *)window platformWindow:(QCocoaWindow *)platformWindow;
- (void)handleWindowEvent:(NSEvent *)theEvent;
- (void) clearWindow;

@end

@interface QNSWindow : NSWindow<QNSWindowProtocol>
{
    QNSWindowHelper *_helper;
}

@property (nonatomic, readonly) QNSWindowHelper *helper;

- (id)initWithContentRect:(NSRect)contentRect
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw;

@end

@interface QNSPanel : NSPanel<QNSWindowProtocol>
{
    QNSWindowHelper *_helper;
}

@property (nonatomic, readonly) QNSWindowHelper *helper;

- (id)initWithContentRect:(NSRect)contentRect
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw;

@end

@class QNSWindowDelegate;

QT_BEGIN_NAMESPACE
// QCocoaWindow
//
// QCocoaWindow is an NSView (not an NSWindow!) in the sense
// that it relies on a NSView for all event handling and
// graphics output and does not require a NSWindow, except for
// for the window-related functions like setWindowTitle.
//
// As a consequence of this it is possible to embed the QCocoaWindow
// in an NSView hierarchy by getting a pointer to the "backing"
// NSView and not calling QCocoaWindow::show():
//
// QWindow *qtWindow = new MyWindow();
// qtWindow->create();
// QPlatformNativeInterface *platformNativeInterface = QGuiApplication::platformNativeInterface();
// NSView *qtView = (NSView *)platformNativeInterface->nativeResourceForWindow("nsview", qtWindow);
// [parentView addSubview:qtView];
//
// See the qt_on_cocoa manual tests for a working example, located
// in tests/manual/cocoa at the time of writing.

class QCocoaMenuBar;

class QCocoaWindow : public QPlatformWindow
{
public:
    QCocoaWindow(QWindow *tlw);
    ~QCocoaWindow();

    void setGeometry(const QRect &rect);
    QRect geometry() const;
    void setCocoaGeometry(const QRect &rect);
    void clipChildWindows();
    void clipWindow(const NSRect &clipRect);
    void show(bool becauseOfAncestor = false);
    void hide(bool becauseOfAncestor = false);
    void setVisible(bool visible);
    void setWindowFlags(Qt::WindowFlags flags);
    void setWindowState(Qt::WindowState state);
    void setWindowTitle(const QString &title);
    void setWindowFilePath(const QString &filePath);
    void setWindowIcon(const QIcon &icon);
    void setAlertState(bool enabled);
    bool isAlertState() const;
    void raise();
    void lower();
    bool isExposed() const;
    bool isOpaque() const;
    void propagateSizeHints();
    void setOpacity(qreal level);
    void setMask(const QRegion &region);
    bool setKeyboardGrabEnabled(bool grab);
    bool setMouseGrabEnabled(bool grab);
    QMargins frameMargins() const;
    QSurfaceFormat format() const;

    void requestActivateWindow();

    WId winId() const;
    void setParent(const QPlatformWindow *window);

    NSView *contentView() const;
    void setContentView(NSView *contentView);
    QNSView *qtView() const;
    NSWindow *nativeWindow() const;

    void setEmbeddedInForeignView(bool subwindow);

    void windowWillMove();
    void windowDidMove();
    void windowDidResize();
    void windowDidEndLiveResize();
    bool windowShouldClose();
    bool windowIsPopupType(Qt::WindowType type = Qt::Widget) const;

    void setSynchedWindowStateFromWindow();

    NSInteger windowLevel(Qt::WindowFlags flags);
    NSUInteger windowStyleMask(Qt::WindowFlags flags);
    void setWindowShadow(Qt::WindowFlags flags);
    void setWindowZoomButton(Qt::WindowFlags flags);

#ifndef QT_NO_OPENGL
    void setCurrentContext(QCocoaGLContext *context);
    QCocoaGLContext *currentContext() const;
#endif

    bool setWindowModified(bool modified) Q_DECL_OVERRIDE;

    void setFrameStrutEventsEnabled(bool enabled);
    bool frameStrutEventsEnabled() const
        { return m_frameStrutEventsEnabled; }

    void setMenubar(QCocoaMenuBar *mb);
    QCocoaMenuBar *menubar() const;

    void setWindowCursor(NSCursor *cursor);

    void registerTouch(bool enable);
    void setContentBorderThickness(int topThickness, int bottomThickness);
    void registerContentBorderArea(quintptr identifier, int upper, int lower);
    void setContentBorderAreaEnabled(quintptr identifier, bool enable);
    void setContentBorderEnabled(bool enable);
    bool testContentBorderAreaPosition(int position) const;
    void applyContentBorderThickness(NSWindow *window);
    void updateNSToolbar();

    qreal devicePixelRatio() const;
    bool isWindowExposable();
    void exposeWindow();
    void obscureWindow();
    void updateExposedGeometry();
    QWindow *childWindowAt(QPoint windowPoint);
protected:
    void recreateWindow(const QPlatformWindow *parentWindow);
    QCocoaNSWindow *createNSWindow();
    void setNSWindow(QCocoaNSWindow *window);

    bool shouldUseNSPanel();

    QRect windowGeometry() const;
    QCocoaWindow *parentCocoaWindow() const;
    void syncWindowState(Qt::WindowState newState);
    void reinsertChildWindow(QCocoaWindow *child);
    void removeChildWindow(QCocoaWindow *child);

// private:
public: // for QNSView
    friend class QCocoaBackingStore;
    friend class QCocoaNativeInterface;

    NSView *m_contentView;
    QNSView *m_qtView;
    QCocoaNSWindow *m_nsWindow;
    QCocoaWindow *m_forwardWindow;

    // TODO merge to one variable if possible
    bool m_contentViewIsEmbedded; // true if the m_contentView is actually embedded in a "foreign" NSView hiearchy
    bool m_contentViewIsToBeEmbedded; // true if the m_contentView is intended to be embedded in a "foreign" NSView hiearchy

    QCocoaWindow *m_parentCocoaWindow;
    bool m_isNSWindowChild; // this window is a non-top level QWindow with a NSWindow.
    QList<QCocoaWindow *> m_childWindows;

    Qt::WindowFlags m_windowFlags;
    bool m_effectivelyMaximized;
    Qt::WindowState m_synchedWindowState;
    Qt::WindowModality m_windowModality;
    QPointer<QWindow> m_activePopupWindow;
    QPointer<QWindow> m_enterLeaveTargetWindow;
    bool m_windowUnderMouse;

    bool m_inConstructor;
#ifndef QT_NO_OPENGL
    QCocoaGLContext *m_glContext;
#endif
    QCocoaMenuBar *m_menubar;
    NSCursor *m_windowCursor;

    bool m_hasModalSession;
    bool m_frameStrutEventsEnabled;
    bool m_geometryUpdateExposeAllowed;
    bool m_isExposed;
    QRect m_exposedGeometry;
    qreal m_exposedDevicePixelRatio;
    int m_registerTouchCount;
    bool m_resizableTransientParent;
    bool m_hiddenByClipping;
    bool m_hiddenByAncestor;

    static const int NoAlertRequest;
    NSInteger m_alertRequest;
    id monitor;

    bool m_drawContentBorderGradient;
    int m_topContentBorderThickness;
    int m_bottomContentBorderThickness;

    // used by showFullScreen in fake mode
    QRect m_normalGeometry;
    Qt::WindowFlags m_oldWindowFlags;
    NSApplicationPresentationOptions m_presentationOptions;

    struct BorderRange {
        BorderRange(quintptr i, int u, int l) : identifier(i), upper(u), lower(l) { }
        quintptr identifier;
        int upper;
        int lower;
        bool operator<(BorderRange const& right) const {
              return upper < right.upper;
        }
    };
    QHash<quintptr, BorderRange> m_contentBorderAreas; // identifer -> uppper/lower
    QHash<quintptr, bool> m_enabledContentBorderAreas; // identifer -> enabled state (true/false)
};

QT_END_NAMESPACE

#endif // QCOCOAWINDOW_H


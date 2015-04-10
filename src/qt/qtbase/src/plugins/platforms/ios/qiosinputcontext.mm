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

#include "qiosinputcontext.h"

#import <UIKit/UIGestureRecognizerSubclass.h>

#include "qiosglobal.h"
#include "qioswindow.h"
#include "quiview.h"
#include <QGuiApplication>
#include <QtGui/private/qwindow_p.h>

@interface QIOSKeyboardListener : UIGestureRecognizer {
@public
    QIOSInputContext *m_context;
    BOOL m_keyboardVisible;
    BOOL m_keyboardVisibleAndDocked;
    BOOL m_ignoreKeyboardChanges;
    BOOL m_touchPressWhileKeyboardVisible;
    BOOL m_keyboardHiddenByGesture;
    QRectF m_keyboardRect;
    QRectF m_keyboardEndRect;
    NSTimeInterval m_duration;
    UIViewAnimationCurve m_curve;
    UIViewController *m_viewController;
}
@end

@implementation QIOSKeyboardListener

- (id)initWithQIOSInputContext:(QIOSInputContext *)context
{
    self = [super initWithTarget:self action:@selector(gestureTriggered)];
    if (self) {
        m_context = context;
        m_keyboardVisible = NO;
        m_keyboardVisibleAndDocked = NO;
        m_ignoreKeyboardChanges = NO;
        m_touchPressWhileKeyboardVisible = NO;
        m_keyboardHiddenByGesture = NO;
        m_duration = 0;
        m_curve = UIViewAnimationCurveEaseOut;
        m_viewController = 0;

        if (isQtApplication()) {
            // Get the root view controller that is on the same screen as the keyboard:
            for (UIWindow *uiWindow in [[UIApplication sharedApplication] windows]) {
                if (uiWindow.screen == [UIScreen mainScreen]) {
                    m_viewController = [uiWindow.rootViewController retain];
                    break;
                }
            }
            Q_ASSERT(m_viewController);

            // Attach 'hide keyboard' gesture to the window, but keep it disabled when the
            // keyboard is not visible. Note that we never trigger the gesture the way it is intended
            // since we don't want to cancel touch events and interrupt flicking etc. Instead we use
            // the gesture framework more as an event filter and hide the keyboard silently.
            self.enabled = NO;
            self.delaysTouchesEnded = NO;
            [m_viewController.view.window addGestureRecognizer:self];
        }

        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(keyboardWillShow:)
            name:@"UIKeyboardWillShowNotification" object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(keyboardWillHide:)
            name:@"UIKeyboardWillHideNotification" object:nil];
        [[NSNotificationCenter defaultCenter]
            addObserver:self
            selector:@selector(keyboardDidChangeFrame:)
            name:@"UIKeyboardDidChangeFrameNotification" object:nil];
    }
    return self;
}

- (void) dealloc
{
    [m_viewController.view.window removeGestureRecognizer:self];
    [m_viewController release];

    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIKeyboardWillShowNotification" object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIKeyboardWillHideNotification" object:nil];
    [[NSNotificationCenter defaultCenter]
        removeObserver:self
        name:@"UIKeyboardDidChangeFrameNotification" object:nil];
    [super dealloc];
}

- (QRectF) getKeyboardRect:(NSNotification *)notification
{
    // For Qt applications we rotate the keyboard rect to align with the screen
    // orientation (which is the interface orientation of the root view controller).
    // For hybrid apps we follow native behavior, and return the rect unmodified:
    CGRect keyboardFrame = [[[notification userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    if (isQtApplication()) {
        UIView *view = m_viewController.view;
        return fromCGRect(CGRectOffset([view convertRect:keyboardFrame fromView:view.window], 0, -view.bounds.origin.y));
    } else {
        return fromCGRect(keyboardFrame);
    }
}

- (void) keyboardDidChangeFrame:(NSNotification *)notification
{
    Q_UNUSED(notification);
    [self handleKeyboardRectChanged];

    // If the keyboard was visible and docked from before, this is just a geometry
    // change (normally caused by an orientation change). In that case, update scroll:
    if (m_keyboardVisibleAndDocked)
        m_context->scrollToCursor();
}

- (void) keyboardWillShow:(NSNotification *)notification
{
    if (m_ignoreKeyboardChanges)
        return;
    // Note that UIKeyboardWillShowNotification is only sendt when the keyboard is docked.
    m_keyboardVisibleAndDocked = YES;
    m_keyboardEndRect = [self getKeyboardRect:notification];
    self.enabled = YES;
    if (!m_duration) {
        m_duration = [[notification.userInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
        m_curve = UIViewAnimationCurve([[notification.userInfo objectForKey:UIKeyboardAnimationCurveUserInfoKey] integerValue] << 16);
    }
    m_context->scrollToCursor();
}

- (void) keyboardWillHide:(NSNotification *)notification
{
    if (m_ignoreKeyboardChanges)
        return;
    // Note that UIKeyboardWillHideNotification is also sendt when the keyboard is undocked.
    m_keyboardVisibleAndDocked = NO;
    m_keyboardEndRect = [self getKeyboardRect:notification];
    if (!m_keyboardHiddenByGesture) {
        // Only disable the gesture if the hiding of the keyboard was not caused by it.
        // Otherwise we need to await the final touchEnd callback for doing some clean-up.
        self.enabled = NO;
    }
    m_context->scroll(0);
}

- (void) handleKeyboardRectChanged
{
    QRectF rect = m_keyboardEndRect;
    rect.moveTop(rect.y() + m_viewController.view.bounds.origin.y);
    if (m_keyboardRect != rect) {
        m_keyboardRect = rect;
        m_context->emitKeyboardRectChanged();
    }

    BOOL visible = m_keyboardEndRect.intersects(fromCGRect([UIScreen mainScreen].bounds));
    if (m_keyboardVisible != visible) {
        m_keyboardVisible = visible;
        m_context->emitInputPanelVisibleChanged();
    }
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    QPointF p = fromCGPoint([[touches anyObject] locationInView:m_viewController.view]);
    if (m_keyboardRect.contains(p)) {
        m_keyboardHiddenByGesture = YES;
        m_context->hideInputPanel();
    }

    [super touchesMoved:touches withEvent:event];
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    Q_ASSERT(m_keyboardVisibleAndDocked);
    m_touchPressWhileKeyboardVisible = YES;
    [super touchesBegan:touches withEvent:event];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    m_touchPressWhileKeyboardVisible = NO;
    [self performSelectorOnMainThread:@selector(touchesEndedPostDelivery) withObject:nil waitUntilDone:NO];
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesEndedPostDelivery
{
    // Do some clean-up _after_ touchEnd has been delivered to QUIView
    m_keyboardHiddenByGesture = NO;
    if (!m_keyboardVisibleAndDocked) {
        self.enabled = NO;
        if (qApp->focusObject()) {
            // UI Controls are told to gain focus on touch release. So when the 'hide keyboard' gesture
            // finishes, the final touch end can trigger a control to gain focus. This is in conflict with
            // the gesture, so we clear focus once more as a work-around.
            static_cast<QWindowPrivate *>(QObjectPrivate::get(qApp->focusWindow()))->clearFocusObject();
        }
    } else {
        m_context->scrollToCursor();
    }
}

@end

QIOSInputContext::QIOSInputContext()
    : QPlatformInputContext()
    , m_keyboardListener([[QIOSKeyboardListener alloc] initWithQIOSInputContext:this])
    , m_focusView(0)
    , m_hasPendingHideRequest(false)
{
    if (isQtApplication())
        connect(qGuiApp->inputMethod(), &QInputMethod::cursorRectangleChanged, this, &QIOSInputContext::cursorRectangleChanged);
    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &QIOSInputContext::focusWindowChanged);
}

QIOSInputContext::~QIOSInputContext()
{
    [m_keyboardListener release];
    [m_focusView release];
}

QRectF QIOSInputContext::keyboardRect() const
{
    return m_keyboardListener->m_keyboardRect;
}

void QIOSInputContext::showInputPanel()
{
    if (m_keyboardListener->m_keyboardHiddenByGesture) {
        // We refuse to re-show the keyboard until the touch
        // sequence that triggered the gesture has ended.
        return;
    }

    // Documentation tells that one should call (and recall, if necessary) becomeFirstResponder/resignFirstResponder
    // to show/hide the keyboard. This is slightly inconvenient, since there exist no API to get the current first
    // responder. Rather than searching for it from the top, we let the active QIOSWindow tell us which view to use.
    // Note that Qt will forward keyevents to whichever QObject that needs it, regardless of which UIView the input
    // actually came from. So in this respect, we're undermining iOS' responder chain.
    m_hasPendingHideRequest = false;
    [m_focusView becomeFirstResponder];
}

void QIOSInputContext::hideInputPanel()
{
    // Delay hiding the keyboard for cases where the user is transferring focus between
    // 'line edits'. In that case the 'line edit' that lost focus will close the input
    // panel, just to see that the new 'line edit' will open it again:
    m_hasPendingHideRequest = true;
    dispatch_async(dispatch_get_main_queue(), ^{
        if (m_hasPendingHideRequest)
            [m_focusView resignFirstResponder];
    });
}

bool QIOSInputContext::isInputPanelVisible() const
{
    return m_keyboardListener->m_keyboardVisible;
}

void QIOSInputContext::setFocusObject(QObject *focusObject)
{
    if (!focusObject || !m_focusView || !m_focusView.isFirstResponder) {
        scroll(0);
        return;
    }

    reset();

    if (m_keyboardListener->m_keyboardVisibleAndDocked)
        scrollToCursor();
}

void QIOSInputContext::focusWindowChanged(QWindow *focusWindow)
{
    QUIView *view = focusWindow ? reinterpret_cast<QUIView *>(focusWindow->handle()->winId()) : 0;
    if ([m_focusView isFirstResponder])
        [view becomeFirstResponder];
    [m_focusView release];
    m_focusView = [view retain];

    if (view.window != m_keyboardListener->m_viewController.view)
        scroll(0);
}

void QIOSInputContext::cursorRectangleChanged()
{
    if (!m_keyboardListener->m_keyboardVisibleAndDocked)
        return;

    // Check if the cursor has changed position inside the input item. Since
    // qApp->inputMethod()->cursorRectangle() will also change when the input item
    // itself moves, we need to ask the focus object for ImCursorRectangle:
    static QPoint prevCursor;
    QInputMethodQueryEvent queryEvent(Qt::ImCursorRectangle);
    QCoreApplication::sendEvent(qApp->focusObject(), &queryEvent);
    QPoint cursor = queryEvent.value(Qt::ImCursorRectangle).toRect().topLeft();
    if (cursor != prevCursor)
        scrollToCursor();
    prevCursor = cursor;
}

void QIOSInputContext::scrollToCursor()
{
    if (!isQtApplication() || !m_focusView)
        return;

    if (m_keyboardListener->m_touchPressWhileKeyboardVisible) {
        // Don't scroll to the cursor if the user is touching the screen. This
        // interferes with selection and the 'hide keyboard' gesture. Instead
        // we update scrolling upon touchEnd.
        return;
    }

    UIView *view = m_keyboardListener->m_viewController.view;
    if (view.window != m_focusView.window)
        return;

    const int margin = 20;
    QRectF translatedCursorPos = qApp->inputMethod()->cursorRectangle();
    translatedCursorPos.translate(m_focusView.qwindow->geometry().topLeft());
    qreal keyboardY = m_keyboardListener->m_keyboardEndRect.y();
    int statusBarY = qGuiApp->primaryScreen()->availableGeometry().y();

    scroll((translatedCursorPos.bottomLeft().y() < keyboardY - margin) ? 0
        : qMin(view.bounds.size.height - keyboardY, translatedCursorPos.y() - statusBarY - margin));
}

void QIOSInputContext::scroll(int y)
{
    // Scroll the view the same way a UIScrollView
    // works: by changing bounds.origin:
    UIView *view = m_keyboardListener->m_viewController.view;
    if (y == view.bounds.origin.y)
        return;

    CGRect newBounds = view.bounds;
    newBounds.origin.y = y;
    QPointer<QIOSInputContext> self = this;
    [UIView animateWithDuration:m_keyboardListener->m_duration delay:0
        options:m_keyboardListener->m_curve | UIViewAnimationOptionBeginFromCurrentState
        animations:^{ view.bounds = newBounds; }
        completion:^(BOOL){
            if (self)
                [m_keyboardListener handleKeyboardRectChanged];
        }
    ];
}

void QIOSInputContext::update(Qt::InputMethodQueries query)
{
    [m_focusView updateInputMethodWithQuery:query];
}

void QIOSInputContext::reset()
{
    // Since the call to reset will cause a 'keyboardWillHide'
    // notification to be sendt, we block keyboard nofifications to avoid artifacts:
    m_keyboardListener->m_ignoreKeyboardChanges = true;
    [m_focusView reset];
    m_keyboardListener->m_ignoreKeyboardChanges = false;
}

void QIOSInputContext::commit()
{
    [m_focusView commit];
}


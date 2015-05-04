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
#include "qiosintegration.h"
#include "qiosscreen.h"
#include "qiostextresponder.h"
#include "qiosviewcontroller.h"
#include "qioswindow.h"
#include "quiview.h"

#include <QGuiApplication>
#include <QtGui/private/qwindow_p.h>

// -------------------------------------------------------------------------

static QUIView *focusView()
{
    return qApp->focusWindow() ?
        reinterpret_cast<QUIView *>(qApp->focusWindow()->winId()) : 0;
}

// -------------------------------------------------------------------------

@interface QIOSKeyboardListener : UIGestureRecognizer <UIGestureRecognizerDelegate> {
  @private
    QIOSInputContext *m_context;
}
@property BOOL hasDeferredScrollToCursor;
@end

@implementation QIOSKeyboardListener

- (id)initWithQIOSInputContext:(QIOSInputContext *)context
{
    if (self = [super initWithTarget:self action:@selector(gestureStateChanged:)]) {

        m_context = context;

        self.hasDeferredScrollToCursor = NO;

        // UIGestureRecognizer
        self.enabled = NO;
        self.cancelsTouchesInView = NO;
        self.delaysTouchesEnded = NO;

        NSNotificationCenter *notificationCenter = [NSNotificationCenter defaultCenter];

        [notificationCenter addObserver:self
            selector:@selector(keyboardWillShow:)
            name:UIKeyboardWillShowNotification object:nil];
        [notificationCenter addObserver:self
            selector:@selector(keyboardWillOrDidChange:)
            name:UIKeyboardDidShowNotification object:nil];
        [notificationCenter addObserver:self
            selector:@selector(keyboardWillHide:)
            name:UIKeyboardWillHideNotification object:nil];
        [notificationCenter addObserver:self
            selector:@selector(keyboardWillOrDidChange:)
            name:UIKeyboardDidHideNotification object:nil];
        [notificationCenter addObserver:self
            selector:@selector(keyboardDidChangeFrame:)
            name:UIKeyboardDidChangeFrameNotification object:nil];
    }

    return self;
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}

// -------------------------------------------------------------------------

- (void)keyboardWillShow:(NSNotification *)notification
{
    [self keyboardWillOrDidChange:notification];

    UIResponder *firstResponder = [UIResponder currentFirstResponder];
    if (![firstResponder isKindOfClass:[QIOSTextInputResponder class]])
        return;

    // Enable hide-keyboard gesture
    self.enabled = YES;

    m_context->scrollToCursor();
}

- (void)keyboardWillHide:(NSNotification *)notification
{
    [self keyboardWillOrDidChange:notification];

    if (self.state != UIGestureRecognizerStateBegan) {
        // Only disable the gesture if the hiding of the keyboard was not caused by it.
        // Otherwise we need to await the final touchEnd callback for doing some clean-up.
        self.enabled = NO;
    }
    m_context->scroll(0);
}

- (void)keyboardDidChangeFrame:(NSNotification *)notification
{
    [self keyboardWillOrDidChange:notification];

    // If the keyboard was visible and docked from before, this is just a geometry
    // change (normally caused by an orientation change). In that case, update scroll:
    if (m_context->isInputPanelVisible())
        m_context->scrollToCursor();
}

- (void)keyboardWillOrDidChange:(NSNotification *)notification
{
    m_context->updateKeyboardState(notification);
}

// -------------------------------------------------------------------------

- (BOOL)canPreventGestureRecognizer:(UIGestureRecognizer *)other
{
    Q_UNUSED(other);
    return NO;
}

- (BOOL)canBePreventedByGestureRecognizer:(UIGestureRecognizer *)other
{
    Q_UNUSED(other);
    return NO;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesBegan:touches withEvent:event];

    Q_ASSERT(m_context->isInputPanelVisible());

    if ([touches count] != 1)
        self.state = UIGestureRecognizerStateFailed;
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesMoved:touches withEvent:event];

    if (self.state != UIGestureRecognizerStatePossible)
        return;

    CGPoint touchPoint = [[touches anyObject] locationInView:self.view];
    if (CGRectContainsPoint(m_context->keyboardState().keyboardEndRect, touchPoint))
        self.state = UIGestureRecognizerStateBegan;
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];

    [self touchesEndedOrCancelled];
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];

    [self touchesEndedOrCancelled];
}

- (void)touchesEndedOrCancelled
{
    // Defer final state change until next runloop iteration, so that Qt
    // has a chance to process the final touch events first, before we eg.
    // scroll the view.
    dispatch_async(dispatch_get_main_queue (), ^{
        // iOS will transition from began to changed by itself
        Q_ASSERT(self.state != UIGestureRecognizerStateBegan);

        if (self.state == UIGestureRecognizerStateChanged)
            self.state = UIGestureRecognizerStateEnded;
        else
            self.state = UIGestureRecognizerStateFailed;
    });
}

- (void)gestureStateChanged:(id)sender
{
    Q_UNUSED(sender);

    if (self.state == UIGestureRecognizerStateBegan) {
        qImDebug() << "hide keyboard gesture was triggered";
        UIResponder *firstResponder = [UIResponder currentFirstResponder];
        Q_ASSERT([firstResponder isKindOfClass:[QIOSTextInputResponder class]]);
        [firstResponder resignFirstResponder];
    }
}

- (void)reset
{
    [super reset];

    if (!m_context->isInputPanelVisible()) {
        qImDebug() << "keyboard was hidden, disabling hide-keyboard gesture";
        self.enabled = NO;
    } else {
        qImDebug() << "gesture completed without triggering";
        if (self.hasDeferredScrollToCursor) {
            qImDebug() << "applying deferred scroll to cursor";
            m_context->scrollToCursor();
        }
    }

    self.hasDeferredScrollToCursor = NO;
}

@end

// -------------------------------------------------------------------------

Qt::InputMethodQueries ImeState::update(Qt::InputMethodQueries properties)
{
    if (!properties)
        return 0;

    QInputMethodQueryEvent newState(properties);

    // Update the focus object that the new state is based on
    focusObject = qApp ? qApp->focusObject() : 0;

    if (focusObject)
        QCoreApplication::sendEvent(focusObject, &newState);

    Qt::InputMethodQueries updatedProperties;
    for (uint i = 0; i < (sizeof(Qt::ImQueryAll) * CHAR_BIT); ++i) {
        if (Qt::InputMethodQuery property = Qt::InputMethodQuery(int(properties & (1 << i)))) {
            if (newState.value(property) != currentState.value(property)) {
                updatedProperties |= property;
                currentState.setValue(property, newState.value(property));
            }
        }
    }

    return updatedProperties;
}

// -------------------------------------------------------------------------

QIOSInputContext *QIOSInputContext::instance()
{
    return static_cast<QIOSInputContext *>(QIOSIntegration::instance()->inputContext());
}

QIOSInputContext::QIOSInputContext()
    : QPlatformInputContext()
    , m_keyboardHideGesture([[QIOSKeyboardListener alloc] initWithQIOSInputContext:this])
    , m_textResponder(0)
{
    if (isQtApplication()) {
        QIOSScreen *iosScreen = static_cast<QIOSScreen*>(QGuiApplication::primaryScreen()->handle());
        [iosScreen->uiWindow() addGestureRecognizer:m_keyboardHideGesture];
    }

    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, &QIOSInputContext::focusWindowChanged);
}

QIOSInputContext::~QIOSInputContext()
{
    [m_keyboardHideGesture.view removeGestureRecognizer:m_keyboardHideGesture];
    [m_keyboardHideGesture release];

    [m_textResponder release];
}

void QIOSInputContext::showInputPanel()
{
    // No-op, keyboard controlled fully by platform based on focus
    qImDebug() << "can't show virtual keyboard without a focus object, ignoring";
}

void QIOSInputContext::hideInputPanel()
{
    if (![m_textResponder isFirstResponder]) {
        qImDebug() << "QIOSTextInputResponder is not first responder, ignoring";
        return;
    }

    if (qGuiApp->focusObject() != m_imeState.focusObject) {
        qImDebug() << "current focus object does not match IM state, likely hiding from focusOut event, so ignoring";
        return;
    }

    qImDebug() << "hiding VKB as requested by QInputMethod::hide()";
    [m_textResponder resignFirstResponder];
}

void QIOSInputContext::clearCurrentFocusObject()
{
    if (QWindow *focusWindow = qApp->focusWindow())
        static_cast<QWindowPrivate *>(QObjectPrivate::get(focusWindow))->clearFocusObject();
}

// -------------------------------------------------------------------------

void QIOSInputContext::updateKeyboardState(NSNotification *notification)
{
    static CGRect currentKeyboardRect = CGRectZero;

    KeyboardState previousState = m_keyboardState;

    if (notification) {
        NSDictionary *userInfo = [notification userInfo];

        CGRect frameBegin = [[userInfo objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
        CGRect frameEnd = [[userInfo objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];

        bool atEndOfKeyboardTransition = [notification.name rangeOfString:@"Did"].location != NSNotFound;

        currentKeyboardRect = atEndOfKeyboardTransition ? frameEnd : frameBegin;

        // The isInputPanelVisible() property is based on whether or not the virtual keyboard
        // is visible on screen, and does not follow the logic of the iOS WillShow and WillHide
        // notifications which are not emitted for undocked keyboards, and are buggy when dealing
        // with input-accesosory-views. The reason for using frameEnd here (the future state),
        // instead of the current state reflected in frameBegin, is that QInputMethod::isVisible()
        // is documented to reflect the future state in the case of animated transitions.
        m_keyboardState.keyboardVisible = CGRectIntersectsRect(frameEnd, [UIScreen mainScreen].bounds);

        // Used for auto-scroller, and will be used for animation-signal in the future
        m_keyboardState.keyboardEndRect = frameEnd;

        if (m_keyboardState.animationCurve < 0) {
            // We only set the animation curve the first time it has a valid value, since iOS will sometimes report
            // an invalid animation curve even if the keyboard is animating, and we don't want to overwrite the
            // curve in that case.
            m_keyboardState.animationCurve = UIViewAnimationCurve([[userInfo objectForKey:UIKeyboardAnimationCurveUserInfoKey] integerValue]);
        }

        m_keyboardState.animationDuration = [[userInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
        m_keyboardState.keyboardAnimating = m_keyboardState.animationDuration > 0 && !atEndOfKeyboardTransition;

        qImDebug() << qPrintable(QString::fromNSString(notification.name)) << "from" << fromCGRect(frameBegin) << "to" << fromCGRect(frameEnd)
                   << "(curve =" << m_keyboardState.animationCurve << "duration =" << m_keyboardState.animationDuration << "s)";
    } else {
        qImDebug() << "No notification to update keyboard state based on, just updating keyboard rect";
    }

    if (!focusView() || CGRectIsEmpty(currentKeyboardRect))
        m_keyboardState.keyboardRect = QRectF();
    else // QInputmethod::keyboardRectangle() is documented to be in window coordinates.
        m_keyboardState.keyboardRect = fromCGRect([focusView() convertRect:currentKeyboardRect fromView:nil]);

    // Emit for all changed properties
    if (m_keyboardState.keyboardVisible != previousState.keyboardVisible)
        emitInputPanelVisibleChanged();
    if (m_keyboardState.keyboardAnimating != previousState.keyboardAnimating)
        emitAnimatingChanged();
    if (m_keyboardState.keyboardRect != previousState.keyboardRect)
        emitKeyboardRectChanged();
}

bool QIOSInputContext::isInputPanelVisible() const
{
    return m_keyboardState.keyboardVisible;
}

bool QIOSInputContext::isAnimating() const
{
    return m_keyboardState.keyboardAnimating;
}

QRectF QIOSInputContext::keyboardRect() const
{
    return m_keyboardState.keyboardRect;
}

// -------------------------------------------------------------------------

UIView *QIOSInputContext::scrollableRootView()
{
    if (!m_keyboardHideGesture.view)
        return 0;

    UIWindow *window = static_cast<UIWindow*>(m_keyboardHideGesture.view);
    if (![window.rootViewController isKindOfClass:[QIOSViewController class]])
        return 0;

    return window.rootViewController.view;
}

void QIOSInputContext::scrollToCursor()
{
    if (!isQtApplication())
        return;

    if (m_keyboardHideGesture.state == UIGestureRecognizerStatePossible && m_keyboardHideGesture.numberOfTouches == 1) {
        // Don't scroll to the cursor if the user is touching the screen and possibly
        // trying to trigger the hide-keyboard gesture.
        qImDebug() << "deferring scrolling to cursor as we're still waiting for a possible gesture";
        m_keyboardHideGesture.hasDeferredScrollToCursor = YES;
        return;
    }

    UIView *rootView = scrollableRootView();
    if (!rootView)
        return;

    if (rootView.window != focusView().window)
        return;

    // We only support auto-scroll for docked keyboards for now, so make sure that's the case
    if (CGRectGetMaxY(m_keyboardState.keyboardEndRect) != CGRectGetMaxY([UIScreen mainScreen].bounds)) {
        qImDebug() << "Keyboard not docked, ignoring request to scroll to reveal cursor";
        return;
    }

    const int margin = 20;
    QRectF translatedCursorPos = qApp->inputMethod()->cursorRectangle();
    translatedCursorPos.translate(focusView().qwindow->geometry().topLeft());

    qreal keyboardY = [rootView convertRect:m_keyboardState.keyboardEndRect fromView:nil].origin.y;
    int statusBarY = qGuiApp->primaryScreen()->availableGeometry().y();

    scroll((translatedCursorPos.bottomLeft().y() < keyboardY - margin) ? 0
        : qMin(rootView.bounds.size.height - keyboardY, translatedCursorPos.y() - statusBarY - margin));
}

void QIOSInputContext::scroll(int y)
{
    UIView *rootView = scrollableRootView();
    if (!rootView)
        return;

    CATransform3D translationTransform = CATransform3DMakeTranslation(0.0, -y, 0.0);
    if (CATransform3DEqualToTransform(translationTransform, rootView.layer.sublayerTransform))
        return;

    QPointer<QIOSInputContext> self = this;
    [UIView animateWithDuration:m_keyboardState.animationDuration delay:0
        options:(m_keyboardState.animationCurve << 16) | UIViewAnimationOptionBeginFromCurrentState
        animations:^{
            // The sublayerTransform property of CALayer is not implicitly animated for a
            // layer-backed view, even inside a UIView animation block, so we need to set up
            // an explicit CoreAnimation animation. Since there is no predefined media timing
            // function that matches the custom keyboard animation curve we cheat by asking
            // the view for an animation of another property, which will give us an animation
            // that matches the parameters we passed to [UIView animateWithDuration] above.
            // The reason we ask for the animation of 'backgroundColor' is that it's a simple
            // property that will not return a compound animation, like eg. bounds will.
            NSObject *action = (NSObject*)[rootView actionForLayer:rootView.layer forKey:@"backgroundColor"];

            CABasicAnimation *animation;
            if ([action isKindOfClass:[CABasicAnimation class]]) {
                animation = static_cast<CABasicAnimation*>(action);
                animation.keyPath = @"sublayerTransform"; // Instead of backgroundColor
            } else {
                animation = [CABasicAnimation animationWithKeyPath:@"sublayerTransform"];
            }

            CATransform3D currentSublayerTransform = static_cast<CALayer *>([rootView.layer presentationLayer]).sublayerTransform;
            animation.fromValue = [NSValue valueWithCATransform3D:currentSublayerTransform];
            animation.toValue = [NSValue valueWithCATransform3D:translationTransform];
            [rootView.layer addAnimation:animation forKey:@"AnimateSubLayerTransform"];
            rootView.layer.sublayerTransform = translationTransform;

            bool keyboardScrollIsActive = y != 0;

            // Raise all known windows to above the status-bar if we're scrolling the screen,
            // while keeping the relative window level between the windows the same.
            NSArray *applicationWindows = [[UIApplication sharedApplication] windows];
            static QHash<UIWindow *, UIWindowLevel> originalWindowLevels;
            for (UIWindow *window in applicationWindows) {
                if (keyboardScrollIsActive && !originalWindowLevels.contains(window))
                    originalWindowLevels.insert(window, window.windowLevel);

                UIWindowLevel windowLevelAdjustment = keyboardScrollIsActive ? UIWindowLevelStatusBar : 0;
                window.windowLevel = originalWindowLevels.value(window) + windowLevelAdjustment;

                if (!keyboardScrollIsActive)
                    originalWindowLevels.remove(window);
            }
        }
        completion:^(BOOL){
            if (self) {
                // Scrolling the root view results in the keyboard being moved
                // relative to the focus window, so we need to re-evaluate the
                // keyboard rectangle.
                updateKeyboardState();
            }
        }
    ];
}

// -------------------------------------------------------------------------

void QIOSInputContext::setFocusObject(QObject *focusObject)
{
    Q_UNUSED(focusObject);

    qImDebug() << "new focus object =" << focusObject;

    if (QPlatformInputContext::inputMethodAccepted()
            && m_keyboardHideGesture.state == UIGestureRecognizerStateChanged) {
        // A new focus object may be set as part of delivering touch events to
        // application during the hide-keyboard gesture, but we don't want that
        // to result in a new object getting focus and bringing the keyboard up
        // again.
        qImDebug() << "clearing focus object" << focusObject << "as hide-keyboard gesture is active";
        clearCurrentFocusObject();
        return;
    }

    reset();

    if (isInputPanelVisible())
        scrollToCursor();
}

void QIOSInputContext::focusWindowChanged(QWindow *focusWindow)
{
    Q_UNUSED(focusWindow);

    qImDebug() << "new focus window =" << focusWindow;

    reset();

    // The keyboard rectangle depend on the focus window, so
    // we need to re-evaluate the keyboard state.
    updateKeyboardState();

    if (isInputPanelVisible())
        scrollToCursor();
}

/*!
    Called by the input item to inform the platform input methods when there has been
    state changes in editor's input method query attributes. When calling the function
    \a queries parameter has to be used to tell what has changes, which input method
    can use to make queries for attributes it's interested with QInputMethodQueryEvent.
*/
void QIOSInputContext::update(Qt::InputMethodQueries updatedProperties)
{
    // Mask for properties that we are interested in and see if any of them changed
    updatedProperties &= (Qt::ImEnabled | Qt::ImHints | Qt::ImQueryInput | Qt::ImPlatformData);

    if (updatedProperties & Qt::ImEnabled) {
        // Switching on and off input-methods needs a re-fresh of hints and platform
        // data when we turn them on again, as the IM state we have may have been
        // invalidated when IM was switched off. We could defer this until we know
        // if IM was turned on, to limit the extra query parameters, but for simplicity
        // we always do the update.
        updatedProperties |= (Qt::ImHints | Qt::ImPlatformData);
    }

    qImDebug() << "fw =" << qApp->focusWindow() << "fo =" << qApp->focusObject();

    Qt::InputMethodQueries changedProperties = m_imeState.update(updatedProperties);
    if (changedProperties & (Qt::ImEnabled | Qt::ImHints | Qt::ImPlatformData)) {
        // Changes to enablement or hints require virtual keyboard reconfigure

        qImDebug() << "changed IM properties" << changedProperties << "require keyboard reconfigure";

        if (inputMethodAccepted()) {
            qImDebug() << "replacing text responder with new text responder";
            [m_textResponder autorelease];
            m_textResponder = [[QIOSTextInputResponder alloc] initWithInputContext:this];
            [m_textResponder becomeFirstResponder];
        } else if ([UIResponder currentFirstResponder] == m_textResponder) {
            qImDebug() << "IM not enabled, resigning text responder as first responder";
            [m_textResponder resignFirstResponder];
        } else {
            qImDebug() << "IM not enabled. Text responder not first responder. Nothing to do";
        }
    } else {
        [m_textResponder notifyInputDelegate:changedProperties];
    }

    if (changedProperties & Qt::ImCursorRectangle)
        scrollToCursor();
}

bool QIOSInputContext::inputMethodAccepted() const
{
    // The IM enablement state is based on the last call to update()
    bool lastKnownImEnablementState = m_imeState.currentState.value(Qt::ImEnabled).toBool();

#if !defined(QT_NO_DEBUG)
    // QPlatformInputContext keeps a cached value of the current IM enablement state that is
    // updated by QGuiApplication when the current focus object changes, or by QInputMethod's
    // update() function. If the focus object changes, but the change is not propagated as
    // a signal to QGuiApplication due to bugs in the widget/graphicsview/qml stack, we'll
    // end up with a stale value for QPlatformInputContext::inputMethodAccepted(). To be on
    // the safe side we always use our own cached value to decide if IM is enabled, and try
    // to detect the case where the two values are out of sync.
    if (lastKnownImEnablementState != QPlatformInputContext::inputMethodAccepted())
        qWarning("QPlatformInputContext::inputMethodAccepted() does not match actual focus object IM enablement!");
#endif

    return lastKnownImEnablementState;
}

/*!
    Called by the input item to reset the input method state.
*/
void QIOSInputContext::reset()
{
    update(Qt::ImQueryAll);

    [m_textResponder setMarkedText:@"" selectedRange:NSMakeRange(0, 0)];
    [m_textResponder notifyInputDelegate:Qt::ImQueryInput];
}

/*!
    Commits the word user is currently composing to the editor. The function is
    mostly needed by the input methods with text prediction features and by the
    methods where the script used for typing characters is different from the
    script that actually gets appended to the editor. Any kind of action that
    interrupts the text composing needs to flush the composing state by calling the
    commit() function, for example when the cursor is moved elsewhere.
*/
void QIOSInputContext::commit()
{
    [m_textResponder unmarkText];
    [m_textResponder notifyInputDelegate:Qt::ImSurroundingText];
}

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

#include "qcolordialog_p.h"
#if !defined(QT_NO_COLORDIALOG) && defined(Q_WS_MAC)
#include <qapplication.h>
#include <qtimer.h>
#include <qdialogbuttonbox.h>
#include <qabstracteventdispatcher.h>
#include <private/qapplication_p.h>
#include <private/qt_mac_p.h>
#include <qdebug.h>
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#if !CGFLOAT_DEFINED
typedef float CGFloat;  // Should only not be defined on 32-bit platforms
#endif


#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_5
@protocol NSWindowDelegate <NSObject>
- (void)windowDidResize:(NSNotification *)notification;
- (BOOL)windowShouldClose:(id)window;
@end
#endif

QT_USE_NAMESPACE

@class QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate);

@interface QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) : NSObject<NSWindowDelegate> {
    NSColorPanel *mColorPanel;
    NSView *mStolenContentView;
    NSButton *mOkButton;
    NSButton *mCancelButton;
    QColorDialogPrivate *mPriv;
    QColor *mQtColor;
    CGFloat mMinWidth;  // currently unused
    CGFloat mExtraHeight;   // currently unused
    BOOL mHackedPanel;
    NSInteger mResultCode;
    BOOL mDialogIsExecuting;
    BOOL mResultSet;
}
- (id)initWithColorPanel:(NSColorPanel *)panel
       stolenContentView:(NSView *)stolenContentView
                okButton:(NSButton *)okButton
            cancelButton:(NSButton *)cancelButton
                    priv:(QColorDialogPrivate *)priv;
- (void)colorChanged:(NSNotification *)notification;
- (void)relayout;
- (void)onOkClicked;
- (void)onCancelClicked;
- (void)updateQtColor;
- (NSColorPanel *)colorPanel;
- (QColor)qtColor;
- (void)finishOffWithCode:(NSInteger)result;
- (void)showColorPanel;
- (void)exec;
- (void)setResultSet:(BOOL)result;
@end

@implementation QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate)
- (id)initWithColorPanel:(NSColorPanel *)panel
       stolenContentView:(NSView *)stolenContentView
                okButton:(NSButton *)okButton
            cancelButton:(NSButton *)cancelButton
                    priv:(QColorDialogPrivate *)priv
{
    self = [super init];

    mColorPanel = panel;
    mStolenContentView = stolenContentView;
    mOkButton = okButton;
    mCancelButton = cancelButton;
    mPriv = priv;
    mMinWidth = 0.0;
    mExtraHeight = 0.0;
    mHackedPanel = (okButton != 0);
    mResultCode = NSCancelButton;
    mDialogIsExecuting = false;
    mResultSet = false;

    if (mHackedPanel) {
        [self relayout];

        [okButton setAction:@selector(onOkClicked)];
        [okButton setTarget:self];

        [cancelButton setAction:@selector(onCancelClicked)];
        [cancelButton setTarget:self];
    }

    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(colorChanged:)
        name:NSColorPanelColorDidChangeNotification
        object:mColorPanel];

    mQtColor = new QColor();
    return self;
}

- (void)dealloc
{
    QMacCocoaAutoReleasePool pool;
    if (mHackedPanel) {
        NSView *ourContentView = [mColorPanel contentView];

        // return stolen stuff to its rightful owner
        [mStolenContentView removeFromSuperview];
        [mColorPanel setContentView:mStolenContentView];

        [mOkButton release];
        [mCancelButton release];
        [ourContentView release];
    }
    [mColorPanel setDelegate:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    delete mQtColor;
    [super dealloc];
}

- (void)setResultSet:(BOOL)result
{
    mResultSet = result;
}

- (BOOL)windowShouldClose:(id)window
{
    Q_UNUSED(window);
    if (!mHackedPanel)
        [self updateQtColor];
    if (mDialogIsExecuting) {
        [self finishOffWithCode:NSCancelButton];
    } else {
        mResultSet = true;
        mPriv->colorDialog()->reject();
    }
    return true;
}

- (void)windowDidResize:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (mHackedPanel)
        [self relayout];
}

- (void)colorChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);
    [self updateQtColor];
}

- (void)relayout
{
    Q_ASSERT(mHackedPanel);

    NSRect rect = [[mStolenContentView superview] frame];

    // should a priori be kept in sync with qfontdialog_mac.mm
    const CGFloat ButtonMinWidth = 78.0; // 84.0 for Carbon
    const CGFloat ButtonMinHeight = 32.0;
    const CGFloat ButtonSpacing = 0.0;
    const CGFloat ButtonTopMargin = 0.0;
    const CGFloat ButtonBottomMargin = 7.0;
    const CGFloat ButtonSideMargin = 9.0;

    [mOkButton sizeToFit];
    NSSize okSizeHint = [mOkButton frame].size;

    [mCancelButton sizeToFit];
    NSSize cancelSizeHint = [mCancelButton frame].size;

    const CGFloat ButtonWidth = qMin(qMax(ButtonMinWidth,
                                          qMax(okSizeHint.width, cancelSizeHint.width)),
                                     CGFloat((rect.size.width - 2.0 * ButtonSideMargin - ButtonSpacing) * 0.5));
    const CGFloat ButtonHeight = qMax(ButtonMinHeight,
                                     qMax(okSizeHint.height, cancelSizeHint.height));

    NSRect okRect = { { rect.size.width - ButtonSideMargin - ButtonWidth,
                        ButtonBottomMargin },
                      { ButtonWidth, ButtonHeight } };
    [mOkButton setFrame:okRect];
    [mOkButton setNeedsDisplay:YES];

    NSRect cancelRect = { { okRect.origin.x - ButtonSpacing - ButtonWidth,
                            ButtonBottomMargin },
                            { ButtonWidth, ButtonHeight } };
    [mCancelButton setFrame:cancelRect];
    [mCancelButton setNeedsDisplay:YES];

    const CGFloat Y = ButtonBottomMargin + ButtonHeight + ButtonTopMargin;
    NSRect stolenCVRect = { { 0.0, Y },
                            { rect.size.width, rect.size.height - Y } };
    [mStolenContentView setFrame:stolenCVRect];
    [mStolenContentView setNeedsDisplay:YES];

    [[mStolenContentView superview] setNeedsDisplay:YES];
    mMinWidth = 2 * ButtonSideMargin + ButtonSpacing + 2 * ButtonWidth;
    mExtraHeight = Y;
}

- (void)onOkClicked
{
    Q_ASSERT(mHackedPanel);
    [[mStolenContentView window] close];
    [self updateQtColor];
    [self finishOffWithCode:NSOKButton];
}

- (void)onCancelClicked
{
    if (mHackedPanel) {
        [[mStolenContentView window] close];
        delete mQtColor;
        mQtColor = new QColor();
        [self finishOffWithCode:NSCancelButton];
    }
}

- (void)updateQtColor
{
    delete mQtColor;
    mQtColor = new QColor();
    NSColor *color = [mColorPanel color];
    NSString *colorSpaceName = [color colorSpaceName];
    if (colorSpaceName == NSDeviceCMYKColorSpace) {
        CGFloat cyan = 0, magenta = 0, yellow = 0, black = 0, alpha = 0;
        [color getCyan:&cyan magenta:&magenta yellow:&yellow black:&black alpha:&alpha];
        mQtColor->setCmykF(cyan, magenta, yellow, black, alpha);
    } else if (colorSpaceName == NSCalibratedRGBColorSpace || colorSpaceName == NSDeviceRGBColorSpace)  {
        CGFloat red = 0, green = 0, blue = 0, alpha = 0;
        [color getRed:&red green:&green blue:&blue alpha:&alpha];
        mQtColor->setRgbF(red, green, blue, alpha);
    } else if (colorSpaceName == NSNamedColorSpace) {
        NSColor *tmpColor = [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
        CGFloat red = 0, green = 0, blue = 0, alpha = 0;
        [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
        mQtColor->setRgbF(red, green, blue, alpha);
    } else {
        NSColorSpace *colorSpace = [color colorSpace];
        if ([colorSpace colorSpaceModel] == NSCMYKColorSpaceModel && [color numberOfComponents] == 5){
            CGFloat components[5];
            [color getComponents:components];
            mQtColor->setCmykF(components[0], components[1], components[2], components[3], components[4]);
        } else {
            NSColor *tmpColor = [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
            CGFloat red = 0, green = 0, blue = 0, alpha = 0;
            [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
            mQtColor->setRgbF(red, green, blue, alpha);
        }
    }

    mPriv->setCurrentQColor(*mQtColor);
}

- (NSColorPanel *)colorPanel
{
    return mColorPanel;
}

- (QColor)qtColor
{
    return *mQtColor;
}

- (void)finishOffWithCode:(NSInteger)code
{
    mResultCode = code;
    if (mDialogIsExecuting) {
        // We stop the current modal event loop. The control
        // will then return inside -(void)exec below.
        // It's important that the modal event loop is stopped before
        // we accept/reject QColorDialog, since QColorDialog has its
        // own event loop that needs to be stopped last. 
        [NSApp stopModalWithCode:code];
    } else {
        // Since we are not in a modal event loop, we can safely close
        // down QColorDialog
        // Calling accept() or reject() can in turn call closeCocoaColorPanel.
        // This check will prevent any such recursion.
        if (!mResultSet) {
            mResultSet = true;
            if (mResultCode == NSCancelButton) {
                mPriv->colorDialog()->reject();
            } else {
                mPriv->colorDialog()->accept();
            }
        } 
    }
}

- (void)showColorPanel
{
    mDialogIsExecuting = false;
    [mColorPanel makeKeyAndOrderFront:mColorPanel];
}

- (void)exec
{
    QBoolBlocker nativeDialogOnTop(QApplicationPrivate::native_modal_dialog_active);
    QMacCocoaAutoReleasePool pool;
    mDialogIsExecuting = true;
    bool modalEnded = false;
    while (!modalEnded) {
#ifndef QT_NO_EXCEPTIONS
        @try {
            [NSApp runModalForWindow:mColorPanel];
            modalEnded = true;
        } @catch (NSException *) {
            // For some reason, NSColorPanel throws an exception when
            // clicking on 'SelectedMenuItemColor' from the 'Developer'
            // palette (tab three).
        }
#else
        [NSApp runModalForWindow:mColorPanel];
        modalEnded = true;
#endif
    }

    QAbstractEventDispatcher::instance()->interrupt();
    if (mResultCode == NSCancelButton)
        mPriv->colorDialog()->reject();
    else
        mPriv->colorDialog()->accept();
}

@end

QT_BEGIN_NAMESPACE

extern void macStartInterceptNSPanelCtor();
extern void macStopInterceptNSPanelCtor();
extern NSButton *macCreateButton(const char *text, NSView *superview);

void QColorDialogPrivate::openCocoaColorPanel(const QColor &initial,
        QWidget *parent, const QString &title, QColorDialog::ColorDialogOptions options)
{
    Q_UNUSED(parent);   // we would use the parent if only NSColorPanel could be a sheet
    QMacCocoaAutoReleasePool pool;

    if (!delegate) {
        /*
           The standard Cocoa color panel has no OK or Cancel button and
           is created as a utility window, whereas we want something like
           the Carbon color panel. We need to take the following steps:

           1. Intercept the color panel constructor to turn off the
           NSUtilityWindowMask flag. This is done by temporarily
           replacing initWithContentRect:styleMask:backing:defer:
           in NSPanel by our own method.

           2. Modify the color panel so that its content view is part
           of a new content view that contains it as well as two
           buttons (OK and Cancel).

           3. Lay out the original content view and the buttons when
           the color panel is shown and whenever it is resized.

           4. Clean up after ourselves.
         */

        bool hackColorPanel = !(options & QColorDialog::NoButtons);

        if (hackColorPanel)
            macStartInterceptNSPanelCtor();
        NSColorPanel *colorPanel = [NSColorPanel sharedColorPanel];
        if (hackColorPanel)
            macStopInterceptNSPanelCtor();

        [colorPanel setHidesOnDeactivate:false];

        // set up the Cocoa color panel
        [colorPanel setShowsAlpha:options & QColorDialog::ShowAlphaChannel];
        [colorPanel setTitle:(NSString*)(CFStringRef)QCFString(title)];

        NSView *stolenContentView = 0;
        NSButton *okButton = 0;
        NSButton *cancelButton = 0;

        if (hackColorPanel) {
            // steal the color panel's contents view
            stolenContentView = [colorPanel contentView];
            [stolenContentView retain];
            [colorPanel setContentView:0];

            // create a new content view and add the stolen one as a subview
            NSRect frameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };
            NSView *ourContentView = [[NSView alloc] initWithFrame:frameRect];
            [ourContentView addSubview:stolenContentView];

            // create OK and Cancel buttons and add these as subviews
            okButton = macCreateButton("&OK", ourContentView);
            cancelButton = macCreateButton("Cancel", ourContentView);

            [colorPanel setContentView:ourContentView];
            [colorPanel setDefaultButtonCell:[okButton cell]];
        }

        delegate = [[QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) alloc] initWithColorPanel:colorPanel
            stolenContentView:stolenContentView
            okButton:okButton
            cancelButton:cancelButton
            priv:this];
        [colorPanel setDelegate:static_cast<QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) *>(delegate)];
    }
    [static_cast<QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) *>(delegate) setResultSet:NO];
    setCocoaPanelColor(initial);
    [static_cast<QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) *>(delegate) showColorPanel];
}

void QColorDialogPrivate::closeCocoaColorPanel()
{
    [static_cast<QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) *>(delegate) onCancelClicked];
}

void QColorDialogPrivate::releaseCocoaColorPanelDelegate()
{
    [static_cast<QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) *>(delegate) release];
}

void QColorDialogPrivate::mac_nativeDialogModalHelp()
{
    // Do a queued meta-call to open the native modal dialog so it opens after the new
    // event loop has started to execute (in QDialog::exec). Using a timer rather than
    // a queued meta call is intentional to ensure that the call is only delivered when
    // [NSApp run] runs (timers are handeled special in cocoa). If NSApp is not
    // running (which is the case if e.g a top-most QEventLoop has been
    // interrupted, and the second-most event loop has not yet been reactivated (regardless
    // if [NSApp run] is still on the stack)), showing a native modal dialog will fail.
    if (delegate){
        Q_Q(QColorDialog);
        QTimer::singleShot(1, q, SLOT(_q_macRunNativeAppModalPanel()));
    }
}

void QColorDialogPrivate::_q_macRunNativeAppModalPanel()
{
    [static_cast<QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) *>(delegate) exec];
}

void QColorDialogPrivate::setCocoaPanelColor(const QColor &color)
{
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) *theDelegate = static_cast<QT_MANGLE_NAMESPACE(QCocoaColorPanelDelegate) *>(delegate);
    NSColor *nsColor;
    const QColor::Spec spec = color.spec();
    if (spec == QColor::Cmyk) {
        nsColor = [NSColor colorWithDeviceCyan:color.cyanF()
                                       magenta:color.magentaF()
                                        yellow:color.yellowF()
                                         black:color.blackF()
                                         alpha:color.alphaF()];
    } else {
        nsColor = [NSColor colorWithCalibratedRed:color.redF()
                                            green:color.greenF()
                                             blue:color.blueF()
                                            alpha:color.alphaF()];
    }
    [[theDelegate colorPanel] setColor:nsColor];
}

QT_END_NAMESPACE

#endif

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

#include "qfontdialog_p.h"
#if !defined(QT_NO_FONTDIALOG) && defined(Q_WS_MAC)
#include <qapplication.h>
#include <qdialogbuttonbox.h>
#include <qlineedit.h>
#include <private/qapplication_p.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>
#include <private/qt_mac_p.h>
#include <qabstracteventdispatcher.h>
#include <qdebug.h>
#include <private/qfontengine_coretext_p.h>
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#if !CGFLOAT_DEFINED
typedef float CGFloat;  // Should only not be defined on 32-bit platforms
#endif

QT_BEGIN_NAMESPACE

extern void macStartInterceptNSPanelCtor();
extern void macStopInterceptNSPanelCtor();
extern NSButton *macCreateButton(const char *text, NSView *superview);
extern bool qt_mac_is_macsheet(const QWidget *w); // qwidget_mac.mm

QT_END_NAMESPACE
QT_USE_NAMESPACE

// should a priori be kept in sync with qcolordialog_mac.mm
const CGFloat ButtonMinWidth = 78.0;
const CGFloat ButtonMinHeight = 32.0;
const CGFloat ButtonSpacing = 0.0;
const CGFloat ButtonTopMargin = 0.0;
const CGFloat ButtonBottomMargin = 7.0;
const CGFloat ButtonSideMargin = 9.0;

// looks better with some margins
const CGFloat DialogTopMargin = 7.0;
const CGFloat DialogSideMargin = 9.0;

const int StyleMask = NSTitledWindowMask | NSClosableWindowMask | NSResizableWindowMask;

@class QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate);


#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_5

@protocol NSWindowDelegate <NSObject>
- (NSSize)windowWillResize:(NSWindow *)window toSize:(NSSize)proposedFrameSize;
@end

#endif

@interface QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) : NSObject <NSWindowDelegate> {
    NSFontPanel *mFontPanel;
    NSView *mStolenContentView;
    NSButton *mOkButton;
    NSButton *mCancelButton;
    QFontDialogPrivate *mPriv;
    QFont *mQtFont;
    BOOL mPanelHackedWithButtons;
    CGFloat mDialogExtraWidth;
    CGFloat mDialogExtraHeight;
    int mReturnCode;
    BOOL mAppModal;
}
- (id)initWithFontPanel:(NSFontPanel *)panel
      stolenContentView:(NSView *)stolenContentView
               okButton:(NSButton *)okButton
           cancelButton:(NSButton *)cancelButton
                   priv:(QFontDialogPrivate *)priv
             extraWidth:(CGFloat)extraWidth
            extraHeight:(CGFloat)extraHeight;
- (void)showModelessPanel;
- (void)showWindowModalSheet:(QWidget *)docWidget;
- (void)runApplicationModalPanel;
- (BOOL)isAppModal;
- (void)changeFont:(id)sender;
- (void)changeAttributes:(id)sender;
- (BOOL)windowShouldClose:(id)window;
- (NSSize)windowWillResize:(NSWindow *)window toSize:(NSSize)proposedFrameSize;
- (void)relayout;
- (void)relayoutToContentSize:(NSSize)frameSize;
- (void)onOkClicked;
- (void)onCancelClicked;
- (NSFontPanel *)fontPanel;
- (NSWindow *)actualPanel;
- (NSSize)dialogExtraSize;
- (void)setQtFont:(const QFont &)newFont;
- (QFont)qtFont;
- (void)finishOffWithCode:(NSInteger)result;
- (void)cleanUpAfterMyself;
- (void)setSubwindowStacking;
@end

static QFont qfontForCocoaFont(NSFont *cocoaFont, const QFont &resolveFont)
{
    QFont newFont;
    if (cocoaFont) {
        int pSize = qRound([cocoaFont pointSize]);
        QString family(qt_mac_NSStringToQString([cocoaFont familyName]));
        QString typeface(qt_mac_NSStringToQString([cocoaFont fontName]));

        int hyphenPos = typeface.indexOf(QLatin1Char('-'));
        if (hyphenPos != -1) {
            typeface.remove(0, hyphenPos + 1);
        } else {
            typeface = QLatin1String("Normal");
        }

        newFont = QFontDatabase().font(family, typeface, pSize);
        newFont.setUnderline(resolveFont.underline());
        newFont.setStrikeOut(resolveFont.strikeOut());

    }
    return newFont;
}

@implementation QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate)
- (id)initWithFontPanel:(NSFontPanel *)panel
       stolenContentView:(NSView *)stolenContentView
                okButton:(NSButton *)okButton
            cancelButton:(NSButton *)cancelButton
                    priv:(QFontDialogPrivate *)priv
              extraWidth:(CGFloat)extraWidth
             extraHeight:(CGFloat)extraHeight
{
    self = [super init];
    mFontPanel = panel;
    mStolenContentView = stolenContentView;
    mOkButton = okButton;
    mCancelButton = cancelButton;
    mPriv = priv;
    mPanelHackedWithButtons = (okButton != 0);
    mDialogExtraWidth = extraWidth;
    mDialogExtraHeight = extraHeight;
    mReturnCode = -1;
    mAppModal = false;

    if (mPanelHackedWithButtons) {
        [self relayout];

        [okButton setAction:@selector(onOkClicked)];
        [okButton setTarget:self];

        [cancelButton setAction:@selector(onCancelClicked)];
        [cancelButton setTarget:self];
    }

    mQtFont = new QFont();
    return self;
}

- (void)setSubwindowStacking
{
#ifdef QT_MAC_USE_COCOA
    // Stack the native dialog in front of its parent, if any:
    QFontDialog *q = mPriv->fontDialog();
    if (!qt_mac_is_macsheet(q)) {
        if (QWidget *parent = q->parentWidget()) {
            if (parent->isWindow()) {
                [qt_mac_window_for(parent)
                    addChildWindow:[mStolenContentView window] ordered:NSWindowAbove];
            }
        }
    }
#endif
}

- (void)dealloc
{
    delete mQtFont;
    [super dealloc];
}

- (void)showModelessPanel
{
    mAppModal = false;
    NSWindow *ourPanel = [mStolenContentView window];
    [ourPanel makeKeyAndOrderFront:self];
}

- (void)runApplicationModalPanel
{
    QBoolBlocker nativeDialogOnTop(QApplicationPrivate::native_modal_dialog_active);
    mAppModal = true;
    NSWindow *ourPanel = [mStolenContentView window];
    [ourPanel setReleasedWhenClosed:NO];
    [NSApp runModalForWindow:ourPanel];
    QAbstractEventDispatcher::instance()->interrupt();

    if (mReturnCode == NSOKButton)
        mPriv->fontDialog()->accept();
    else
        mPriv->fontDialog()->reject();
}

- (BOOL)isAppModal
{
    return mAppModal;
}

- (void)showWindowModalSheet:(QWidget *)docWidget
{
#ifdef QT_MAC_USE_COCOA
    NSWindow *window = qt_mac_window_for(docWidget);
#else
    WindowRef hiwindowRef = qt_mac_window_for(docWidget);
    NSWindow *window = [[NSWindow alloc] initWithWindowRef:hiwindowRef];
    CFRetain(hiwindowRef);
#endif

    mAppModal = false;
    NSWindow *ourPanel = [mStolenContentView window];
    [NSApp beginSheet:ourPanel
        modalForWindow:window
        modalDelegate:0
        didEndSelector:0
        contextInfo:0 ];

#ifndef QT_MAC_USE_COCOA
    CFRelease(hiwindowRef);
#endif
}

- (void)changeFont:(id)sender
{
    NSFont *dummyFont = [NSFont userFontOfSize:12.0];
    [self setQtFont:qfontForCocoaFont([sender convertFont:dummyFont], *mQtFont)];
    if (mPriv)
        mPriv->updateSampleFont(*mQtFont);
}

- (void)changeAttributes:(id)sender
{
    NSDictionary *dummyAttribs = [NSDictionary dictionary];
    NSDictionary *attribs = [sender convertAttributes:dummyAttribs];

#ifdef QT_MAC_USE_COCOA
    for (id key in attribs) {
#else
    NSEnumerator *enumerator = [attribs keyEnumerator];
    id key;
    while((key = [enumerator nextObject])) {
#endif
        NSNumber *number = static_cast<NSNumber *>([attribs objectForKey:key]);
        if ([key isEqual:NSUnderlineStyleAttributeName]) {
            mQtFont->setUnderline([number intValue] != NSUnderlineStyleNone);
        } else if ([key isEqual:NSStrikethroughStyleAttributeName]) {
            mQtFont->setStrikeOut([number intValue] != NSUnderlineStyleNone);
        }
    }

    if (mPriv)
        mPriv->updateSampleFont(*mQtFont);
}

- (BOOL)windowShouldClose:(id)window
{
    Q_UNUSED(window);
    if (mPanelHackedWithButtons) {
        [self onCancelClicked];
    } else {
        [self finishOffWithCode:NSCancelButton];
    }
    return true;
}

- (NSSize)windowWillResize:(NSWindow *)window toSize:(NSSize)proposedFrameSize
{
    if (mFontPanel == window) {
        proposedFrameSize = [static_cast<id <NSWindowDelegate> >(mFontPanel) windowWillResize:mFontPanel toSize:proposedFrameSize];
    } else {
        /*
            Ugly hack: NSFontPanel rearranges the layout of its main
            component in windowWillResize:toSize:. So we temporarily
            restore the stolen content view to its rightful owner,
            call windowWillResize:toSize:, and steal the content view
            again.
        */
        [mStolenContentView removeFromSuperview];
        [mFontPanel setContentView:mStolenContentView];
        NSSize extraSize = [self dialogExtraSize];
        proposedFrameSize.width -= extraSize.width;
        proposedFrameSize.height -= extraSize.height;
        proposedFrameSize = [static_cast<id <NSWindowDelegate> >(mFontPanel) windowWillResize:mFontPanel toSize:proposedFrameSize];
        NSRect frameRect = { { 0.0, 0.0 }, proposedFrameSize };
        [mFontPanel setFrame:frameRect display:NO];
        [mFontPanel setContentView:0];
        [[window contentView] addSubview:mStolenContentView];
        proposedFrameSize.width += extraSize.width;
        proposedFrameSize.height += extraSize.height;
    }
    if (mPanelHackedWithButtons) {
        NSRect frameRect = { { 0.0, 0.0 }, proposedFrameSize };
        NSRect contentRect = [NSWindow contentRectForFrameRect:frameRect styleMask:[window styleMask]];
        [self relayoutToContentSize:contentRect.size];
    }
    return proposedFrameSize;
}

- (void)relayout
{
    [self relayoutToContentSize:[[mStolenContentView superview] frame].size];
}

- (void)relayoutToContentSize:(NSSize)frameSize
{
    Q_ASSERT(mPanelHackedWithButtons);

    [mOkButton sizeToFit];
    NSSize okSizeHint = [mOkButton frame].size;

    [mCancelButton sizeToFit];
    NSSize cancelSizeHint = [mCancelButton frame].size;

    const CGFloat ButtonWidth = qMin(qMax(ButtonMinWidth,
                qMax(okSizeHint.width, cancelSizeHint.width)),
            CGFloat((frameSize.width - 2.0 * ButtonSideMargin - ButtonSpacing) * 0.5));
    const CGFloat ButtonHeight = qMax(ButtonMinHeight,
                                     qMax(okSizeHint.height, cancelSizeHint.height));

    const CGFloat X = DialogSideMargin;
    const CGFloat Y = ButtonBottomMargin + ButtonHeight + ButtonTopMargin;

    NSRect okRect = { { frameSize.width - ButtonSideMargin - ButtonWidth,
                        ButtonBottomMargin },
                      { ButtonWidth, ButtonHeight } };
    [mOkButton setFrame:okRect];
    [mOkButton setNeedsDisplay:YES];

    NSRect cancelRect = { { okRect.origin.x - ButtonSpacing - ButtonWidth,
                            ButtonBottomMargin },
                            { ButtonWidth, ButtonHeight } };
    [mCancelButton setFrame:cancelRect];
    [mCancelButton setNeedsDisplay:YES];

    NSRect stolenCVRect = { { X, Y },
                            { frameSize.width - X - X, frameSize.height - Y - DialogTopMargin } };
    [mStolenContentView setFrame:stolenCVRect];
    [mStolenContentView setNeedsDisplay:YES];

    [[mStolenContentView superview] setNeedsDisplay:YES];
}

- (void)onOkClicked
{
    Q_ASSERT(mPanelHackedWithButtons);
    NSFontManager *fontManager = [NSFontManager sharedFontManager];
    [self setQtFont:qfontForCocoaFont([fontManager convertFont:[fontManager selectedFont]],
                                      *mQtFont)];
    [self finishOffWithCode:NSOKButton];
}

- (void)onCancelClicked
{
    Q_ASSERT(mPanelHackedWithButtons);
    [self finishOffWithCode:NSCancelButton];
}

- (NSFontPanel *)fontPanel
{
    return mFontPanel;
}

- (NSWindow *)actualPanel
{
    return [mStolenContentView window];
}

- (NSSize)dialogExtraSize
{
    // this must be recomputed each time, because sometimes the
    // NSFontPanel has the NSDocModalWindowMask flag set, and sometimes
    // not -- which affects the frame rect vs. content rect measurements

    // take the different frame rectangles into account for dialogExtra{Width,Height}
    NSRect someRect = { { 0.0, 0.0 }, { 100000.0, 100000.0 } };
    NSRect sharedFontPanelContentRect = [mFontPanel contentRectForFrameRect:someRect];
    NSRect ourPanelContentRect = [NSWindow contentRectForFrameRect:someRect styleMask:StyleMask];

    NSSize result = { mDialogExtraWidth, mDialogExtraHeight };
    result.width -= ourPanelContentRect.size.width - sharedFontPanelContentRect.size.width;
    result.height -= ourPanelContentRect.size.height - sharedFontPanelContentRect.size.height;
    return result;
}

- (void)setQtFont:(const QFont &)newFont
{
    delete mQtFont;
    mQtFont = new QFont(newFont);
}

- (QFont)qtFont
{
    return *mQtFont;
}

- (void)finishOffWithCode:(NSInteger)code
{
#ifdef QT_MAC_USE_COCOA
    QFontDialog *q = mPriv->fontDialog();
    if (QWidget *parent = q->parentWidget()) {
        if (parent->isWindow()) {
            [qt_mac_window_for(parent) removeChildWindow:[mStolenContentView window]];
        }
    }
#endif

    if(code == NSOKButton)
        mPriv->sampleEdit->setFont([self qtFont]);

    if (mAppModal) {
        mReturnCode = code;
        [NSApp stopModalWithCode:code];
    } else {
        if (code == NSOKButton)
            mPriv->fontDialog()->accept();
        else
            mPriv->fontDialog()->reject();
    }
}

- (void)cleanUpAfterMyself
{
    if (mPanelHackedWithButtons) {
        NSView *ourContentView = [mFontPanel contentView];

        // return stolen stuff to its rightful owner
        [mStolenContentView removeFromSuperview];
        [mFontPanel setContentView:mStolenContentView];

        [mOkButton release];
        [mCancelButton release];
        [ourContentView release];
    }
    [mFontPanel setDelegate:nil];
    [[NSFontManager sharedFontManager] setDelegate:nil];
#ifdef QT_MAC_USE_COCOA
    [[NSFontManager sharedFontManager] setTarget:nil];
#endif
}
@end

QT_BEGIN_NAMESPACE

void QFontDialogPrivate::closeCocoaFontPanel()
{
    QMacCocoaAutoReleasePool pool;
    QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) *theDelegate = static_cast<QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) *>(delegate);
    NSWindow *ourPanel = [theDelegate actualPanel];
    [ourPanel close];
    if ([theDelegate isAppModal])
        [ourPanel release];
    [theDelegate cleanUpAfterMyself];
    [theDelegate release];
    this->delegate = 0;
    sharedFontPanelAvailable = true;
}

void QFontDialogPrivate::setFont(void *delegate, const QFont &font)
{
    QMacCocoaAutoReleasePool pool;
    QFontEngine *fe = font.d->engineForScript(QUnicodeTables::Common);
    NSFontManager *mgr = [NSFontManager sharedFontManager];
    const NSFont *nsFont = 0;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
    if (qstrcmp(fe->name(), "CoreText") == 0) {
        nsFont = reinterpret_cast<const NSFont *>(static_cast<QCoreTextFontEngineMulti *>(fe)->ctfont);
    } else
#endif
    {
        int weight = 5;
        NSFontTraitMask mask = 0;
        if (font.style() == QFont::StyleItalic) {
            mask |= NSItalicFontMask;
        }
        if (font.weight() == QFont::Bold) {
            weight = 9;
            mask |= NSBoldFontMask;
        }

        NSFontManager *mgr = [NSFontManager sharedFontManager];
        QFontInfo fontInfo(font);
        nsFont = [mgr fontWithFamily:qt_mac_QStringToNSString(fontInfo.family())
            traits:mask
            weight:weight
            size:fontInfo.pointSize()];
    }

    [mgr setSelectedFont:const_cast<NSFont *>(nsFont) isMultiple:NO];
    [static_cast<QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) *>(delegate) setQtFont:font];
}

void QFontDialogPrivate::createNSFontPanelDelegate()
{
    if (delegate)
        return;

    sharedFontPanelAvailable = false;
    QMacCocoaAutoReleasePool pool;
    bool sharedFontPanelExisted = [NSFontPanel sharedFontPanelExists];
    NSFontPanel *sharedFontPanel = [NSFontPanel sharedFontPanel];
    [sharedFontPanel setHidesOnDeactivate:false];

    // hack to ensure that QCocoaApplication's validModesForFontPanel:
    // implementation is honored
    if (!sharedFontPanelExisted) {
        [sharedFontPanel makeKeyAndOrderFront:sharedFontPanel];
        [sharedFontPanel close];
    }

    NSPanel *ourPanel = 0;
    NSView *stolenContentView = 0;
    NSButton *okButton = 0;
    NSButton *cancelButton = 0;

    CGFloat dialogExtraWidth = 0.0;
    CGFloat dialogExtraHeight = 0.0;

    // compute dialogExtra{Width,Height}
    dialogExtraWidth = 2.0 * DialogSideMargin;
    dialogExtraHeight = DialogTopMargin + ButtonTopMargin + ButtonMinHeight + ButtonBottomMargin;

    // compute initial contents rectangle
    NSRect contentRect = [sharedFontPanel contentRectForFrameRect:[sharedFontPanel frame]];
    contentRect.size.width += dialogExtraWidth;
    contentRect.size.height += dialogExtraHeight;

    // create the new panel
    ourPanel = [[NSPanel alloc] initWithContentRect:contentRect
                styleMask:StyleMask
                    backing:NSBackingStoreBuffered
                        defer:YES];
    [ourPanel setReleasedWhenClosed:YES];
    stolenContentView = [sharedFontPanel contentView];

    // steal the font panel's contents view
    [stolenContentView retain];
    [sharedFontPanel setContentView:0];

    {
        // create a new content view and add the stolen one as a subview
        NSRect frameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };
        NSView *ourContentView = [[NSView alloc] initWithFrame:frameRect];
        [ourContentView addSubview:stolenContentView];

        // create OK and Cancel buttons and add these as subviews
        okButton = macCreateButton("&OK", ourContentView);
        cancelButton = macCreateButton("Cancel", ourContentView);

        [ourPanel setContentView:ourContentView];
        [ourPanel setDefaultButtonCell:[okButton cell]];
    }

    // create the delegate and set it
    QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) *del = [[QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) alloc] initWithFontPanel:sharedFontPanel
                                             stolenContentView:stolenContentView
                                                      okButton:okButton
                                                  cancelButton:cancelButton
                                                          priv:this
                                                    extraWidth:dialogExtraWidth
                                                   extraHeight:dialogExtraHeight];
    delegate = del;
    [ourPanel setDelegate:del];

    [[NSFontManager sharedFontManager] setDelegate:del];
#ifdef QT_MAC_USE_COCOA
    [[NSFontManager sharedFontManager] setTarget:del];
#endif
    setFont(del, q_func()->currentFont());

    {
        // hack to get correct initial layout
        NSRect frameRect = [ourPanel frame];
        frameRect.size.width += 1.0;
        [ourPanel setFrame:frameRect display:NO];
        frameRect.size.width -= 1.0;
        frameRect.size = [del windowWillResize:ourPanel toSize:frameRect.size];
        [ourPanel setFrame:frameRect display:NO];
        [ourPanel center];
    }
    [del setSubwindowStacking];
    NSString *title = @"Select font";
    [ourPanel setTitle:title];
}

void QFontDialogPrivate::mac_nativeDialogModalHelp()
{
    // Copied from QFileDialogPrivate
    // Do a queued meta-call to open the native modal dialog so it opens after the new
    // event loop has started to execute (in QDialog::exec). Using a timer rather than
    // a queued meta call is intentional to ensure that the call is only delivered when
    // [NSApp run] runs (timers are handeled special in cocoa). If NSApp is not
    // running (which is the case if e.g a top-most QEventLoop has been
    // interrupted, and the second-most event loop has not yet been reactivated (regardless
    // if [NSApp run] is still on the stack)), showing a native modal dialog will fail.
    if (nativeDialogInUse) {
        Q_Q(QFontDialog);
        QTimer::singleShot(1, q, SLOT(_q_macRunNativeAppModalPanel()));
    }
}

// The problem with the native font dialog is that OS X does not
// offer a proper dialog, but a panel (i.e. without Ok and Cancel buttons).
// This means we need to "construct" a native dialog by taking the panel
// and "adding" the buttons.
void QFontDialogPrivate::_q_macRunNativeAppModalPanel()
{
    createNSFontPanelDelegate();
    QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) *del = static_cast<QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) *>(delegate);
    [del runApplicationModalPanel];
}

bool QFontDialogPrivate::showCocoaFontPanel()
{
    if (!sharedFontPanelAvailable)
        return false;

    Q_Q(QFontDialog);
    QMacCocoaAutoReleasePool pool;
    createNSFontPanelDelegate();
    QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) *del = static_cast<QT_MANGLE_NAMESPACE(QCocoaFontPanelDelegate) *>(delegate);
    if (qt_mac_is_macsheet(q))
        [del showWindowModalSheet:q->parentWidget()];
    else
        [del showModelessPanel];
    return true;
}

bool QFontDialogPrivate::hideCocoaFontPanel()
{
    if (!delegate){
        // Nothing to do. We return false to leave the question
        // open regarding whether or not to go native:
        return false;
    } else {
        closeCocoaFontPanel();
        // Even when we hide it, we are still using a
        // native dialog, so return true:
        return true;
    }
}
bool QFontDialogPrivate::setVisible_sys(bool visible)
{
    Q_Q(QFontDialog);
    if (!visible == q->isHidden())
        return false;

    return visible ? showCocoaFontPanel() : hideCocoaFontPanel();
}

QT_END_NAMESPACE

#endif

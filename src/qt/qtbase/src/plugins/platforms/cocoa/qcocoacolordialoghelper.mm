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

#include "qcocoacolordialoghelper.h"

#ifndef QT_NO_COLORDIALOG

#include <QtCore/qdebug.h>
#include <QtCore/qtimer.h>

#include "qcocoahelpers.h"

#import <AppKit/AppKit.h>

QT_USE_NAMESPACE

static NSButton *macCreateButton(const char *text, NSView *superview)
{
    static const NSRect buttonFrameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };

    NSButton *button = [[NSButton alloc] initWithFrame:buttonFrameRect];
    [button setButtonType:NSMomentaryLightButton];
    [button setBezelStyle:NSRoundedBezelStyle];
    [button setTitle:(NSString*)(CFStringRef)QCFString(QCoreApplication::translate("QDialogButtonBox", text)
                                                       .remove(QLatin1Char('&')))];
    [[button cell] setFont:[NSFont systemFontOfSize:
            [NSFont systemFontSizeForControlSize:NSRegularControlSize]]];
    [superview addSubview:button];
    return button;
}

@class QT_MANGLE_NAMESPACE(QNSColorPanelDelegate);

@interface QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) : NSObject<NSWindowDelegate>
{
    @public
    NSColorPanel *mColorPanel;
    QCocoaColorDialogHelper *mHelper;
    NSView *mStolenContentView;
    NSButton *mOkButton;
    NSButton *mCancelButton;
    QColor mQtColor;
    NSInteger mResultCode;
    BOOL mDialogIsExecuting;
    BOOL mResultSet;
};
- (void)restoreOriginalContentView;
- (void)relayout;
- (void)updateQtColor;
- (void)finishOffWithCode:(NSInteger)code;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSColorPanelDelegate);

@implementation QNSColorPanelDelegate

- (id)init
{
    self = [super init];
    mColorPanel = [NSColorPanel sharedColorPanel];
    mHelper = 0;
    mStolenContentView = 0;
    mOkButton = 0;
    mCancelButton = 0;
    mResultCode = NSCancelButton;
    mDialogIsExecuting = false;
    mResultSet = false;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7)
        [mColorPanel setRestorable:NO];
#endif

    [[NSNotificationCenter defaultCenter] addObserver:self
        selector:@selector(colorChanged:)
        name:NSColorPanelColorDidChangeNotification
        object:mColorPanel];

    [mColorPanel retain];
    return self;
}

- (void)dealloc
{
    [self restoreOriginalContentView];
    [mColorPanel setDelegate:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    [super dealloc];
}

- (void)setDialogHelper:(QCocoaColorDialogHelper *)helper
{
    mHelper = helper;
    [mColorPanel setShowsAlpha:mHelper->options()->testOption(QColorDialogOptions::ShowAlphaChannel)];
    if (mHelper->options()->testOption(QColorDialogOptions::NoButtons)) {
        [self restoreOriginalContentView];
    } else if (!mStolenContentView) {
        // steal the color panel's contents view
        mStolenContentView = [mColorPanel contentView];
        [mStolenContentView retain];
        [mColorPanel setContentView:0];

        // create a new content view and add the stolen one as a subview
        NSRect frameRect = { { 0.0, 0.0 }, { 0.0, 0.0 } };
        NSView *ourContentView = [[NSView alloc] initWithFrame:frameRect];
        [ourContentView addSubview:mStolenContentView];

        // create OK and Cancel buttons and add these as subviews
        mOkButton = macCreateButton("&OK", ourContentView);
        mCancelButton = macCreateButton("Cancel", ourContentView);

        [mColorPanel setContentView:ourContentView];
        [mColorPanel setDefaultButtonCell:[mOkButton cell]];
        [self relayout];

        [mOkButton setAction:@selector(onOkClicked)];
        [mOkButton setTarget:self];

        [mCancelButton setAction:@selector(onCancelClicked)];
        [mCancelButton setTarget:self];
    }
}

- (void)closePanel
{
    [mColorPanel close];
}

- (void)windowDidResize:(NSNotification *)notification
{
    Q_UNUSED(notification);
    [self relayout];
}

- (void)colorChanged:(NSNotification *)notification
{
    Q_UNUSED(notification);
    [self updateQtColor];
    if (mHelper)
        emit mHelper->colorSelected(mQtColor);
}

- (void)restoreOriginalContentView
{
    if (mStolenContentView) {
        NSView *ourContentView = [mColorPanel contentView];

        // return stolen stuff to its rightful owner
        [mStolenContentView removeFromSuperview];
        [mColorPanel setContentView:mStolenContentView];
        [mOkButton release];
        [mCancelButton release];
        [ourContentView release];
        mOkButton = 0;
        mCancelButton = 0;
        mStolenContentView = 0;
    }
}

- (void)relayout
{
    if (!mOkButton)
        return;

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
}

- (void)onOkClicked
{
    [mColorPanel close];
    [self updateQtColor];
    [self finishOffWithCode:NSOKButton];
}

- (void)onCancelClicked
{
    if (mOkButton) {
        [mColorPanel close];
        mQtColor = QColor();
        [self finishOffWithCode:NSCancelButton];
    }
}

- (void)updateQtColor
{
    NSColor *color = [mColorPanel color];
    NSString *colorSpaceName = [color colorSpaceName];
    if (colorSpaceName == NSDeviceCMYKColorSpace) {
        CGFloat cyan = 0, magenta = 0, yellow = 0, black = 0, alpha = 0;
        [color getCyan:&cyan magenta:&magenta yellow:&yellow black:&black alpha:&alpha];
        mQtColor.setCmykF(cyan, magenta, yellow, black, alpha);
    } else if (colorSpaceName == NSCalibratedRGBColorSpace || colorSpaceName == NSDeviceRGBColorSpace)  {
        CGFloat red = 0, green = 0, blue = 0, alpha = 0;
        [color getRed:&red green:&green blue:&blue alpha:&alpha];
        mQtColor.setRgbF(red, green, blue, alpha);
    } else if (colorSpaceName == NSNamedColorSpace) {
        NSColor *tmpColor = [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
        CGFloat red = 0, green = 0, blue = 0, alpha = 0;
        [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
        mQtColor.setRgbF(red, green, blue, alpha);
    } else {
        NSColorSpace *colorSpace = [color colorSpace];
        if ([colorSpace colorSpaceModel] == NSCMYKColorSpaceModel && [color numberOfComponents] == 5){
            CGFloat components[5];
            [color getComponents:components];
            mQtColor.setCmykF(components[0], components[1], components[2], components[3], components[4]);
        } else {
            NSColor *tmpColor = [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
            CGFloat red = 0, green = 0, blue = 0, alpha = 0;
            [tmpColor getRed:&red green:&green blue:&blue alpha:&alpha];
            mQtColor.setRgbF(red, green, blue, alpha);
        }
    }
    if (mHelper)
        emit mHelper->currentColorChanged(mQtColor);
}

- (void)showModelessPanel
{
    mDialogIsExecuting = false;
    mResultSet = false;
    [mColorPanel makeKeyAndOrderFront:mColorPanel];
}

- (BOOL)runApplicationModalPanel
{
    mDialogIsExecuting = true;
    [mColorPanel setDelegate:self];
    [mColorPanel setContinuous:YES];
    // Call processEvents in case the event dispatcher has been interrupted, and needs to do
    // cleanup of modal sessions. Do this before showing the native dialog, otherwise it will
    // close down during the cleanup.
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    [NSApp runModalForWindow:mColorPanel];
    mDialogIsExecuting = false;
    return (mResultCode == NSOKButton);
}

- (QPlatformDialogHelper::DialogCode)dialogResultCode
{
    return (mResultCode == NSOKButton) ? QPlatformDialogHelper::Accepted : QPlatformDialogHelper::Rejected;
}

- (BOOL)windowShouldClose:(id)window
{
    Q_UNUSED(window);
    if (!mOkButton)
        [self updateQtColor];
    if (mDialogIsExecuting) {
        [self finishOffWithCode:NSCancelButton];
    } else {
        mResultSet = true;
        if (mHelper)
            emit mHelper->reject();
    }
    return true;
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
                emit mHelper->reject();
            } else {
                emit mHelper->accept();
            }
        }
    }
}

@end

QT_BEGIN_NAMESPACE

class QCocoaColorPanel
{
public:
    QCocoaColorPanel()
    {
        mDelegate = [[QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) alloc] init];
    }

    ~QCocoaColorPanel()
    {
        [mDelegate release];
    }

    void init(QCocoaColorDialogHelper *helper)
    {
        [mDelegate setDialogHelper:helper];
    }

    void cleanup(QCocoaColorDialogHelper *helper)
    {
        if (mDelegate->mHelper == helper)
            mDelegate->mHelper = 0;
    }

    bool exec()
    {
        // Note: If NSApp is not running (which is the case if e.g a top-most
        // QEventLoop has been interrupted, and the second-most event loop has not
        // yet been reactivated (regardless if [NSApp run] is still on the stack)),
        // showing a native modal dialog will fail.
        return [mDelegate runApplicationModalPanel];
    }

    bool show(Qt::WindowModality windowModality, QWindow *parent)
    {
        Q_UNUSED(parent);
        if (windowModality != Qt::WindowModal)
            [mDelegate showModelessPanel];
        // no need to show a Qt::WindowModal dialog here, because it's necessary to call exec() in that case
        return true;
    }

    void hide()
    {
        [mDelegate closePanel];
    }

    QColor currentColor() const
    {
        return mDelegate->mQtColor;
    }

    void setCurrentColor(const QColor &color)
    {
        // make sure that if ShowAlphaChannel option is set then also setShowsAlpha
        // needs to be set, otherwise alpha value is omitted
        if (color.alpha() < 255)
            [mDelegate->mColorPanel setShowsAlpha:YES];

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
        mDelegate->mQtColor = color;
        [mDelegate->mColorPanel setColor:nsColor];
    }

private:
    QT_MANGLE_NAMESPACE(QNSColorPanelDelegate) *mDelegate;
};

Q_GLOBAL_STATIC(QCocoaColorPanel, sharedColorPanel)

QCocoaColorDialogHelper::QCocoaColorDialogHelper()
{
}

QCocoaColorDialogHelper::~QCocoaColorDialogHelper()
{
    sharedColorPanel()->cleanup(this);
}

void QCocoaColorDialogHelper::exec()
{
    if (sharedColorPanel()->exec())
        emit accept();
    else
        emit reject();
}

bool QCocoaColorDialogHelper::show(Qt::WindowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    if (windowModality == Qt::WindowModal)
        windowModality = Qt::ApplicationModal;
    sharedColorPanel()->init(this);
    return sharedColorPanel()->show(windowModality, parent);
}

void QCocoaColorDialogHelper::hide()
{
    sharedColorPanel()->hide();
}

void QCocoaColorDialogHelper::setCurrentColor(const QColor &color)
{
    sharedColorPanel()->init(this);
    sharedColorPanel()->setCurrentColor(color);
}

QColor QCocoaColorDialogHelper::currentColor() const
{
    return sharedColorPanel()->currentColor();
}

QT_END_NAMESPACE

#endif // QT_NO_COLORDIALOG

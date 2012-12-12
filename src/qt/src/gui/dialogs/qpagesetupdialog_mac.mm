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

#include "qpagesetupdialog.h"

#include <qhash.h>
#include <private/qapplication_p.h>
#include <private/qprintengine_mac_p.h>
#include <private/qabstractpagesetupdialog_p.h>

#ifndef QT_NO_PRINTDIALOG

QT_USE_NAMESPACE

@class QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate);

@interface QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) : NSObject {
    QMacPrintEnginePrivate *pe;
}
- (id)initWithMacPrintEngine:(QMacPrintEnginePrivate *)printEngine;
- (void)pageLayoutDidEnd:(NSPageLayout *)pageLayout
        returnCode:(int)returnCode contextInfo:(void *)contextInfo;
@end

@implementation QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate)
- (id)initWithMacPrintEngine:(QMacPrintEnginePrivate *)printEngine
{
    self = [super init];
    if (self) {
        pe = printEngine;
    }
    return self;

}
- (void)pageLayoutDidEnd:(NSPageLayout *)pageLayout
        returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    Q_UNUSED(pageLayout);
    QPageSetupDialog *dialog = static_cast<QPageSetupDialog *>(contextInfo);
    if (returnCode == NSOKButton) {
        PMRect paperRect;
        PMGetUnadjustedPaperRect(pe->format, &paperRect);
        pe->customSize = QSizeF(paperRect.right - paperRect.left,
                                paperRect.bottom - paperRect.top);
    }
    dialog->done((returnCode == NSOKButton) ? QDialog::Accepted : QDialog::Rejected);
}
@end

QT_BEGIN_NAMESPACE

extern void macStartInterceptWindowTitle(QWidget *window);
extern void macStopInterceptWindowTitle();

class QPageSetupDialogPrivate : public QAbstractPageSetupDialogPrivate
{
    Q_DECLARE_PUBLIC(QPageSetupDialog)

public:
    QPageSetupDialogPrivate() : ep(0)
#ifndef QT_MAC_USE_COCOA
    ,upp(0)
#else
    ,pageLayout(0)
#endif
    {}

    ~QPageSetupDialogPrivate() {
#ifndef QT_MAC_USE_COCOA
        if (upp) {
            DisposePMSheetDoneUPP(upp);
            upp = 0;
        }
        QHash<PMPrintSession, QPageSetupDialogPrivate *>::iterator it = sheetCallbackMap.begin();
        while (it != sheetCallbackMap.end()) {
            if (it.value() == this) {
                it = sheetCallbackMap.erase(it);
            } else {
                ++it;
            }
        }
#endif
    }

#ifndef QT_MAC_USE_COCOA
    void openCarbonPageLayout(Qt::WindowModality modality);
    void closeCarbonPageLayout();
    static void pageSetupDialogSheetDoneCallback(PMPrintSession printSession, WindowRef /*documentWindow*/, Boolean accepted) {
        QPageSetupDialogPrivate *priv = sheetCallbackMap.value(printSession);
        if (!priv) {
            qWarning("%s:%d: QPageSetupDialog::exec: Could not retrieve data structure, "
                     "you most likely now have an infinite modal loop", __FILE__, __LINE__);
            return;
        }
        priv->q_func()->done(accepted ? QDialog::Accepted : QDialog::Rejected);
    }
#else
    void openCocoaPageLayout(Qt::WindowModality modality);
    void closeCocoaPageLayout();
#endif

    QMacPrintEnginePrivate *ep;
#ifndef QT_MAC_USE_COCOA
    PMSheetDoneUPP upp;
    static QHash<PMPrintSession, QPageSetupDialogPrivate*> sheetCallbackMap;
#else
    NSPageLayout *pageLayout;
#endif
};

#ifndef QT_MAC_USE_COCOA
QHash<PMPrintSession, QPageSetupDialogPrivate*> QPageSetupDialogPrivate::sheetCallbackMap;
void QPageSetupDialogPrivate::openCarbonPageLayout(Qt::WindowModality modality)
{
    Q_Q(QPageSetupDialog);
    // If someone is reusing a QPrinter object, the end released all our old
    // information. In this case, we must reinitialize.
    if (ep->session == 0)
        ep->initialize();

    sheetCallbackMap.insert(ep->session, this);
    if (modality == Qt::ApplicationModal) {
	QWidget modal_widg(0, Qt::Window);
        modal_widg.setObjectName(QLatin1String(__FILE__ "__modal_dlg"));
        modal_widg.createWinId();
	QApplicationPrivate::enterModal(&modal_widg);
        QApplicationPrivate::native_modal_dialog_active = true;
        Boolean accepted;
        PMSessionPageSetupDialog(ep->session, ep->format, &accepted);
	QApplicationPrivate::leaveModal(&modal_widg);
        QApplicationPrivate::native_modal_dialog_active = false;
        pageSetupDialogSheetDoneCallback(ep->session, 0, accepted);
    } else {
        // Window Modal means that we use a sheet at the moment, there's no other way to do it correctly.
        if (!upp)
            upp = NewPMSheetDoneUPP(QPageSetupDialogPrivate::pageSetupDialogSheetDoneCallback);
        PMSessionUseSheets(ep->session, qt_mac_window_for(q->parentWidget()), upp);
        Boolean unused;
        PMSessionPageSetupDialog(ep->session, ep->format, &unused);
    }
}

void QPageSetupDialogPrivate::closeCarbonPageLayout()
{
    // if the margins have changed, we have to use the margins from the new
    // PMFormat object
    if (q_func()->result() == QDialog::Accepted) {
        PMPaper paper;
        PMPaperMargins margins;
        PMGetPageFormatPaper(ep->format, &paper);
        PMPaperGetMargins(paper, &margins);
        ep->leftMargin = margins.left;
        ep->topMargin = margins.top;
        ep->rightMargin = margins.right;
        ep->bottomMargin = margins.bottom;

	PMRect paperRect;
	PMGetUnadjustedPaperRect(ep->format, &paperRect);
	ep->customSize = QSizeF(paperRect.right - paperRect.left,
				paperRect.bottom - paperRect.top);
    }
    sheetCallbackMap.remove(ep->session);
}
#else
void QPageSetupDialogPrivate::openCocoaPageLayout(Qt::WindowModality modality)
{
    Q_Q(QPageSetupDialog);

    // If someone is reusing a QPrinter object, the end released all our old
    // information. In this case, we must reinitialize.
    if (ep->session == 0)
        ep->initialize();

    macStartInterceptWindowTitle(q);
    pageLayout = [NSPageLayout pageLayout];
    // Keep a copy to this since we plan on using it for a bit.
    [pageLayout retain];
    QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) *delegate = [[QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) alloc] initWithMacPrintEngine:ep];

    if (modality == Qt::ApplicationModal) {
        int rval = [pageLayout runModalWithPrintInfo:ep->printInfo];
        [delegate pageLayoutDidEnd:pageLayout returnCode:rval contextInfo:q];
    } else {
        Q_ASSERT(q->parentWidget());
        [pageLayout beginSheetWithPrintInfo:ep->printInfo
                             modalForWindow:qt_mac_window_for(q->parentWidget())
                                   delegate:delegate
                             didEndSelector:@selector(pageLayoutDidEnd:returnCode:contextInfo:)
                                contextInfo:q];
    }

    macStopInterceptWindowTitle();
}

void QPageSetupDialogPrivate::closeCocoaPageLayout()
{
    // NSPageLayout can change the session behind our back and then our
    // d->ep->session object will become a dangling pointer. Update it
    // based on the "current" session
    ep->session = static_cast<PMPrintSession>([ep->printInfo PMPrintSession]);

    [pageLayout release];
    pageLayout = 0;
}
#endif

QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QAbstractPageSetupDialog(*(new QPageSetupDialogPrivate), printer, parent)
{
    Q_D(QPageSetupDialog);
    d->ep = static_cast<QMacPrintEngine *>(d->printer->paintEngine())->d_func();
}

QPageSetupDialog::QPageSetupDialog(QWidget *parent)
    : QAbstractPageSetupDialog(*(new QPageSetupDialogPrivate), 0, parent)
{
    Q_D(QPageSetupDialog);
    d->ep = static_cast<QMacPrintEngine *>(d->printer->paintEngine())->d_func();
}

void QPageSetupDialog::setVisible(bool visible)
{
    Q_D(QPageSetupDialog);

    if (d->printer->outputFormat() != QPrinter::NativeFormat)
        return;

#ifndef QT_MAC_USE_COCOA
    bool isCurrentlyVisible = d->sheetCallbackMap.contains(d->ep->session);
#else
    bool isCurrentlyVisible = (d->pageLayout != 0);
#endif
    if (!visible == !isCurrentlyVisible)
        return;

    if (visible) {
#ifndef QT_MAC_USE_COCOA
        d->openCarbonPageLayout(parentWidget() ? Qt::WindowModal
                                               : Qt::ApplicationModal);
#else
        d->openCocoaPageLayout(parentWidget() ? Qt::WindowModal
                                              : Qt::ApplicationModal);
#endif
        return;
    } else {
#ifndef QT_MAC_USE_COCOA
        d->closeCarbonPageLayout();
#else
        if (d->pageLayout) {
            d->closeCocoaPageLayout();
            return;
        }
#endif
    }
}

int QPageSetupDialog::exec()
{
    Q_D(QPageSetupDialog);

    if (d->printer->outputFormat() != QPrinter::NativeFormat)
        return Rejected;

#ifndef QT_MAC_USE_COCOA
    d->openCarbonPageLayout(Qt::ApplicationModal);
    d->closeCarbonPageLayout();
#else
    QMacCocoaAutoReleasePool pool;
    d->openCocoaPageLayout(Qt::ApplicationModal);
    d->closeCocoaPageLayout();
#endif
    return result();
}

QT_END_NAMESPACE

#endif /* QT_NO_PRINTDIALOG */

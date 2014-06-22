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

#include "qpagesetupdialog.h"

#ifndef QT_NO_PRINTDIALOG
#include <qapplication.h>

#include "../kernel/qprintengine_win_p.h"
#include "qpagesetupdialog_p.h"
#include "qprinter.h"
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QDialog(*(new QPageSetupDialogPrivate(printer)), parent)
{
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    setAttribute(Qt::WA_DontShowOnScreen);
}

QPageSetupDialog::QPageSetupDialog(QWidget *parent)
    : QDialog(*(new QPageSetupDialogPrivate(0)), parent)
{
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    setAttribute(Qt::WA_DontShowOnScreen);
}

int QPageSetupDialog::exec()
{
    Q_D(QPageSetupDialog);

    if (d->printer->outputFormat() != QPrinter::NativeFormat)
        return Rejected;

    QWin32PrintEngine *engine = static_cast<QWin32PrintEngine*>(d->printer->paintEngine());
    QWin32PrintEnginePrivate *ep = static_cast<QWin32PrintEnginePrivate *>(engine->d_ptr.data());

    PAGESETUPDLG psd;
    memset(&psd, 0, sizeof(PAGESETUPDLG));
    psd.lStructSize = sizeof(PAGESETUPDLG);

    // we need a temp DEVMODE struct if we don't have a global DEVMODE
    HGLOBAL hDevMode = 0;
    int devModeSize = 0;
    if (!engine->globalDevMode()) {
        devModeSize = sizeof(DEVMODE) + ep->devMode->dmDriverExtra;
        hDevMode = GlobalAlloc(GHND, devModeSize);
        if (hDevMode) {
            void *dest = GlobalLock(hDevMode);
            memcpy(dest, ep->devMode, devModeSize);
            GlobalUnlock(hDevMode);
        }
        psd.hDevMode = hDevMode;
    } else {
        psd.hDevMode = engine->globalDevMode();
    }

    HGLOBAL *tempDevNames = engine->createGlobalDevNames();
    psd.hDevNames = tempDevNames;

    QWidget *parent = parentWidget();
    parent = parent ? parent->window() : QApplication::activeWindow();
    Q_ASSERT(!parent ||parent->testAttribute(Qt::WA_WState_Created));

    QWindow *parentWindow = parent->windowHandle();
    psd.hwndOwner = parentWindow ? (HWND)QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", parentWindow) : 0;

    psd.Flags = PSD_MARGINS;
    QPageLayout layout = d->printer->pageLayout();
    switch (layout.units()) {
    case QPageLayout::Millimeter:
    case QPageLayout::Inch:
        break;
    case QPageLayout::Point:
    case QPageLayout::Pica:
    case QPageLayout::Didot:
    case QPageLayout::Cicero:
        layout.setUnits(QLocale::system().measurementSystem() == QLocale::MetricSystem ? QPageLayout::Millimeter
                                                                                       : QPageLayout::Inch);
        break;
    }
    qreal multiplier = 1.0;
    if (layout.units() == QPageLayout::Millimeter) {
        psd.Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
        multiplier = 100.0;
    } else { // QPageLayout::Inch)
        psd.Flags |= PSD_INTHOUSANDTHSOFINCHES;
        multiplier = 1000.0;
    }
    psd.rtMargin.left   = layout.margins().left() * multiplier;
    psd.rtMargin.top    = layout.margins().top() * multiplier;
    psd.rtMargin.right  = layout.margins().right() * multiplier;
    psd.rtMargin.bottom = layout.margins().bottom() * multiplier;

    QDialog::setVisible(true);
    bool result = PageSetupDlg(&psd);
    QDialog::setVisible(false);
    if (result) {
        engine->setGlobalDevMode(psd.hDevNames, psd.hDevMode);
        d->printer->setPageMargins(QMarginsF(psd.rtMargin.left / multiplier, psd.rtMargin.right / multiplier,
                                             psd.rtMargin.top / multiplier, psd.rtMargin.bottom / multiplier),
                                   layout.units());

        // copy from our temp DEVMODE struct
        if (!engine->globalDevMode() && hDevMode) {
            void *src = GlobalLock(hDevMode);
            memcpy(ep->devMode, src, devModeSize);
            GlobalUnlock(hDevMode);
        }
    }

    if (!engine->globalDevMode() && hDevMode)
        GlobalFree(hDevMode);
    GlobalFree(tempDevNames);
    done(result);
    return result;
}

void QPageSetupDialog::setVisible(bool visible)
{
    if (!visible)
        return;
    exec();
}

QT_END_NAMESPACE
#endif

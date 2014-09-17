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

#include "qabstractpagesetupdialog.h"
#include "qabstractpagesetupdialog_p.h"

#ifndef QT_NO_PRINTDIALOG

#include <QtCore/qcoreapplication.h>
#include <QtGui/qprinter.h>

QT_BEGIN_NAMESPACE

/*!
    \internal
    \class QAbstractPageSetupDialog

    \brief The QAbstractPageSetupDialog class provides a base for
    implementations of page setup dialogs.
*/

/*!
    Constructs the page setup dialog for the printer \a printer with
    \a parent as parent widget.
*/
QAbstractPageSetupDialog::QAbstractPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QDialog(*(new QAbstractPageSetupDialogPrivate), parent)
{
    Q_D(QAbstractPageSetupDialog);
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    d->setPrinter(printer);
}

/*!
    \internal
*/
QAbstractPageSetupDialog::QAbstractPageSetupDialog(QAbstractPageSetupDialogPrivate &ptr,
                                                   QPrinter *printer, QWidget *parent)
    : QDialog(ptr, parent)
{
    Q_D(QAbstractPageSetupDialog);
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    d->setPrinter(printer);
}

QAbstractPageSetupDialog::~QAbstractPageSetupDialog()
{
    Q_D(QAbstractPageSetupDialog);
    if (d->opts & QPageSetupDialog::OwnsPrinter)
        delete d->printer;
}

/*!
    Returns the printer that this page setup dialog is operating on.
*/
QPrinter *QAbstractPageSetupDialog::printer()
{
    Q_D(QAbstractPageSetupDialog);
    return d->printer;
}

void QAbstractPageSetupDialogPrivate::setPrinter(QPrinter *newPrinter)
{
    if (newPrinter) {
        printer = newPrinter;
    } else {
        printer = new QPrinter;
        opts |= QPageSetupDialog::OwnsPrinter;
    }
#ifndef Q_WS_X11
    if (printer->outputFormat() != QPrinter::NativeFormat)
        qWarning("QPageSetupDialog: Cannot be used on non-native printers");
#endif
}

/*!
    \fn int QAbstractPageSetupDialog::exec()

    This virtual function is called to pop up the dialog. It must be
    reimplemented in subclasses.
*/

/*!
    \reimp
*/
void QAbstractPageSetupDialog::done(int result)
{
    Q_D(QAbstractPageSetupDialog);
    QDialog::done(result);
    if (d->receiverToDisconnectOnClose) {
        disconnect(this, SIGNAL(accepted()),
                   d->receiverToDisconnectOnClose, d->memberToDisconnectOnClose);
        d->receiverToDisconnectOnClose = 0;
    }
    d->memberToDisconnectOnClose.clear();

}

QT_END_NAMESPACE

#endif // QT_NO_PRINTDIALOG

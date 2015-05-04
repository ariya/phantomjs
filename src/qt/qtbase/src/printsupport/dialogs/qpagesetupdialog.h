/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPAGESETUPDIALOG_H
#define QPAGESETUPDIALOG_H

#include <QtWidgets/qdialog.h>
#include <QtPrintSupport/qtprintsupportglobal.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_PRINTDIALOG

class QPrinter;
class QPageSetupDialogPrivate;

class Q_PRINTSUPPORT_EXPORT QPageSetupDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPageSetupDialog)

public:
    explicit QPageSetupDialog(QPrinter *printer, QWidget *parent = 0);
    explicit QPageSetupDialog(QWidget *parent = 0);
    virtual ~QPageSetupDialog();

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    virtual void setVisible(bool visible);
#endif
    virtual int exec();

#ifdef Q_NO_USING_KEYWORD
#ifndef Q_QDOC
    void open() { QDialog::open(); }
#endif
#else
    using QDialog::open;
#endif
    void open(QObject *receiver, const char *member);

    void done(int result);

    QPrinter *printer();
};

#endif // QT_NO_PRINTDIALOG

QT_END_NAMESPACE

#endif // QPAGESETUPDIALOG_H

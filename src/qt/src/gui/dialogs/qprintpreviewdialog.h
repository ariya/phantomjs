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

#ifndef QPRINTPREVIEWDIALOG_H
#define QPRINTPREVIEWDIALOG_H

#include <QtGui/qdialog.h>

#ifndef QT_NO_PRINTPREVIEWDIALOG

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QGraphicsView;
class QPrintPreviewDialogPrivate;

class Q_GUI_EXPORT QPrintPreviewDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPrintPreviewDialog)

public:
    explicit QPrintPreviewDialog(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    explicit QPrintPreviewDialog(QPrinter *printer, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~QPrintPreviewDialog();

#ifdef Q_NO_USING_KEYWORD
#ifndef Q_QDOC
    void open() { QDialog::open(); }
#endif
#else
    using QDialog::open;
#endif
    void open(QObject *receiver, const char *member);

    QPrinter *printer();

    void setVisible(bool visible);
    void done(int result);

Q_SIGNALS:
    void paintRequested(QPrinter *printer);

private:
    Q_PRIVATE_SLOT(d_func(), void _q_fit(QAction *action))
    Q_PRIVATE_SLOT(d_func(), void _q_zoomIn())
    Q_PRIVATE_SLOT(d_func(), void _q_zoomOut())
    Q_PRIVATE_SLOT(d_func(), void _q_navigate(QAction *action))
    Q_PRIVATE_SLOT(d_func(), void _q_setMode(QAction *action))
    Q_PRIVATE_SLOT(d_func(), void _q_pageNumEdited())
    Q_PRIVATE_SLOT(d_func(), void _q_print())
    Q_PRIVATE_SLOT(d_func(), void _q_pageSetup())
    Q_PRIVATE_SLOT(d_func(), void _q_previewChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_zoomFactorChanged())

    void *dummy; // ### Qt 5 - remove me
};


QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_PRINTPREVIEWDIALOG

#endif // QPRINTPREVIEWDIALOG_H

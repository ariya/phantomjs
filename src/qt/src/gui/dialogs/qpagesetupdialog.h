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

#ifndef QPAGESETUPDIALOG_H
#define QPAGESETUPDIALOG_H

#include <QtGui/qabstractpagesetupdialog.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_PRINTDIALOG

class QPageSetupDialogPrivate;

class Q_GUI_EXPORT QPageSetupDialog : public QAbstractPageSetupDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPageSetupDialog)
    Q_ENUMS(PageSetupDialogOption)
    Q_PROPERTY(PageSetupDialogOptions options READ options WRITE setOptions)

public:
    enum PageSetupDialogOption {
        None                    = 0x00000000, // internal
        DontUseSheet            = 0x00000001,
        OwnsPrinter             = 0x80000000  // internal
    };

    Q_DECLARE_FLAGS(PageSetupDialogOptions, PageSetupDialogOption)

    explicit QPageSetupDialog(QPrinter *printer, QWidget *parent = 0);
    explicit QPageSetupDialog(QWidget *parent = 0);

    // obsolete
    void addEnabledOption(PageSetupDialogOption option);
    void setEnabledOptions(PageSetupDialogOptions options);
    PageSetupDialogOptions enabledOptions() const;
    bool isOptionEnabled(PageSetupDialogOption option) const;

    void setOption(PageSetupDialogOption option, bool on = true);
    bool testOption(PageSetupDialogOption option) const;
    void setOptions(PageSetupDialogOptions options);
    PageSetupDialogOptions options() const;

#if defined(Q_WS_MAC) || defined(Q_OS_WIN)
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

#ifdef qdoc
    QPrinter *printer();
#endif
};

#endif // QT_NO_PRINTDIALOG

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPAGESETUPDIALOG_H

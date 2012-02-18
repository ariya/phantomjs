/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QPRINTDIALOG_H
#define QPRINTDIALOG_H

#include <QtGui/qabstractprintdialog.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_PRINTDIALOG

class QPrintDialogPrivate;
class QPushButton;
class QPrinter;

#if defined (Q_OS_UNIX) && !defined(QTOPIA_PRINTDIALOG) && !defined(Q_WS_MAC) && !defined(Q_OS_SYMBIAN)
class QUnixPrintWidgetPrivate;

class Q_GUI_EXPORT QUnixPrintWidget : public QWidget
{
    Q_OBJECT

public:
    QUnixPrintWidget(QPrinter *printer, QWidget *parent = 0);
    ~QUnixPrintWidget();
    void updatePrinter();

private:
    friend class QPrintDialogPrivate;
    friend class QUnixPrintWidgetPrivate;
    QUnixPrintWidgetPrivate *d;
    Q_PRIVATE_SLOT(d, void _q_printerChanged(int))
    Q_PRIVATE_SLOT(d, void _q_btnBrowseClicked())
    Q_PRIVATE_SLOT(d, void _q_btnPropertiesClicked())
};
#endif

class Q_GUI_EXPORT QPrintDialog : public QAbstractPrintDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPrintDialog)
    Q_ENUMS(PrintDialogOption)
    Q_PROPERTY(PrintDialogOptions options READ options WRITE setOptions)

public:
    explicit QPrintDialog(QPrinter *printer, QWidget *parent = 0);
    explicit QPrintDialog(QWidget *parent = 0);
    ~QPrintDialog();

    int exec();
#if defined (Q_OS_UNIX) && !defined(QTOPIA_PRINTDIALOG) && !defined(Q_WS_MAC)
    virtual void accept();
#endif
    void done(int result);

#if defined (Q_OS_UNIX) && defined (QT3_SUPPORT)
    QT3_SUPPORT void setPrinter(QPrinter *, bool = false);
    QT3_SUPPORT QPrinter *printer() const;
    QT3_SUPPORT void addButton(QPushButton *button);
#endif

    void setOption(PrintDialogOption option, bool on = true);
    bool testOption(PrintDialogOption option) const;
    void setOptions(PrintDialogOptions options);
    PrintDialogOptions options() const;

#if defined(Q_OS_UNIX) || defined(Q_WS_MAC) || defined(Q_OS_WIN)
    void setVisible(bool visible);
#endif

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

#ifdef QTOPIA_PRINTDIALOG
public:
    bool eventFilter(QObject *, QEvent *);
#endif

#ifdef Q_NO_USING_KEYWORD
#ifndef Q_QDOC
    void accepted() { QDialog::accepted(); }
#endif
#else
    using QDialog::accepted;
#endif

Q_SIGNALS:
    void accepted(QPrinter *printer);

private:
#ifndef QTOPIA_PRINTDIALOG
    Q_PRIVATE_SLOT(d_func(), void _q_chbPrintLastFirstToggled(bool))
#if defined (Q_OS_UNIX) && !defined (Q_OS_MAC)
    Q_PRIVATE_SLOT(d_func(), void _q_collapseOrExpandDialog())
#endif
# if defined(Q_OS_UNIX) && !defined (Q_OS_MAC) && !defined(QT_NO_MESSAGEBOX)
    Q_PRIVATE_SLOT(d_func(), void _q_checkFields())
# endif
#else // QTOPIA_PRINTDIALOG
    Q_PRIVATE_SLOT(d_func(), void _q_okClicked())
    Q_PRIVATE_SLOT(d_func(),void _q_printerOrFileSelected(QAbstractButton *b))
    Q_PRIVATE_SLOT(d_func(),void _q_paperSizeSelected(int))
    Q_PRIVATE_SLOT(d_func(), void _q_orientSelected(int))
    Q_PRIVATE_SLOT(d_func(), void _q_pageOrderSelected(int))
    Q_PRIVATE_SLOT(d_func(), void _q_colorModeSelected(QAbstractButton *))
    Q_PRIVATE_SLOT(d_func(), void _q_setNumCopies(int))
    Q_PRIVATE_SLOT(d_func(), void _q_printRangeSelected(int))
    Q_PRIVATE_SLOT(d_func(), void _q_setFirstPage(int))
    Q_PRIVATE_SLOT(d_func(), void _q_setLastPage(int))
    Q_PRIVATE_SLOT(d_func(), void _q_fileNameEditChanged(const QString &text))
#endif // QTOPIA_PRINTDIALOG
    friend class QUnixPrintWidget;
};

#endif // QT_NO_PRINTDIALOG

QT_END_NAMESPACE

QT_END_HEADER

#endif // QPRINTDIALOG_H

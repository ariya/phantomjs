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

#ifndef QDIALOG_H
#define QDIALOG_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QPushButton;
class QDialogPrivate;

class Q_GUI_EXPORT QDialog : public QWidget
{
    Q_OBJECT
    friend class QPushButton;

    Q_PROPERTY(bool sizeGripEnabled READ isSizeGripEnabled WRITE setSizeGripEnabled)
    Q_PROPERTY(bool modal READ isModal WRITE setModal)

public:
    explicit QDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
#ifdef QT3_SUPPORT
    QT3_SUPPORT_CONSTRUCTOR QDialog(QWidget *parent, const char *name, bool modal = false,
                                  Qt::WindowFlags f = 0);
#endif
    ~QDialog();

    enum DialogCode { Rejected, Accepted };

    int result() const;

    void setVisible(bool visible);

    void setOrientation(Qt::Orientation orientation);
    Qt::Orientation orientation() const;

    void setExtension(QWidget* extension);
    QWidget* extension() const;

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    void setSizeGripEnabled(bool);
    bool isSizeGripEnabled() const;

    void setModal(bool modal);
    void setResult(int r);

Q_SIGNALS:
    void finished(int result);
    void accepted();
    void rejected();

public Q_SLOTS:
    void open();
    int exec();
    virtual void done(int);
    virtual void accept();
    virtual void reject();

    void showExtension(bool);

protected:
    QDialog(QDialogPrivate &, QWidget *parent, Qt::WindowFlags f = 0);

#if defined(Q_WS_WINCE) || defined(Q_OS_SYMBIAN)
    bool event(QEvent *e);
#endif
    void keyPressEvent(QKeyEvent *);
    void closeEvent(QCloseEvent *);
    void showEvent(QShowEvent *);
    void resizeEvent(QResizeEvent *);
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *);
#endif
    bool eventFilter(QObject *, QEvent *);
    void adjustPosition(QWidget*);
private:
    Q_DECLARE_PRIVATE(QDialog)
    Q_DISABLE_COPY(QDialog)

#if defined(Q_OS_SYMBIAN)
    bool symbianAdjustedPosition();
#endif


#ifdef Q_WS_WINCE_WM
    Q_PRIVATE_SLOT(d_func(), void _q_doneAction())
#endif
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDIALOG_H

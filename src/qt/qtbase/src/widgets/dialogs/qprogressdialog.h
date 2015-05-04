/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QPROGRESSDIALOG_H
#define QPROGRESSDIALOG_H

#include <QtWidgets/qdialog.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_PROGRESSDIALOG

class QPushButton;
class QLabel;
class QProgressBar;
class QTimer;
class QProgressDialogPrivate;

class Q_WIDGETS_EXPORT QProgressDialog : public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QProgressDialog)
    Q_PROPERTY(bool wasCanceled READ wasCanceled)
    Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(bool autoReset READ autoReset WRITE setAutoReset)
    Q_PROPERTY(bool autoClose READ autoClose WRITE setAutoClose)
    Q_PROPERTY(int minimumDuration READ minimumDuration WRITE setMinimumDuration)
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText)

public:
    explicit QProgressDialog(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    QProgressDialog(const QString &labelText, const QString &cancelButtonText,
                    int minimum, int maximum, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~QProgressDialog();

    void setLabel(QLabel *label);
    void setCancelButton(QPushButton *button);
    void setBar(QProgressBar *bar);

    bool wasCanceled() const;

    int minimum() const;
    int maximum() const;

    int value() const;

    QSize sizeHint() const;

    QString labelText() const;
    int minimumDuration() const;

    void setAutoReset(bool reset);
    bool autoReset() const;
    void setAutoClose(bool close);
    bool autoClose() const;

#ifdef Q_NO_USING_KEYWORD
#ifndef Q_QDOC
    void open() { QDialog::open(); }
#endif
#else
    using QDialog::open;
#endif
    void open(QObject *receiver, const char *member);

public Q_SLOTS:
    void cancel();
    void reset();
    void setMaximum(int maximum);
    void setMinimum(int minimum);
    void setRange(int minimum, int maximum);
    void setValue(int progress);
    void setLabelText(const QString &text);
    void setCancelButtonText(const QString &text);
    void setMinimumDuration(int ms);

Q_SIGNALS:
    void canceled();

protected:
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);
    void changeEvent(QEvent *event);
    void showEvent(QShowEvent *event);

protected Q_SLOTS:
    void forceShow();

private:
    Q_DISABLE_COPY(QProgressDialog)

    Q_PRIVATE_SLOT(d_func(), void _q_disconnectOnClose())
};

#endif // QT_NO_PROGRESSDIALOG

QT_END_NAMESPACE

#endif // QPROGRESSDIALOG_H

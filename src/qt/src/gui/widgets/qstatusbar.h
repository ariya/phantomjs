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

#ifndef QSTATUSBAR_H
#define QSTATUSBAR_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_STATUSBAR

class QStatusBarPrivate;

class Q_GUI_EXPORT QStatusBar: public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool sizeGripEnabled READ isSizeGripEnabled WRITE setSizeGripEnabled)

public:
    explicit QStatusBar(QWidget* parent=0);
    virtual ~QStatusBar();

    void addWidget(QWidget *widget, int stretch = 0);
    int insertWidget(int index, QWidget *widget, int stretch = 0);
    void addPermanentWidget(QWidget *widget, int stretch = 0);
    int insertPermanentWidget(int index, QWidget *widget, int stretch = 0);
    void removeWidget(QWidget *widget);

    void setSizeGripEnabled(bool);
    bool isSizeGripEnabled() const;

    QString currentMessage() const;

public Q_SLOTS:
    void showMessage(const QString &text, int timeout = 0);
    void clearMessage();

#ifdef QT3_SUPPORT
public:
    QT3_SUPPORT_CONSTRUCTOR QStatusBar(QWidget* parent, const char* name);
    QT3_SUPPORT void addWidget(QWidget *w, int stretch, bool permanent)
        { if (permanent) addPermanentWidget(w, stretch); else addWidget(w, stretch); }
public Q_SLOTS:
    inline QT_MOC_COMPAT void message(const QString &text, int timeout = 0) { showMessage(text, timeout); }
    inline QT_MOC_COMPAT void clear() { clearMessage(); }
#endif

Q_SIGNALS:
    void messageChanged(const QString &text);

protected:
    void showEvent(QShowEvent *);
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);

    // ### Qt 5: consider making reformat() and hideOrShow() private
    void reformat();
    void hideOrShow();
    bool event(QEvent *);

private:
    Q_DISABLE_COPY(QStatusBar)
    Q_DECLARE_PRIVATE(QStatusBar)
};

#endif // QT_NO_STATUSBAR

QT_END_NAMESPACE

QT_END_HEADER

#endif // QSTATUSBAR_H

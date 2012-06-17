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

#ifndef QCOMMANDLINKBUTTON_H
#define QCOMMANDLINKBUTTON_H

#include <QtGui/qpushbutton.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QCommandLinkButtonPrivate;

class Q_GUI_EXPORT QCommandLinkButton: public QPushButton 
{
    Q_OBJECT

    Q_PROPERTY(QString description READ description WRITE setDescription)
    Q_PROPERTY(bool flat READ isFlat WRITE setFlat DESIGNABLE false)

public:
    explicit QCommandLinkButton(QWidget *parent=0);
    explicit QCommandLinkButton(const QString &text, QWidget *parent=0);
    QCommandLinkButton(const QString &text, const QString &description, QWidget *parent=0);
    QString description() const;
    void setDescription(const QString &description);

protected:
    QSize sizeHint() const;
    int heightForWidth(int) const;
    QSize minimumSizeHint() const;
    bool event(QEvent *e);
    void paintEvent(QPaintEvent *);

private:
    Q_DISABLE_COPY(QCommandLinkButton)
    Q_DECLARE_PRIVATE(QCommandLinkButton)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QCOMMANDLINKBUTTON

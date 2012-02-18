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

#ifndef QERRORMESSAGE_H
#define QERRORMESSAGE_H

#include <QtGui/qdialog.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_ERRORMESSAGE

class QErrorMessagePrivate;

class Q_GUI_EXPORT QErrorMessage: public QDialog
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QErrorMessage)
public:
    explicit QErrorMessage(QWidget* parent = 0);
    ~QErrorMessage();

    static QErrorMessage * qtHandler();

public Q_SLOTS:
    void showMessage(const QString &message);
    void showMessage(const QString &message, const QString &type);
#ifdef QT3_SUPPORT
    inline QT_MOC_COMPAT void message(const QString &text) { showMessage(text); }
#endif

protected:
    void done(int);
    void changeEvent(QEvent *e);

private:
    Q_DISABLE_COPY(QErrorMessage)
};

#endif // QT_NO_ERRORMESSAGE

QT_END_NAMESPACE

QT_END_HEADER

#endif // QERRORMESSAGE_H

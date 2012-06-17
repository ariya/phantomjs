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

#ifndef QSOFTKEYMANAGER_P_H
#define QSOFTKEYMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include "QtGui/qaction.h"

QT_BEGIN_HEADER

#ifndef QT_NO_SOFTKEYMANAGER
QT_BEGIN_NAMESPACE

class QSoftKeyManagerPrivate;

class Q_AUTOTEST_EXPORT QSoftKeyManager : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSoftKeyManager)

public:

    enum StandardSoftKey {
        OkSoftKey,
        SelectSoftKey,
        DoneSoftKey,
        MenuSoftKey,
        CancelSoftKey
    };

    static void updateSoftKeys();
#ifdef Q_WS_S60
    static bool handleCommand(int);
#endif

    static QAction *createAction(StandardSoftKey standardKey, QWidget *actionWidget);
    static QAction *createKeyedAction(StandardSoftKey standardKey, Qt::Key key, QWidget *actionWidget);
    static QString standardSoftKeyText(StandardSoftKey standardKey);
    static void setForceEnabledInSoftkeys(QAction *action);
    static bool isForceEnabledInSofkeys(QAction *action);

protected:
    bool event(QEvent *e);

private:
    QSoftKeyManager();
    static QSoftKeyManager *instance();
    bool appendSoftkeys(const QWidget &source, int level);
    QWidget *softkeySource(QWidget *previousSource, bool& recursiveMerging);
    bool handleUpdateSoftKeys();

private Q_SLOTS:
    void cleanupHash(QObject* obj);
    void sendKeyEvent();

private:
    Q_DISABLE_COPY(QSoftKeyManager)
};

QT_END_NAMESPACE
#endif //QT_NO_SOFTKEYMANAGER

QT_END_HEADER

#endif //QSOFTKEYMANAGER_P_H

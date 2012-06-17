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

#ifndef QSOFTKEYMANAGER_S60_P_H
#define QSOFTKEYMANAGER_S60_P_H

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

#include "qbitarray.h"
#include "private/qobject_p.h"
#include "private/qsoftkeymanager_common_p.h"

QT_BEGIN_HEADER

#ifndef QT_NO_SOFTKEYMANAGER

QT_BEGIN_NAMESPACE

class CEikButtonGroupContainer;
class QAction;

class QSoftKeyManagerPrivateS60 : public QSoftKeyManagerPrivate
{
    Q_DECLARE_PUBLIC(QSoftKeyManager)

public:
    QSoftKeyManagerPrivateS60();

public:
    void updateSoftKeys_sys();
    bool handleCommand(int command);

private:
    void tryDisplayMenuBarL();
    bool skipCbaUpdate();
    void ensureCbaVisibilityAndResponsiviness(CEikButtonGroupContainer &cba);
    void clearSoftkeys(CEikButtonGroupContainer &cba);
    QString softkeyText(QAction &softkeyAction);
    QAction *highestPrioritySoftkey(QAction::SoftKeyRole role);
    static bool actionPriorityMoreThan(const QAction* item1, const QAction* item2);
    void setNativeSoftkey(CEikButtonGroupContainer &cba, TInt position, TInt command, const TDesC& text);
    QPoint softkeyIconPosition(int position, QSize sourceSize, QSize targetSize);
    QPixmap prepareSoftkeyPixmap(QPixmap src, int position, QSize targetSize);
    bool isOrientationLandscape();
    QSize cbaIconSize(CEikButtonGroupContainer *cba, int position);
    bool setSoftkeyImage(CEikButtonGroupContainer *cba, QAction &action, int position);
    bool setSoftkey(CEikButtonGroupContainer &cba, QAction::SoftKeyRole role, int position);
    bool setLeftSoftkey(CEikButtonGroupContainer &cba);
    bool setMiddleSoftkey(CEikButtonGroupContainer &cba);
    bool setRightSoftkey(CEikButtonGroupContainer &cba);
    void setSoftkeys(CEikButtonGroupContainer &cba);

private:
    QHash<int, QAction*> realSoftKeyActions;
    QSize cachedCbaIconSize[4];
    QBitArray cbaHasImage;
};


QT_END_NAMESPACE

#endif //QT_NO_SOFTKEYMANAGER

QT_END_HEADER

#endif // QSOFTKEYMANAGER_S60_P_H

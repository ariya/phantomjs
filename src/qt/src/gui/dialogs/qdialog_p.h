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

#ifndef QDIALOG_P_H
#define QDIALOG_P_H

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

#include "private/qwidget_p.h"
#include "QtCore/qeventloop.h"
#include "QtCore/qpointer.h"
#include "QtGui/qdialog.h"
#include "QtGui/qpushbutton.h"

QT_BEGIN_NAMESPACE

class QSizeGrip;

class QDialogPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QDialog)
public:

    QDialogPrivate()
        : mainDef(0), orientation(Qt::Horizontal),extension(0), doShowExtension(false),
#ifndef QT_NO_SIZEGRIP
          resizer(0),
          sizeGripEnabled(false),
#endif
          rescode(0), resetModalityTo(-1), wasModalitySet(true), eventLoop(0)
        {}

    QPointer<QPushButton> mainDef;
    Qt::Orientation orientation;
    QWidget *extension;
    bool doShowExtension;
    QSize size, min, max;
#ifndef QT_NO_SIZEGRIP
    QSizeGrip *resizer;
    bool sizeGripEnabled;
#endif
    QPoint lastRMBPress;

    void setDefault(QPushButton *);
    void setMainDefault(QPushButton *);
    void hideDefault();
    void resetModalitySetByOpen();

#ifdef Q_WS_WINCE_WM
    void _q_doneAction();
#endif

#ifdef Q_WS_MAC
    virtual void mac_nativeDialogModalHelp() {}
#endif

    int rescode;
    int resetModalityTo;
    bool wasModalitySet;

    QPointer<QEventLoop> eventLoop;
};

QT_END_NAMESPACE

#endif // QDIALOG_P_H

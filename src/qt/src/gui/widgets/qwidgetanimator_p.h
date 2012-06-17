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

#ifndef QWIDGET_ANIMATOR_P_H
#define QWIDGET_ANIMATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <qobject.h>
#include <qmap.h>

QT_BEGIN_NAMESPACE

class QWidget;
class QMainWindowLayout;
class QPropertyAnimation;
class QRect;

class QWidgetAnimator : public QObject
{
    Q_OBJECT
public:
    QWidgetAnimator(QMainWindowLayout *layout);
    void animate(QWidget *widget, const QRect &final_geometry, bool animate);
    bool animating() const;

    void abort(QWidget *widget);

#ifndef QT_NO_ANIMATION
private Q_SLOTS:
    void animationFinished();
#endif

private:
    typedef QMap<QWidget*, QPropertyAnimation*> AnimationMap;
    AnimationMap m_animation_map;
    QMainWindowLayout *m_mainWindowLayout;
};

QT_END_NAMESPACE

#endif // QWIDGET_ANIMATOR_P_H

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

#ifndef QPAINTENGINE_P_H
#define QPAINTENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtGui/qpainter.h"
#include "QtGui/qpaintengine.h"
#include "QtGui/qregion.h"
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

class QPaintDevice;

class QPaintEnginePrivate
{
    Q_DECLARE_PUBLIC(QPaintEngine)
public:
    QPaintEnginePrivate() : pdev(0), q_ptr(0), currentClipWidget(0), hasSystemTransform(0),
                            hasSystemViewport(0) {}
    virtual ~QPaintEnginePrivate() { }
    QPaintDevice *pdev;
    QPaintEngine *q_ptr;
    QRegion systemClip;
    QRect systemRect;
    QRegion systemViewport;
    QTransform systemTransform;
    QWidget *currentClipWidget;
    uint hasSystemTransform : 1;
    uint hasSystemViewport : 1;

    inline void transformSystemClip()
    {
        if (systemClip.isEmpty())
            return;

        if (hasSystemTransform) {
            if (systemTransform.type() <= QTransform::TxTranslate)
                systemClip.translate(qRound(systemTransform.dx()), qRound(systemTransform.dy()));
            else
                systemClip = systemTransform.map(systemClip);
        }

        // Make sure we're inside the viewport.
        if (hasSystemViewport) {
            systemClip &= systemViewport;
            if (systemClip.isEmpty()) {
                // We don't want to paint without system clip, so set it to 1 pixel :)
                systemClip = QRect(systemViewport.boundingRect().topLeft(), QSize(1, 1));
            }
        }
    }

    inline void setSystemTransform(const QTransform &xform)
    {
        systemTransform = xform;
        if ((hasSystemTransform = !xform.isIdentity()) || hasSystemViewport)
            transformSystemClip();
        systemStateChanged();
    }

    inline void setSystemViewport(const QRegion &region)
    {
        systemViewport = region;
        hasSystemViewport = !systemViewport.isEmpty();
    }

    virtual void systemStateChanged() { }

    void drawBoxTextItem(const QPointF &p, const QTextItemInt &ti);
};

QT_END_NAMESPACE

#endif // QPAINTENGINE_P_H

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

#ifndef QLAYOUTENGINE_P_H
#define QLAYOUTENGINE_P_H

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

#include "QtGui/qlayoutitem.h"
#include "QtGui/qstyle.h"

QT_BEGIN_NAMESPACE

template <typename T> class QVector;

struct QLayoutStruct
{
    inline void init(int stretchFactor = 0, int minSize = 0) {
        stretch = stretchFactor;
        minimumSize = sizeHint = minSize;
        maximumSize = QLAYOUTSIZE_MAX;
        expansive = false;
        empty = true;
        spacing = 0;
    }

    int smartSizeHint() {
        return (stretch > 0) ? minimumSize : sizeHint;
    }
    int effectiveSpacer(int uniformSpacer) const {
        Q_ASSERT(uniformSpacer >= 0 || spacing >= 0);
        return (uniformSpacer >= 0) ? uniformSpacer : spacing;
    }

    // parameters
    int stretch;
    int sizeHint;
    int maximumSize;
    int minimumSize;
    bool expansive;
    bool empty;
    int spacing;

    // temporary storage
    bool done;

    // result
    int pos;
    int size;
};


Q_GUI_EXPORT void qGeomCalc(QVector<QLayoutStruct> &chain, int start, int count,
                            int pos, int space, int spacer = -1);
Q_GUI_EXPORT QSize qSmartMinSize(const QSize &sizeHint, const QSize &minSizeHint,
                                 const QSize &minSize, const QSize &maxSize,
                                 const QSizePolicy &sizePolicy);
Q_GUI_EXPORT QSize qSmartMinSize(const QWidgetItem *i);
Q_GUI_EXPORT QSize qSmartMinSize(const QWidget *w);
Q_GUI_EXPORT QSize qSmartMaxSize(const QSize &sizeHint,
                                 const QSize &minSize, const QSize &maxSize,
                                 const QSizePolicy &sizePolicy, Qt::Alignment align = 0);
Q_GUI_EXPORT QSize qSmartMaxSize(const QWidgetItem *i, Qt::Alignment align = 0);
Q_GUI_EXPORT QSize qSmartMaxSize(const QWidget *w, Qt::Alignment align = 0);

Q_GUI_EXPORT int qSmartSpacing(const QLayout *layout, QStyle::PixelMetric pm);

/*
  Modify total maximum (max), total expansion (exp), and total empty
  when adding boxmax/boxexp.

  Expansive boxes win over non-expansive boxes.
  Non-empty boxes win over empty boxes.
*/
static inline void qMaxExpCalc(int & max, bool &exp, bool &empty,
                               int boxmax, bool boxexp, bool boxempty)
{
    if (exp) {
        if (boxexp)
            max = qMax(max, boxmax);
    } else {
        if (boxexp || (empty && (!boxempty || max == 0)))
            max = boxmax;
        else if (empty == boxempty)
            max = qMin(max, boxmax);
    }
    exp = exp || boxexp;
    empty = empty && boxempty;
}

QT_END_NAMESPACE

#endif // QLAYOUTENGINE_P_H

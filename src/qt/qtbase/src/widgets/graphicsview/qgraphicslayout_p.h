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

#ifndef QGRAPHICSLAYOUT_P_H
#define QGRAPHICSLAYOUT_P_H

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

#include <QtCore/qglobal.h>

#if !defined(QT_NO_GRAPHICSVIEW)

#include "qgraphicslayout.h"
#include "qgraphicslayoutitem_p.h"
#include <QtWidgets/qstyle.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qstyleoption.h>

QT_BEGIN_NAMESPACE

class QGraphicsLayoutItem;
class QGraphicsWidget;

#ifdef QT_DEBUG
inline bool qt_graphicsLayoutDebug()
{
    static int checked_env = -1;
    if(checked_env == -1)
        checked_env = !!qgetenv("QT_GRAPHICSLAYOUT_DEBUG").toInt();
    return checked_env;
}
#endif


class QLayoutStyleInfo
{
public:
    inline QLayoutStyleInfo() { invalidate(); }
    inline QLayoutStyleInfo(QStyle *style, QWidget *widget)
        : m_valid(true), m_style(style), m_widget(widget)
    {
        Q_ASSERT(style);
        if (widget) //###
            m_styleOption.initFrom(widget);
        m_defaultSpacing[0] = style->pixelMetric(QStyle::PM_LayoutHorizontalSpacing);
        m_defaultSpacing[1] = style->pixelMetric(QStyle::PM_LayoutVerticalSpacing);
    }

    inline void invalidate() { m_valid = false; m_style = 0; m_widget = 0; }

    inline QStyle *style() const { return m_style; }
    inline QWidget *widget() const { return m_widget; }

    inline bool operator==(const QLayoutStyleInfo &other) const
        { return m_style == other.m_style && m_widget == other.m_widget; }
    inline bool operator!=(const QLayoutStyleInfo &other) const
        { return !(*this == other); }

    inline void setDefaultSpacing(Qt::Orientation o, qreal spacing){
        if (spacing >= 0)
            m_defaultSpacing[o - 1] = spacing;
    }

    inline qreal defaultSpacing(Qt::Orientation o) const {
        return m_defaultSpacing[o - 1];
    }

    inline qreal perItemSpacing(QSizePolicy::ControlType control1,
                                  QSizePolicy::ControlType control2,
                                  Qt::Orientation orientation) const
    {
        Q_ASSERT(style());
        return style()->layoutSpacing(control1, control2, orientation, &m_styleOption, widget());
    }
private:
    bool m_valid;
    QStyle *m_style;
    QWidget *m_widget;
    QStyleOption m_styleOption;
    qreal m_defaultSpacing[2];
};

class Q_AUTOTEST_EXPORT QGraphicsLayoutPrivate : public QGraphicsLayoutItemPrivate
{
    Q_DECLARE_PUBLIC(QGraphicsLayout)

public:
    QGraphicsLayoutPrivate() : QGraphicsLayoutItemPrivate(0, true), left(-1.0), top(-1.0), right(-1.0), bottom(-1.0),
        activated(true) { }

    void reparentChildItems(QGraphicsItem *newParent);
    void getMargin(qreal *result, qreal userMargin, QStyle::PixelMetric pm) const;
    Qt::LayoutDirection visualDirection() const;

    void addChildLayoutItem(QGraphicsLayoutItem *item);
    void activateRecursive(QGraphicsLayoutItem *item);

    qreal left, top, right, bottom;
    bool activated;
};


QT_END_NAMESPACE

#endif //QT_NO_GRAPHICSVIEW

#endif

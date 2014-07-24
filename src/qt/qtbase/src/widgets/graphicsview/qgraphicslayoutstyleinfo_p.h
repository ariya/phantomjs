/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QGRAPHICSLAYOUTSTYLEINFO_P_H
#define QGRAPHICSLAYOUTSTYLEINFO_P_H

#include <QtGui/private/qabstractlayoutstyleinfo_p.h>
#include <QtWidgets/qstyleoption.h>

QT_BEGIN_NAMESPACE

class QStyle;
class QWidget;
class QGraphicsLayoutPrivate;

class QGraphicsLayoutStyleInfo : public QAbstractLayoutStyleInfo
{
public:
    QGraphicsLayoutStyleInfo(const QGraphicsLayoutPrivate *layout);
    ~QGraphicsLayoutStyleInfo();

    virtual qreal combinedLayoutSpacing(QLayoutPolicy::ControlTypes controls1,
                                        QLayoutPolicy::ControlTypes controls2,
                                        Qt::Orientation orientation) const Q_DECL_OVERRIDE;

    virtual qreal perItemSpacing(QLayoutPolicy::ControlType control1,
                                 QLayoutPolicy::ControlType control2,
                                 Qt::Orientation orientation) const Q_DECL_OVERRIDE;

    virtual qreal spacing(Qt::Orientation orientation) const Q_DECL_OVERRIDE;

    virtual qreal windowMargin(Qt::Orientation orientation) const Q_DECL_OVERRIDE;

    virtual void invalidate() Q_DECL_OVERRIDE
    {
        m_style = 0;
        QAbstractLayoutStyleInfo::invalidate();
    }

    virtual bool hasChangedCore() const Q_DECL_OVERRIDE
    {
        QStyle *s = m_style;
        // Note that style() will change m_style
        return s != style();
    }

    QWidget *widget() const;
    QStyle *style() const;

private:
    const QGraphicsLayoutPrivate *m_layout;
    mutable QStyle *m_style;
    QStyleOption m_styleOption;
    QWidget *m_widget;
};

QT_END_NAMESPACE

#endif // QGRAPHICSLAYOUTSTYLEINFO_P_H
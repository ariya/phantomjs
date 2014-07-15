/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QGRAPHICSLAYOUT_H
#define QGRAPHICSLAYOUT_H

#include <QtGui/qgraphicslayoutitem.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

class QGraphicsLayoutPrivate;
class QGraphicsLayoutItem;
class QGraphicsWidget;

class Q_GUI_EXPORT QGraphicsLayout : public QGraphicsLayoutItem
{
public:
    QGraphicsLayout(QGraphicsLayoutItem *parent = 0);
    ~QGraphicsLayout();

    void setContentsMargins(qreal left, qreal top, qreal right, qreal bottom);
    void getContentsMargins(qreal *left, qreal *top, qreal *right, qreal *bottom) const;

    void activate();
    bool isActivated() const;
    virtual void invalidate();
    virtual void updateGeometry();

    virtual void widgetEvent(QEvent *e);

    virtual int count() const = 0;
    virtual QGraphicsLayoutItem *itemAt(int i) const = 0;
    virtual void removeAt(int index) = 0;

    static void setInstantInvalidatePropagation(bool enable);
    static bool instantInvalidatePropagation();
protected:
    QGraphicsLayout(QGraphicsLayoutPrivate &, QGraphicsLayoutItem *);
    void addChildLayoutItem(QGraphicsLayoutItem *layoutItem);

private:
    Q_DISABLE_COPY(QGraphicsLayout)
    Q_DECLARE_PRIVATE(QGraphicsLayout)
    friend class QGraphicsWidget;
};

Q_DECLARE_INTERFACE(QGraphicsLayout, "com.trolltech.Qt.QGraphicsLayout")

#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif


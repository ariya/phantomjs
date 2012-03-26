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

#ifndef QGRAPHICSANCHORLAYOUT_H
#define QGRAPHICSANCHORLAYOUT_H

#include <QtGui/qgraphicsitem.h>
#include <QtGui/qgraphicslayout.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#if !defined(QT_NO_GRAPHICSVIEW) || (QT_EDITION & QT_MODULE_GRAPHICSVIEW) != QT_MODULE_GRAPHICSVIEW

class QGraphicsAnchorPrivate;
class QGraphicsAnchorLayout;
class QGraphicsAnchorLayoutPrivate;

class Q_GUI_EXPORT QGraphicsAnchor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing RESET unsetSpacing)
    Q_PROPERTY(QSizePolicy::Policy sizePolicy READ sizePolicy WRITE setSizePolicy)
public:
    void setSpacing(qreal spacing);
    void unsetSpacing();
    qreal spacing() const;
    void setSizePolicy(QSizePolicy::Policy policy);
    QSizePolicy::Policy sizePolicy() const;
    ~QGraphicsAnchor();
private:
    QGraphicsAnchor(QGraphicsAnchorLayout *parent);

    Q_DECLARE_PRIVATE(QGraphicsAnchor)

    friend class QGraphicsAnchorLayoutPrivate;
    friend struct AnchorData;
};

class Q_GUI_EXPORT QGraphicsAnchorLayout : public QGraphicsLayout
{
public:
    QGraphicsAnchorLayout(QGraphicsLayoutItem *parent = 0);
    virtual ~QGraphicsAnchorLayout();

    QGraphicsAnchor *addAnchor(QGraphicsLayoutItem *firstItem, Qt::AnchorPoint firstEdge,
                               QGraphicsLayoutItem *secondItem, Qt::AnchorPoint secondEdge);
    QGraphicsAnchor *anchor(QGraphicsLayoutItem *firstItem, Qt::AnchorPoint firstEdge,
                            QGraphicsLayoutItem *secondItem, Qt::AnchorPoint secondEdge);

    void addCornerAnchors(QGraphicsLayoutItem *firstItem, Qt::Corner firstCorner,
                          QGraphicsLayoutItem *secondItem, Qt::Corner secondCorner);

    void addAnchors(QGraphicsLayoutItem *firstItem,
                    QGraphicsLayoutItem *secondItem,
                    Qt::Orientations orientations = Qt::Horizontal | Qt::Vertical);

    void setHorizontalSpacing(qreal spacing);
    void setVerticalSpacing(qreal spacing);
    void setSpacing(qreal spacing);
    qreal horizontalSpacing() const;
    qreal verticalSpacing() const;

    void removeAt(int index);
    void setGeometry(const QRectF &rect);
    int count() const;
    QGraphicsLayoutItem *itemAt(int index) const;

    void invalidate();
protected:
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

private:
    Q_DISABLE_COPY(QGraphicsAnchorLayout)
    Q_DECLARE_PRIVATE(QGraphicsAnchorLayout)

    friend class QGraphicsAnchor;
};

#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif

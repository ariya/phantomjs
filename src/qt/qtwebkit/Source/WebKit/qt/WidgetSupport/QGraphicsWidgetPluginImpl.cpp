/*
 * Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "QGraphicsWidgetPluginImpl.h"

#include <QGraphicsScene>
#include <QGraphicsWidget>

#ifndef QT_NO_GRAPHICSVIEW

QGraphicsWidgetPluginImpl::~QGraphicsWidgetPluginImpl()
{
    m_graphicsWidget->deleteLater();
}

void QGraphicsWidgetPluginImpl::update(const QRect &rect)
{
    QGraphicsScene* scene = m_graphicsWidget->scene();
    if (scene)
        scene->update(rect);
}

void QGraphicsWidgetPluginImpl::setGeometryAndClip(const QRect &geometry, const QRect &, bool)
{
    m_graphicsWidget->setGeometry(geometry);
    // FIXME: Make the code handle clipping of graphics widgets.
}

void QGraphicsWidgetPluginImpl::setVisible(bool visible)
{
    m_graphicsWidget->setVisible(visible);
}

void QGraphicsWidgetPluginImpl::setWidgetParent(QObject* parent)
{
    QGraphicsObject* parentItem = qobject_cast<QGraphicsObject*>(parent);
    if (parentItem)
        m_graphicsWidget->setParentItem(parentItem);
}

QObject* QGraphicsWidgetPluginImpl::handle() const
{
    return m_graphicsWidget;
}

#include "moc_QGraphicsWidgetPluginImpl.cpp"

#endif // !defined(QT_NO_GRAPHICSVIEW)

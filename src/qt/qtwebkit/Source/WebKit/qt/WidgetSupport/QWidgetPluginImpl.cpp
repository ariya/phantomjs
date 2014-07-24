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
#include "QWidgetPluginImpl.h"

#include <QWidget>

QWidgetPluginImpl::~QWidgetPluginImpl()
{
    m_widget->deleteLater();
}

void QWidgetPluginImpl::update(const QRect &rect)
{
    m_widget->update(rect);
}

void QWidgetPluginImpl::setGeometryAndClip(const QRect &geometry, const QRect &clipRect, bool isVisible)
{
    m_widget->setGeometry(geometry);
    if (!clipRect.isNull()) {
        QRect clip(clipRect.intersected(m_widget->rect()));
        m_widget->setMask(QRegion(clip));
    }
    m_widget->update();
    setVisible(isVisible);
}

void QWidgetPluginImpl::setVisible(bool visible)
{
    // If setMask is set with an empty QRegion, no clipping will
    // be performed, so in that case we hide the platformWidget.
    QRegion mask = m_widget->mask();
    m_widget->setVisible(visible && !mask.isEmpty());
}

void QWidgetPluginImpl::setStyleSheet(const QString &stylesheet)
{
    m_widget->setStyleSheet(stylesheet);
}

void QWidgetPluginImpl::setWidgetParent(QObject *parent)
{
    if (!parent->isWidgetType())
        return;
    m_widget->setParent(qobject_cast<QWidget*>(parent));
}

QObject* QWidgetPluginImpl::handle() const
{
    return m_widget;
}

#include "moc_QWidgetPluginImpl.cpp"

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
#ifndef QGraphicsWidgetPluginImpl_h
#define QGraphicsWidgetPluginImpl_h

#ifndef QT_NO_GRAPHICSVIEW

#include "QtPluginWidgetAdapter.h"

QT_BEGIN_NAMESPACE
class QGraphicsWidget;
QT_END_NAMESPACE

class QGraphicsWidgetPluginImpl : public QtPluginWidgetAdapter {
    Q_OBJECT
public:
    QGraphicsWidgetPluginImpl(QGraphicsWidget *w) : m_graphicsWidget(w) { }
    virtual ~QGraphicsWidgetPluginImpl();
    virtual void update(const QRect &) OVERRIDE;
    virtual void setGeometryAndClip(const QRect&, const QRect&, bool) OVERRIDE;
    virtual void setVisible(bool);
    virtual void setStyleSheet(const QString&) OVERRIDE { }
    virtual void setWidgetParent(QObject*) OVERRIDE;
    virtual QObject* handle() const OVERRIDE;
private:
    QGraphicsWidget *m_graphicsWidget;
};
#endif // !defined(QT_NO_GRAPHICSVIEW)

#endif // QGraphicsWidgetPluginImpl_h

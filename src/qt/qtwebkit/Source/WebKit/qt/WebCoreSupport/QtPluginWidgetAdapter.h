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
#ifndef QtPluginWidgetAdapter_h
#define QtPluginWidgetAdapter_h

#include <PlatformExportMacros.h>
#include <QObject>
#include <QRect>
#include <QString>
#include <qwebkitglobal.h>

class QtPluginWidgetAdapter : public QObject {
    Q_OBJECT
public:
    QtPluginWidgetAdapter();
    virtual void update(const QRect&) = 0;
    virtual void setGeometryAndClip(const QRect&, const QRect&, bool isVisible = false) = 0;
    virtual void setVisible(bool) = 0;
    virtual void setStyleSheet(const QString&) = 0;
    virtual void setWidgetParent(QObject*) = 0;
    virtual QObject* handle() const = 0;
};

#endif // QtPluginWidgetAdapter_h

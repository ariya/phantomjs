/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
#ifndef DeviceOrientationClientQt_h
#define DeviceOrientationClientQt_h

#include "DeviceOrientation.h"
#include "DeviceOrientationClient.h"

#include <QObject>

class QWebPage;

namespace WebCore {

class DeviceOrientationProviderQt;

class DeviceOrientationClientQt : public QObject, public DeviceOrientationClient {
    Q_OBJECT
public:
    DeviceOrientationClientQt(QWebPage*);
    virtual ~DeviceOrientationClientQt();

    virtual void setController(DeviceOrientationController*);
    virtual void startUpdating();
    virtual void stopUpdating();
    virtual DeviceOrientation* lastOrientation() const;
    virtual void deviceOrientationControllerDestroyed();

public Q_SLOTS:
    void changeDeviceOrientation(DeviceOrientation*);

private:
    QWebPage* m_page;
    DeviceOrientationController* m_controller;
    DeviceOrientationProviderQt* m_provider;
};

} // namespace WebCore

#endif // DeviceOrientationClientQt_h

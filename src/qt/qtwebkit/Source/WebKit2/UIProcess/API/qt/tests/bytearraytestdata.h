/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef bytearraytestdata_h
#define bytearraytestdata_h

#if 0
#pragma qt_no_master_include
#endif

#include "qwebkitglobal.h"
#include <QByteArray>
#include <QObject>
#include <QtQuick/qquickitem.h>

class ByteArrayTestData : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariant latin1Data READ latin1Data)
    Q_PROPERTY(QVariant utf8Data READ utf8Data)

public:
    ByteArrayTestData(QObject* parent = 0);
    virtual ~ByteArrayTestData();
    QVariant latin1Data() const;
    QVariant utf8Data() const;

private:
    QByteArray m_latin1Data;
    QByteArray m_utf8Data;
};

QML_DECLARE_TYPE(ByteArrayTestData)

#endif // bytearraytestdata_h

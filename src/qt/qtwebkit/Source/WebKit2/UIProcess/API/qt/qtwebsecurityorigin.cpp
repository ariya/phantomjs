/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "qtwebsecurityorigin_p.h"

#include <QtCore/QFileInfo>
#include <QtCore/QStringList>
#include <SchemeRegistry.h>
#include <SecurityOrigin.h>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKRetainPtr.h>
#include <WebKit2/WKSecurityOrigin.h>

using namespace WebCore;

QtWebSecurityOrigin::QtWebSecurityOrigin(QObject* parent)
    : QObject(parent)
{
}

QtWebSecurityOrigin::~QtWebSecurityOrigin()
{
}

QString QtWebSecurityOrigin::host() const
{
    return m_url.host();
}

QString QtWebSecurityOrigin::scheme() const
{
    return m_url.scheme();
}

QString QtWebSecurityOrigin::path() const
{
    return m_url.path();
}

int QtWebSecurityOrigin::port() const
{
    return m_url.port();
}

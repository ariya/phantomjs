/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#include "FrameNetworkingContextQt.h"

#include "qwebframe.h"
#include "qwebpage.h"
#include <QNetworkAccessManager>
#include <QObject>

namespace WebCore {

FrameNetworkingContextQt::FrameNetworkingContextQt(Frame* frame, QObject* originatingObject, bool mimeSniffingEnabled, QNetworkAccessManager* networkAccessManager)
    : FrameNetworkingContext(frame)
    , m_originatingObject(originatingObject)
    , m_networkAccessManager(networkAccessManager)
    , m_mimeSniffingEnabled(mimeSniffingEnabled)
{
}

PassRefPtr<FrameNetworkingContextQt> FrameNetworkingContextQt::create(Frame* frame, QObject* originatingObject, bool mimeSniffingEnabled, QNetworkAccessManager* networkAccessManager)
{
    return adoptRef(new FrameNetworkingContextQt(frame, originatingObject, mimeSniffingEnabled, networkAccessManager));
}

QObject* FrameNetworkingContextQt::originatingObject() const
{
    return m_originatingObject;
}

QNetworkAccessManager* FrameNetworkingContextQt::networkAccessManager() const
{
    return (qobject_cast<QWebFrame*>(m_originatingObject))->page()->networkAccessManager();
}

bool FrameNetworkingContextQt::mimeSniffingEnabled() const
{
    return m_mimeSniffingEnabled;
}

}

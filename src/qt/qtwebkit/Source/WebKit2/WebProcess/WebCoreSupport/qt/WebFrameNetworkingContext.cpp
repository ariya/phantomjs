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
#include "WebFrameNetworkingContext.h"

#include "WebFrame.h"
#include "WebPage.h"
#include <QObject>
#include <QVariant>

namespace WebKit {

WebFrameNetworkingContext::WebFrameNetworkingContext(WebFrame* frame)
    : FrameNetworkingContext(frame->coreFrame())
    , m_mimeSniffingEnabled(true)
{
    // Save the page ID for a valid page as it is needed later for HTTP authentication and SSL errors.
    if (frame->page()) {
        m_originatingObject = adoptPtr(new QObject);
        m_originatingObject->setProperty("pageID", qulonglong(frame->page()->pageID()));
    }
}

PassRefPtr<WebFrameNetworkingContext> WebFrameNetworkingContext::create(WebFrame* frame)
{
    return adoptRef(new WebFrameNetworkingContext(frame));
}

}

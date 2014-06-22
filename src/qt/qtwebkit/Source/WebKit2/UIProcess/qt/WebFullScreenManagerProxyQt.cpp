/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebFullScreenManagerProxy.h"

#if ENABLE(FULLSCREEN_API)

#include "WebContext.h"
#include "WebFullScreenManagerMessages.h"
#include "WebFullScreenManagerProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/NotImplemented.h>

#if HAVE(QTQUICK)
#include "qquickwebview_p.h"
#endif

using namespace WebCore;

namespace WebKit {

void WebFullScreenManagerProxy::invalidate()
{
#if HAVE(QTQUICK)
    m_webView = 0;
#endif
}

void WebFullScreenManagerProxy::close()
{
    notImplemented();
}

bool WebFullScreenManagerProxy::isFullScreen()
{
    notImplemented();
    return false;
}

void WebFullScreenManagerProxy::enterFullScreen()
{
    willEnterFullScreen();
#if HAVE(QTQUICK)
    emit m_webView->experimental()->enterFullScreenRequested();
#endif
    didEnterFullScreen();
}

void WebFullScreenManagerProxy::exitFullScreen()
{
    willExitFullScreen();
#if HAVE(QTQUICK)
    emit m_webView->experimental()->exitFullScreenRequested();
#endif
    didExitFullScreen();
}

void WebFullScreenManagerProxy::beganEnterFullScreen(const IntRect& initialFrame, const IntRect& finalFrame)
{
    notImplemented();
}

void WebFullScreenManagerProxy::beganExitFullScreen(const IntRect& initialFrame, const IntRect& finalFrame)
{
    notImplemented();
}

} // namespace WebKit

#endif // ENABLE(FULLSCREEN_API)

/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef PageClientBlackBerry_h
#define PageClientBlackBerry_h

#include "Cursor.h"
#include "WebPageClient.h"

namespace BlackBerry {
namespace Platform {
class NetworkStreamFactory;
namespace Graphics {
class Window;
}
}
}

namespace WebCore {
class AuthenticationChallengeClient;
class Credential;
class IntRect;
class IntSize;
class KURL;
class PluginView;
class ProtectionSpace;
}

class PageClientBlackBerry {
public:
    enum SaveCredentialType {
        SaveCredentialNeverForThisSite = 0,
        SaveCredentialNotNow,
        SaveCredentialYes
    };

    virtual int playerID() const = 0;
    virtual void setCursor(WebCore::PlatformCursor) = 0;
    virtual BlackBerry::Platform::NetworkStreamFactory* networkStreamFactory() = 0;
    virtual BlackBerry::Platform::Graphics::Window* platformWindow() const = 0;
    virtual void setPreventsScreenDimming(bool) = 0;
    virtual void showVirtualKeyboard(bool) = 0;
    virtual void ensureContentVisible(bool centerInView = true) = 0;
    virtual void zoomToContentRect(const WebCore::IntRect&) = 0;
    virtual void registerPlugin(WebCore::PluginView*, bool) = 0;
    virtual void notifyPageOnLoad() = 0;
    virtual bool shouldPluginEnterFullScreen(WebCore::PluginView*, const char* uniquePluginPrefix) = 0;
    virtual void didPluginEnterFullScreen(WebCore::PluginView*, const char* uniquePluginPrefix) = 0;
    virtual void didPluginExitFullScreen(WebCore::PluginView*, const char* uniquePluginPrefix) = 0;
    virtual void onPluginStartBackgroundPlay(WebCore::PluginView*, const char* uniquePluginPrefix) = 0;
    virtual void onPluginStopBackgroundPlay(WebCore::PluginView*, const char* uniquePluginPrefix) = 0;
    virtual bool lockOrientation(bool landscape) = 0;
    virtual void unlockOrientation() = 0;
    virtual int orientation() const = 0;
    virtual double currentZoomFactor() const = 0;
    virtual WebCore::IntSize viewportSize() const = 0;
    virtual int showAlertDialog(BlackBerry::WebKit::WebPageClient::AlertType) = 0;
    virtual bool isActive() const = 0;
    virtual bool isVisible() const = 0;
    virtual void authenticationChallenge(const WebCore::KURL&, const WebCore::ProtectionSpace&, const WebCore::Credential&) = 0;
    virtual SaveCredentialType notifyShouldSaveCredential(bool) = 0;
    virtual void syncProxyCredential(const WebCore::Credential&) = 0;
};

#endif // PageClientBlackBerry_h

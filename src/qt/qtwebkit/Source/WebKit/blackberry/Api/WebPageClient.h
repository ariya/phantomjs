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

#ifndef WebPageClient_h
#define WebPageClient_h

#include "BlackBerryGlobal.h"

#include <BlackBerryPlatformCursor.h>
#include <BlackBerryPlatformInputEvents.h>
#include <BlackBerryPlatformNavigationType.h>
#include <BlackBerryPlatformPrimitives.h>
#include <BlackBerryPlatformString.h>
#include <imf/events.h>
#include <interaction/ScrollViewBase.h>
#include <vector>

template<typename T> class ScopeArray;
template<typename T> class SharedArray;

typedef void* WebFrame;

namespace BlackBerry {

namespace Platform {
class FilterStream;
class GeoTrackerListener;
class IntRectRegion;
class NetworkRequest;
class NetworkStreamFactory;
struct SelectionDetails;
class ViewportAccessor;
class WebUserMediaRequest;

namespace Graphics {
class Window;
}
}

namespace WebKit {
class WebPage;

class BLACKBERRY_EXPORT WebPageClient {
public:
    virtual ~WebPageClient() { }
    enum WindowStyleFlag {
        FlagWindowHasMenuBar = 0x00000001,
        FlagWindowHasToolBar = 0x00000002,
        FlagWindowHasLocationBar = 0x00000004,
        FlagWindowHasStatusBar = 0x00000008,
        FlagWindowHasScrollBar = 0x00000010,
        FlagWindowIsResizable = 0x00000020,
        FlagWindowIsFullScreen = 0x00000040,
        FlagWindowIsDialog = 0x00000080,
        FlagWindowDefault = 0xFFFFFFFF,
    };

    enum AlertType {
        MediaOK = 0,
        MediaDecodeError,
        MediaMetaDataError,
        MediaMetaDataTimeoutError,
        MediaNoMetaDataError,
        MediaVideoReceiveError,
        MediaAudioReceiveError,
        MediaInvalidError,
    };

    enum SaveCredentialType {
        SaveCredentialNeverForThisSite = 0,
        SaveCredentialNotNow,
        SaveCredentialYes
    };

    virtual int getInstanceId() const = 0;

    virtual void notifyLoadStarted() = 0;
    virtual void notifyLoadCommitted(const unsigned short* originalUrl, unsigned originalUrlLength, const unsigned short* finalUrl, unsigned finalUrlLength, const unsigned short* networkToken, unsigned networkTokenLength) = 0;
    virtual void notifyLoadFailedBeforeCommit(const unsigned short* originalUrl, unsigned originalUrlLength, const unsigned short* finalUrl, unsigned finalUrlLength, const unsigned short* networkToken, unsigned networkTokenLength) = 0;
    virtual void notifyLoadToAnchor(const unsigned short* url, unsigned urlLength, const unsigned short* networkToken, unsigned networkTokenLength) = 0;
    virtual void notifyLoadProgress(int percentage) = 0;
    virtual void notifyLoadReadyToRender(bool pageIsVisuallyNonEmpty) = 0;
    virtual void notifyFirstVisuallyNonEmptyLayout() = 0;
    virtual void notifyLoadFinished(int status) = 0;
    virtual void notifyClientRedirect(const unsigned short* originalUrl, unsigned originalUrlLength, const unsigned short* finalUrl, unsigned finalUrlLength) = 0;

    virtual void notifyFrameDetached(const WebFrame) = 0;

    virtual void notifyRunLayoutTestsFinished() = 0;

    virtual void notifyInRegionScrollableAreasChanged(const std::vector<Platform::ScrollViewBase*>&) = 0;

    virtual void notifyDocumentOnLoad(bool) = 0;

    virtual void notifyWindowObjectCleared() = 0;

    virtual void addMessageToConsole(const unsigned short* message, unsigned messageLength, const unsigned short* source, unsigned sourceLength, unsigned lineNumber, unsigned columnNumber) = 0;
    virtual int showAlertDialog(AlertType) = 0;

    virtual BlackBerry::Platform::String serializePageCacheState() const = 0;
    virtual void deserializePageCacheState(const BlackBerry::Platform::String& state) = 0;

    virtual void runJavaScriptAlert(const unsigned short* message, unsigned messageLength, const char* origin, unsigned originLength) = 0;
    virtual bool runJavaScriptConfirm(const unsigned short* message, unsigned messageLength, const char* origin, unsigned originLength) = 0;
    virtual bool runJavaScriptPrompt(const unsigned short* message, unsigned messageLength, const unsigned short* defaultValue, unsigned defaultValueLength, const char* origin, unsigned originLength, BlackBerry::Platform::String& result) = 0;
    virtual bool runBeforeUnloadConfirmPanel(const unsigned short* message, unsigned messageLength, const char* origin, unsigned originLength) = 0;

    virtual bool shouldInterruptJavaScript() = 0;

    virtual void contentsSizeChanged() = 0;
    virtual void scrollChanged() = 0;
    virtual void scaleChanged() = 0;

    virtual void updateInteractionViews() = 0;

    virtual void requestUpdateViewport(int width, int height) = 0;

    virtual void setPageTitle(const unsigned short* title, unsigned titleLength) = 0;

    virtual Platform::Graphics::Window* window() const = 0;
    virtual void postToSurface(const Platform::IntRect&) = 0;

    virtual void notifyPixelContentRendered(const Platform::IntRect&) = 0;

    virtual void inputFocusGained(int64_t inputStyle, Platform::VirtualKeyboardType, Platform::VirtualKeyboardEnterKeyType) = 0;
    virtual void inputFocusLost() = 0;
    virtual void inputTextChanged() = 0;
    virtual void inputSelectionChanged(unsigned selectionStart, unsigned selectionEnd) = 0;
    virtual void inputLearnText(wchar_t* text, int length) = 0;

    virtual void showFormControls(bool visible, bool previousActive = false, bool nextActive = false) = 0;
    virtual void showVirtualKeyboard(bool) = 0;

    virtual void requestSpellingCheckingOptions(imf_sp_text_t&, const BlackBerry::Platform::IntRect& documentCaretRect, const BlackBerry::Platform::IntSize& screenOffset, const bool shouldMoveDialog) = 0;
    virtual int32_t checkSpellingOfStringAsync(wchar_t* text, const unsigned length) = 0;

    virtual void notifySelectionDetailsChanged(const BlackBerry::Platform::SelectionDetails&) = 0;
    virtual void cancelSelectionVisuals() = 0;
    virtual void notifySelectionHandlesReversed() = 0;
    virtual void notifyCaretChanged(const Platform::IntRect& documentCaretRect, bool userTouchTriggered, bool isSingleLineInput = false, const Platform::IntRect& singleLineDocumentBoundingBox = Platform::IntRect(), bool textFieldIsEmpty = false) = 0;
    virtual void notifySelectionScrollView(Platform::ScrollViewBase*) = 0;

    virtual void cursorChanged(Platform::CursorType, const char* url, const Platform::IntPoint& hotSpotInImage) = 0;

    virtual void requestGlobalLocalServicePermission(Platform::GeoTrackerListener*, const BlackBerry::Platform::String& origin) = 0;

    virtual void requestGeolocationPermission(Platform::GeoTrackerListener*, const BlackBerry::Platform::String& origin) = 0;
    virtual void cancelGeolocationPermission() = 0;
    virtual Platform::NetworkStreamFactory* networkStreamFactory() = 0;

    virtual void handleStringPattern(const unsigned short* pattern, unsigned length) = 0;
    virtual void handleExternalLink(const Platform::NetworkRequest&, const unsigned short* context, unsigned contextLength, bool isClientRedirect) = 0;

    virtual void resetBackForwardList(unsigned listSize, unsigned currentIndex) = 0;

    virtual void openPopupList(bool multiple, int size, const ScopeArray<BlackBerry::Platform::String>& labels, const bool* enableds, const int* itemType, const bool* selecteds) = 0;
    virtual bool chooseFilenames(bool allowMultiple, const SharedArray<BlackBerry::Platform::String>& acceptTypes, const SharedArray<BlackBerry::Platform::String>& initialFiles, const BlackBerry::Platform::String& capture, SharedArray<BlackBerry::Platform::String>& chosenFiles) = 0;

    virtual WebPage* createWindow(int x, int y, int width, int height, unsigned flags, const BlackBerry::Platform::String& url, const BlackBerry::Platform::String& windowName, const BlackBerry::Platform::String& openerFrameUrl, bool userGesture) = 0;

    virtual void scheduleCloseWindow() = 0;

    // Database interface.
    virtual unsigned long long databaseQuota(const BlackBerry::Platform::String& origin, const BlackBerry::Platform::String& databaseName, unsigned long long originUsage, unsigned long long currentQuota, unsigned long long estimatedSize) = 0;

    virtual void setIconForUrl(const BlackBerry::Platform::String& originalPageUrl, const BlackBerry::Platform::String& finalPageUrl, const BlackBerry::Platform::String& iconUrl) = 0;
    virtual void setFavicon(const BlackBerry::Platform::String& dataInBase64, const BlackBerry::Platform::String& url) = 0;
    virtual void setLargeIcon(const BlackBerry::Platform::String& iconUrl) = 0;
    virtual void setWebAppCapable() = 0;
    virtual void setSearchProviderDetails(const BlackBerry::Platform::String& title, const BlackBerry::Platform::String& documentUrl) = 0;
    virtual void setAlternateFeedDetails(const BlackBerry::Platform::String& title, const BlackBerry::Platform::String& feedUrl) = 0;

    virtual BlackBerry::Platform::String getErrorPage(int errorCode, const BlackBerry::Platform::String& errorMessage, const BlackBerry::Platform::String& url) = 0;

    virtual void willDeferLoading() = 0;
    virtual void didResumeLoading() = 0;

    // Headers is a list of alternating key and value.
    virtual void setMetaHeaders(const ScopeArray<BlackBerry::Platform::String>& headers, unsigned headersSize) = 0;

    virtual void needMoreData() = 0;
    virtual void handleWebInspectorMessageToFrontend(int id, const char* message, int length) = 0;

    virtual BlackBerry::Platform::ViewportAccessor* userInterfaceViewportAccessor() const = 0;

    virtual void animateToScaleAndDocumentScrollPosition(double finalScale, const Platform::FloatPoint& finalDocumentScrollPosition, bool shouldConstrainScrollingToContentEdge) = 0;

    virtual void setPreventsScreenIdleDimming(bool noDimming) = 0;
    virtual bool authenticationChallenge(const unsigned short* realm, unsigned realmLength, BlackBerry::Platform::String& username, BlackBerry::Platform::String& password, BlackBerry::Platform::String& requestURL, bool isProxy) = 0;
    virtual SaveCredentialType notifyShouldSaveCredential(bool isNew) = 0;
    virtual void syncProxyCredential(const BlackBerry::Platform::String& username, const BlackBerry::Platform::String& password) = 0;
    virtual void notifyPopupAutofillDialog(const std::vector<BlackBerry::Platform::String>&) = 0;
    virtual void notifyDismissAutofillDialog() = 0;

    virtual bool shouldPluginEnterFullScreen() = 0;
    virtual void didPluginEnterFullScreen() = 0;
    virtual void didPluginExitFullScreen() = 0;
    virtual void onPluginStartBackgroundPlay() = 0;
    virtual void onPluginStopBackgroundPlay() = 0;
    virtual bool lockOrientation(bool landscape) = 0;
    virtual void unlockOrientation() = 0;
    virtual bool isActive() const = 0;
    virtual bool isVisible() const = 0;

    virtual void setToolTip(const BlackBerry::Platform::String&) = 0;
    virtual void setStatus(const BlackBerry::Platform::String&) = 0;
    virtual bool acceptNavigationRequest(const Platform::NetworkRequest&, Platform::NavigationType) = 0;
    virtual void cursorEventModeChanged(Platform::CursorEventMode) = 0;
    virtual void touchEventModeChanged(Platform::TouchEventMode) = 0;

    virtual bool downloadAllowed(const BlackBerry::Platform::String& url) = 0;
    virtual void downloadRequested(Platform::FilterStream*, const BlackBerry::Platform::String& suggestedFilename) = 0;

    virtual int fullscreenStart() = 0;
    virtual int fullscreenStart(const char* contextName, Platform::Graphics::Window*, const BlackBerry::Platform::IntRect& windowScreenRect) = 0;

    virtual int fullscreenStop() = 0;

    virtual int fullscreenSetWindowRect(const BlackBerry::Platform::IntRect& newWindowScreenRect) = 0;

    virtual void populateCustomHeaders(Platform::NetworkRequest&) = 0;

    virtual void notifyWillUpdateApplicationCache() = 0;
    virtual void notifyDidLoadFromApplicationCache() = 0;

    virtual void clearCookies() = 0;
    virtual void clearCache() = 0;

    virtual bool hasKeyboardFocus() = 0;
    virtual bool createPopupWebView(const Platform::IntRect&) = 0;
    virtual void closePopupWebView() = 0;

    virtual void addSearchProvider(const Platform::String&) = 0;
    virtual int  isSearchProviderInstalled(const Platform::String&) = 0;

    // Match with ChromeClient::CustomHandlersState.
    enum ProtocolHandlersState {
        ProtocolHandlersNew,
        ProtocolHandlersRegistered,
        ProtocolHandlersDeclined
    };
    virtual void registerProtocolHandler(const BlackBerry::Platform::String& /*scheme*/, const BlackBerry::Platform::String& /*baseURL*/, const BlackBerry::Platform::String& /*url*/, const BlackBerry::Platform::String& /*title*/) = 0;
    virtual ProtocolHandlersState isProtocolHandlerRegistered(const BlackBerry::Platform::String& /*scheme*/, const BlackBerry::Platform::String& /*baseURL*/, const BlackBerry::Platform::String& /*url*/) = 0;
    virtual void unregisterProtocolHandler(const BlackBerry::Platform::String& /*scheme*/, const BlackBerry::Platform::String& /*baseURL*/, const BlackBerry::Platform::String& /*url*/) = 0;

    virtual void requestUserMedia(const Platform::WebUserMediaRequest&) = 0;
    virtual void cancelUserMediaRequest(const Platform::WebUserMediaRequest&) = 0;
    virtual void updateFindStringResult(int numMatches, int currentIndex) = 0;

    // Match with NotificationClient::Permission.
    enum Permission {
        PermissionAllowed, // User has allowed notifications
        PermissionNotAllowed, // User has not yet allowed
        PermissionDenied // User has explicitly denied permission
    };
    virtual void requestNotificationPermission(const BlackBerry::Platform::String& /*requestId*/, const BlackBerry::Platform::String& /*origin*/) = 0;
    virtual Permission checkNotificationPermission(const BlackBerry::Platform::String& /*origin*/) = 0;
    virtual void showNotification(const BlackBerry::Platform::String& /*notificationId*/, const BlackBerry::Platform::String& /*title*/, const BlackBerry::Platform::String& /*body*/, const BlackBerry::Platform::String& /*iconUrl*/, const BlackBerry::Platform::String& /*tag*/, const BlackBerry::Platform::String& /*origin*/) = 0;
    virtual void cancelNotification(const BlackBerry::Platform::String& /*id*/) = 0;
    virtual void clearNotifications(const std::vector<BlackBerry::Platform::String>& /*notificationIds*/) = 0;
    virtual void notificationDestroyed(const BlackBerry::Platform::String& /*notificationId*/) = 0;
    virtual void startSelectionScroll() = 0;
    virtual void stopExpandingSelection() = 0;
    virtual void suppressCaretChangeNotification(bool shouldClearState) = 0;
};
} // namespace WebKit
} // namespace BlackBerry

#endif // WebPageClient_h

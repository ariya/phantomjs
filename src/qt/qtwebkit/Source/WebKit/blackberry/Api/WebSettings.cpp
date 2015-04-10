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

#include "config.h"
#include "WebSettings.h"

#include "MIMETypeRegistry.h"
#include "WebSettings_p.h"

#include <BlackBerryPlatformDeviceInfo.h>
#include <BlackBerryPlatformFontInfo.h>
#include <BlackBerryPlatformScreen.h>
#include <BlackBerryPlatformString.h>
#include <Color.h>
#include <FloatSize.h>
#include <PageCache.h>
#include <ViewportArguments.h>
#include <wtf/HashSet.h>

namespace BlackBerry {
namespace WebKit {

DEFINE_STATIC_LOCAL(String, BlackBerryAllowCrossSiteRequests, (ASCIILiteral("BlackBerryAllowCrossSiteRequests")));
DEFINE_STATIC_LOCAL(String, BlackBerryApplyDeviceScaleFactorInCompositor, (ASCIILiteral("BlackBerryApplyDeviceScaleFactorInCompositor")));
DEFINE_STATIC_LOCAL(String, BlackBerryAsynchronousSpellChecking, (ASCIILiteral("BlackBerryAsynchronousSpellChecking")));
DEFINE_STATIC_LOCAL(String, BlackBerryBackgroundColor, (ASCIILiteral("BlackBerryBackgroundColor")));
DEFINE_STATIC_LOCAL(String, BlackBerryCookiesEnabled, (ASCIILiteral("BlackBerryCookiesEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryDrawBorderWhileLoadingImages, (ASCIILiteral("BlackBerryDrawBorderWhileLoadingImages")));
DEFINE_STATIC_LOCAL(String, BlackBerryEmailModeEnabled, (ASCIIliteral("BlackBerryEmailModeEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryGetFocusNodeContextEnabled, (ASCIILiteral("BlackBerryGetFocusNodeContextEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryHandlePatternURLs, (ASCIILiteral("BlackBerryHandlePatternURLs")));
DEFINE_STATIC_LOCAL(String, BlackBerryInitialScale, (ASCIILiteral("BlackBerryInitialScale")));
DEFINE_STATIC_LOCAL(String, BlackBerryLinksHandledExternallyEnabled, (ASCIILiteral("BlackBerryLinksHandledExternallyEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryMaxPluginInstances, (ASCIILiteral("BlackBerryMaxPluginInstances")));
DEFINE_STATIC_LOCAL(String, BlackBerryOverScrollColor, (ASCIILiteral("BlackBerryOverScrollColor")));
DEFINE_STATIC_LOCAL(String, BlackBerryEnableDefaultOverScrollBackground, (ASCIILiteral("BlackBerryEnableDefaultOverScrollBackground")));
DEFINE_STATIC_LOCAL(String, BlackBerryRenderAnimationsOnScrollOrZoomEnabled, (ASCIILiteral("BlackBerryRenderAnimationsOnScrollOrZoomEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryScrollbarsEnabled, (ASCIILiteral("BlackBerryScrollbarsEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryTextReflowMode, (ASCIILiteral("BlackBerryTextReflowMode")));
DEFINE_STATIC_LOCAL(String, BlackBerryUseRTLWritingDirection, (ASCIILiteral("BlackBerryUseRTLWritingDirection")));
DEFINE_STATIC_LOCAL(String, BlackBerryUseWebKitCache, (ASCIILiteral("BlackBerryUseWebKitCache")));
DEFINE_STATIC_LOCAL(String, BlackBerryUserAgentString, (ASCIILiteral("BlackBerryUserAgentString")));
DEFINE_STATIC_LOCAL(String, BlackBerryUserScalableEnabled, (ASCIILiteral("BlackBerryUserScalableEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryViewportWidth, (ASCIILiteral("BlackBerryViewportWidth")));
DEFINE_STATIC_LOCAL(String, BlackBerryZoomToFitOnLoadEnabled, (ASCIILiteral("BlackBerryZoomToFitOnLoadEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryFullScreenVideoCapable, (ASCIILiteral("BlackBerryFullScreenVideoCapable")));
DEFINE_STATIC_LOCAL(String, BlackBerryCredentialAutofillEnabled, (ASCIILiteral("BlackBerryCredentialAutofillEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryFormAutofillEnabled, (ASCIILiteral("BlackBerryFormAutofillEnabled")));
DEFINE_STATIC_LOCAL(String, BlackBerryDevicePixelRatio, (ASCIILiteral("BlackBerryDevicePixelRatio")));
DEFINE_STATIC_LOCAL(String, BlackBerryBackingStoreEnabled, (ASCIILiteral("BlackBerryBackingStoreEnabled")));
DEFINE_STATIC_LOCAL(String, SpatialNavigationEnabled, (ASCIILiteral("SpatialNavigationEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitDatabasePath, (ASCIILiteral("WebKitDatabasePath")));
DEFINE_STATIC_LOCAL(String, WebKitDatabasesEnabled, (ASCIILiteral("WebKitDatabasesEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitDefaultFixedFontSize, (ASCIILiteral("WebKitDefaultFixedFontSize")));
DEFINE_STATIC_LOCAL(String, WebKitDefaultFontSize, (ASCIILiteral("WebKitDefaultFontSize")));
DEFINE_STATIC_LOCAL(String, WebKitDefaultTextEncodingName, (ASCIILiteral("WebKitDefaultTextEncodingName")));
DEFINE_STATIC_LOCAL(String, WebKitDownloadableBinaryFontsEnabled, (ASCIILiteral("WebKitDownloadableBinaryFontsEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitFirstScheduledLayoutDelay, (ASCIILiteral("WebKitFirstScheduledLayoutDelay")));
DEFINE_STATIC_LOCAL(String, WebKitFixedFontFamily, (ASCIILiteral("WebKitFixedFontFamily")));
DEFINE_STATIC_LOCAL(String, WebKitFrameFlatteningEnabled, (ASCIILiteral("WebKitFrameFlatteningEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitGeolocationEnabled, (ASCIILiteral("WebKitGeolocationEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitIndexedDataBasePath, (ASCIILiteral("WebKitIndexedDataBasePath")));
DEFINE_STATIC_LOCAL(String, WebKitJavaScriptCanOpenWindowsAutomaticallyEnabled, (ASCIILiteral("WebKitJavaScriptCanOpenWindowsAutomaticallyEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitJavaScriptEnabled, (ASCIILiteral("WebKitJavaScriptEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitLoadsImagesAutomatically, (ASCIILiteral("WebKitLoadsImagesAutomatically")));
DEFINE_STATIC_LOCAL(String, WebKitLocalStorageEnabled, (ASCIILiteral("WebKitLocalStorageEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitLocalStoragePath, (ASCIILiteral("WebKitLocalStoragePath")));
DEFINE_STATIC_LOCAL(String, WebKitLocalStorageQuota, (ASCIILiteral("WebKitLocalStorageQuota")));
DEFINE_STATIC_LOCAL(String, WebKitSessionStorageQuota, (ASCIILiteral("WebKitSessionStorageQuota")));
DEFINE_STATIC_LOCAL(String, WebKitMaximumPagesInCache, (ASCIILiteral("WebKitMaximumPagesInCache")));
DEFINE_STATIC_LOCAL(String, WebKitMinimumFontSize, (ASCIILiteral("WebKitMinimumFontSize")));
DEFINE_STATIC_LOCAL(String, WebKitOfflineWebApplicationCacheEnabled, (ASCIILiteral("WebKitOfflineWebApplicationCacheEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitOfflineWebApplicationCachePath, (ASCIILiteral("WebKitOfflineWebApplicationCachePath")));
DEFINE_STATIC_LOCAL(String, WebKitPageGroupName, (ASCIILiteral("WebKitPageGroupName")));
DEFINE_STATIC_LOCAL(String, WebKitPluginsEnabled, (ASCIILiteral("WebKitPluginsEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitPrivateBrowsingEnabled, (ASCIILiteral("WebKitPrivateBrowsingEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitDeviceSupportsMouse, (ASCIILiteral("WebKitDeviceSupportsMouse")));
DEFINE_STATIC_LOCAL(String, WebKitSansSeriffFontFamily, (ASCIILiteral("WebKitSansSeriffFontFamily")));
DEFINE_STATIC_LOCAL(String, WebKitSeriffFontFamily, (ASCIILiteral("WebKitSeriffFontFamily")));
DEFINE_STATIC_LOCAL(String, WebKitStandardFontFamily, (ASCIILiteral("WebKitStandardFontFamily")));
DEFINE_STATIC_LOCAL(String, WebKitUserStyleSheetLocation, (ASCIILiteral("WebKitUserStyleSheetLocation")));
DEFINE_STATIC_LOCAL(String, WebKitWebSocketsEnabled, (ASCIILiteral("WebKitWebSocketsEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitXSSAuditorEnabled, (ASCIILiteral("WebKitXSSAuditorEnabled")));
DEFINE_STATIC_LOCAL(String, WebKitTextAutosizingEnabled, (ASCIILiteral("WebKitTextAutosizingEnabled")));

static HashSet<String>* s_supportedObjectMIMETypes;

WebSettingsPrivate::WebSettingsPrivate()
    : impl(0)
    , delegate(0)
    , sender(0)
    , copyOnWrite(true)
{
}

WebSettings::WebSettings()
{
    m_private = new WebSettingsPrivate();
}

WebSettings::WebSettings(const WebSettings& otherSettings)
{
    m_private->impl = otherSettings.m_private->impl;
}

WebSettings::~WebSettings()
{
    if (!m_private->copyOnWrite) {
        delete m_private->impl;
        m_private->impl = 0;
    }
    delete m_private;
    m_private = 0;
}

void WebSettings::setDelegate(WebSettingsDelegate* delegate)
{
    m_private->delegate = delegate;
    m_private->sender = this;
}

WebSettingsDelegate* WebSettings::delegate()
{
    return m_private->delegate;
}

WebSettings* WebSettings::createFromStandardSettings(WebSettingsDelegate* delegate)
{
    WebSettings* settings = new WebSettings();
    settings->m_private->impl = standardSettings()->m_private->impl;
    settings->m_private->delegate = delegate;
    settings->m_private->sender = settings;
    if (delegate)
        delegate->didChangeSettings(settings);

    return settings;
}

WebSettings* WebSettings::standardSettings()
{
    static WebSettings *settings = 0;
    if (settings)
        return settings;

    settings = new WebSettings;
    settings->m_private->impl = new WebSettingsPrivate::WebSettingsPrivateImpl();
    settings->m_private->copyOnWrite = false;
    settings->m_private->setBoolean(BlackBerryAllowCrossSiteRequests, false);
    settings->m_private->setUnsigned(BlackBerryBackgroundColor, WebCore::Color::white);
    settings->m_private->setBoolean(BlackBerryCookiesEnabled, true);
    settings->m_private->setBoolean(BlackBerryBackingStoreEnabled, true);
    settings->m_private->setDouble(BlackBerryInitialScale, -1);
    settings->m_private->setUnsigned(BlackBerryMaxPluginInstances, 1);
    settings->m_private->setUnsigned(BlackBerryOverScrollColor, WebCore::Color::white);
    settings->m_private->setBoolean(BlackBerryEnableDefaultOverScrollBackground, true);
    settings->m_private->setBoolean(BlackBerryScrollbarsEnabled, true);

    // FIXME: We should detect whether we are embedded in a browser or an email client and default to TextReflowEnabledOnlyForBlockZoom and TextReflowEnabled, respectively.
    settings->m_private->setTextReflowMode(BlackBerryTextReflowMode, TextReflowDisabled);

    settings->m_private->setBoolean(BlackBerryUseWebKitCache, true);
    settings->m_private->setBoolean(BlackBerryUserScalableEnabled, true);
    settings->m_private->setBoolean(BlackBerryZoomToFitOnLoadEnabled, true);
    settings->m_private->setBoolean(BlackBerryFullScreenVideoCapable, false);
    settings->m_private->setBoolean(BlackBerryCredentialAutofillEnabled, false);
    settings->m_private->setBoolean(BlackBerryFormAutofillEnabled, false);
    settings->m_private->setBoolean(BlackBerryAsynchronousSpellChecking, true);
    settings->m_private->setBoolean(BlackBerryApplyDeviceScaleFactorInCompositor, false);

    if (BlackBerry::Platform::DeviceInfo::instance()->isMobile()) {
        WebCore::FloatSize currentPPI = Platform::Graphics::Screen::primaryScreen()->pixelsPerInch(-1);
        int deviceDPI = int(roundf((currentPPI.width() + currentPPI.height()) / 2));
        settings->m_private->setDouble(BlackBerryDevicePixelRatio, deviceDPI / WebCore::ViewportArguments::deprecatedTargetDPI);
    } else
        settings->m_private->setDouble(BlackBerryDevicePixelRatio, 1);

    settings->m_private->setInteger(WebKitDefaultFontSize, 16);
    settings->m_private->setInteger(WebKitDefaultFixedFontSize, 13);
    settings->m_private->setString(WebKitDefaultTextEncodingName, "iso-8859-1");
    settings->m_private->setBoolean(WebKitDownloadableBinaryFontsEnabled, true);
    settings->m_private->setInteger(WebKitFirstScheduledLayoutDelay, 250); // Match Document::cLayoutScheduleThreshold.
    settings->m_private->setBoolean(WebKitJavaScriptEnabled, true);
    settings->m_private->setBoolean(WebKitLoadsImagesAutomatically, true);
    settings->m_private->setUnsignedLongLong(WebKitLocalStorageQuota, 5 * 1024 * 1024);
    settings->m_private->setUnsignedLongLong(WebKitSessionStorageQuota, 5 * 1024 * 1024);
    settings->m_private->setInteger(WebKitMaximumPagesInCache, 0);
    settings->m_private->setInteger(WebKitMinimumFontSize, 8);
    settings->m_private->setBoolean(WebKitWebSocketsEnabled, true);
    settings->m_private->setBoolean(WebKitTextAutosizingEnabled, false);

    settings->m_private->setString(WebKitFixedFontFamily, BlackBerry::Platform::FontInfo::instance()->fontFamily(BlackBerry::Platform::String::fromAscii("-webkit-monospace"), BlackBerry::Platform::String::emptyString()));
    settings->m_private->setString(WebKitSansSeriffFontFamily, BlackBerry::Platform::FontInfo::instance()->fontFamily(BlackBerry::Platform::String::fromAscii("-webkit-sans-serif"), BlackBerry::Platform::String::emptyString()));
    settings->m_private->setString(WebKitSeriffFontFamily, BlackBerry::Platform::FontInfo::instance()->fontFamily(BlackBerry::Platform::String::fromAscii("-webkit-serif"), BlackBerry::Platform::String::emptyString()));
    settings->m_private->setString(WebKitStandardFontFamily, BlackBerry::Platform::FontInfo::instance()->fontFamily(BlackBerry::Platform::String::fromAscii("-webkit-standard"), BlackBerry::Platform::String::emptyString()));

    return settings;
}

void WebSettings::addSupportedObjectPluginMIMEType(const BlackBerry::Platform::String& type)
{
    if (!s_supportedObjectMIMETypes)
        s_supportedObjectMIMETypes = new HashSet<String>;

    s_supportedObjectMIMETypes->add(type);
}

bool WebSettings::isSupportedObjectMIMEType(const BlackBerry::Platform::String& mimeType)
{
    if (mimeType.empty())
        return false;

    if (!s_supportedObjectMIMETypes)
        return false;

    return s_supportedObjectMIMETypes->contains(MIMETypeRegistry::getNormalizedMIMEType(mimeType));
}

bool WebSettings::xssAuditorEnabled() const
{
    return m_private->getBoolean(WebKitXSSAuditorEnabled);
}

void WebSettings::setXSSAuditorEnabled(bool enabled)
{
    return m_private->setBoolean(WebKitXSSAuditorEnabled, enabled);
}

bool WebSettings::loadsImagesAutomatically() const
{
    return m_private->getBoolean(WebKitLoadsImagesAutomatically);
}

void WebSettings::setLoadsImagesAutomatically(bool enabled)
{
    m_private->setBoolean(WebKitLoadsImagesAutomatically, enabled);
}

bool WebSettings::downloadableBinaryFontsEnabled() const
{
    return m_private->getBoolean(WebKitDownloadableBinaryFontsEnabled);
}

void WebSettings::setDownloadableBinaryFontsEnabled(bool enabled)
{
    return m_private->setBoolean(WebKitDownloadableBinaryFontsEnabled, enabled);
}

bool WebSettings::shouldDrawBorderWhileLoadingImages() const
{
    return m_private->getBoolean(BlackBerryDrawBorderWhileLoadingImages);
}

void WebSettings::setShouldDrawBorderWhileLoadingImages(bool enabled)
{
    m_private->setBoolean(BlackBerryDrawBorderWhileLoadingImages, enabled);
}

bool WebSettings::isJavaScriptEnabled() const
{
    return m_private->getBoolean(WebKitJavaScriptEnabled);
}

void WebSettings::setJavaScriptEnabled(bool enabled)
{
    m_private->setBoolean(WebKitJavaScriptEnabled, enabled);
}

bool WebSettings::isPrivateBrowsingEnabled() const
{
    return m_private->getBoolean(WebKitPrivateBrowsingEnabled);
}

void WebSettings::setPrivateBrowsingEnabled(bool enabled)
{
    m_private->setBoolean(WebKitPrivateBrowsingEnabled, enabled);
}

bool WebSettings::deviceSupportsMouse() const
{
    return m_private->getBoolean(WebKitDeviceSupportsMouse);
}

void WebSettings::setDeviceSupportsMouse(bool enabled)
{
    m_private->setBoolean(WebKitDeviceSupportsMouse, enabled);
}

int WebSettings::defaultFixedFontSize() const
{
    return m_private->getInteger(WebKitDefaultFixedFontSize);
}

void WebSettings::setDefaultFixedFontSize(int defaultFixedFontSize)
{
    m_private->setInteger(WebKitDefaultFixedFontSize, defaultFixedFontSize);
}

int WebSettings::defaultFontSize() const
{
    return m_private->getInteger(WebKitDefaultFontSize);
}

void WebSettings::setDefaultFontSize(int defaultFontSize)
{
    m_private->setInteger(WebKitDefaultFontSize, defaultFontSize);
}

int WebSettings::minimumFontSize() const
{
    return m_private->getInteger(WebKitMinimumFontSize);
}

void WebSettings::setMinimumFontSize(int minimumFontSize)
{
    m_private->setInteger(WebKitMinimumFontSize, minimumFontSize);
}

BlackBerry::Platform::String WebSettings::serifFontFamily() const
{
    return m_private->getString(WebKitSeriffFontFamily);
}

void WebSettings::setSerifFontFamily(const BlackBerry::Platform::String& seriffFontFamily)
{
    m_private->setString(WebKitSeriffFontFamily, seriffFontFamily);
}

BlackBerry::Platform::String WebSettings::fixedFontFamily() const
{
    return m_private->getString(WebKitFixedFontFamily);
}

void WebSettings::setFixedFontFamily(const BlackBerry::Platform::String& fixedFontFamily)
{
    m_private->setString(WebKitFixedFontFamily, fixedFontFamily);
}

BlackBerry::Platform::String WebSettings::sansSerifFontFamily() const
{
    return m_private->getString(WebKitSansSeriffFontFamily);
}

void WebSettings::setSansSerifFontFamily(const BlackBerry::Platform::String& sansSeriffFontFamily)
{
    m_private->setString(WebKitSansSeriffFontFamily, sansSeriffFontFamily);
}

BlackBerry::Platform::String WebSettings::standardFontFamily() const
{
    return m_private->getString(WebKitStandardFontFamily);
}

void WebSettings::setStandardFontFamily(const BlackBerry::Platform::String& standardFontFamily)
{
    m_private->setString(WebKitStandardFontFamily, standardFontFamily);
}

BlackBerry::Platform::String WebSettings::userAgentString() const
{
    return m_private->getString(BlackBerryUserAgentString);
}

void WebSettings::setUserAgentString(const BlackBerry::Platform::String& userAgentString)
{
    m_private->setString(BlackBerryUserAgentString, userAgentString);
}

BlackBerry::Platform::String WebSettings::defaultTextEncodingName() const
{
    return m_private->getString(WebKitDefaultTextEncodingName);
}

void WebSettings::setDefaultTextEncodingName(const BlackBerry::Platform::String& defaultTextEncodingName)
{
    m_private->setString(WebKitDefaultTextEncodingName, defaultTextEncodingName);
}

bool WebSettings::isZoomToFitOnLoad() const
{
    return m_private->getBoolean(BlackBerryZoomToFitOnLoadEnabled);
}

void WebSettings::setZoomToFitOnLoad(bool enabled)
{
    m_private->setBoolean(BlackBerryZoomToFitOnLoadEnabled, enabled);
}

WebSettings::TextReflowMode WebSettings::textReflowMode() const
{
    return m_private->getTextReflowMode(BlackBerryTextReflowMode);
}

void WebSettings::setTextReflowMode(TextReflowMode textReflowMode)
{
    m_private->setTextReflowMode(BlackBerryTextReflowMode, textReflowMode);
}

bool WebSettings::isScrollbarsEnabled() const
{
    return m_private->getBoolean(BlackBerryScrollbarsEnabled);
}

void WebSettings::setScrollbarsEnabled(bool enabled)
{
    m_private->setBoolean(BlackBerryScrollbarsEnabled, enabled);
}

bool WebSettings::canJavaScriptOpenWindowsAutomatically() const
{
    return m_private->getBoolean(WebKitJavaScriptCanOpenWindowsAutomaticallyEnabled);
}

void WebSettings::setJavaScriptOpenWindowsAutomatically(bool enabled)
{
    m_private->setBoolean(WebKitJavaScriptCanOpenWindowsAutomaticallyEnabled, enabled);
}

bool WebSettings::arePluginsEnabled() const
{
    return m_private->getBoolean(WebKitPluginsEnabled);
}

void WebSettings::setPluginsEnabled(bool enabled)
{
    m_private->setBoolean(WebKitPluginsEnabled, enabled);
}

bool WebSettings::isGeolocationEnabled() const
{
    return m_private->getBoolean(WebKitGeolocationEnabled);
}

void WebSettings::setGeolocationEnabled(bool enabled)
{
    m_private->setBoolean(WebKitGeolocationEnabled, enabled);
}

bool WebSettings::doesGetFocusNodeContext() const
{
    return m_private->getBoolean(BlackBerryGetFocusNodeContextEnabled);
}

void WebSettings::setGetFocusNodeContext(bool enabled)
{
    m_private->setBoolean(BlackBerryGetFocusNodeContextEnabled, enabled);
}

BlackBerry::Platform::String WebSettings::userStyleSheetLocation()
{
    return m_private->getString(WebKitUserStyleSheetLocation);
}

void WebSettings::setUserStyleSheetLocation(const BlackBerry::Platform::String& userStyleSheetLocation)
{
    m_private->setString(WebKitUserStyleSheetLocation, userStyleSheetLocation);
}

bool WebSettings::areLinksHandledExternally() const
{
    return m_private->getBoolean(BlackBerryLinksHandledExternallyEnabled);
}

void WebSettings::setAreLinksHandledExternally(bool enabled)
{
    m_private->setBoolean(BlackBerryLinksHandledExternallyEnabled, enabled);
}

void WebSettings::setAllowCrossSiteRequests(bool allow)
{
    m_private->setBoolean(BlackBerryAllowCrossSiteRequests, allow);
}

bool WebSettings::allowCrossSiteRequests() const
{
    return m_private->getBoolean(BlackBerryAllowCrossSiteRequests);
}

bool WebSettings::isUserScalable() const
{
    return m_private->getBoolean(BlackBerryUserScalableEnabled);
}

void WebSettings::setUserScalable(bool enabled)
{
    m_private->setBoolean(BlackBerryUserScalableEnabled, enabled);
}

int WebSettings::viewportWidth() const
{
    return m_private->getInteger(BlackBerryViewportWidth);
}

void WebSettings::setViewportWidth(int width)
{
    m_private->setInteger(BlackBerryViewportWidth, width);
}

double WebSettings::initialScale() const
{
    return m_private->getDouble(BlackBerryInitialScale);
}

void WebSettings::setInitialScale(double initialScale)
{
    m_private->setDouble(BlackBerryInitialScale, initialScale);
}

int WebSettings::firstScheduledLayoutDelay() const
{
    return m_private->getInteger(WebKitFirstScheduledLayoutDelay);
}

void WebSettings::setFirstScheduledLayoutDelay(int delay)
{
    m_private->setInteger(WebKitFirstScheduledLayoutDelay, delay);
}

// Whether to include pattern: in the list of string patterns.
bool WebSettings::shouldHandlePatternUrls() const
{
    return m_private->getBoolean(BlackBerryHandlePatternURLs);
}

void WebSettings::setShouldHandlePatternUrls(bool handlePatternURLs)
{
    m_private->setBoolean(BlackBerryHandlePatternURLs, handlePatternURLs);
}

bool WebSettings::areCookiesEnabled() const
{
    return m_private->getBoolean(BlackBerryCookiesEnabled);
}

void WebSettings::setAreCookiesEnabled(bool enable)
{
    m_private->setBoolean(BlackBerryCookiesEnabled, enable);
}

bool WebSettings::isLocalStorageEnabled() const
{
    return m_private->getBoolean(WebKitLocalStorageEnabled);
}

void WebSettings::setIsLocalStorageEnabled(bool enable)
{
    m_private->setBoolean(WebKitLocalStorageEnabled, enable);
}

bool WebSettings::isDatabasesEnabled() const
{
    return m_private->getBoolean(WebKitDatabasesEnabled);
}

void WebSettings::setIsDatabasesEnabled(bool enable)
{
    m_private->setBoolean(WebKitDatabasesEnabled, enable);
}

bool WebSettings::isAppCacheEnabled() const
{
    return m_private->getBoolean(WebKitOfflineWebApplicationCacheEnabled);
}

void WebSettings::setIsAppCacheEnabled(bool enable)
{
    m_private->setBoolean(WebKitOfflineWebApplicationCacheEnabled, enable);
}

unsigned long long WebSettings::localStorageQuota() const
{
    return m_private->getUnsignedLongLong(WebKitLocalStorageQuota);
}

void WebSettings::setLocalStorageQuota(unsigned long long quota)
{
    m_private->setUnsignedLongLong(WebKitLocalStorageQuota, quota);
}

unsigned long long WebSettings::sessionStorageQuota() const
{
    return m_private->getUnsignedLongLong(WebKitSessionStorageQuota);
}

void WebSettings::setSessionStorageQuota(unsigned long long quota)
{
    m_private->setUnsignedLongLong(WebKitSessionStorageQuota, quota);
}

int WebSettings::maximumPagesInCache() const
{
    // FIXME: We shouldn't be calling into WebCore from here. This class should just be a state store.
    return WebCore::pageCache()->capacity();
}

void WebSettings::setMaximumPagesInCache(int pages)
{
    // FIXME: We shouldn't be calling into WebCore from here. This class should just be a state store.
    unsigned realPages = std::max(0, pages);
    WebCore::pageCache()->setCapacity(realPages);
    m_private->setUnsigned(WebKitMaximumPagesInCache, realPages);
}

BlackBerry::Platform::String WebSettings::localStoragePath() const
{
    return m_private->getString(WebKitLocalStoragePath);
}

void WebSettings::setLocalStoragePath(const BlackBerry::Platform::String& path)
{
    m_private->setString(WebKitLocalStoragePath, path);
}

BlackBerry::Platform::String WebSettings::indexedDataBasePath() const
{
    return m_private->getString(WebKitIndexedDataBasePath);
}

void WebSettings::setIndexedDataBasePath(const BlackBerry::Platform::String& path)
{
    m_private->setString(WebKitIndexedDataBasePath, path);
}

BlackBerry::Platform::String WebSettings::databasePath() const
{
    return m_private->getString(WebKitDatabasePath);
}

void WebSettings::setDatabasePath(const BlackBerry::Platform::String& path)
{
    m_private->setString(WebKitDatabasePath, path);
}

BlackBerry::Platform::String WebSettings::appCachePath() const
{
    return m_private->getString(WebKitOfflineWebApplicationCachePath);
}

void WebSettings::setAppCachePath(const BlackBerry::Platform::String& path)
{
    m_private->setString(WebKitOfflineWebApplicationCachePath, path);
}

BlackBerry::Platform::String WebSettings::pageGroupName() const
{
    return m_private->getString(WebKitPageGroupName);
}

void WebSettings::setPageGroupName(const BlackBerry::Platform::String& pageGroupName)
{
    m_private->setString(WebKitPageGroupName, pageGroupName);
}

bool WebSettings::isEmailMode() const
{
    return m_private->getBoolean(BlackBerryEmailModeEnabled);
}

void WebSettings::setEmailMode(bool enable)
{
    m_private->setBoolean(BlackBerryEmailModeEnabled, enable);
}

bool WebSettings::shouldRenderAnimationsOnScrollOrZoom() const
{
    return m_private->getBoolean(BlackBerryRenderAnimationsOnScrollOrZoomEnabled);
}

void WebSettings::setShouldRenderAnimationsOnScrollOrZoom(bool enable)
{
    m_private->setBoolean(BlackBerryRenderAnimationsOnScrollOrZoomEnabled, enable);
}

unsigned WebSettings::overScrollColor() const
{
    return m_private->getUnsigned(BlackBerryOverScrollColor);
}

void WebSettings::setOverScrollColor(unsigned color)
{
    m_private->setUnsigned(BlackBerryOverScrollColor, color);
}

bool WebSettings::isEnableDefaultOverScrollBackground() const
{
    return m_private->getBoolean(BlackBerryEnableDefaultOverScrollBackground);
}

void WebSettings::setEnableDefaultOverScrollBackground(bool enabled)
{
    m_private->setBoolean(BlackBerryEnableDefaultOverScrollBackground, enabled);
}

unsigned WebSettings::backgroundColor() const
{
    return m_private->getUnsigned(BlackBerryBackgroundColor);
}

void WebSettings::setBackgroundColor(unsigned color)
{
    m_private->setUnsigned(BlackBerryBackgroundColor, color);
}

bool WebSettings::isWritingDirectionRTL() const
{
    return m_private->getBoolean(BlackBerryUseRTLWritingDirection);
}

void WebSettings::setWritingDirectionRTL(bool useRTLWritingDirection)
{
    m_private->setBoolean(BlackBerryUseRTLWritingDirection, useRTLWritingDirection);
}

bool WebSettings::useWebKitCache() const
{
    return m_private->getBoolean(BlackBerryUseWebKitCache);
}

void WebSettings::setUseWebKitCache(bool useWebKitCache)
{
    m_private->setBoolean(BlackBerryUseWebKitCache, useWebKitCache);
}

bool WebSettings::isFrameFlatteningEnabled() const
{
    return m_private->getBoolean(WebKitFrameFlatteningEnabled);
}

void WebSettings::setFrameFlatteningEnabled(bool enable)
{
    m_private->setBoolean(WebKitFrameFlatteningEnabled, enable);
}

bool WebSettings::isBackingStoreEnabled() const
{
    return m_private->getBoolean(BlackBerryBackingStoreEnabled);
}

void WebSettings::setBackingStoreEnabled(bool enable)
{
    m_private->setBoolean(BlackBerryBackingStoreEnabled, enable);
}

unsigned WebSettings::maxPluginInstances() const
{
    return m_private->getUnsigned(BlackBerryMaxPluginInstances);
}

void WebSettings::setMaxPluginInstances(unsigned maxPluginInstances)
{
    m_private->setUnsigned(BlackBerryMaxPluginInstances, maxPluginInstances);
}

bool WebSettings::areWebSocketsEnabled() const
{
    return m_private->getBoolean(WebKitWebSocketsEnabled);
}

void WebSettings::setWebSocketsEnabled(bool enable)
{
    m_private->setBoolean(WebKitWebSocketsEnabled, enable);
}

bool WebSettings::isSpatialNavigationEnabled() const
{
    return m_private->getBoolean(SpatialNavigationEnabled);
}

void WebSettings::setSpatialNavigationEnabled(bool enable)
{
    m_private->setBoolean(SpatialNavigationEnabled, enable);
}

bool WebSettings::isAsynchronousSpellCheckingEnabled() const
{
    return m_private->getBoolean(BlackBerryAsynchronousSpellChecking);
}

void WebSettings::setAsynchronousSpellCheckingEnabled(bool enable) const
{
    return m_private->setBoolean(BlackBerryAsynchronousSpellChecking, enable);
}

bool WebSettings::fullScreenVideoCapable() const
{
    return m_private->getBoolean(BlackBerryFullScreenVideoCapable);
}

void WebSettings::setFullScreenVideoCapable(bool enable)
{
    m_private->setBoolean(BlackBerryFullScreenVideoCapable, enable);
}

bool WebSettings::isCredentialAutofillEnabled() const
{
    return m_private->getBoolean(BlackBerryCredentialAutofillEnabled);
}

void WebSettings::setCredentialAutofillEnabled(bool enable)
{
    return m_private->setBoolean(BlackBerryCredentialAutofillEnabled, enable);
}

bool WebSettings::isFormAutofillEnabled() const
{
    return m_private->getBoolean(BlackBerryFormAutofillEnabled);
}

void WebSettings::setFormAutofillEnabled(bool enable)
{
    return m_private->setBoolean(BlackBerryFormAutofillEnabled, enable);
}

double WebSettings::devicePixelRatio() const
{
    return m_private->getDouble(BlackBerryDevicePixelRatio);
}

void WebSettings::setDevicePixelRatio(double ratio)
{
    m_private->setDouble(BlackBerryDevicePixelRatio, ratio);
}

bool WebSettings::applyDeviceScaleFactorInCompositor() const
{
    return m_private->getBoolean(BlackBerryApplyDeviceScaleFactorInCompositor);
}

void WebSettings::setApplyDeviceScaleFactorInCompositor(bool applyDeviceScaleFactorInCompositor)
{
    m_private->setBoolean(BlackBerryApplyDeviceScaleFactorInCompositor, applyDeviceScaleFactorInCompositor);
}

bool WebSettings::isTextAutosizingEnabled() const
{
    return m_private->getBoolean(WebKitTextAutosizingEnabled);
}

void WebSettings::setTextAutosizingEnabled(bool textAutosizingEnabled)
{
    m_private->setBoolean(WebKitTextAutosizingEnabled, textAutosizingEnabled);
}

} // namespace WebKit
} // namespace BlackBerry

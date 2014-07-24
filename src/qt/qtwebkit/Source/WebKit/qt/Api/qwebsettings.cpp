/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)

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
#include "qwebsettings.h"

#include "qwebplugindatabase_p.h"

#include "ApplicationCacheStorage.h"
#include "CrossOriginPreflightResultCache.h"
#include "DatabaseManager.h"
#include "FileSystem.h"
#include "FontCache.h"
#include "GCController.h"
#include "GroupSettings.h"
#include "IconDatabase.h"
#include "Image.h"
#if ENABLE(ICONDATABASE)
#include "IconDatabaseClientQt.h"
#endif
#include "InitWebCoreQt.h"
#include "IntSize.h"
#include "KURL.h"
#include "MemoryCache.h"
#include "NetworkStateNotifier.h"
#include "Page.h"
#include "PageCache.h"
#include "PluginDatabase.h"
#include "RuntimeEnabledFeatures.h"
#include "Settings.h"
#include "StorageThread.h"
#include "WorkerThread.h"
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QGuiApplication>
#include <QHash>
#include <QSharedData>
#include <QStandardPaths>
#include <QUrl>
#include <wtf/FastMalloc.h>
#include <wtf/text/WTFString.h>



QWEBKIT_EXPORT void qt_networkAccessAllowed(bool isAllowed)
{
#ifndef QT_NO_BEARERMANAGEMENT
    WebCore::networkStateNotifier().setNetworkAccessAllowed(isAllowed);
#endif
}

class QWebSettingsPrivate {
public:
    QWebSettingsPrivate(WebCore::Settings* wcSettings = 0, WebCore::GroupSettings* wcGroupSettings = 0)
        : offlineStorageDefaultQuota(0)
        , settings(wcSettings)
        , groupSettings(wcGroupSettings)
    {
    }

    QHash<int, QString> fontFamilies;
    QHash<int, int> fontSizes;
    QHash<int, bool> attributes;
    QUrl userStyleSheetLocation;
    QString defaultTextEncoding;
    QString localStoragePath;
    QString offlineWebApplicationCachePath;
    QString offlineDatabasePath;
    QString mediaType;
    qint64 offlineStorageDefaultQuota;
    QWebSettings::ThirdPartyCookiePolicy thirdPartyCookiePolicy;
    void apply();
    WebCore::Settings* settings;
    WebCore::GroupSettings* groupSettings;
};

Q_GLOBAL_STATIC(QList<QWebSettingsPrivate*>, allSettings);

void QWebSettingsPrivate::apply()
{
    if (settings) {
        settings->setTextAreasAreResizable(true);

        QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;

        QString family = fontFamilies.value(QWebSettings::StandardFont,
                                            global->fontFamilies.value(QWebSettings::StandardFont));
        settings->setStandardFontFamily(family);

        family = fontFamilies.value(QWebSettings::FixedFont,
                                    global->fontFamilies.value(QWebSettings::FixedFont));
        settings->setFixedFontFamily(family);

        family = fontFamilies.value(QWebSettings::SerifFont,
                                    global->fontFamilies.value(QWebSettings::SerifFont));
        settings->setSerifFontFamily(family);

        family = fontFamilies.value(QWebSettings::SansSerifFont,
                                    global->fontFamilies.value(QWebSettings::SansSerifFont));
        settings->setSansSerifFontFamily(family);

        family = fontFamilies.value(QWebSettings::CursiveFont,
                                    global->fontFamilies.value(QWebSettings::CursiveFont));
        settings->setCursiveFontFamily(family);

        family = fontFamilies.value(QWebSettings::FantasyFont,
                                    global->fontFamilies.value(QWebSettings::FantasyFont));
        settings->setFantasyFontFamily(family);

        int size = fontSizes.value(QWebSettings::MinimumFontSize,
                                   global->fontSizes.value(QWebSettings::MinimumFontSize));
        settings->setMinimumFontSize(size);

        size = fontSizes.value(QWebSettings::MinimumLogicalFontSize,
                                   global->fontSizes.value(QWebSettings::MinimumLogicalFontSize));
        settings->setMinimumLogicalFontSize(size);

        size = fontSizes.value(QWebSettings::DefaultFontSize,
                                   global->fontSizes.value(QWebSettings::DefaultFontSize));
        settings->setDefaultFontSize(size);

        size = fontSizes.value(QWebSettings::DefaultFixedFontSize,
                                   global->fontSizes.value(QWebSettings::DefaultFixedFontSize));
        settings->setDefaultFixedFontSize(size);

        bool value = attributes.value(QWebSettings::AutoLoadImages,
                                      global->attributes.value(QWebSettings::AutoLoadImages));
        settings->setLoadsImagesAutomatically(value);

        value = attributes.value(QWebSettings::DnsPrefetchEnabled,
                                 global->attributes.value(QWebSettings::DnsPrefetchEnabled));
        settings->setDNSPrefetchingEnabled(value);

        value = attributes.value(QWebSettings::JavascriptEnabled,
                                      global->attributes.value(QWebSettings::JavascriptEnabled));
        settings->setScriptEnabled(value);
#if USE(ACCELERATED_COMPOSITING)
        value = attributes.value(QWebSettings::AcceleratedCompositingEnabled,
                                      global->attributes.value(QWebSettings::AcceleratedCompositingEnabled));

        settings->setAcceleratedCompositingEnabled(value);

        bool showDebugVisuals = qgetenv("WEBKIT_SHOW_COMPOSITING_DEBUG_VISUALS") == "1";
        settings->setShowDebugBorders(showDebugVisuals);
        settings->setShowRepaintCounter(showDebugVisuals);
#endif
#if ENABLE(WEBGL)
        value = attributes.value(QWebSettings::WebGLEnabled,
                                 global->attributes.value(QWebSettings::WebGLEnabled));

        settings->setWebGLEnabled(value);
#if ENABLE(CSS_SHADERS)
        // For now, enable CSS shaders when WebGL is enabled.
        settings->setCSSCustomFilterEnabled(value);
#endif
#endif
#if ENABLE(WEB_AUDIO)
        value = attributes.value(QWebSettings::WebAudioEnabled, global->attributes.value(QWebSettings::WebAudioEnabled));
        settings->setWebAudioEnabled(value);
#endif

        value = attributes.value(QWebSettings::CSSRegionsEnabled,
                                 global->attributes.value(QWebSettings::CSSRegionsEnabled));
        WebCore::RuntimeEnabledFeatures::setCSSRegionsEnabled(value);

        value = attributes.value(QWebSettings::CSSGridLayoutEnabled,
                                 global->attributes.value(QWebSettings::CSSGridLayoutEnabled));
        settings->setCSSGridLayoutEnabled(value);

        value = attributes.value(QWebSettings::HyperlinkAuditingEnabled,
                                 global->attributes.value(QWebSettings::HyperlinkAuditingEnabled));

        settings->setHyperlinkAuditingEnabled(value);
 
        value = attributes.value(QWebSettings::JavascriptCanOpenWindows,
                                      global->attributes.value(QWebSettings::JavascriptCanOpenWindows));
        settings->setJavaScriptCanOpenWindowsAutomatically(value);

        value = attributes.value(QWebSettings::JavascriptCanCloseWindows,
                                      global->attributes.value(QWebSettings::JavascriptCanCloseWindows));
        settings->setAllowScriptsToCloseWindows(value);

        value = attributes.value(QWebSettings::JavaEnabled,
                                      global->attributes.value(QWebSettings::JavaEnabled));
        settings->setJavaEnabled(value);

        value = attributes.value(QWebSettings::PluginsEnabled,
                                      global->attributes.value(QWebSettings::PluginsEnabled));
        settings->setPluginsEnabled(value);

        value = attributes.value(QWebSettings::PrivateBrowsingEnabled,
                                      global->attributes.value(QWebSettings::PrivateBrowsingEnabled));
        settings->setPrivateBrowsingEnabled(value);

        value = attributes.value(QWebSettings::SpatialNavigationEnabled,
                                      global->attributes.value(QWebSettings::SpatialNavigationEnabled));
        settings->setSpatialNavigationEnabled(value);

        value = attributes.value(QWebSettings::JavascriptCanAccessClipboard,
                                      global->attributes.value(QWebSettings::JavascriptCanAccessClipboard));
        settings->setDOMPasteAllowed(value);
        settings->setJavaScriptCanAccessClipboard(value);

        value = attributes.value(QWebSettings::DeveloperExtrasEnabled,
                                      global->attributes.value(QWebSettings::DeveloperExtrasEnabled));
        settings->setDeveloperExtrasEnabled(value);

        value = attributes.value(QWebSettings::FrameFlatteningEnabled,
                                      global->attributes.value(QWebSettings::FrameFlatteningEnabled));
        settings->setFrameFlatteningEnabled(value);

        QUrl location = !userStyleSheetLocation.isEmpty() ? userStyleSheetLocation : global->userStyleSheetLocation;
        settings->setUserStyleSheetLocation(WebCore::KURL(location));

        QString encoding = !defaultTextEncoding.isEmpty() ? defaultTextEncoding: global->defaultTextEncoding;
        settings->setDefaultTextEncodingName(encoding);

        QString storagePath = !localStoragePath.isEmpty() ? localStoragePath : global->localStoragePath;
        settings->setLocalStorageDatabasePath(storagePath);

        if (mediaType.isEmpty())
            mediaType = global->mediaType;

        value = attributes.value(QWebSettings::PrintElementBackgrounds,
                                      global->attributes.value(QWebSettings::PrintElementBackgrounds));
        settings->setShouldPrintBackgrounds(value);

        value = attributes.value(QWebSettings::OfflineStorageDatabaseEnabled,
                                      global->attributes.value(QWebSettings::OfflineStorageDatabaseEnabled));
#if ENABLE(SQL_DATABASE)
        WebCore::DatabaseManager::manager().setIsAvailable(value);
#endif

#if ENABLE(INDEXED_DATABASE)
        QString path = !offlineDatabasePath.isEmpty() ? offlineDatabasePath : global->offlineDatabasePath;
        Q_ASSERT(groupSettings);
        // Setting the path to empty string disables persistent storage of the indexed database.
        if (!value)
            path = QString();
        groupSettings->setIndexedDBDatabasePath(path);
        qint64 quota = offlineStorageDefaultQuota ? offlineStorageDefaultQuota : global->offlineStorageDefaultQuota;
        groupSettings->setIndexedDBQuotaBytes(quota);
#endif

        value = attributes.value(QWebSettings::OfflineWebApplicationCacheEnabled,
                                      global->attributes.value(QWebSettings::OfflineWebApplicationCacheEnabled));
        settings->setOfflineWebApplicationCacheEnabled(value);

        value = attributes.value(QWebSettings::LocalStorageEnabled,
                                      global->attributes.value(QWebSettings::LocalStorageEnabled));
        settings->setLocalStorageEnabled(value);

        value = attributes.value(QWebSettings::LocalContentCanAccessRemoteUrls,
                                      global->attributes.value(QWebSettings::LocalContentCanAccessRemoteUrls));
        settings->setAllowUniversalAccessFromFileURLs(value);

        value = attributes.value(QWebSettings::LocalContentCanAccessFileUrls,
                                      global->attributes.value(QWebSettings::LocalContentCanAccessFileUrls));
        settings->setAllowFileAccessFromFileURLs(value);

        value = attributes.value(QWebSettings::XSSAuditingEnabled,
                                      global->attributes.value(QWebSettings::XSSAuditingEnabled));
        settings->setXSSAuditorEnabled(value);

#if USE(TILED_BACKING_STORE)
        value = attributes.value(QWebSettings::TiledBackingStoreEnabled,
                                      global->attributes.value(QWebSettings::TiledBackingStoreEnabled));
        settings->setTiledBackingStoreEnabled(value);
#endif

#if ENABLE(SMOOTH_SCROLLING)
        value = attributes.value(QWebSettings::ScrollAnimatorEnabled,
                                      global->attributes.value(QWebSettings::ScrollAnimatorEnabled));
        settings->setScrollAnimatorEnabled(value);
#endif

        value = attributes.value(QWebSettings::CaretBrowsingEnabled,
                                      global->attributes.value(QWebSettings::CaretBrowsingEnabled));
        settings->setCaretBrowsingEnabled(value);

        value = attributes.value(QWebSettings::NotificationsEnabled,
                                      global->attributes.value(QWebSettings::NotificationsEnabled));
        settings->setNotificationsEnabled(value);

        value = attributes.value(QWebSettings::SiteSpecificQuirksEnabled,
                                      global->attributes.value(QWebSettings::SiteSpecificQuirksEnabled));
        settings->setNeedsSiteSpecificQuirks(value);

        settings->setUsesPageCache(WebCore::pageCache()->capacity());

        value = attributes.value(QWebSettings::WebSecurityEnabled,
                                              global->attributes.value(QWebSettings::WebSecurityEnabled));
        settings->setWebSecurityEnabled(value);
    } else {
        QList<QWebSettingsPrivate*> settings = *::allSettings();
        for (int i = 0; i < settings.count(); ++i)
            settings[i]->apply();
    }
}

/*!
    Returns the global settings object.

    Any setting changed on the default object is automatically applied to all
    QWebPage instances where the particular setting is not overriden already.
*/
QWebSettings* QWebSettings::globalSettings()
{
    static QWebSettings* global = 0;
    if (!global) {
        WebCore::initializeWebCoreQt();
        global = new QWebSettings;
    }
    return global;
}

/*!
    \class QWebSettings
    \since 4.4
    \brief The QWebSettings class provides an object to store the settings used
    by QWebPage and QWebFrame.

    \inmodule QtWebKit

    Each QWebPage object has its own QWebSettings object, which configures the
    settings for that page. If a setting is not configured, then it is looked
    up in the global settings object, which can be accessed using
    globalSettings().

    QWebSettings allows configuration of browser properties, such as font sizes and
    families, the location of a custom style sheet, and generic attributes like
    JavaScript and plugins. Individual attributes are set using the setAttribute()
    function. The \l{QWebSettings::WebAttribute}{WebAttribute} enum further describes
    each attribute.

    QWebSettings also configures global properties such as the web page memory
    cache, icon database, local database storage and offline
    applications storage.

    \section1 Enabling Plugins

    Support for browser plugins can enabled by setting the
    \l{QWebSettings::PluginsEnabled}{PluginsEnabled} attribute. For many applications,
    this attribute is enabled for all pages by setting it on the
    \l{globalSettings()}{global settings object}. Qt WebKit will always ignore this setting
    when processing Qt plugins. The decision to allow a Qt plugin is made by the client
    in its reimplementation of QWebPage::createPlugin().

    \section1 Web Application Support

    WebKit provides support for features specified in \l{HTML 5} that improve the
    performance and capabilities of Web applications. These include client-side
    (offline) storage and the use of a Web application cache.

    Client-side (offline) storage is an improvement over the use of cookies to
    store persistent data in Web applications. Applications can configure and
    enable the use of an offline storage database by calling the
    setOfflineStoragePath() with an appropriate file path, and can limit the quota
    for each application by calling setOfflineStorageDefaultQuota().

    \sa QWebPage::settings(), QWebView::settings(), {Tab Browser}
*/

/*!
    \enum QWebSettings::FontFamily

    This enum describes the generic font families defined by CSS 2.
    For more information see the
    \l{http://www.w3.org/TR/REC-CSS2/fonts.html#generic-font-families}{CSS standard}.

    \value StandardFont
    \value FixedFont
    \value SerifFont
    \value SansSerifFont
    \value CursiveFont
    \value FantasyFont
*/

/*!
    \enum QWebSettings::FontSize

    This enum describes the font sizes configurable through QWebSettings.

    \value MinimumFontSize The hard minimum font size.
    \value MinimumLogicalFontSize The minimum logical font size that is applied
        when zooming out with QWebFrame::setTextSizeMultiplier().
    \value DefaultFontSize The default font size for regular text.
    \value DefaultFixedFontSize The default font size for fixed-pitch text.
*/

/*!
    \enum QWebSettings::ThirdPartyCookiePolicy

    This enum describes the policies configurable for accepting and sending
    third-party cookies. These are cookies that are set or retrieved when fetching
    a resource that is stored for a different registry-controlled domain from the page containing it.

    \value AlwaysAllowThirdPartyCookies Allow third-party resources to set and retrieve cookies.
    \value AlwaysBlockThirdPartyCookies Never allow third-party resources to set and retrieve cookies.
    \value AllowThirdPartyWithExistingCookies If the cookie jar already contains cookies
        from a third-party, allow it to set and retrieve new and existing cookies.

    \since QtWebKit 2,3
*/

/*!
    \enum QWebSettings::WebGraphic

    This enums describes the standard graphical elements used in webpages.

    \value MissingImageGraphic The replacement graphic shown when an image could not be loaded.
    \value MissingPluginGraphic The replacement graphic shown when a plugin could not be loaded.
    \value DefaultFrameIconGraphic The default icon for QWebFrame::icon().
    \value TextAreaSizeGripCornerGraphic The graphic shown for the size grip of text areas.
    \value DeleteButtonGraphic The graphic shown for the WebKit-Editing-Delete-Button in Deletion UI.
    \value InputSpeechButtonGraphic The graphic shown in input fields that support speech recognition.
    \value SearchCancelButtonGraphic The graphic shown for clearing the text in a search field.
    \value SearchCancelButtonPressedGraphic The graphic shown when SearchCancelButtonGraphic is pressed.
*/

/*!
    \enum QWebSettings::WebAttribute

    This enum describes various attributes that are configurable through QWebSettings.

    \value AutoLoadImages Specifies whether images are automatically loaded in
        web pages. This is enabled by default.
    \value DnsPrefetchEnabled Specifies whether Qt WebKit will try to pre-fetch DNS entries to
        speed up browsing. This only works as a global attribute. Only for Qt 4.6 and later. This is disabled by default.
    \value JavascriptEnabled Enables or disables the running of JavaScript
        programs. This is enabled by default
    \value JavaEnabled Enables or disables Java applets.
        Currently Java applets are not supported.
    \value PluginsEnabled Enables or disables plugins in Web pages (e.g. using NPAPI). Qt plugins
        with a mimetype such as "application/x-qt-plugin" are not affected by this setting. This is disabled by default.
    \value PrivateBrowsingEnabled Private browsing prevents WebKit from
        recording visited pages in the history and storing web page icons. This is disabled by default.
    \value JavascriptCanOpenWindows Specifies whether JavaScript programs
        can open new windows. This is disabled by default.
    \value JavascriptCanCloseWindows Specifies whether JavaScript programs
        can close windows. This is disabled by default.
    \value JavascriptCanAccessClipboard Specifies whether JavaScript programs
        can read or write to the clipboard. This is disabled by default.
    \value DeveloperExtrasEnabled Enables extra tools for Web developers.
        Currently this enables the "Inspect" element in the context menu as
        well as the use of QWebInspector which controls the web inspector
        for web site debugging. This is disabled by default.
    \value SpatialNavigationEnabled Enables or disables the Spatial Navigation
        feature, which consists in the ability to navigate between focusable
        elements in a Web page, such as hyperlinks and form controls, by using
        Left, Right, Up and Down arrow keys. For example, if a user presses the
        Right key, heuristics determine whether there is an element he might be
        trying to reach towards the right and which element he probably wants.
        This is disabled by default.
    \value LinksIncludedInFocusChain Specifies whether hyperlinks should be
        included in the keyboard focus chain. This is enabled by default.
    \value ZoomTextOnly Specifies whether the zoom factor on a frame applies
        only to the text or to all content. This is disabled by default.
    \value PrintElementBackgrounds Specifies whether the background color and images
        are also drawn when the page is printed. This is enabled by default.
    \value OfflineStorageDatabaseEnabled Specifies whether support for the HTML 5
        offline storage feature is enabled or not. This is disabled by default.
    \value OfflineWebApplicationCacheEnabled Specifies whether support for the HTML 5
        web application cache feature is enabled or not. This is disabled by default.
    \value LocalStorageEnabled Specifies whether support for the HTML 5
        local storage feature is enabled or not. This is disabled by default.
        (This value was introduced in 4.6.)
    \value LocalStorageDatabaseEnabled \e{This enum value is deprecated.} Use
        QWebSettings::LocalStorageEnabled instead.
    \value LocalContentCanAccessRemoteUrls Specifies whether locally loaded documents are
        allowed to access remote urls. This is disabled by default. For more information
        about security origins and local vs. remote content see QWebSecurityOrigin.
        (This value was introduced in 4.6.)
    \value LocalContentCanAccessFileUrls Specifies whether locally loaded documents are
        allowed to access other local urls. This is enabled by default. For more information
        about security origins and local vs. remote content see QWebSecurityOrigin.
    \value XSSAuditingEnabled Specifies whether load requests should be monitored for cross-site
        scripting attempts. Suspicious scripts will be blocked and reported in the inspector's
        JavaScript console. Enabling this feature might have an impact on performance
        and it is disabled by default.
    \value AcceleratedCompositingEnabled This feature, when used in conjunction with
        QGraphicsWebView, accelerates animations of web content. CSS animations of the transform and
        opacity properties will be rendered by composing the cached content of the animated elements.
        This is enabled by default.
    \value TiledBackingStoreEnabled This setting enables the tiled backing store feature
        for a QGraphicsWebView. With the tiled backing store enabled, the web page contents in and around
        the current visible area is speculatively cached to bitmap tiles. The tiles are automatically kept
        in sync with the web page as it changes. Enabling tiling can significantly speed up painting heavy 
        operations like scrolling. Enabling the feature increases memory consumption. It does not work well 
        with contents using CSS fixed positioning (see also \l{QGraphicsWebView::}{resizesToContents} property).
        \l{QGraphicsWebView::}{tiledBackingStoreFrozen} property allows application to temporarily
        freeze the contents of the backing store. This is disabled by default.
    \value FrameFlatteningEnabled With this setting each subframe is expanded to its contents.
        On touch devices, it is desired to not have any scrollable sub parts of the page
        as it results in a confusing user experience, with scrolling sometimes scrolling sub parts
        and at other times scrolling the page itself. For this reason iframes and framesets are
        barely usable on touch devices. This will flatten all the frames to become one scrollable page.
        This is disabled by default.
    \value SiteSpecificQuirksEnabled This setting enables WebKit's workaround for broken sites. It is
        enabled by default.
    \value CSSGridLayoutEnabled This setting enables support for the CSS 3 Grid Layout module. This
        CSS module is currently only a draft and support for it is disabled by default.
    \value ScrollAnimatorEnabled This setting enables animated scrolling. It is disabled by default.
    \value CaretBrowsingEnabled This setting enables caret browsing. It is disabled by default.
    \value NotificationsEnabled Specifies whether support for the HTML 5 web notifications is enabled
        or not. This is enabled by default.
*/

/*!
    \internal
*/
QWebSettings::QWebSettings()
    : d(new QWebSettingsPrivate)
{
    // Initialize our global defaults
    d->fontSizes.insert(QWebSettings::MinimumFontSize, 0);
    d->fontSizes.insert(QWebSettings::MinimumLogicalFontSize, 0);
    d->fontSizes.insert(QWebSettings::DefaultFontSize, 16);
    d->fontSizes.insert(QWebSettings::DefaultFixedFontSize, 13);

    QFont defaultFont;
    defaultFont.setStyleHint(QFont::Serif);
    d->fontFamilies.insert(QWebSettings::StandardFont, defaultFont.defaultFamily());
    d->fontFamilies.insert(QWebSettings::SerifFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Fantasy);
    d->fontFamilies.insert(QWebSettings::FantasyFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Cursive);
    d->fontFamilies.insert(QWebSettings::CursiveFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::SansSerif);
    d->fontFamilies.insert(QWebSettings::SansSerifFont, defaultFont.defaultFamily());

    defaultFont.setStyleHint(QFont::Monospace);
    d->fontFamilies.insert(QWebSettings::FixedFont, defaultFont.defaultFamily());

    d->attributes.insert(QWebSettings::AutoLoadImages, true);
    d->attributes.insert(QWebSettings::DnsPrefetchEnabled, false);
    d->attributes.insert(QWebSettings::JavascriptEnabled, true);
    d->attributes.insert(QWebSettings::SpatialNavigationEnabled, false);
    d->attributes.insert(QWebSettings::LinksIncludedInFocusChain, true);
    d->attributes.insert(QWebSettings::ZoomTextOnly, false);
    d->attributes.insert(QWebSettings::PrintElementBackgrounds, true);
    d->attributes.insert(QWebSettings::OfflineStorageDatabaseEnabled, false);
    d->attributes.insert(QWebSettings::OfflineWebApplicationCacheEnabled, false);
    d->attributes.insert(QWebSettings::LocalStorageEnabled, false);
    d->attributes.insert(QWebSettings::LocalContentCanAccessRemoteUrls, false);
    d->attributes.insert(QWebSettings::LocalContentCanAccessFileUrls, true);
    d->attributes.insert(QWebSettings::AcceleratedCompositingEnabled, true);
    d->attributes.insert(QWebSettings::WebGLEnabled, true);
    d->attributes.insert(QWebSettings::WebAudioEnabled, false);
    d->attributes.insert(QWebSettings::CSSRegionsEnabled, true);
    d->attributes.insert(QWebSettings::CSSGridLayoutEnabled, false);
    d->attributes.insert(QWebSettings::HyperlinkAuditingEnabled, false);
    d->attributes.insert(QWebSettings::TiledBackingStoreEnabled, false);
    d->attributes.insert(QWebSettings::FrameFlatteningEnabled, false);
    d->attributes.insert(QWebSettings::SiteSpecificQuirksEnabled, true);
    d->attributes.insert(QWebSettings::ScrollAnimatorEnabled, false);
    d->attributes.insert(QWebSettings::CaretBrowsingEnabled, false);
    d->attributes.insert(QWebSettings::NotificationsEnabled, true);
    d->attributes.insert(QWebSettings::WebSecurityEnabled, true);
    d->offlineStorageDefaultQuota = 5 * 1024 * 1024;
    d->defaultTextEncoding = QLatin1String("iso-8859-1");
    d->thirdPartyCookiePolicy = AlwaysAllowThirdPartyCookies;
}

/*!
    \internal
*/
QWebSettings::QWebSettings(WebCore::Settings* settings, WebCore::GroupSettings* groupSettings)
    : d(new QWebSettingsPrivate(settings, groupSettings))
{
    d->apply();
    allSettings()->append(d);
}

/*!
    \internal
*/
QWebSettings::~QWebSettings()
{
    if (d->settings)
        allSettings()->removeAll(d);

    delete d;
}

/*!
    Sets the font size for \a type to \a size.
*/
void QWebSettings::setFontSize(FontSize type, int size)
{
    d->fontSizes.insert(type, size);
    d->apply();
}

/*!
    Returns the default font size for \a type.
*/
int QWebSettings::fontSize(FontSize type) const
{
    int defaultValue = 0;
    if (d->settings) {
        QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;
        defaultValue = global->fontSizes.value(type);
    }
    return d->fontSizes.value(type, defaultValue);
}

/*!
    Resets the font size for \a type to the size specified in the global
    settings object.

    This function has no effect on the global QWebSettings instance.
*/
void QWebSettings::resetFontSize(FontSize type)
{
    if (d->settings) {
        d->fontSizes.remove(type);
        d->apply();
    }
}

/*!
    Specifies the location of a user stylesheet to load with every web page.

    The \a location must be either a path on the local filesystem, or a data URL
    with UTF-8 and Base64 encoded data, such as:

    "data:text/css;charset=utf-8;base64,cCB7IGJhY2tncm91bmQtY29sb3I6IHJlZCB9Ow=="

    \note If the base64 data is not valid, the style will not be applied.

    \sa userStyleSheetUrl()
*/
void QWebSettings::setUserStyleSheetUrl(const QUrl& location)
{
    d->userStyleSheetLocation = location;
    d->apply();
}

/*!
    Returns the location of the user stylesheet.

    \sa setUserStyleSheetUrl()
*/
QUrl QWebSettings::userStyleSheetUrl() const
{
    return d->userStyleSheetLocation;
}

/*!
    \since 4.6
    Specifies the default text encoding system.

    The \a encoding, must be a string describing an encoding such as "utf-8",
    "iso-8859-1", etc. If left empty a default value will be used. For a more
    extensive list of encoding names see \l{QTextCodec}

    \sa defaultTextEncoding()
*/
void QWebSettings::setDefaultTextEncoding(const QString& encoding)
{
    d->defaultTextEncoding = encoding;
    d->apply();
}

/*!
    \since 4.6
    Returns the default text encoding.

    \sa setDefaultTextEncoding()
*/
QString QWebSettings::defaultTextEncoding() const
{
    return d->defaultTextEncoding;
}

/*!
    Sets the path of the icon database to \a path. The icon database is used
    to store "favicons" associated with web sites.

    \a path must point to an existing directory.

    Setting an empty path disables the icon database.

    \sa iconDatabasePath(), clearIconDatabase()
*/
void QWebSettings::setIconDatabasePath(const QString& path)
{
    WebCore::initializeWebCoreQt();
#if ENABLE(ICONDATABASE)
    // Make sure that IconDatabaseClientQt is instantiated.
    WebCore::IconDatabaseClientQt::instance();
#endif

    WebCore::IconDatabase::delayDatabaseCleanup();

    WebCore::IconDatabaseBase& db = WebCore::iconDatabase();

    if (!path.isEmpty()) {
        db.setEnabled(true);
        if (db.isOpen())
            db.close();
        QFileInfo info(path);
        if (info.isDir() && info.isWritable())
            db.open(path, WebCore::IconDatabase::defaultDatabaseFilename());
    } else {
        db.setEnabled(false);
        db.close();
    }
}

/*!
    Returns the path of the icon database or an empty string if the icon
    database is disabled.

    \sa setIconDatabasePath(), clearIconDatabase()
*/
QString QWebSettings::iconDatabasePath()
{
    WebCore::initializeWebCoreQt();
    if (WebCore::iconDatabase().isEnabled() && WebCore::iconDatabase().isOpen())
        return WebCore::iconDatabase().databasePath();
    else
        return QString();
}

/*!
    Clears the icon database.
*/
void QWebSettings::clearIconDatabase()
{
    WebCore::initializeWebCoreQt();
    if (WebCore::iconDatabase().isEnabled() && WebCore::iconDatabase().isOpen())
        WebCore::iconDatabase().removeAllIcons();
}

/*!
    Returns the web site's icon for \a url.

    If the web site does not specify an icon \b OR if the icon is not in the
    database, a null QIcon is returned.

    \note The returned icon's size is arbitrary.

    \sa setIconDatabasePath()
*/
QIcon QWebSettings::iconForUrl(const QUrl& url)
{
    WebCore::initializeWebCoreQt();
    QPixmap* icon = WebCore::iconDatabase().synchronousNativeIconForPageURL(WebCore::KURL(url).string(),
                                WebCore::IntSize(16, 16));
    if (!icon)
        return QIcon();

    return* icon;
}

/*
    Returns the plugin database object.

QWebPluginDatabase *QWebSettings::pluginDatabase()
{
    WebCore::initializeWebCoreQt();
    static QWebPluginDatabase* database = 0;
    if (!database)
        database = new QWebPluginDatabase();
    return database;
}
*/

static const char* resourceNameForWebGraphic(QWebSettings::WebGraphic type)
{
    switch (type) {
    case QWebSettings::MissingImageGraphic: return "missingImage";
    case QWebSettings::MissingPluginGraphic: return "nullPlugin";
    case QWebSettings::DefaultFrameIconGraphic: return "urlIcon";
    case QWebSettings::TextAreaSizeGripCornerGraphic: return "textAreaResizeCorner";
    case QWebSettings::DeleteButtonGraphic: return "deleteButton";
    case QWebSettings::InputSpeechButtonGraphic: return "inputSpeech";
    case QWebSettings::SearchCancelButtonGraphic: return "searchCancelButton";
    case QWebSettings::SearchCancelButtonPressedGraphic: return "searchCancelButtonPressed";
    }
    return 0;
}

/*!
    Sets \a graphic to be drawn when Qt WebKit needs to draw an image of the
    given \a type.

    For example, when an image cannot be loaded, the pixmap specified by
    \l{QWebSettings::WebGraphic}{MissingImageGraphic} is drawn instead.

    \sa webGraphic()
*/
void QWebSettings::setWebGraphic(WebGraphic type, const QPixmap& graphic)
{
    WebCore::initializeWebCoreQt();
    WebCore::Image::setPlatformResource(resourceNameForWebGraphic(type), graphic);
}

/*!
    Returns a previously set pixmap used to draw replacement graphics of the
    specified \a type.

    \sa setWebGraphic()
*/
QPixmap QWebSettings::webGraphic(WebGraphic type)
{
    WebCore::initializeWebCoreQt();
    RefPtr<WebCore::Image> img = WebCore::Image::loadPlatformResource(resourceNameForWebGraphic(type));
    if (!img)
        return QPixmap();
    QPixmap* pixmap = img->nativeImageForCurrentFrame();
    if (!pixmap)
        return QPixmap();
    return *pixmap;
}

/*!
    Frees up as much memory as possible by calling the JavaScript garbage collector and cleaning all memory caches such
    as page, object and font cache.

    \since 4.6
 */
void QWebSettings::clearMemoryCaches()
{
    WebCore::initializeWebCoreQt();
    // Turn the cache on and off.  Disabling the object cache will remove all
    // resources from the cache.  They may still live on if they are referenced
    // by some Web page though.
    if (!WebCore::memoryCache()->disabled()) {
        WebCore::memoryCache()->setDisabled(true);
        WebCore::memoryCache()->setDisabled(false);
    }

    int pageCapacity = WebCore::pageCache()->capacity();
    // Setting size to 0, makes all pages be released.
    WebCore::pageCache()->setCapacity(0);
    WebCore::pageCache()->setCapacity(pageCapacity);

    // Invalidating the font cache and freeing all inactive font data.
    WebCore::fontCache()->invalidate();

    // Empty the Cross-Origin Preflight cache
    WebCore::CrossOriginPreflightResultCache::shared().empty();

    // Drop JIT compiled code from ExecutableAllocator.
    WebCore::gcController().discardAllCompiledCode();
    // Garbage Collect to release the references of CachedResource from dead objects.
    WebCore::gcController().garbageCollectNow();

    // FastMalloc has lock-free thread specific caches that can only be cleared from the thread itself.
    WebCore::StorageThread::releaseFastMallocFreeMemoryInAllThreads();
#if ENABLE(WORKERS)
    WebCore::WorkerThread::releaseFastMallocFreeMemoryInAllThreads();
#endif
    WTF::releaseFastMallocFreeMemory();        
}

/*!
    Sets the maximum number of pages to hold in the memory page cache to \a pages.

    The Page Cache allows for a nicer user experience when navigating forth or back
    to pages in the forward/back history, by pausing and resuming up to \a pages.

    For more information about the feature, please refer to:

    http://webkit.org/blog/427/webkit-page-cache-i-the-basics/
*/
void QWebSettings::setMaximumPagesInCache(int pages)
{
    QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;
    WebCore::pageCache()->setCapacity(qMax(0, pages));
    global->apply();
}

/*!
    Returns the maximum number of web pages that are kept in the memory cache.
*/
int QWebSettings::maximumPagesInCache()
{
    WebCore::initializeWebCoreQt();
    return WebCore::pageCache()->capacity();
}

/*!
   Specifies the capacities for the memory cache for dead objects such as
   stylesheets or scripts.

   The \a cacheMinDeadCapacity specifies the \e minimum number of bytes that
   dead objects should consume when the cache is under pressure.

   \a cacheMaxDead is the \e maximum number of bytes that dead objects should
   consume when the cache is \b not under pressure.

   \a totalCapacity specifies the \e maximum number of bytes that the cache
   should consume \b overall.

   The cache is enabled by default. Calling setObjectCacheCapacities(0, 0, 0)
   will disable the cache. Calling it with one non-zero enables it again.
*/
void QWebSettings::setObjectCacheCapacities(int cacheMinDeadCapacity, int cacheMaxDead, int totalCapacity)
{
    WebCore::initializeWebCoreQt();
    bool disableCache = !cacheMinDeadCapacity && !cacheMaxDead && !totalCapacity;
    WebCore::memoryCache()->setDisabled(disableCache);

    WebCore::memoryCache()->setCapacities(qMax(0, cacheMinDeadCapacity),
                                    qMax(0, cacheMaxDead),
                                    qMax(0, totalCapacity));
    WebCore::memoryCache()->setDeadDecodedDataDeletionInterval(disableCache ? 0 : 60);
}

/*!
    Sets the third-party cookie policy, the default is AlwaysAllowThirdPartyCookies.
*/
void QWebSettings::setThirdPartyCookiePolicy(ThirdPartyCookiePolicy policy)
{
    d->thirdPartyCookiePolicy = policy;
}

/*!
    Returns the third-party cookie policy.
*/
QWebSettings::ThirdPartyCookiePolicy QWebSettings::thirdPartyCookiePolicy() const
{
    return d->thirdPartyCookiePolicy;
}

/*!
    Sets the CSS media type to \a type.
    
    Setting this will override the normal value of the CSS media property.
    
    \note Setting the value to null QString will restore the default value.
*/
void QWebSettings::setCSSMediaType(const QString& type)
{
    d->mediaType = type;
    d->apply();
}

/*!
    Returns the current CSS media type.
    
    \note It will only return the value set through setCSSMediaType and not the one used internally.
*/
QString QWebSettings::cssMediaType() const
{
    return d->mediaType;
}

/*!
    Sets the actual font family to \a family for the specified generic family,
    \a which.
*/
void QWebSettings::setFontFamily(FontFamily which, const QString& family)
{
    d->fontFamilies.insert(which, family);
    d->apply();
}

/*!
    Returns the actual font family for the specified generic font family,
    \a which.
*/
QString QWebSettings::fontFamily(FontFamily which) const
{
    QString defaultValue;
    if (d->settings) {
        QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;
        defaultValue = global->fontFamilies.value(which);
    }
    return d->fontFamilies.value(which, defaultValue);
}

/*!
    Resets the actual font family specified by \a which to the one set
    in the global QWebSettings instance.

    This function has no effect on the global QWebSettings instance.
*/
void QWebSettings::resetFontFamily(FontFamily which)
{
    if (d->settings) {
        d->fontFamilies.remove(which);
        d->apply();
    }
}

/*!
    \fn void QWebSettings::setAttribute(WebAttribute attribute, bool on)

    Enables or disables the specified \a attribute feature depending on the
    value of \a on.
*/
void QWebSettings::setAttribute(WebAttribute attr, bool on)
{
    d->attributes.insert(attr, on);
    d->apply();
}

/*!
    \fn bool QWebSettings::testAttribute(WebAttribute attribute) const

    Returns true if \a attribute is enabled; otherwise returns false.
*/
bool QWebSettings::testAttribute(WebAttribute attr) const
{
    bool defaultValue = false;
    if (d->settings) {
        QWebSettingsPrivate* global = QWebSettings::globalSettings()->d;
        defaultValue = global->attributes.value(attr);
    }
    return d->attributes.value(attr, defaultValue);
}

/*!
    \fn void QWebSettings::resetAttribute(WebAttribute attribute)

    Resets the setting of \a attribute to the value specified in the
    global QWebSettings instance.

    This function has no effect on the global QWebSettings instance.

    \sa globalSettings()
*/
void QWebSettings::resetAttribute(WebAttribute attr)
{
    if (d->settings) {
        d->attributes.remove(attr);
        d->apply();
    }
}

/*!
    \since 4.5

    Sets \a path as the save location for HTML5 client-side database storage data.

    \a path must point to an existing directory.

    Setting an empty path disables the feature.

    Support for client-side databases can enabled by setting the
    \l{QWebSettings::OfflineStorageDatabaseEnabled}{OfflineStorageDatabaseEnabled} attribute.

    \sa offlineStoragePath()
*/
void QWebSettings::setOfflineStoragePath(const QString& path)
{
    WebCore::initializeWebCoreQt();
    QWebSettings::globalSettings()->d->offlineDatabasePath = path;
#if ENABLE(SQL_DATABASE)
    WebCore::DatabaseManager::manager().setDatabaseDirectoryPath(path);
#endif
}

/*!
    \since 4.5

    Returns the path of the HTML5 client-side database storage or an empty string if the
    feature is disabled.

    \sa setOfflineStoragePath()
*/
QString QWebSettings::offlineStoragePath()
{
    WebCore::initializeWebCoreQt();
    return QWebSettings::globalSettings()->d->offlineDatabasePath;
}

/*!
    \since 4.5

    Sets the value of the default quota for new offline storage databases
    to \a maximumSize.
*/
void QWebSettings::setOfflineStorageDefaultQuota(qint64 maximumSize)
{
    QWebSettings::globalSettings()->d->offlineStorageDefaultQuota = maximumSize;
}

/*!
    \since 4.5

    Returns the value of the default quota for new offline storage databases.
*/
qint64 QWebSettings::offlineStorageDefaultQuota()
{
    return QWebSettings::globalSettings()->d->offlineStorageDefaultQuota;
}

/*!
    \since 4.6

    Sets the path for HTML5 offline web application cache storage to \a path.

    An application cache acts like an HTTP cache in some sense. For documents
    that use the application cache via JavaScript, the loader engine will
    first ask the application cache for the contents, before hitting the
    network.

    The feature is described in details at:
    http://dev.w3.org/html5/spec/Overview.html#appcache

    \a path must point to an existing directory.

    Setting an empty path disables the feature.

    Support for offline web application cache storage can enabled by setting the
    \l{QWebSettings::OfflineWebApplicationCacheEnabled}{OfflineWebApplicationCacheEnabled} attribute.

    \sa offlineWebApplicationCachePath()
*/
void QWebSettings::setOfflineWebApplicationCachePath(const QString& path)
{
    WebCore::initializeWebCoreQt();
    WebCore::cacheStorage().setCacheDirectory(path);
}

/*!
    \since 4.6

    Returns the path of the HTML5 offline web application cache storage
    or an empty string if the feature is disabled.

    \sa setOfflineWebApplicationCachePath()
*/
QString QWebSettings::offlineWebApplicationCachePath()
{
    WebCore::initializeWebCoreQt();
    return WebCore::cacheStorage().cacheDirectory();
}

/*!
    \since 4.6

    Sets the value of the quota for the offline web application cache
    to \a maximumSize.
*/
void QWebSettings::setOfflineWebApplicationCacheQuota(qint64 maximumSize)
{
    WebCore::initializeWebCoreQt();
    WebCore::cacheStorage().empty();
    WebCore::cacheStorage().vacuumDatabaseFile();
    WebCore::cacheStorage().setMaximumSize(maximumSize);
}

/*!
    \since 4.6

    Returns the value of the quota for the offline web application cache.
*/
qint64 QWebSettings::offlineWebApplicationCacheQuota()
{
    WebCore::initializeWebCoreQt();
    return WebCore::cacheStorage().maximumSize();
}

/*!
    \since 4.6

    Sets the path for HTML5 local storage to \a path.
    
    For more information on HTML5 local storage see the
    \l{http://www.w3.org/TR/webstorage/#the-localstorage-attribute}{Web Storage standard}.
    
    Support for local storage can enabled by setting the
    \l{QWebSettings::LocalStorageEnabled}{LocalStorageEnabled} attribute.     

    \sa localStoragePath()
*/
void QWebSettings::setLocalStoragePath(const QString& path)
{
    d->localStoragePath = path;
    d->apply();
}

/*!
    \since 4.6

    Returns the path for HTML5 local storage.

    \sa setLocalStoragePath()
*/
QString QWebSettings::localStoragePath() const
{
    return d->localStoragePath;
}

/*!
    \since 4.6

    Enables WebKit data persistence and sets the path to \a path.
    If \a path is empty, the user-specific data location specified by
    \l{QDesktopServices::DataLocation}{DataLocation} will be used instead.

    This method will simultaneously set and enable the iconDatabasePath(),
    localStoragePath(), offlineStoragePath() and offlineWebApplicationCachePath().

    \sa localStoragePath()
*/
void QWebSettings::enablePersistentStorage(const QString& path)
{
    WebCore::initializeWebCoreQt();
#ifndef QT_NO_DESKTOPSERVICES
    QString storagePath;

    if (path.isEmpty()) {

        storagePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        if (storagePath.isEmpty())
            storagePath = WebCore::pathByAppendingComponent(QDir::homePath(), QCoreApplication::applicationName());
    } else
        storagePath = path;

    WebCore::makeAllDirectories(storagePath);

    QWebSettings::setIconDatabasePath(storagePath);
    QWebSettings::setOfflineWebApplicationCachePath(storagePath);
    QWebSettings::setOfflineStoragePath(WebCore::pathByAppendingComponent(storagePath, "Databases"));
    QWebSettings::globalSettings()->setLocalStoragePath(WebCore::pathByAppendingComponent(storagePath, "LocalStorage"));
    QWebSettings::globalSettings()->setAttribute(QWebSettings::LocalStorageEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    QWebSettings::globalSettings()->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);

#if ENABLE(NETSCAPE_PLUGIN_METADATA_CACHE)
    // All applications can share the common QtWebkit cache file(s).
    // Path is not configurable and uses QDesktopServices::CacheLocation by default.
    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    WebCore::makeAllDirectories(cachePath);

    QFileInfo info(cachePath);
    if (info.isDir() && info.isWritable()) {
        WebCore::PluginDatabase::setPersistentMetadataCacheEnabled(true);
        WebCore::PluginDatabase::setPersistentMetadataCachePath(cachePath);
    }
#endif
#endif
}

/*!
    \fn QWebSettingsPrivate* QWebSettings::handle() const
    \internal
*/

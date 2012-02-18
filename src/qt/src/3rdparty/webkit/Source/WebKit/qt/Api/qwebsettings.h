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

#ifndef QWEBSETTINGS_H
#define QWEBSETTINGS_H

#include "qwebkitglobal.h"

#include <QtCore/qstring.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qicon.h>
#include <QtCore/qshareddata.h>

namespace WebCore {
    class Settings;
}

class QWebPage;
class QWebPluginDatabase;
class QWebSettingsPrivate;
QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

class QWEBKIT_EXPORT QWebSettings {
public:
    enum FontFamily {
        StandardFont,
        FixedFont,
        SerifFont,
        SansSerifFont,
        CursiveFont,
        FantasyFont
    };
    enum WebAttribute {
        AutoLoadImages,
        JavascriptEnabled,
        JavaEnabled,
        PluginsEnabled,
        PrivateBrowsingEnabled,
        JavascriptCanOpenWindows,
        JavascriptCanAccessClipboard,
        DeveloperExtrasEnabled,
        LinksIncludedInFocusChain,
        ZoomTextOnly,
        PrintElementBackgrounds,
        OfflineStorageDatabaseEnabled,
        OfflineWebApplicationCacheEnabled,
        LocalStorageEnabled,
#if defined(QT_DEPRECATED) || defined(qdoc)
        LocalStorageDatabaseEnabled = LocalStorageEnabled,
#endif
        LocalContentCanAccessRemoteUrls,
        DnsPrefetchEnabled,
        XSSAuditingEnabled,
        AcceleratedCompositingEnabled,
        SpatialNavigationEnabled,
        LocalContentCanAccessFileUrls,
        TiledBackingStoreEnabled,
        FrameFlatteningEnabled,
        SiteSpecificQuirksEnabled,
        JavascriptCanCloseWindows,
        WebGLEnabled,
        HyperlinkAuditingEnabled
    };
    enum WebGraphic {
        MissingImageGraphic,
        MissingPluginGraphic,
        DefaultFrameIconGraphic,
        TextAreaSizeGripCornerGraphic,
        DeleteButtonGraphic,
        InputSpeechButtonGraphic,
        SearchCancelButtonGraphic,
        SearchCancelButtonPressedGraphic
    };
    enum FontSize {
        MinimumFontSize,
        MinimumLogicalFontSize,
        DefaultFontSize,
        DefaultFixedFontSize
    };

    static QWebSettings *globalSettings();

    void setFontFamily(FontFamily which, const QString &family);
    QString fontFamily(FontFamily which) const;
    void resetFontFamily(FontFamily which);

    void setFontSize(FontSize type, int size);
    int fontSize(FontSize type) const;
    void resetFontSize(FontSize type);

    void setAttribute(WebAttribute attr, bool on);
    bool testAttribute(WebAttribute attr) const;
    void resetAttribute(WebAttribute attr);

    void setUserStyleSheetUrl(const QUrl &location);
    QUrl userStyleSheetUrl() const;

    void setDefaultTextEncoding(const QString &encoding);
    QString defaultTextEncoding() const;

    static void setIconDatabasePath(const QString &location);
    static QString iconDatabasePath();
    static void clearIconDatabase();
    static QIcon iconForUrl(const QUrl &url);

    //static QWebPluginDatabase *pluginDatabase();

    static void setWebGraphic(WebGraphic type, const QPixmap &graphic);
    static QPixmap webGraphic(WebGraphic type);

    static void setMaximumPagesInCache(int pages);
    static int maximumPagesInCache();
    static void setObjectCacheCapacities(int cacheMinDeadCapacity, int cacheMaxDead, int totalCapacity);

    static void setOfflineStoragePath(const QString& path);
    static QString offlineStoragePath();
    static void setOfflineStorageDefaultQuota(qint64 maximumSize);
    static qint64 offlineStorageDefaultQuota();

    static void setOfflineWebApplicationCachePath(const QString& path);
    static QString offlineWebApplicationCachePath();
    static void setOfflineWebApplicationCacheQuota(qint64 maximumSize);
    static qint64 offlineWebApplicationCacheQuota();
    
    void setLocalStoragePath(const QString& path);
    QString localStoragePath() const; 

    static void clearMemoryCaches();

    static void enablePersistentStorage(const QString& path = QString());

    inline QWebSettingsPrivate* handle() const { return d; }

private:
    friend class QWebPagePrivate;
    friend class QWebSettingsPrivate;

    Q_DISABLE_COPY(QWebSettings)

    QWebSettings();
    QWebSettings(WebCore::Settings *settings);
    ~QWebSettings();

    QWebSettingsPrivate *d;
};

#endif

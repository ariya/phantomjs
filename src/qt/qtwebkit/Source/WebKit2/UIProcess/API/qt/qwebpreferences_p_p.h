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

#ifndef qwebpreferences_p_p_h
#define qwebpreferences_p_p_h

class QQuickWebViewPrivate;

class QWebPreferencesPrivate {
public:

    enum WebAttribute {
        AutoLoadImages,
        FullScreenEnabled,
        JavascriptEnabled,
        PluginsEnabled,
        OfflineWebApplicationCacheEnabled,
        LocalStorageEnabled,
        XSSAuditingEnabled,
        FrameFlatteningEnabled,
        PrivateBrowsingEnabled,
        DnsPrefetchEnabled,
        DeveloperExtrasEnabled,
        WebGLEnabled,
        CSSCustomFilterEnabled,
        WebAudioEnabled,
        CaretBrowsingEnabled,
        NotificationsEnabled,
        UniversalAccessFromFileURLsAllowed,
        FileAccessFromFileURLsAllowed
    };

    enum FontFamily {
        StandardFont,
        FixedFont,
        SerifFont,
        SansSerifFont,
        CursiveFont,
        FantasyFont
    };

    enum FontSizeType {
        MinimumFontSize,
        DefaultFontSize,
        DefaultFixedFontSize
    };

    static QWebPreferences* createPreferences(QQuickWebViewPrivate*);

    void setAttribute(WebAttribute attr, bool enable);
    bool testAttribute(WebAttribute attr) const;

    void initializeDefaultFontSettings();
    void setFontFamily(FontFamily which, const QString& family);
    QString fontFamily(FontFamily which) const;

    void setFontSize(FontSizeType type, unsigned size);
    unsigned fontSize(FontSizeType type) const;

    QQuickWebViewPrivate* webViewPrivate;

    static QWebPreferencesPrivate* get(QWebPreferences*);
};

#endif // qwebpreferences_p_p_h

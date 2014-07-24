/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef qwebpreferences_p_h
#define qwebpreferences_p_h

#include "qwebkitglobal.h"

#include <QtCore/QObject>

class QWebPreferencesPrivate;

class QWEBKIT_EXPORT QWebPreferences : public QObject {
    Q_OBJECT
public:
    ~QWebPreferences();

    Q_PROPERTY(bool autoLoadImages READ autoLoadImages WRITE setAutoLoadImages NOTIFY autoLoadImagesChanged FINAL)
    Q_PROPERTY(bool fullScreenEnabled READ fullScreenEnabled WRITE setFullScreenEnabled NOTIFY fullScreenEnabledChanged FINAL)
    Q_PROPERTY(bool javascriptEnabled READ javascriptEnabled WRITE setJavascriptEnabled NOTIFY javascriptEnabledChanged FINAL)
    Q_PROPERTY(bool pluginsEnabled READ pluginsEnabled WRITE setPluginsEnabled NOTIFY pluginsEnabledChanged FINAL)
    Q_PROPERTY(bool offlineWebApplicationCacheEnabled READ offlineWebApplicationCacheEnabled WRITE setOfflineWebApplicationCacheEnabled NOTIFY offlineWebApplicationCacheEnabledChanged FINAL)
    Q_PROPERTY(bool localStorageEnabled READ localStorageEnabled WRITE setLocalStorageEnabled NOTIFY localStorageEnabledChanged FINAL)
    Q_PROPERTY(bool xssAuditingEnabled READ xssAuditingEnabled WRITE setXssAuditingEnabled NOTIFY xssAuditingEnabledChanged FINAL)
    Q_PROPERTY(bool privateBrowsingEnabled READ privateBrowsingEnabled WRITE setPrivateBrowsingEnabled NOTIFY privateBrowsingEnabledChanged FINAL)
    Q_PROPERTY(bool dnsPrefetchEnabled READ dnsPrefetchEnabled WRITE setDnsPrefetchEnabled NOTIFY dnsPrefetchEnabledChanged FINAL)
    Q_PROPERTY(bool navigatorQtObjectEnabled READ navigatorQtObjectEnabled WRITE setNavigatorQtObjectEnabled NOTIFY navigatorQtObjectEnabledChanged FINAL)
    Q_PROPERTY(bool frameFlatteningEnabled READ frameFlatteningEnabled WRITE setFrameFlatteningEnabled NOTIFY frameFlatteningEnabledChanged FINAL)
    Q_PROPERTY(bool developerExtrasEnabled READ developerExtrasEnabled WRITE setDeveloperExtrasEnabled NOTIFY developerExtrasEnabledChanged FINAL)
    Q_PROPERTY(bool webGLEnabled READ webGLEnabled WRITE setWebGLEnabled NOTIFY webGLEnabledChanged FINAL)
    Q_PROPERTY(bool webAudioEnabled READ webAudioEnabled WRITE setWebAudioEnabled NOTIFY webAudioEnabledChanged FINAL)
    Q_PROPERTY(bool caretBrowsingEnabled READ caretBrowsingEnabled WRITE setCaretBrowsingEnabled NOTIFY caretBrowsingEnabledChanged FINAL)
    Q_PROPERTY(bool notificationsEnabled READ notificationsEnabled WRITE setNotificationsEnabled NOTIFY notificationsEnabledChanged FINAL)
    Q_PROPERTY(bool universalAccessFromFileURLsAllowed READ universalAccessFromFileURLsAllowed WRITE setUniversalAccessFromFileURLsAllowed NOTIFY universalAccessFromFileURLsAllowedChanged FINAL)
    Q_PROPERTY(bool fileAccessFromFileURLsAllowed READ fileAccessFromFileURLsAllowed WRITE setFileAccessFromFileURLsAllowed NOTIFY fileAccessFromFileURLsAllowedChanged FINAL)

    Q_PROPERTY(QString standardFontFamily READ standardFontFamily WRITE setStandardFontFamily NOTIFY standardFontFamilyChanged FINAL)
    Q_PROPERTY(QString fixedFontFamily READ fixedFontFamily WRITE setFixedFontFamily NOTIFY fixedFontFamilyChanged FINAL)
    Q_PROPERTY(QString serifFontFamily READ serifFontFamily WRITE setSerifFontFamily NOTIFY serifFontFamilyChanged FINAL)
    Q_PROPERTY(QString sansSerifFontFamily READ sansSerifFontFamily WRITE setSansSerifFontFamily NOTIFY sansSerifFontFamilyChanged FINAL)
    Q_PROPERTY(QString cursiveFontFamily READ cursiveFontFamily WRITE setCursiveFontFamily NOTIFY cursiveFontFamilyChanged FINAL)
    Q_PROPERTY(QString fantasyFontFamily READ fantasyFontFamily WRITE setFantasyFontFamily NOTIFY fantasyFontFamilyChanged FINAL)

    Q_PROPERTY(unsigned minimumFontSize READ minimumFontSize WRITE setMinimumFontSize NOTIFY minimumFontSizeChanged FINAL)
    Q_PROPERTY(unsigned defaultFontSize READ defaultFontSize WRITE setDefaultFontSize NOTIFY defaultFontSizeChanged FINAL)
    Q_PROPERTY(unsigned defaultFixedFontSize READ defaultFixedFontSize WRITE setDefaultFixedFontSize NOTIFY defaultFixedFontSizeChanged FINAL)

    bool autoLoadImages() const;
    void setAutoLoadImages(bool enable);

    bool fullScreenEnabled() const;
    void setFullScreenEnabled(bool enable);

    bool javascriptEnabled() const;
    void setJavascriptEnabled(bool enable);

    bool pluginsEnabled() const;
    void setPluginsEnabled(bool enable);

    bool offlineWebApplicationCacheEnabled() const;
    void setOfflineWebApplicationCacheEnabled(bool enable);

    bool localStorageEnabled() const;
    void setLocalStorageEnabled(bool enable);

    bool xssAuditingEnabled() const;
    void setXssAuditingEnabled(bool enable);

    bool privateBrowsingEnabled() const;
    void setPrivateBrowsingEnabled(bool enable);

    bool dnsPrefetchEnabled() const;
    void setDnsPrefetchEnabled(bool enable);

    bool navigatorQtObjectEnabled() const;
    void setNavigatorQtObjectEnabled(bool);

    bool frameFlatteningEnabled() const;
    void setFrameFlatteningEnabled(bool enable);

    bool developerExtrasEnabled() const;
    void setDeveloperExtrasEnabled(bool enable);

    bool webGLEnabled() const;
    void setWebGLEnabled(bool enable);

    bool webAudioEnabled() const;
    void setWebAudioEnabled(bool enable);

    bool caretBrowsingEnabled() const;
    void setCaretBrowsingEnabled(bool enable);

    bool notificationsEnabled() const;
    void setNotificationsEnabled(bool enable);

    bool universalAccessFromFileURLsAllowed() const;
    void setUniversalAccessFromFileURLsAllowed(bool enable);

    bool fileAccessFromFileURLsAllowed() const;
    void setFileAccessFromFileURLsAllowed(bool enable);

    QString standardFontFamily() const;
    void setStandardFontFamily(const QString& family);

    QString fixedFontFamily() const;
    void setFixedFontFamily(const QString& family);

    QString serifFontFamily() const;
    void setSerifFontFamily(const QString& family);

    QString sansSerifFontFamily() const;
    void setSansSerifFontFamily(const QString& family);

    QString cursiveFontFamily() const;
    void setCursiveFontFamily(const QString& family);

    QString fantasyFontFamily() const;
    void setFantasyFontFamily(const QString& family);

    unsigned minimumFontSize() const;
    void setMinimumFontSize(unsigned size);

    unsigned defaultFontSize() const;
    void setDefaultFontSize(unsigned size);

    unsigned defaultFixedFontSize() const;
    void setDefaultFixedFontSize(unsigned size);

Q_SIGNALS:
    void autoLoadImagesChanged();
    void pluginsEnabledChanged();
    void fullScreenEnabledChanged();
    void javascriptEnabledChanged();
    void offlineWebApplicationCacheEnabledChanged();
    void localStorageEnabledChanged();
    void xssAuditingEnabledChanged();
    void privateBrowsingEnabledChanged();
    void dnsPrefetchEnabledChanged();
    void navigatorQtObjectEnabledChanged();
    void frameFlatteningEnabledChanged();
    void developerExtrasEnabledChanged();
    void webGLEnabledChanged();
    void webAudioEnabledChanged();
    void caretBrowsingEnabledChanged();
    void notificationsEnabledChanged();
    void universalAccessFromFileURLsAllowedChanged();
    void fileAccessFromFileURLsAllowedChanged();

    void standardFontFamilyChanged();
    void fixedFontFamilyChanged();
    void serifFontFamilyChanged();
    void sansSerifFontFamilyChanged();
    void cursiveFontFamilyChanged();
    void fantasyFontFamilyChanged();

    void minimumFontSizeChanged();
    void defaultFontSizeChanged();
    void defaultFixedFontSizeChanged();

private:
    Q_DISABLE_COPY(QWebPreferences)

    QWebPreferences();

    QWebPreferencesPrivate *d;

    friend class QWebPreferencesPrivate;
};

#endif // qwebpreferences_p_h

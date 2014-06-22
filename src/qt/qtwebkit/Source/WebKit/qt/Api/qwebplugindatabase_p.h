/*
    Copyright (C) 2009 Jakub Wieczorek <faw217@gmail.com>

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

#ifndef QWEBPLUGINDATABASE_H
#define QWEBPLUGINDATABASE_H

#include "qwebkitglobal.h"
#include "qwebpluginfactory.h"

#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>

namespace WebCore {
    class PluginDatabase;
    class PluginPackage;
}

class QWebPluginInfoPrivate;
class QWEBKIT_EXPORT QWebPluginInfo {
public:
    QWebPluginInfo();
    QWebPluginInfo(const QWebPluginInfo& other);
    QWebPluginInfo &operator=(const QWebPluginInfo& other);
    ~QWebPluginInfo();

private:
    QWebPluginInfo(WebCore::PluginPackage* package);

public:
    typedef QWebPluginFactory::MimeType MimeType;

    QString name() const;
    QString description() const;
    QList<MimeType> mimeTypes() const;
    bool supportsMimeType(const QString& mimeType) const;
    QString path() const;

    bool isNull() const;

    void setEnabled(bool enabled);
    bool isEnabled() const;

    bool operator==(const QWebPluginInfo& other) const;
    bool operator!=(const QWebPluginInfo& other) const;

    friend class QWebPluginDatabase;

private:
    QWebPluginInfoPrivate* d;
    WebCore::PluginPackage* m_package;
    mutable QList<MimeType> m_mimeTypes;
};

class QWebPluginDatabasePrivate;
class QWEBKIT_EXPORT QWebPluginDatabase : public QObject {
    Q_OBJECT

private:
    QWebPluginDatabase(QObject* parent = 0);
    ~QWebPluginDatabase();

public:
    QList<QWebPluginInfo> plugins() const;

    static QStringList defaultSearchPaths();
    QStringList searchPaths() const;
    void setSearchPaths(const QStringList& paths);
    void addSearchPath(const QString& path);

    void refresh();

    QWebPluginInfo pluginForMimeType(const QString& mimeType);
    void setPreferredPluginForMimeType(const QString& mimeType, const QWebPluginInfo& plugin);

    friend class QWebSettings;

private:
    QWebPluginDatabasePrivate* d;
    WebCore::PluginDatabase* m_database;
};

#endif // QWEBPLUGINDATABASE_H

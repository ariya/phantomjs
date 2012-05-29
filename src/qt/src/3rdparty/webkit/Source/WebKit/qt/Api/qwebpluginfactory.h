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

#ifndef QWEBPLUGINFACTORY_H
#define QWEBPLUGINFACTORY_H

#include "qwebkitglobal.h"

#include <QtCore/qobject.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE
class QUrl;
class QString;
QT_END_NAMESPACE
class QWebPluginFactoryPrivate;

class QWEBKIT_EXPORT QWebPluginFactory : public QObject {
    Q_OBJECT
public:
    struct QWEBKIT_EXPORT MimeType {
        QString name;
        QString description;
        QStringList fileExtensions;
        bool operator==(const MimeType& other) const;
        inline bool operator!=(const MimeType& other) const { return !operator==(other); }
    };

    struct Plugin {
        QString name;
        QString description;
        QList<MimeType> mimeTypes;
    };

    explicit QWebPluginFactory(QObject* parent = 0);
    virtual ~QWebPluginFactory();

    virtual QList<Plugin> plugins() const = 0;
    virtual void refreshPlugins();

    virtual QObject *create(const QString& mimeType,
                            const QUrl&,
                            const QStringList& argumentNames,
                            const QStringList& argumentValues) const = 0;

    enum Extension {
    };
    class ExtensionOption
    {};
    class ExtensionReturn
    {};
    virtual bool extension(Extension extension, const ExtensionOption* option = 0, ExtensionReturn* output = 0);
    virtual bool supportsExtension(Extension extension) const;

private:
    QWebPluginFactoryPrivate* d;
};

#endif

/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef QtWebContext_h
#define QtWebContext_h

#include <QScopedPointer>
#include <QtGlobal>
#include <WKContext.h>
#include <WKRetainPtr.h>

namespace WebKit {

class QtDownloadManager;
class QtWebIconDatabaseClient;

class QtWebContext {
public:
    ~QtWebContext();

    enum StorageType {
        DatabaseStorage,
        LocalStorage,
        CookieStorage,
        DiskCacheStorage,
        IconDatabaseStorage
    };

    static QtWebContext* create(WKContextRef);
    static QtWebContext* defaultContext();

    static QString preparedStoragePath(StorageType);

    WKContextRef context() { return m_context.get(); }

    QtDownloadManager* downloadManager() { return m_downloadManager.data(); }
    QtWebIconDatabaseClient* iconDatabase() { return m_iconDatabase.data(); }

private:
    explicit QtWebContext(WKContextRef);

    WKRetainPtr<WKContextRef> m_context;
    QScopedPointer<QtDownloadManager> m_downloadManager;
    QScopedPointer<QtWebIconDatabaseClient> m_iconDatabase;
};

} // namespace WebKit

#endif // QtWebContext_h

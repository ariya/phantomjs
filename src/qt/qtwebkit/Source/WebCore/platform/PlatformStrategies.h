/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PlatformStrategies_h
#define PlatformStrategies_h

namespace WebCore {

class CookiesStrategy;
class DatabaseStrategy;
class LoaderStrategy;
class PasteboardStrategy;
class PluginStrategy;
class SharedWorkerStrategy;
class StorageStrategy;
class VisitedLinkStrategy;

class PlatformStrategies {
public:
    CookiesStrategy* cookiesStrategy()
    {
        if (!m_cookiesStrategy)
            m_cookiesStrategy = createCookiesStrategy();
        return m_cookiesStrategy;
    }

    DatabaseStrategy* databaseStrategy()
    {
        if (!m_databaseStrategy)
            m_databaseStrategy = createDatabaseStrategy();
        return m_databaseStrategy;
    }

    LoaderStrategy* loaderStrategy()
    {
        if (!m_loaderStrategy)
            m_loaderStrategy = createLoaderStrategy();
        return m_loaderStrategy;
    }
    
    PasteboardStrategy* pasteboardStrategy()
    {
        if (!m_pasteboardStrategy)
            m_pasteboardStrategy = createPasteboardStrategy();
        return m_pasteboardStrategy;
    }

    PluginStrategy* pluginStrategy()
    {
        if (!m_pluginStrategy)
            m_pluginStrategy = createPluginStrategy();
        return m_pluginStrategy;
    }

    SharedWorkerStrategy* sharedWorkerStrategy()
    {
        if (!m_sharedWorkerStrategy)
            m_sharedWorkerStrategy = createSharedWorkerStrategy();
        return m_sharedWorkerStrategy;
    }

    VisitedLinkStrategy* visitedLinkStrategy()
    {
        if (!m_visitedLinkStrategy)
            m_visitedLinkStrategy = createVisitedLinkStrategy();
        return m_visitedLinkStrategy;
    }

    StorageStrategy* storageStrategy()
    {
        if (!m_storageStrategy)
            m_storageStrategy = createStorageStrategy();
        return m_storageStrategy;
    }

protected:
    PlatformStrategies()
        : m_cookiesStrategy(0)
        , m_databaseStrategy(0)
        , m_loaderStrategy(0)
        , m_pasteboardStrategy(0)
        , m_pluginStrategy(0)
        , m_sharedWorkerStrategy(0)
        , m_storageStrategy(0)
        , m_visitedLinkStrategy(0)
    {
    }

    virtual ~PlatformStrategies()
    {
    }

private:
    virtual CookiesStrategy* createCookiesStrategy() = 0;
    virtual DatabaseStrategy* createDatabaseStrategy() = 0;
    virtual LoaderStrategy* createLoaderStrategy() = 0;
    virtual PasteboardStrategy* createPasteboardStrategy() = 0;
    virtual PluginStrategy* createPluginStrategy() = 0;
    virtual SharedWorkerStrategy* createSharedWorkerStrategy() = 0;
    virtual StorageStrategy* createStorageStrategy() = 0;
    virtual VisitedLinkStrategy* createVisitedLinkStrategy() = 0;

    CookiesStrategy* m_cookiesStrategy;
    DatabaseStrategy* m_databaseStrategy;
    LoaderStrategy* m_loaderStrategy;
    PasteboardStrategy* m_pasteboardStrategy;
    PluginStrategy* m_pluginStrategy;
    SharedWorkerStrategy* m_sharedWorkerStrategy;
    StorageStrategy* m_storageStrategy;
    VisitedLinkStrategy* m_visitedLinkStrategy;
};

PlatformStrategies* platformStrategies();
void setPlatformStrategies(PlatformStrategies*);
bool hasPlatformStrategies();
    
} // namespace WebCore

#endif // PlatformStrategies_h

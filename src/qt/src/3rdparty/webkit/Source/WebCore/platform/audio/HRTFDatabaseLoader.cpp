/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "HRTFDatabaseLoader.h"

#include "HRTFDatabase.h"

namespace WebCore {

// Singleton
HRTFDatabaseLoader* HRTFDatabaseLoader::s_loader = 0;

PassRefPtr<HRTFDatabaseLoader> HRTFDatabaseLoader::createAndLoadAsynchronouslyIfNecessary(double sampleRate)
{
    ASSERT(isMainThread());

    RefPtr<HRTFDatabaseLoader> loader;
    
    if (!s_loader) {
        // Lazily create and load.
        loader = adoptRef(new HRTFDatabaseLoader(sampleRate));
        s_loader = loader.get();
        loader->loadAsynchronously();
    } else {
        loader = s_loader;
        ASSERT(sampleRate == loader->databaseSampleRate());
    }
    
    return loader;
}

HRTFDatabaseLoader::HRTFDatabaseLoader(double sampleRate)
    : m_databaseLoaderThread(0)
    , m_startedLoadingDatabase(false)
    , m_databaseSampleRate(sampleRate)
{
    ASSERT(isMainThread());
}

HRTFDatabaseLoader::~HRTFDatabaseLoader()
{
    ASSERT(isMainThread());

    if (m_startedLoadingDatabase)
        waitForThreadCompletion(m_databaseLoaderThread, 0);
    
    m_startedLoadingDatabase = false;
    m_databaseLoaderThread = 0;
    
    m_hrtfDatabase.clear();
    
    // Clear out singleton.
    ASSERT(this == s_loader);
    s_loader = 0;
}


// Asynchronously load the database in this thread.
static void* databaseLoaderEntry(void* threadData)
{
    HRTFDatabaseLoader* loader = reinterpret_cast<HRTFDatabaseLoader*>(threadData);
    ASSERT(loader);
    loader->load();
    
    return 0;
}

void HRTFDatabaseLoader::load()
{
    ASSERT(!isMainThread());
    if (!m_hrtfDatabase.get()) {
        // Load the default HRTF database.
        m_hrtfDatabase = HRTFDatabase::create(m_databaseSampleRate);
    }
}

void HRTFDatabaseLoader::loadAsynchronously()
{
    ASSERT(isMainThread());
    
    if (!m_hrtfDatabase.get() && !m_startedLoadingDatabase) {
        // Start the asynchronous database loading process.
        m_startedLoadingDatabase = true;
        m_databaseLoaderThread = createThread(databaseLoaderEntry, this, "HRTF database loader");
    }
}

bool HRTFDatabaseLoader::isLoaded() const
{
    return m_hrtfDatabase.get();
}


void HRTFDatabaseLoader::waitForLoaderThreadCompletion()
{
    ASSERT(!isMainThread());
    ASSERT(m_databaseLoaderThread);
    waitForThreadCompletion(m_databaseLoaderThread, 0);    
}

HRTFDatabase* HRTFDatabaseLoader::defaultHRTFDatabase()
{
    if (!s_loader)
        return 0;
    
    return s_loader->database();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)

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

#ifndef HRTFDatabaseLoader_h
#define HRTFDatabaseLoader_h

#include "HRTFDatabase.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

// HRTFDatabaseLoader will asynchronously load the default HRTFDatabase in a new thread.

class HRTFDatabaseLoader : public RefCounted<HRTFDatabaseLoader> {
public:
    // Lazily creates the singleton HRTFDatabaseLoader (if not already created) and starts loading asynchronously (when created the first time).
    // Returns the singleton HRTFDatabaseLoader.
    // Must be called from the main thread.
    static PassRefPtr<HRTFDatabaseLoader> createAndLoadAsynchronouslyIfNecessary(double sampleRate);

    // Returns the singleton HRTFDatabaseLoader.
    static HRTFDatabaseLoader* loader() { return s_loader; }
    
    // Both constructor and destructor must be called from the main thread.
    ~HRTFDatabaseLoader();
    
    // Returns true once the default database has been completely loaded.
    bool isLoaded() const;

    // May not be called on the main thread.
    // This is so a different background thread may synchronize with the loader thread.
    void waitForLoaderThreadCompletion();
    
    HRTFDatabase* database() { return m_hrtfDatabase.get(); }
    
    // Called in asynchronous loading thread.
    void load();

    // defaultHRTFDatabase() gives access to the loaded database.
    // This can be called from any thread, but it is the callers responsibilty to call this while the context (and thus HRTFDatabaseLoader)
    // is still alive.  Otherwise this will return 0.
    static HRTFDatabase* defaultHRTFDatabase();

private:
    // Both constructor and destructor must be called from the main thread.
    explicit HRTFDatabaseLoader(double sampleRate);    
    
    // If it hasn't already been loaded, creates a new thread and initiates asynchronous loading of the default database.
    // This must be called from the main thread.
    void loadAsynchronously();

    double databaseSampleRate() const { return m_databaseSampleRate; }

    static HRTFDatabaseLoader* s_loader; // singleton
    OwnPtr<HRTFDatabase> m_hrtfDatabase;
    ThreadIdentifier m_databaseLoaderThread;
    bool m_startedLoadingDatabase;
    double m_databaseSampleRate;    
};


} // namespace WebCore

#endif // HRTFDatabaseLoader_h

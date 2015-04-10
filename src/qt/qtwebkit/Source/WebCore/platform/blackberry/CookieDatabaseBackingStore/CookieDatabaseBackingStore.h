/*
 * Copyright (C) 2009 Julien Chaffraix <jchaffraix@pleyo.com>
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CookieDatabaseBackingStore_h
#define CookieDatabaseBackingStore_h

#include "SQLiteDatabase.h"
#include "Timer.h"

#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformTimer.h>
#include <GenericTimerClient.h>
#include <ThreadTimerClient.h>
#include <wtf/ThreadingPrimitives.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ParsedCookie;

class CookieDatabaseBackingStore : public BlackBerry::Platform::MessageClient , public BlackBerry::Platform::ThreadTimerClient {
public:
    static CookieDatabaseBackingStore* create() { return new CookieDatabaseBackingStore; }

    void open(const String& cookieJar);

    void insert(const PassRefPtr<ParsedCookie>);
    void update(const PassRefPtr<ParsedCookie>);
    void remove(const PassRefPtr<ParsedCookie>);

    void removeAll();

    // If a limit is not set, the method will return all cookies in the database
    void getCookiesFromDatabase(Vector<RefPtr<ParsedCookie> >& stackOfCookies, unsigned limit = 0);

    void openAndLoadDatabaseSynchronously(const String& cookieJar);
    void sendChangesToDatabaseSynchronously();

    // MessageClient methods
    virtual void onThreadFinished();

    // ThreadTimerClient methods
    virtual bool willFireTimer() { return true; }
    virtual int getPulseCode() const { return pulseCode(); }
    virtual int getConnectionId() const { return connectionId(); }
    virtual int defaultTimerPriority() const { return threadPriority(); }

private:
    enum UpdateParameter {
        Insert,
        Update,
        Delete,
    };

    CookieDatabaseBackingStore();
    ~CookieDatabaseBackingStore();

    void addToChangeQueue(const PassRefPtr<ParsedCookie> changedCookie, UpdateParameter actionParam);
    void sendChangesToDatabase(int interval);
    void sendChangesToDatabaseTimerFired();

    void invokeOpen(const String& cookieJar);
    void invokeRemoveAll();
    Vector<RefPtr<ParsedCookie> >* invokeGetCookiesWithLimit(unsigned limit);
    void invokeSendChangesToDatabase();

    void close();

    typedef pair<const RefPtr<ParsedCookie>, UpdateParameter> CookieAction;
    Vector<CookieAction> m_changedCookies;
    Mutex m_mutex;

    String m_tableName;
    BlackBerry::Platform::Timer<CookieDatabaseBackingStore> m_dbTimer;
    BlackBerry::Platform::GenericTimerClient* m_dbTimerClient;
    SQLiteDatabase m_db;
    SQLiteStatement *m_insertStatement;
    SQLiteStatement *m_updateStatement;
    SQLiteStatement *m_deleteStatement;
};

CookieDatabaseBackingStore& cookieBackingStore();

} // namespace WebCore

#endif // CookieDatabaseBackingStore_h

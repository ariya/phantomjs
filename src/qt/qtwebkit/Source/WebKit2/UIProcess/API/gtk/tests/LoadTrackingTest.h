/*
 * Copyright (C) 2011 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef LoadTrackingTest_h
#define LoadTrackingTest_h

#include "TestMain.h"
#include "WebViewTest.h"
#include <wtf/Vector.h>

class LoadTrackingTest : public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(LoadTrackingTest);
    LoadTrackingTest();
    virtual ~LoadTrackingTest();
    void waitUntilLoadFinished();

    virtual void provisionalLoadStarted();
    virtual void provisionalLoadReceivedServerRedirect();
    virtual void provisionalLoadFailed(const gchar* failingURI, GError*);
    virtual void loadCommitted();
    virtual void loadFinished();
    virtual void loadFailed(const char* failingURI, GError*);
    virtual void estimatedProgressChanged();

    void loadURI(const char* uri);
    void loadHtml(const char* html, const char* baseURI);
    void loadPlainText(const char* plainText);
    void loadRequest(WebKitURIRequest*);
    void reload();
    void goBack();
    void goForward();

    void setRedirectURI(const char* uri) { m_redirectURI = uri; }

    enum LoadEvents {
        ProvisionalLoadStarted,
        ProvisionalLoadReceivedServerRedirect,
        ProvisionalLoadFailed,
        LoadCommitted,
        LoadFinished,
        LoadFailed
    };
    bool m_runLoadUntilCompletion;
    bool m_loadFailed;
    GOwnPtr<GError> m_error;
    Vector<LoadEvents> m_loadEvents;
    float m_estimatedProgress;
    CString m_redirectURI;
};

#endif // LoadTrackingTest_h

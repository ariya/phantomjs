/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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

#import "WebDocumentLoaderMac.h"

#import "WebKitVersionChecks.h"
#import "WebView.h"

using namespace WebCore;

WebDocumentLoaderMac::WebDocumentLoaderMac(const ResourceRequest& request, const SubstituteData& substituteData)
    : DocumentLoader(request, substituteData)
    , m_dataSource(nil)
    , m_isDataSourceRetained(false)
{
}

static inline bool needsDataLoadWorkaround(WebView *webView)
{
    static bool needsWorkaround = !WebKitLinkedOnOrAfter(WEBKIT_FIRST_VERSION_WITHOUT_ADOBE_INSTALLER_QUIRK) 
                                  && [[[NSBundle mainBundle] bundleIdentifier] isEqualToString:@"com.adobe.Installers.Setup"];
    return needsWorkaround;
}

void WebDocumentLoaderMac::setDataSource(WebDataSource *dataSource, WebView *webView)
{
    ASSERT(!m_dataSource);
    ASSERT(!m_isDataSourceRetained);

    m_dataSource = dataSource;
    retainDataSource();

    m_resourceLoadDelegate = [webView resourceLoadDelegate];
    m_downloadDelegate = [webView downloadDelegate];
    
    // Some clients run the run loop in a way that prevents the data load timer
    // from firing. We work around that issue here. See <rdar://problem/5266289>
    // and <rdar://problem/5049509>.
    if (needsDataLoadWorkaround(webView))
        m_deferMainResourceDataLoad = false;
}

WebDataSource *WebDocumentLoaderMac::dataSource() const
{
    return m_dataSource;
}

void WebDocumentLoaderMac::attachToFrame()
{
    DocumentLoader::attachToFrame();

    retainDataSource();
}

void WebDocumentLoaderMac::detachFromFrame()
{
    DocumentLoader::detachFromFrame();

    if (m_loadingResources.isEmpty())
        releaseDataSource();

    // FIXME: What prevents the data source from getting deallocated while the
    // frame is not attached?
}

void WebDocumentLoaderMac::increaseLoadCount(unsigned long identifier)
{
    ASSERT(m_dataSource);

    if (m_loadingResources.contains(identifier))
        return;
    m_loadingResources.add(identifier);

    retainDataSource();
}

void WebDocumentLoaderMac::decreaseLoadCount(unsigned long identifier)
{
    HashSet<unsigned long>::iterator it = m_loadingResources.find(identifier);
    
    // It is valid for a load to be cancelled before it's started.
    if (it == m_loadingResources.end())
        return;
    
    m_loadingResources.remove(it);
    
    if (m_loadingResources.isEmpty()) {
        m_resourceLoadDelegate = 0;
        m_downloadDelegate = 0;
        if (!frame())
            releaseDataSource();
    }
}

void WebDocumentLoaderMac::retainDataSource()
{
    if (m_isDataSourceRetained || !m_dataSource)
        return;
    m_isDataSourceRetained = true;
    CFRetain(m_dataSource);
}

void WebDocumentLoaderMac::releaseDataSource()
{
    if (!m_isDataSourceRetained)
        return;
    ASSERT(m_dataSource);
    m_isDataSourceRetained = false;
    CFRelease(m_dataSource);
}

void WebDocumentLoaderMac::detachDataSource()
{
    ASSERT(!m_isDataSourceRetained);
    m_dataSource = nil;
}

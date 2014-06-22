/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

#include "config.h"
#include "WebDocumentLoader.h"

#include "WebKitDLL.h"

using namespace WebCore;

WebDocumentLoader::WebDocumentLoader(const ResourceRequest& request, const SubstituteData& substituteData)
    : DocumentLoader(request, substituteData)
    , m_dataSource(0)
    , m_detachedDataSource(0)
{
    gClassCount++;
    gClassNameCount.add("WebDocumentLoader");
}

PassRefPtr<WebDocumentLoader> WebDocumentLoader::create(const ResourceRequest& req, const SubstituteData& data)
{
    return adoptRef(new WebDocumentLoader(req, data));
}

WebDocumentLoader::~WebDocumentLoader()
{
    gClassCount--;
    gClassNameCount.remove("WebDocumentLoader");
    if (m_dataSource) {
        ASSERT(!m_detachedDataSource);
        m_dataSource->Release();
    }
}

void WebDocumentLoader::setDataSource(WebDataSource *dataSource)
{
    ASSERT(!m_dataSource);
    m_dataSource = dataSource;
    if (m_dataSource)
        m_dataSource->AddRef();
}

WebDataSource* WebDocumentLoader::dataSource() const
{
    return m_dataSource;
}

void WebDocumentLoader::detachDataSource()
{
    // we only call detachDataSource when the WebDataSource is freed - and we won't get there if m_dataSource is not
    // null (because that would mean the loader still has a reference to the data source)
    ASSERT(!m_dataSource);
    m_detachedDataSource = 0;
}

void WebDocumentLoader::attachToFrame()
{
    DocumentLoader::attachToFrame();
    if (m_detachedDataSource) {
        ASSERT(!m_dataSource);
        setDataSource(m_detachedDataSource);
        m_detachedDataSource = 0;
    }
}

void WebDocumentLoader::detachFromFrame()
{
    DocumentLoader::detachFromFrame();
    m_detachedDataSource = m_dataSource;
    if (m_dataSource) {
        WebDataSource* dataSourceToBeReleased = m_dataSource;
        // It's important to null out m_dataSource before calling release on the data source.  That release can cause the data
        // source to be deleted - which ends up calling loader->detachDataSource() which makes the assumption that the loader no 
        // longer holds a reference to the data source.
        m_dataSource = 0;
        dataSourceToBeReleased->Release();
    }
}

/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2009  Jan Michael Alonzo
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

#ifndef DocumentLoaderGtk_h
#define DocumentLoaderGtk_h

#include "DocumentLoader.h"
#include "webkitdefines.h"
#include "wtf/HashSet.h"

namespace WebCore {
    class ResourceRequest;
    class SubstituteData;
}

namespace WebKit {

class DocumentLoader : public WebCore::DocumentLoader {
public:
    static PassRefPtr<WebKit::DocumentLoader> create(const WebCore::ResourceRequest& request, const WebCore::SubstituteData& data)
    {
        return adoptRef(new DocumentLoader(request, data));
    }

    void setDataSource(WebKitWebDataSource*);
    void detachDataSource();
    WebKitWebDataSource* dataSource() const { return m_dataSource; }

    void increaseLoadCount(unsigned long identifier);
    void decreaseLoadCount(unsigned long identifier);

private:
    DocumentLoader(const WebCore::ResourceRequest&, const WebCore::SubstituteData&);

    virtual void attachToFrame();
    virtual void detachFromFrame();

    void refDataSource();
    void unrefDataSource();

    bool m_isDataSourceReffed;
    WebKitWebDataSource* m_dataSource;
    HashSet<unsigned long> m_loadingResources;
};

} // end namespace WebKit

#endif

/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
#include "ArchiveResourceCollection.h"

namespace WebCore {

ArchiveResourceCollection::ArchiveResourceCollection()
{
}

void ArchiveResourceCollection::addAllResources(Archive* archive)
{
    ASSERT(archive);
    if (!archive)
        return;
    
    const Vector<RefPtr<ArchiveResource> >& subresources = archive->subresources();
    Vector<RefPtr<ArchiveResource> >::const_iterator iRes = subresources.begin();
    Vector<RefPtr<ArchiveResource> >::const_iterator endRes = subresources.end();
    
    for (; iRes != endRes; ++iRes)
        m_subresources.set((*iRes)->url(), iRes->get());

    const Vector<RefPtr<Archive> >& subframes = archive->subframeArchives();
    Vector<RefPtr<Archive> >::const_iterator iFrame = subframes.begin();
    Vector<RefPtr<Archive> >::const_iterator endFrame = subframes.end();
        
    for (; iFrame != endFrame; ++iFrame) {        
        ASSERT((*iFrame)->mainResource());
        const String& frameName = (*iFrame)->mainResource()->frameName();
        if (!frameName.isNull())
            m_subframes.set(frameName, iFrame->get());
    }
}
    
// FIXME: Adding a resource directly to a DocumentLoader/ArchiveResourceCollection seems like bad design, but is API some apps rely on.
// Can we change the design in a manner that will let us deprecate that API without reducing functionality of those apps?
void ArchiveResourceCollection::addResource(PassRefPtr<ArchiveResource> resource)
{
    ASSERT(resource);
    if (!resource)
        return;

    const KURL& url = resource->url(); // get before passing PassRefPtr (which sets it to 0)
    m_subresources.set(url, resource);
}

ArchiveResource* ArchiveResourceCollection::archiveResourceForURL(const KURL& url)
{
    ArchiveResource* resource = m_subresources.get(url).get();
    if (!resource)
        return 0;
        
    return resource;
}

PassRefPtr<Archive> ArchiveResourceCollection::popSubframeArchive(const String& frameName)
{
    return m_subframes.take(frameName);
}

}

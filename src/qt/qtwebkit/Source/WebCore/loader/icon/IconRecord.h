/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
 
#ifndef IconRecord_h
#define IconRecord_h

#include "PageURLRecord.h"
#include "SharedBuffer.h"
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

#if OS(SOLARIS)
#include <sys/types.h> // For time_t structure.
#endif

namespace WebCore { 

class IconDataSnapshot;
class Image;
class IntSize;
class SQLDatabase;

enum ImageDataStatus {
    ImageDataStatusPresent, ImageDataStatusMissing, ImageDataStatusUnknown
};

class IconSnapshot {
public:
    IconSnapshot() : m_timestamp(0) { }
    
    IconSnapshot(const String& iconURL, int timestamp, SharedBuffer* data)
        : m_iconURL(iconURL)
        , m_timestamp(timestamp)
        , m_data(data)
    { }

    const String& iconURL() const { return m_iconURL; }
    int timestamp() const { return m_timestamp; }
    SharedBuffer* data() const { return m_data.get(); }

private:
    String m_iconURL;
    int m_timestamp;
    RefPtr<SharedBuffer> m_data;
};
    
class IconRecord : public RefCounted<IconRecord> {
    friend class PageURLRecord;
public:
    static PassRefPtr<IconRecord> create(const String& url)
    {
        return adoptRef(new IconRecord(url));
    }
    ~IconRecord();
    
    time_t getTimestamp() { return m_stamp; }
    void setTimestamp(time_t stamp) { m_stamp = stamp; }
        
    void setImageData(PassRefPtr<SharedBuffer> data);
    Image* image(const IntSize&);    
    
    String iconURL() { return m_iconURL; }

    void loadImageFromResource(const char*);
        
    ImageDataStatus imageDataStatus();
    
    const HashSet<String>& retainingPageURLs() { return m_retainingPageURLs; }
    
    IconSnapshot snapshot(bool forDeletion = false) const;

private:
    IconRecord(const String& url); 

    String m_iconURL;
    time_t m_stamp;
    RefPtr<Image> m_image;
    
    HashSet<String> m_retainingPageURLs;
        
    // This allows us to cache whether or not a SiteIcon has had its data set yet
    // This helps the IconDatabase know if it has to set the data on a new object or not,
    // and also to determine if the icon is missing data or if it just hasn't been brought
    // in from the DB yet
    bool m_dataSet;
    
    // FIXME - Right now WebCore::Image doesn't have a very good API for accessing multiple representations
    // Even the NSImage way of doing things that we do in WebKit isn't very clean...  once we come up with a 
    // better way of handling that, we'll likely have a map of size-to-images similar to below
    // typedef HashMap<IntSize, Image*> SizeImageMap;
    // SizeImageMap m_images;
};


} //namespace WebCore

#endif

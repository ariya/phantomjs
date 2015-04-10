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

#include "config.h"
#include "IconRecord.h"

#include "BitmapImage.h"
#include "IconDatabase.h"
#include "Logging.h"
#include "SQLiteStatement.h"
#include "SQLiteTransaction.h"
#include <wtf/text/CString.h>

#include <limits.h>

namespace WebCore {

IconRecord::IconRecord(const String& url)
    : m_iconURL(url)
    , m_stamp(0)
    , m_dataSet(false)
{

}

IconRecord::~IconRecord()
{
    LOG(IconDatabase, "Destroying IconRecord for icon url %s", m_iconURL.ascii().data());
}

Image* IconRecord::image(const IntSize&)
{
    // FIXME rdar://4680377 - For size right now, we are returning our one and only image and the Bridge
    // is resizing it in place.  We need to actually store all the original representations here and return a native
    // one, or resize the best one to the requested size and cache that result.
    
    return m_image.get();
}

void IconRecord::setImageData(PassRefPtr<SharedBuffer> data)
{
    // It's okay to delete the raw image here. Any existing clients using this icon will be
    // managing an image that was created with a copy of this raw image data.
    m_image = BitmapImage::create();

    // Copy the provided data into the buffer of the new Image object.
    if (!m_image->setData(data, true)) {
        LOG(IconDatabase, "Manual image data for iconURL '%s' FAILED - it was probably invalid image data", m_iconURL.ascii().data());
        m_image.clear();
    }
    
    m_dataSet = true;
}

void IconRecord::loadImageFromResource(const char* resource)
{
    if (!resource)
        return;
        
    m_image = Image::loadPlatformResource(resource);
    m_dataSet = true;
}

ImageDataStatus IconRecord::imageDataStatus()
{
    if (!m_dataSet)
        return ImageDataStatusUnknown;
    if (!m_image)
        return ImageDataStatusMissing;
    return ImageDataStatusPresent;
}

IconSnapshot IconRecord::snapshot(bool forDeletion) const
{
    if (forDeletion)
        return IconSnapshot(m_iconURL, 0, 0);
    
    return IconSnapshot(m_iconURL, m_stamp, m_image ? m_image->data() : 0);
}

} // namespace WebCore    

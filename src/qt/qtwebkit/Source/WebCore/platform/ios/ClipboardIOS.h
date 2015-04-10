/*
 * Copyright (C) 2004, 2006, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ClipboardIOS_h
#define ClipboardIOS_h

#include "CachedResourceClient.h"
#include "Clipboard.h"
#include <wtf/RetainPtr.h>

namespace WebCore {

class Frame;
class FileList;

class ClipboardIOS : public Clipboard, public CachedResourceClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<ClipboardIOS> create(ClipboardType clipboardType, ClipboardAccessPolicy policy, Frame* frame)
    {
        return adoptRef(new ClipboardIOS(clipboardType, policy, frame));
    }

    virtual ~ClipboardIOS();

    virtual void clearData(const String& type) OVERRIDE;
    virtual void clearData() OVERRIDE;
    virtual String getData(const String& type) const OVERRIDE;
    virtual bool setData(const String& type, const String& data) OVERRIDE;

    virtual bool hasData() OVERRIDE;

    // Extensions beyond IE's API.
    virtual ListHashSet<String> types() const OVERRIDE;
    virtual PassRefPtr<FileList> files() const OVERRIDE;

    virtual void setDragImage(CachedImage*, const IntPoint&) OVERRIDE;
    virtual void setDragImageElement(Node *, const IntPoint&) OVERRIDE;

    virtual DragImageRef createDragImage(IntPoint& dragLoc) const OVERRIDE;
#if ENABLE(DRAG_SUPPORT)
    virtual void declareAndWriteDragImage(Element*, const KURL&, const String& title, Frame*) OVERRIDE;
#endif
    virtual void writeRange(Range*, Frame*) OVERRIDE;
    virtual void writeURL(const KURL&, const String&, Frame*) OVERRIDE;
    virtual void writePlainText(const String&) OVERRIDE;

private:
    ClipboardIOS(ClipboardType, ClipboardAccessPolicy, Frame*);

    int m_changeCount;
    Frame* m_frame; // Used to access the UIPasteboard through the editor client.
};

}

#endif

/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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

#ifndef ClipboardQt_h
#define ClipboardQt_h

#include "CachedResourceClient.h"
#include "Clipboard.h"

QT_BEGIN_NAMESPACE
class QMimeData;
QT_END_NAMESPACE

namespace WebCore {

class CachedImage;

// State available during IE's events for drag and drop and copy/paste
class ClipboardQt : public Clipboard, public CachedResourceClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<ClipboardQt> create(ClipboardAccessPolicy policy, const QMimeData* readableClipboard, Frame* frame)
    {
        return adoptRef(new ClipboardQt(policy, readableClipboard, frame));
    }
    static PassRefPtr<ClipboardQt> create(ClipboardAccessPolicy policy, Frame *frame, ClipboardType clipboardType = CopyAndPaste)
    {
        return adoptRef(new ClipboardQt(policy, clipboardType, frame));
    }
    virtual ~ClipboardQt();

    void clearData(const String& type);
    void clearAllData();
    String getData(const String& type, bool& success) const;
    bool setData(const String& type, const String& data);

    // extensions beyond IE's API
    virtual HashSet<String> types() const;
    virtual PassRefPtr<FileList> files() const;

    void setDragImage(CachedImage*, const IntPoint&);
    void setDragImageElement(Node*, const IntPoint&);

    virtual DragImageRef createDragImage(IntPoint& dragLoc) const;
    virtual void declareAndWriteDragImage(Element*, const KURL&, const String& title, Frame*);
    virtual void writeURL(const KURL&, const String&, Frame*);
    virtual void writeRange(Range*, Frame*);
    virtual void writePlainText(const String&);

    virtual bool hasData();

    QMimeData* clipboardData() const { return m_writableData; }
    void invalidateWritableData() { m_writableData = 0; }

#if ENABLE(DATA_TRANSFER_ITEMS)
    virtual PassRefPtr<DataTransferItems> items();
#endif

private:
    ClipboardQt(ClipboardAccessPolicy, const QMimeData* readableClipboard, Frame*);

    // Clipboard is writable so it will create its own QMimeData object
    ClipboardQt(ClipboardAccessPolicy, ClipboardType, Frame*);

    void setDragImage(CachedImage*, Node*, const IntPoint& loc);

    const QMimeData* m_readableData;
    QMimeData* m_writableData;
    Frame* m_frame;
};
}

#endif // ClipboardQt_h

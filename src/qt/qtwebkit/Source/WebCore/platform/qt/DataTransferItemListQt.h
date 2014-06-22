/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef DataTransferItemListQt_h
#define DataTransferItemListQt_h

#if ENABLE(DATA_TRANSFER_ITEMS)

#include "DataTransferItemList.h"
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class Clipboard;
class DataTransferItemQt;
class ScriptExecutionContext;

class DataTransferItemListQt : public DataTransferItemList {
public:
    static PassRefPtr<DataTransferItemListQt> create(PassRefPtr<Clipboard>, ScriptExecutionContext*);

    virtual size_t length() const;
    virtual PassRefPtr<DataTransferItem> item(unsigned long index);
    virtual void deleteItem(unsigned long index, ExceptionCode&);
    virtual void clear();
    virtual void add(const String& data, const String& type, ExceptionCode&);
    virtual void add(PassRefPtr<File>);


    friend class ClipboardQt;
private:
    DataTransferItemListQt(PassRefPtr<Clipboard>, ScriptExecutionContext*);

    virtual void addPasteboardItem(const String& type);

    RefPtr<Clipboard> m_owner;
    // Indirectly owned by our parent.
    ScriptExecutionContext* m_context;
    Vector<RefPtr<DataTransferItem> > m_items;
};

}

#endif // ENABLE(DATA_TRANSFER_ITEMS)

#endif

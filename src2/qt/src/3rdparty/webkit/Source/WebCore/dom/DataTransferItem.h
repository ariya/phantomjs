/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DataTransferItem_h
#define DataTransferItem_h

#if ENABLE(DATA_TRANSFER_ITEMS)

#include "Clipboard.h"

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class Blob;
class StringCallback;
class ScriptExecutionContext;

class DataTransferItem : public RefCounted<DataTransferItem> {
public:
    ~DataTransferItem() {}

    static PassRefPtr<DataTransferItem> create(PassRefPtr<Clipboard> owner, ScriptExecutionContext*, const String& data, const String& type);

    static const char kindString[];
    static const char kindFile[];

    String kind() const;
    String type() const;

    virtual void getAsString(PassRefPtr<StringCallback>) = 0;
    virtual PassRefPtr<Blob> getAsFile() = 0;

protected:
    DataTransferItem(RefPtr<Clipboard> owner, const String& kind, const String& type);
    Clipboard* owner();

private:
    const RefPtr<Clipboard> m_owner;
    const String m_kind;
    const String m_type;
};

} // namespace WebCore

#endif // ENABLE(DATA_TRANSFER_ITEMS)

#endif // DataTransferItem_h

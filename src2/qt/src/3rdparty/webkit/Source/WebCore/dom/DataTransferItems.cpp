/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#include "config.h"
#include "DataTransferItems.h"

#include "DataTransferItem.h"
#include "ExceptionCode.h"

#if ENABLE(DATA_TRANSFER_ITEMS)

namespace WebCore {

DataTransferItems::DataTransferItems(RefPtr<Clipboard> clipboard, ScriptExecutionContext* context)
    : m_owner(clipboard)
    , m_context(context)
{
}

size_t DataTransferItems::length() const
{
    if (m_owner->policy() == ClipboardNumb)
        return 0;

    return m_items.size();
}

PassRefPtr<DataTransferItem> DataTransferItems::item(unsigned long index)
{
    if (m_owner->policy() == ClipboardNumb || index >= length())
        return 0;

    return m_items[index];
}

void DataTransferItems::deleteItem(unsigned long index, ExceptionCode& ec)
{
    if (m_owner->policy() != ClipboardWritable) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (index >= length())
        return;

    m_items.remove(index);
}

void DataTransferItems::clear()
{
    if (m_owner->policy() != ClipboardWritable)
        return;

    m_items.clear();

}

void DataTransferItems::add(const String& data, const String& type, ExceptionCode& ec)
{
    if (m_owner->policy() != ClipboardWritable)
        return;

    // Only one 'string' item with a given type is allowed in the collection.
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->type() == type && m_items[i]->kind() == DataTransferItem::kindString) {
            ec = INVALID_STATE_ERR;
            return;
        }
    }

    m_items.append(DataTransferItem::create(m_owner, m_context, data, type));
}

}

#endif

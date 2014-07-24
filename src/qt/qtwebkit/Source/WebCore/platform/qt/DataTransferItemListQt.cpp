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

#include "config.h"
#include "DataTransferItemListQt.h"

#if ENABLE(DATA_TRANSFER_ITEMS)

#include "Clipboard.h"
#include "DataTransferItemQt.h"
#include "ExceptionCode.h"

namespace WebCore {

PassRefPtr<DataTransferItemListQt> DataTransferItemListQt::create(PassRefPtr<Clipboard> owner, ScriptExecutionContext* context)
{
    return adoptRef(new DataTransferItemListQt(owner, context));
}

DataTransferItemListQt::DataTransferItemListQt(PassRefPtr<Clipboard> owner, ScriptExecutionContext* context)
    : m_owner(clipboard)
    , m_context(context)
{
}

size_t DataTransferItemListQt::length() const
{
    if (!m_owner->canReadTypes())
        return 0;

    return m_items.size();
}

PassRefPtr<DataTransferItem> DataTransferItemListQt::item(unsigned long index)
{
    if (!m_owner->canReadTypes() || index >= length())
        return 0;

    return m_items[index];
}

void DataTransferItemListQt::deleteItem(unsigned long index, ExceptionCode& ec)
{
    if (!m_owner->canWriteData()) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (index >= length())
        return;

    m_items.remove(index);
}

void DataTransferItemListQt::clear()
{
    if (!m_owner->canWriteData())
        return;

    m_items.clear();

}

void DataTransferItemListQt::add(const String& data, const String& type, ExceptionCode& ec)
{
    if (!m_owner->canWriteData())
        return;

    // Only one 'string' item with a given type is allowed in the collection.
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i]->type() == type && m_items[i]->kind() == DataTransferItem::kindString) {
            ec = NOT_SUPPORTED_ERR;
            return;
        }
    }

    m_items.append(DataTransferItem::create(m_owner, m_context, data, type));
}

void DataTransferItemListQt::add(PassRefPtr<File> file)
{
    if (!m_owner->canWriteData() || !file)
        return;

    m_items.append(DataTransferItem::create(m_owner, m_context, file));
}

void DataTransferItemListQt::addPasteboardItem(const String& type)
{
    m_items.append(DataTransferItemQt::createFromPasteboard(m_owner, m_context, type));
}

} // namespace WebCore

#endif // ENABLE(DATA_TRANSFER_ITEMS)

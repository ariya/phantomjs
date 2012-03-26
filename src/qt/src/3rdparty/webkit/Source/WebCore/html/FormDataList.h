/*
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef FormDataList_h
#define FormDataList_h

#include "Blob.h"
#include "TextEncoding.h"
#include <wtf/Forward.h>
#include <wtf/text/CString.h>

namespace WebCore {

class FormDataList {
public:
    class Item {
    public:
        Item() { }
        Item(const WTF::CString& data) : m_data(data) { }
        Item(PassRefPtr<Blob> blob) : m_blob(blob) { }

        const WTF::CString& data() const { return m_data; }
        Blob* blob() const { return m_blob.get(); }

    private:
        WTF::CString m_data;
        RefPtr<Blob> m_blob;
    };

    FormDataList(const TextEncoding&);

    void appendData(const String& key, const String& value)
    {
        appendString(key);
        appendString(value);
    }
    void appendData(const String& key, const CString& value)
    {
        appendString(key);
        appendString(value);
    }
    void appendData(const String& key, int value)
    {
        appendString(key);
        appendString(String::number(value));
    }
    void appendBlob(const String& key, PassRefPtr<Blob> blob)
    {
        appendString(key);
        appendBlob(blob);
    }

    const Vector<Item>& items() const { return m_items; }
    const TextEncoding& encoding() const { return m_encoding; }

private:
    void appendString(const CString&);
    void appendString(const String&);
    void appendBlob(PassRefPtr<Blob>);

    TextEncoding m_encoding;
    Vector<Item> m_items;
};

} // namespace WebCore

#endif // FormDataList_h

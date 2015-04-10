/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StorageMap_h
#define StorageMap_h

#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class StorageMap : public RefCounted<StorageMap> {
public:
    // Quota size measured in bytes.
    static PassRefPtr<StorageMap> create(unsigned quotaSize);

    unsigned length() const;
    String key(unsigned index);
    String getItem(const String&) const;
    PassRefPtr<StorageMap> setItem(const String& key, const String& value, String& oldValue, bool& quotaException);
    PassRefPtr<StorageMap> setItemIgnoringQuota(const String& key, const String& value);
    PassRefPtr<StorageMap> removeItem(const String&, String& oldValue);

    bool contains(const String& key) const;

    void importItems(const HashMap<String, String>&);
    const HashMap<String, String>& items() const { return m_map; }

    unsigned quota() const { return m_quotaSize; }

    static const unsigned noQuota = UINT_MAX;

private:
    explicit StorageMap(unsigned quota);
    PassRefPtr<StorageMap> copy();
    void invalidateIterator();
    void setIteratorToIndex(unsigned);

    HashMap<String, String> m_map;
    HashMap<String, String>::iterator m_iterator;
    unsigned m_iteratorIndex;

    unsigned m_quotaSize; // Measured in bytes.
    unsigned m_currentLength; // Measured in UChars.
};

} // namespace WebCore

#endif // StorageMap_h

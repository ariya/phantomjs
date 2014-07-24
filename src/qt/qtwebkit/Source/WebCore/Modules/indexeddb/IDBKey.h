/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef IDBKey_h
#define IDBKey_h

#if ENABLE(INDEXED_DATABASE)

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class IDBKey : public RefCounted<IDBKey> {
public:
    typedef Vector<RefPtr<IDBKey> > KeyArray;

    static PassRefPtr<IDBKey> createInvalid()
    {
        return adoptRef(new IDBKey());
    }

    static PassRefPtr<IDBKey> createNumber(double number)
    {
        return adoptRef(new IDBKey(NumberType, number));
    }

    static PassRefPtr<IDBKey> createString(const String& string)
    {
        return adoptRef(new IDBKey(string));
    }

    static PassRefPtr<IDBKey> createDate(double date)
    {
        return adoptRef(new IDBKey(DateType, date));
    }

    static PassRefPtr<IDBKey> createMultiEntryArray(const KeyArray& array)
    {
        KeyArray result;

        size_t sizeEstimate = 0;
        for (size_t i = 0; i < array.size(); i++) {
            if (!array[i]->isValid())
                continue;

            bool skip = false;
            for (size_t j = 0; j < result.size(); j++) {
                if (array[i]->isEqual(result[j].get())) {
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                result.append(array[i]);
                sizeEstimate += array[i]->m_sizeEstimate;
            }
        }
        RefPtr<IDBKey> idbKey = adoptRef(new IDBKey(result, sizeEstimate));
        ASSERT(idbKey->isValid());
        return idbKey.release();
    }

    static PassRefPtr<IDBKey> createArray(const KeyArray& array)
    {
        size_t sizeEstimate = 0;
        for (size_t i = 0; i < array.size(); ++i)
            sizeEstimate += array[i]->m_sizeEstimate;

        return adoptRef(new IDBKey(array, sizeEstimate));
    }

    ~IDBKey();

    // In order of the least to the highest precedent in terms of sort order.
    enum Type {
        InvalidType = 0,
        ArrayType,
        StringType,
        DateType,
        NumberType,
        MinType
    };

    Type type() const { return m_type; }
    bool isValid() const;

    const KeyArray& array() const
    {
        ASSERT(m_type == ArrayType);
        return m_array;
    }

    const String& string() const
    {
        ASSERT(m_type == StringType);
        return m_string;
    }

    double date() const
    {
        ASSERT(m_type == DateType);
        return m_number;
    }

    double number() const
    {
        ASSERT(m_type == NumberType);
        return m_number;
    }

    int compare(const IDBKey* other) const;
    bool isLessThan(const IDBKey* other) const;
    bool isEqual(const IDBKey* other) const;

    size_t sizeEstimate() const { return m_sizeEstimate; }

    static int compareTypes(Type a, Type b)
    {
        return b - a;
    }

    using RefCounted<IDBKey>::ref;
    using RefCounted<IDBKey>::deref;

private:
    IDBKey() : m_type(InvalidType), m_number(0), m_sizeEstimate(OverheadSize) { }
    IDBKey(Type type, double number) : m_type(type), m_number(number), m_sizeEstimate(OverheadSize + sizeof(double)) { }
    explicit IDBKey(const String& value) : m_type(StringType), m_string(value), m_number(0), m_sizeEstimate(OverheadSize + value.length() * sizeof(UChar)) { }
    IDBKey(const KeyArray& keyArray, size_t arraySize) : m_type(ArrayType), m_array(keyArray), m_number(0), m_sizeEstimate(OverheadSize + arraySize) { }

    const Type m_type;
    const KeyArray m_array;
    const String m_string;
    const double m_number;

    const size_t m_sizeEstimate;

    // Very rough estimate of minimum key size overhead.
    enum { OverheadSize = 16 };
};

}

#endif // ENABLE(INDEXED_DATABASE)

#endif // IDBKey_h

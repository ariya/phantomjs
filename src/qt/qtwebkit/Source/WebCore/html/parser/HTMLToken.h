/*
 * Copyright (C) 2013 Google, Inc. All Rights Reserved.
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

#ifndef HTMLToken_h
#define HTMLToken_h

#include "Attribute.h"
#include "HTMLToken.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class DoctypeData {
    WTF_MAKE_NONCOPYABLE(DoctypeData);
public:
    DoctypeData()
        : m_hasPublicIdentifier(false)
        , m_hasSystemIdentifier(false)
        , m_forceQuirks(false)
    {
    }

    // FIXME: This should use String instead of Vector<UChar>.
    bool m_hasPublicIdentifier;
    bool m_hasSystemIdentifier;
    WTF::Vector<UChar> m_publicIdentifier;
    WTF::Vector<UChar> m_systemIdentifier;
    bool m_forceQuirks;
};

static inline Attribute* findAttributeInVector(Vector<Attribute>& attributes, const QualifiedName& name)
{
    for (unsigned i = 0; i < attributes.size(); ++i) {
        if (attributes.at(i).name().matches(name))
            return &attributes.at(i);
    }
    return 0;
}

class HTMLToken {
    WTF_MAKE_NONCOPYABLE(HTMLToken);
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum Type {
        Uninitialized,
        DOCTYPE,
        StartTag,
        EndTag,
        Comment,
        Character,
        EndOfFile,
    };

    class Attribute {
    public:
        class Range {
        public:
            int start;
            int end;
        };

        Range nameRange;
        Range valueRange;
        Vector<UChar, 32> name;
        Vector<UChar, 32> value;
    };

    typedef Vector<Attribute, 10> AttributeList;
    typedef Vector<UChar, 256> DataVector;

    HTMLToken() { clear(); }

    void clear()
    {
        m_type = Uninitialized;
        m_range.start = 0;
        m_range.end = 0;
        m_baseOffset = 0;
        m_data.clear();
        m_orAllData = 0;
    }

    bool isUninitialized() { return m_type == Uninitialized; }
    Type type() const { return m_type; }

    void makeEndOfFile()
    {
        ASSERT(m_type == Uninitialized);
        m_type = EndOfFile;
    }

    /* Range and offset methods exposed for HTMLSourceTracker and HTMLViewSourceParser */
    int startIndex() const { return m_range.start; }
    int endIndex() const { return m_range.end; }

    void setBaseOffset(int offset)
    {
        m_baseOffset = offset;
    }

    void end(int endOffset)
    {
        m_range.end = endOffset - m_baseOffset;
    }

    const DataVector& data() const
    {
        ASSERT(m_type == Character || m_type == Comment || m_type == StartTag || m_type == EndTag);
        return m_data;
    }

    bool isAll8BitData() const
    {
        return (m_orAllData <= 0xff);
    }

    const DataVector& name() const
    {
        ASSERT(m_type == StartTag || m_type == EndTag || m_type == DOCTYPE);
        return m_data;
    }

    void appendToName(UChar character)
    {
        ASSERT(m_type == StartTag || m_type == EndTag || m_type == DOCTYPE);
        ASSERT(character);
        m_data.append(character);
        m_orAllData |= character;
    }

    /* DOCTYPE Tokens */

    bool forceQuirks() const
    {
        ASSERT(m_type == DOCTYPE);
        return m_doctypeData->m_forceQuirks;
    }

    void setForceQuirks()
    {
        ASSERT(m_type == DOCTYPE);
        m_doctypeData->m_forceQuirks = true;
    }

    void beginDOCTYPE()
    {
        ASSERT(m_type == Uninitialized);
        m_type = DOCTYPE;
        m_doctypeData = adoptPtr(new DoctypeData);
    }

    void beginDOCTYPE(UChar character)
    {
        ASSERT(character);
        beginDOCTYPE();
        m_data.append(character);
        m_orAllData |= character;
    }

    // FIXME: Distinguish between a missing public identifer and an empty one.
    const WTF::Vector<UChar>& publicIdentifier() const
    {
        ASSERT(m_type == DOCTYPE);
        return m_doctypeData->m_publicIdentifier;
    }

    // FIXME: Distinguish between a missing system identifer and an empty one.
    const WTF::Vector<UChar>& systemIdentifier() const
    {
        ASSERT(m_type == DOCTYPE);
        return m_doctypeData->m_systemIdentifier;
    }

    void setPublicIdentifierToEmptyString()
    {
        ASSERT(m_type == DOCTYPE);
        m_doctypeData->m_hasPublicIdentifier = true;
        m_doctypeData->m_publicIdentifier.clear();
    }

    void setSystemIdentifierToEmptyString()
    {
        ASSERT(m_type == DOCTYPE);
        m_doctypeData->m_hasSystemIdentifier = true;
        m_doctypeData->m_systemIdentifier.clear();
    }

    void appendToPublicIdentifier(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == DOCTYPE);
        ASSERT(m_doctypeData->m_hasPublicIdentifier);
        m_doctypeData->m_publicIdentifier.append(character);
    }

    void appendToSystemIdentifier(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == DOCTYPE);
        ASSERT(m_doctypeData->m_hasSystemIdentifier);
        m_doctypeData->m_systemIdentifier.append(character);
    }

    PassOwnPtr<DoctypeData> releaseDoctypeData()
    {
        return m_doctypeData.release();
    }

    /* Start/End Tag Tokens */

    bool selfClosing() const
    {
        ASSERT(m_type == StartTag || m_type == EndTag);
        return m_selfClosing;
    }

    void setSelfClosing()
    {
        ASSERT(m_type == StartTag || m_type == EndTag);
        m_selfClosing = true;
    }

    void beginStartTag(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == Uninitialized);
        m_type = StartTag;
        m_selfClosing = false;
        m_currentAttribute = 0;
        m_attributes.clear();

        m_data.append(character);
        m_orAllData |= character;
    }

    void beginEndTag(LChar character)
    {
        ASSERT(m_type == Uninitialized);
        m_type = EndTag;
        m_selfClosing = false;
        m_currentAttribute = 0;
        m_attributes.clear();

        m_data.append(character);
    }

    void beginEndTag(const Vector<LChar, 32>& characters)
    {
        ASSERT(m_type == Uninitialized);
        m_type = EndTag;
        m_selfClosing = false;
        m_currentAttribute = 0;
        m_attributes.clear();

        m_data.appendVector(characters);
    }

    void addNewAttribute()
    {
        ASSERT(m_type == StartTag || m_type == EndTag);
        m_attributes.grow(m_attributes.size() + 1);
        m_currentAttribute = &m_attributes.last();
#ifndef NDEBUG
        m_currentAttribute->nameRange.start = 0;
        m_currentAttribute->nameRange.end = 0;
        m_currentAttribute->valueRange.start = 0;
        m_currentAttribute->valueRange.end = 0;
#endif
    }

    void beginAttributeName(int offset)
    {
        m_currentAttribute->nameRange.start = offset - m_baseOffset;
    }

    void endAttributeName(int offset)
    {
        int index = offset - m_baseOffset;
        m_currentAttribute->nameRange.end = index;
        m_currentAttribute->valueRange.start = index;
        m_currentAttribute->valueRange.end = index;
    }

    void beginAttributeValue(int offset)
    {
        m_currentAttribute->valueRange.start = offset - m_baseOffset;
#ifndef NDEBUG
        m_currentAttribute->valueRange.end = 0;
#endif
    }

    void endAttributeValue(int offset)
    {
        m_currentAttribute->valueRange.end = offset - m_baseOffset;
    }

    void appendToAttributeName(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == StartTag || m_type == EndTag);
        // FIXME: We should be able to add the following ASSERT once we fix
        // https://bugs.webkit.org/show_bug.cgi?id=62971
        //   ASSERT(m_currentAttribute->nameRange.start);
        m_currentAttribute->name.append(character);
    }

    void appendToAttributeValue(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == StartTag || m_type == EndTag);
        ASSERT(m_currentAttribute->valueRange.start);
        m_currentAttribute->value.append(character);
    }

    void appendToAttributeValue(size_t i, const String& value)
    {
        ASSERT(!value.isEmpty());
        ASSERT(m_type == StartTag || m_type == EndTag);
        append(m_attributes[i].value, value);
    }

    const AttributeList& attributes() const
    {
        ASSERT(m_type == StartTag || m_type == EndTag);
        return m_attributes;
    }

    const Attribute* getAttributeItem(const QualifiedName& name) const
    {
        for (unsigned i = 0; i < m_attributes.size(); ++i) {
            if (AtomicString(m_attributes.at(i).name) == name.localName())
                return &m_attributes.at(i);
        }
        return 0;
    }

    // Used by the XSSAuditor to nuke XSS-laden attributes.
    void eraseValueOfAttribute(size_t i)
    {
        ASSERT(m_type == StartTag || m_type == EndTag);
        m_attributes[i].value.clear();
    }

    /* Character Tokens */

    // Starting a character token works slightly differently than starting
    // other types of tokens because we want to save a per-character branch.
    void ensureIsCharacterToken()
    {
        ASSERT(m_type == Uninitialized || m_type == Character);
        m_type = Character;
    }

    const DataVector& characters() const
    {
        ASSERT(m_type == Character);
        return m_data;
    }

    void appendToCharacter(char character)
    {
        ASSERT(m_type == Character);
        m_data.append(character);
    }

    void appendToCharacter(UChar character)
    {
        ASSERT(m_type == Character);
        m_data.append(character);
        m_orAllData |= character;
    }

    void appendToCharacter(const Vector<LChar, 32>& characters)
    {
        ASSERT(m_type == Character);
        m_data.appendVector(characters);
    }

    /* Comment Tokens */

    const DataVector& comment() const
    {
        ASSERT(m_type == Comment);
        return m_data;
    }

    void beginComment()
    {
        ASSERT(m_type == Uninitialized);
        m_type = Comment;
    }

    void appendToComment(UChar character)
    {
        ASSERT(character);
        ASSERT(m_type == Comment);
        m_data.append(character);
        m_orAllData |= character;
    }

    void eraseCharacters()
    {
        ASSERT(m_type == Character);
        m_data.clear();
        m_orAllData = 0;
    }

private:
    Type m_type;
    Attribute::Range m_range; // Always starts at zero.
    int m_baseOffset;
    DataVector m_data;
    UChar m_orAllData;

    // For StartTag and EndTag
    bool m_selfClosing;
    AttributeList m_attributes;

    // A pointer into m_attributes used during lexing.
    Attribute* m_currentAttribute;

    // For DOCTYPE
    OwnPtr<DoctypeData> m_doctypeData;
};

}

#endif

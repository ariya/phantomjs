/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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

#ifndef TextPosition_h
#define TextPosition_h

#include <wtf/Assertions.h>

namespace WTF {

/*
 * Text Position
 *
 * TextPosition structure specifies coordinates within an text resource. It is used mostly
 * for saving script source position.
 *
 * Later TextPosition0 and TextPosition1 and both number types can be merged together quite easily.
 *
 * 0-based and 1-based
 *
 * Line and column numbers could be interpreted as zero-based or 1-based. Since
 * both practices coexist in WebKit source base, 'int' type should be replaced with
 * a dedicated wrapper types, so that compiler helped us with this ambiguity.
 *
 * Here we introduce 2 types of numbers: ZeroBasedNumber and OneBasedNumber and
 * 2 corresponding types of TextPosition structure. While only one type ought to be enough,
 * this is done to keep transition to the new types as transparent as possible:
 * e.g. in areas where 0-based integers are used, TextPosition0 should be introduced. This
 * way all changes will remain trackable.
 *
 * Later both number types can be merged in one type quite easily.
 *
 * For type safety and for the future type merge it is important that all operations in API
 * that accept or return integer have a name explicitly defining base of integer. For this reason
 * int-receiving constructors are hidden from API.
 */

template<typename NUMBER>
class TextPosition {
public:
    TextPosition(NUMBER line, NUMBER column)
        : m_line(line)
        , m_column(column)
    {
    }
    TextPosition() {}

    bool operator==(const TextPosition& other) { return m_line == other.m_line && m_column == other.m_column; }
    bool operator!=(const TextPosition& other) { return !((*this) == other); }

    // A 'minimum' value of position, used as a default value.
    static TextPosition<NUMBER> minimumPosition() { return TextPosition<NUMBER>(NUMBER::base(), NUMBER::base()); }

    // A value with line value less than a minimum; used as an impossible position.
    static TextPosition<NUMBER> belowRangePosition() { return TextPosition<NUMBER>(NUMBER::belowBase(), NUMBER::belowBase()); }

    NUMBER m_line;
    NUMBER m_column;
};

class OneBasedNumber;

// An int wrapper that always reminds you that the number should be 0-based
class ZeroBasedNumber {
public:
    static ZeroBasedNumber fromZeroBasedInt(int zeroBasedInt) { return ZeroBasedNumber(zeroBasedInt); }

    ZeroBasedNumber() {}

    int zeroBasedInt() const { return m_value; }
    int convertAsOneBasedInt() const { return m_value + 1; }
    OneBasedNumber convertToOneBased() const;

    bool operator==(ZeroBasedNumber other) { return m_value == other.m_value; }
    bool operator!=(ZeroBasedNumber other) { return !((*this) == other); }

    static ZeroBasedNumber base() { return 0; }
    static ZeroBasedNumber belowBase() { return -1; }

private:
    ZeroBasedNumber(int value) : m_value(value) {}
    int m_value;
};

// An int wrapper that always reminds you that the number should be 1-based
class OneBasedNumber {
public:
    static OneBasedNumber fromOneBasedInt(int oneBasedInt) { return OneBasedNumber(oneBasedInt); }
    OneBasedNumber() {}

    int oneBasedInt() const { return m_value; }
    int convertAsZeroBasedInt() const { return m_value - 1; }
    ZeroBasedNumber convertToZeroBased() const { return ZeroBasedNumber::fromZeroBasedInt(m_value - 1); }

    bool operator==(OneBasedNumber other) { return m_value == other.m_value; }
    bool operator!=(OneBasedNumber other) { return !((*this) == other); }

    static OneBasedNumber base() { return 1; }
    static OneBasedNumber belowBase() { return 0; }

private:
    OneBasedNumber(int value) : m_value(value) {}
    int m_value;
};

typedef TextPosition<ZeroBasedNumber> TextPosition0;
typedef TextPosition<OneBasedNumber> TextPosition1;

inline TextPosition0 toZeroBasedTextPosition(const TextPosition1& position)
{
    return TextPosition0(position.m_line.convertToZeroBased(), position.m_column.convertToZeroBased());
}

inline TextPosition1 toOneBasedTextPosition(const TextPosition0& position)
{
    return TextPosition1(position.m_line.convertToOneBased(), position.m_column.convertToOneBased());
}

inline OneBasedNumber ZeroBasedNumber::convertToOneBased() const
{
    return OneBasedNumber::fromOneBasedInt(m_value + 1);
}

}

using WTF::TextPosition0;
using WTF::TextPosition1;

#endif // TextPosition_h

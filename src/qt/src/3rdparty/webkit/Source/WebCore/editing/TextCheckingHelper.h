/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 */

#ifndef TextCheckingHelper_h
#define TextCheckingHelper_h

#include "EditorClient.h"

namespace WebCore {

class Range;
class Position;

class TextCheckingParagraph {
public:
    explicit TextCheckingParagraph(PassRefPtr<Range> checkingRange);
    ~TextCheckingParagraph();

    int rangeLength() const;
    PassRefPtr<Range> subrange(int characterOffset, int characterCount) const;
    int offsetTo(const Position&, ExceptionCode&) const;
    void expandRangeToNextEnd();

    int textLength() const { return text().length(); }
    String textSubstring(unsigned pos, unsigned len = UINT_MAX) const { return text().substring(pos, len); }
    const UChar* textCharacters() const { return text().characters(); }
    UChar textCharAt(int index) const { return text()[index]; }

    bool isEmpty() const;
    bool isTextEmpty() const { return text().isEmpty(); }
    bool isRangeEmpty() const { return checkingStart() >= checkingEnd(); }

    int checkingStart() const;
    int checkingEnd() const;
    int checkingLength() const;
    String checkingSubstring() const { return textSubstring(checkingStart(), checkingLength()); }

    bool checkingRangeMatches(int location, int length) const { return location == checkingStart() && length == checkingLength(); }
    bool isCheckingRangeCoveredBy(int location, int length) const { return location <= checkingStart() && location + length >= checkingStart() + checkingLength(); }
    bool checkingRangeCovers(int location, int length) const { return location < checkingEnd() && location + length > checkingStart(); }
    PassRefPtr<Range> paragraphRange() const;

private:
    void invalidateParagraphRangeValues();
    PassRefPtr<Range> checkingRange() const { return m_checkingRange; }
    PassRefPtr<Range> offsetAsRange() const;
    const String& text() const;

    RefPtr<Range> m_checkingRange;
    mutable RefPtr<Range> m_paragraphRange;
    mutable RefPtr<Range> m_offsetAsRange;
    mutable String m_text;
    mutable int m_checkingStart;
    mutable int m_checkingEnd;
    mutable int m_checkingLength;
};

class TextCheckingHelper {
    WTF_MAKE_NONCOPYABLE(TextCheckingHelper);
public:
    TextCheckingHelper(EditorClient*, PassRefPtr<Range>);
    ~TextCheckingHelper();

    String findFirstMisspelling(int& firstMisspellingOffset, bool markAll, RefPtr<Range>& firstMisspellingRange);
    String findFirstMisspellingOrBadGrammar(bool checkGrammar, bool& outIsSpelling, int& outFirstFoundOffset, GrammarDetail& outGrammarDetail);
    String findFirstBadGrammar(GrammarDetail& outGrammarDetail, int& outGrammarPhraseOffset, bool markAll);
    int findFirstGrammarDetail(const Vector<GrammarDetail>& grammarDetails, int badGrammarPhraseLocation, int badGrammarPhraseLength, int startOffset, int endOffset, bool markAll);
    void markAllMisspellings(RefPtr<Range>& firstMisspellingRange);
    void markAllBadGrammar();

    bool isUngrammatical(Vector<String>& guessesVector) const;
    Vector<String> guessesForMisspelledOrUngrammaticalRange(bool checkGrammar, bool& misspelled, bool& ungrammatical) const;
private:
    EditorClient* m_client;
    RefPtr<Range> m_range;
};

} // namespace WebCore

#endif // TextCheckingHelper_h

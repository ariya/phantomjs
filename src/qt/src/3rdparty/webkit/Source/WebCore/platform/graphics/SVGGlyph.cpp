/*
 * Copyright (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010-2011. All rights reserved.
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

#include "config.h"

#if ENABLE(SVG_FONTS)
#include "SVGGlyph.h"

#include <wtf/unicode/Unicode.h>

using namespace WTF::Unicode;

namespace WebCore {

// Helper functions to determine the arabic character forms (initial, medial, terminal, isolated)
enum ArabicCharShapingMode {
    SNone = 0,
    SRight = 1,
    SDual = 2
};

static const ArabicCharShapingMode s_arabicCharShapingMode[222] = {
    SRight, SRight, SRight, SRight, SDual , SRight, SDual , SRight, SDual , SDual , SDual , SDual , SDual , SRight,                 /* 0x0622 - 0x062F */
    SRight, SRight, SRight, SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SNone , SNone , SNone , SNone , SNone , /* 0x0630 - 0x063F */
    SNone , SDual , SDual , SDual , SDual , SDual , SDual , SRight, SDual , SDual , SNone , SNone , SNone , SNone , SNone , SNone , /* 0x0640 - 0x064F */
    SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , /* 0x0650 - 0x065F */
    SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , /* 0x0660 - 0x066F */
    SNone , SRight, SRight, SRight, SNone , SRight, SRight, SRight, SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , /* 0x0670 - 0x067F */
    SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SRight, SRight, SRight, SRight, SRight, SRight, SRight, SRight, /* 0x0680 - 0x068F */
    SRight, SRight, SRight, SRight, SRight, SRight, SRight, SRight, SRight, SRight, SDual , SDual , SDual , SDual , SDual , SDual , /* 0x0690 - 0x069F */
    SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , /* 0x06A0 - 0x06AF */
    SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , SDual , /* 0x06B0 - 0x06BF */
    SRight, SDual , SRight, SRight, SRight, SRight, SRight, SRight, SRight, SRight, SRight, SRight, SDual , SRight, SDual , SRight, /* 0x06C0 - 0x06CF */
    SDual , SDual , SRight, SRight, SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , /* 0x06D0 - 0x06DF */
    SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , /* 0x06E0 - 0x06EF */
    SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SNone , SDual , SDual , SDual , SNone , SNone , SNone   /* 0x06F0 - 0x06FF */
};

static inline SVGGlyph::ArabicForm processArabicFormDetection(const UChar& curChar, bool& lastCharShapesRight, SVGGlyph::ArabicForm* prevForm)
{
    SVGGlyph::ArabicForm curForm;

    ArabicCharShapingMode shapingMode = SNone;
    if (curChar >= 0x0622 && curChar <= 0x06FF)
        shapingMode = s_arabicCharShapingMode[curChar - 0x0622];

    // Use a simple state machine to identify the actual arabic form
    // It depends on the order of the arabic form enum:
    // enum ArabicForm { None = 0, Isolated, Terminal, Initial, Medial };

    if (lastCharShapesRight && shapingMode == SDual) {
        if (prevForm) {
            int correctedForm = (int) *prevForm + 1;
            ASSERT(correctedForm >= SVGGlyph::None && correctedForm <= SVGGlyph::Medial);
            *prevForm = static_cast<SVGGlyph::ArabicForm>(correctedForm);
        }

        curForm = SVGGlyph::Initial;
    } else
        curForm = shapingMode == SNone ? SVGGlyph::None : SVGGlyph::Isolated;

    lastCharShapesRight = shapingMode != SNone;
    return curForm;
}

Vector<SVGGlyph::ArabicForm> charactersWithArabicForm(const String& input, bool mirror)
{
    Vector<SVGGlyph::ArabicForm> forms;
    unsigned length = input.length();

    bool containsArabic = false;
    for (unsigned i = 0; i < length; ++i) {
        if (isArabicChar(input[i])) {
            containsArabic = true;
            break;
        }
    }

    if (!containsArabic)
        return forms;

    bool lastCharShapesRight = false;

    // Start identifying arabic forms
    if (mirror) {
        for (int i = length - 1; i >= 0; --i)
            forms.prepend(processArabicFormDetection(input[i], lastCharShapesRight, forms.isEmpty() ? 0 : &forms.first()));
    } else {
        for (unsigned i = 0; i < length; ++i)
            forms.append(processArabicFormDetection(input[i], lastCharShapesRight, forms.isEmpty() ? 0 : &forms.last()));
    }

    return forms;
}

static inline bool isCompatibleArabicForm(const SVGGlyph& identifier, const Vector<SVGGlyph::ArabicForm>& chars, unsigned startPosition, unsigned endPosition)
{
    if (chars.isEmpty())
        return true;

    Vector<SVGGlyph::ArabicForm>::const_iterator realEnd = chars.end();
    Vector<SVGGlyph::ArabicForm>::const_iterator it = chars.begin() + startPosition;
    if (it >= realEnd)
        return true;

    Vector<SVGGlyph::ArabicForm>::const_iterator end = chars.begin() + endPosition;
    if (end >= realEnd)
        end = realEnd;

    for (; it != end; ++it) {
        if (*it != static_cast<SVGGlyph::ArabicForm>(identifier.arabicForm) && *it != SVGGlyph::None)
            return false;
    }

    return true;
}

bool isCompatibleGlyph(const SVGGlyph& identifier, bool isVerticalText, const String& language,
                       const Vector<SVGGlyph::ArabicForm>& chars, unsigned startPosition, unsigned endPosition)
{
    bool valid = true;

    // Check wheter orientation if glyph fits within the request
    switch (identifier.orientation) {
    case SVGGlyph::Vertical:
        valid = isVerticalText;
        break;
    case SVGGlyph::Horizontal:
        valid = !isVerticalText;
        break;
    case SVGGlyph::Both:
        break;
    }

    if (!valid)
        return false;

    // Check wheter languages are compatible
    if (!identifier.languages.isEmpty()) {
        // This glyph exists only in certain languages, if we're not specifying a
        // language on the referencing element we're unable to use this glyph.
        if (language.isEmpty())
            return false;

        // Split subcode from language, if existant.
        String languagePrefix;

        size_t subCodeSeparator = language.find('-');
        if (subCodeSeparator != notFound)
            languagePrefix = language.left(subCodeSeparator);

        Vector<String>::const_iterator it = identifier.languages.begin();
        Vector<String>::const_iterator end = identifier.languages.end();

        bool found = false;
        for (; it != end; ++it) {
            const String& cur = *it;
            if (cur == language || cur == languagePrefix) {
                found = true;
                break;
            }
        }

        if (!found)
            return false;
    }

    // Check wheter arabic form is compatible
    return isCompatibleArabicForm(identifier, chars, startPosition, endPosition);
}

}

#endif

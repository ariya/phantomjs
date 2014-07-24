/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InspectorStyleTextEditor.h"

#include "CSSPropertySourceData.h"
#include "HTMLParserIdioms.h"
#include "InspectorStyleSheet.h"

namespace WebCore {

InspectorStyleTextEditor::InspectorStyleTextEditor(Vector<InspectorStyleProperty>* allProperties, Vector<InspectorStyleProperty>* disabledProperties, const String& styleText, const NewLineAndWhitespace& format)
    : m_allProperties(allProperties)
    , m_disabledProperties(disabledProperties)
    , m_styleText(styleText)
    , m_format(format)
{
}

void InspectorStyleTextEditor::insertProperty(unsigned index, const String& propertyText, unsigned styleBodyLength)
{
    long propertyStart = 0;

    bool insertLast = true;
    if (index < m_allProperties->size()) {
        const InspectorStyleProperty& property = m_allProperties->at(index);
        if (property.hasSource) {
            propertyStart = property.sourceData.range.start;
            // If inserting before a disabled property, it should be shifted, too.
            insertLast = false;
        }
    }

    bool insertFirstInSource = true;
    for (unsigned i = 0, size = m_allProperties->size(); i < index && i < size; ++i) {
        const InspectorStyleProperty& property = m_allProperties->at(i);
        if (property.hasSource && !property.disabled) {
            insertFirstInSource = false;
            break;
        }
    }

    bool insertLastInSource = true;
    for (unsigned i = index, size = m_allProperties->size(); i < size; ++i) {
      const InspectorStyleProperty& property = m_allProperties->at(i);
        if (property.hasSource && !property.disabled) {
            insertLastInSource = false;
            break;
        }
    }

    String textToSet = propertyText;

    int formattingPrependOffset = 0;
    if (insertLast && !insertFirstInSource) {
        propertyStart = styleBodyLength;
        if (propertyStart && textToSet.length()) {
            const UChar* characters = m_styleText.characters();

            long curPos = propertyStart - 1; // The last position of style declaration, since propertyStart points past one.
            while (curPos && isHTMLSpace(characters[curPos]))
                --curPos;
            if (curPos && characters[curPos] != ';') {
                // Prepend a ";" to the property text if appending to a style declaration where
                // the last property has no trailing ";".
                textToSet.insert(";", 0);
                formattingPrependOffset = 1;
            }
        }
    }

    const String& formatLineFeed = m_format.first;
    const String& formatPropertyPrefix = m_format.second;
    if (insertLastInSource) {
        long formatPropertyPrefixLength = formatPropertyPrefix.length();
        if (!formattingPrependOffset && (propertyStart < formatPropertyPrefixLength || m_styleText.substring(propertyStart - formatPropertyPrefixLength, formatPropertyPrefixLength) != formatPropertyPrefix)) {
            textToSet.insert(formatPropertyPrefix, formattingPrependOffset);
            if (!propertyStart || !isHTMLLineBreak(m_styleText[propertyStart - 1]))
                textToSet.insert(formatLineFeed, formattingPrependOffset);
        }
        if (!isHTMLLineBreak(m_styleText[propertyStart]))
            textToSet.append(formatLineFeed);
    } else {
        String fullPrefix = formatLineFeed + formatPropertyPrefix;
        long fullPrefixLength = fullPrefix.length();
        textToSet.append(fullPrefix);
        if (insertFirstInSource && (propertyStart < fullPrefixLength || m_styleText.substring(propertyStart - fullPrefixLength, fullPrefixLength) != fullPrefix))
            textToSet.insert(fullPrefix, formattingPrependOffset);
    }
    m_styleText.insert(textToSet, propertyStart);

    // Recompute disabled property ranges after an inserted property.
    long propertyLengthDelta = textToSet.length();
    shiftDisabledProperties(disabledIndexByOrdinal(index, true), propertyLengthDelta);
}

void InspectorStyleTextEditor::replaceProperty(unsigned index, const String& newText)
{
    ASSERT_WITH_SECURITY_IMPLICATION(index < m_allProperties->size());

    const InspectorStyleProperty& property = m_allProperties->at(index);
    long propertyStart = property.sourceData.range.start;
    long propertyEnd = property.sourceData.range.end;
    long oldLength = propertyEnd - propertyStart;
    long newLength = newText.length();
    long propertyLengthDelta = newLength - oldLength;

    if (!property.disabled) {
        SourceRange overwrittenRange;
        unsigned insertedLength;
        internalReplaceProperty(property, newText, &overwrittenRange, &insertedLength);
        propertyLengthDelta = static_cast<long>(insertedLength) - static_cast<long>(overwrittenRange.length());

        // Recompute subsequent disabled property ranges if acting on a non-disabled property.
        shiftDisabledProperties(disabledIndexByOrdinal(index, true), propertyLengthDelta);
    } else {
        long textLength = newText.length();
        unsigned disabledIndex = disabledIndexByOrdinal(index, false);
        if (!textLength) {
            // Delete disabled property.
            m_disabledProperties->remove(disabledIndex);
        } else {
            // Patch disabled property text.
            m_disabledProperties->at(disabledIndex).rawText = newText;
        }
    }
}

void InspectorStyleTextEditor::removeProperty(unsigned index)
{
    replaceProperty(index, "");
}

void InspectorStyleTextEditor::enableProperty(unsigned index)
{
    ASSERT(m_allProperties->at(index).disabled);

    unsigned disabledIndex = disabledIndexByOrdinal(index, false);
    ASSERT(disabledIndex != UINT_MAX);

    InspectorStyleProperty disabledProperty = m_disabledProperties->at(disabledIndex);
    m_disabledProperties->remove(disabledIndex);
    SourceRange removedRange;
    unsigned insertedLength;
    internalReplaceProperty(disabledProperty, disabledProperty.rawText, &removedRange, &insertedLength);
    shiftDisabledProperties(disabledIndex, static_cast<long>(insertedLength) - static_cast<long>(removedRange.length()));
}

void InspectorStyleTextEditor::disableProperty(unsigned index)
{
    ASSERT(!m_allProperties->at(index).disabled);

    const InspectorStyleProperty& property = m_allProperties->at(index);
    InspectorStyleProperty disabledProperty(property);
    disabledProperty.setRawTextFromStyleDeclaration(m_styleText);
    disabledProperty.disabled = true;

    SourceRange removedRange;
    unsigned insertedLength;
    internalReplaceProperty(property, "", &removedRange, &insertedLength);

    // If some preceding formatting has been removed, move the property to the start of the removed range.
    if (property.sourceData.range.start > removedRange.start)
        disabledProperty.sourceData.range.start = removedRange.start;
    disabledProperty.sourceData.range.end = disabledProperty.sourceData.range.start;

    // Add disabled property at correct position.
    unsigned insertionIndex = disabledIndexByOrdinal(index, true);
    if (insertionIndex == UINT_MAX)
        m_disabledProperties->append(disabledProperty);
    else {
        m_disabledProperties->insert(insertionIndex, disabledProperty);
        long styleLengthDelta = -(static_cast<long>(removedRange.length()));
        shiftDisabledProperties(insertionIndex + 1, styleLengthDelta); // Property removed from text - shift these back.
    }
}

unsigned InspectorStyleTextEditor::disabledIndexByOrdinal(unsigned ordinal, bool canUseSubsequent)
{
    unsigned disabledIndex = 0;
    for (unsigned i = 0, size = m_allProperties->size(); i < size; ++i) {
        if (m_allProperties->at(i).disabled) {
            if (i == ordinal || (canUseSubsequent && i > ordinal))
                return disabledIndex;
            ++disabledIndex;
        }
    }

    return UINT_MAX;
}

void InspectorStyleTextEditor::shiftDisabledProperties(unsigned fromIndex, long delta)
{
    for (unsigned i = fromIndex, size = m_disabledProperties->size(); i < size; ++i) {
        SourceRange& range = m_disabledProperties->at(i).sourceData.range;
        range.start += delta;
        range.end += delta;
    }
}

void InspectorStyleTextEditor::internalReplaceProperty(const InspectorStyleProperty& property, const String& newText, SourceRange* removedRange, unsigned* insertedLength)
{
    const SourceRange& range = property.sourceData.range;
    long replaceRangeStart = range.start;
    long replaceRangeEnd = range.end;
    const UChar* characters = m_styleText.characters();
    long newTextLength = newText.length();
    String finalNewText = newText;

    // Removing a property - remove preceding prefix.
    String fullPrefix = m_format.first + m_format.second;
    long fullPrefixLength = fullPrefix.length();
    if (!newTextLength && fullPrefixLength) {
        if (replaceRangeStart >= fullPrefixLength && m_styleText.substring(replaceRangeStart - fullPrefixLength, fullPrefixLength) == fullPrefix)
            replaceRangeStart -= fullPrefixLength;
    } else if (newTextLength) {
        if (isHTMLLineBreak(newText.characters()[newTextLength - 1])) {
            // Coalesce newlines of the original and new property values (to avoid a lot of blank lines while incrementally applying property values).
            bool foundNewline = false;
            bool isLastNewline = false;
            int i;
            int textLength = m_styleText.length();
            for (i = replaceRangeEnd; i < textLength && isSpaceOrNewline(characters[i]); ++i) {
                isLastNewline = isHTMLLineBreak(characters[i]);
                if (isLastNewline)
                    foundNewline = true;
                else if (foundNewline && !isLastNewline) {
                    replaceRangeEnd = i;
                    break;
                }
            }
            if (foundNewline && isLastNewline)
                replaceRangeEnd = i;
        }

        if (fullPrefixLength > replaceRangeStart || m_styleText.substring(replaceRangeStart - fullPrefixLength, fullPrefixLength) != fullPrefix)
            finalNewText.insert(fullPrefix, 0);
    }

    int replacedLength = replaceRangeEnd - replaceRangeStart;
    m_styleText.replace(replaceRangeStart, replacedLength, finalNewText);
    *removedRange = SourceRange(replaceRangeStart, replaceRangeEnd);
    *insertedLength = finalNewText.length();
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)

/*
 * Copyright (C) 2006 Lars Knoll <lars@trolltech.com>
 * Copyright (C) 2007, 2011, 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "TextBreakIterator.h"

#include "LineBreakIteratorPoolICU.h"
#include <wtf/Atomics.h>
#include <wtf/text/WTFString.h>

using namespace WTF;
using namespace std;

namespace WebCore {

static TextBreakIterator* setUpIterator(bool& createdIterator, TextBreakIterator*& iterator,
    UBreakIteratorType type, const UChar* string, int length)
{
    if (!string)
        return 0;

    if (!createdIterator) {
        UErrorCode openStatus = U_ZERO_ERROR;
        iterator = reinterpret_cast<TextBreakIterator*>(ubrk_open(type, currentTextBreakLocaleID(), 0, 0, &openStatus));
        createdIterator = true;
        ASSERT_WITH_MESSAGE(U_SUCCESS(openStatus), "ICU could not open a break iterator: %s (%d)", u_errorName(openStatus), openStatus);
    }
    if (!iterator)
        return 0;

    UErrorCode setTextStatus = U_ZERO_ERROR;
    ubrk_setText(reinterpret_cast<UBreakIterator*>(iterator), string, length, &setTextStatus);
    if (U_FAILURE(setTextStatus))
        return 0;

    return iterator;
}

enum TextContext { NoContext, PriorContext, PrimaryContext };

const int textBufferCapacity = 16;

typedef struct {
    UText text;
    UChar buffer[textBufferCapacity];
} UTextWithBuffer;

static inline int64_t textPinIndex(int64_t& index, int64_t limit)
{
    if (index < 0)
        index = 0;
    else if (index > limit)
        index = limit;
    return index;
}

static inline int64_t textNativeLength(UText* text)
{
    return text->a + text->b;
}

// Relocate pointer from source into destination as required.
static void textFixPointer(const UText* source, UText* destination, const void*& pointer)
{
    if (pointer >= source->pExtra && pointer < static_cast<char*>(source->pExtra) + source->extraSize) {
        // Pointer references source extra buffer.
        pointer = static_cast<char*>(destination->pExtra) + (static_cast<const char*>(pointer) - static_cast<const char*>(source->pExtra));
    } else if (pointer >= source && pointer < reinterpret_cast<const char*>(source) + source->sizeOfStruct) {
        // Pointer references source text structure, but not source extra buffer.
        pointer = reinterpret_cast<char*>(destination) + (static_cast<const char*>(pointer) - reinterpret_cast<const char*>(source));
    }
}

static UText* textClone(UText* destination, const UText* source, UBool deep, UErrorCode* status)
{
    ASSERT_UNUSED(deep, !deep);
    if (U_FAILURE(*status))
        return 0;
    int32_t extraSize = source->extraSize;
    destination = utext_setup(destination, extraSize, status);
    if (U_FAILURE(*status))
        return destination;
    void* extraNew = destination->pExtra;
    int32_t flags = destination->flags;
    int sizeToCopy = min(source->sizeOfStruct, destination->sizeOfStruct);
    memcpy(destination, source, sizeToCopy);
    destination->pExtra = extraNew;
    destination->flags = flags;
    memcpy(destination->pExtra, source->pExtra, extraSize);
    textFixPointer(source, destination, destination->context);
    textFixPointer(source, destination, destination->p);
    textFixPointer(source, destination, destination->q);
    ASSERT(!destination->r);
    const void * chunkContents = static_cast<const void*>(destination->chunkContents);
    textFixPointer(source, destination, chunkContents);
    destination->chunkContents = static_cast<const UChar*>(chunkContents);
    return destination;
}

static int32_t textExtract(UText* text, int64_t start, int64_t limit, UChar* destination, int32_t destinationCapacity, UErrorCode* errorCode)
{
    UNUSED_PARAM(text);
    UNUSED_PARAM(start);
    UNUSED_PARAM(limit);
    UNUSED_PARAM(destination);
    UNUSED_PARAM(destinationCapacity);
    // In the present context, this text provider is used only with ICU functions
    // that do not perform an extract operation.
    ASSERT_NOT_REACHED();
    *errorCode = U_UNSUPPORTED_ERROR;
    return 0;
}

static void textClose(UText* text)
{
    text->context = 0;
}

static inline TextContext textGetContext(const UText* text, int64_t nativeIndex, UBool forward)
{
    if (!text->b || nativeIndex > text->b)
        return PrimaryContext;
    if (nativeIndex == text->b)
        return forward ? PrimaryContext : PriorContext;
    return PriorContext;
}

static inline TextContext textLatin1GetCurrentContext(const UText* text)
{
    if (!text->chunkContents)
        return NoContext;
    return text->chunkContents == text->pExtra ? PrimaryContext : PriorContext;
}

static void textLatin1MoveInPrimaryContext(UText* text, int64_t nativeIndex, int64_t nativeLength, UBool forward)
{
    ASSERT(text->chunkContents == text->pExtra);
    if (forward) {
        ASSERT(nativeIndex >= text->b && nativeIndex < nativeLength);
        text->chunkNativeStart = nativeIndex;
        text->chunkNativeLimit = nativeIndex + text->extraSize / sizeof(UChar);
        if (text->chunkNativeLimit > nativeLength)
            text->chunkNativeLimit = nativeLength;
    } else {
        ASSERT(nativeIndex > text->b && nativeIndex <= nativeLength);
        text->chunkNativeLimit = nativeIndex;
        text->chunkNativeStart = nativeIndex - text->extraSize / sizeof(UChar);
        if (text->chunkNativeStart < text->b)
            text->chunkNativeStart = text->b;
    }
    int64_t length = text->chunkNativeLimit - text->chunkNativeStart;
    // Ensure chunk length is well defined if computed length exceeds int32_t range.
    ASSERT(length < numeric_limits<int32_t>::max());
    text->chunkLength = length < numeric_limits<int32_t>::max() ? static_cast<int32_t>(length) : 0;
    text->nativeIndexingLimit = text->chunkLength;
    text->chunkOffset = forward ? 0 : text->chunkLength;
    StringImpl::copyChars(const_cast<UChar*>(text->chunkContents), static_cast<const LChar*>(text->p) + (text->chunkNativeStart - text->b), static_cast<unsigned>(text->chunkLength));
}

static void textLatin1SwitchToPrimaryContext(UText* text, int64_t nativeIndex, int64_t nativeLength, UBool forward)
{
    ASSERT(!text->chunkContents || text->chunkContents == text->q);
    text->chunkContents = static_cast<const UChar*>(text->pExtra);
    textLatin1MoveInPrimaryContext(text, nativeIndex, nativeLength, forward);
}

static void textLatin1MoveInPriorContext(UText* text, int64_t nativeIndex, int64_t nativeLength, UBool forward)
{
    ASSERT(text->chunkContents == text->q);
    ASSERT(forward ? nativeIndex < text->b : nativeIndex <= text->b);
    ASSERT_UNUSED(nativeLength, forward ? nativeIndex < nativeLength : nativeIndex <= nativeLength);
    ASSERT_UNUSED(forward, forward ? nativeIndex < nativeLength : nativeIndex <= nativeLength);
    text->chunkNativeStart = 0;
    text->chunkNativeLimit = text->b;
    text->chunkLength = text->b;
    text->nativeIndexingLimit = text->chunkLength;
    int64_t offset = nativeIndex - text->chunkNativeStart;
    // Ensure chunk offset is well defined if computed offset exceeds int32_t range or chunk length.
    ASSERT(offset < numeric_limits<int32_t>::max());
    text->chunkOffset = min(offset < numeric_limits<int32_t>::max() ? static_cast<int32_t>(offset) : 0, text->chunkLength);
}

static void textLatin1SwitchToPriorContext(UText* text, int64_t nativeIndex, int64_t nativeLength, UBool forward)
{
    ASSERT(!text->chunkContents || text->chunkContents == text->pExtra);
    text->chunkContents = static_cast<const UChar*>(text->q);
    textLatin1MoveInPriorContext(text, nativeIndex, nativeLength, forward);
}

static inline bool textInChunkOrOutOfRange(UText* text, int64_t nativeIndex, int64_t nativeLength, UBool forward, UBool& isAccessible)
{
    if (forward) {
        if (nativeIndex >= text->chunkNativeStart && nativeIndex < text->chunkNativeLimit) {
            int64_t offset = nativeIndex - text->chunkNativeStart;
            // Ensure chunk offset is well formed if computed offset exceeds int32_t range.
            ASSERT(offset < numeric_limits<int32_t>::max());
            text->chunkOffset = offset < numeric_limits<int32_t>::max() ? static_cast<int32_t>(offset) : 0;
            isAccessible = TRUE;
            return true;
        }
        if (nativeIndex >= nativeLength && text->chunkNativeLimit == nativeLength) {
            text->chunkOffset = text->chunkLength;
            isAccessible = FALSE;
            return true;
        }
    } else {
        if (nativeIndex > text->chunkNativeStart && nativeIndex <= text->chunkNativeLimit) {
            int64_t offset = nativeIndex - text->chunkNativeStart;
            // Ensure chunk offset is well formed if computed offset exceeds int32_t range.
            ASSERT(offset < numeric_limits<int32_t>::max());
            text->chunkOffset = offset < numeric_limits<int32_t>::max() ? static_cast<int32_t>(offset) : 0;
            isAccessible = TRUE;
            return true;
        }
        if (nativeIndex <= 0 && !text->chunkNativeStart) {
            text->chunkOffset = 0;
            isAccessible = FALSE;
            return true;
        }
    }
    return false;
}

static UBool textLatin1Access(UText* text, int64_t nativeIndex, UBool forward)
{
    if (!text->context)
        return FALSE;
    int64_t nativeLength = textNativeLength(text);
    UBool isAccessible;
    if (textInChunkOrOutOfRange(text, nativeIndex, nativeLength, forward, isAccessible))
        return isAccessible;
    nativeIndex = textPinIndex(nativeIndex, nativeLength);
    TextContext currentContext = textLatin1GetCurrentContext(text);
    TextContext newContext = textGetContext(text, nativeIndex, forward);
    ASSERT(newContext != NoContext);
    if (newContext == currentContext) {
        if (currentContext == PrimaryContext)
            textLatin1MoveInPrimaryContext(text, nativeIndex, nativeLength, forward);
        else
            textLatin1MoveInPriorContext(text, nativeIndex, nativeLength, forward);
    } else if (newContext == PrimaryContext)
        textLatin1SwitchToPrimaryContext(text, nativeIndex, nativeLength, forward);
    else {
        ASSERT(newContext == PriorContext);
        textLatin1SwitchToPriorContext(text, nativeIndex, nativeLength, forward);
    }
    return TRUE;
}

static const struct UTextFuncs textLatin1Funcs = {
    sizeof(UTextFuncs),
    0, 0, 0,
    textClone,
    textNativeLength,
    textLatin1Access,
    textExtract,
    0, 0, 0, 0,
    textClose,
    0, 0, 0,
};

static void textInit(UText* text, const UTextFuncs* funcs, const void* string, unsigned length, const UChar* priorContext, int priorContextLength)
{
    text->pFuncs = funcs;
    text->providerProperties = 1 << UTEXT_PROVIDER_STABLE_CHUNKS;
    text->context = string;
    text->p = string;
    text->a = length;
    text->q = priorContext;
    text->b = priorContextLength;
}

static UText* textOpenLatin1(UTextWithBuffer* utWithBuffer, const LChar* string, unsigned length, const UChar* priorContext, int priorContextLength, UErrorCode* status)
{
    if (U_FAILURE(*status))
        return 0;
    if (!string || length > static_cast<unsigned>(numeric_limits<int32_t>::max())) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    UText* text = utext_setup(&utWithBuffer->text, sizeof(utWithBuffer->buffer), status);
    if (U_FAILURE(*status)) {
        ASSERT(!text);
        return 0;
    }
    textInit(text, &textLatin1Funcs, string, length, priorContext, priorContextLength);
    return text;
}

static inline TextContext textUTF16GetCurrentContext(const UText* text)
{
    if (!text->chunkContents)
        return NoContext;
    return text->chunkContents == text->p ? PrimaryContext : PriorContext;
}

static void textUTF16MoveInPrimaryContext(UText* text, int64_t nativeIndex, int64_t nativeLength, UBool forward)
{
    ASSERT(text->chunkContents == text->p);
    ASSERT_UNUSED(forward, forward ? nativeIndex >= text->b : nativeIndex > text->b);
    ASSERT_UNUSED(forward, forward ? nativeIndex < nativeLength : nativeIndex <= nativeLength);
    text->chunkNativeStart = text->b;
    text->chunkNativeLimit = nativeLength;
    int64_t length = text->chunkNativeLimit - text->chunkNativeStart;
    // Ensure chunk length is well defined if computed length exceeds int32_t range.
    ASSERT(length < numeric_limits<int32_t>::max());
    text->chunkLength = length < numeric_limits<int32_t>::max() ? static_cast<int32_t>(length) : 0;
    text->nativeIndexingLimit = text->chunkLength;
    int64_t offset = nativeIndex - text->chunkNativeStart;
    // Ensure chunk offset is well defined if computed offset exceeds int32_t range or chunk length.
    ASSERT(offset < numeric_limits<int32_t>::max());
    text->chunkOffset = min(offset < numeric_limits<int32_t>::max() ? static_cast<int32_t>(offset) : 0, text->chunkLength);
}

static void textUTF16SwitchToPrimaryContext(UText* text, int64_t nativeIndex, int64_t nativeLength, UBool forward)
{
    ASSERT(!text->chunkContents || text->chunkContents == text->q);
    text->chunkContents = static_cast<const UChar*>(text->p);
    textUTF16MoveInPrimaryContext(text, nativeIndex, nativeLength, forward);
}

static void textUTF16MoveInPriorContext(UText* text, int64_t nativeIndex, int64_t nativeLength, UBool forward)
{
    ASSERT(text->chunkContents == text->q);
    ASSERT(forward ? nativeIndex < text->b : nativeIndex <= text->b);
    ASSERT_UNUSED(nativeLength, forward ? nativeIndex < nativeLength : nativeIndex <= nativeLength);
    ASSERT_UNUSED(forward, forward ? nativeIndex < nativeLength : nativeIndex <= nativeLength);
    text->chunkNativeStart = 0;
    text->chunkNativeLimit = text->b;
    text->chunkLength = text->b;
    text->nativeIndexingLimit = text->chunkLength;
    int64_t offset = nativeIndex - text->chunkNativeStart;
    // Ensure chunk offset is well defined if computed offset exceeds int32_t range or chunk length.
    ASSERT(offset < numeric_limits<int32_t>::max());
    text->chunkOffset = min(offset < numeric_limits<int32_t>::max() ? static_cast<int32_t>(offset) : 0, text->chunkLength);
}

static void textUTF16SwitchToPriorContext(UText* text, int64_t nativeIndex, int64_t nativeLength, UBool forward)
{
    ASSERT(!text->chunkContents || text->chunkContents == text->p);
    text->chunkContents = static_cast<const UChar*>(text->q);
    textUTF16MoveInPriorContext(text, nativeIndex, nativeLength, forward);
}

static UBool textUTF16Access(UText* text, int64_t nativeIndex, UBool forward)
{
    if (!text->context)
        return FALSE;
    int64_t nativeLength = textNativeLength(text);
    UBool isAccessible;
    if (textInChunkOrOutOfRange(text, nativeIndex, nativeLength, forward, isAccessible))
        return isAccessible;
    nativeIndex = textPinIndex(nativeIndex, nativeLength);
    TextContext currentContext = textUTF16GetCurrentContext(text);
    TextContext newContext = textGetContext(text, nativeIndex, forward);
    ASSERT(newContext != NoContext);
    if (newContext == currentContext) {
        if (currentContext == PrimaryContext)
            textUTF16MoveInPrimaryContext(text, nativeIndex, nativeLength, forward);
        else
            textUTF16MoveInPriorContext(text, nativeIndex, nativeLength, forward);
    } else if (newContext == PrimaryContext)
        textUTF16SwitchToPrimaryContext(text, nativeIndex, nativeLength, forward);
    else {
        ASSERT(newContext == PriorContext);
        textUTF16SwitchToPriorContext(text, nativeIndex, nativeLength, forward);
    }
    return TRUE;
}

static const struct UTextFuncs textUTF16Funcs = {
    sizeof(UTextFuncs),
    0, 0, 0,
    textClone,
    textNativeLength,
    textUTF16Access,
    textExtract,
    0, 0, 0, 0,
    textClose,
    0, 0, 0,
};

static UText* textOpenUTF16(UText* text, const UChar* string, unsigned length, const UChar* priorContext, int priorContextLength, UErrorCode* status)
{
    if (U_FAILURE(*status))
        return 0;
    if (!string || length > static_cast<unsigned>(numeric_limits<int32_t>::max())) {
        *status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }
    text = utext_setup(text, 0, status);
    if (U_FAILURE(*status)) {
        ASSERT(!text);
        return 0;
    }
    textInit(text, &textUTF16Funcs, string, length, priorContext, priorContextLength);
    return text;
}

TextBreakIterator* wordBreakIterator(const UChar* string, int length)
{
    static bool createdWordBreakIterator = false;
    static TextBreakIterator* staticWordBreakIterator;
    return setUpIterator(createdWordBreakIterator,
        staticWordBreakIterator, UBRK_WORD, string, length);
}

static UText emptyText = UTEXT_INITIALIZER;

TextBreakIterator* acquireLineBreakIterator(const LChar* string, int length, const AtomicString& locale, const UChar* priorContext, unsigned priorContextLength)
{
    UBreakIterator* iterator = LineBreakIteratorPool::sharedPool().take(locale);
    if (!iterator)
        return 0;

    UTextWithBuffer textLocal;
    textLocal.text = emptyText;
    textLocal.text.extraSize = sizeof(textLocal.buffer);
    textLocal.text.pExtra = textLocal.buffer;

    UErrorCode openStatus = U_ZERO_ERROR;
    UText* text = textOpenLatin1(&textLocal, string, length, priorContext, priorContextLength, &openStatus);
    if (U_FAILURE(openStatus)) {
        LOG_ERROR("textOpenUTF16 failed with status %d", openStatus);
        return 0;
    }

    UErrorCode setTextStatus = U_ZERO_ERROR;
    ubrk_setUText(iterator, text, &setTextStatus);
    if (U_FAILURE(setTextStatus)) {
        LOG_ERROR("ubrk_setUText failed with status %d", setTextStatus);
        return 0;
    }

    utext_close(text);

    return reinterpret_cast<TextBreakIterator*>(iterator);
}

TextBreakIterator* acquireLineBreakIterator(const UChar* string, int length, const AtomicString& locale, const UChar* priorContext, unsigned priorContextLength)
{
    UBreakIterator* iterator = LineBreakIteratorPool::sharedPool().take(locale);
    if (!iterator)
        return 0;

    UText textLocal = UTEXT_INITIALIZER;

    UErrorCode openStatus = U_ZERO_ERROR;
    UText* text = textOpenUTF16(&textLocal, string, length, priorContext, priorContextLength, &openStatus);
    if (U_FAILURE(openStatus)) {
        LOG_ERROR("textOpenUTF16 failed with status %d", openStatus);
        return 0;
    }

    UErrorCode setTextStatus = U_ZERO_ERROR;
    ubrk_setUText(iterator, text, &setTextStatus);
    if (U_FAILURE(setTextStatus)) {
        LOG_ERROR("ubrk_setUText failed with status %d", setTextStatus);
        return 0;
    }

    utext_close(text);

    return reinterpret_cast<TextBreakIterator*>(iterator);
}

void releaseLineBreakIterator(TextBreakIterator* iterator)
{
    ASSERT_ARG(iterator, iterator);

    LineBreakIteratorPool::sharedPool().put(reinterpret_cast<UBreakIterator*>(iterator));
}

static TextBreakIterator* nonSharedCharacterBreakIterator;

static inline bool compareAndSwapNonSharedCharacterBreakIterator(TextBreakIterator* expected, TextBreakIterator* newValue)
{
#if ENABLE(COMPARE_AND_SWAP)
    return weakCompareAndSwap(reinterpret_cast<void**>(&nonSharedCharacterBreakIterator), expected, newValue);
#else
    DEFINE_STATIC_LOCAL(Mutex, nonSharedCharacterBreakIteratorMutex, ());
    MutexLocker locker(nonSharedCharacterBreakIteratorMutex);
    if (nonSharedCharacterBreakIterator != expected)
        return false;
    nonSharedCharacterBreakIterator = newValue;
    return true;
#endif
}

NonSharedCharacterBreakIterator::NonSharedCharacterBreakIterator(const UChar* buffer, int length)
{
    m_iterator = nonSharedCharacterBreakIterator;
    bool createdIterator = m_iterator && compareAndSwapNonSharedCharacterBreakIterator(m_iterator, 0);
    m_iterator = setUpIterator(createdIterator, m_iterator, UBRK_CHARACTER, buffer, length);
}

NonSharedCharacterBreakIterator::~NonSharedCharacterBreakIterator()
{
    if (!compareAndSwapNonSharedCharacterBreakIterator(0, m_iterator))
        ubrk_close(reinterpret_cast<UBreakIterator*>(m_iterator));
}

TextBreakIterator* sentenceBreakIterator(const UChar* string, int length)
{
    static bool createdSentenceBreakIterator = false;
    static TextBreakIterator* staticSentenceBreakIterator;
    return setUpIterator(createdSentenceBreakIterator,
        staticSentenceBreakIterator, UBRK_SENTENCE, string, length);
}

int textBreakFirst(TextBreakIterator* iterator)
{
    return ubrk_first(reinterpret_cast<UBreakIterator*>(iterator));
}

int textBreakLast(TextBreakIterator* iterator)
{
    return ubrk_last(reinterpret_cast<UBreakIterator*>(iterator));
}

int textBreakNext(TextBreakIterator* iterator)
{
    return ubrk_next(reinterpret_cast<UBreakIterator*>(iterator));
}

int textBreakPrevious(TextBreakIterator* iterator)
{
    return ubrk_previous(reinterpret_cast<UBreakIterator*>(iterator));
}

int textBreakPreceding(TextBreakIterator* iterator, int pos)
{
    return ubrk_preceding(reinterpret_cast<UBreakIterator*>(iterator), pos);
}

int textBreakFollowing(TextBreakIterator* iterator, int pos)
{
    return ubrk_following(reinterpret_cast<UBreakIterator*>(iterator), pos);
}

int textBreakCurrent(TextBreakIterator* iterator)
{
    return ubrk_current(reinterpret_cast<UBreakIterator*>(iterator));
}

bool isTextBreak(TextBreakIterator* iterator, int position)
{
    return ubrk_isBoundary(reinterpret_cast<UBreakIterator*>(iterator), position);
}

bool isWordTextBreak(TextBreakIterator* iterator)
{
    int ruleStatus = ubrk_getRuleStatus(reinterpret_cast<UBreakIterator*>(iterator));
    return ruleStatus != UBRK_WORD_NONE;
}

static TextBreakIterator* setUpIteratorWithRules(bool& createdIterator, TextBreakIterator*& iterator,
    const char* breakRules, const UChar* string, int length)
{
    if (!string)
        return 0;

    if (!createdIterator) {
        UParseError parseStatus;
        UErrorCode openStatus = U_ZERO_ERROR;
        String rules(breakRules);
        iterator = reinterpret_cast<TextBreakIterator*>(ubrk_openRules(rules.characters(), rules.length(), 0, 0, &parseStatus, &openStatus));
        createdIterator = true;
        ASSERT_WITH_MESSAGE(U_SUCCESS(openStatus), "ICU could not open a break iterator: %s (%d)", u_errorName(openStatus), openStatus);
    }
    if (!iterator)
        return 0;

    UErrorCode setTextStatus = U_ZERO_ERROR;
    ubrk_setText(reinterpret_cast<UBreakIterator*>(iterator), string, length, &setTextStatus);
    if (U_FAILURE(setTextStatus))
        return 0;

    return iterator;
}

TextBreakIterator* cursorMovementIterator(const UChar* string, int length)
{
    // This rule set is based on character-break iterator rules of ICU 4.0
    // <http://source.icu-project.org/repos/icu/icu/tags/release-4-0/source/data/brkitr/char.txt>.
    // The major differences from the original ones are listed below:
    // * Replaced '[\p{Grapheme_Cluster_Break = SpacingMark}]' with '[\p{General_Category = Spacing Mark} - $Extend]' for ICU 3.8 or earlier;
    // * Removed rules that prevent a cursor from moving after prepend characters (Bug 24342);
    // * Added rules that prevent a cursor from moving after virama signs of Indic languages except Tamil (Bug 15790), and;
    // * Added rules that prevent a cursor from moving before Japanese half-width katakara voiced marks.
    // * Added rules for regional indicator symbols.
    static const char* kRules =
        "$CR      = [\\p{Grapheme_Cluster_Break = CR}];"
        "$LF      = [\\p{Grapheme_Cluster_Break = LF}];"
        "$Control = [\\p{Grapheme_Cluster_Break = Control}];"
        "$VoiceMarks = [\\uFF9E\\uFF9F];"  // Japanese half-width katakana voiced marks
        "$Extend  = [\\p{Grapheme_Cluster_Break = Extend} $VoiceMarks - [\\u0E30 \\u0E32 \\u0E45 \\u0EB0 \\u0EB2]];"
        "$SpacingMark = [[\\p{General_Category = Spacing Mark}] - $Extend];"
        "$L       = [\\p{Grapheme_Cluster_Break = L}];"
        "$V       = [\\p{Grapheme_Cluster_Break = V}];"
        "$T       = [\\p{Grapheme_Cluster_Break = T}];"
        "$LV      = [\\p{Grapheme_Cluster_Break = LV}];"
        "$LVT     = [\\p{Grapheme_Cluster_Break = LVT}];"
        "$Hin0    = [\\u0905-\\u0939];"    // Devanagari Letter A,...,Ha
        "$HinV    = \\u094D;"              // Devanagari Sign Virama
        "$Hin1    = [\\u0915-\\u0939];"    // Devanagari Letter Ka,...,Ha
        "$Ben0    = [\\u0985-\\u09B9];"    // Bengali Letter A,...,Ha
        "$BenV    = \\u09CD;"              // Bengali Sign Virama
        "$Ben1    = [\\u0995-\\u09B9];"    // Bengali Letter Ka,...,Ha
        "$Pan0    = [\\u0A05-\\u0A39];"    // Gurmukhi Letter A,...,Ha
        "$PanV    = \\u0A4D;"              // Gurmukhi Sign Virama
        "$Pan1    = [\\u0A15-\\u0A39];"    // Gurmukhi Letter Ka,...,Ha
        "$Guj0    = [\\u0A85-\\u0AB9];"    // Gujarati Letter A,...,Ha
        "$GujV    = \\u0ACD;"              // Gujarati Sign Virama
        "$Guj1    = [\\u0A95-\\u0AB9];"    // Gujarati Letter Ka,...,Ha
        "$Ori0    = [\\u0B05-\\u0B39];"    // Oriya Letter A,...,Ha
        "$OriV    = \\u0B4D;"              // Oriya Sign Virama
        "$Ori1    = [\\u0B15-\\u0B39];"    // Oriya Letter Ka,...,Ha
        "$Tel0    = [\\u0C05-\\u0C39];"    // Telugu Letter A,...,Ha
        "$TelV    = \\u0C4D;"              // Telugu Sign Virama
        "$Tel1    = [\\u0C14-\\u0C39];"    // Telugu Letter Ka,...,Ha
        "$Kan0    = [\\u0C85-\\u0CB9];"    // Kannada Letter A,...,Ha
        "$KanV    = \\u0CCD;"              // Kannada Sign Virama
        "$Kan1    = [\\u0C95-\\u0CB9];"    // Kannada Letter A,...,Ha
        "$Mal0    = [\\u0D05-\\u0D39];"    // Malayalam Letter A,...,Ha
        "$MalV    = \\u0D4D;"              // Malayalam Sign Virama
        "$Mal1    = [\\u0D15-\\u0D39];"    // Malayalam Letter A,...,Ha
        "$RI      = [\\U0001F1E6-\\U0001F1FF];" // Emoji regional indicators
        "!!chain;"
        "!!forward;"
        "$CR $LF;"
        "$L ($L | $V | $LV | $LVT);"
        "($LV | $V) ($V | $T);"
        "($LVT | $T) $T;"
        "[^$Control $CR $LF] $Extend;"
        "[^$Control $CR $LF] $SpacingMark;"
        "$RI $RI / $RI;"
        "$RI $RI;"
        "$Hin0 $HinV $Hin1;"               // Devanagari Virama (forward)
        "$Ben0 $BenV $Ben1;"               // Bengali Virama (forward)
        "$Pan0 $PanV $Pan1;"               // Gurmukhi Virama (forward)
        "$Guj0 $GujV $Guj1;"               // Gujarati Virama (forward)
        "$Ori0 $OriV $Ori1;"               // Oriya Virama (forward)
        "$Tel0 $TelV $Tel1;"               // Telugu Virama (forward)
        "$Kan0 $KanV $Kan1;"               // Kannada Virama (forward)
        "$Mal0 $MalV $Mal1;"               // Malayalam Virama (forward)
        "!!reverse;"
        "$LF $CR;"
        "($L | $V | $LV | $LVT) $L;"
        "($V | $T) ($LV | $V);"
        "$T ($LVT | $T);"
        "$Extend      [^$Control $CR $LF];"
        "$SpacingMark [^$Control $CR $LF];"
        "$RI $RI / $RI $RI;"
        "$RI $RI;"
        "$Hin1 $HinV $Hin0;"               // Devanagari Virama (backward)
        "$Ben1 $BenV $Ben0;"               // Bengali Virama (backward)
        "$Pan1 $PanV $Pan0;"               // Gurmukhi Virama (backward)
        "$Guj1 $GujV $Guj0;"               // Gujarati Virama (backward)
        "$Ori1 $OriV $Ori0;"               // Gujarati Virama (backward)
        "$Tel1 $TelV $Tel0;"               // Telugu Virama (backward)
        "$Kan1 $KanV $Kan0;"               // Kannada Virama (backward)
        "$Mal1 $MalV $Mal0;"               // Malayalam Virama (backward)
        "!!safe_reverse;"
        "!!safe_forward;";
    static bool createdCursorMovementIterator = false;
    static TextBreakIterator* staticCursorMovementIterator;
    return setUpIteratorWithRules(createdCursorMovementIterator, staticCursorMovementIterator, kRules, string, length);
}

}

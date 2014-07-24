/*
 * Copyright (C) 2006, 2007, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
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
#include "TextEncodingRegistry.h"

#include "TextCodecLatin1.h"
#include "TextCodecUserDefined.h"
#include "TextCodecUTF16.h"
#include "TextCodecUTF8.h"
#include "TextEncoding.h"
#include <wtf/ASCIICType.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringExtras.h>

#if USE(ICU_UNICODE)
#include "TextCodecICU.h"
#endif
#if PLATFORM(MAC)
#include "TextCodecMac.h"
#endif
#if OS(WINDOWS) && USE(WCHAR_UNICODE)
#include "win/TextCodecWin.h"
#endif

#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>

using namespace WTF;

namespace WebCore {

const size_t maxEncodingNameLength = 63;

// Hash for all-ASCII strings that does case folding.
struct TextEncodingNameHash {
    static bool equal(const char* s1, const char* s2)
    {
        char c1;
        char c2;
        do {
#if defined(_MSC_VER) && _MSC_VER == 1700
            // Workaround for a bug in the VS2012 Update1 and Update2 optimizer, remove once the fix is released.
            // https://connect.microsoft.com/VisualStudio/feedback/details/781189/vs2012-update-ctp4-c-optimizing-bug
            c1 = toASCIILower(*s1++);
            c2 = toASCIILower(*s2++);
            if (c1 != c2)
                return false;
#else
            c1 = *s1++;
            c2 = *s2++;
            if (toASCIILower(c1) != toASCIILower(c2))
                return false;
#endif
        } while (c1 && c2);
        return !c1 && !c2;
    }

    // This algorithm is the one-at-a-time hash from:
    // http://burtleburtle.net/bob/hash/hashfaq.html
    // http://burtleburtle.net/bob/hash/doobs.html
    static unsigned hash(const char* s)
    {
        unsigned h = WTF::stringHashingStartValue;
        for (;;) {
            char c = *s++;
            if (!c) {
                h += (h << 3);
                h ^= (h >> 11);
                h += (h << 15);
                return h;
            }
            h += toASCIILower(c);
            h += (h << 10); 
            h ^= (h >> 6); 
        }
    }

    static const bool safeToCompareToEmptyOrDeleted = false;
};

struct TextCodecFactory {
    NewTextCodecFunction function;
    const void* additionalData;
    TextCodecFactory(NewTextCodecFunction f = 0, const void* d = 0) : function(f), additionalData(d) { }
};

typedef HashMap<const char*, const char*, TextEncodingNameHash> TextEncodingNameMap;
typedef HashMap<const char*, TextCodecFactory> TextCodecMap;

static Mutex& encodingRegistryMutex()
{
    // We don't have to use AtomicallyInitializedStatic here because
    // this function is called on the main thread for any page before
    // it is used in worker threads.
    DEFINE_STATIC_LOCAL(Mutex, mutex, ());
    return mutex;
}

static TextEncodingNameMap* textEncodingNameMap;
static TextCodecMap* textCodecMap;
static bool didExtendTextCodecMaps;
static HashSet<const char*>* japaneseEncodings;
static HashSet<const char*>* nonBackslashEncodings;

static const char* const textEncodingNameBlacklist[] = { "UTF-7" };

#if ERROR_DISABLED

static inline void checkExistingName(const char*, const char*) { }

#else

static void checkExistingName(const char* alias, const char* atomicName)
{
    const char* oldAtomicName = textEncodingNameMap->get(alias);
    if (!oldAtomicName)
        return;
    if (oldAtomicName == atomicName)
        return;
    // Keep the warning silent about one case where we know this will happen.
    if (strcmp(alias, "ISO-8859-8-I") == 0
            && strcmp(oldAtomicName, "ISO-8859-8-I") == 0
            && strcasecmp(atomicName, "iso-8859-8") == 0)
        return;
    LOG_ERROR("alias %s maps to %s already, but someone is trying to make it map to %s", alias, oldAtomicName, atomicName);
}

#endif

static bool isUndesiredAlias(const char* alias)
{
    // Reject aliases with version numbers that are supported by some back-ends (such as "ISO_2022,locale=ja,version=0" in ICU).
    for (const char* p = alias; *p; ++p) {
        if (*p == ',')
            return true;
    }
    // 8859_1 is known to (at least) ICU, but other browsers don't support this name - and having it caused a compatibility
    // problem, see bug 43554.
    if (0 == strcmp(alias, "8859_1"))
        return true;
    return false;
}

static void addToTextEncodingNameMap(const char* alias, const char* name)
{
    ASSERT(strlen(alias) <= maxEncodingNameLength);
    if (isUndesiredAlias(alias))
        return;
    const char* atomicName = textEncodingNameMap->get(name);
    ASSERT(strcmp(alias, name) == 0 || atomicName);
    if (!atomicName)
        atomicName = name;
    checkExistingName(alias, atomicName);
    textEncodingNameMap->add(alias, atomicName);
}

static void addToTextCodecMap(const char* name, NewTextCodecFunction function, const void* additionalData)
{
    const char* atomicName = textEncodingNameMap->get(name);
    ASSERT(atomicName);
    textCodecMap->add(atomicName, TextCodecFactory(function, additionalData));
}

static void pruneBlacklistedCodecs()
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(textEncodingNameBlacklist); ++i) {
        const char* atomicName = textEncodingNameMap->get(textEncodingNameBlacklist[i]);
        if (!atomicName)
            continue;

        Vector<const char*> names;
        TextEncodingNameMap::const_iterator it = textEncodingNameMap->begin();
        TextEncodingNameMap::const_iterator end = textEncodingNameMap->end();
        for (; it != end; ++it) {
            if (it->value == atomicName)
                names.append(it->key);
        }

        size_t length = names.size();
        for (size_t j = 0; j < length; ++j)
            textEncodingNameMap->remove(names[j]);

        textCodecMap->remove(atomicName);
    }
}

static void buildBaseTextCodecMaps()
{
    ASSERT(isMainThread());
    ASSERT(!textCodecMap);
    ASSERT(!textEncodingNameMap);

    textCodecMap = new TextCodecMap;
    textEncodingNameMap = new TextEncodingNameMap;

    TextCodecLatin1::registerEncodingNames(addToTextEncodingNameMap);
    TextCodecLatin1::registerCodecs(addToTextCodecMap);

    TextCodecUTF8::registerEncodingNames(addToTextEncodingNameMap);
    TextCodecUTF8::registerCodecs(addToTextCodecMap);

    TextCodecUTF16::registerEncodingNames(addToTextEncodingNameMap);
    TextCodecUTF16::registerCodecs(addToTextCodecMap);

    TextCodecUserDefined::registerEncodingNames(addToTextEncodingNameMap);
    TextCodecUserDefined::registerCodecs(addToTextCodecMap);
}

static void addEncodingName(HashSet<const char*>* set, const char* name)
{
    // We must not use atomicCanonicalTextEncodingName() because this function is called in it.
    const char* atomicName = textEncodingNameMap->get(name);
    if (atomicName)
        set->add(atomicName);
}

static void buildQuirksSets()
{
    // FIXME: Having isJapaneseEncoding() and shouldShowBackslashAsCurrencySymbolIn()
    // and initializing the sets for them in TextEncodingRegistry.cpp look strange.

    ASSERT(!japaneseEncodings);
    ASSERT(!nonBackslashEncodings);

    japaneseEncodings = new HashSet<const char*>;
    addEncodingName(japaneseEncodings, "EUC-JP");
    addEncodingName(japaneseEncodings, "ISO-2022-JP");
    addEncodingName(japaneseEncodings, "ISO-2022-JP-1");
    addEncodingName(japaneseEncodings, "ISO-2022-JP-2");
    addEncodingName(japaneseEncodings, "ISO-2022-JP-3");
    addEncodingName(japaneseEncodings, "JIS_C6226-1978");
    addEncodingName(japaneseEncodings, "JIS_X0201");
    addEncodingName(japaneseEncodings, "JIS_X0208-1983");
    addEncodingName(japaneseEncodings, "JIS_X0208-1990");
    addEncodingName(japaneseEncodings, "JIS_X0212-1990");
    addEncodingName(japaneseEncodings, "Shift_JIS");
    addEncodingName(japaneseEncodings, "Shift_JIS_X0213-2000");
    addEncodingName(japaneseEncodings, "cp932");
    addEncodingName(japaneseEncodings, "x-mac-japanese");

    nonBackslashEncodings = new HashSet<const char*>;
    // The text encodings below treat backslash as a currency symbol for IE compatibility.
    // See http://blogs.msdn.com/michkap/archive/2005/09/17/469941.aspx for more information.
    addEncodingName(nonBackslashEncodings, "x-mac-japanese");
    addEncodingName(nonBackslashEncodings, "ISO-2022-JP");
    addEncodingName(nonBackslashEncodings, "EUC-JP");
    // Shift_JIS_X0213-2000 is not the same encoding as Shift_JIS on Mac. We need to register both of them.
    addEncodingName(nonBackslashEncodings, "Shift_JIS");
    addEncodingName(nonBackslashEncodings, "Shift_JIS_X0213-2000");
}

bool isJapaneseEncoding(const char* canonicalEncodingName)
{
    return canonicalEncodingName && japaneseEncodings && japaneseEncodings->contains(canonicalEncodingName);
}

bool shouldShowBackslashAsCurrencySymbolIn(const char* canonicalEncodingName)
{
    return canonicalEncodingName && nonBackslashEncodings && nonBackslashEncodings->contains(canonicalEncodingName);
}

static void extendTextCodecMaps()
{
#if USE(ICU_UNICODE)
    TextCodecICU::registerEncodingNames(addToTextEncodingNameMap);
    TextCodecICU::registerCodecs(addToTextCodecMap);
#endif

#if PLATFORM(MAC)
    TextCodecMac::registerEncodingNames(addToTextEncodingNameMap);
    TextCodecMac::registerCodecs(addToTextCodecMap);
#endif

#if OS(WINDOWS) && USE(WCHAR_UNICODE)
    TextCodecWin::registerExtendedEncodingNames(addToTextEncodingNameMap);
    TextCodecWin::registerExtendedCodecs(addToTextCodecMap);
#endif

    pruneBlacklistedCodecs();
    buildQuirksSets();
}

PassOwnPtr<TextCodec> newTextCodec(const TextEncoding& encoding)
{
    MutexLocker lock(encodingRegistryMutex());

    ASSERT(textCodecMap);
    TextCodecFactory factory = textCodecMap->get(encoding.name());
    ASSERT(factory.function);
    return factory.function(encoding, factory.additionalData);
}

const char* atomicCanonicalTextEncodingName(const char* name)
{
    if (!name || !name[0])
        return 0;
    if (!textEncodingNameMap)
        buildBaseTextCodecMaps();

    MutexLocker lock(encodingRegistryMutex());

    if (const char* atomicName = textEncodingNameMap->get(name))
        return atomicName;
    if (didExtendTextCodecMaps)
        return 0;
    extendTextCodecMaps();
    didExtendTextCodecMaps = true;
    return textEncodingNameMap->get(name);
}

template <typename CharacterType>
const char* atomicCanonicalTextEncodingName(const CharacterType* characters, size_t length)
{
    char buffer[maxEncodingNameLength + 1];
    size_t j = 0;
    for (size_t i = 0; i < length; ++i) {
        CharacterType c = characters[i];
        if (j == maxEncodingNameLength)
            return 0;
        buffer[j++] = c;
    }
    buffer[j] = 0;
    return atomicCanonicalTextEncodingName(buffer);
}

const char* atomicCanonicalTextEncodingName(const String& alias)
{
    if (!alias.length())
        return 0;

    if (alias.is8Bit())
        return atomicCanonicalTextEncodingName<LChar>(alias.characters8(), alias.length());

    return atomicCanonicalTextEncodingName<UChar>(alias.characters(), alias.length());
}

bool noExtendedTextEncodingNameUsed()
{
    // If the calling thread did not use extended encoding names, it is fine for it to use a stale false value.
    return !didExtendTextCodecMaps;
}

#ifndef NDEBUG
void dumpTextEncodingNameMap()
{
    unsigned size = textEncodingNameMap->size();
    fprintf(stderr, "Dumping %u entries in WebCore::textEncodingNameMap...\n", size);

    MutexLocker lock(encodingRegistryMutex());

    TextEncodingNameMap::const_iterator it = textEncodingNameMap->begin();
    TextEncodingNameMap::const_iterator end = textEncodingNameMap->end();
    for (; it != end; ++it)
        fprintf(stderr, "'%s' => '%s'\n", it->key, it->value);
}
#endif

} // namespace WebCore

/*
 * Copyright (C) 2010, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Language.h"

#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(MAC)
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace WebCore {

typedef HashMap<void*, LanguageChangeObserverFunction> ObserverMap;
static ObserverMap& observerMap()
{
    DEFINE_STATIC_LOCAL(ObserverMap, map, ());
    return map;
}

void addLanguageChangeObserver(void* context, LanguageChangeObserverFunction customObserver)
{
    observerMap().set(context, customObserver);
}

void removeLanguageChangeObserver(void* context)
{
    ASSERT(observerMap().contains(context));
    observerMap().remove(context);
}

void languageDidChange()
{
    ObserverMap::iterator end = observerMap().end();
    for (ObserverMap::iterator iter = observerMap().begin(); iter != end; ++iter)
        iter->value(iter->key);
}

String defaultLanguage()
{
    Vector<String> languages = userPreferredLanguages();
    if (languages.size())
        return languages[0];

    return emptyString();
}

static Vector<String>& preferredLanguagesOverride()
{
    DEFINE_STATIC_LOCAL(Vector<String>, override, ());
    return override;
}

Vector<String> userPreferredLanguagesOverride()
{
    return preferredLanguagesOverride();
}

void overrideUserPreferredLanguages(const Vector<String>& override)
{
    preferredLanguagesOverride() = override;
}
    
Vector<String> userPreferredLanguages()
{
    Vector<String>& override = preferredLanguagesOverride();
    if (!override.isEmpty())
        return override;
    
    return platformUserPreferredLanguages();
}

static String canonicalLanguageIdentifier(const String& languageCode)
{
    String lowercaseLanguageCode = languageCode.lower();
    
    if (lowercaseLanguageCode.length() >= 3 && lowercaseLanguageCode[2] == '_')
        lowercaseLanguageCode.replace(2, 1, "-");

    return lowercaseLanguageCode;
}

size_t indexOfBestMatchingLanguageInList(const String& language, const Vector<String>& languageList)
{
    String languageWithoutLocaleMatch;
    String languageMatchButNotLocale;
    size_t languageWithoutLocaleMatchIndex = 0;
    size_t languageMatchButNotLocaleMatchIndex = 0;
    bool canMatchLanguageOnly = (language.length() == 2 || (language.length() >= 3 && language[2] == '-'));

    for (size_t i = 0; i < languageList.size(); ++i) {
        String canonicalizedLanguageFromList = canonicalLanguageIdentifier(languageList[i]);

        if (language == canonicalizedLanguageFromList)
            return i;

        if (canMatchLanguageOnly && canonicalizedLanguageFromList.length() >= 2) {
            if (language[0] == canonicalizedLanguageFromList[0] && language[1] == canonicalizedLanguageFromList[1]) {
                if (!languageWithoutLocaleMatch.length() && canonicalizedLanguageFromList.length() == 2) {
                    languageWithoutLocaleMatch = languageList[i];
                    languageWithoutLocaleMatchIndex = i;
                }
                if (!languageMatchButNotLocale.length() && canonicalizedLanguageFromList.length() >= 3) {
                    languageMatchButNotLocale = languageList[i];
                    languageMatchButNotLocaleMatchIndex = i;
                }
            }
        }
    }

    // If we have both a language-only match and a languge-but-not-locale match, return the
    // languge-only match as is considered a "better" match. For example, if the list
    // provided has both "en-GB" and "en" and the user prefers "en-US" we will return "en".
    if (languageWithoutLocaleMatch.length())
        return languageWithoutLocaleMatchIndex;

    if (languageMatchButNotLocale.length())
        return languageMatchButNotLocaleMatchIndex;

    return languageList.size();
}

String displayNameForLanguageLocale(const String& localeName)
{
#if PLATFORM(MAC)
    if (!localeName.isNull() && !localeName.isEmpty()) {
        RetainPtr<CFLocaleRef> currentLocale = adoptCF(CFLocaleCopyCurrent());
        return CFLocaleCopyDisplayNameForPropertyValue(currentLocale.get(), kCFLocaleIdentifier, localeName.createCFString().get());
    }
#endif
    return localeName;
}

}

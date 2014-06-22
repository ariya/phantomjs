/*
 * Copyright (C) 2012 Igalia S.L.
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

#ifndef WebKitTextChecker_h
#define WebKitTextChecker_h

#if ENABLE(SPELLCHECK)

#include <WebCore/TextCheckerEnchant.h>
#include <wtf/FastAllocBase.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

class WebKitTextChecker {
    WTF_MAKE_FAST_ALLOCATED;

public:
    static PassOwnPtr<WebKitTextChecker> create() { return adoptPtr(new WebKitTextChecker()); }
    virtual ~WebKitTextChecker();

    // For implementing TextCheckerClient.
    bool isSpellCheckingEnabled() { return m_spellCheckingEnabled; }
    void setSpellCheckingEnabled(bool enabled);
    void checkSpellingOfString(const String& string, int& misspellingLocation, int& misspellingLength);
    Vector<String> getGuessesForWord(const String& word);
    void learnWord(const String& word);
    void ignoreWord(const String& word);

    // To be called from WebKitWebContext only.
    const char* const* getSpellCheckingLanguages();
    void setSpellCheckingLanguages(const char* const* spellCheckingLanguages);

private:
    WebKitTextChecker();

    OwnPtr<WebCore::TextCheckerEnchant> m_textChecker;
    GRefPtr<GPtrArray> m_spellCheckingLanguages;
    bool m_spellCheckingEnabled;
};

#endif // ENABLE(SPELLCHECK)

#endif // WebKitTextChecker_h

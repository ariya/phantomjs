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

#ifndef TextCheckerEnchant_h
#define TextCheckerEnchant_h

#if ENABLE(SPELLCHECK)

#include <enchant.h>
#include <wtf/FastAllocBase.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class TextCheckerEnchant {
    WTF_MAKE_FAST_ALLOCATED;

public:
    static PassOwnPtr<TextCheckerEnchant> create() { return adoptPtr(new TextCheckerEnchant); }
    virtual ~TextCheckerEnchant();

    void ignoreWord(const String&);
    void learnWord(const String&);
    void checkSpellingOfString(const String&, int& misspellingLocation, int& misspellingLength);
    Vector<String> getGuessesForWord(const String&);
    void updateSpellCheckingLanguages(const Vector<String>& languages);
    Vector<String> loadedSpellCheckingLanguages() const;
    bool hasDictionary() const { return !m_enchantDictionaries.isEmpty(); }
    Vector<String> availableSpellCheckingLanguages() const;

private:
    TextCheckerEnchant();
    void freeEnchantBrokerDictionaries();
    void checkSpellingOfWord(const CString&, int start, int end, int& misspellingLocation, int& misspellingLength);

    EnchantBroker* m_broker;
    Vector<EnchantDict*> m_enchantDictionaries;
};

} // namespace WebCore

#endif // ENABLE(SPELLCHECK)

#endif

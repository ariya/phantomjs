/*
 *  Copyright (C) 2012 Igalia S.L.
 *  Copyright (C) 2012 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "TextCheckerEnchant.h"

#if ENABLE(SPELLCHECK)

#include <Language.h>
#include <glib.h>
#include <text/TextBreakIterator.h>

namespace WebCore {

static const size_t maximumNumberOfSuggestions = 10;

static void enchantDictDescribeCallback(const char* const languageTag, const char* const, const char* const, const char* const, void* data)
{
    Vector<CString>* dictionaries = static_cast<Vector<CString>*>(data);
    dictionaries->append(languageTag);
}

TextCheckerEnchant::TextCheckerEnchant()
    : m_broker(enchant_broker_init())
    , m_enchantDictionaries(0)
{
}

TextCheckerEnchant::~TextCheckerEnchant()
{
    if (!m_broker)
        return;

    freeEnchantBrokerDictionaries();
    enchant_broker_free(m_broker);
}

void TextCheckerEnchant::ignoreWord(const String& word)
{
    for (Vector<EnchantDict*>::const_iterator iter = m_enchantDictionaries.begin(); iter != m_enchantDictionaries.end(); ++iter)
        enchant_dict_add_to_session(*iter, word.utf8().data(), -1);
}

void TextCheckerEnchant::learnWord(const String& word)
{
    for (Vector<EnchantDict*>::const_iterator iter = m_enchantDictionaries.begin(); iter != m_enchantDictionaries.end(); ++iter)
        enchant_dict_add(*iter, word.utf8().data(), -1);
}

void TextCheckerEnchant::checkSpellingOfWord(const CString& word, int start, int end, int& misspellingLocation, int& misspellingLength)
{
    const char* string = word.data();
    char* startPtr = g_utf8_offset_to_pointer(string, start);
    int numberOfBytes = static_cast<int>(g_utf8_offset_to_pointer(string, end) - startPtr);

    for (Vector<EnchantDict*>::const_iterator dictIter = m_enchantDictionaries.begin(); dictIter != m_enchantDictionaries.end(); ++dictIter) {
        if (!enchant_dict_check(*dictIter, startPtr, numberOfBytes)) {
            // Stop checking, this word is ok in at least one dict.
            misspellingLocation = -1;
            misspellingLength = 0;
            return;
        }
    }

    misspellingLocation = start;
    misspellingLength = end - start;
}

void TextCheckerEnchant::checkSpellingOfString(const String& string, int& misspellingLocation, int& misspellingLength)
{
    // Assume that the words in the string are spelled correctly.
    misspellingLocation = -1;
    misspellingLength = 0;

    if (!hasDictionary())
        return;

    TextBreakIterator* iter = wordBreakIterator(string.characters(), string.length());
    if (!iter)
        return;

    CString utf8String = string.utf8();
    int start = textBreakFirst(iter);
    for (int end = textBreakNext(iter); end != TextBreakDone; end = textBreakNext(iter)) {
        if (isWordTextBreak(iter)) {
            checkSpellingOfWord(utf8String, start, end, misspellingLocation, misspellingLength);
            // Stop checking the next words If the current word is misspelled, to do not overwrite its misspelled location and length.
            if (misspellingLength)
                return;
        }
        start = end;
    }
}

Vector<String> TextCheckerEnchant::getGuessesForWord(const String& word)
{
    Vector<String> guesses;
    if (!hasDictionary())
        return guesses;

    for (Vector<EnchantDict*>::const_iterator iter = m_enchantDictionaries.begin(); iter != m_enchantDictionaries.end(); ++iter) {
        size_t numberOfSuggestions;
        size_t i;

        char** suggestions = enchant_dict_suggest(*iter, word.utf8().data(), -1, &numberOfSuggestions);
        if (numberOfSuggestions <= 0)
            continue;

        if (numberOfSuggestions > maximumNumberOfSuggestions)
            numberOfSuggestions = maximumNumberOfSuggestions;

        for (i = 0; i < numberOfSuggestions; i++)
            guesses.append(String::fromUTF8(suggestions[i]));

        enchant_dict_free_suggestions(*iter, suggestions);
    }

    return guesses;
}

void TextCheckerEnchant::updateSpellCheckingLanguages(const Vector<String>& languages)
{
    Vector<EnchantDict*> spellDictionaries;

    if (!languages.isEmpty()) {
        for (Vector<String>::const_iterator iter = languages.begin(); iter != languages.end(); ++iter) {
            CString currentLanguage = iter->utf8();
            if (enchant_broker_dict_exists(m_broker, currentLanguage.data())) {
                EnchantDict* dict = enchant_broker_request_dict(m_broker, currentLanguage.data());
                spellDictionaries.append(dict);
            }
        }
    } else {
        // Languages are not specified by user, try to get default language.
        CString utf8Language = defaultLanguage().utf8();
        const char* language = utf8Language.data();
        if (enchant_broker_dict_exists(m_broker, language)) {
            EnchantDict* dict = enchant_broker_request_dict(m_broker, language);
            spellDictionaries.append(dict);
        } else {
            // No dictionaries selected, we get the first one from the list.
            Vector<CString> allDictionaries;
            enchant_broker_list_dicts(m_broker, enchantDictDescribeCallback, &allDictionaries);
            if (!allDictionaries.isEmpty()) {
                EnchantDict* dict = enchant_broker_request_dict(m_broker, allDictionaries.first().data());
                spellDictionaries.append(dict);
            }
        }
    }
    freeEnchantBrokerDictionaries();
    m_enchantDictionaries = spellDictionaries;
}

Vector<String> TextCheckerEnchant::loadedSpellCheckingLanguages() const
{
    Vector<String> languages;
    if (!hasDictionary())
        return languages;

    // Get a Vector<CString> with the list of languages in use.
    Vector<CString> currentDictionaries;
    for (Vector<EnchantDict*>::const_iterator iter = m_enchantDictionaries.begin(); iter != m_enchantDictionaries.end(); ++iter)
        enchant_dict_describe(*iter, enchantDictDescribeCallback, &currentDictionaries);

    for (Vector<CString>::const_iterator iter = currentDictionaries.begin(); iter != currentDictionaries.end(); ++iter)
        languages.append(String::fromUTF8(iter->data()));

    return languages;
}

Vector<String> TextCheckerEnchant::availableSpellCheckingLanguages() const
{
    Vector<CString> allDictionaries;
    enchant_broker_list_dicts(m_broker, enchantDictDescribeCallback, &allDictionaries);

    Vector<String> languages;
    for (Vector<CString>::const_iterator iter = allDictionaries.begin(); iter != allDictionaries.end(); ++iter)
        languages.append(String::fromUTF8(iter->data()));

    return languages;
}

void TextCheckerEnchant::freeEnchantBrokerDictionaries()
{
    for (Vector<EnchantDict*>::const_iterator iter = m_enchantDictionaries.begin(); iter != m_enchantDictionaries.end(); ++iter)
        enchant_broker_free_dict(m_broker, *iter);
}

} // namespace WebCore

#endif // ENABLE(SPELLCHECK)


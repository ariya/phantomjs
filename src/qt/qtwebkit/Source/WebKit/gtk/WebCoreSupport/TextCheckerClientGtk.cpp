/*
 *  Copyright (C) 2007 Alp Toker <alp@atoker.com>
 *  Copyright (C) 2008 Nuanti Ltd.
 *  Copyright (C) 2009 Diego Escalante Urrelo <diegoe@gnome.org>
 *  Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 *  Copyright (C) 2009, 2010, 2011 Igalia S.L.
 *  Copyright (C) 2010, Martin Robinson <mrobinson@webkit.org>
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
#include "TextCheckerClientGtk.h"

#include "NotImplemented.h"
#include "webkitspellchecker.h"
#include "webkitwebsettingsprivate.h"
#include <glib.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

TextCheckerClientGtk::TextCheckerClientGtk(WebKitSpellChecker* spellChecker)
    : m_spellChecker(spellChecker)
{
}

TextCheckerClientGtk::~TextCheckerClientGtk()
{
}

bool TextCheckerClientGtk::shouldEraseMarkersAfterChangeSelection(TextCheckingType) const
{
    return true;
}

void TextCheckerClientGtk::ignoreWordInSpellDocument(const String& text)
{
    webkit_spell_checker_ignore_word(m_spellChecker.get(), text.utf8().data());
}

void TextCheckerClientGtk::learnWord(const String& text)
{
    webkit_spell_checker_learn_word(m_spellChecker.get(), text.utf8().data());
}

void TextCheckerClientGtk::checkSpellingOfString(const UChar* text, int length, int* misspellingLocation, int* misspellingLength)
{
    String textAsString(text, length);
    webkit_spell_checker_check_spelling_of_string(m_spellChecker.get(), textAsString.utf8().data(), misspellingLocation, misspellingLength);
}

String TextCheckerClientGtk::getAutoCorrectSuggestionForMisspelledWord(const String& inputWord)
{
    return webkit_spell_checker_get_autocorrect_suggestions_for_misspelled_word(m_spellChecker.get(), inputWord.utf8().data());
}

void TextCheckerClientGtk::checkGrammarOfString(const UChar*, int, Vector<GrammarDetail>&, int*, int*)
{
    notImplemented();
}

void TextCheckerClientGtk::getGuessesForWord(const String& word, const String& context, WTF::Vector<String>& guesses)
{
    char** suggestions = webkit_spell_checker_get_guesses_for_word(m_spellChecker.get(), word.utf8().data(), context.utf8().data());
    if (!suggestions)
        return;

    guesses.clear();

    for (int i = 0; suggestions[i]; i++)
        guesses.append(String::fromUTF8(suggestions[i]));

    g_strfreev(suggestions);
}

void TextCheckerClientGtk::updateSpellCheckingLanguage(const char* spellCheckingLanguages)
{
    webkit_spell_checker_update_spell_checking_languages(m_spellChecker.get(), spellCheckingLanguages);
}
}

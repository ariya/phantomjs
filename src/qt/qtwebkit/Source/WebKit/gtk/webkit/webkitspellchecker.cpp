/*
 *  Copyright (C) 2011 Igalia S.L.
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
#include "webkitspellchecker.h"

/**
 * SECTION:webkitspellchecker
 * @short_description: the spell checking API for WebKit
 *
 * #WebKitSpellChecker provides APIs for the spell checking
 * functionality used internally by WebKit to perform spell checking
 * in editable areas. This can be used, for example, by browsers to
 * implement custom spell checking context menus or sophisticated
 * auto-correct features.
 */

G_DEFINE_INTERFACE(WebKitSpellChecker, webkit_spell_checker, G_TYPE_OBJECT);

static void webkit_spell_checker_default_init(WebKitSpellCheckerInterface* interface)
{
}

/**
 * webkit_spell_checker_check_spelling_of_string:
 * @checker: a #WebKitSpellChecker
 * @string: the string to check for misspellings
 * @misspelling_location: (out) (allow-none) a pointer to an integer to store the location of the first misspelling
 * @misspelling_length: (out) (allow-none) a pointer to an integer to store the length of the first misspelling
 *
 * Checks @string for misspellings using @checker, storing the
 * location and length of the first misspelling in
 * @misspelling_location and @misspelling_length respectively.
 *
 * Since: 1.5.1
 **/
void webkit_spell_checker_check_spelling_of_string(WebKitSpellChecker* checker, const char* string, int* misspelling_location, int* misspelling_length)
{
    g_return_if_fail(WEBKIT_IS_SPELL_CHECKER(checker));
    g_return_if_fail(string);

    WebKitSpellCheckerInterface* interface = WEBKIT_SPELL_CHECKER_GET_IFACE(checker);
    if (interface->check_spelling_of_string)
        interface->check_spelling_of_string(checker, string, misspelling_location, misspelling_length);
}

/**
 * webkit_spell_checker_get_guesses_for_word:
 * @checker: a #WebKitSpellChecker
 * @word: the misspelled word
 * @context: (allow-none) the surrounding context of the misspelled word
 *
 * Returns a %NULL-terminated array of guesses for corrections of the
 * misspelled word @word.
 *
 * Returns: (transfer full): a newly allocated %NULL-terminated array
 * of suggested corrections for a misspelled word @word. Free it with
 * %g_strfreev when done with it.
 *
 * Since: 1.5.1
 **/
char** webkit_spell_checker_get_guesses_for_word(WebKitSpellChecker* checker, const char* word, const char* context)
{
    g_return_val_if_fail(WEBKIT_IS_SPELL_CHECKER(checker), 0);
    g_return_val_if_fail(word, 0);

    WebKitSpellCheckerInterface* interface = WEBKIT_SPELL_CHECKER_GET_IFACE(checker);
    if (interface->get_guesses_for_word)
        return interface->get_guesses_for_word(checker, word, context);

    return 0;
}

/**
 * webkit_spell_checker_update_spell_checking_languages:
 * @checker: a #WebKitSpellChecker
 * @languages: (allow-none) a string of languages to use for @checker
 *
 * Sets @languages as the list of languages to use by @checker. The
 * accepted format is a list of comma (',') separated language codes
 * of the form 'en_US', ie, language_VARIANT.
 *
 * Since: 1.5.1
 **/
void webkit_spell_checker_update_spell_checking_languages(WebKitSpellChecker* checker, const char* languages)
{
    g_return_if_fail(WEBKIT_IS_SPELL_CHECKER(checker));

    WebKitSpellCheckerInterface* interface = WEBKIT_SPELL_CHECKER_GET_IFACE(checker);
    if (interface->update_spell_checking_languages)
        interface->update_spell_checking_languages(checker, languages);
}

/**
 * webkit_spell_checker_get_autocorrect_suggestions_for_misspelled_word:
 * @checker: a #WebKitSpellChecker
 * @word: a misspelled word
 *
 * Returns a suggestion for a word to use in an "autocorrect" feature.
 *
 * Returns: (transfer full) the suggestion for the autocorrection of
 * @word
 *
 * Since: 1.5.1
 **/
char* webkit_spell_checker_get_autocorrect_suggestions_for_misspelled_word(WebKitSpellChecker* checker, const char* word)
{
    g_return_val_if_fail(WEBKIT_IS_SPELL_CHECKER(checker), 0);
    g_return_val_if_fail(word, 0);

    WebKitSpellCheckerInterface* interface = WEBKIT_SPELL_CHECKER_GET_IFACE(checker);
    if (interface->get_autocorrect_suggestions_for_misspelled_word)
        return interface->get_autocorrect_suggestions_for_misspelled_word(checker, word);

    return 0;
}

/**
 * webkit_spell_checker_learn_word:
 * @checker: a #WebKitSpellChecker
 * @word: the word to learn
 *
 * Instructs the @checker to add @word to its dictionary as a properly
 * spelled word. The word will be learned permanently in the user's
 * personal dictionary.
 *
 * Since: 1.5.1
 **/
void webkit_spell_checker_learn_word(WebKitSpellChecker* checker, const char* word)
{
    g_return_if_fail(WEBKIT_IS_SPELL_CHECKER(checker));
    g_return_if_fail(word);

    WebKitSpellCheckerInterface* interface = WEBKIT_SPELL_CHECKER_GET_IFACE(checker);
    if (interface->learn_word)
        interface->learn_word(checker, word);
}

/**
 * webkit_spell_checker_ignore_word:
 * @checker: a #WebKitSpellChecker
 * @word: the word to ignore
 *
 * Instructs the @checker to ignore @word as a misspelling for this
 * session.
 *
 * Since: 1.5.1
 **/
void webkit_spell_checker_ignore_word(WebKitSpellChecker* checker, const char* word)
{
    g_return_if_fail(WEBKIT_IS_SPELL_CHECKER(checker));
    g_return_if_fail(word);

    WebKitSpellCheckerInterface* interface = WEBKIT_SPELL_CHECKER_GET_IFACE(checker);
    if (interface->ignore_word)
        interface->ignore_word(checker, word);
}

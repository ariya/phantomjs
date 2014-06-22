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
#include "webkitspellcheckerenchant.h"

#if ENABLE(SPELLCHECK)

#include "TextCheckerEnchant.h"
#include "webkitspellchecker.h"
#include <gtk/gtk.h>
#include <wtf/OwnPtr.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

using namespace WebCore;

/**
 * SECTION:webkitspellcheckerenchant
 * @short_description: the default spell checking implementation for WebKitGTK+.
 *
 * #WebKitSpellCheckerEnchant is the default spell checking implementation for
 * WebKitGTK+. It uses the Enchant dictionaries installed on the system to
 * correct spelling.
 */

struct _WebKitSpellCheckerEnchantPrivate {
    OwnPtr<TextCheckerEnchant> textCheckerEnchant;
};

static void webkit_spell_checker_enchant_spell_checker_interface_init(WebKitSpellCheckerInterface* checkerInterface);

G_DEFINE_TYPE_WITH_CODE(WebKitSpellCheckerEnchant, webkit_spell_checker_enchant, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(WEBKIT_TYPE_SPELL_CHECKER,
                                              webkit_spell_checker_enchant_spell_checker_interface_init))

static void webkit_spell_checker_enchant_finalize(GObject* object)
{
    WebKitSpellCheckerEnchantPrivate* priv = WEBKIT_SPELL_CHECKER_ENCHANT(object)->priv;
    priv->~WebKitSpellCheckerEnchantPrivate();
}

static void webkit_spell_checker_enchant_class_init(WebKitSpellCheckerEnchantClass* klass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(klass);
    objectClass->finalize = webkit_spell_checker_enchant_finalize;
    g_type_class_add_private(klass, sizeof(WebKitSpellCheckerEnchantPrivate));
}

static void webkit_spell_checker_enchant_init(WebKitSpellCheckerEnchant* checker)
{
    WebKitSpellCheckerEnchantPrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(checker, WEBKIT_TYPE_SPELL_CHECKER_ENCHANT, WebKitSpellCheckerEnchantPrivate);
    checker->priv = priv;
    new (priv) WebKitSpellCheckerEnchantPrivate();

    priv->textCheckerEnchant = TextCheckerEnchant::create();
}

static void checkSpellingOfString(WebKitSpellChecker* checker, const char* string, int* misspellingLocation, int* misspellingLength)
{
    WebKitSpellCheckerEnchantPrivate* priv = WEBKIT_SPELL_CHECKER_ENCHANT(checker)->priv;
    priv->textCheckerEnchant->checkSpellingOfString(String::fromUTF8(string), *misspellingLocation, *misspellingLength);
}

static char** getGuessesForWord(WebKitSpellChecker* checker, const char* word, const char* context)
{
    WebKitSpellCheckerEnchantPrivate* priv = WEBKIT_SPELL_CHECKER_ENCHANT(checker)->priv;

    Vector<String> guesses = priv->textCheckerEnchant->getGuessesForWord(String::fromUTF8(word));

    if (guesses.isEmpty())
        return 0;

    int i = 0;
    int numberOfGuesses = guesses.size();
    char** guessesArray = static_cast<char**>(g_malloc0((numberOfGuesses + 1) * sizeof(char*)));
    for (Vector<String>::const_iterator iter = guesses.begin(); iter != guesses.end(); ++iter)
        guessesArray[i++] = g_strdup(iter->utf8().data());
    guessesArray[i] = 0;

    return guessesArray;
}

static void updateSpellCheckingLanguages(WebKitSpellChecker* checker, const char* languages)
{
    WebKitSpellCheckerEnchantPrivate* priv = WEBKIT_SPELL_CHECKER_ENCHANT(checker)->priv;

    Vector<String> languagesVector;
    String::fromUTF8(languages).split(static_cast<UChar>(','), languagesVector);
    priv->textCheckerEnchant->updateSpellCheckingLanguages(languagesVector);
}

static char* getAutocorrectSuggestionsForMisspelledWord(WebKitSpellChecker* checker, const char* word)
{
    return 0;
}

static void learnWord(WebKitSpellChecker* checker, const char* word)
{
    WebKitSpellCheckerEnchantPrivate* priv = WEBKIT_SPELL_CHECKER_ENCHANT(checker)->priv;
    priv->textCheckerEnchant->learnWord(String::fromUTF8(word));
}

static void ignoreWord(WebKitSpellChecker* checker, const char* word)
{
    WebKitSpellCheckerEnchantPrivate* priv = WEBKIT_SPELL_CHECKER_ENCHANT(checker)->priv;
    priv->textCheckerEnchant->ignoreWord(String::fromUTF8(word));
}

static void webkit_spell_checker_enchant_spell_checker_interface_init(WebKitSpellCheckerInterface* checkerInterface)
{
    checkerInterface->check_spelling_of_string = checkSpellingOfString;
    checkerInterface->get_guesses_for_word = getGuessesForWord;
    checkerInterface->update_spell_checking_languages = updateSpellCheckingLanguages;
    checkerInterface->get_autocorrect_suggestions_for_misspelled_word = getAutocorrectSuggestionsForMisspelledWord;
    checkerInterface->learn_word = learnWord;
    checkerInterface->ignore_word = ignoreWord;
}

#endif /* ENABLE(SPELLCHECK) */


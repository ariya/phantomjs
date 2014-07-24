/*
 * Copyright (C) 2011 Igalia S.L.
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

#ifndef webkitspellchecker_h
#define webkitspellchecker_h

#include "webkitdefines.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_SPELL_CHECKER           (webkit_spell_checker_get_type())
#define WEBKIT_SPELL_CHECKER(obj)           (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_SPELL_CHECKER, WebKitSpellChecker))
#define WEBKIT_IS_SPELL_CHECKER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WEBKIT_TYPE_SPELL_CHECKER))
#define WEBKIT_SPELL_CHECKER_GET_IFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), WEBKIT_TYPE_SPELL_CHECKER, WebKitSpellCheckerInterface))

struct _WebKitSpellCheckerInterface {
    GTypeInterface g_iface;

    void   (*check_spelling_of_string)                        (WebKitSpellChecker* checker, const char* string, int* misspelling_location, int* misspelling_length);
    char** (*get_guesses_for_word)                            (WebKitSpellChecker* checker, const char* word, const char* context);
    void   (*update_spell_checking_languages)                 (WebKitSpellChecker* checker, const char* languages);
    char*  (*get_autocorrect_suggestions_for_misspelled_word) (WebKitSpellChecker* checker, const char* word);
    void   (*learn_word)                                      (WebKitSpellChecker* checker, const char* word);
    void   (*ignore_word)                                     (WebKitSpellChecker* checker, const char* word);
};

WEBKIT_API GType   webkit_spell_checker_get_type                                        (void) G_GNUC_CONST;

WEBKIT_API void    webkit_spell_checker_check_spelling_of_string                        (WebKitSpellChecker *checker,
                                                                                         const char         *string,
                                                                                         int                *misspelling_location,
                                                                                         int                *misspelling_length);

WEBKIT_API char**  webkit_spell_checker_get_guesses_for_word                            (WebKitSpellChecker *checker,
                                                                                         const char         *word,
                                                                                         const char         *context);

WEBKIT_API void    webkit_spell_checker_update_spell_checking_languages                 (WebKitSpellChecker *checker,
                                                                                         const char         *languages);

WEBKIT_API char*   webkit_spell_checker_get_autocorrect_suggestions_for_misspelled_word (WebKitSpellChecker *checker,
                                                                                         const char         *word);

WEBKIT_API void    webkit_spell_checker_learn_word                                      (WebKitSpellChecker *checker,
                                                                                         const char         *word);

WEBKIT_API void    webkit_spell_checker_ignore_word                                     (WebKitSpellChecker *checker,
                                                                                         const char         *word);

G_END_DECLS

#endif


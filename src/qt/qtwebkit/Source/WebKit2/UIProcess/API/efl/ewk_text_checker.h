/*
 * Copyright (C) 2012-2013 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file ewk_text_checker.h
 * @brief Provides API to overwrite the default WebKit spellchecker implementation
 *        and contains API to manipulate spellchecker settings.
 *
 * There is one spellchecker object per application.
 * It allows to check spelling in the editable areas, get suggestions for the misspelled word,
 * learn and ignore spelling.
 *
 * If application wants to check spelling while typing, ewk_text_checker_continuous_spell_checking_enabled_set API
 * should be used.
 *
 * The default WebKit spellchecker implementation is based on the Enchant library.
 * It doesn't ensure grammar checking. Application is able to overwrite the default
 * WebKit spellchecker implementation by defining its own implementation and setting
 * appropriate callback functions.
 */

#ifndef ewk_text_checker_h
#define ewk_text_checker_h

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a type name for the callback function used to notify the client when
 * the continuous spell checking setting was changed by WebKit.
 *
 * @param enable @c EINA_TRUE if continuous spell checking is enabled or @c EINA_FALSE if it's disabled
 */
typedef void (*Ewk_Text_Checker_Continuous_Spell_Checking_Change_Cb)(Eina_Bool enable);

/**
 * Defines a type name for the callback function to return a tag (identifier) which is guaranteed to be unique.
 *
 * Unique tags help to avoid collisions with other objects that are checked for spelling mistakes.
 *
 * @param o the view object to get unique tag
 *
 * @return unique tag for the given @a o view object
 */
typedef uint64_t (*Ewk_Text_Checker_Unique_Spell_Document_Tag_Get_Cb)(const Evas_Object *o);

/**
 * Defines a type name for the callback function to close the prviously set tag.
 *
 * This callback will notify the receiver that the user has finished with the tagged document.
 *
 * @param tag the tag to be closed
 */
typedef void (*Ewk_Text_Checker_Unique_Spell_Document_Tag_Close_Cb)(uint64_t tag);

/**
 * Defines a type name for the callback function to search for a misspelled words in the given string.
 *
 * @param tag unique tag to notify the spell checker which document that @a text is associated,
 *        in most cases not necessarily, just for ignored word,
 *        @c 0 can be passed in for text not associated with a particular document
 * @param text the text containing the words to spellcheck
 * @param misspelling_location a pointer to store the beginning of the misspelled @a text, @c -1 if the @a text is correct
 * @param misspelling_length a pointer to store the length of misspelled @a text, @c 0 if the @a text is correct
 */
typedef void (*Ewk_Text_Checker_String_Spelling_Check_Cb)(uint64_t tag, const char *text, int32_t *misspelling_location, int32_t *misspelling_length);

/**
 * Defines a type name for the callback function to get a list of suggested spellings for a misspelled @a word.
 *
 * @param tag unique tag to notify the spell checker which document that @a text is associated,
 *        @c 0 can be passed for text not associated with a particular document
 * @param word the word to get guesses
 * @return a list of dynamically allocated strings (as char*),
 *         the list and its items will be freed by WebKit.
 */
typedef Eina_List *(*Ewk_Text_Checker_Word_Guesses_Get_Cb)(uint64_t tag, const char *word);

/**
 * Sets a callback function to add the word to the spell checker dictionary.
 *
 * @param tag unique tag to notify the spell checker which document that @a text is associated,
 *        @c 0 can be passed for text not associated with a particular document
 * @param word the word to add
 */
typedef void (*Ewk_Text_Checker_Word_Learn_Cb)(uint64_t tag, const char *word);

/**
 * Sets a callback function to tell the spell checker to ignore a given word.
 *
 * @param tag unique tag to notify the spell checker which document that @a text is associated,
 *        @c 0 can be passed for text not associated with a particular document
 * @param word the word to ignore
 */
typedef void (*Ewk_Text_Checker_Word_Ignore_Cb)(uint64_t tag, const char *word);


/**
 * Queries if continuous spell checking is enabled.
 *
 * @return @c EINA_TRUE if continuous spell checking is enabled or @c EINA_FALSE if it's disabled
 */
EAPI Eina_Bool ewk_text_checker_continuous_spell_checking_enabled_get(void);

/**
 * Enables/disables continuous spell checking.
 *
 * This feature is disabled by default.
 *
 * @see ewk_text_checker_continuous_spell_checking_change_cb_set
 *
 * @param enable @c EINA_TRUE to enable continuous spell checking or @c EINA_FALSE to disable
 */
EAPI void ewk_text_checker_continuous_spell_checking_enabled_set(Eina_Bool enable);

/**
 * Gets the the list of all available the spell checking languages to use.
 *
 * @see ewk_settings_spell_checking_languages_set
 *
 * @return the list with available spell checking languages, or @c NULL on failure
 *         the Eina_List and its items should be freed after, use eina_stringshare_del()
 */
EAPI Eina_List *ewk_text_checker_spell_checking_available_languages_get(void);

/**
 * Sets @a languages as the list of languages to use by default WebKit
 * implementation of spellchecker feature with Enchant library support.
 *
 * If @languages is @c NULL, the default language is used.
 * If the default language can not be determined then any available dictionary will be used.
 *
 * @note This function invalidates the previously set languages.
 *       The dictionaries are requested asynchronously.
 *
 * @param languages a list of comma (',') separated language codes
 *        of the form 'en_US', ie, language_VARIANT, may be @c NULL.
 */
EAPI void ewk_text_checker_spell_checking_languages_set(const char *languages);

/**
 * Gets the the list of the spell checking languages in use.
 *
 * @see ewk_settings_spell_checking_available_languages_get
 * @see ewk_settings_spell_checking_languages_set
 *
 * @return the list with the spell checking languages in use,
 *         the Eina_List and its items should be freed after, use eina_stringshare_del()
 */
EAPI Eina_List *ewk_text_checker_spell_checking_languages_get(void);

/**
 * Sets a callback function used to notify the client when
 * the continuous spell checking setting was changed by WebKit.
 *
 * Specifying of this callback is needed if the application wants to receive notifications
 * once WebKit changes this setting.
 * If the application is not interested, this callback is not set.
 * Changing of this setting at the WebKit level can be made as a result of modifying
 * options in a Context Menu by a user.
 *
 * @param cb a new callback function to set or @c NULL to invalidate the previous one
 */
EAPI void ewk_text_checker_continuous_spell_checking_change_cb_set(Ewk_Text_Checker_Continuous_Spell_Checking_Change_Cb cb);

/**
 * Sets a callback function to get a unique spell document tag.
 *
 * @param cb a new callback to set or @c NULL to restore the default WebKit callback implementation
 */
EAPI void ewk_text_checker_unique_spell_document_tag_get_cb_set(Ewk_Text_Checker_Unique_Spell_Document_Tag_Get_Cb cb);

/**
 * Sets a callback function to close a unique spell document tag.
 *
 * @param cb a new callback to set or @c NULL to restore the default WebKit callback implementation
 */
EAPI void ewk_text_checker_unique_spell_document_tag_close_cb_set(Ewk_Text_Checker_Unique_Spell_Document_Tag_Close_Cb cb);

/**
 * Sets a callback function to search for a misspelled words in the given string.
 *
 * @param cb a new callback to set or @c NULL to restore the default WebKit callback implementation
 */
EAPI void ewk_text_checker_string_spelling_check_cb_set(Ewk_Text_Checker_String_Spelling_Check_Cb cb);

/**
 * Sets a callback function to get an array of suggested spellings for a misspelled word.
 *
 * @param cb a new callback to set or @c NULL to restore the default WebKit callback implementation
 */
EAPI void ewk_text_checker_word_guesses_get_cb_set(Ewk_Text_Checker_Word_Guesses_Get_Cb cb);

/**
 * Sets a callback function to add the word to the spell checker dictionary.
 *
 * @param cb a new callback to set or @c NULL to restore the default WebKit callback implementation
 */
EAPI void ewk_text_checker_word_learn_cb_set(Ewk_Text_Checker_Word_Learn_Cb cb);

/**
 * Sets a callback function to tell the spell checker to ignore a given word.
 *
 * @param cb a new callback to set or @c NULL to restore the default WebKit callback implementation
 */
EAPI void ewk_text_checker_word_ignore_cb_set(Ewk_Text_Checker_Word_Ignore_Cb cb);

#ifdef __cplusplus
}
#endif
#endif // ewk_text_checker_h

/*
 * Copyright (C) 2012-2013 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation
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

#include "config.h"
#include "ewk_text_checker.h"

#if ENABLE(SPELLCHECK)
#include "TextCheckerClientEfl.h"

using namespace WebKit;

static Eina_List* convertLanguagesToEinaList(const Vector<String>& languages)
{
    Eina_List* listOflanguages = 0;
    size_t numberOfLanguages = languages.size();

    for (size_t i = 0; i < numberOfLanguages; ++i)
        listOflanguages = eina_list_append(listOflanguages, eina_stringshare_add(languages[i].utf8().data()));

    return listOflanguages;
}

#define EWK_TEXT_CHECKER_CALLBACK_SET(TYPE_NAME, NAME)            \
void ewk_text_checker_##NAME##_cb_set(TYPE_NAME cb)               \
{                                                                 \
    TextCheckerClientEfl::instance().clientCallbacks().NAME = cb; \
}

#else

// Defines an empty API to do not break build.
#define EWK_TEXT_CHECKER_CALLBACK_SET(TYPE_NAME, NAME)  \
void ewk_text_checker_##NAME##_cb_set(TYPE_NAME)        \
{                                                       \
}
#endif // ENABLE(SPELLCHECK)

Eina_Bool ewk_text_checker_continuous_spell_checking_enabled_get()
{
#if ENABLE(SPELLCHECK)
    return TextCheckerClientEfl::instance().isContinuousSpellCheckingEnabled();
#else
    return false;
#endif
}

void ewk_text_checker_continuous_spell_checking_enabled_set(Eina_Bool enable)
{
#if ENABLE(SPELLCHECK)
    WKTextCheckerContinuousSpellCheckingEnabledStateChanged(!!enable);
#else
    UNUSED_PARAM(enable);
#endif
}

Eina_List* ewk_text_checker_spell_checking_available_languages_get()
{
    Eina_List* listOflanguages = 0;
#if ENABLE(SPELLCHECK)
    // FIXME: Expose WK2 C API to get available spell checking languages.
    listOflanguages = convertLanguagesToEinaList(TextCheckerClientEfl::instance().availableSpellCheckingLanguages());
#endif
    return listOflanguages;
}

void ewk_text_checker_spell_checking_languages_set(const char* languages)
{
#if ENABLE(SPELLCHECK)
    Vector<String> newLanguages;
    String::fromUTF8(languages).split(',', newLanguages);

    // FIXME: Expose WK2 C API to set spell checking languages.
    TextCheckerClientEfl::instance().updateSpellCheckingLanguages(newLanguages);
#else
    UNUSED_PARAM(languages);
#endif
}

Eina_List* ewk_text_checker_spell_checking_languages_get()
{
    Eina_List* listOflanguages = 0;
#if ENABLE(SPELLCHECK)
    // FIXME: Expose WK2 C API to get loaded spell checking languages.
    listOflanguages = convertLanguagesToEinaList(TextCheckerClientEfl::instance().loadedSpellCheckingLanguages());
#endif
    return listOflanguages;
}

EWK_TEXT_CHECKER_CALLBACK_SET(Ewk_Text_Checker_Continuous_Spell_Checking_Change_Cb, continuous_spell_checking_change)
EWK_TEXT_CHECKER_CALLBACK_SET(Ewk_Text_Checker_Unique_Spell_Document_Tag_Get_Cb, unique_spell_document_tag_get)
EWK_TEXT_CHECKER_CALLBACK_SET(Ewk_Text_Checker_Unique_Spell_Document_Tag_Close_Cb, unique_spell_document_tag_close)
EWK_TEXT_CHECKER_CALLBACK_SET(Ewk_Text_Checker_String_Spelling_Check_Cb, string_spelling_check)
EWK_TEXT_CHECKER_CALLBACK_SET(Ewk_Text_Checker_Word_Guesses_Get_Cb, word_guesses_get)
EWK_TEXT_CHECKER_CALLBACK_SET(Ewk_Text_Checker_Word_Learn_Cb, word_learn)
EWK_TEXT_CHECKER_CALLBACK_SET(Ewk_Text_Checker_Word_Ignore_Cb, word_ignore)

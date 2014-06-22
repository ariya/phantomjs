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

#ifndef ewk_text_checker_private_h
#define ewk_text_checker_private_h

#if ENABLE(SPELLCHECK)

#include "ewk_text_checker.h"

/**
 * @brief Structure to store client callback functions.
 *
 * @internal
 */
struct ClientCallbacks {
    Ewk_Text_Checker_Continuous_Spell_Checking_Change_Cb continuous_spell_checking_change;
    Ewk_Text_Checker_Unique_Spell_Document_Tag_Get_Cb unique_spell_document_tag_get;
    Ewk_Text_Checker_Unique_Spell_Document_Tag_Close_Cb unique_spell_document_tag_close;
    Ewk_Text_Checker_String_Spelling_Check_Cb string_spelling_check;
    Ewk_Text_Checker_Word_Guesses_Get_Cb word_guesses_get;
    Ewk_Text_Checker_Word_Learn_Cb word_learn;
    Ewk_Text_Checker_Word_Ignore_Cb word_ignore;
};

#endif // ENABLE(SPELLCHECK)
#endif // ewk_text_checker_private_h

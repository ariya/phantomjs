/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#include <harfbuzz-external.h>
#include <Qt/private/qunicodetables_p.h>
#include <QLibrary>
#include <QTextCodec>

extern "C" {

HB_LineBreakClass HB_GetLineBreakClass(HB_UChar32 ch)
{
#if QT_VERSION >= 0x040300
    return (HB_LineBreakClass)QUnicodeTables::lineBreakClass(ch);
#else
#error "This test currently requires Qt >= 4.3"
#endif
}

void HB_GetUnicodeCharProperties(HB_UChar32 ch, HB_CharCategory *category, int *combiningClass)
{
    *category = (HB_CharCategory)QChar::category(ch);
    *combiningClass = QChar::combiningClass(ch);
}

HB_CharCategory HB_GetUnicodeCharCategory(HB_UChar32 ch)
{
    return (HB_CharCategory)QChar::category(ch);
}

int HB_GetUnicodeCharCombiningClass(HB_UChar32 ch)
{
    return QChar::combiningClass(ch);
}

HB_UChar16 HB_GetMirroredChar(HB_UChar16 ch)
{
    return QChar::mirroredChar(ch);
}

HB_WordClass HB_GetWordClass(HB_UChar32 ch)
{
    const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ch);
    return (HB_WordClass) prop->wordBreak;
}


HB_SentenceClass HB_GetSentenceClass(HB_UChar32 ch)
{
    const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ch);
    return (HB_SentenceClass) prop->sentenceBreak;
}

void HB_GetGraphemeAndLineBreakClass(HB_UChar32 ch, HB_GraphemeClass *grapheme, HB_LineBreakClass *lineBreak)
{
    const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ch);
    *grapheme = (HB_GraphemeClass) prop->graphemeBreak;
    *lineBreak = (HB_LineBreakClass) prop->line_break_class;
}

void *HB_Library_Resolve(const char *library, int version, const char *symbol)
{
    return QLibrary::resolve(library, version, symbol);
}

}

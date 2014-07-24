/*
 * Copyright (C) 2011 Lindsay Mathieson <lindsay.mathieson@gmail.com>
 * Copyright (C) 2011 Dawit Alemayehu  <adawit@kde.org>
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TextCheckerClientQt.h"

#include "NotImplemented.h"
#include "QtPlatformPlugin.h"

#include <QStringList>
#include <QVector>
#include <wtf/text/WTFString.h>


static void convertToVectorList(const QStringList& list, Vector<String>& vList)
{
    const int count = list.count();
    vList.resize(count);
    for (int i = 0; i < count; ++i)
        vList.append(list.at(i));
}

namespace WebCore {

bool TextCheckerClientQt::shouldEraseMarkersAfterChangeSelection(TextCheckingType) const
{
    return true;
}

void TextCheckerClientQt::ignoreWordInSpellDocument(const String& word)
{
    if (!loadSpellChecker())
        return;

    m_spellChecker->ignoreWordInSpellDocument(word);
}

void TextCheckerClientQt::learnWord(const String& word)
{
    if (!loadSpellChecker())
        return;

    m_spellChecker->learnWord(word);
}

String TextCheckerClientQt::getAutoCorrectSuggestionForMisspelledWord(const String& misspelledWord)
{
    if (!loadSpellChecker())
        return String();

    return m_spellChecker->autoCorrectSuggestionForMisspelledWord(misspelledWord);
}

void TextCheckerClientQt::checkSpellingOfString(const UChar* buffer, int length, int* misspellingLocation, int* misspellingLength)
{
    if (!loadSpellChecker())
        return;

    const QString text = QString::fromRawData(reinterpret_cast<const QChar*>(buffer), length);
    m_spellChecker->checkSpellingOfString(text, misspellingLocation, misspellingLength);
}

void TextCheckerClientQt::checkGrammarOfString(const UChar* buffer, int length, Vector<GrammarDetail>& details, int* badGrammarLocation, int* badGrammarLength)
{
    if (!loadSpellChecker())
        return;

    const QString text = QString::fromRawData(reinterpret_cast<const QChar*>(buffer), length);

    // Do Grammer check.
    QList<QWebSpellChecker::GrammarDetail> qGrammarDetails;
    m_spellChecker->checkGrammarOfString(text, qGrammarDetails, badGrammarLocation, badGrammarLength);

    // Copy the grammar detail from the Qt plugin to the webkit structure.
    const int count = qGrammarDetails.count();
    for (int i = 0; i < count; ++i) {
        const QWebSpellChecker::GrammarDetail qGrammarDetail = qGrammarDetails.at(i);
        GrammarDetail webkitGrammarDetail;
        webkitGrammarDetail.location = qGrammarDetail.location;
        webkitGrammarDetail.length = qGrammarDetail.length;
        // Copy guesses strings.
        convertToVectorList(qGrammarDetail.guesses, webkitGrammarDetail.guesses);
        webkitGrammarDetail.userDescription = qGrammarDetail.userDescription;
        details.append(webkitGrammarDetail);
    }
}

void TextCheckerClientQt::getGuessesForWord(const String& word, const String& context, Vector<String>& guesses)
{
    if (!loadSpellChecker())
        return;

    QStringList guessesList;
    m_spellChecker->guessesForWord(word, context, guessesList);
    convertToVectorList(guessesList, guesses);
}

bool TextCheckerClientQt::isContinousSpellCheckingEnabled()
{
    if (!loadSpellChecker())
        return false;

    return m_spellChecker->isContinousSpellCheckingEnabled();
}

void TextCheckerClientQt::toggleContinousSpellChecking()
{
    if (!loadSpellChecker())
        return;

    m_spellChecker->toggleContinousSpellChecking();
}

bool TextCheckerClientQt::isGrammarCheckingEnabled()
{
    if (!loadSpellChecker())
        return false;

    return m_spellChecker->isGrammarCheckingEnabled();
}

void TextCheckerClientQt::toggleGrammarChecking()
{
    if (!loadSpellChecker())
        return;

    m_spellChecker->toggleGrammarChecking();
}

bool TextCheckerClientQt::loadSpellChecker()
{
    if (m_spellChecker)
        return true;

    if ((m_spellChecker = m_platformPlugin.createSpellChecker()))
        return true;

    return false;
}

}

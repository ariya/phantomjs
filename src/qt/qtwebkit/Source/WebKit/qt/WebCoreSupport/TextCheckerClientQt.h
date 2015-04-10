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

#ifndef TextCheckerClientQt_h
#define TextCheckerClientQt_h

#include "QtPlatformPlugin.h"
#include "TextCheckerClient.h"
#include "qwebkitplatformplugin.h"

#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>


namespace WebCore {

class TextCheckerClientQt : public TextCheckerClient {
public:
    virtual bool shouldEraseMarkersAfterChangeSelection(TextCheckingType) const;
    virtual void ignoreWordInSpellDocument(const String&);
    virtual void learnWord(const String&);
    virtual void checkSpellingOfString(const UChar*, int length, int* misspellingLocation, int* misspellingLength);
    virtual String getAutoCorrectSuggestionForMisspelledWord(const String& misspelledWord);
    virtual void checkGrammarOfString(const UChar*, int length, Vector<GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength);
    virtual void getGuessesForWord(const String& word, const String& context, Vector<String>& guesses);
    virtual void requestCheckingOfString(WTF::PassRefPtr<WebCore::TextCheckingRequest>) { }

    virtual bool isContinousSpellCheckingEnabled();
    virtual void toggleContinousSpellChecking();

    virtual bool isGrammarCheckingEnabled();
    virtual void toggleGrammarChecking();

private:
    bool loadSpellChecker();

private:
    QtPlatformPlugin m_platformPlugin;
    OwnPtr<QWebSpellChecker> m_spellChecker;
};

}

#endif

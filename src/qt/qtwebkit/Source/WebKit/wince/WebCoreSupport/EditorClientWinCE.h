/*
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef EditorClientWinCE_h
#define EditorClientWinCE_h

#include "EditorClient.h"
#include "TextCheckerClient.h"

class WebView;

namespace WebKit {

class EditorClientWinCE : public WebCore::EditorClient, public WebCore::TextCheckerClient {
public:
    EditorClientWinCE(WebView*);
    ~EditorClientWinCE();

    virtual void pageDestroyed();
    virtual void frameWillDetachPage(WebCore::Frame*) { }

    virtual bool shouldDeleteRange(WebCore::Range*);
    virtual bool smartInsertDeleteEnabled();
    virtual bool isSelectTrailingWhitespaceEnabled();
    virtual bool isContinuousSpellCheckingEnabled();
    virtual void toggleContinuousSpellChecking();
    virtual bool isGrammarCheckingEnabled();
    virtual void toggleGrammarChecking();
    virtual int spellCheckerDocumentTag();

    virtual bool shouldBeginEditing(WebCore::Range*);
    virtual bool shouldEndEditing(WebCore::Range*);
    virtual bool shouldInsertNode(WebCore::Node*, WebCore::Range*, WebCore::EditorInsertAction);
    virtual bool shouldInsertText(const WTF::String&, WebCore::Range*, WebCore::EditorInsertAction);
    virtual bool shouldChangeSelectedRange(WebCore::Range*, WebCore::Range*, WebCore::EAffinity, bool);

    virtual bool shouldApplyStyle(WebCore::StylePropertySet*, WebCore::Range*);
    virtual bool shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range*);

    virtual void didBeginEditing();
    virtual void respondToChangedContents();
    virtual void respondToChangedSelection(WebCore::Frame*);
    virtual void didEndEditing();
    virtual void willWriteSelectionToPasteboard(WebCore::Range*);
    virtual void didWriteSelectionToPasteboard();
    virtual void getClientPasteboardDataForRange(WebCore::Range*, Vector<String>& pasteboardTypes, Vector<RefPtr<WebCore::SharedBuffer> >& pasteboardData);
    virtual void didSetSelectionTypesForPasteboard();

    virtual void registerUndoStep(WTF::PassRefPtr<WebCore::UndoStep>);
    virtual void registerRedoStep(WTF::PassRefPtr<WebCore::UndoStep>);
    virtual void clearUndoRedoOperations();

    virtual bool canCopyCut(WebCore::Frame*, bool defaultValue) const;
    virtual bool canPaste(WebCore::Frame*, bool defaultValue) const;
    virtual bool canUndo() const;
    virtual bool canRedo() const;

    virtual void undo();
    virtual void redo();

    virtual const char* interpretKeyEvent(const WebCore::KeyboardEvent*);
    virtual bool handleEditingKeyboardEvent(WebCore::KeyboardEvent*);
    virtual void handleKeyboardEvent(WebCore::KeyboardEvent*);
    virtual void handleInputMethodKeydown(WebCore::KeyboardEvent*);

    virtual void textFieldDidBeginEditing(WebCore::Element*);
    virtual void textFieldDidEndEditing(WebCore::Element*);
    virtual void textDidChangeInTextField(WebCore::Element*);
    virtual bool doTextFieldCommandFromEvent(WebCore::Element*, WebCore::KeyboardEvent*);
    virtual void textWillBeDeletedInTextField(WebCore::Element*);
    virtual void textDidChangeInTextArea(WebCore::Element*);

    virtual bool shouldEraseMarkersAfterChangeSelection(WebCore::TextCheckingType) const;
    virtual void ignoreWordInSpellDocument(const WTF::String&);
    virtual void learnWord(const WTF::String&);
    virtual void checkSpellingOfString(const UChar*, int, int*, int*);
    virtual WTF::String getAutoCorrectSuggestionForMisspelledWord(const WTF::String&);
    virtual void checkGrammarOfString(const UChar*, int, WTF::Vector<WebCore::GrammarDetail>&, int*, int*);
    virtual void updateSpellingUIWithGrammarString(const WTF::String&, const WebCore::GrammarDetail&);
    virtual void updateSpellingUIWithMisspelledWord(const WTF::String&);
    virtual void showSpellingUI(bool);
    virtual bool spellingUIIsShowing();
    virtual void getGuessesForWord(const WTF::String& word, const WTF::String& context, WTF::Vector<WTF::String>& guesses);
    virtual void willSetInputMethodState();
    virtual void setInputMethodState(bool);
    virtual void requestCheckingOfString(WTF::PassRefPtr<WebCore::TextCheckingRequest>) { }
    virtual WebCore::TextCheckerClient* textChecker() { return this; }

private:
    WebView* m_webView;
};

} // namespace WebKit

#endif // EditorClientWinCE_h

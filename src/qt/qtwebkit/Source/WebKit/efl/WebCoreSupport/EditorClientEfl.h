/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
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

#ifndef EditorClientEfl_h
#define EditorClientEfl_h

#include "EditorClient.h"

#include "EditorInsertAction.h"
#include "TextAffinity.h"
#include "TextCheckerClient.h"
#include <wtf/Deque.h>
#include <wtf/Forward.h>

struct Ewk_Should_Insert_Node_Event {
    WebCore::Node* node;
    WebCore::Range* range;
    WebCore::EditorInsertAction action;
};

struct Ewk_Should_Insert_Text_Event {
    const char* text;
    WebCore::Range* range;
    WebCore::EditorInsertAction action;
};

struct Ewk_Should_Change_Selected_Range_Event {
    WebCore::Range* fromRange;
    WebCore::Range* toRange;
    WebCore::EAffinity affinity;
    bool stillSelecting;
};

struct Ewk_Should_Apply_Style_Event {
    WebCore::StylePropertySet* style;
    WebCore::Range* range;
};

namespace WebCore {
class Page;

class EditorClientEfl : public EditorClient, public TextCheckerClient {
protected:
    bool m_isInRedo;
    WTF::Deque<WTF::RefPtr<WebCore::UndoStep> > undoStack;
    WTF::Deque<WTF::RefPtr<WebCore::UndoStep> > redoStack;

public:
    explicit EditorClientEfl(Evas_Object* view);
    ~EditorClientEfl();

    // from EditorClient
    virtual void pageDestroyed();
    virtual void frameWillDetachPage(Frame*) { }

    virtual bool shouldDeleteRange(Range*);
    virtual bool smartInsertDeleteEnabled();
    virtual bool isSelectTrailingWhitespaceEnabled();
    virtual bool isContinuousSpellCheckingEnabled();
    virtual void toggleContinuousSpellChecking();
    virtual bool isGrammarCheckingEnabled();
    virtual void toggleGrammarChecking();
    virtual int spellCheckerDocumentTag();

    virtual bool shouldBeginEditing(Range*);
    virtual bool shouldEndEditing(Range*);
    virtual bool shouldInsertNode(Node*, Range*, EditorInsertAction);
    virtual bool shouldInsertText(const String&, Range*, EditorInsertAction);
    virtual bool shouldChangeSelectedRange(Range* fromRange, Range* toRange, EAffinity, bool stillSelecting);

    virtual bool shouldApplyStyle(StylePropertySet*, Range*);

    virtual bool shouldMoveRangeAfterDelete(Range*, Range*);

    virtual void didBeginEditing();
    virtual void respondToChangedContents();
    virtual void respondToChangedSelection(Frame*);
    virtual void didEndEditing();
    virtual void willWriteSelectionToPasteboard(WebCore::Range*);
    virtual void didWriteSelectionToPasteboard();
    virtual void getClientPasteboardDataForRange(WebCore::Range*, Vector<String>& pasteboardTypes, Vector<RefPtr<WebCore::SharedBuffer> >& pasteboardData);
    virtual void didSetSelectionTypesForPasteboard();

    virtual void registerUndoStep(WTF::PassRefPtr<UndoStep>);
    virtual void registerRedoStep(WTF::PassRefPtr<UndoStep>);
    virtual void clearUndoRedoOperations();

    virtual bool canCopyCut(Frame*, bool defaultValue) const;
    virtual bool canPaste(Frame*, bool defaultValue) const;
    virtual bool canUndo() const;
    virtual bool canRedo() const;

    virtual void undo();
    virtual void redo();

    virtual const char* interpretKeyEvent(const KeyboardEvent*);
    virtual bool handleEditingKeyboardEvent(KeyboardEvent*);
    virtual void handleKeyboardEvent(KeyboardEvent*);
    virtual void handleInputMethodKeydown(KeyboardEvent*);

    virtual void textFieldDidBeginEditing(Element*);
    virtual void textFieldDidEndEditing(Element*);
    virtual void textDidChangeInTextField(Element*);
    virtual bool doTextFieldCommandFromEvent(Element*, KeyboardEvent*);
    virtual void textWillBeDeletedInTextField(Element*);
    virtual void textDidChangeInTextArea(Element*);

    virtual bool shouldEraseMarkersAfterChangeSelection(TextCheckingType) const;
    virtual void ignoreWordInSpellDocument(const String&);
    virtual void learnWord(const String&);
    virtual void checkSpellingOfString(const UChar*, int length, int* misspellingLocation, int* misspellingLength);
    virtual String getAutoCorrectSuggestionForMisspelledWord(const String& misspelledWord);
    virtual void checkGrammarOfString(const UChar*, int length, WTF::Vector<GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength);
    virtual void updateSpellingUIWithGrammarString(const String&, const GrammarDetail&);
    virtual void updateSpellingUIWithMisspelledWord(const String&);
    virtual void showSpellingUI(bool show);
    virtual bool spellingUIIsShowing();
    virtual void getGuessesForWord(const String& word, const String& context, WTF::Vector<String>& guesses);
    virtual void willSetInputMethodState();
    virtual void setInputMethodState(bool enabled);
    virtual void requestCheckingOfString(WTF::PassRefPtr<WebCore::TextCheckingRequest>) { }
#if USE(UNIFIED_TEXT_CHECKING)
    virtual void checkTextOfParagraph(const UChar*, int, TextCheckingTypeMask, Vector<TextCheckingResult>&) { }
#endif
    virtual TextCheckerClient* textChecker() { return this; }

private:
    Evas_Object *m_view;
};
}

#endif // EditorClientEfl_h

/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
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

#ifndef EditorClientQt_h
#define EditorClientQt_h

#include "EditorClient.h"
#include "TextCheckerClientQt.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

class QWebPage;
class QWebPageAdapter;

namespace WebCore {

class EditorClientQt : public EditorClient {
public:
    EditorClientQt(QWebPageAdapter*);
    
    virtual void pageDestroyed();
    virtual void frameWillDetachPage(Frame*) { }
    
    virtual bool shouldDeleteRange(Range*);
    virtual bool smartInsertDeleteEnabled(); 
    virtual void toggleSmartInsertDelete();
    virtual bool isSelectTrailingWhitespaceEnabled(); 
    virtual bool isContinuousSpellCheckingEnabled();
    virtual void toggleContinuousSpellChecking();
    virtual bool isGrammarCheckingEnabled();
    virtual void toggleGrammarChecking();
    virtual int spellCheckerDocumentTag();
    virtual bool selectWordBeforeMenuEvent();

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
    virtual void willWriteSelectionToPasteboard(Range*);
    virtual void didWriteSelectionToPasteboard();
    virtual void getClientPasteboardDataForRange(Range*, Vector<String>& pasteboardTypes, Vector<RefPtr<WebCore::SharedBuffer> >& pasteboardData);
    virtual void didSetSelectionTypesForPasteboard();
    
    virtual void registerUndoStep(PassRefPtr<UndoStep>);
    virtual void registerRedoStep(PassRefPtr<UndoStep>);
    virtual void clearUndoRedoOperations();

    virtual bool canCopyCut(Frame*, bool defaultValue) const;
    virtual bool canPaste(Frame*, bool defaultValue) const;
    virtual bool canUndo() const;
    virtual bool canRedo() const;
    
    virtual void undo();
    virtual void redo();

    virtual void handleKeyboardEvent(KeyboardEvent*);
    virtual void handleInputMethodKeydown(KeyboardEvent*);

    virtual void textFieldDidBeginEditing(Element*);
    virtual void textFieldDidEndEditing(Element*);
    virtual void textDidChangeInTextField(Element*);
    virtual bool doTextFieldCommandFromEvent(Element*, KeyboardEvent*);
    virtual void textWillBeDeletedInTextField(Element*);
    virtual void textDidChangeInTextArea(Element*);

    virtual void updateSpellingUIWithGrammarString(const String&, const GrammarDetail&);
    virtual void updateSpellingUIWithMisspelledWord(const String&);
    virtual void showSpellingUI(bool show);
    virtual bool spellingUIIsShowing();
    virtual void willSetInputMethodState();
    virtual void setInputMethodState(bool enabled);
    virtual TextCheckerClient* textChecker() { return &m_textCheckerClient; }

    virtual bool supportsGlobalSelection() OVERRIDE;

    bool isEditing() const;

    static bool dumpEditingCallbacks;
    static bool acceptsEditing;

private:
    TextCheckerClientQt m_textCheckerClient;
    QWebPageAdapter* m_page;
    bool m_editing;
    bool m_inUndoRedo; // our undo stack works differently - don't re-enter!
};

}

#endif

// vim: ts=4 sw=4 et

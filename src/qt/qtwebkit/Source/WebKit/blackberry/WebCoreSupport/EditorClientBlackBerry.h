/*
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef EditorClientBlackBerry_h
#define EditorClientBlackBerry_h

#include "EditorClient.h"
#include "TextCheckerClient.h"

#include <wtf/Deque.h>

namespace BlackBerry {
namespace WebKit {
class WebPagePrivate;
}
}

namespace WebCore {

class EditorClientBlackBerry : public EditorClient, public TextCheckerClient {
public:
    EditorClientBlackBerry(BlackBerry::WebKit::WebPagePrivate*);
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
    virtual bool shouldChangeSelectedRange(Range*, Range*, EAffinity, bool);
    virtual bool shouldApplyStyle(StylePropertySet*, Range*);
    virtual bool shouldMoveRangeAfterDelete(Range*, Range*);
    virtual void didBeginEditing();
    virtual void respondToChangedContents();
    virtual void respondToChangedSelection(Frame*);
    virtual void respondToSelectionAppearanceChange();
    virtual void didEndEditing();
    virtual void willWriteSelectionToPasteboard(WebCore::Range*);
    virtual void didWriteSelectionToPasteboard();
    virtual void getClientPasteboardDataForRange(WebCore::Range*, Vector<String>& pasteboardTypes, Vector<RefPtr<WebCore::SharedBuffer> >& pasteboardData);
    virtual void didSetSelectionTypesForPasteboard();
    virtual void registerUndoStep(PassRefPtr<UndoStep>);
    virtual void registerRedoStep(PassRefPtr<UndoStep>);
    virtual void clearUndoRedoOperations();
    virtual bool canCopyCut(Frame*, bool) const;
    virtual bool canPaste(Frame*, bool) const;
    virtual bool canUndo() const;
    virtual bool canRedo() const;
    virtual void undo();
    virtual void redo();
    virtual const char* interpretKeyEvent(const KeyboardEvent*);
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
    virtual void checkSpellingOfString(const UChar*, int, int*, int*);
    virtual String getAutoCorrectSuggestionForMisspelledWord(const String& misspelledWord);
    virtual void checkGrammarOfString(const UChar*, int, Vector<GrammarDetail, 0u>&, int*, int*);
    virtual void getGuessesForWord(const String&, const String&, Vector<String>&);
    virtual void requestCheckingOfString(WTF::PassRefPtr<WebCore::TextCheckingRequest>);
    virtual void checkTextOfParagraph(const UChar*, int, TextCheckingTypeMask, Vector<TextCheckingResult>&);

    virtual TextCheckerClient* textChecker();
    virtual void updateSpellingUIWithGrammarString(const String&, const GrammarDetail&);
    virtual void updateSpellingUIWithMisspelledWord(const String&);
    virtual void showSpellingUI(bool);
    virtual bool spellingUIIsShowing();
    virtual void getGuessesForWord(const String&, Vector<String, 0u>&);
    virtual void willSetInputMethodState();
    virtual void setInputMethodState(bool);

    void enableSpellChecking(bool);

private:
    bool shouldSpellCheckFocusedField();

    BlackBerry::WebKit::WebPagePrivate* m_webPagePrivate;
    bool m_waitingForCursorFocus;

    enum SpellCheckState { SpellCheckDefault, SpellCheckOn, SpellCheckOff };
    SpellCheckState m_spellCheckState;

    bool m_inRedo;

    typedef Deque<RefPtr<WebCore::UndoStep> > EditCommandStack;
    EditCommandStack m_undoStack;
    EditCommandStack m_redoStack;
};

} // WebCore

#endif // EditorClientBlackBerry_h

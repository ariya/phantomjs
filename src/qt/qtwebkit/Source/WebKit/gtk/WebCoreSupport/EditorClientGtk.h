/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2010 Martin Robinson <mrobinson@webkit.org>
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

#ifndef EditorClientGtk_h
#define EditorClientGtk_h

#include "EditorClient.h"
#include "KeyBindingTranslator.h"
#include "TextCheckerClient.h"
#include <wtf/Deque.h>
#include <wtf/Forward.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>

#if ENABLE(SPELLCHECK)
#include "TextCheckerClientGtk.h"
#else
#include "EmptyClients.h"
#endif

typedef struct _WebKitWebView WebKitWebView;

namespace WebCore {
class Frame;
class KeyboardEvent;
}

namespace WebKit {

class EditorClient : public WebCore::EditorClient {
    protected:
        bool m_isInRedo;

        WTF::Deque<WTF::RefPtr<WebCore::UndoStep> > undoStack;
        WTF::Deque<WTF::RefPtr<WebCore::UndoStep> > redoStack;

    public:
        EditorClient(WebKitWebView*);
        ~EditorClient();
        WebKitWebView* webView() { return m_webView; }
        void addPendingEditorCommand(const char* command) { m_pendingEditorCommands.append(command); }
        void generateEditorCommands(const WebCore::KeyboardEvent*);
        bool executePendingEditorCommands(WebCore::Frame*, bool);

        // from EditorClient
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
        virtual bool shouldChangeSelectedRange(WebCore::Range* fromRange, WebCore::Range* toRange, WebCore::EAffinity, bool stillSelecting);

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

        virtual void handleKeyboardEvent(WebCore::KeyboardEvent*);
        virtual void handleInputMethodKeydown(WebCore::KeyboardEvent*);

        virtual void textFieldDidBeginEditing(WebCore::Element*);
        virtual void textFieldDidEndEditing(WebCore::Element*);
        virtual void textDidChangeInTextField(WebCore::Element*);
        virtual bool doTextFieldCommandFromEvent(WebCore::Element*, WebCore::KeyboardEvent*);
        virtual void textWillBeDeletedInTextField(WebCore::Element*);
        virtual void textDidChangeInTextArea(WebCore::Element*);

        virtual WebCore::TextCheckerClient* textChecker() { return &m_textCheckerClient; }

        virtual void updateSpellingUIWithGrammarString(const WTF::String&, const WebCore::GrammarDetail&);
        virtual void updateSpellingUIWithMisspelledWord(const WTF::String&);
        virtual void showSpellingUI(bool show);
        virtual bool spellingUIIsShowing();
        virtual void willSetInputMethodState();
        virtual void setInputMethodState(bool enabled);

        virtual bool shouldShowUnicodeMenu();

        virtual bool supportsGlobalSelection() OVERRIDE;

    private:
#if ENABLE(SPELLCHECK)
        TextCheckerClientGtk m_textCheckerClient;
#else
        WebCore::EmptyTextCheckerClient m_textCheckerClient;
#endif
        WebKitWebView* m_webView;
        WebCore::KeyBindingTranslator m_keyBindingTranslator;
        Vector<WTF::String> m_pendingEditorCommands;
    };
}

#endif

// vim: ts=4 sw=4 et

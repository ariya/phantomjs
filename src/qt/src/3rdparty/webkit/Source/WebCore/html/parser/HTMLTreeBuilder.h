/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef HTMLTreeBuilder_h
#define HTMLTreeBuilder_h

#include "Element.h"
#include "FragmentScriptingPermission.h"
#include "HTMLConstructionSite.h"
#include "HTMLElementStack.h"
#include "HTMLFormattingElementList.h"
#include "HTMLTokenizer.h"
#include <wtf/text/TextPosition.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/unicode/Unicode.h>

namespace WebCore {

class AtomicHTMLToken;
class Document;
class DocumentFragment;
class Frame;
class HTMLToken;
class HTMLDocument;
class Node;
class HTMLDocumentParser;

class HTMLTreeBuilder {
    WTF_MAKE_NONCOPYABLE(HTMLTreeBuilder); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<HTMLTreeBuilder> create(HTMLDocumentParser* parser, HTMLDocument* document, bool reportErrors, bool usePreHTML5ParserQuirks)
    {
        return adoptPtr(new HTMLTreeBuilder(parser, document, reportErrors, usePreHTML5ParserQuirks));
    }
    static PassOwnPtr<HTMLTreeBuilder> create(HTMLDocumentParser* parser, DocumentFragment* fragment, Element* contextElement, FragmentScriptingPermission scriptingPermission, bool usePreHTML5ParserQuirks)
    {
        return adoptPtr(new HTMLTreeBuilder(parser, fragment, contextElement, scriptingPermission, usePreHTML5ParserQuirks));
    }
    ~HTMLTreeBuilder();

    bool isParsingFragment() const { return !!m_fragmentContext.fragment(); }

    void detach();

    void setPaused(bool paused) { m_isPaused = paused; }
    bool isPaused() const { return m_isPaused; }

    // The token really should be passed as a const& since it's never modified.
    void constructTreeFromToken(HTMLToken&);
    void constructTreeFromAtomicToken(AtomicHTMLToken&);

    // Must be called when parser is paused before calling the parser again.
    PassRefPtr<Element> takeScriptToProcess(TextPosition1& scriptStartPosition);

    // Done, close any open tags, etc.
    void finished();

    static bool scriptEnabled(Frame*);
    static bool pluginsEnabled(Frame*);

private:
    class FakeInsertionMode;
    class ExternalCharacterTokenBuffer;
    // Represents HTML5 "insertion mode"
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#insertion-mode
    enum InsertionMode {
        InitialMode,
        BeforeHTMLMode,
        BeforeHeadMode,
        InHeadMode,
        InHeadNoscriptMode,
        AfterHeadMode,
        InBodyMode,
        TextMode,
        InTableMode,
        InTableTextMode,
        InCaptionMode,
        InColumnGroupMode,
        InTableBodyMode,
        InRowMode,
        InCellMode,
        InSelectMode,
        InSelectInTableMode,
        InForeignContentMode,
        AfterBodyMode,
        InFramesetMode,
        AfterFramesetMode,
        AfterAfterBodyMode,
        AfterAfterFramesetMode,
    };

    HTMLTreeBuilder(HTMLDocumentParser* parser, HTMLDocument*, bool reportErrors, bool usePreHTML5ParserQuirks);
    HTMLTreeBuilder(HTMLDocumentParser* parser, DocumentFragment*, Element* contextElement, FragmentScriptingPermission, bool usePreHTML5ParserQuirks);

    void processToken(AtomicHTMLToken&);

    void processDoctypeToken(AtomicHTMLToken&);
    void processStartTag(AtomicHTMLToken&);
    void processEndTag(AtomicHTMLToken&);
    void processComment(AtomicHTMLToken&);
    void processCharacter(AtomicHTMLToken&);
    void processEndOfFile(AtomicHTMLToken&);

    bool processStartTagForInHead(AtomicHTMLToken&);
    void processStartTagForInBody(AtomicHTMLToken&);
    void processStartTagForInTable(AtomicHTMLToken&);
    void processEndTagForInBody(AtomicHTMLToken&);
    void processEndTagForInTable(AtomicHTMLToken&);
    void processEndTagForInTableBody(AtomicHTMLToken&);
    void processEndTagForInRow(AtomicHTMLToken&);
    void processEndTagForInCell(AtomicHTMLToken&);

    void processIsindexStartTagForInBody(AtomicHTMLToken&);
    bool processBodyEndTagForInBody(AtomicHTMLToken&);
    bool processTableEndTagForInTable();
    bool processCaptionEndTagForInCaption();
    bool processColgroupEndTagForInColumnGroup();
    bool processTrEndTagForInRow();
    // FIXME: This function should be inlined into its one call site or it
    // needs to assert which tokens it can be called with.
    void processAnyOtherEndTagForInBody(AtomicHTMLToken&);

    void processCharacterBuffer(ExternalCharacterTokenBuffer&);

    void processFakeStartTag(const QualifiedName&, PassRefPtr<NamedNodeMap> attributes = 0);
    void processFakeEndTag(const QualifiedName&);
    void processFakeCharacters(const String&);
    void processFakePEndTagIfPInButtonScope();

    void processGenericRCDATAStartTag(AtomicHTMLToken&);
    void processGenericRawTextStartTag(AtomicHTMLToken&);
    void processScriptStartTag(AtomicHTMLToken&);

    // Default processing for the different insertion modes.
    void defaultForInitial();
    void defaultForBeforeHTML();
    void defaultForBeforeHead();
    void defaultForInHead();
    void defaultForInHeadNoscript();
    void defaultForAfterHead();
    void defaultForInTableText();

    void prepareToReprocessToken();

    void reprocessStartTag(AtomicHTMLToken&);
    void reprocessEndTag(AtomicHTMLToken&);

    PassRefPtr<NamedNodeMap> attributesForIsindexInput(AtomicHTMLToken&);

    HTMLElementStack::ElementRecord* furthestBlockForFormattingElement(Element*);
    void callTheAdoptionAgency(AtomicHTMLToken&);

    void closeTheCell();

    template <bool shouldClose(const ContainerNode*)>
    void processCloseWhenNestedTag(AtomicHTMLToken&);

    bool m_framesetOk;

    void parseError(AtomicHTMLToken&);

    InsertionMode insertionMode() const { return m_insertionMode; }
    void setInsertionMode(InsertionMode mode)
    {
        m_insertionMode = mode;
        m_isFakeInsertionMode = false;
    }

    bool isFakeInsertionMode() { return m_isFakeInsertionMode; }
    void setFakeInsertionMode(InsertionMode mode)
    {
        m_insertionMode = mode;
        m_isFakeInsertionMode = true;
    }

    void resetInsertionModeAppropriately();

    void processForeignContentUsingInBodyModeAndResetMode(AtomicHTMLToken& token);
    void resetForeignInsertionMode();

    class FragmentParsingContext {
        WTF_MAKE_NONCOPYABLE(FragmentParsingContext);
    public:
        FragmentParsingContext();
        FragmentParsingContext(DocumentFragment*, Element* contextElement, FragmentScriptingPermission);
        ~FragmentParsingContext();

        DocumentFragment* fragment() const { return m_fragment; }
        Element* contextElement() const { ASSERT(m_fragment); return m_contextElement; }
        FragmentScriptingPermission scriptingPermission() const { ASSERT(m_fragment); return m_scriptingPermission; }

    private:
        DocumentFragment* m_fragment;
        Element* m_contextElement;

        // FragmentScriptingNotAllowed causes the Parser to remove children
        // from <script> tags (so javascript doesn't show up in pastes).
        FragmentScriptingPermission m_scriptingPermission;
    };

    FragmentParsingContext m_fragmentContext;

    Document* m_document;
    HTMLConstructionSite m_tree;

    bool m_reportErrors;
    bool m_isPaused;
    bool m_isFakeInsertionMode;

    // FIXME: InsertionModes should be a separate object to prevent direct
    // manipulation of these variables.  For now, be careful to always use
    // setInsertionMode and never set m_insertionMode directly.
    InsertionMode m_insertionMode;
    InsertionMode m_originalInsertionMode;

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#pending-table-character-tokens
    Vector<UChar> m_pendingTableCharacters;

    // We access parser because HTML5 spec requires that we be able to change the state of the tokenizer
    // from within parser actions. We also need it to track the current position.
    HTMLDocumentParser* m_parser;

    RefPtr<Element> m_scriptToProcess; // <script> tag which needs processing before resuming the parser.
    TextPosition1 m_scriptToProcessStartPosition; // Starting line number of the script tag needing processing.

    // FIXME: We probably want to remove this member.  Originally, it was
    // created to service the legacy tree builder, but it seems to be used for
    // some other things now.
    TextPosition0 m_lastScriptElementStartPosition;

    bool m_usePreHTML5ParserQuirks;

    bool m_hasPendingForeignInsertionModeSteps;
};

}

#endif

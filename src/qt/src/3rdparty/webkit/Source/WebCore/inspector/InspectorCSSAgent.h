/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorCSSAgent_h
#define InspectorCSSAgent_h

#include "InspectorDOMAgent.h"
#include "InspectorStyleSheet.h"
#include "InspectorValues.h"
#include "PlatformString.h"

#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSRule;
class CSSRuleList;
class CSSStyleDeclaration;
class CSSStyleRule;
class CSSStyleSheet;
class Document;
class Element;
class InspectorFrontend;
class InstrumentingAgents;
class NameNodeMap;
class Node;
class StyleBase;

#if ENABLE(INSPECTOR)

class InspectorCSSAgent : public InspectorDOMAgent::DOMListener {
    WTF_MAKE_NONCOPYABLE(InspectorCSSAgent);
public:
    static CSSStyleSheet* parentStyleSheet(StyleBase*);
    static CSSStyleRule* asCSSStyleRule(StyleBase*);

    InspectorCSSAgent(InstrumentingAgents*, InspectorDOMAgent*);
    ~InspectorCSSAgent();

    void reset();
    void getStylesForNode(ErrorString*, int nodeId, RefPtr<InspectorObject>* result);
    void getInlineStyleForNode(ErrorString*, int nodeId, RefPtr<InspectorObject>* style);
    void getComputedStyleForNode(ErrorString*, int nodeId, RefPtr<InspectorObject>* style);
    void getAllStyleSheets(ErrorString*, RefPtr<InspectorArray>* styleSheetInfos);
    void getStyleSheet(ErrorString*, const String& styleSheetId, RefPtr<InspectorObject>* result);
    void getStyleSheetText(ErrorString*, const String& styleSheetId, String* result);
    void setStyleSheetText(ErrorString*, const String& styleSheetId, const String& text);
    void setPropertyText(ErrorString*, const RefPtr<InspectorObject>& styleId, int propertyIndex, const String& text, bool overwrite, RefPtr<InspectorObject>* result);
    void toggleProperty(ErrorString*, const RefPtr<InspectorObject>& styleId, int propertyIndex, bool disable, RefPtr<InspectorObject>* result);
    void setRuleSelector(ErrorString*, const RefPtr<InspectorObject>& ruleId, const String& selector, RefPtr<InspectorObject>* result);
    void addRule(ErrorString*, const int contextNodeId, const String& selector, RefPtr<InspectorObject>* result);
    void getSupportedCSSProperties(ErrorString*, RefPtr<InspectorArray>* result);

private:
    typedef HashMap<String, RefPtr<InspectorStyleSheet> > IdToInspectorStyleSheet;
    typedef HashMap<CSSStyleSheet*, RefPtr<InspectorStyleSheet> > CSSStyleSheetToInspectorStyleSheet;
    typedef HashMap<Node*, RefPtr<InspectorStyleSheetForInlineStyle> > NodeToInspectorStyleSheet; // bogus "stylesheets" with elements' inline styles
    typedef HashMap<RefPtr<Document>, RefPtr<InspectorStyleSheet> > DocumentToViaInspectorStyleSheet; // "via inspector" stylesheets

    static Element* inlineStyleElement(CSSStyleDeclaration*);

    InspectorStyleSheetForInlineStyle* asInspectorStyleSheet(Element* element);
    Element* elementForId(ErrorString*, int nodeId);

    InspectorStyleSheet* bindStyleSheet(CSSStyleSheet*);
    InspectorStyleSheet* viaInspectorStyleSheet(Document*, bool createIfAbsent);
    InspectorStyleSheet* assertStyleSheetForId(ErrorString*, const String&);
    String detectOrigin(CSSStyleSheet* pageStyleSheet, Document* ownerDocument);

    PassRefPtr<InspectorArray> buildArrayForRuleList(CSSRuleList* ruleList);
    PassRefPtr<InspectorArray> buildArrayForAttributeStyles(Element*);

    // InspectorDOMAgent::DOMListener interface
    virtual void didRemoveDocument(Document*);
    virtual void didRemoveDOMNode(Node*);
    virtual void didModifyDOMAttr(Element*);

    InstrumentingAgents* m_instrumentingAgents;
    InspectorDOMAgent* m_domAgent;

    IdToInspectorStyleSheet m_idToInspectorStyleSheet;
    CSSStyleSheetToInspectorStyleSheet m_cssStyleSheetToInspectorStyleSheet;
    NodeToInspectorStyleSheet m_nodeToInspectorStyleSheet;
    DocumentToViaInspectorStyleSheet m_documentToInspectorStyleSheet;

    int m_lastStyleSheetId;
    int m_lastRuleId;
    int m_lastStyleId;
};

#endif

} // namespace WebCore

#endif // !defined(InspectorCSSAgent_h)

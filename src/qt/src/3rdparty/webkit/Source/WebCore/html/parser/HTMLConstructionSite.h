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

#ifndef HTMLConstructionSite_h
#define HTMLConstructionSite_h

#include "FragmentScriptingPermission.h"
#include "HTMLElementStack.h"
#include "HTMLFormattingElementList.h"
#include "NotImplemented.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class AtomicHTMLToken;
class Document;
class Element;

class HTMLConstructionSite {
    WTF_MAKE_NONCOPYABLE(HTMLConstructionSite);
public:
    HTMLConstructionSite(Document*);
    HTMLConstructionSite(DocumentFragment*, FragmentScriptingPermission);
    ~HTMLConstructionSite();

    void detach();

    void insertDoctype(AtomicHTMLToken&);
    void insertComment(AtomicHTMLToken&);
    void insertCommentOnDocument(AtomicHTMLToken&);
    void insertCommentOnHTMLHtmlElement(AtomicHTMLToken&);
    void insertHTMLElement(AtomicHTMLToken&);
    void insertSelfClosingHTMLElement(AtomicHTMLToken&);
    void insertFormattingElement(AtomicHTMLToken&);
    void insertHTMLHeadElement(AtomicHTMLToken&);
    void insertHTMLBodyElement(AtomicHTMLToken&);
    void insertHTMLFormElement(AtomicHTMLToken&, bool isDemoted = false);
    void insertScriptElement(AtomicHTMLToken&);
    void insertTextNode(const String&);
    void insertForeignElement(AtomicHTMLToken&, const AtomicString& namespaceURI);

    void insertHTMLHtmlStartTagBeforeHTML(AtomicHTMLToken&);
    void insertHTMLHtmlStartTagInBody(AtomicHTMLToken&);
    void insertHTMLBodyStartTagInBody(AtomicHTMLToken&);

    PassRefPtr<Element> createHTMLElement(AtomicHTMLToken&);
    PassRefPtr<Element> createHTMLElementFromElementRecord(HTMLElementStack::ElementRecord*);

    bool shouldFosterParent() const;
    void fosterParent(Node*);

    bool indexOfFirstUnopenFormattingElement(unsigned& firstUnopenElementIndex) const;
    void reconstructTheActiveFormattingElements();

    void generateImpliedEndTags();
    void generateImpliedEndTagsWithExclusion(const AtomicString& tagName);

    Element* currentElement() const { return m_openElements.top(); }
    ContainerNode* currentNode() const { return m_openElements.topNode(); }
    Element* oneBelowTop() const { return m_openElements.oneBelowTop(); }

    HTMLElementStack* openElements() const { return &m_openElements; }
    HTMLFormattingElementList* activeFormattingElements() const { return &m_activeFormattingElements; }

    Element* head() const { return m_head.get(); }

    void setForm(HTMLFormElement*);
    HTMLFormElement* form() const { return m_form.get(); }
    PassRefPtr<HTMLFormElement> takeForm();

    class RedirectToFosterParentGuard {
        WTF_MAKE_NONCOPYABLE(RedirectToFosterParentGuard);
    public:
        RedirectToFosterParentGuard(HTMLConstructionSite& tree)
            : m_tree(tree)
            , m_wasRedirectingBefore(tree.m_redirectAttachToFosterParent)
        {
            m_tree.m_redirectAttachToFosterParent = true;
        }

        ~RedirectToFosterParentGuard()
        {
            m_tree.m_redirectAttachToFosterParent = m_wasRedirectingBefore;
        }

    private:
        HTMLConstructionSite& m_tree;
        bool m_wasRedirectingBefore;
    };

private:
    struct AttachmentSite {
        ContainerNode* parent;
        Node* nextChild;
    };

    template<typename ChildType>
    PassRefPtr<ChildType> attach(ContainerNode* parent, PassRefPtr<ChildType> child);
    PassRefPtr<Element> attachToCurrent(PassRefPtr<Element>);

    void attachAtSite(const AttachmentSite&, PassRefPtr<Node> child);
    void findFosterSite(AttachmentSite&);

    PassRefPtr<Element> createHTMLElementFromSavedElement(Element*);
    PassRefPtr<Element> createElement(AtomicHTMLToken&, const AtomicString& namespaceURI);

    void mergeAttributesFromTokenIntoElement(AtomicHTMLToken&, Element*);
    void dispatchDocumentElementAvailableIfNeeded();

    Document* m_document;
    
    // This is the root ContainerNode to which the parser attaches all newly
    // constructed nodes. It points to a DocumentFragment when parsing fragments
    // and a Document in all other cases.
    ContainerNode* m_attachmentRoot;
    
    RefPtr<Element> m_head;
    RefPtr<HTMLFormElement> m_form;
    mutable HTMLElementStack m_openElements;
    mutable HTMLFormattingElementList m_activeFormattingElements;

    FragmentScriptingPermission m_fragmentScriptingPermission;
    bool m_isParsingFragment;

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#parsing-main-intable
    // In the "in table" insertion mode, we sometimes get into a state where
    // "whenever a node would be inserted into the current node, it must instead
    // be foster parented."  This flag tracks whether we're in that state.
    bool m_redirectAttachToFosterParent;
};

}

#endif

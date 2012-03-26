/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef MarkupAccumulator_h
#define MarkupAccumulator_h

#include "PlatformString.h"
#include "markup.h"
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace WebCore {

class Attribute;
class DocumentType;
class Element;
class Node;
class Range;

typedef HashMap<AtomicStringImpl*, AtomicStringImpl*> Namespaces;

enum EntityMask {
    EntityAmp = 0x0001,
    EntityLt = 0x0002,
    EntityGt = 0x0004,
    EntityQuot = 0x0008,
    EntityNbsp = 0x0010,

    // Non-breaking space needs to be escaped in innerHTML for compatibility reason. See http://trac.webkit.org/changeset/32879
    // However, we cannot do this in a XML document because it does not have the entity reference defined (See the bug 19215).
    EntityMaskInCDATA = 0,
    EntityMaskInPCDATA = EntityAmp | EntityLt | EntityGt,
    EntityMaskInHTMLPCDATA = EntityMaskInPCDATA | EntityNbsp,
    EntityMaskInAttributeValue = EntityAmp | EntityLt | EntityGt | EntityQuot,
    EntityMaskInHTMLAttributeValue = EntityMaskInAttributeValue | EntityNbsp,
};

struct EntityDescription {
    UChar entity;
    const String& reference;
    EntityMask mask;
};

// FIXME: Noncopyable?
class MarkupAccumulator {
public:
    MarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs, const Range* range = 0);
    virtual ~MarkupAccumulator();

    String serializeNodes(Node* node, Node* nodeToSkip, EChildrenOnly childrenOnly);

protected:
    virtual void appendString(const String&);
    void appendStartTag(Node*, Namespaces* = 0);
    virtual void appendEndTag(Node*);
    static size_t totalLength(const Vector<String>&);
    size_t length() const { return totalLength(m_succeedingMarkup); }
    void concatenateMarkup(Vector<UChar>& out);
    void appendAttributeValue(Vector<UChar>& result, const String& attribute, bool documentIsHTML);
    virtual void appendCustomAttributes(Vector<UChar>&, Element*, Namespaces*);
    void appendQuotedURLAttributeValue(Vector<UChar>& result, const String& urlString);
    void appendNodeValue(Vector<UChar>& out, const Node*, const Range*, EntityMask);
    bool shouldAddNamespaceElement(const Element*);
    bool shouldAddNamespaceAttribute(const Attribute&, Namespaces&);
    void appendNamespace(Vector<UChar>& result, const AtomicString& prefix, const AtomicString& namespaceURI, Namespaces&);
    EntityMask entityMaskForText(Text*) const;
    virtual void appendText(Vector<UChar>& out, Text*);
    void appendComment(Vector<UChar>& out, const String& comment);
    void appendDocumentType(Vector<UChar>& result, const DocumentType*);
    void appendProcessingInstruction(Vector<UChar>& out, const String& target, const String& data);
    virtual void appendElement(Vector<UChar>& out, Element*, Namespaces*);
    void appendOpenTag(Vector<UChar>& out, Element*, Namespaces*);
    void appendCloseTag(Vector<UChar>& out, Element*);
    void appendAttribute(Vector<UChar>& out, Element*, const Attribute&, Namespaces*);
    void appendCDATASection(Vector<UChar>& out, const String& section);
    void appendStartMarkup(Vector<UChar>& result, const Node*, Namespaces*);
    bool shouldSelfClose(const Node*);
    bool elementCannotHaveEndTag(const Node* node);
    void appendEndMarkup(Vector<UChar>& result, const Node*);

    bool shouldResolveURLs() { return m_shouldResolveURLs == AbsoluteURLs; }

    Vector<Node*>* const m_nodes;
    const Range* const m_range;

private:
    void serializeNodesWithNamespaces(Node*, Node* nodeToSkip, EChildrenOnly, const Namespaces*);

    Vector<String> m_succeedingMarkup;
    const bool m_shouldResolveURLs;
};

// FIXME: This method should be integrated with MarkupAccumulator.
void appendCharactersReplacingEntities(Vector<UChar>& out, const UChar* content, size_t length, EntityMask entityMask);

}

#endif

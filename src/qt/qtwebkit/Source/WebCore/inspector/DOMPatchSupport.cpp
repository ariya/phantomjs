/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR)

#include "DOMPatchSupport.h"

#include "Attribute.h"
#include "ContextFeatures.h"
#include "DOMEditor.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "HTMLDocument.h"
#include "HTMLDocumentParser.h"
#include "HTMLElement.h"
#include "HTMLHeadElement.h"
#include "HTMLNames.h"
#include "InspectorHistory.h"
#include "Node.h"
#include "XMLDocumentParser.h"

#include <wtf/Deque.h>
#include <wtf/HashTraits.h>
#include <wtf/RefPtr.h>
#include <wtf/SHA1.h>
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>

using namespace std;

namespace WebCore {

using HTMLNames::bodyTag;
using HTMLNames::headTag;
using HTMLNames::htmlTag;

struct DOMPatchSupport::Digest {
    explicit Digest(Node* node) : m_node(node) { }

    String m_sha1;
    String m_attrsSHA1;
    Node* m_node;
    Vector<OwnPtr<Digest> > m_children;
};

void DOMPatchSupport::patchDocument(Document* document, const String& markup)
{
    InspectorHistory history;
    DOMEditor domEditor(&history);
    DOMPatchSupport patchSupport(&domEditor, document);
    patchSupport.patchDocument(markup);
}

DOMPatchSupport::DOMPatchSupport(DOMEditor* domEditor, Document* document)
    : m_domEditor(domEditor)
    , m_document(document)
{
}

DOMPatchSupport::~DOMPatchSupport() { }

void DOMPatchSupport::patchDocument(const String& markup)
{
    RefPtr<Document> newDocument;
    if (m_document->isHTMLDocument())
        newDocument = HTMLDocument::create(0, KURL());
    else if (m_document->isXHTMLDocument())
        newDocument = HTMLDocument::createXHTML(0, KURL());
#if ENABLE(SVG)
    else if (m_document->isSVGDocument())
        newDocument = Document::create(0, KURL());
#endif

    ASSERT(newDocument);
    newDocument->setContextFeatures(m_document->contextFeatures());
    RefPtr<DocumentParser> parser;
    if (m_document->isHTMLDocument())
        parser = HTMLDocumentParser::create(static_cast<HTMLDocument*>(newDocument.get()), false);
    else
        parser = XMLDocumentParser::create(newDocument.get(), 0);
    parser->insert(markup); // Use insert() so that the parser will not yield.
    parser->finish();
    parser->detach();

    OwnPtr<Digest> oldInfo = createDigest(m_document->documentElement(), 0);
    OwnPtr<Digest> newInfo = createDigest(newDocument->documentElement(), &m_unusedNodesMap);

    if (!innerPatchNode(oldInfo.get(), newInfo.get(), IGNORE_EXCEPTION)) {
        // Fall back to rewrite.
        m_document->write(markup);
        m_document->close();
    }
}

Node* DOMPatchSupport::patchNode(Node* node, const String& markup, ExceptionCode& ec)
{
    // Don't parse <html> as a fragment.
    if (node->isDocumentNode() || (node->parentNode() && node->parentNode()->isDocumentNode())) {
        patchDocument(markup);
        return 0;
    }

    Node* previousSibling = node->previousSibling();
    // FIXME: This code should use one of createFragment* in markup.h
    RefPtr<DocumentFragment> fragment = DocumentFragment::create(m_document);
    if (m_document->isHTMLDocument())
        fragment->parseHTML(markup, node->parentElement() ? node->parentElement() : m_document->documentElement());
    else
        fragment->parseXML(markup, node->parentElement() ? node->parentElement() : m_document->documentElement());

    // Compose the old list.
    ContainerNode* parentNode = node->parentNode();
    Vector<OwnPtr<Digest> > oldList;
    for (Node* child = parentNode->firstChild(); child; child = child->nextSibling())
        oldList.append(createDigest(child, 0));

    // Compose the new list.
    String markupCopy = markup;
    markupCopy.makeLower();
    Vector<OwnPtr<Digest> > newList;
    for (Node* child = parentNode->firstChild(); child != node; child = child->nextSibling())
        newList.append(createDigest(child, 0));
    for (Node* child = fragment->firstChild(); child; child = child->nextSibling()) {
        if (child->hasTagName(headTag) && !child->firstChild() && markupCopy.find("</head>") == notFound)
            continue; // HTML5 parser inserts empty <head> tag whenever it parses <body>
        if (child->hasTagName(bodyTag) && !child->firstChild() && markupCopy.find("</body>") == notFound)
            continue; // HTML5 parser inserts empty <body> tag whenever it parses </head>
        newList.append(createDigest(child, &m_unusedNodesMap));
    }
    for (Node* child = node->nextSibling(); child; child = child->nextSibling())
        newList.append(createDigest(child, 0));

    if (!innerPatchChildren(parentNode, oldList, newList, ec)) {
        // Fall back to total replace.
        ec = 0;
        if (!m_domEditor->replaceChild(parentNode, fragment.release(), node, ec))
            return 0;
    }
    return previousSibling ? previousSibling->nextSibling() : parentNode->firstChild();
}

bool DOMPatchSupport::innerPatchNode(Digest* oldDigest, Digest* newDigest, ExceptionCode& ec)
{
    if (oldDigest->m_sha1 == newDigest->m_sha1)
        return true;

    Node* oldNode = oldDigest->m_node;
    Node* newNode = newDigest->m_node;

    if (newNode->nodeType() != oldNode->nodeType() || newNode->nodeName() != oldNode->nodeName())
        return m_domEditor->replaceChild(oldNode->parentNode(), newNode, oldNode, ec);

    if (oldNode->nodeValue() != newNode->nodeValue()) {
        if (!m_domEditor->setNodeValue(oldNode, newNode->nodeValue(), ec))
            return false;
    }

    if (oldNode->nodeType() != Node::ELEMENT_NODE)
        return true;

    // Patch attributes
    Element* oldElement = toElement(oldNode);
    Element* newElement = toElement(newNode);
    if (oldDigest->m_attrsSHA1 != newDigest->m_attrsSHA1) {
        // FIXME: Create a function in Element for removing all properties. Take in account whether did/willModifyAttribute are important.
        if (oldElement->hasAttributesWithoutUpdate()) {
            while (oldElement->attributeCount()) {
                const Attribute* attribute = oldElement->attributeItem(0);
                if (!m_domEditor->removeAttribute(oldElement, attribute->localName(), ec))
                    return false;
            }
        }

        // FIXME: Create a function in Element for copying properties. cloneDataFromElement() is close but not enough for this case.
        if (newElement->hasAttributesWithoutUpdate()) {
            size_t numAttrs = newElement->attributeCount();
            for (size_t i = 0; i < numAttrs; ++i) {
                const Attribute* attribute = newElement->attributeItem(i);
                if (!m_domEditor->setAttribute(oldElement, attribute->name().localName(), attribute->value(), ec))
                    return false;
            }
        }
    }

    bool result = innerPatchChildren(oldElement, oldDigest->m_children, newDigest->m_children, ec);
    m_unusedNodesMap.remove(newDigest->m_sha1);
    return result;
}

pair<DOMPatchSupport::ResultMap, DOMPatchSupport::ResultMap>
DOMPatchSupport::diff(const Vector<OwnPtr<Digest> >& oldList, const Vector<OwnPtr<Digest> >& newList)
{
    ResultMap newMap(newList.size());
    ResultMap oldMap(oldList.size());

    for (size_t i = 0; i < oldMap.size(); ++i) {
        oldMap[i].first = 0;
        oldMap[i].second = 0;
    }

    for (size_t i = 0; i < newMap.size(); ++i) {
        newMap[i].first = 0;
        newMap[i].second = 0;
    }

    // Trim head and tail.
    for (size_t i = 0; i < oldList.size() && i < newList.size() && oldList[i]->m_sha1 == newList[i]->m_sha1; ++i) {
        oldMap[i].first = oldList[i].get();
        oldMap[i].second = i;
        newMap[i].first = newList[i].get();
        newMap[i].second = i;
    }
    for (size_t i = 0; i < oldList.size() && i < newList.size() && oldList[oldList.size() - i - 1]->m_sha1 == newList[newList.size() - i - 1]->m_sha1; ++i) {
        size_t oldIndex = oldList.size() - i - 1;
        size_t newIndex = newList.size() - i - 1;
        oldMap[oldIndex].first = oldList[oldIndex].get();
        oldMap[oldIndex].second = newIndex;
        newMap[newIndex].first = newList[newIndex].get();
        newMap[newIndex].second = oldIndex;
    }

    typedef HashMap<String, Vector<size_t> > DiffTable;
    DiffTable newTable;
    DiffTable oldTable;

    for (size_t i = 0; i < newList.size(); ++i) {
        DiffTable::iterator it = newTable.add(newList[i]->m_sha1, Vector<size_t>()).iterator;
        it->value.append(i);
    }

    for (size_t i = 0; i < oldList.size(); ++i) {
        DiffTable::iterator it = oldTable.add(oldList[i]->m_sha1, Vector<size_t>()).iterator;
        it->value.append(i);
    }

    for (DiffTable::iterator newIt = newTable.begin(); newIt != newTable.end(); ++newIt) {
        if (newIt->value.size() != 1)
            continue;

        DiffTable::iterator oldIt = oldTable.find(newIt->key);
        if (oldIt == oldTable.end() || oldIt->value.size() != 1)
            continue;

        newMap[newIt->value[0]] = make_pair(newList[newIt->value[0]].get(), oldIt->value[0]);
        oldMap[oldIt->value[0]] = make_pair(oldList[oldIt->value[0]].get(), newIt->value[0]);
    }

    for (size_t i = 0; newList.size() > 0 && i < newList.size() - 1; ++i) {
        if (!newMap[i].first || newMap[i + 1].first)
            continue;

        size_t j = newMap[i].second + 1;
        if (j < oldMap.size() && !oldMap[j].first && newList[i + 1]->m_sha1 == oldList[j]->m_sha1) {
            newMap[i + 1] = make_pair(newList[i + 1].get(), j);
            oldMap[j] = make_pair(oldList[j].get(), i + 1);
        }
    }

    for (size_t i = newList.size() - 1; newList.size() > 0 && i > 0; --i) {
        if (!newMap[i].first || newMap[i - 1].first || newMap[i].second <= 0)
            continue;

        size_t j = newMap[i].second - 1;
        if (!oldMap[j].first && newList[i - 1]->m_sha1 == oldList[j]->m_sha1) {
            newMap[i - 1] = make_pair(newList[i - 1].get(), j);
            oldMap[j] = make_pair(oldList[j].get(), i - 1);
        }
    }

#ifdef DEBUG_DOM_PATCH_SUPPORT
    dumpMap(oldMap, "OLD");
    dumpMap(newMap, "NEW");
#endif

    return make_pair(oldMap, newMap);
}

bool DOMPatchSupport::innerPatchChildren(ContainerNode* parentNode, const Vector<OwnPtr<Digest> >& oldList, const Vector<OwnPtr<Digest> >& newList, ExceptionCode& ec)
{
    pair<ResultMap, ResultMap> resultMaps = diff(oldList, newList);
    ResultMap& oldMap = resultMaps.first;
    ResultMap& newMap = resultMaps.second;

    Digest* oldHead = 0;
    Digest* oldBody = 0;

    // 1. First strip everything except for the nodes that retain. Collect pending merges.
    HashMap<Digest*, Digest*> merges;
    HashSet<size_t, WTF::IntHash<size_t>, WTF::UnsignedWithZeroKeyHashTraits<size_t> > usedNewOrdinals;
    for (size_t i = 0; i < oldList.size(); ++i) {
        if (oldMap[i].first) {
            if (!usedNewOrdinals.contains(oldMap[i].second)) {
                usedNewOrdinals.add(oldMap[i].second);
                continue;
            }
            oldMap[i].first = 0;
            oldMap[i].second = 0;
        }

        // Always match <head> and <body> tags with each other - we can't remove them from the DOM
        // upon patching.
        if (oldList[i]->m_node->hasTagName(headTag)) {
            oldHead = oldList[i].get();
            continue;
        }
        if (oldList[i]->m_node->hasTagName(bodyTag)) {
            oldBody = oldList[i].get();
            continue;
        }

        // Check if this change is between stable nodes. If it is, consider it as "modified".
        if (!m_unusedNodesMap.contains(oldList[i]->m_sha1) && (!i || oldMap[i - 1].first) && (i == oldMap.size() - 1 || oldMap[i + 1].first)) {
            size_t anchorCandidate = i ? oldMap[i - 1].second + 1 : 0;
            size_t anchorAfter = i == oldMap.size() - 1 ? anchorCandidate + 1 : oldMap[i + 1].second;
            if (anchorAfter - anchorCandidate == 1 && anchorCandidate < newList.size())
                merges.set(newList[anchorCandidate].get(), oldList[i].get());
            else {
                if (!removeChildAndMoveToNew(oldList[i].get(), ec))
                    return false;
            }
        } else {
            if (!removeChildAndMoveToNew(oldList[i].get(), ec))
                return false;
        }
    }

    // Mark retained nodes as used, do not reuse node more than once.
    HashSet<size_t, WTF::IntHash<size_t>, WTF::UnsignedWithZeroKeyHashTraits<size_t> >  usedOldOrdinals;
    for (size_t i = 0; i < newList.size(); ++i) {
        if (!newMap[i].first)
            continue;
        size_t oldOrdinal = newMap[i].second;
        if (usedOldOrdinals.contains(oldOrdinal)) {
            // Do not map node more than once
            newMap[i].first = 0;
            newMap[i].second = 0;
            continue;
        }
        usedOldOrdinals.add(oldOrdinal);
        markNodeAsUsed(newMap[i].first);
    }

    // Mark <head> and <body> nodes for merge.
    if (oldHead || oldBody) {
        for (size_t i = 0; i < newList.size(); ++i) {
            if (oldHead && newList[i]->m_node->hasTagName(headTag))
                merges.set(newList[i].get(), oldHead);
            if (oldBody && newList[i]->m_node->hasTagName(bodyTag))
                merges.set(newList[i].get(), oldBody);
        }
    }

    // 2. Patch nodes marked for merge.
    for (HashMap<Digest*, Digest*>::iterator it = merges.begin(); it != merges.end(); ++it) {
        if (!innerPatchNode(it->value, it->key, ec))
            return false;
    }

    // 3. Insert missing nodes.
    for (size_t i = 0; i < newMap.size(); ++i) {
        if (newMap[i].first || merges.contains(newList[i].get()))
            continue;
        if (!insertBeforeAndMarkAsUsed(parentNode, newList[i].get(), parentNode->childNode(i), ec))
            return false;
    }

    // 4. Then put all nodes that retained into their slots (sort by new index).
    for (size_t i = 0; i < oldMap.size(); ++i) {
        if (!oldMap[i].first)
            continue;
        RefPtr<Node> node = oldMap[i].first->m_node;
        Node* anchorNode = parentNode->childNode(oldMap[i].second);
        if (node.get() == anchorNode)
            continue;
        if (node->hasTagName(bodyTag) || node->hasTagName(headTag))
            continue; // Never move head or body, move the rest of the nodes around them.

        if (!m_domEditor->insertBefore(parentNode, node.release(), anchorNode, ec))
            return false;
    }
    return true;
}

static void addStringToSHA1(SHA1& sha1, const String& string)
{
    CString cString = string.utf8();
    sha1.addBytes(reinterpret_cast<const uint8_t*>(cString.data()), cString.length());
}

PassOwnPtr<DOMPatchSupport::Digest> DOMPatchSupport::createDigest(Node* node, UnusedNodesMap* unusedNodesMap)
{
    Digest* digest = new Digest(node);

    SHA1 sha1;

    Node::NodeType nodeType = node->nodeType();
    sha1.addBytes(reinterpret_cast<const uint8_t*>(&nodeType), sizeof(nodeType));
    addStringToSHA1(sha1, node->nodeName());
    addStringToSHA1(sha1, node->nodeValue());

    if (node->nodeType() == Node::ELEMENT_NODE) {
        Node* child = node->firstChild();
        while (child) {
            OwnPtr<Digest> childInfo = createDigest(child, unusedNodesMap);
            addStringToSHA1(sha1, childInfo->m_sha1);
            child = child->nextSibling();
            digest->m_children.append(childInfo.release());
        }
        Element* element = toElement(node);

        if (element->hasAttributesWithoutUpdate()) {
            size_t numAttrs = element->attributeCount();
            SHA1 attrsSHA1;
            for (size_t i = 0; i < numAttrs; ++i) {
                const Attribute* attribute = element->attributeItem(i);
                addStringToSHA1(attrsSHA1, attribute->name().toString());
                addStringToSHA1(attrsSHA1, attribute->value());
            }
            Vector<uint8_t, 20> attrsHash;
            attrsSHA1.computeHash(attrsHash);
            digest->m_attrsSHA1 = base64Encode(reinterpret_cast<const char*>(attrsHash.data()), 10);
            addStringToSHA1(sha1, digest->m_attrsSHA1);
        }
    }

    Vector<uint8_t, 20> hash;
    sha1.computeHash(hash);
    digest->m_sha1 = base64Encode(reinterpret_cast<const char*>(hash.data()), 10);
    if (unusedNodesMap)
        unusedNodesMap->add(digest->m_sha1, digest);
    return adoptPtr(digest);
}

bool DOMPatchSupport::insertBeforeAndMarkAsUsed(ContainerNode* parentNode, Digest* digest, Node* anchor, ExceptionCode& ec)
{
    bool result = m_domEditor->insertBefore(parentNode, digest->m_node, anchor, ec);
    markNodeAsUsed(digest);
    return result;
}

bool DOMPatchSupport::removeChildAndMoveToNew(Digest* oldDigest, ExceptionCode& ec)
{
    RefPtr<Node> oldNode = oldDigest->m_node;
    if (!m_domEditor->removeChild(oldNode->parentNode(), oldNode.get(), ec))
        return false;

    // Diff works within levels. In order not to lose the node identity when user
    // prepends his HTML with "<div>" (i.e. all nodes are shifted to the next nested level),
    // prior to dropping the original node on the floor, check whether new DOM has a digest
    // with matching sha1. If it does, replace it with the original DOM chunk. Chances are
    // high that it will get merged back into the original DOM during the further patching.
    UnusedNodesMap::iterator it = m_unusedNodesMap.find(oldDigest->m_sha1);
    if (it != m_unusedNodesMap.end()) {
        Digest* newDigest = it->value;
        Node* newNode = newDigest->m_node;
        if (!m_domEditor->replaceChild(newNode->parentNode(), oldNode, newNode, ec))
            return false;
        newDigest->m_node = oldNode.get();
        markNodeAsUsed(newDigest);
        return true;
    }

    for (size_t i = 0; i < oldDigest->m_children.size(); ++i) {
        if (!removeChildAndMoveToNew(oldDigest->m_children[i].get(), ec))
            return false;
    }
    return true;
}

void DOMPatchSupport::markNodeAsUsed(Digest* digest)
{
    Deque<Digest*> queue;
    queue.append(digest);
    while (!queue.isEmpty()) {
        Digest* first = queue.takeFirst();
        m_unusedNodesMap.remove(first->m_sha1);
        for (size_t i = 0; i < first->m_children.size(); ++i)
            queue.append(first->m_children[i].get());
    }
}

#ifdef DEBUG_DOM_PATCH_SUPPORT
static String nodeName(Node* node)
{
    if (node->document()->isXHTMLDocument())
         return node->nodeName();
    return node->nodeName().lower();
}

void DOMPatchSupport::dumpMap(const ResultMap& map, const String& name)
{
    fprintf(stderr, "\n\n");
    for (size_t i = 0; i < map.size(); ++i)
        fprintf(stderr, "%s[%lu]: %s (%p) - [%lu]\n", name.utf8().data(), i, map[i].first ? nodeName(map[i].first->m_node).utf8().data() : "", map[i].first, map[i].second);
}
#endif

} // namespace WebCore

#endif // ENABLE(INSPECTOR)

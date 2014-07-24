/*
 * Copyright (C) 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XPathStep.h"

#include "Attr.h"
#include "Document.h"
#include "Element.h"
#include "NodeTraversal.h"
#include "XMLNSNames.h"
#include "XPathParser.h"
#include "XPathUtil.h"

namespace WebCore {
namespace XPath {

Step::Step(Axis axis, const NodeTest& nodeTest, const Vector<Predicate*>& predicates)
    : m_axis(axis)
    , m_nodeTest(nodeTest)
    , m_predicates(predicates)
{
}

Step::~Step()
{
    deleteAllValues(m_predicates);
    deleteAllValues(m_nodeTest.mergedPredicates());
}

void Step::optimize()
{
    // Evaluate predicates as part of node test if possible to avoid building unnecessary NodeSets.
    // E.g., there is no need to build a set of all "foo" nodes to evaluate "foo[@bar]", we can check the predicate while enumerating.
    // This optimization can be applied to predicates that are not context node list sensitive, or to first predicate that is only context position sensitive, e.g. foo[position() mod 2 = 0].
    Vector<Predicate*> remainingPredicates;
    for (size_t i = 0; i < m_predicates.size(); ++i) {
        Predicate* predicate = m_predicates[i];
        if ((!predicate->isContextPositionSensitive() || m_nodeTest.mergedPredicates().isEmpty()) && !predicate->isContextSizeSensitive() && remainingPredicates.isEmpty()) {
            m_nodeTest.mergedPredicates().append(predicate);
        } else
            remainingPredicates.append(predicate);
    }
    swap(remainingPredicates, m_predicates);
}

void optimizeStepPair(Step* first, Step* second, bool& dropSecondStep)
{
    dropSecondStep = false;

    if (first->m_axis == Step::DescendantOrSelfAxis
        && first->m_nodeTest.kind() == Step::NodeTest::AnyNodeTest
        && !first->m_predicates.size()
        && !first->m_nodeTest.mergedPredicates().size()) {

        ASSERT(first->m_nodeTest.data().isEmpty());
        ASSERT(first->m_nodeTest.namespaceURI().isEmpty());

        // Optimize the common case of "//" AKA /descendant-or-self::node()/child::NodeTest to /descendant::NodeTest.
        if (second->m_axis == Step::ChildAxis && second->predicatesAreContextListInsensitive()) {
            first->m_axis = Step::DescendantAxis;
            first->m_nodeTest = Step::NodeTest(second->m_nodeTest.kind(), second->m_nodeTest.data(), second->m_nodeTest.namespaceURI());
            swap(second->m_nodeTest.mergedPredicates(), first->m_nodeTest.mergedPredicates());
            swap(second->m_predicates, first->m_predicates);
            first->optimize();
            dropSecondStep = true;
        }
    }
}

bool Step::predicatesAreContextListInsensitive() const
{
    for (size_t i = 0; i < m_predicates.size(); ++i) {
        Predicate* predicate = m_predicates[i];
        if (predicate->isContextPositionSensitive() || predicate->isContextSizeSensitive())
            return false;
    }

    for (size_t i = 0; i < m_nodeTest.mergedPredicates().size(); ++i) {
        Predicate* predicate = m_nodeTest.mergedPredicates()[i];
        if (predicate->isContextPositionSensitive() || predicate->isContextSizeSensitive())
            return false;
    }

    return true;
}

void Step::evaluate(Node* context, NodeSet& nodes) const
{
    EvaluationContext& evaluationContext = Expression::evaluationContext();
    evaluationContext.position = 0;

    nodesInAxis(context, nodes);

    // Check predicates that couldn't be merged into node test.
    for (unsigned i = 0; i < m_predicates.size(); i++) {
        Predicate* predicate = m_predicates[i];

        NodeSet newNodes;
        if (!nodes.isSorted())
            newNodes.markSorted(false);

        for (unsigned j = 0; j < nodes.size(); j++) {
            Node* node = nodes[j];

            evaluationContext.node = node;
            evaluationContext.size = nodes.size();
            evaluationContext.position = j + 1;
            if (predicate->evaluate())
                newNodes.append(node);
        }

        nodes.swap(newNodes);
    }
}

static inline Node::NodeType primaryNodeType(Step::Axis axis)
{
    switch (axis) {
        case Step::AttributeAxis:
            return Node::ATTRIBUTE_NODE;
        case Step::NamespaceAxis:
            return Node::XPATH_NAMESPACE_NODE;
        default:
            return Node::ELEMENT_NODE;
    }
}

// Evaluate NodeTest without considering merged predicates.
static inline bool nodeMatchesBasicTest(Node* node, Step::Axis axis, const Step::NodeTest& nodeTest)
{
    switch (nodeTest.kind()) {
        case Step::NodeTest::TextNodeTest:
            return node->nodeType() == Node::TEXT_NODE || node->nodeType() == Node::CDATA_SECTION_NODE;
        case Step::NodeTest::CommentNodeTest:
            return node->nodeType() == Node::COMMENT_NODE;
        case Step::NodeTest::ProcessingInstructionNodeTest: {
            const AtomicString& name = nodeTest.data();
            return node->nodeType() == Node::PROCESSING_INSTRUCTION_NODE && (name.isEmpty() || node->nodeName() == name);
        }
        case Step::NodeTest::AnyNodeTest:
            return true;
        case Step::NodeTest::NameTest: {
            const AtomicString& name = nodeTest.data();
            const AtomicString& namespaceURI = nodeTest.namespaceURI();

            if (axis == Step::AttributeAxis) {
                ASSERT(node->isAttributeNode());

                // In XPath land, namespace nodes are not accessible on the attribute axis.
                if (node->namespaceURI() == XMLNSNames::xmlnsNamespaceURI)
                    return false;

                if (name == starAtom)
                    return namespaceURI.isEmpty() || node->namespaceURI() == namespaceURI;

                return node->localName() == name && node->namespaceURI() == namespaceURI;
            }

            // Node test on the namespace axis is not implemented yet, the caller has a check for it.
            ASSERT(axis != Step::NamespaceAxis);

            // For other axes, the principal node type is element.
            ASSERT(primaryNodeType(axis) == Node::ELEMENT_NODE);
            if (node->nodeType() != Node::ELEMENT_NODE)
                return false;

            if (name == starAtom)
                return namespaceURI.isEmpty() || namespaceURI == node->namespaceURI();

            if (node->document()->isHTMLDocument()) {
                if (node->isHTMLElement()) {
                    // Paths without namespaces should match HTML elements in HTML documents despite those having an XHTML namespace. Names are compared case-insensitively.
                    return equalIgnoringCase(toElement(node)->localName(), name) && (namespaceURI.isNull() || namespaceURI == node->namespaceURI());
                }
                // An expression without any prefix shouldn't match no-namespace nodes (because HTML5 says so).
                return toElement(node)->hasLocalName(name) && namespaceURI == node->namespaceURI() && !namespaceURI.isNull();
            }
            return toElement(node)->hasLocalName(name) && namespaceURI == node->namespaceURI();
        }
    }
    ASSERT_NOT_REACHED();
    return false;
}

static inline bool nodeMatches(Node* node, Step::Axis axis, const Step::NodeTest& nodeTest)
{
    if (!nodeMatchesBasicTest(node, axis, nodeTest))
        return false;

    EvaluationContext& evaluationContext = Expression::evaluationContext();

    // Only the first merged predicate may depend on position.
    ++evaluationContext.position;

    const Vector<Predicate*>& mergedPredicates = nodeTest.mergedPredicates();
    for (unsigned i = 0; i < mergedPredicates.size(); i++) {
        Predicate* predicate = mergedPredicates[i];

        evaluationContext.node = node;
        // No need to set context size - we only get here when evaluating predicates that do not depend on it.
        if (!predicate->evaluate())
            return false;
    }

    return true;
}

// Result nodes are ordered in axis order. Node test (including merged predicates) is applied.
void Step::nodesInAxis(Node* context, NodeSet& nodes) const
{
    ASSERT(nodes.isEmpty());
    switch (m_axis) {
        case ChildAxis:
            if (context->isAttributeNode()) // In XPath model, attribute nodes do not have children.
                return;

            for (Node* n = context->firstChild(); n; n = n->nextSibling())
                if (nodeMatches(n, ChildAxis, m_nodeTest))
                    nodes.append(n);
            return;
        case DescendantAxis:
            if (context->isAttributeNode()) // In XPath model, attribute nodes do not have children.
                return;

            for (Node* n = context->firstChild(); n; n = NodeTraversal::next(n, context))
                if (nodeMatches(n, DescendantAxis, m_nodeTest))
                    nodes.append(n);
            return;
        case ParentAxis:
            if (context->isAttributeNode()) {
                Element* n = static_cast<Attr*>(context)->ownerElement();
                if (nodeMatches(n, ParentAxis, m_nodeTest))
                    nodes.append(n);
            } else {
                ContainerNode* n = context->parentNode();
                if (n && nodeMatches(n, ParentAxis, m_nodeTest))
                    nodes.append(n);
            }
            return;
        case AncestorAxis: {
            Node* n = context;
            if (context->isAttributeNode()) {
                n = static_cast<Attr*>(context)->ownerElement();
                if (nodeMatches(n, AncestorAxis, m_nodeTest))
                    nodes.append(n);
            }
            for (n = n->parentNode(); n; n = n->parentNode())
                if (nodeMatches(n, AncestorAxis, m_nodeTest))
                    nodes.append(n);
            nodes.markSorted(false);
            return;
        }
        case FollowingSiblingAxis:
            if (context->nodeType() == Node::ATTRIBUTE_NODE ||
                 context->nodeType() == Node::XPATH_NAMESPACE_NODE) 
                return;
            
            for (Node* n = context->nextSibling(); n; n = n->nextSibling())
                if (nodeMatches(n, FollowingSiblingAxis, m_nodeTest))
                    nodes.append(n);
            return;
        case PrecedingSiblingAxis:
            if (context->nodeType() == Node::ATTRIBUTE_NODE ||
                 context->nodeType() == Node::XPATH_NAMESPACE_NODE)
                return;
            
            for (Node* n = context->previousSibling(); n; n = n->previousSibling())
                if (nodeMatches(n, PrecedingSiblingAxis, m_nodeTest))
                    nodes.append(n);

            nodes.markSorted(false);
            return;
        case FollowingAxis:
            if (context->isAttributeNode()) {
                Node* p = static_cast<Attr*>(context)->ownerElement();
                while ((p = NodeTraversal::next(p)))
                    if (nodeMatches(p, FollowingAxis, m_nodeTest))
                        nodes.append(p);
            } else {
                for (Node* p = context; !isRootDomNode(p); p = p->parentNode()) {
                    for (Node* n = p->nextSibling(); n; n = n->nextSibling()) {
                        if (nodeMatches(n, FollowingAxis, m_nodeTest))
                            nodes.append(n);
                        for (Node* c = n->firstChild(); c; c = NodeTraversal::next(c, n))
                            if (nodeMatches(c, FollowingAxis, m_nodeTest))
                                nodes.append(c);
                    }
                }
            }
            return;
        case PrecedingAxis: {
            if (context->isAttributeNode())
                context = static_cast<Attr*>(context)->ownerElement();

            Node* n = context;
            while (ContainerNode* parent = n->parentNode()) {
                for (n = NodeTraversal::previous(n); n != parent; n = NodeTraversal::previous(n))
                    if (nodeMatches(n, PrecedingAxis, m_nodeTest))
                        nodes.append(n);
                n = parent;
            }
            nodes.markSorted(false);
            return;
        }
        case AttributeAxis: {
            if (!context->isElementNode())
                return;

            Element* contextElement = toElement(context);

            // Avoid lazily creating attribute nodes for attributes that we do not need anyway.
            if (m_nodeTest.kind() == NodeTest::NameTest && m_nodeTest.data() != starAtom) {
                RefPtr<Node> n = contextElement->getAttributeNodeNS(m_nodeTest.namespaceURI(), m_nodeTest.data());
                if (n && n->namespaceURI() != XMLNSNames::xmlnsNamespaceURI) { // In XPath land, namespace nodes are not accessible on the attribute axis.
                    if (nodeMatches(n.get(), AttributeAxis, m_nodeTest)) // Still need to check merged predicates.
                        nodes.append(n.release());
                }
                return;
            }
            
            if (!contextElement->hasAttributes())
                return;

            for (unsigned i = 0; i < contextElement->attributeCount(); ++i) {
                RefPtr<Attr> attr = contextElement->ensureAttr(contextElement->attributeItem(i)->name());
                if (nodeMatches(attr.get(), AttributeAxis, m_nodeTest))
                    nodes.append(attr.release());
            }
            return;
        }
        case NamespaceAxis:
            // XPath namespace nodes are not implemented yet.
            return;
        case SelfAxis:
            if (nodeMatches(context, SelfAxis, m_nodeTest))
                nodes.append(context);
            return;
        case DescendantOrSelfAxis:
            if (nodeMatches(context, DescendantOrSelfAxis, m_nodeTest))
                nodes.append(context);
            if (context->isAttributeNode()) // In XPath model, attribute nodes do not have children.
                return;

            for (Node* n = context->firstChild(); n; n = NodeTraversal::next(n, context))
            if (nodeMatches(n, DescendantOrSelfAxis, m_nodeTest))
                nodes.append(n);
            return;
        case AncestorOrSelfAxis: {
            if (nodeMatches(context, AncestorOrSelfAxis, m_nodeTest))
                nodes.append(context);
            Node* n = context;
            if (context->isAttributeNode()) {
                n = static_cast<Attr*>(context)->ownerElement();
                if (nodeMatches(n, AncestorOrSelfAxis, m_nodeTest))
                    nodes.append(n);
            }
            for (n = n->parentNode(); n; n = n->parentNode())
                if (nodeMatches(n, AncestorOrSelfAxis, m_nodeTest))
                    nodes.append(n);

            nodes.markSorted(false);
            return;
        }
    }
    ASSERT_NOT_REACHED();
}


}
}

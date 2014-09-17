/*
 * Copyright (C) 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006, 2009 Apple Inc. All rights reserved.
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
#include "XPathResult.h"

#if ENABLE(XPATH)

#include "Document.h"
#include "Node.h"
#include "ExceptionCode.h"
#include "XPathEvaluator.h"
#include "XPathException.h"

#if COMPILER(WINSCW)
#define BOOL_TO_VALUE_CAST (unsigned long)
#else
#define BOOL_TO_VALUE_CAST
#endif

namespace WebCore {

using namespace XPath;

XPathResult::XPathResult(Document* document, const Value& value)
    : m_value(value)
    , m_nodeSetPosition(0)
    , m_domTreeVersion(0)
{
    switch (m_value.type()) {
        case Value::BooleanValue:
            m_resultType = BOOLEAN_TYPE;
            return;
        case Value::NumberValue:
            m_resultType = NUMBER_TYPE;
            return;
        case Value::StringValue:
            m_resultType = STRING_TYPE;
            return;
        case Value::NodeSetValue:
            m_resultType = UNORDERED_NODE_ITERATOR_TYPE;
            m_nodeSetPosition = 0;
            m_nodeSet = m_value.toNodeSet();
            m_document = document;
            m_domTreeVersion = document->domTreeVersion();
            return;
    }
    ASSERT_NOT_REACHED();
}

XPathResult::~XPathResult()
{
}

void XPathResult::convertTo(unsigned short type, ExceptionCode& ec)
{
    switch (type) {
        case ANY_TYPE:
            break;
        case NUMBER_TYPE:
            m_resultType = type;
            m_value = m_value.toNumber();
            break;
        case STRING_TYPE:
            m_resultType = type;
            m_value = m_value.toString();
            break;
        case BOOLEAN_TYPE:
            m_resultType = type;
            m_value = BOOL_TO_VALUE_CAST (m_value.toBoolean());
            break;
        case UNORDERED_NODE_ITERATOR_TYPE:
        case UNORDERED_NODE_SNAPSHOT_TYPE:
        case ANY_UNORDERED_NODE_TYPE:
        case FIRST_ORDERED_NODE_TYPE: // This is correct - singleNodeValue() will take care of ordering.
            if (!m_value.isNodeSet()) {
                ec = XPathException::TYPE_ERR;
                return;
            }
            m_resultType = type;
            break;
        case ORDERED_NODE_ITERATOR_TYPE:
            if (!m_value.isNodeSet()) {
                ec = XPathException::TYPE_ERR;
                return;
            }
            m_nodeSet.sort();
            m_resultType = type;
            break;
        case ORDERED_NODE_SNAPSHOT_TYPE:
            if (!m_value.isNodeSet()) {
                ec = XPathException::TYPE_ERR;
                return;
            }
            m_value.toNodeSet().sort();
            m_resultType = type;
            break;
    }
}

unsigned short XPathResult::resultType() const
{
    return m_resultType;
}

double XPathResult::numberValue(ExceptionCode& ec) const
{
    if (resultType() != NUMBER_TYPE) {
        ec = XPathException::TYPE_ERR;
        return 0.0;
    }
    return m_value.toNumber();
}

String XPathResult::stringValue(ExceptionCode& ec) const
{
    if (resultType() != STRING_TYPE) {
        ec = XPathException::TYPE_ERR;
        return String();
    }
    return m_value.toString();
}

bool XPathResult::booleanValue(ExceptionCode& ec) const
{
    if (resultType() != BOOLEAN_TYPE) {
        ec = XPathException::TYPE_ERR;
        return false;
    }
    return m_value.toBoolean();
}

Node* XPathResult::singleNodeValue(ExceptionCode& ec) const
{
    if (resultType() != ANY_UNORDERED_NODE_TYPE && resultType() != FIRST_ORDERED_NODE_TYPE) {
        ec = XPathException::TYPE_ERR;
        return 0;
    }
  
    const NodeSet& nodes = m_value.toNodeSet();
    if (resultType() == FIRST_ORDERED_NODE_TYPE)
        return nodes.firstNode();
    else
        return nodes.anyNode();
}

bool XPathResult::invalidIteratorState() const
{
    if (resultType() != UNORDERED_NODE_ITERATOR_TYPE && resultType() != ORDERED_NODE_ITERATOR_TYPE)
        return false;

    ASSERT(m_document);
    return m_document->domTreeVersion() != m_domTreeVersion;
}

unsigned long XPathResult::snapshotLength(ExceptionCode& ec) const
{
    if (resultType() != UNORDERED_NODE_SNAPSHOT_TYPE && resultType() != ORDERED_NODE_SNAPSHOT_TYPE) {
        ec = XPathException::TYPE_ERR;
        return 0;
    }

    return m_value.toNodeSet().size();
}

Node* XPathResult::iterateNext(ExceptionCode& ec)
{
    if (resultType() != UNORDERED_NODE_ITERATOR_TYPE && resultType() != ORDERED_NODE_ITERATOR_TYPE) {
        ec = XPathException::TYPE_ERR;
        return 0;
    }
    
    if (invalidIteratorState()) {
        ec = INVALID_STATE_ERR;
        return 0;
    }
    
    if (m_nodeSetPosition + 1 > m_nodeSet.size())
        return 0;

    Node* node = m_nodeSet[m_nodeSetPosition];
    
    m_nodeSetPosition++;

    return node;
}

Node* XPathResult::snapshotItem(unsigned long index, ExceptionCode& ec)
{
    if (resultType() != UNORDERED_NODE_SNAPSHOT_TYPE && resultType() != ORDERED_NODE_SNAPSHOT_TYPE) {
        ec = XPathException::TYPE_ERR;
        return 0;
    }
    
    const NodeSet& nodes = m_value.toNodeSet();
    if (index >= nodes.size())
        return 0;
    
    return nodes[index];
}

}

#endif // ENABLE(XPATH)

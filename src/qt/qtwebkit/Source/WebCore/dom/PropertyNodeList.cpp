/*
 * Copyright (c) 2012 Motorola Mobility, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the name of Motorola Mobility, Inc. nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MICRODATA)
#include "PropertyNodeList.h"

#include "DOMSettableTokenList.h"
#include "DOMStringList.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "NodeRareData.h"

namespace WebCore {

using namespace HTMLNames;

PropertyNodeList::PropertyNodeList(Node* rootNode, const String& name)
    : LiveNodeList(rootNode, PropertyNodeListType, InvalidateOnItemAttrChange, NodeListIsRootedAtDocumentIfOwnerHasItemrefAttr)
    , m_name(name)
{
}

PropertyNodeList::~PropertyNodeList()
{
    ownerNode()->nodeLists()->removeCacheWithName(this, PropertyNodeListType, m_name);
}

bool PropertyNodeList::elementIsPropertyOfRefElement(const Node* testElement, const Node *refElement) const
{
    for (const ContainerNode* node = testElement->parentNode(); node; node = node->parentNode()) {
        if (node->isHTMLElement() && toHTMLElement(node)->fastHasAttribute(itemscopeAttr) && node != ownerNode())
            return false;

        if (node == refElement)
            return true;
    }
    return false;
}

void PropertyNodeList::updateRefElements() const
{
    if (isItemRefElementsCacheValid())
        return;

    m_itemRefElementsCache.clear();
    setItemRefElementsCacheValid();
    toHTMLElement(ownerNode())->getItemRefElements(m_itemRefElementsCache);
}

bool PropertyNodeList::nodeMatches(Element* testElement) const
{
    if (!testElement->isHTMLElement() || !testElement->fastHasAttribute(itempropAttr) || testElement == ownerNode())
        return false;

    for (unsigned i = 0; i < m_itemRefElementsCache.size(); ++i) {
        if (testElement == m_itemRefElementsCache[i] || elementIsPropertyOfRefElement(testElement, m_itemRefElementsCache[i])) {
            if (testElement->itemProp()->tokens().contains(m_name))
                return true;
        }
    }

    return false;
}

PropertyValueArray PropertyNodeList::getValues() const
{
    PropertyValueArray propertyValue;

    for (unsigned offset = 0; Node* node = item(offset); ++offset)
        propertyValue.append(toHTMLElement(node)->itemValue());

    return propertyValue;
}

} // namespace WebCore

#endif // ENABLE(MICRODATA)

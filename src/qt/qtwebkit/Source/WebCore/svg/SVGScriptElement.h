/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SVGScriptElement_h
#define SVGScriptElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedString.h"
#include "SVGElement.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGNames.h"
#include "SVGURIReference.h"
#include "ScriptElement.h"

namespace WebCore {

class SVGScriptElement FINAL : public SVGElement
                             , public SVGURIReference
                             , public SVGExternalResourcesRequired
                             , public ScriptElement {
public:
    static PassRefPtr<SVGScriptElement> create(const QualifiedName&, Document*, bool wasInsertedByParser);

    String type() const;
    void setType(const String&);

private:
    SVGScriptElement(const QualifiedName&, Document*, bool wasInsertedByParser, bool alreadyStarted);

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual void svgAttributeChanged(const QualifiedName&);
    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;
    virtual void finishParsingChildren();

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    virtual bool haveLoadedRequiredResources() { return SVGExternalResourcesRequired::haveLoadedRequiredResources(); }

    virtual String sourceAttributeValue() const;
    virtual String charsetAttributeValue() const;
    virtual String typeAttributeValue() const;
    virtual String languageAttributeValue() const;
    virtual String forAttributeValue() const;
    virtual String eventAttributeValue() const;
    virtual bool asyncAttributeValue() const;
    virtual bool deferAttributeValue() const;
    virtual bool hasSourceAttribute() const;

    virtual void dispatchLoadEvent() { SVGExternalResourcesRequired::dispatchLoadEvent(this); }

    virtual PassRefPtr<Element> cloneElementWithoutAttributesAndChildren();

    // SVGExternalResourcesRequired
    virtual void setHaveFiredLoadEvent(bool haveFiredLoadEvent) { ScriptElement::setHaveFiredLoadEvent(haveFiredLoadEvent); }
    virtual bool isParserInserted() const { return ScriptElement::isParserInserted(); }
    virtual bool haveFiredLoadEvent() const { return ScriptElement::haveFiredLoadEvent(); }
    virtual Timer<SVGElement>* svgLoadEventTimer() OVERRIDE { return &m_svgLoadEventTimer; }

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGScriptElement)
        DECLARE_ANIMATED_STRING(Href, href)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    String m_type;
    Timer<SVGElement> m_svgLoadEventTimer;
};

inline bool isSVGScriptElement(Node* node)
{
    return node->hasTagName(SVGNames::scriptTag);
}

inline SVGScriptElement* toSVGScriptElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || node->hasTagName(SVGNames::scriptTag));
    return static_cast<SVGScriptElement*>(node);
}

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

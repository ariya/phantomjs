/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2010 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
 *
 */

#include "config.h"
#include "HTMLKeygenElement.h"

#include "Attribute.h"
#include "Document.h"
#include "FormDataList.h"
#include "HTMLNames.h"
#include "HTMLSelectElement.h"
#include "HTMLOptionElement.h"
#include "SSLKeyGenerator.h"
#include "ShadowRoot.h"
#include "Text.h"
#include <wtf/StdLibExtras.h>

using namespace WebCore;

namespace WebCore {

using namespace HTMLNames;

class KeygenSelectElement : public HTMLSelectElement {
public:
    static PassRefPtr<KeygenSelectElement> create(Document* document)
    {
        return adoptRef(new KeygenSelectElement(document));
    }

    virtual const AtomicString& shadowPseudoId() const
    {
        DEFINE_STATIC_LOCAL(AtomicString, pseudoId, ("-webkit-keygen-select"));
        return pseudoId;
    }

protected:
    KeygenSelectElement(Document* document)
        : HTMLSelectElement(selectTag, document, 0)
    {
    }
};

inline HTMLKeygenElement::HTMLKeygenElement(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
    : HTMLFormControlElementWithState(tagName, document, form)
{
    ASSERT(hasTagName(keygenTag));

    // Create a select element with one option element for each key size.
    Vector<String> keys;
    getSupportedKeySizes(keys);

    RefPtr<HTMLSelectElement> select = KeygenSelectElement::create(document);
    ExceptionCode ec = 0;
    for (size_t i = 0; i < keys.size(); ++i) {
        RefPtr<HTMLOptionElement> option = HTMLOptionElement::create(document, this->form());
        select->appendChild(option, ec);
        option->appendChild(Text::create(document, keys[i]), ec);
    }

    ensureShadowRoot()->appendChild(select, ec);
}

PassRefPtr<HTMLKeygenElement> HTMLKeygenElement::create(const QualifiedName& tagName, Document* document, HTMLFormElement* form)
{
    return adoptRef(new HTMLKeygenElement(tagName, document, form));
}

void HTMLKeygenElement::parseMappedAttribute(Attribute* attr)
{
    // Reflect disabled attribute on the shadow select element
    if (attr->name() == disabledAttr)
        shadowSelect()->setAttribute(attr->name(), attr->value());

    if (attr->name() == challengeAttr)
        m_challenge = attr->value();
    else if (attr->name() == keytypeAttr)
        m_keyType = attr->value();
    else
        HTMLFormControlElement::parseMappedAttribute(attr);
}

bool HTMLKeygenElement::appendFormData(FormDataList& encoded_values, bool)
{
    // Only RSA is supported at this time.
    if (!m_keyType.isNull() && !equalIgnoringCase(m_keyType, "rsa"))
        return false;
    String value = signedPublicKeyAndChallengeString(shadowSelect()->selectedIndex(), m_challenge, document()->baseURL());
    if (value.isNull())
        return false;
    encoded_values.appendData(name(), value.utf8());
    return true;
}

const AtomicString& HTMLKeygenElement::formControlType() const
{
    DEFINE_STATIC_LOCAL(const AtomicString, keygen, ("keygen"));
    return keygen;
}

void HTMLKeygenElement::reset()
{
    static_cast<HTMLFormControlElement*>(shadowSelect())->reset();
}

HTMLSelectElement* HTMLKeygenElement::shadowSelect() const
{
    Node* shadow = shadowRoot();
    ASSERT(shadow);
    return shadow ? static_cast<HTMLSelectElement*>(shadow->firstChild()) : 0;
}

} // namespace

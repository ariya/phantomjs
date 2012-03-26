/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(VIDEO_TRACK)
#include "HTMLTrackElement.h"

#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "Logging.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

inline HTMLTrackElement::HTMLTrackElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
{
    LOG(Media, "HTMLTrackElement::HTMLTrackElement - %p", this);
    ASSERT(hasTagName(trackTag));
}

PassRefPtr<HTMLTrackElement> HTMLTrackElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLTrackElement(tagName, document));
}

void HTMLTrackElement::insertedIntoTree(bool deep)
{
    HTMLElement::insertedIntoTree(deep);
    Element* parent = parentElement();
    if (parent && parent->isMediaElement()) {
        // TODO(annacc):
        // static_cast<HTMLMediaElement*>(parentNode())->trackWasAdded(this);
    }
}

void HTMLTrackElement::willRemove()
{
    Element* parent = parentElement();
    if (parent && parent->isMediaElement()) {
        // TODO(annacc):
        // static_cast<HTMLMediaElement*>(parentNode())->trackWillBeRemoved(this);
    }
    HTMLElement::willRemove();
}

KURL HTMLTrackElement::src() const
{
    return document()->completeURL(getAttribute(srcAttr));
}

void HTMLTrackElement::setSrc(const String& url)
{
    setAttribute(srcAttr, url);
}

String HTMLTrackElement::kind() const
{
    return getAttribute(kindAttr);
}

void HTMLTrackElement::setKind(const String& kind)
{
    setAttribute(kindAttr, kind);
}

String HTMLTrackElement::srclang() const
{
    return getAttribute(srclangAttr);
}

void HTMLTrackElement::setSrclang(const String& srclang)
{
    setAttribute(srclangAttr, srclang);
}

String HTMLTrackElement::label() const
{
    return getAttribute(labelAttr);
}

void HTMLTrackElement::setLabel(const String& label)
{
    setAttribute(labelAttr, label);
}

bool HTMLTrackElement::isDefault() const
{
    return hasAttribute(defaultAttr);
}

void HTMLTrackElement::setIsDefault(bool isDefault)
{
    setBooleanAttribute(defaultAttr, isDefault);
}

bool HTMLTrackElement::isURLAttribute(Attribute* attribute) const
{
    return attribute->name() == srcAttr;
}

}

#endif

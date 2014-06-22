/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#if ENABLE(PROGRESS_ELEMENT)
#include "HTMLProgressElement.h"

#include "Attribute.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "HTMLDivElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "NodeRenderingContext.h"
#include "ProgressShadowElement.h"
#include "RenderProgress.h"
#include "ShadowRoot.h"

namespace WebCore {

using namespace HTMLNames;

const double HTMLProgressElement::IndeterminatePosition = -1;
const double HTMLProgressElement::InvalidPosition = -2;

HTMLProgressElement::HTMLProgressElement(const QualifiedName& tagName, Document* document)
    : LabelableElement(tagName, document)
    , m_value(0)
{
    ASSERT(hasTagName(progressTag));
}

HTMLProgressElement::~HTMLProgressElement()
{
}

PassRefPtr<HTMLProgressElement> HTMLProgressElement::create(const QualifiedName& tagName, Document* document)
{
    RefPtr<HTMLProgressElement> progress = adoptRef(new HTMLProgressElement(tagName, document));
    progress->ensureUserAgentShadowRoot();
    return progress.release();
}

RenderObject* HTMLProgressElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    if (!style->hasAppearance() || hasAuthorShadowRoot())
        return RenderObject::createObject(this, style);

    return new (arena) RenderProgress(this);
}

bool HTMLProgressElement::childShouldCreateRenderer(const NodeRenderingContext& childContext) const
{
    return childContext.isOnUpperEncapsulationBoundary() && HTMLElement::childShouldCreateRenderer(childContext);
}

RenderProgress* HTMLProgressElement::renderProgress() const
{
    if (renderer() && renderer()->isProgress())
        return static_cast<RenderProgress*>(renderer());

    RenderObject* renderObject = userAgentShadowRoot()->firstChild()->renderer();
    ASSERT_WITH_SECURITY_IMPLICATION(!renderObject || renderObject->isProgress());
    return static_cast<RenderProgress*>(renderObject);
}

void HTMLProgressElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == valueAttr)
        didElementStateChange();
    else if (name == maxAttr)
        didElementStateChange();
    else
        LabelableElement::parseAttribute(name, value);
}

void HTMLProgressElement::attach(const AttachContext& context)
{
    LabelableElement::attach(context);
    if (RenderProgress* render = renderProgress())
        render->updateFromElement();
}

double HTMLProgressElement::value() const
{
    double value = parseToDoubleForNumberType(fastGetAttribute(valueAttr));
    return !std::isfinite(value) || value < 0 ? 0 : std::min(value, max());
}

void HTMLProgressElement::setValue(double value, ExceptionCode& ec)
{
    if (!std::isfinite(value)) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
    setAttribute(valueAttr, String::number(value >= 0 ? value : 0));
}

double HTMLProgressElement::max() const
{
    double max = parseToDoubleForNumberType(getAttribute(maxAttr));
    return !std::isfinite(max) || max <= 0 ? 1 : max;
}

void HTMLProgressElement::setMax(double max, ExceptionCode& ec)
{
    if (!std::isfinite(max)) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
    setAttribute(maxAttr, String::number(max > 0 ? max : 1));
}

double HTMLProgressElement::position() const
{
    if (!isDeterminate())
        return HTMLProgressElement::IndeterminatePosition;
    return value() / max();
}

bool HTMLProgressElement::isDeterminate() const
{
    return fastHasAttribute(valueAttr);
}
    
void HTMLProgressElement::didElementStateChange()
{
    m_value->setWidthPercentage(position() * 100);
    if (RenderProgress* render = renderProgress()) {
        bool wasDeterminate = render->isDeterminate();
        render->updateFromElement();
        if (wasDeterminate != isDeterminate())
            didAffectSelector(AffectedSelectorIndeterminate);
    }
}

void HTMLProgressElement::didAddUserAgentShadowRoot(ShadowRoot* root)
{
    ASSERT(!m_value);

    RefPtr<ProgressInnerElement> inner = ProgressInnerElement::create(document());
    root->appendChild(inner);

    RefPtr<ProgressBarElement> bar = ProgressBarElement::create(document());
    RefPtr<ProgressValueElement> value = ProgressValueElement::create(document());
    m_value = value.get();
    m_value->setWidthPercentage(HTMLProgressElement::IndeterminatePosition * 100);
    bar->appendChild(m_value, ASSERT_NO_EXCEPTION);

    inner->appendChild(bar, ASSERT_NO_EXCEPTION);
}

bool HTMLProgressElement::shouldAppearIndeterminate() const
{
    return !isDeterminate();
}

} // namespace
#endif

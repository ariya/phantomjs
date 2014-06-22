/*
 * Copyright (C) 2007 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "RemoveFormatCommand.h"

#include "ApplyStyleCommand.h"
#include "CSSValueKeywords.h"
#include "EditingStyle.h"
#include "Element.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "HTMLNames.h"
#include "StylePropertySet.h"

namespace WebCore {

using namespace HTMLNames;

RemoveFormatCommand::RemoveFormatCommand(Document* document)
    : CompositeEditCommand(document)
{
}

static bool isElementForRemoveFormatCommand(const Element* element)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, elements, ());
    if (elements.isEmpty()) {
        elements.add(acronymTag);
        elements.add(bTag);
        elements.add(bdoTag);
        elements.add(bigTag);
        elements.add(citeTag);
        elements.add(codeTag);
        elements.add(dfnTag);
        elements.add(emTag);
        elements.add(fontTag);
        elements.add(iTag);
        elements.add(insTag);
        elements.add(kbdTag);
        elements.add(nobrTag);
        elements.add(qTag);
        elements.add(sTag);
        elements.add(sampTag);
        elements.add(smallTag);
        elements.add(strikeTag);
        elements.add(strongTag);
        elements.add(subTag);
        elements.add(supTag);
        elements.add(ttTag);
        elements.add(uTag);
        elements.add(varTag);
    }
    return elements.contains(element->tagQName());
}

void RemoveFormatCommand::doApply()
{
    Frame* frame = document()->frame();

    if (!frame->selection()->selection().isNonOrphanedCaretOrRange())
        return;

    // Get the default style for this editable root, it's the style that we'll give the
    // content that we're operating on.
    Node* root = frame->selection()->rootEditableElement();
    RefPtr<EditingStyle> defaultStyle = EditingStyle::create(root);

    // We want to remove everything but transparent background.
    // FIXME: We shouldn't access style().
    defaultStyle->style()->setProperty(CSSPropertyBackgroundColor, CSSValueTransparent);

    applyCommandToComposite(ApplyStyleCommand::create(document(), defaultStyle.get(), isElementForRemoveFormatCommand, editingAction()));
}

}

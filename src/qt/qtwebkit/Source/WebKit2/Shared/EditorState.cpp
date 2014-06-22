/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EditorState.h"

#include "Arguments.h"
#include "WebCoreArgumentCoders.h"

namespace WebKit {

void EditorState::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << shouldIgnoreCompositionSelectionChange;
    encoder << selectionIsNone;
    encoder << selectionIsRange;
    encoder << isContentEditable;
    encoder << isContentRichlyEditable;
    encoder << isInPasswordField;
    encoder << isInPlugin;
    encoder << hasComposition;

#if PLATFORM(QT)
    encoder << cursorPosition;
    encoder << anchorPosition;
    encoder << editorRect;
    encoder << compositionRect;
    encoder << inputMethodHints;
    encoder << selectedText;
    encoder << surroundingText;
#endif

#if PLATFORM(QT) || PLATFORM(GTK)
    encoder << cursorRect;
#endif
}

bool EditorState::decode(CoreIPC::ArgumentDecoder& decoder, EditorState& result)
{
    if (!decoder.decode(result.shouldIgnoreCompositionSelectionChange))
        return false;

    if (!decoder.decode(result.selectionIsNone))
        return false;

    if (!decoder.decode(result.selectionIsRange))
        return false;

    if (!decoder.decode(result.isContentEditable))
        return false;

    if (!decoder.decode(result.isContentRichlyEditable))
        return false;

    if (!decoder.decode(result.isInPasswordField))
        return false;

    if (!decoder.decode(result.isInPlugin))
        return false;

    if (!decoder.decode(result.hasComposition))
        return false;

#if PLATFORM(QT)
    if (!decoder.decode(result.cursorPosition))
        return false;

    if (!decoder.decode(result.anchorPosition))
        return false;

    if (!decoder.decode(result.editorRect))
        return false;

    if (!decoder.decode(result.compositionRect))
        return false;

    if (!decoder.decode(result.inputMethodHints))
        return false;

    if (!decoder.decode(result.selectedText))
        return false;

    if (!decoder.decode(result.surroundingText))
        return false;
#endif

#if PLATFORM(QT) || PLATFORM(GTK)
    if (!decoder.decode(result.cursorRect))
        return false;
#endif

    return true;
}

}

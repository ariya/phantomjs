/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "WebEditCommandProxy.h"

#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <WebCore/LocalizedStrings.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

WebEditCommandProxy::WebEditCommandProxy(uint64_t commandID, WebCore::EditAction editAction, WebPageProxy* page)
    : m_commandID(commandID)
    , m_editAction(editAction)
    , m_page(page)
{
    m_page->addEditCommand(this);
}

WebEditCommandProxy::~WebEditCommandProxy()
{
    if (m_page)
        m_page->removeEditCommand(this);
}

void WebEditCommandProxy::unapply()
{
    if (!m_page || !m_page->isValid())
        return;

    m_page->process()->send(Messages::WebPage::UnapplyEditCommand(m_commandID), m_page->pageID(), CoreIPC::DispatchMessageEvenWhenWaitingForSyncReply);
    m_page->registerEditCommand(this, WebPageProxy::Redo);
}

void WebEditCommandProxy::reapply()
{
    if (!m_page || !m_page->isValid())
        return;

    m_page->process()->send(Messages::WebPage::ReapplyEditCommand(m_commandID), m_page->pageID(), CoreIPC::DispatchMessageEvenWhenWaitingForSyncReply);
    m_page->registerEditCommand(this, WebPageProxy::Undo);
}

String WebEditCommandProxy::nameForEditAction(EditAction editAction)
{
    switch (editAction) {
    case EditActionUnspecified:
        return String();
    case EditActionSetColor:
        return WEB_UI_STRING_KEY("Set Color", "Set Color (Undo action name)", "Undo action name");
    case EditActionSetBackgroundColor:
        return WEB_UI_STRING_KEY("Set Background Color", "Set Background Color (Undo action name)", "Undo action name");
    case EditActionTurnOffKerning:
        return WEB_UI_STRING_KEY("Turn Off Kerning", "Turn Off Kerning (Undo action name)", "Undo action name");
    case EditActionTightenKerning:
        return WEB_UI_STRING_KEY("Tighten Kerning", "Tighten Kerning (Undo action name)", "Undo action name");
    case EditActionLoosenKerning:
        return WEB_UI_STRING_KEY("Loosen Kerning", "Loosen Kerning (Undo action name)", "Undo action name");
    case EditActionUseStandardKerning:
        return WEB_UI_STRING_KEY("Use Standard Kerning", "Use Standard Kerning (Undo action name)", "Undo action name");
    case EditActionTurnOffLigatures:
        return WEB_UI_STRING_KEY("Turn Off Ligatures", "Turn Off Ligatures (Undo action name)", "Undo action name");
    case EditActionUseStandardLigatures:
        return WEB_UI_STRING_KEY("Use Standard Ligatures", "Use Standard Ligatures (Undo action name)", "Undo action name");
    case EditActionUseAllLigatures:
        return WEB_UI_STRING_KEY("Use All Ligatures", "Use All Ligatures (Undo action name)", "Undo action name");
    case EditActionRaiseBaseline:
        return WEB_UI_STRING_KEY("Raise Baseline", "Raise Baseline (Undo action name)", "Undo action name");
    case EditActionLowerBaseline:
        return WEB_UI_STRING_KEY("Lower Baseline", "Lower Baseline (Undo action name)", "Undo action name");
    case EditActionSetTraditionalCharacterShape:
        return WEB_UI_STRING_KEY("Set Traditional Character Shape", "Set Traditional Character Shape (Undo action name)", "Undo action name");
    case EditActionSetFont:
        return WEB_UI_STRING_KEY("Set Font", "Set Font (Undo action name)", "Undo action name");
    case EditActionChangeAttributes:
        return WEB_UI_STRING_KEY("Change Attributes", "Change Attributes (Undo action name)", "Undo action name");
    case EditActionAlignLeft:
        return WEB_UI_STRING_KEY("Align Left", "Align Left (Undo action name)", "Undo action name");
    case EditActionAlignRight:
        return WEB_UI_STRING_KEY("Align Right", "Align Right (Undo action name)", "Undo action name");
    case EditActionCenter:
        return WEB_UI_STRING_KEY("Center", "Center (Undo action name)", "Undo action name");
    case EditActionJustify:
        return WEB_UI_STRING_KEY("Justify", "Justify (Undo action name)", "Undo action name");
    case EditActionSetWritingDirection:
        return WEB_UI_STRING_KEY("Set Writing Direction", "Set Writing Direction (Undo action name)", "Undo action name");
    case EditActionSubscript:
        return WEB_UI_STRING_KEY("Subscript", "Subscript (Undo action name)", "Undo action name");
    case EditActionSuperscript:
        return WEB_UI_STRING_KEY("Superscript", "Superscript (Undo action name)", "Undo action name");
    case EditActionUnderline:
        return WEB_UI_STRING_KEY("Underline", "Underline (Undo action name)", "Undo action name");
    case EditActionOutline:
        return WEB_UI_STRING_KEY("Outline", "Outline (Undo action name)", "Undo action name");
    case EditActionUnscript:
        return WEB_UI_STRING_KEY("Unscript", "Unscript (Undo action name)", "Undo action name");
    case EditActionDrag:
        return WEB_UI_STRING_KEY("Drag", "Drag (Undo action name)", "Undo action name");
    case EditActionCut:
        return WEB_UI_STRING_KEY("Cut", "Cut (Undo action name)", "Undo action name");
    case EditActionBold:
        return WEB_UI_STRING_KEY("Bold", "Bold (Undo action name)", "Undo action name");
    case EditActionItalics:
        return WEB_UI_STRING_KEY("Italics", "Italics (Undo action name)", "Undo action name");
    case EditActionPaste:
        return WEB_UI_STRING_KEY("Paste", "Paste (Undo action name)", "Undo action name");
    case EditActionPasteFont:
        return WEB_UI_STRING_KEY("Paste Font", "Paste Font (Undo action name)", "Undo action name");
    case EditActionPasteRuler:
        return WEB_UI_STRING_KEY("Paste Ruler", "Paste Ruler (Undo action name)", "Undo action name");
    case EditActionTyping:
        return WEB_UI_STRING_KEY("Typing", "Typing (Undo action name)", "Undo action name");
    case EditActionCreateLink:
        return WEB_UI_STRING_KEY("Create Link", "Create Link (Undo action name)", "Undo action name");
    case EditActionUnlink:
        return WEB_UI_STRING_KEY("Unlink", "Unlink (Undo action name)", "Undo action name");
    case EditActionInsertList:
        return WEB_UI_STRING_KEY("Insert List", "Insert List (Undo action name)", "Undo action name");
    case EditActionFormatBlock:
        return WEB_UI_STRING_KEY("Formatting", "Format Block (Undo action name)", "Undo action name");
    case EditActionIndent:
        return WEB_UI_STRING_KEY("Indent", "Indent (Undo action name)", "Undo action name");
    case EditActionOutdent:
        return WEB_UI_STRING_KEY("Outdent", "Outdent (Undo action name)", "Undo action name");
    }
    return String();
}

} // namespace WebKit

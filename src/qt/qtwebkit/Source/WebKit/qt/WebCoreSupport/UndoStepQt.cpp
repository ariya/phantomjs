/*
    Copyright (C) 2007 Staikos Computing Services Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "UndoStepQt.h"

#include <qobject.h>

using namespace WebCore;

static QString undoNameForEditAction(const EditAction editAction)
{
    switch (editAction) {
    case EditActionUnspecified:
        return QString();
    case EditActionSetColor:
        return QObject::tr("Set Color");
    case EditActionSetBackgroundColor:
        return QObject::tr("Set Background Color");
    case EditActionTurnOffKerning:
        return QObject::tr("Turn Off Kerning");
    case EditActionTightenKerning:
        return QObject::tr("Tighten Kerning");
    case EditActionLoosenKerning:
        return QObject::tr("Loosen Kerning");
    case EditActionUseStandardKerning:
        return QObject::tr("Use Standard Kerning");
    case EditActionTurnOffLigatures:
        return QObject::tr("Turn Off Ligatures");
    case EditActionUseStandardLigatures:
        return QObject::tr("Use Standard Ligatures");
    case EditActionUseAllLigatures:
        return QObject::tr("Use All Ligatures");
    case EditActionRaiseBaseline:
        return QObject::tr("Raise Baseline");
    case EditActionLowerBaseline:
        return QObject::tr("Lower Baseline");
    case EditActionSetTraditionalCharacterShape:
        return QObject::tr("Set Traditional Character Shape");
    case EditActionSetFont:
        return QObject::tr("Set Font");
    case EditActionChangeAttributes:
        return QObject::tr("Change Attributes");
    case EditActionAlignLeft:
        return QObject::tr("Align Left");
    case EditActionAlignRight:
        return QObject::tr("Align Right");
    case EditActionCenter:
        return QObject::tr("Center");
    case EditActionJustify:
        return QObject::tr("Justify");
    case EditActionSetWritingDirection:
        return QObject::tr("Set Writing Direction");
    case EditActionSubscript:
        return QObject::tr("Subscript");
    case EditActionBold:
        return QObject::tr("Bold");
    case EditActionItalics:
        return QObject::tr("Italic");
    case EditActionSuperscript:
        return QObject::tr("Superscript");
    case EditActionUnderline:
        return QObject::tr("Underline");
    case EditActionOutline:
        return QObject::tr("Outline");
    case EditActionUnscript:
        return QObject::tr("Unscript");
    case EditActionDrag:
        return QObject::tr("Drag");
    case EditActionCut:
        return QObject::tr("Cut");
    case EditActionPaste:
        return QObject::tr("Paste");
    case EditActionPasteFont:
        return QObject::tr("Paste Font");
    case EditActionPasteRuler:
        return QObject::tr("Paste Ruler");
    case EditActionTyping:
        return QObject::tr("Typing");
    case EditActionCreateLink:
        return QObject::tr("Create Link");
    case EditActionUnlink:
        return QObject::tr("Unlink");
    case EditActionInsertList:
        return QObject::tr("Insert List");
    case EditActionFormatBlock:
        return QObject::tr("Formatting");
    case EditActionIndent:
        return QObject::tr("Indent");
    case EditActionOutdent:
        return QObject::tr("Outdent");
    }

    ASSERT_NOT_REACHED();
    return QString();
}

UndoStepQt::UndoStepQt(WTF::RefPtr<UndoStep> step)
    : m_step(step)
    , m_first(true)
{
    m_text = undoNameForEditAction(step->editingAction());
}


UndoStepQt::~UndoStepQt()
{
}

void UndoStepQt::redo()
{
    if (m_first) {
        m_first = false;
        return;
    }
    if (m_step)
        m_step->reapply();
}


void UndoStepQt::undo()
{
    if (m_step)
        m_step->unapply();
}

QString UndoStepQt::text() const
{
    return m_text;
}

// vim: ts=4 sw=4 et

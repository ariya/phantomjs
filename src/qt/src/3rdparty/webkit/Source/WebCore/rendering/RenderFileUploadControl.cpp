/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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
#include "RenderFileUploadControl.h"

#include "Chrome.h"
#include "FileList.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "ShadowElement.h"
#include "Icon.h"
#include "LocalizedStrings.h"
#include "Page.h"
#include "PaintInfo.h"
#include "RenderButton.h"
#include "RenderText.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "TextRun.h"
#include <math.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

const int afterButtonSpacing = 4;
const int iconHeight = 16;
const int iconWidth = 16;
const int iconFilenameSpacing = 2;
const int defaultWidthNumChars = 34;
const int buttonShadowHeight = 2;

RenderFileUploadControl::RenderFileUploadControl(HTMLInputElement* input)
    : RenderBlock(input)
{
    FileList* list = input->files();
    Vector<String> filenames;
    unsigned length = list ? list->length() : 0;
    for (unsigned i = 0; i < length; ++i)
        filenames.append(list->item(i)->path());
    m_fileChooser = FileChooser::create(this, filenames);
}

RenderFileUploadControl::~RenderFileUploadControl()
{
    if (m_button)
        m_button->detach();
    m_fileChooser->disconnectClient();
}

void RenderFileUploadControl::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    if (m_button)
        m_button->renderer()->setStyle(createButtonStyle(style()));
}

void RenderFileUploadControl::valueChanged()
{
    // dispatchFormControlChangeEvent may destroy this renderer
    RefPtr<FileChooser> fileChooser = m_fileChooser;

    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(node());
    inputElement->setFileListFromRenderer(fileChooser->filenames());
    inputElement->dispatchFormControlChangeEvent();
 
    // only repaint if it doesn't seem we have been destroyed
    if (!fileChooser->disconnected())
        repaint();
}

bool RenderFileUploadControl::allowsMultipleFiles()
{
#if ENABLE(DIRECTORY_UPLOAD)
    if (allowsDirectoryUpload())
      return true;
#endif

    HTMLInputElement* input = static_cast<HTMLInputElement*>(node());
    return input->fastHasAttribute(multipleAttr);
}

#if ENABLE(DIRECTORY_UPLOAD)
bool RenderFileUploadControl::allowsDirectoryUpload()
{
    HTMLInputElement* input = static_cast<HTMLInputElement*>(node());
    return input->fastHasAttribute(webkitdirectoryAttr);
}

void RenderFileUploadControl::receiveDropForDirectoryUpload(const Vector<String>& paths)
{
    if (Chrome* chromePointer = chrome())
        chromePointer->enumerateChosenDirectory(paths[0], m_fileChooser.get());
}
#endif

String RenderFileUploadControl::acceptTypes()
{
    return static_cast<HTMLInputElement*>(node())->accept();
}

void RenderFileUploadControl::chooseIconForFiles(FileChooser* chooser, const Vector<String>& filenames)
{
    if (Chrome* chromePointer = chrome())
        chromePointer->chooseIconForFiles(filenames, chooser);
}

void RenderFileUploadControl::click()
{
    // Requires a user gesture to open the file dialog.
    if (!frame() || !frame()->loader()->isProcessingUserGesture())
        return;
    if (Chrome* chromePointer = chrome())
        chromePointer->runOpenPanel(frame(), m_fileChooser);
}

Chrome* RenderFileUploadControl::chrome() const
{
    Frame* frame = node()->document()->frame();
    if (!frame)
        return 0;
    Page* page = frame->page();
    if (!page)
        return 0;
    return page->chrome();
}

void RenderFileUploadControl::updateFromElement()
{
    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(node());
    ASSERT(inputElement->isFileUpload());
    
    if (!m_button) {
        m_button = ShadowInputElement::create(inputElement);
        m_button->setType("button");
        m_button->setValue(fileButtonChooseFileLabel());
        RefPtr<RenderStyle> buttonStyle = createButtonStyle(style());
        RenderObject* renderer = m_button->createRenderer(renderArena(), buttonStyle.get());
        m_button->setRenderer(renderer);
        renderer->setStyle(buttonStyle.release());
        renderer->updateFromElement();
        m_button->setAttached();
        m_button->setInDocument();

        addChild(renderer);
    }

    m_button->setDisabled(!theme()->isEnabled(this));

    // This only supports clearing out the files, but that's OK because for
    // security reasons that's the only change the DOM is allowed to make.
    FileList* files = inputElement->files();
    ASSERT(files);
    if (files && files->isEmpty() && !m_fileChooser->filenames().isEmpty()) {
        m_fileChooser->clear();
        repaint();
    }
}

int RenderFileUploadControl::maxFilenameWidth() const
{
    return max(0, contentWidth() - m_button->renderBox()->width() - afterButtonSpacing
        - (m_fileChooser->icon() ? iconWidth + iconFilenameSpacing : 0));
}

PassRefPtr<RenderStyle> RenderFileUploadControl::createButtonStyle(const RenderStyle* parentStyle) const
{
    RefPtr<RenderStyle> style = getCachedPseudoStyle(FILE_UPLOAD_BUTTON);
    if (!style) {
        style = RenderStyle::create();
        if (parentStyle)
            style->inheritFrom(parentStyle);
    }

    // Button text will wrap on file upload controls with widths smaller than the intrinsic button width
    // without this setWhiteSpace.
    style->setWhiteSpace(NOWRAP);

    return style.release();
}

void RenderFileUploadControl::paintObject(PaintInfo& paintInfo, int tx, int ty)
{
    if (style()->visibility() != VISIBLE)
        return;
    ASSERT(m_fileChooser);
    
    // Push a clip.
    GraphicsContextStateSaver stateSaver(*paintInfo.context, false);
    if (paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseChildBlockBackgrounds) {
        IntRect clipRect(tx + borderLeft(), ty + borderTop(),
                         width() - borderLeft() - borderRight(), height() - borderBottom() - borderTop() + buttonShadowHeight);
        if (clipRect.isEmpty())
            return;
        stateSaver.save();
        paintInfo.context->clip(clipRect);
    }

    if (paintInfo.phase == PaintPhaseForeground) {
        const String& displayedFilename = fileTextValue();
        unsigned length = displayedFilename.length();
        const UChar* string = displayedFilename.characters();
        TextRun textRun(string, length, false, 0, 0, TextRun::AllowTrailingExpansion, style()->direction(), style()->unicodeBidi() == Override);
        
        // Determine where the filename should be placed
        int contentLeft = tx + borderLeft() + paddingLeft();
        int buttonAndIconWidth = m_button->renderBox()->width() + afterButtonSpacing
            + (m_fileChooser->icon() ? iconWidth + iconFilenameSpacing : 0);
        int textX;
        if (style()->isLeftToRightDirection())
            textX = contentLeft + buttonAndIconWidth;
        else
            textX = contentLeft + contentWidth() - buttonAndIconWidth - style()->font().width(textRun);
        // We want to match the button's baseline
        RenderButton* buttonRenderer = toRenderButton(m_button->renderer());
        int textY = buttonRenderer->absoluteBoundingBoxRect().y()
            + buttonRenderer->marginTop() + buttonRenderer->borderTop() + buttonRenderer->paddingTop()
            + buttonRenderer->baselinePosition(AlphabeticBaseline, true, HorizontalLine, PositionOnContainingLine);

        paintInfo.context->setFillColor(style()->visitedDependentColor(CSSPropertyColor), style()->colorSpace());
        
        // Draw the filename
        paintInfo.context->drawBidiText(style()->font(), textRun, IntPoint(textX, textY));
        
        if (m_fileChooser->icon()) {
            // Determine where the icon should be placed
            int iconY = ty + borderTop() + paddingTop() + (contentHeight() - iconHeight) / 2;
            int iconX;
            if (style()->isLeftToRightDirection())
                iconX = contentLeft + m_button->renderBox()->width() + afterButtonSpacing;
            else
                iconX = contentLeft + contentWidth() - m_button->renderBox()->width() - afterButtonSpacing - iconWidth;

            // Draw the file icon
            m_fileChooser->icon()->paint(paintInfo.context, IntRect(iconX, iconY, iconWidth, iconHeight));
        }
    }

    // Paint the children.
    RenderBlock::paintObject(paintInfo, tx, ty);
}

void RenderFileUploadControl::computePreferredLogicalWidths()
{
    ASSERT(preferredLogicalWidthsDirty());

    m_minPreferredLogicalWidth = 0;
    m_maxPreferredLogicalWidth = 0;

    if (style()->width().isFixed() && style()->width().value() > 0)
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth = computeContentBoxLogicalWidth(style()->width().value());
    else {
        // Figure out how big the filename space needs to be for a given number of characters
        // (using "0" as the nominal character).
        const UChar ch = '0';
        float charWidth = style()->font().width(TextRun(&ch, 1, false, 0, 0, TextRun::AllowTrailingExpansion));
        m_maxPreferredLogicalWidth = (int)ceilf(charWidth * defaultWidthNumChars);
    }

    if (style()->minWidth().isFixed() && style()->minWidth().value() > 0) {
        m_maxPreferredLogicalWidth = max(m_maxPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->minWidth().value()));
        m_minPreferredLogicalWidth = max(m_minPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->minWidth().value()));
    } else if (style()->width().isPercent() || (style()->width().isAuto() && style()->height().isPercent()))
        m_minPreferredLogicalWidth = 0;
    else
        m_minPreferredLogicalWidth = m_maxPreferredLogicalWidth;

    if (style()->maxWidth().isFixed() && style()->maxWidth().value() != undefinedLength) {
        m_maxPreferredLogicalWidth = min(m_maxPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->maxWidth().value()));
        m_minPreferredLogicalWidth = min(m_minPreferredLogicalWidth, computeContentBoxLogicalWidth(style()->maxWidth().value()));
    }

    int toAdd = borderAndPaddingWidth();
    m_minPreferredLogicalWidth += toAdd;
    m_maxPreferredLogicalWidth += toAdd;

    setPreferredLogicalWidthsDirty(false);
}

VisiblePosition RenderFileUploadControl::positionForPoint(const IntPoint&)
{
    return VisiblePosition();
}

void RenderFileUploadControl::receiveDroppedFiles(const Vector<String>& paths)
{
#if ENABLE(DIRECTORY_UPLOAD)
    if (allowsDirectoryUpload()) {
        receiveDropForDirectoryUpload(paths);
        return;
    }
#endif

    if (allowsMultipleFiles())
        m_fileChooser->chooseFiles(paths);
    else
        m_fileChooser->chooseFile(paths[0]);
}

String RenderFileUploadControl::buttonValue()
{
    if (!m_button)
        return String();
    
    return m_button->value();
}

String RenderFileUploadControl::fileTextValue() const
{
    return m_fileChooser->basenameForWidth(style()->font(), maxFilenameWidth());
}
    
} // namespace WebCore

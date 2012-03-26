/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "FillLayer.h"

namespace WebCore {

FillLayer::FillLayer(EFillLayerType type)
    : m_next(0)
    , m_image(FillLayer::initialFillImage(type))
    , m_xPosition(FillLayer::initialFillXPosition(type))
    , m_yPosition(FillLayer::initialFillYPosition(type))
    , m_attachment(FillLayer::initialFillAttachment(type))
    , m_clip(FillLayer::initialFillClip(type))
    , m_origin(FillLayer::initialFillOrigin(type))
    , m_repeatX(FillLayer::initialFillRepeatX(type))
    , m_repeatY(FillLayer::initialFillRepeatY(type))
    , m_composite(FillLayer::initialFillComposite(type))
    , m_sizeType(SizeNone)
    , m_sizeLength(FillLayer::initialFillSizeLength(type))
    , m_imageSet(false)
    , m_attachmentSet(false)
    , m_clipSet(false)
    , m_originSet(false)
    , m_repeatXSet(false)
    , m_repeatYSet(false)
    , m_xPosSet(false)
    , m_yPosSet(false)
    , m_compositeSet(type == MaskFillLayer)
    , m_type(type)
{
}

FillLayer::FillLayer(const FillLayer& o)
    : m_next(o.m_next ? new FillLayer(*o.m_next) : 0)
    , m_image(o.m_image)
    , m_xPosition(o.m_xPosition)
    , m_yPosition(o.m_yPosition)
    , m_attachment(o.m_attachment)
    , m_clip(o.m_clip)
    , m_origin(o.m_origin)
    , m_repeatX(o.m_repeatX)
    , m_repeatY(o.m_repeatY)
    , m_composite(o.m_composite)
    , m_sizeType(o.m_sizeType)
    , m_sizeLength(o.m_sizeLength)
    , m_imageSet(o.m_imageSet)
    , m_attachmentSet(o.m_attachmentSet)
    , m_clipSet(o.m_clipSet)
    , m_originSet(o.m_originSet)
    , m_repeatXSet(o.m_repeatXSet)
    , m_repeatYSet(o.m_repeatYSet)
    , m_xPosSet(o.m_xPosSet)
    , m_yPosSet(o.m_yPosSet)
    , m_compositeSet(o.m_compositeSet)
    , m_type(o.m_type)
{
}

FillLayer::~FillLayer()
{
    delete m_next;
}

FillLayer& FillLayer::operator=(const FillLayer& o)
{
    if (m_next != o.m_next) {
        delete m_next;
        m_next = o.m_next ? new FillLayer(*o.m_next) : 0;
    }

    m_image = o.m_image;
    m_xPosition = o.m_xPosition;
    m_yPosition = o.m_yPosition;
    m_attachment = o.m_attachment;
    m_clip = o.m_clip;
    m_composite = o.m_composite;
    m_origin = o.m_origin;
    m_repeatX = o.m_repeatX;
    m_repeatY = o.m_repeatY;
    m_sizeType = o.m_sizeType;
    m_sizeLength = o.m_sizeLength;

    m_imageSet = o.m_imageSet;
    m_attachmentSet = o.m_attachmentSet;
    m_clipSet = o.m_clipSet;
    m_compositeSet = o.m_compositeSet;
    m_originSet = o.m_originSet;
    m_repeatXSet = o.m_repeatXSet;
    m_repeatYSet = o.m_repeatYSet;
    m_xPosSet = o.m_xPosSet;
    m_yPosSet = o.m_yPosSet;
    
    m_type = o.m_type;

    return *this;
}

bool FillLayer::operator==(const FillLayer& o) const
{
    // We do not check the "isSet" booleans for each property, since those are only used during initial construction
    // to propagate patterns into layers.  All layer comparisons happen after values have all been filled in anyway.
    return StyleImage::imagesEquivalent(m_image.get(), o.m_image.get()) && m_xPosition == o.m_xPosition && m_yPosition == o.m_yPosition &&
           m_attachment == o.m_attachment && m_clip == o.m_clip && 
           m_composite == o.m_composite && m_origin == o.m_origin && m_repeatX == o.m_repeatX &&
           m_repeatY == o.m_repeatY && m_sizeType == o.m_sizeType && m_sizeLength == o.m_sizeLength && 
           m_type == o.m_type && ((m_next && o.m_next) ? *m_next == *o.m_next : m_next == o.m_next);
}

void FillLayer::fillUnsetProperties()
{
    FillLayer* curr;
    for (curr = this; curr && curr->isXPositionSet(); curr = curr->next()) { }
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (FillLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_xPosition = pattern->m_xPosition;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }
    
    for (curr = this; curr && curr->isYPositionSet(); curr = curr->next()) { }
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (FillLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_yPosition = pattern->m_yPosition;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }
    
    for (curr = this; curr && curr->isAttachmentSet(); curr = curr->next()) { }
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (FillLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_attachment = pattern->m_attachment;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }
    
    for (curr = this; curr && curr->isClipSet(); curr = curr->next()) { }
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (FillLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_clip = pattern->m_clip;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isCompositeSet(); curr = curr->next()) { }
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (FillLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_composite = pattern->m_composite;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isOriginSet(); curr = curr->next()) { }
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (FillLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_origin = pattern->m_origin;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isRepeatXSet(); curr = curr->next()) { }
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (FillLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_repeatX = pattern->m_repeatX;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }

    for (curr = this; curr && curr->isRepeatYSet(); curr = curr->next()) { }
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (FillLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_repeatY = pattern->m_repeatY;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }
    
    for (curr = this; curr && curr->isSizeSet(); curr = curr->next()) { }
    if (curr && curr != this) {
        // We need to fill in the remaining values with the pattern specified.
        for (FillLayer* pattern = this; curr; curr = curr->next()) {
            curr->m_sizeType = pattern->m_sizeType;
            curr->m_sizeLength = pattern->m_sizeLength;
            pattern = pattern->next();
            if (pattern == curr || !pattern)
                pattern = this;
        }
    }
}

void FillLayer::cullEmptyLayers()
{
    FillLayer* next;
    for (FillLayer* p = this; p; p = next) {
        next = p->m_next;
        if (next && !next->isImageSet()) {
            delete next;
            p->m_next = 0;
            break;
        }
    }
}

bool FillLayer::containsImage(StyleImage* s) const
{
    if (!s)
        return false;
    if (m_image && *s == *m_image)
        return true;
    if (m_next)
        return m_next->containsImage(s);
    return false;
}

bool FillLayer::imagesAreLoaded() const
{
    const FillLayer* curr;
    for (curr = this; curr; curr = curr->next()) {
        if (curr->m_image && !curr->m_image->isLoaded())
            return false;
    }

    return true;
}

} // namespace WebCore

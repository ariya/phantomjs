/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef PatternAttributes_h
#define PatternAttributes_h

#if ENABLE(SVG)
#include "SVGLength.h"
#include "SVGPreserveAspectRatio.h"

namespace WebCore {

class SVGPatternElement;

struct PatternAttributes {
    PatternAttributes()
        : m_x()
        , m_y()
        , m_width()
        , m_height()
        , m_viewBox()
        , m_preserveAspectRatio()
        , m_boundingBoxMode(true)
        , m_boundingBoxModeContent(false)
        , m_patternContentElement(0)
        , m_xSet(false)
        , m_ySet(false)
        , m_widthSet(false)
        , m_heightSet(false)
        , m_viewBoxSet(false)
        , m_preserveAspectRatioSet(false)
        , m_boundingBoxModeSet(false)
        , m_boundingBoxModeContentSet(false)
        , m_patternTransformSet(false)
        , m_patternContentElementSet(false)
    {
    }

    SVGLength x() const { return m_x; }
    SVGLength y() const { return m_y; }
    SVGLength width() const { return m_width; }
    SVGLength height() const { return m_height; }
    FloatRect viewBox() const { return m_viewBox; }
    SVGPreserveAspectRatio preserveAspectRatio() const { return m_preserveAspectRatio; }
    bool boundingBoxMode() const { return m_boundingBoxMode; }
    bool boundingBoxModeContent() const { return m_boundingBoxModeContent; }
    AffineTransform patternTransform() const { return m_patternTransform; }
    const SVGPatternElement* patternContentElement() const { return m_patternContentElement; }

    void setX(const SVGLength& value)
    {
        m_x = value;
        m_xSet = true;
    }

    void setY(const SVGLength& value)
    {
        m_y = value;
        m_ySet = true;
    }

    void setWidth(const SVGLength& value)
    {
        m_width = value;
        m_widthSet = true;
    }

    void setHeight(const SVGLength& value)
    {
        m_height = value;
        m_heightSet = true;
    }
    
    void setViewBox(const FloatRect& value)
    {
        m_viewBox = value;
        m_viewBoxSet = true;
    }

    void setPreserveAspectRatio(const SVGPreserveAspectRatio& value)
    {
        m_preserveAspectRatio = value;
        m_preserveAspectRatioSet = true;
    }

    void setBoundingBoxMode(bool value)
    {
        m_boundingBoxMode = value;
        m_boundingBoxModeSet = true;
    }

    void setBoundingBoxModeContent(bool value)
    {
        m_boundingBoxModeContent = value;
        m_boundingBoxModeContentSet = true;
    }

    void setPatternTransform(const AffineTransform& value)
    {
        m_patternTransform = value;
        m_patternTransformSet = true;
    }

    void setPatternContentElement(const SVGPatternElement* value)
    {
        m_patternContentElement = value;
        m_patternContentElementSet = true;
    }

    bool hasX() const { return m_xSet; }
    bool hasY() const { return m_ySet; }
    bool hasWidth() const { return m_widthSet; }
    bool hasHeight() const { return m_heightSet; }
    bool hasViewBox() const { return m_viewBoxSet; }
    bool hasPreserveAspectRatio() const { return m_preserveAspectRatioSet; }
    bool hasBoundingBoxMode() const { return m_boundingBoxModeSet; }
    bool hasBoundingBoxModeContent() const { return m_boundingBoxModeContentSet; }
    bool hasPatternTransform() const { return m_patternTransformSet; }
    bool hasPatternContentElement() const { return m_patternContentElementSet; }

private:
    // Properties
    SVGLength m_x;
    SVGLength m_y;
    SVGLength m_width;
    SVGLength m_height;
    FloatRect m_viewBox;
    SVGPreserveAspectRatio m_preserveAspectRatio;
    bool m_boundingBoxMode;
    bool m_boundingBoxModeContent;
    AffineTransform m_patternTransform;
    const SVGPatternElement* m_patternContentElement;

    // Property states
    bool m_xSet : 1;
    bool m_ySet : 1;
    bool m_widthSet : 1;
    bool m_heightSet : 1;
    bool m_viewBoxSet : 1;
    bool m_preserveAspectRatioSet : 1;
    bool m_boundingBoxModeSet : 1;
    bool m_boundingBoxModeContentSet : 1;
    bool m_patternTransformSet : 1;
    bool m_patternContentElementSet : 1;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif

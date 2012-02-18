/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2010 Dirk Schulze <krit@webkit.org>
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

#include "config.h"

#if ENABLE(SVG)
#include "SVGPreserveAspectRatio.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "SVGParserUtilities.h"
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

SVGPreserveAspectRatio::SVGPreserveAspectRatio()
    : m_align(SVG_PRESERVEASPECTRATIO_XMIDYMID)
    , m_meetOrSlice(SVG_MEETORSLICE_MEET)
{
}

void SVGPreserveAspectRatio::setAlign(unsigned short align, ExceptionCode& ec)
{
    if (align == SVG_PRESERVEASPECTRATIO_UNKNOWN || align > SVG_PRESERVEASPECTRATIO_XMAXYMAX) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_align = static_cast<SVGPreserveAspectRatioType>(align);
}

void SVGPreserveAspectRatio::setMeetOrSlice(unsigned short meetOrSlice, ExceptionCode& ec)
{
    if (meetOrSlice == SVG_MEETORSLICE_UNKNOWN || meetOrSlice > SVG_MEETORSLICE_SLICE) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_meetOrSlice = static_cast<SVGMeetOrSliceType>(meetOrSlice);
}

SVGPreserveAspectRatio SVGPreserveAspectRatio::parsePreserveAspectRatio(const UChar*& currParam, const UChar* end, bool validate, bool& result)
{
    SVGPreserveAspectRatio aspectRatio;
    aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_NONE;
    aspectRatio.m_meetOrSlice = SVG_MEETORSLICE_MEET;
    result = false;

    // FIXME: Rewrite this parser, without gotos!
    if (!skipOptionalSpaces(currParam, end))
        goto bailOut;

    if (*currParam == 'd') {
        if (!skipString(currParam, end, "defer"))
            goto bailOut;
        // FIXME: We just ignore the "defer" here.
        if (!skipOptionalSpaces(currParam, end))
            goto bailOut;
    }

    if (*currParam == 'n') {
        if (!skipString(currParam, end, "none"))
            goto bailOut;
        skipOptionalSpaces(currParam, end);
    } else if (*currParam == 'x') {
        if ((end - currParam) < 8)
            goto bailOut;
        if (currParam[1] != 'M' || currParam[4] != 'Y' || currParam[5] != 'M')
            goto bailOut;
        if (currParam[2] == 'i') {
            if (currParam[3] == 'n') {
                if (currParam[6] == 'i') {
                    if (currParam[7] == 'n')
                        aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_XMINYMIN;
                    else if (currParam[7] == 'd')
                        aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_XMINYMID;
                    else
                        goto bailOut;
                } else if (currParam[6] == 'a' && currParam[7] == 'x')
                     aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_XMINYMAX;
                else
                     goto bailOut;
             } else if (currParam[3] == 'd') {
                if (currParam[6] == 'i') {
                    if (currParam[7] == 'n')
                        aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_XMIDYMIN;
                    else if (currParam[7] == 'd')
                        aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_XMIDYMID;
                    else
                        goto bailOut;
                } else if (currParam[6] == 'a' && currParam[7] == 'x')
                    aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_XMIDYMAX;
                else
                    goto bailOut;
            } else
                goto bailOut;
        } else if (currParam[2] == 'a' && currParam[3] == 'x') {
            if (currParam[6] == 'i') {
                if (currParam[7] == 'n')
                    aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_XMAXYMIN;
                else if (currParam[7] == 'd')
                    aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_XMAXYMID;
                else
                    goto bailOut;
            } else if (currParam[6] == 'a' && currParam[7] == 'x')
                aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_XMAXYMAX;
            else
                goto bailOut;
        } else
            goto bailOut;
        currParam += 8;
        skipOptionalSpaces(currParam, end);
    } else
        goto bailOut;

    if (currParam < end) {
        if (*currParam == 'm') {
            if (!skipString(currParam, end, "meet"))
                goto bailOut;
            skipOptionalSpaces(currParam, end);
        } else if (*currParam == 's') {
            if (!skipString(currParam, end, "slice"))
                goto bailOut;
            skipOptionalSpaces(currParam, end);
            if (aspectRatio.m_align != SVG_PRESERVEASPECTRATIO_NONE)
                aspectRatio.m_meetOrSlice = SVG_MEETORSLICE_SLICE;    
        }
    }

    if (end != currParam && validate) {
bailOut:
        // FIXME: Should the two values be set to UNKNOWN instead?
        aspectRatio.m_align = SVG_PRESERVEASPECTRATIO_NONE;
        aspectRatio.m_meetOrSlice = SVG_MEETORSLICE_MEET;
    } else
        result = true;

    return aspectRatio;
}

void SVGPreserveAspectRatio::transformRect(FloatRect& destRect, FloatRect& srcRect)
{
    FloatSize imageSize = srcRect.size();
    float origDestWidth = destRect.width();
    float origDestHeight = destRect.height();
    switch (m_meetOrSlice) {
    case SVGPreserveAspectRatio::SVG_MEETORSLICE_UNKNOWN:
        break;
    case SVGPreserveAspectRatio::SVG_MEETORSLICE_MEET: {
        float widthToHeightMultiplier = srcRect.height() / srcRect.width();
        if (origDestHeight > origDestWidth * widthToHeightMultiplier) {
            destRect.setHeight(origDestWidth * widthToHeightMultiplier);
            switch (m_align) {
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMINYMID:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMID:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMID:
                destRect.setY(destRect.y() + origDestHeight / 2 - destRect.height() / 2);
                break;
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMINYMAX:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMAX:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMAX:
                destRect.setY(destRect.y() + origDestHeight - destRect.height());
                break;
            default:
                break;
            }
        }
        if (origDestWidth > origDestHeight / widthToHeightMultiplier) {
            destRect.setWidth(origDestHeight / widthToHeightMultiplier);
            switch (m_align) {
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMIN:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMID:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMAX:
                destRect.setX(destRect.x() + origDestWidth / 2 - destRect.width() / 2);
                break;
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMIN:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMID:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMAX:
                destRect.setX(destRect.x() + origDestWidth - destRect.width());
                break;
            default:
                break;
            }
        }
        break;
    }
    case SVGPreserveAspectRatio::SVG_MEETORSLICE_SLICE: {
        float widthToHeightMultiplier = srcRect.height() / srcRect.width();
        // if the destination height is less than the height of the image we'll be drawing
        if (origDestHeight < origDestWidth * widthToHeightMultiplier) {
            float destToSrcMultiplier = srcRect.width() / destRect.width();
            srcRect.setHeight(destRect.height() * destToSrcMultiplier);
            switch (m_align) {
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMINYMID:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMID:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMID:
                srcRect.setY(destRect.y() + imageSize.height() / 2 - srcRect.height() / 2);
                break;
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMINYMAX:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMAX:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMAX:
                srcRect.setY(destRect.y() + imageSize.height() - srcRect.height());
                break;
            default:
                break;
            }
        }
        // if the destination width is less than the width of the image we'll be drawing
        if (origDestWidth < origDestHeight / widthToHeightMultiplier) {
            float destToSrcMultiplier = srcRect.height() / destRect.height();
            srcRect.setWidth(destRect.width() * destToSrcMultiplier);
            switch (m_align) {
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMIN:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMID:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMIDYMAX:
                srcRect.setX(destRect.x() + imageSize.width() / 2 - srcRect.width() / 2);
                break;
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMIN:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMID:
            case SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_XMAXYMAX:
                srcRect.setX(destRect.x() + imageSize.width() - srcRect.width());
                break;
            default:
                break;
            }
        }
        break;
    }
    }
}

AffineTransform SVGPreserveAspectRatio::getCTM(float logicX, float logicY, float logicWidth, float logicHeight, float physWidth, float physHeight) const
{
    AffineTransform transform;
    if (m_align == SVG_PRESERVEASPECTRATIO_UNKNOWN)
        return transform;

    float logicalRatio = logicWidth / logicHeight;
    float physRatio = physWidth / physHeight;

    if (m_align == SVG_PRESERVEASPECTRATIO_NONE) {
        transform.scaleNonUniform(physWidth / logicWidth, physHeight / logicHeight);
        transform.translate(-logicX, -logicY);
        return transform;
    }

    if ((logicalRatio < physRatio && (m_meetOrSlice == SVG_MEETORSLICE_MEET)) || (logicalRatio >= physRatio && (m_meetOrSlice == SVG_MEETORSLICE_SLICE))) {
        transform.scaleNonUniform(physHeight / logicHeight, physHeight / logicHeight);

        if (m_align == SVG_PRESERVEASPECTRATIO_XMINYMIN || m_align == SVG_PRESERVEASPECTRATIO_XMINYMID || m_align == SVG_PRESERVEASPECTRATIO_XMINYMAX)
            transform.translate(-logicX, -logicY);
        else if (m_align == SVG_PRESERVEASPECTRATIO_XMIDYMIN || m_align == SVG_PRESERVEASPECTRATIO_XMIDYMID || m_align == SVG_PRESERVEASPECTRATIO_XMIDYMAX)
            transform.translate(-logicX - (logicWidth - physWidth * logicHeight / physHeight) / 2, -logicY);
        else
            transform.translate(-logicX - (logicWidth - physWidth * logicHeight / physHeight), -logicY);
        
        return transform;
    }

    transform.scaleNonUniform(physWidth / logicWidth, physWidth / logicWidth);

    if (m_align == SVG_PRESERVEASPECTRATIO_XMINYMIN || m_align == SVG_PRESERVEASPECTRATIO_XMIDYMIN || m_align == SVG_PRESERVEASPECTRATIO_XMAXYMIN)
        transform.translate(-logicX, -logicY);
    else if (m_align == SVG_PRESERVEASPECTRATIO_XMINYMID || m_align == SVG_PRESERVEASPECTRATIO_XMIDYMID || m_align == SVG_PRESERVEASPECTRATIO_XMAXYMID)
        transform.translate(-logicX, -logicY - (logicHeight - physHeight * logicWidth / physWidth) / 2);
    else
        transform.translate(-logicX, -logicY - (logicHeight - physHeight * logicWidth / physWidth));

    return transform;
}

String SVGPreserveAspectRatio::valueAsString() const
{
    String alignType;

    switch (m_align) {
    case SVG_PRESERVEASPECTRATIO_NONE:
        alignType = "none";
        break;
    case SVG_PRESERVEASPECTRATIO_XMINYMIN:
        alignType = "xMinYMin";
        break;
    case SVG_PRESERVEASPECTRATIO_XMIDYMIN:
        alignType = "xMidYMin";
        break;
    case SVG_PRESERVEASPECTRATIO_XMAXYMIN:
        alignType = "xMaxYMin";
        break;
    case SVG_PRESERVEASPECTRATIO_XMINYMID:
        alignType = "xMinYMid";
        break;
    case SVG_PRESERVEASPECTRATIO_XMIDYMID:
        alignType = "xMidYMid";
        break;
    case SVG_PRESERVEASPECTRATIO_XMAXYMID:
        alignType = "xMaxYMid";
        break;
    case SVG_PRESERVEASPECTRATIO_XMINYMAX:
        alignType = "xMinYMax";
        break;
    case SVG_PRESERVEASPECTRATIO_XMIDYMAX:
        alignType = "xMidYMax";
        break;
    case SVG_PRESERVEASPECTRATIO_XMAXYMAX:
        alignType = "xMaxYMax";
        break;
    case SVG_PRESERVEASPECTRATIO_UNKNOWN:
        alignType = "unknown";
        break;
    };

    switch (m_meetOrSlice) {
    default:
    case SVG_MEETORSLICE_UNKNOWN:
        return alignType;
    case SVG_MEETORSLICE_MEET:
        return makeString(alignType, " meet");
    case SVG_MEETORSLICE_SLICE:
        return makeString(alignType, " slice");
    };
}

}

#endif // ENABLE(SVG)

/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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

#if ENABLE(FILTERS)
#include "FEColorMatrix.h"

#include "Filter.h"
#include "GraphicsContext.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/MathExtras.h>
#include <wtf/Uint8ClampedArray.h>

namespace WebCore {

FEColorMatrix::FEColorMatrix(Filter* filter, ColorMatrixType type, const Vector<float>& values)
    : FilterEffect(filter)
    , m_type(type)
    , m_values(values)
{
}

PassRefPtr<FEColorMatrix> FEColorMatrix::create(Filter* filter, ColorMatrixType type, const Vector<float>& values)
{
    return adoptRef(new FEColorMatrix(filter, type, values));
}

ColorMatrixType FEColorMatrix::type() const
{
    return m_type;
}

bool FEColorMatrix::setType(ColorMatrixType type)
{
    if (m_type == type)
        return false;
    m_type = type;
    return true;
}

const Vector<float>& FEColorMatrix::values() const
{
    return m_values;
}

bool FEColorMatrix::setValues(const Vector<float> &values)
{
    if (m_values == values)
        return false;
    m_values = values;
    return true;
}

inline void matrix(float& red, float& green, float& blue, float& alpha, const Vector<float>& values)
{
    float r = values[0] * red + values[1] * green + values[2] * blue + values[3] * alpha + values[4] * 255;
    float g = values[5] * red + values[6] * green + values[7] * blue + values[8] * alpha + values[9] * 255;
    float b = values[10] * red + values[11] * green + values[12] * blue + values[13] * alpha + values[14] * 255;
    float a = values[15] * red + values[16] * green + values[17] * blue + values[18] * alpha + values[19] * 255;

    red = r;
    green = g;
    blue = b;
    alpha = a;
}

inline void saturateAndHueRotate(float& red, float& green, float& blue, const float* components)
{
    float r = red * components[0] + green * components[1] + blue * components[2];
    float g = red * components[3] + green * components[4] + blue * components[5];
    float b = red * components[6] + green * components[7] + blue * components[8];

    red = r;
    green = g;
    blue = b;
}

inline void luminance(float& red, float& green, float& blue, float& alpha)
{
    alpha = 0.2125 * red + 0.7154 * green + 0.0721 * blue;
    red = 0;
    green = 0;
    blue = 0;
}

template<ColorMatrixType filterType>
void effectType(Uint8ClampedArray* pixelArray, const Vector<float>& values)
{
    unsigned pixelArrayLength = pixelArray->length();
    float components[9];

    if (filterType == FECOLORMATRIX_TYPE_SATURATE)
        FEColorMatrix::calculateSaturateComponents(components, values[0]);
    else if (filterType == FECOLORMATRIX_TYPE_HUEROTATE)
        FEColorMatrix::calculateHueRotateComponents(components, values[0]);

    for (unsigned pixelByteOffset = 0; pixelByteOffset < pixelArrayLength; pixelByteOffset += 4) {
        float red = pixelArray->item(pixelByteOffset);
        float green = pixelArray->item(pixelByteOffset + 1);
        float blue = pixelArray->item(pixelByteOffset + 2);
        float alpha = pixelArray->item(pixelByteOffset + 3);

        switch (filterType) {
            case FECOLORMATRIX_TYPE_MATRIX:
                matrix(red, green, blue, alpha, values);
                break;
            case FECOLORMATRIX_TYPE_SATURATE:
            case FECOLORMATRIX_TYPE_HUEROTATE:
                saturateAndHueRotate(red, green, blue, components);
                break;
            case FECOLORMATRIX_TYPE_LUMINANCETOALPHA:
                luminance(red, green, blue, alpha);
                break;
        }

        pixelArray->set(pixelByteOffset, red);
        pixelArray->set(pixelByteOffset + 1, green);
        pixelArray->set(pixelByteOffset + 2, blue);
        pixelArray->set(pixelByteOffset + 3, alpha);
    }
}

void FEColorMatrix::platformApplySoftware()
{
    FilterEffect* in = inputEffect(0);

    ImageBuffer* resultImage = createImageBufferResult();
    if (!resultImage)
        return;

    resultImage->context()->drawImageBuffer(in->asImageBuffer(), ColorSpaceDeviceRGB, drawingRegionOfInputImage(in->absolutePaintRect()));

    IntRect imageRect(IntPoint(), absolutePaintRect().size());
    RefPtr<Uint8ClampedArray> pixelArray = resultImage->getUnmultipliedImageData(imageRect);

    switch (m_type) {
    case FECOLORMATRIX_TYPE_UNKNOWN:
        break;
    case FECOLORMATRIX_TYPE_MATRIX:
        effectType<FECOLORMATRIX_TYPE_MATRIX>(pixelArray.get(), m_values);
        break;
    case FECOLORMATRIX_TYPE_SATURATE: 
        effectType<FECOLORMATRIX_TYPE_SATURATE>(pixelArray.get(), m_values);
        break;
    case FECOLORMATRIX_TYPE_HUEROTATE:
        effectType<FECOLORMATRIX_TYPE_HUEROTATE>(pixelArray.get(), m_values);
        break;
    case FECOLORMATRIX_TYPE_LUMINANCETOALPHA:
        effectType<FECOLORMATRIX_TYPE_LUMINANCETOALPHA>(pixelArray.get(), m_values);
        setIsAlphaImage(true);
        break;
    }

    resultImage->putByteArray(Unmultiplied, pixelArray.get(), imageRect.size(), imageRect, IntPoint());
}

void FEColorMatrix::dump()
{
}

static TextStream& operator<<(TextStream& ts, const ColorMatrixType& type)
{
    switch (type) {
    case FECOLORMATRIX_TYPE_UNKNOWN:
        ts << "UNKNOWN";
        break;
    case FECOLORMATRIX_TYPE_MATRIX:
        ts << "MATRIX";
        break;
    case FECOLORMATRIX_TYPE_SATURATE:
        ts << "SATURATE";
        break;
    case FECOLORMATRIX_TYPE_HUEROTATE:
        ts << "HUEROTATE";
        break;
    case FECOLORMATRIX_TYPE_LUMINANCETOALPHA:
        ts << "LUMINANCETOALPHA";
        break;
    }
    return ts;
}

TextStream& FEColorMatrix::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feColorMatrix";
    FilterEffect::externalRepresentation(ts);
    ts << " type=\"" << m_type << "\"";
    if (!m_values.isEmpty()) {
        ts << " values=\"";
        Vector<float>::const_iterator ptr = m_values.begin();
        const Vector<float>::const_iterator end = m_values.end();
        while (ptr < end) {
            ts << *ptr;
            ++ptr;
            if (ptr < end) 
                ts << " ";
        }
        ts << "\"";
    }
    ts << "]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    return ts;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)

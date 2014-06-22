/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "ViewportArguments.h"

#include "Chrome.h"
#include "Console.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Frame.h"
#include "IntSize.h"
#include "Page.h"
#include "ScriptableDocumentParser.h"
#include <wtf/text/WTFString.h>

using namespace std;

namespace WebCore {

#if PLATFORM(BLACKBERRY) || PLATFORM(GTK) || PLATFORM(QT)
const float ViewportArguments::deprecatedTargetDPI = 160;
#endif

static const float& compareIgnoringAuto(const float& value1, const float& value2, const float& (*compare) (const float&, const float&))
{
    ASSERT(value1 != ViewportArguments::ValueAuto || value2 != ViewportArguments::ValueAuto);

    if (value1 == ViewportArguments::ValueAuto)
        return value2;

    if (value2 == ViewportArguments::ValueAuto)
        return value1;

    return compare(value1, value2);
}

static inline float clampLengthValue(float value)
{
    ASSERT(value != ViewportArguments::ValueDeviceWidth);
    ASSERT(value != ViewportArguments::ValueDeviceHeight);

    // Limits as defined in the css-device-adapt spec.
    if (value != ViewportArguments::ValueAuto)
        return min(float(10000), max(value, float(1)));
    return value;
}

static inline float clampScaleValue(float value)
{
    ASSERT(value != ViewportArguments::ValueDeviceWidth);
    ASSERT(value != ViewportArguments::ValueDeviceHeight);

    // Limits as defined in the css-device-adapt spec.
    if (value != ViewportArguments::ValueAuto)
        return min(float(10), max(value, float(0.1)));
    return value;
}

ViewportAttributes ViewportArguments::resolve(const FloatSize& initialViewportSize, const FloatSize& deviceSize, int defaultWidth) const
{
    float resultWidth = width;
    float resultMaxWidth = maxWidth;
    float resultMinWidth = minWidth;
    float resultHeight = height;
    float resultMinHeight = minHeight;
    float resultMaxHeight = maxHeight;
    float resultZoom = zoom;
    float resultMinZoom = minZoom;
    float resultMaxZoom = maxZoom;
    float resultUserZoom = userZoom;

    switch (int(resultWidth)) {
    case ViewportArguments::ValueDeviceWidth:
        resultWidth = deviceSize.width();
        break;
    case ViewportArguments::ValueDeviceHeight:
        resultWidth = deviceSize.height();
        break;
    }

    switch (int(resultHeight)) {
    case ViewportArguments::ValueDeviceWidth:
        resultHeight = deviceSize.width();
        break;
    case ViewportArguments::ValueDeviceHeight:
        resultHeight = deviceSize.height();
        break;
    }

    if (type == ViewportArguments::CSSDeviceAdaptation) {
        switch (int(resultMinWidth)) {
        case ViewportArguments::ValueDeviceWidth:
            resultMinWidth = deviceSize.width();
            break;
        case ViewportArguments::ValueDeviceHeight:
            resultMinWidth = deviceSize.height();
            break;
        }

        switch (int(resultMaxWidth)) {
        case ViewportArguments::ValueDeviceWidth:
            resultMaxWidth = deviceSize.width();
            break;
        case ViewportArguments::ValueDeviceHeight:
            resultMaxWidth = deviceSize.height();
            break;
        }

        switch (int(resultMinHeight)) {
        case ViewportArguments::ValueDeviceWidth:
            resultMinHeight = deviceSize.width();
            break;
        case ViewportArguments::ValueDeviceHeight:
            resultMinHeight = deviceSize.height();
            break;
        }

        switch (int(resultMaxHeight)) {
        case ViewportArguments::ValueDeviceWidth:
            resultMaxHeight = deviceSize.width();
            break;
        case ViewportArguments::ValueDeviceHeight:
            resultMaxHeight = deviceSize.height();
            break;
        }

        if (resultMinWidth != ViewportArguments::ValueAuto || resultMaxWidth != ViewportArguments::ValueAuto)
            resultWidth = compareIgnoringAuto(resultMinWidth, compareIgnoringAuto(resultMaxWidth, deviceSize.width(), min), max);

        if (resultMinHeight != ViewportArguments::ValueAuto || resultMaxHeight != ViewportArguments::ValueAuto)
            resultHeight = compareIgnoringAuto(resultMinHeight, compareIgnoringAuto(resultMaxHeight, deviceSize.height(), min), max);

        if (resultMinZoom != ViewportArguments::ValueAuto && resultMaxZoom != ViewportArguments::ValueAuto)
            resultMaxZoom = max(resultMinZoom, resultMaxZoom);

        if (resultZoom != ViewportArguments::ValueAuto)
            resultZoom = compareIgnoringAuto(resultMinZoom, compareIgnoringAuto(resultMaxZoom, resultZoom, min), max);

        if (resultWidth == ViewportArguments::ValueAuto && resultZoom == ViewportArguments::ValueAuto)
            resultWidth = deviceSize.width();

        if (resultWidth == ViewportArguments::ValueAuto && resultHeight == ViewportArguments::ValueAuto)
            resultWidth = deviceSize.width() / resultZoom;

        if (resultWidth == ViewportArguments::ValueAuto)
            resultWidth = resultHeight * deviceSize.width() / deviceSize.height();

        if (resultHeight == ViewportArguments::ValueAuto)
            resultHeight = resultWidth * deviceSize.height() / deviceSize.width();

        if (resultZoom != ViewportArguments::ValueAuto || resultMaxZoom != ViewportArguments::ValueAuto) {
            resultWidth = compareIgnoringAuto(resultWidth, deviceSize.width() / compareIgnoringAuto(resultZoom, resultMaxZoom, min), max);
            resultHeight = compareIgnoringAuto(resultHeight, deviceSize.height() / compareIgnoringAuto(resultZoom, resultMaxZoom, min), max);
        }

        resultWidth = max<float>(1, resultWidth);
        resultHeight = max<float>(1, resultHeight);
    }

    if (type != ViewportArguments::CSSDeviceAdaptation && type != ViewportArguments::Implicit) {
        // Clamp values to a valid range, but not for @viewport since is
        // not mandated by the specification.
        resultWidth = clampLengthValue(resultWidth);
        resultHeight = clampLengthValue(resultHeight);
        resultZoom = clampScaleValue(resultZoom);
        resultMinZoom = clampScaleValue(resultMinZoom);
        resultMaxZoom = clampScaleValue(resultMaxZoom);
    }

    ViewportAttributes result;

    // Resolve minimum-scale and maximum-scale values according to spec.
    if (resultMinZoom == ViewportArguments::ValueAuto)
        result.minimumScale = float(0.25);
    else
        result.minimumScale = resultMinZoom;

    if (resultMaxZoom == ViewportArguments::ValueAuto) {
        result.maximumScale = float(5.0);
        result.minimumScale = min(float(5.0), result.minimumScale);
    } else
        result.maximumScale = resultMaxZoom;
    result.maximumScale = max(result.minimumScale, result.maximumScale);

    // Resolve initial-scale value.
    result.initialScale = resultZoom;
    if (resultZoom == ViewportArguments::ValueAuto) {
        result.initialScale = initialViewportSize.width() / defaultWidth;
        if (resultWidth != ViewportArguments::ValueAuto)
            result.initialScale = initialViewportSize.width() / resultWidth;
        if (resultHeight != ViewportArguments::ValueAuto) {
            // if 'auto', the initial-scale will be negative here and thus ignored.
            result.initialScale = max<float>(result.initialScale, initialViewportSize.height() / resultHeight);
        }
    }

    // Constrain initial-scale value to minimum-scale/maximum-scale range.
    result.initialScale = min(result.maximumScale, max(result.minimumScale, result.initialScale));

    // Resolve width value.
    if (resultWidth == ViewportArguments::ValueAuto) {
        if (resultZoom == ViewportArguments::ValueAuto)
            resultWidth = defaultWidth;
        else if (resultHeight != ViewportArguments::ValueAuto)
            resultWidth = resultHeight * (initialViewportSize.width() / initialViewportSize.height());
        else
            resultWidth = initialViewportSize.width() / result.initialScale;
    }

    // Resolve height value.
    if (resultHeight == ViewportArguments::ValueAuto)
        resultHeight = resultWidth * (initialViewportSize.height() / initialViewportSize.width());

    if (type == ViewportArguments::ViewportMeta) {
        // Extend width and height to fill the visual viewport for the resolved initial-scale.
        resultWidth = max<float>(resultWidth, initialViewportSize.width() / result.initialScale);
        resultHeight = max<float>(resultHeight, initialViewportSize.height() / result.initialScale);
    }

    result.layoutSize.setWidth(resultWidth);
    result.layoutSize.setHeight(resultHeight);

    // FIXME: This might affect some ports, but is the right thing to do.
    // Only set initialScale to a value if it was explicitly set.
    // if (resultZoom == ViewportArguments::ValueAuto)
    //    result.initialScale = ViewportArguments::ValueAuto;

    result.userScalable = resultUserZoom;
    result.orientation = orientation;

    return result;
}

static FloatSize convertToUserSpace(const FloatSize& deviceSize, float devicePixelRatio)
{
    FloatSize result = deviceSize;
    if (devicePixelRatio != 1)
        result.scale(1 / devicePixelRatio);
    return result;
}

ViewportAttributes computeViewportAttributes(ViewportArguments args, int desktopWidth, int deviceWidth, int deviceHeight, float devicePixelRatio, IntSize visibleViewport)
{
    FloatSize initialViewportSize = convertToUserSpace(visibleViewport, devicePixelRatio);
    FloatSize deviceSize = convertToUserSpace(FloatSize(deviceWidth, deviceHeight), devicePixelRatio);

    return args.resolve(initialViewportSize, deviceSize, desktopWidth);
}

float computeMinimumScaleFactorForContentContained(const ViewportAttributes& result, const IntSize& visibleViewport, const IntSize& contentsSize)
{
    FloatSize viewportSize(visibleViewport);
    return max<float>(result.minimumScale, max(viewportSize.width() / contentsSize.width(), viewportSize.height() / contentsSize.height()));
}

void restrictMinimumScaleFactorToViewportSize(ViewportAttributes& result, IntSize visibleViewport, float devicePixelRatio)
{
    FloatSize viewportSize = convertToUserSpace(visibleViewport, devicePixelRatio);

    result.minimumScale = max<float>(result.minimumScale, max(viewportSize.width() / result.layoutSize.width(), viewportSize.height() / result.layoutSize.height()));
}

void restrictScaleFactorToInitialScaleIfNotUserScalable(ViewportAttributes& result)
{
    if (!result.userScalable)
        result.maximumScale = result.minimumScale = result.initialScale;
}

static float numericPrefix(const String& keyString, const String& valueString, Document* document, bool* ok = 0)
{
    size_t parsedLength;
    float value;
    if (valueString.is8Bit())
        value = charactersToFloat(valueString.characters8(), valueString.length(), parsedLength);
    else
        value = charactersToFloat(valueString.characters16(), valueString.length(), parsedLength);
    if (!parsedLength) {
        reportViewportWarning(document, UnrecognizedViewportArgumentValueError, valueString, keyString);
        if (ok)
            *ok = false;
        return 0;
    }
    if (parsedLength < valueString.length())
        reportViewportWarning(document, TruncatedViewportArgumentValueError, valueString, keyString);
    if (ok)
        *ok = true;
    return value;
}

static float findSizeValue(const String& keyString, const String& valueString, Document* document)
{
    // 1) Non-negative number values are translated to px lengths.
    // 2) Negative number values are translated to auto.
    // 3) device-width and device-height are used as keywords.
    // 4) Other keywords and unknown values translate to 0.0.

    if (equalIgnoringCase(valueString, "device-width"))
        return ViewportArguments::ValueDeviceWidth;
    if (equalIgnoringCase(valueString, "device-height"))
        return ViewportArguments::ValueDeviceHeight;

    float value = numericPrefix(keyString, valueString, document);

    if (value < 0)
        return ViewportArguments::ValueAuto;

    return value;
}

static float findScaleValue(const String& keyString, const String& valueString, Document* document)
{
    // 1) Non-negative number values are translated to <number> values.
    // 2) Negative number values are translated to auto.
    // 3) yes is translated to 1.0.
    // 4) device-width and device-height are translated to 10.0.
    // 5) no and unknown values are translated to 0.0

    if (equalIgnoringCase(valueString, "yes"))
        return 1;
    if (equalIgnoringCase(valueString, "no"))
        return 0;
    if (equalIgnoringCase(valueString, "device-width"))
        return 10;
    if (equalIgnoringCase(valueString, "device-height"))
        return 10;

    float value = numericPrefix(keyString, valueString, document);

    if (value < 0)
        return ViewportArguments::ValueAuto;

    if (value > 10.0)
        reportViewportWarning(document, MaximumScaleTooLargeError, String(), String());

    return value;
}

static float findUserScalableValue(const String& keyString, const String& valueString, Document* document)
{
    // yes and no are used as keywords.
    // Numbers >= 1, numbers <= -1, device-width and device-height are mapped to yes.
    // Numbers in the range <-1, 1>, and unknown values, are mapped to no.

    if (equalIgnoringCase(valueString, "yes"))
        return 1;
    if (equalIgnoringCase(valueString, "no"))
        return 0;
    if (equalIgnoringCase(valueString, "device-width"))
        return 1;
    if (equalIgnoringCase(valueString, "device-height"))
        return 1;

    float value = numericPrefix(keyString, valueString, document);

    if (fabs(value) < 1)
        return 0;

    return 1;
}

void setViewportFeature(const String& keyString, const String& valueString, Document* document, void* data)
{
    ViewportArguments* arguments = static_cast<ViewportArguments*>(data);

    if (keyString == "width")
        arguments->width = findSizeValue(keyString, valueString, document);
    else if (keyString == "height")
        arguments->height = findSizeValue(keyString, valueString, document);
    else if (keyString == "initial-scale")
        arguments->zoom = findScaleValue(keyString, valueString, document);
    else if (keyString == "minimum-scale")
        arguments->minZoom = findScaleValue(keyString, valueString, document);
    else if (keyString == "maximum-scale")
        arguments->maxZoom = findScaleValue(keyString, valueString, document);
    else if (keyString == "user-scalable")
        arguments->userZoom = findUserScalableValue(keyString, valueString, document);
    else
        reportViewportWarning(document, UnrecognizedViewportArgumentKeyError, keyString, String());
}

static const char* viewportErrorMessageTemplate(ViewportErrorCode errorCode)
{
    static const char* const errors[] = {
        "Viewport argument key \"%replacement1\" not recognized and ignored.",
        "Viewport argument value \"%replacement1\" for key \"%replacement2\" is invalid, and has been ignored.",
        "Viewport argument value \"%replacement1\" for key \"%replacement2\" was truncated to its numeric prefix.",
        "Viewport maximum-scale cannot be larger than 10.0. The maximum-scale will be set to 10.0."
    };

    return errors[errorCode];
}

static MessageLevel viewportErrorMessageLevel(ViewportErrorCode errorCode)
{
    switch (errorCode) {
    case TruncatedViewportArgumentValueError:
        return WarningMessageLevel;
    case UnrecognizedViewportArgumentKeyError:
    case UnrecognizedViewportArgumentValueError:
    case MaximumScaleTooLargeError:
        return ErrorMessageLevel;
    }

    ASSERT_NOT_REACHED();
    return ErrorMessageLevel;
}

void reportViewportWarning(Document* document, ViewportErrorCode errorCode, const String& replacement1, const String& replacement2)
{
    Frame* frame = document->frame();
    if (!frame)
        return;

    String message = viewportErrorMessageTemplate(errorCode);
    if (!replacement1.isNull())
        message.replace("%replacement1", replacement1);
    if (!replacement2.isNull())
        message.replace("%replacement2", replacement2);

    if ((errorCode == UnrecognizedViewportArgumentValueError || errorCode == TruncatedViewportArgumentValueError) && replacement1.find(';') != WTF::notFound)
        message.append(" Note that ';' is not a separator in viewport values. The list should be comma-separated.");

    // FIXME: This message should be moved off the console once a solution to https://bugs.webkit.org/show_bug.cgi?id=103274 exists.
    document->addConsoleMessage(RenderingMessageSource, viewportErrorMessageLevel(errorCode), message);
}

} // namespace WebCore

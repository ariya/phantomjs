/*
 * CSS Media Query Evaluator
 *
 * Copyright (C) 2006 Kimmo Kinnunen <kimmo.t.kinnunen@nokia.com>.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MediaQueryEvaluator.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "CSSPrimitiveValue.h"
#include "CSSStyleSelector.h"
#include "CSSValueList.h"
#include "FloatRect.h"
#include "Frame.h"
#include "FrameView.h"
#include "IntRect.h"
#include "MediaFeatureNames.h"
#include "MediaList.h"
#include "MediaQuery.h"
#include "MediaQueryExp.h"
#include "NodeRenderStyle.h"
#include "Page.h"
#include "RenderView.h"
#include "RenderStyle.h"
#include "PlatformScreen.h"
#include <wtf/HashMap.h>

#if ENABLE(3D_RENDERING) && USE(ACCELERATED_COMPOSITING)
#include "RenderLayerCompositor.h"
#endif

namespace WebCore {

using namespace MediaFeatureNames;

enum MediaFeaturePrefix { MinPrefix, MaxPrefix, NoPrefix };

typedef bool (*EvalFunc)(CSSValue*, RenderStyle*, Frame*, MediaFeaturePrefix);
typedef HashMap<AtomicStringImpl*, EvalFunc> FunctionMap;
static FunctionMap* gFunctionMap;

/*
 * FIXME: following media features are not implemented: color_index, scan, resolution
 *
 * color_index, min-color-index, max_color_index: It's unknown how to retrieve
 * the information if the display mode is indexed
 * scan: The "scan" media feature describes the scanning process of
 * tv output devices. It's unknown how to retrieve this information from
 * the platform
 * resolution, min-resolution, max-resolution: css parser doesn't seem to
 * support CSS_DIMENSION
 */

MediaQueryEvaluator::MediaQueryEvaluator(bool mediaFeatureResult)
    : m_frame(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
{
}

MediaQueryEvaluator:: MediaQueryEvaluator(const String& acceptedMediaType, bool mediaFeatureResult)
    : m_mediaType(acceptedMediaType)
    , m_frame(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
{
}

MediaQueryEvaluator:: MediaQueryEvaluator(const char* acceptedMediaType, bool mediaFeatureResult)
    : m_mediaType(acceptedMediaType)
    , m_frame(0)
    , m_style(0)
    , m_expResult(mediaFeatureResult)
{
}

MediaQueryEvaluator:: MediaQueryEvaluator(const String& acceptedMediaType, Frame* frame, RenderStyle* style)
    : m_mediaType(acceptedMediaType)
    , m_frame(frame)
    , m_style(style)
    , m_expResult(false) // doesn't matter when we have m_frame and m_style
{
}

MediaQueryEvaluator::~MediaQueryEvaluator()
{
}

bool MediaQueryEvaluator::mediaTypeMatch(const String& mediaTypeToMatch) const
{
    return mediaTypeToMatch.isEmpty()
        || equalIgnoringCase(mediaTypeToMatch, "all")
        || equalIgnoringCase(mediaTypeToMatch, m_mediaType);
}

bool MediaQueryEvaluator::mediaTypeMatchSpecific(const char* mediaTypeToMatch) const
{
    // Like mediaTypeMatch, but without the special cases for "" and "all".
    ASSERT(mediaTypeToMatch);
    ASSERT(mediaTypeToMatch[0] != '\0');
    ASSERT(!equalIgnoringCase(mediaTypeToMatch, String("all")));
    return equalIgnoringCase(mediaTypeToMatch, m_mediaType);
}

static bool applyRestrictor(MediaQuery::Restrictor r, bool value)
{
    return r == MediaQuery::Not ? !value : value;
}

bool MediaQueryEvaluator::eval(const MediaList* mediaList, CSSStyleSelector* styleSelector) const
{
    if (!mediaList)
        return true;

    const Vector<MediaQuery*>& queries = mediaList->mediaQueries();
    if (!queries.size())
        return true; // empty query list evaluates to true

    // iterate over queries, stop if any of them eval to true (OR semantics)
    bool result = false;
    for (size_t i = 0; i < queries.size() && !result; ++i) {
        MediaQuery* query = queries.at(i);

        if (query->ignored())
            continue;

        if (mediaTypeMatch(query->mediaType())) {
            const Vector<OwnPtr<MediaQueryExp> >* exps = query->expressions();
            // iterate through expressions, stop if any of them eval to false
            // (AND semantics)
            size_t j = 0;
            for (; j < exps->size(); ++j) {
                bool exprResult = eval(exps->at(j).get());
                if (styleSelector && exps->at(j)->isViewportDependent())
                    styleSelector->addViewportDependentMediaQueryResult(exps->at(j).get(), exprResult);
                if (!exprResult)
                    break;
            }

            // assume true if we are at the end of the list,
            // otherwise assume false
            result = applyRestrictor(query->restrictor(), exps->size() == j);
        } else
            result = applyRestrictor(query->restrictor(), false);
    }

    return result;
}

static bool parseAspectRatio(CSSValue* value, int& h, int& v)
{
    if (value->isValueList()) {
        CSSValueList* valueList = static_cast<CSSValueList*>(value);
        if (valueList->length() == 3) {
            CSSValue* i0 = valueList->itemWithoutBoundsCheck(0);
            CSSValue* i1 = valueList->itemWithoutBoundsCheck(1);
            CSSValue* i2 = valueList->itemWithoutBoundsCheck(2);
            if (i0->isPrimitiveValue() && static_cast<CSSPrimitiveValue*>(i0)->primitiveType() == CSSPrimitiveValue::CSS_NUMBER
                && i1->isPrimitiveValue() && static_cast<CSSPrimitiveValue*>(i1)->primitiveType() == CSSPrimitiveValue::CSS_STRING
                && i2->isPrimitiveValue() && static_cast<CSSPrimitiveValue*>(i2)->primitiveType() == CSSPrimitiveValue::CSS_NUMBER) {
                String str = static_cast<CSSPrimitiveValue*>(i1)->getStringValue();
                if (!str.isNull() && str.length() == 1 && str[0] == '/') {
                    h = static_cast<CSSPrimitiveValue*>(i0)->getIntValue(CSSPrimitiveValue::CSS_NUMBER);
                    v = static_cast<CSSPrimitiveValue*>(i2)->getIntValue(CSSPrimitiveValue::CSS_NUMBER);
                    return true;
                }
            }
        }
    }
    return false;
}

template<typename T>
bool compareValue(T a, T b, MediaFeaturePrefix op)
{
    switch (op) {
    case MinPrefix:
        return a >= b;
    case MaxPrefix:
        return a <= b;
    case NoPrefix:
        return a == b;
    }
    return false;
}

static bool numberValue(CSSValue* value, float& result)
{
    if (value->isPrimitiveValue()
        && static_cast<CSSPrimitiveValue*>(value)->primitiveType() == CSSPrimitiveValue::CSS_NUMBER) {
        result = static_cast<CSSPrimitiveValue*>(value)->getFloatValue(CSSPrimitiveValue::CSS_NUMBER);
        return true;
    }
    return false;
}

static bool colorMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op)
{
    int bitsPerComponent = screenDepthPerComponent(frame->page()->mainFrame()->view());
    float number;
    if (value)
        return numberValue(value, number) && compareValue(bitsPerComponent, static_cast<int>(number), op);

    return bitsPerComponent != 0;
}

static bool monochromeMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op)
{
    if (!screenIsMonochrome(frame->page()->mainFrame()->view())) {
        if (value) {
            float number;
            return numberValue(value, number) && compareValue(0, static_cast<int>(number), op);
        }
        return false;
    }

    return colorMediaFeatureEval(value, style, frame, op);
}

static bool orientationMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix)
{
    // A missing parameter should fail
    if (!value)
        return false;

    FrameView* view = frame->view();
    int width = view->layoutWidth();
    int height = view->layoutHeight();
    if (width > height) // Square viewport is portrait
        return "landscape" == static_cast<CSSPrimitiveValue*>(value)->getStringValue();
    return "portrait" == static_cast<CSSPrimitiveValue*>(value)->getStringValue();
}

static bool aspect_ratioMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op)
{
    if (value) {
        FrameView* view = frame->view();
        int width = view->layoutWidth();
        int height = view->layoutHeight();
        int h = 0;
        int v = 0;
        if (parseAspectRatio(value, h, v))
            return v != 0 && compareValue(width * v, height * h, op);
        return false;
    }

    // ({,min-,max-}aspect-ratio)
    // assume if we have a device, its aspect ratio is non-zero
    return true;
}

static bool device_aspect_ratioMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op)
{
    if (value) {
        FloatRect sg = screenRect(frame->page()->mainFrame()->view());
        int h = 0;
        int v = 0;
        if (parseAspectRatio(value, h, v))
            return v != 0  && compareValue(static_cast<int>(sg.width()) * v, static_cast<int>(sg.height()) * h, op);
        return false;
    }

    // ({,min-,max-}device-aspect-ratio)
    // assume if we have a device, its aspect ratio is non-zero
    return true;
}

static bool device_pixel_ratioMediaFeatureEval(CSSValue *value, RenderStyle*, Frame* frame, MediaFeaturePrefix op)
{
    if (value)
        return value->isPrimitiveValue() && compareValue(frame->page()->chrome()->scaleFactor(), static_cast<CSSPrimitiveValue*>(value)->getFloatValue(), op);

    return frame->page()->chrome()->scaleFactor() != 0;
}

static bool gridMediaFeatureEval(CSSValue* value, RenderStyle*, Frame*, MediaFeaturePrefix op)
{
    // if output device is bitmap, grid: 0 == true
    // assume we have bitmap device
    float number;
    if (value && numberValue(value, number))
        return compareValue(static_cast<int>(number), 0, op);
    return false;
}

static bool device_heightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op)
{
    if (value) {
        FloatRect sg = screenRect(frame->page()->mainFrame()->view());
        RenderStyle* rootStyle = frame->document()->documentElement()->renderStyle();
        return value->isPrimitiveValue() && compareValue(static_cast<int>(sg.height()), static_cast<CSSPrimitiveValue*>(value)->computeLengthInt(style, rootStyle), op);
    }
    // ({,min-,max-}device-height)
    // assume if we have a device, assume non-zero
    return true;
}

static bool device_widthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op)
{
    if (value) {
        FloatRect sg = screenRect(frame->page()->mainFrame()->view());
        RenderStyle* rootStyle = frame->document()->documentElement()->renderStyle();
        return value->isPrimitiveValue() && compareValue(static_cast<int>(sg.width()), static_cast<CSSPrimitiveValue*>(value)->computeLengthInt(style, rootStyle), op);
    }
    // ({,min-,max-}device-width)
    // assume if we have a device, assume non-zero
    return true;
}

static bool heightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op)
{
    FrameView* view = frame->view();
    RenderStyle* rootStyle = frame->document()->documentElement()->renderStyle();

    if (value)
        return value->isPrimitiveValue() && compareValue(view->layoutHeight(), static_cast<CSSPrimitiveValue*>(value)->computeLengthInt(style, rootStyle), op);

    return view->layoutHeight() != 0;
}

static bool widthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix op)
{
    FrameView* view = frame->view();
    RenderStyle* rootStyle = frame->document()->documentElement()->renderStyle();

    if (value)
        return value->isPrimitiveValue() && compareValue(view->layoutWidth(), static_cast<CSSPrimitiveValue*>(value)->computeLengthInt(style, rootStyle), op);

    return view->layoutWidth() != 0;
}

// rest of the functions are trampolines which set the prefix according to the media feature expression used

static bool min_colorMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return colorMediaFeatureEval(value, style, frame, MinPrefix);
}

static bool max_colorMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return colorMediaFeatureEval(value, style, frame, MaxPrefix);
}

static bool min_monochromeMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return monochromeMediaFeatureEval(value, style, frame, MinPrefix);
}

static bool max_monochromeMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return monochromeMediaFeatureEval(value, style, frame, MaxPrefix);
}

static bool min_aspect_ratioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return aspect_ratioMediaFeatureEval(value, style, frame, MinPrefix);
}

static bool max_aspect_ratioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return aspect_ratioMediaFeatureEval(value, style, frame, MaxPrefix);
}

static bool min_device_aspect_ratioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return device_aspect_ratioMediaFeatureEval(value, style, frame, MinPrefix);
}

static bool max_device_aspect_ratioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return device_aspect_ratioMediaFeatureEval(value, style, frame, MaxPrefix);
}

static bool min_device_pixel_ratioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return device_pixel_ratioMediaFeatureEval(value, style, frame, MinPrefix);
}

static bool max_device_pixel_ratioMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return device_pixel_ratioMediaFeatureEval(value, style, frame, MaxPrefix);
}

static bool min_heightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return heightMediaFeatureEval(value, style, frame, MinPrefix);
}

static bool max_heightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return heightMediaFeatureEval(value, style, frame, MaxPrefix);
}

static bool min_widthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return widthMediaFeatureEval(value, style, frame, MinPrefix);
}

static bool max_widthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return widthMediaFeatureEval(value, style, frame, MaxPrefix);
}

static bool min_device_heightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return device_heightMediaFeatureEval(value, style, frame, MinPrefix);
}

static bool max_device_heightMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return device_heightMediaFeatureEval(value, style, frame, MaxPrefix);
}

static bool min_device_widthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return device_widthMediaFeatureEval(value, style, frame, MinPrefix);
}

static bool max_device_widthMediaFeatureEval(CSSValue* value, RenderStyle* style, Frame* frame, MediaFeaturePrefix)
{
    return device_widthMediaFeatureEval(value, style, frame, MaxPrefix);
}

static bool animationMediaFeatureEval(CSSValue* value, RenderStyle*, Frame*, MediaFeaturePrefix op)
{
    if (value) {
        float number;
        return numberValue(value, number) && compareValue(1, static_cast<int>(number), op);
    }
    return true;
}

static bool transitionMediaFeatureEval(CSSValue* value, RenderStyle*, Frame*, MediaFeaturePrefix op)
{
    if (value) {
        float number;
        return numberValue(value, number) && compareValue(1, static_cast<int>(number), op);
    }
    return true;
}

static bool transform_2dMediaFeatureEval(CSSValue* value, RenderStyle*, Frame*, MediaFeaturePrefix op)
{
    if (value) {
        float number;
        return numberValue(value, number) && compareValue(1, static_cast<int>(number), op);
    }
    return true;
}

static bool transform_3dMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op)
{
    bool returnValueIfNoParameter;
    int have3dRendering;

#if ENABLE(3D_RENDERING)
    bool threeDEnabled = false;
#if USE(ACCELERATED_COMPOSITING)
    if (RenderView* view = frame->contentRenderer())
        threeDEnabled = view->compositor()->canRender3DTransforms();
#endif

    returnValueIfNoParameter = threeDEnabled;
    have3dRendering = threeDEnabled ? 1 : 0;
#else
    UNUSED_PARAM(frame);
    returnValueIfNoParameter = false;
    have3dRendering = 0;
#endif

    if (value) {
        float number;
        return numberValue(value, number) && compareValue(have3dRendering, static_cast<int>(number), op);
    }
    return returnValueIfNoParameter;
}

static bool view_modeMediaFeatureEval(CSSValue* value, RenderStyle*, Frame* frame, MediaFeaturePrefix op)
{
    UNUSED_PARAM(op);
    if (!value)
        return true;
    return Page::stringToViewMode(static_cast<CSSPrimitiveValue*>(value)->getStringValue()) == frame->page()->viewMode();
}

static void createFunctionMap()
{
    // Create the table.
    gFunctionMap = new FunctionMap;
#define ADD_TO_FUNCTIONMAP(name, str)  \
    gFunctionMap->set(name##MediaFeature.impl(), name##MediaFeatureEval);
    CSS_MEDIAQUERY_NAMES_FOR_EACH_MEDIAFEATURE(ADD_TO_FUNCTIONMAP);
#undef ADD_TO_FUNCTIONMAP
}

bool MediaQueryEvaluator::eval(const MediaQueryExp* expr) const
{
    if (!m_frame || !m_style)
        return m_expResult;

    if (!expr->isValid())
        return false;

    if (!gFunctionMap)
        createFunctionMap();

    // call the media feature evaluation function. Assume no prefix
    // and let trampoline functions override the prefix if prefix is
    // used
    EvalFunc func = gFunctionMap->get(expr->mediaFeature().impl());
    if (func)
        return func(expr->value(), m_style.get(), m_frame, NoPrefix);

    return false;
}

} // namespace

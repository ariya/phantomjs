/*
 * This file is part of the WebKit project.
 *
 * Copyright (C) 2006, 2007 Apple Computer, Inc.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
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
#include "RenderThemeWinCE.h"

#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "Document.h"
#include "FontMetrics.h"
#include "GraphicsContext.h"
#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"
#endif
#include "NotImplemented.h"
#include "PaintInfo.h"

#include <windows.h>

/* 
 * The following constants are used to determine how a widget is drawn using
 * Windows' Theme API. For more information on theme parts and states see
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/commctls/userex/topics/partsandstates.asp
 */
#define THEME_COLOR 204
#define THEME_FONT  210

// Generic state constants
#define TS_NORMAL    1
#define TS_HOVER     2
#define TS_ACTIVE    3
#define TS_DISABLED  4
#define TS_FOCUSED   5

// Button constants
#define BP_BUTTON    1
#define BP_RADIO     2
#define BP_CHECKBOX  3

// Textfield constants
#define TFP_TEXTFIELD 1
#define TFS_READONLY  6

typedef HANDLE (WINAPI*openThemeDataPtr)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (WINAPI*closeThemeDataPtr)(HANDLE hTheme);
typedef HRESULT (WINAPI*drawThemeBackgroundPtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                          int iStateId, const RECT *pRect,
                                          const RECT* pClipRect);
typedef HRESULT (WINAPI*drawThemeEdgePtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                          int iStateId, const RECT *pRect,
                                          unsigned uEdge, unsigned uFlags,
                                          const RECT* pClipRect);
typedef HRESULT (WINAPI*getThemeContentRectPtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                          int iStateId, const RECT* pRect,
                                          RECT* pContentRect);
typedef HRESULT (WINAPI*getThemePartSizePtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                       int iStateId, RECT* prc, int ts,
                                       SIZE* psz);
typedef HRESULT (WINAPI*getThemeSysFontPtr)(HANDLE hTheme, int iFontId, OUT LOGFONT* pFont);
typedef HRESULT (WINAPI*getThemeColorPtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                   int iStateId, int iPropId, OUT COLORREF* pFont);

namespace WebCore {

static const int dropDownButtonWidth = 17;
static const int trackWidth = 4;

PassRefPtr<RenderTheme> RenderThemeWinCE::create()
{
    return adoptRef(new RenderThemeWinCE);
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    static RenderTheme* winceTheme = RenderThemeWinCE::create().leakRef();
    return winceTheme;
}

RenderThemeWinCE::RenderThemeWinCE()
{
}

RenderThemeWinCE::~RenderThemeWinCE()
{
}

Color RenderThemeWinCE::platformActiveSelectionBackgroundColor() const
{
    COLORREF color = GetSysColor(COLOR_HIGHLIGHT);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color), 255);
}

Color RenderThemeWinCE::platformInactiveSelectionBackgroundColor() const
{
    COLORREF color = GetSysColor(COLOR_GRAYTEXT);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color), 255);
}

Color RenderThemeWinCE::platformActiveSelectionForegroundColor() const
{
    COLORREF color = GetSysColor(COLOR_HIGHLIGHTTEXT);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color), 255);
}

Color RenderThemeWinCE::platformInactiveSelectionForegroundColor() const
{
    return Color::white;
}

bool RenderThemeWinCE::supportsFocus(ControlPart appearance) const
{
    switch (appearance) {
    case PushButtonPart:
    case ButtonPart:
    case TextFieldPart:
    case TextAreaPart:
        return true;
    default:
        return false;
    }

    return false;
}

bool RenderThemeWinCE::supportsFocusRing(const RenderStyle *style) const
{
    return supportsFocus(style->appearance());
}

unsigned RenderThemeWinCE::determineClassicState(RenderObject* o)
{
    unsigned result = 0;
    if (!isEnabled(o) || isReadOnlyControl(o))
        result = DFCS_INACTIVE;
    else if (isPressed(o)) // Active supersedes hover
        result = DFCS_PUSHED;

    if (isChecked(o))
        result |= DFCS_CHECKED;
    return result;
}

ThemeData RenderThemeWinCE::getThemeData(RenderObject* o)
{
    ThemeData result;
    switch (o->style()->appearance()) {
    case PushButtonPart:
    case ButtonPart:
        result.m_part = BP_BUTTON;
        result.m_classicState = DFCS_BUTTONPUSH;
        break;
    case CheckboxPart:
        result.m_part = BP_CHECKBOX;
        result.m_classicState = DFCS_BUTTONCHECK;
        break;
    case RadioPart:
        result.m_part = BP_RADIO;
        result.m_classicState = DFCS_BUTTONRADIO;
        break;
    case ListboxPart:
    case MenulistPart:
    case TextFieldPart:
    case TextAreaPart:
        result.m_part = TFP_TEXTFIELD;
        break;
    }

    result.m_classicState |= determineClassicState(o);

    return result;
}

bool RenderThemeWinCE::paintButton(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    // Get the correct theme data for a button
    ThemeData themeData = getThemeData(o);

    // Now paint the button.
    i.context->drawFrameControl(r, DFC_BUTTON, themeData.m_classicState);
    if (isFocused(o)) {
        if (themeData.m_part == BP_BUTTON) {
            IntRect focusRect(r);
            focusRect.inflate(-2);
            i.context->drawFocusRect(focusRect);
        } else
            i.context->drawFocusRect(r);
    }

    return false;
}

void RenderThemeWinCE::setCheckboxSize(RenderStyle* style) const
{
    // If the width and height are both specified, then we have nothing to do.
    if (!style->width().isIntrinsicOrAuto() && !style->height().isAuto())
        return;

    // FIXME:  A hard-coded size of 13 is used.  This is wrong but necessary for now.  It matches Firefox.
    // At different DPI settings on Windows, querying the theme gives you a larger size that accounts for
    // the higher DPI.  Until our entire engine honors a DPI setting other than 96, we can't rely on the theme's
    // metrics.
    if (style->width().isIntrinsicOrAuto())
        style->setWidth(Length(13, Fixed));
    if (style->height().isAuto())
        style->setHeight(Length(13, Fixed));
}

bool RenderThemeWinCE::paintTextField(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    // Get the correct theme data for a textfield
    ThemeData themeData = getThemeData(o);

    // Now paint the text field.
    i.context->paintTextField(r, themeData.m_classicState);

    return false;
}

void RenderThemeWinCE::adjustMenuListStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    style->resetBorder();
    adjustMenuListButtonStyle(styleResolver, style, e);
}

bool RenderThemeWinCE::paintMenuList(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    paintTextField(o, i, r);
    paintMenuListButton(o, i, r);
    return true;
}

bool RenderThemeWinCE::paintMenuListButton(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    IntRect buttonRect(r.maxX() - dropDownButtonWidth - 1, r.y(), dropDownButtonWidth, r.height());
    buttonRect.inflateY(-1);
    i.context->drawFrameControl(buttonRect, DFC_SCROLL, DFCS_SCROLLCOMBOBOX | determineClassicState(o));
    return true;
}

void RenderThemeWinCE::systemFont(CSSValueID, FontDescription& fontDescription) const
{
    notImplemented();
}

void RenderThemeWinCE::themeChanged()
{
}

String RenderThemeWinCE::extraDefaultStyleSheet()
{
    notImplemented();
    return String();
}

String RenderThemeWinCE::extraQuirksStyleSheet()
{
    notImplemented();
    return String();
}

bool RenderThemeWinCE::supportsHover(const RenderStyle*) const
{
    return false;
}

// Map a CSSValue* system color to an index understood by GetSysColor
static int cssValueIdToSysColorIndex(CSSValueID cssValueId)
{
    switch (cssValueId) {
    case CSSValueActiveborder: return COLOR_ACTIVEBORDER;
    case CSSValueActivecaption: return COLOR_ACTIVECAPTION;
    case CSSValueAppworkspace: return COLOR_APPWORKSPACE;
    case CSSValueBackground: return COLOR_BACKGROUND;
    case CSSValueButtonface: return COLOR_BTNFACE;
    case CSSValueButtonhighlight: return COLOR_BTNHIGHLIGHT;
    case CSSValueButtonshadow: return COLOR_BTNSHADOW;
    case CSSValueButtontext: return COLOR_BTNTEXT;
    case CSSValueCaptiontext: return COLOR_CAPTIONTEXT;
    case CSSValueGraytext: return COLOR_GRAYTEXT;
    case CSSValueHighlight: return COLOR_HIGHLIGHT;
    case CSSValueHighlighttext: return COLOR_HIGHLIGHTTEXT;
    case CSSValueInactiveborder: return COLOR_INACTIVEBORDER;
    case CSSValueInactivecaption: return COLOR_INACTIVECAPTION;
    case CSSValueInactivecaptiontext: return COLOR_INACTIVECAPTIONTEXT;
    case CSSValueInfobackground: return COLOR_INFOBK;
    case CSSValueInfotext: return COLOR_INFOTEXT;
    case CSSValueMenu: return COLOR_MENU;
    case CSSValueMenutext: return COLOR_MENUTEXT;
    case CSSValueScrollbar: return COLOR_SCROLLBAR;
    case CSSValueThreeddarkshadow: return COLOR_3DDKSHADOW;
    case CSSValueThreedface: return COLOR_3DFACE;
    case CSSValueThreedhighlight: return COLOR_3DHIGHLIGHT;
    case CSSValueThreedlightshadow: return COLOR_3DLIGHT;
    case CSSValueThreedshadow: return COLOR_3DSHADOW;
    case CSSValueWindow: return COLOR_WINDOW;
    case CSSValueWindowframe: return COLOR_WINDOWFRAME;
    case CSSValueWindowtext: return COLOR_WINDOWTEXT;
    default: return -1; // Unsupported CSSValue
    }
}

Color RenderThemeWinCE::systemColor(CSSValueID cssValueId) const
{
    int sysColorIndex = cssValueIdToSysColorIndex(cssValueId);
    if (sysColorIndex == -1)
        return RenderTheme::systemColor(cssValueId);

    COLORREF color = GetSysColor(sysColorIndex);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color));
}

const int sliderThumbWidth = 7;
const int sliderThumbHeight = 15;

void RenderThemeWinCE::adjustSliderThumbSize(RenderStyle* style, Element*) const
{
    if (style->appearance() == SliderThumbVerticalPart) {
        style->setWidth(Length(sliderThumbHeight, Fixed));
        style->setHeight(Length(sliderThumbWidth, Fixed));
    } else if (style->appearance() == SliderThumbHorizontalPart) {
        style->setWidth(Length(sliderThumbWidth, Fixed));
        style->setHeight(Length(sliderThumbHeight, Fixed));
    }
}

#if 0
void RenderThemeWinCE::adjustButtonInnerStyle(RenderStyle* style) const
{
    // This inner padding matches Firefox.
    style->setPaddingTop(Length(1, Fixed));
    style->setPaddingRight(Length(3, Fixed));
    style->setPaddingBottom(Length(1, Fixed));
    style->setPaddingLeft(Length(3, Fixed));
}

void RenderThemeWinCE::adjustSearchFieldStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    // Override padding size to match AppKit text positioning.
    const int padding = 1;
    style->setPaddingLeft(Length(padding, Fixed));
    style->setPaddingRight(Length(padding, Fixed));
    style->setPaddingTop(Length(padding, Fixed));
    style->setPaddingBottom(Length(padding, Fixed));
}
#endif

bool RenderThemeWinCE::paintSearchField(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    return paintTextField(o, i, r);
}

bool RenderThemeWinCE::paintSearchFieldCancelButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    Color buttonColor = (o->node() && o->node()->isElementNode() && toElement(o->node())->active()) ? Color(138, 138, 138) : Color(186, 186, 186);

    IntSize cancelSize(10, 10);
    IntSize cancelRadius(cancelSize.width() / 2, cancelSize.height() / 2);
    int x = r.x() + (r.width() - cancelSize.width()) / 2;
    int y = r.y() + (r.height() - cancelSize.height()) / 2 + 1;
    IntRect cancelBounds(IntPoint(x, y), cancelSize);
    paintInfo.context->save();
    paintInfo.context->clipRoundedRect(RoundedRect(cancelBounds, cancelRadius, cancelRadius, cancelRadius, cancelRadius));
    paintInfo.context->fillRect(cancelBounds, buttonColor, ColorSpaceDeviceRGB);

    // Draw the 'x'
    IntSize xSize(3, 3);
    IntRect xBounds(cancelBounds.location() + IntSize(3, 3), xSize);
    paintInfo.context->setStrokeColor(Color::white, ColorSpaceDeviceRGB);
    paintInfo.context->drawLine(xBounds.location(),  xBounds.location() + xBounds.size());
    paintInfo.context->drawLine(IntPoint(xBounds.maxX(), xBounds.y()),  IntPoint(xBounds.x(), xBounds.maxY()));

    paintInfo.context->restore();
    return false;
}

void RenderThemeWinCE::adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    IntSize cancelSize(13, 11);
    style->setWidth(Length(cancelSize.width(), Fixed));
    style->setHeight(Length(cancelSize.height(), Fixed));
}

void RenderThemeWinCE::adjustSearchFieldDecorationStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    IntSize emptySize(1, 11);
    style->setWidth(Length(emptySize.width(), Fixed));
    style->setHeight(Length(emptySize.height(), Fixed));
}

void RenderThemeWinCE::adjustSearchFieldResultsDecorationStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    IntSize magnifierSize(15, 11);
    style->setWidth(Length(magnifierSize.width(), Fixed));
    style->setHeight(Length(magnifierSize.height(), Fixed));
}

bool RenderThemeWinCE::paintSearchFieldResultsDecoration(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    notImplemented();
    return false;
}

void RenderThemeWinCE::adjustSearchFieldResultsButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    IntSize magnifierSize(15, 11);
    style->setWidth(Length(magnifierSize.width(), Fixed));
    style->setHeight(Length(magnifierSize.height(), Fixed));
}

bool RenderThemeWinCE::paintSearchFieldResultsButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    paintSearchFieldResultsDecoration(o, paintInfo, r);
    return false;
}

void RenderThemeWinCE::adjustMenuListButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // These are the paddings needed to place the text correctly in the <select> box
    const int dropDownBoxPaddingTop    = 2;
    const int dropDownBoxPaddingRight  = style->direction() == LTR ? 4 + dropDownButtonWidth : 4;
    const int dropDownBoxPaddingBottom = 2;
    const int dropDownBoxPaddingLeft   = style->direction() == LTR ? 4 : 4 + dropDownButtonWidth;
    // The <select> box must be at least 12px high for the button to render nicely on Windows
    const int dropDownBoxMinHeight = 12;

    // Position the text correctly within the select box and make the box wide enough to fit the dropdown button
    style->setPaddingTop(Length(dropDownBoxPaddingTop, Fixed));
    style->setPaddingRight(Length(dropDownBoxPaddingRight, Fixed));
    style->setPaddingBottom(Length(dropDownBoxPaddingBottom, Fixed));
    style->setPaddingLeft(Length(dropDownBoxPaddingLeft, Fixed));

    // Height is locked to auto
    style->setHeight(Length(Auto));

    // Calculate our min-height
    int minHeight = style->fontMetrics().height();
    minHeight = max(minHeight, dropDownBoxMinHeight);

    style->setMinHeight(Length(minHeight, Fixed));

    // White-space is locked to pre
    style->setWhiteSpace(PRE);

    DWORD colorMenu = GetSysColor(COLOR_MENU);
    DWORD colorMenuText = GetSysColor(COLOR_MENUTEXT);
    Color bgColor(GetRValue(colorMenu), GetGValue(colorMenu), GetBValue(colorMenu), 255);
    Color textColor(GetRValue(colorMenuText), GetGValue(colorMenuText), GetBValue(colorMenuText), 255);
    if (bgColor == textColor)
        textColor.setRGB((~bgColor.rgb()) | 0xFF000000);
    style->clearBackgroundLayers();
    style->accessBackgroundLayers()->setClip(ContentFillBox);
    style->setBackgroundColor(bgColor);
    style->setColor(textColor);
}

#if ENABLE(VIDEO)
// Attempt to retrieve a HTMLMediaElement from a Node. Returns 0 if one cannot be found.
static HTMLMediaElement* mediaElementParent(Node* node)
{
    if (!node)
        return 0;
    Node* mediaNode = node->shadowHost();
    if (!mediaNode)
        mediaNode = node;
    if (!mediaNode || !mediaNode->isElementNode() || !toElement(mediaNode)->isMediaElement())
        return 0;

    return toHTMLMediaElement(mediaNode);
}
#endif

bool RenderThemeWinCE::paintSliderTrack(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    bool rc = RenderTheme::paintSliderTrack(o, i, r);
    IntPoint left = IntPoint(r.x() + 2, (r.y() + r.maxY()) / 2);
    i.context->save();
    i.context->setStrokeColor(Color::gray, ColorSpaceDeviceRGB);
    i.context->setFillColor(Color::gray, ColorSpaceDeviceRGB);
    i.context->fillRect(r);
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = mediaElementParent(o->node());
    if (mediaElement) {
        i.context->setStrokeColor(Color(0, 0xff, 0));
        IntPoint right = IntPoint(left.x() + mediaElement->percentLoaded() * (r.maxX() - r.x() - 4), (r.y() + r.maxY()) / 2);
        i.context->drawLine(left, right);
        left = right;
    }
#endif
    i.context->setStrokeColor(Color::black, ColorSpaceDeviceRGB);
    i.context->drawLine(left, IntPoint(r.maxX() - 2, left.y()));
    i.context->restore();
    return rc;
}

bool RenderThemeWinCE::paintSliderThumb(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    bool rc = RenderTheme::paintSliderThumb(o, i, r);
    i.context->save();
    i.context->setStrokeColor(Color::black, ColorSpaceDeviceRGB);
    i.context->setFillColor(Color::black, ColorSpaceDeviceRGB);
#if ENABLE(VIDEO)
    HTMLMediaElement* mediaElement = mediaElementParent(o->node());
    if (mediaElement) {
        float pt = (mediaElement->currentTime() - mediaElement->startTime()) / mediaElement->duration();
        FloatRect intRect = r;
        intRect.setX(intRect.x() + intRect.width() * pt - 2);
        intRect.setWidth(5);
        i.context->fillRect(intRect);
    }
#endif
    i.context->restore();
    return rc;
}

void RenderThemeWinCE::adjustSearchFieldStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    const int padding = 1;
    style->setPaddingLeft(Length(padding, Fixed));
    style->setPaddingRight(Length(padding, Fixed));
    style->setPaddingTop(Length(padding, Fixed));
    style->setPaddingBottom(Length(padding, Fixed));
}

#if ENABLE(VIDEO)

bool RenderThemeWinCE::paintMediaFullscreenButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    bool rc = paintButton(o, paintInfo, r);
    FloatRect imRect = r;
    imRect.inflate(-2);
    paintInfo.context->save();
    paintInfo.context->setStrokeColor(Color::black);
    paintInfo.context->setFillColor(Color::gray);
    paintInfo.context->fillRect(imRect);
    paintInfo.context->restore();
    return rc;
}

bool RenderThemeWinCE::paintMediaMuteButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    bool rc = paintButton(o, paintInfo, r);
    HTMLMediaElement* mediaElement = mediaElementParent(o->node());
    bool muted = !mediaElement || mediaElement->muted();
    FloatRect imRect = r;
    imRect.inflate(-2);
    paintInfo.context->save();
    paintInfo.context->setStrokeColor(Color::black);
    paintInfo.context->setFillColor(Color::black);
    FloatPoint pts[6] = {
        FloatPoint(imRect.x() + 1, imRect.y() + imRect.height() / 3.0),
        FloatPoint(imRect.x() + 1 + imRect.width() / 2.0, imRect.y() + imRect.height() / 3.0),
        FloatPoint(imRect.maxX() - 1, imRect.y()),
        FloatPoint(imRect.maxX() - 1, imRect.maxY()),
        FloatPoint(imRect.x() + 1 + imRect.width() / 2.0, imRect.y() + 2.0 * imRect.height() / 3.0),
        FloatPoint(imRect.x() + 1, imRect.y() + 2.0 * imRect.height() / 3.0)
    };
    paintInfo.context->drawConvexPolygon(6, pts);
    if (muted)
        paintInfo.context->drawLine(IntPoint(imRect.maxX(), imRect.y()), IntPoint(imRect.x(), imRect.maxY()));
    paintInfo.context->restore();
    return rc;
}

bool RenderThemeWinCE::paintMediaPlayButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    bool rc = paintButton(o, paintInfo, r);
    FloatRect imRect = r;
    imRect.inflate(-3);
    paintInfo.context->save();
    paintInfo.context->setStrokeColor(Color::black);
    paintInfo.context->setFillColor(Color::black);
    HTMLMediaElement* mediaElement = mediaElementParent(o->node());
    bool paused = !mediaElement || mediaElement->paused();
    if (paused) {
        float width = imRect.width();
        imRect.setWidth(width / 3.0);
        paintInfo.context->fillRect(imRect);
        imRect.move(2.0 * width / 3.0, 0);
        paintInfo.context->fillRect(imRect);
    } else {
        FloatPoint pts[3] = { FloatPoint(imRect.x(), imRect.y()), FloatPoint(imRect.maxX(), (imRect.y() + imRect.maxY()) / 2.0), FloatPoint(imRect.x(), imRect.maxY()) };
        paintInfo.context->drawConvexPolygon(3, pts);
    }
    paintInfo.context->restore();
    return rc;
}

bool RenderThemeWinCE::paintMediaSeekBackButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    bool rc = paintButton(o, paintInfo, r);
    FloatRect imRect = r;
    imRect.inflate(-3);
    FloatPoint pts[3] = { FloatPoint((imRect.x() + imRect.maxX()) / 2.0, imRect.y()), FloatPoint(imRect.x(), (imRect.y() + imRect.maxY()) / 2.0), FloatPoint((imRect.x() + imRect.maxX()) / 2.0, imRect.maxY()) };
    FloatPoint pts2[3] = { FloatPoint(imRect.maxX(), imRect.y()), FloatPoint((imRect.x() + imRect.maxX()) / 2.0, (imRect.y() + imRect.maxY()) / 2.0), FloatPoint(imRect.maxX(), imRect.maxY()) };
    paintInfo.context->save();
    paintInfo.context->setStrokeColor(Color::black);
    paintInfo.context->setFillColor(Color::black);
    paintInfo.context->drawConvexPolygon(3, pts);
    paintInfo.context->drawConvexPolygon(3, pts2);
    paintInfo.context->restore();
    return rc;
}

bool RenderThemeWinCE::paintMediaSeekForwardButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    bool rc = paintButton(o, paintInfo, r);
    FloatRect imRect = r;
    imRect.inflate(-3);
    FloatPoint pts[3] = { FloatPoint(imRect.x(), imRect.y()), FloatPoint((imRect.x() + imRect.maxX()) / 2.0, (imRect.y() + imRect.maxY()) / 2.0), FloatPoint(imRect.x(), imRect.maxY()) };
    FloatPoint pts2[3] = { FloatPoint((imRect.x() + imRect.maxX()) / 2.0, imRect.y()), FloatPoint(imRect.maxX(), (imRect.y() + imRect.maxY()) / 2.0), FloatPoint((imRect.x() + imRect.maxX()) / 2.0, imRect.maxY()) };
    paintInfo.context->save();
    paintInfo.context->setStrokeColor(Color::black);
    paintInfo.context->setFillColor(Color::black);
    paintInfo.context->drawConvexPolygon(3, pts);
    paintInfo.context->drawConvexPolygon(3, pts2);
    paintInfo.context->restore();
    return rc;
}

bool RenderThemeWinCE::paintMediaSliderTrack(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return paintSliderTrack(o, paintInfo, r);
}

bool RenderThemeWinCE::paintMediaSliderThumb(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return paintSliderThumb(o, paintInfo, r);
}
#endif

} // namespace WebCore

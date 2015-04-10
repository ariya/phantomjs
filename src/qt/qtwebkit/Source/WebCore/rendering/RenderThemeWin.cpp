/*
 * Copyright (C) 2006, 2007, 2013 Apple Inc.
 * Copyright (C) 2009 Kenneth Rohde Christiansen
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "RenderThemeWin.h"

#include "CSSValueKeywords.h"
#include "Element.h"
#include "FontMetrics.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "GraphicsContext.h"
#include "LocalWindowsContext.h"
#include "PaintInfo.h"
#include "RenderSlider.h"
#include "Settings.h"
#include "SoftLinking.h"
#include "SystemInfo.h"
#include "UserAgentStyleSheets.h"

#if ENABLE(VIDEO)
#include "RenderMediaControls.h"
#endif

#include <tchar.h>

/* 
 * The following constants are used to determine how a widget is drawn using
 * Windows' Theme API. For more information on theme parts and states see
 * http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/commctls/userex/topics/partsandstates.asp
 */

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
#define EP_EDITBORDER_NOSCROLL 6
#define TFS_READONLY  6

// ComboBox constants (from vsstyle.h)
#define CP_DROPDOWNBUTTON 1
#define CP_BORDER 4
#define CP_READONLY 5
#define CP_DROPDOWNBUTTONRIGHT 6

// TrackBar (slider) parts
#define TKP_TRACK       1
#define TKP_TRACKVERT   2

// TrackBar (slider) thumb parts
#define TKP_THUMBBOTTOM 4
#define TKP_THUMBTOP    5
#define TKP_THUMBLEFT   7
#define TKP_THUMBRIGHT  8

// Trackbar (slider) thumb states
#define TUS_NORMAL      1
#define TUS_HOT         2
#define TUS_PRESSED     3
#define TUS_FOCUSED     4
#define TUS_DISABLED    5

// button states
#define PBS_NORMAL      1
#define PBS_HOT         2
#define PBS_PRESSED     3
#define PBS_DISABLED    4
#define PBS_DEFAULTED   5

// Spin button parts
#define SPNP_UP         1
#define SPNP_DOWN       2

// Spin button states
#define DNS_NORMAL      1
#define DNS_HOT         2
#define DNS_PRESSED     3
#define DNS_DISABLED    4
#define UPS_NORMAL      1
#define UPS_HOT         2
#define UPS_PRESSED     3
#define UPS_DISABLED    4

// Progress bar parts
#define PP_BAR          1
#define PP_BARVERT      2
#define PP_CHUNK        3
#define PP_CHUNKVERT    4
#define PP_FILL         5
#define PP_FILLVERT     6
#define PP_PULSEOVERLAY 7
#define PP_MOVEOVERLAY  8
#define PP_PULSEOVERLAYVERT 9
#define PP_MOVEOVERLAYVERT  10
#define PP_TRANSPARENTBAR   11
#define PP_TRANSPARENTBARVERT 12

// Progress bar states
#define PBBS_NORMAL     1
#define PBBS_PARTIAL    2
#define PBBVS_NORMAL    1 // Vertical
#define PBBVS_PARTIAL   2

// Progress bar fill states
#define PBFS_NORMAL     1
#define PBFS_ERROR      2
#define PBFS_PAUSED     3
#define PBFS_PARTIAL    4
#define PBFVS_NORMAL    1 // Vertical
#define PBFVS_ERROR     2
#define PBFVS_PAUSED    3
#define PBFVS_PARTIAL   4


SOFT_LINK_LIBRARY(uxtheme)
SOFT_LINK(uxtheme, OpenThemeData, HANDLE, WINAPI, (HWND hwnd, LPCWSTR pszClassList), (hwnd, pszClassList))
SOFT_LINK(uxtheme, CloseThemeData, HRESULT, WINAPI, (HANDLE hTheme), (hTheme))
SOFT_LINK(uxtheme, DrawThemeBackground, HRESULT, WINAPI, (HANDLE hTheme, HDC hdc, int iPartId, int iStateId, const RECT* pRect, const RECT* pClipRect), (hTheme, hdc, iPartId, iStateId, pRect, pClipRect))
SOFT_LINK(uxtheme, IsThemeActive, BOOL, WINAPI, (), ())
SOFT_LINK(uxtheme, IsThemeBackgroundPartiallyTransparent, BOOL, WINAPI, (HANDLE hTheme, int iPartId, int iStateId), (hTheme, iPartId, iStateId))

static bool haveTheme;

static const unsigned vistaMenuListButtonOutset = 1;

using namespace std;

namespace WebCore {

// This is the fixed width IE and Firefox use for buttons on dropdown menus
static const int dropDownButtonWidth = 17;

static const int shell32MagnifierIconIndex = 22;

// Default font size to match Firefox.
static const float defaultControlFontPixelSize = 13;

static const float defaultCancelButtonSize = 9;
static const float minCancelButtonSize = 5;
static const float maxCancelButtonSize = 21;
static const float defaultSearchFieldResultsDecorationSize = 13;
static const float minSearchFieldResultsDecorationSize = 9;
static const float maxSearchFieldResultsDecorationSize = 30;
static const float defaultSearchFieldResultsButtonWidth = 18;

static bool gWebKitIsBeingUnloaded;

static bool documentIsInApplicationChromeMode(const Document* document)
{
    Settings* settings = document->settings();
    return settings && settings->applicationChromeMode();
}

void RenderThemeWin::setWebKitIsBeingUnloaded()
{
    gWebKitIsBeingUnloaded = true;
}

PassRefPtr<RenderTheme> RenderThemeWin::create()
{
    return adoptRef(new RenderThemeWin);
}

#if !USE(SAFARI_THEME)
PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    static RenderTheme* winTheme = RenderThemeWin::create().leakRef();
    return winTheme;
}
#endif

RenderThemeWin::RenderThemeWin()
    : m_buttonTheme(0)
    , m_textFieldTheme(0)
    , m_menuListTheme(0)
    , m_sliderTheme(0)
    , m_spinButtonTheme(0)
    , m_progressBarTheme(0)
{
    haveTheme = uxthemeLibrary() && IsThemeActive();
}

RenderThemeWin::~RenderThemeWin()
{
    // If WebKit is being unloaded, then uxtheme.dll is no longer available.
    if (gWebKitIsBeingUnloaded || !uxthemeLibrary())
        return;
    close();
}

HANDLE RenderThemeWin::buttonTheme() const
{
    if (haveTheme && !m_buttonTheme)
        m_buttonTheme = OpenThemeData(0, L"Button");
    return m_buttonTheme;
}

HANDLE RenderThemeWin::textFieldTheme() const
{
    if (haveTheme && !m_textFieldTheme)
        m_textFieldTheme = OpenThemeData(0, L"Edit");
    return m_textFieldTheme;
}

HANDLE RenderThemeWin::menuListTheme() const
{
    if (haveTheme && !m_menuListTheme)
        m_menuListTheme = OpenThemeData(0, L"ComboBox");
    return m_menuListTheme;
}

HANDLE RenderThemeWin::sliderTheme() const
{
    if (haveTheme && !m_sliderTheme)
        m_sliderTheme = OpenThemeData(0, L"TrackBar");
    return m_sliderTheme;
}

HANDLE RenderThemeWin::spinButtonTheme() const
{
    if (haveTheme && !m_spinButtonTheme)
        m_spinButtonTheme = OpenThemeData(0, L"Spin");
    return m_spinButtonTheme;
}

HANDLE RenderThemeWin::progressBarTheme() const
{
    if (haveTheme && !m_progressBarTheme)
        m_progressBarTheme = OpenThemeData(0, L"Progress");
    return m_progressBarTheme;
}

void RenderThemeWin::close()
{
    // This method will need to be called when the OS theme changes to flush our cached themes.
    if (m_buttonTheme)
        CloseThemeData(m_buttonTheme);
    if (m_textFieldTheme)
        CloseThemeData(m_textFieldTheme);
    if (m_menuListTheme)
        CloseThemeData(m_menuListTheme);
    if (m_sliderTheme)
        CloseThemeData(m_sliderTheme);
    if (m_spinButtonTheme)
        CloseThemeData(m_spinButtonTheme);
    if (m_progressBarTheme)
        CloseThemeData(m_progressBarTheme);
    m_buttonTheme = m_textFieldTheme = m_menuListTheme = m_sliderTheme = m_spinButtonTheme = m_progressBarTheme = 0;

    haveTheme = uxthemeLibrary() && IsThemeActive();
}

void RenderThemeWin::themeChanged()
{
    close();
}

String RenderThemeWin::extraDefaultStyleSheet()
{
    return String(themeWinUserAgentStyleSheet, sizeof(themeWinUserAgentStyleSheet));
}

String RenderThemeWin::extraQuirksStyleSheet()
{
    return String(themeWinQuirksUserAgentStyleSheet, sizeof(themeWinQuirksUserAgentStyleSheet));
}

bool RenderThemeWin::supportsHover(const RenderStyle*) const
{
    // The Classic/2k look has no hover effects.
    return haveTheme;
}

Color RenderThemeWin::platformActiveSelectionBackgroundColor() const
{
    COLORREF color = GetSysColor(COLOR_HIGHLIGHT);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color));
}

Color RenderThemeWin::platformInactiveSelectionBackgroundColor() const
{
    // This color matches Firefox.
    return Color(176, 176, 176);
}

Color RenderThemeWin::platformActiveSelectionForegroundColor() const
{
    COLORREF color = GetSysColor(COLOR_HIGHLIGHTTEXT);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color));
}

Color RenderThemeWin::platformInactiveSelectionForegroundColor() const
{
    return platformActiveSelectionForegroundColor();
}

static void fillFontDescription(FontDescription& fontDescription, LOGFONT& logFont, float fontSize)
{    
    fontDescription.setIsAbsoluteSize(true);
    fontDescription.setGenericFamily(FontDescription::NoFamily);
    fontDescription.setOneFamily(String(logFont.lfFaceName));
    fontDescription.setSpecifiedSize(fontSize);
    fontDescription.setWeight(logFont.lfWeight >= 700 ? FontWeightBold : FontWeightNormal); // FIXME: Use real weight.
    fontDescription.setItalic(logFont.lfItalic);
}

static void fillFontDescription(FontDescription& fontDescription, LOGFONT& logFont)
{   
    fillFontDescription(fontDescription, logFont, abs(logFont.lfHeight));
}

void RenderThemeWin::systemFont(CSSValueID valueID, FontDescription& fontDescription) const
{
    static FontDescription captionFont;
    static FontDescription controlFont;
    static FontDescription smallCaptionFont;
    static FontDescription menuFont;
    static FontDescription iconFont;
    static FontDescription messageBoxFont;
    static FontDescription statusBarFont;
    static FontDescription systemFont;
    
    static bool initialized;
    static NONCLIENTMETRICS ncm;

    if (!initialized) {
        initialized = true;
        ncm.cbSize = sizeof(NONCLIENTMETRICS);
        ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    }
 
    switch (valueID) {
    case CSSValueIcon: {
        if (!iconFont.isAbsoluteSize()) {
            LOGFONT logFont;
            ::SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(logFont), &logFont, 0);
            fillFontDescription(iconFont, logFont);
        }
        fontDescription = iconFont;
        break;
    }
    case CSSValueMenu:
        if (!menuFont.isAbsoluteSize())
            fillFontDescription(menuFont, ncm.lfMenuFont);
        fontDescription = menuFont;
        break;
    case CSSValueMessageBox:
        if (!messageBoxFont.isAbsoluteSize())
            fillFontDescription(messageBoxFont, ncm.lfMessageFont);
        fontDescription = messageBoxFont;
        break;
    case CSSValueStatusBar:
        if (!statusBarFont.isAbsoluteSize())
            fillFontDescription(statusBarFont, ncm.lfStatusFont);
        fontDescription = statusBarFont;
        break;
    case CSSValueCaption:
        if (!captionFont.isAbsoluteSize())
            fillFontDescription(captionFont, ncm.lfCaptionFont);
        fontDescription = captionFont;
        break;
    case CSSValueSmallCaption:
        if (!smallCaptionFont.isAbsoluteSize())
            fillFontDescription(smallCaptionFont, ncm.lfSmCaptionFont);
        fontDescription = smallCaptionFont;
        break;
    case CSSValueWebkitSmallControl:
    case CSSValueWebkitMiniControl: // Just map to small.
    case CSSValueWebkitControl: // Just map to small.
        if (!controlFont.isAbsoluteSize()) {
            HGDIOBJ hGDI = ::GetStockObject(DEFAULT_GUI_FONT);
            if (hGDI) {
                LOGFONT logFont;
                if (::GetObject(hGDI, sizeof(logFont), &logFont) > 0)
                    fillFontDescription(controlFont, logFont, defaultControlFontPixelSize);
            }
        }
        fontDescription = controlFont;
        break;
    default: { // Everything else uses the stock GUI font.
        if (!systemFont.isAbsoluteSize()) {
            HGDIOBJ hGDI = ::GetStockObject(DEFAULT_GUI_FONT);
            if (hGDI) {
                LOGFONT logFont;
                if (::GetObject(hGDI, sizeof(logFont), &logFont) > 0)
                    fillFontDescription(systemFont, logFont);
            }
        }
        fontDescription = systemFont;
    }
    }
}

bool RenderThemeWin::supportsFocus(ControlPart appearance) const
{
    switch (appearance) {
        case PushButtonPart:
        case ButtonPart:
        case DefaultButtonPart:
            return true;
        default:
            return false;
    }
}

bool RenderThemeWin::supportsFocusRing(const RenderStyle* style) const
{
    return supportsFocus(style->appearance());
}

unsigned RenderThemeWin::determineClassicState(RenderObject* o, ControlSubPart subPart)
{
    unsigned state = 0;
    switch (o->style()->appearance()) {
        case PushButtonPart:
        case ButtonPart:
        case DefaultButtonPart:
            state = DFCS_BUTTONPUSH;
            if (!isEnabled(o))
                state |= DFCS_INACTIVE;
            else if (isPressed(o))
                state |= DFCS_PUSHED;
            break;
        case RadioPart:
        case CheckboxPart:
            state = (o->style()->appearance() == RadioPart) ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK;
            if (isChecked(o))
                state |= DFCS_CHECKED;
            if (!isEnabled(o))
                state |= DFCS_INACTIVE;
            else if (isPressed(o))
                state |= DFCS_PUSHED;
            break;
        case MenulistPart:
            state = DFCS_SCROLLCOMBOBOX;
            if (!isEnabled(o))
                state |= DFCS_INACTIVE;
            else if (isPressed(o))
                state |= DFCS_PUSHED;
            break;
        case InnerSpinButtonPart: {
            bool isUpButton = subPart == SpinButtonUp;
            state = isUpButton ? DFCS_SCROLLUP : DFCS_SCROLLDOWN;
            if (!isEnabled(o) || isReadOnlyControl(o))
                state |= DFCS_INACTIVE;
            else if (isPressed(o) && isUpButton == isSpinUpButtonPartPressed(o))
                state |= DFCS_PUSHED;
            else if (isHovered(o) && isUpButton == isSpinUpButtonPartHovered(o))
                state |= DFCS_HOT;
            break;
        }
        default:
            break;
    }
    return state;
}

unsigned RenderThemeWin::determineState(RenderObject* o)
{
    unsigned result = TS_NORMAL;
    ControlPart appearance = o->style()->appearance();
    if (!isEnabled(o))
        result = TS_DISABLED;
    else if (isReadOnlyControl(o) && (TextFieldPart == appearance || TextAreaPart == appearance || SearchFieldPart == appearance))
        result = TFS_READONLY; // Readonly is supported on textfields.
    else if (isPressed(o)) // Active overrides hover and focused.
        result = TS_ACTIVE;
    else if (supportsFocus(appearance) && isFocused(o))
        result = TS_FOCUSED;
    else if (isHovered(o))
        result = TS_HOVER;
    if (isChecked(o))
        result += 4; // 4 unchecked states, 4 checked states.
    else if (isIndeterminate(o) && appearance == CheckboxPart)
        result += 8;
    return result;
}

unsigned RenderThemeWin::determineSliderThumbState(RenderObject* o)
{
    unsigned result = TUS_NORMAL;
    if (!isEnabled(o))
        result = TUS_DISABLED;
    else if (supportsFocus(o->style()->appearance()) && isFocused(o))
        result = TUS_FOCUSED;
    else if (isPressed(o))
        result = TUS_PRESSED;
    else if (isHovered(o))
        result = TUS_HOT;
    return result;
}

unsigned RenderThemeWin::determineButtonState(RenderObject* o)
{
    unsigned result = PBS_NORMAL;
    if (!isEnabled(o))
        result = PBS_DISABLED;
    else if (isPressed(o))
        result = PBS_PRESSED;
    else if (supportsFocus(o->style()->appearance()) && isFocused(o))
        result = PBS_DEFAULTED;
    else if (isHovered(o))
        result = PBS_HOT;
    else if (isDefault(o))
        result = PBS_DEFAULTED;
    return result;
}

unsigned RenderThemeWin::determineSpinButtonState(RenderObject* o, ControlSubPart subPart)
{
    bool isUpButton = subPart == SpinButtonUp;
    unsigned result = isUpButton ? UPS_NORMAL : DNS_NORMAL;
    if (!isEnabled(o) || isReadOnlyControl(o))
        result = isUpButton ? UPS_DISABLED : DNS_DISABLED;
    else if (isPressed(o) && isUpButton == isSpinUpButtonPartPressed(o))
        result = isUpButton ? UPS_PRESSED : DNS_PRESSED;
    else if (isHovered(o) && isUpButton == isSpinUpButtonPartHovered(o))
        result = isUpButton ? UPS_HOT : DNS_HOT;
    return result;
}

ThemeData RenderThemeWin::getClassicThemeData(RenderObject* o, ControlSubPart subPart)
{
    ThemeData result;
    switch (o->style()->appearance()) {
        case PushButtonPart:
        case ButtonPart:
        case DefaultButtonPart:
        case CheckboxPart:
        case RadioPart:
            result.m_part = DFC_BUTTON;
            result.m_state = determineClassicState(o);
            break;
        case MenulistPart:
            result.m_part = DFC_SCROLL;
            result.m_state = determineClassicState(o);
            break;
        case MeterPart:
            result.m_part = PP_BAR;
            result.m_state = determineState(o);
            break;
        case SearchFieldPart:
        case TextFieldPart:
        case TextAreaPart:
            result.m_part = TFP_TEXTFIELD;
            result.m_state = determineState(o);
            break;
        case SliderHorizontalPart:
            result.m_part = TKP_TRACK;
            result.m_state = TS_NORMAL;
            break;
        case SliderVerticalPart:
            result.m_part = TKP_TRACKVERT;
            result.m_state = TS_NORMAL;
            break;
        case SliderThumbHorizontalPart:
            result.m_part = TKP_THUMBBOTTOM;
            result.m_state = determineSliderThumbState(o);
            break;
        case SliderThumbVerticalPart:
            result.m_part = TKP_THUMBRIGHT;
            result.m_state = determineSliderThumbState(o);
            break;
        case InnerSpinButtonPart:
            result.m_part = DFC_SCROLL;
            result.m_state = determineClassicState(o, subPart);
            break;
        default:
            break;
    }
    return result;
}

ThemeData RenderThemeWin::getThemeData(RenderObject* o, ControlSubPart subPart)
{
    if (!haveTheme)
        return getClassicThemeData(o, subPart);

    ThemeData result;
    switch (o->style()->appearance()) {
        case PushButtonPart:
        case ButtonPart:
        case DefaultButtonPart:
            result.m_part = BP_BUTTON;
            result.m_state = determineButtonState(o);
            break;
        case CheckboxPart:
            result.m_part = BP_CHECKBOX;
            result.m_state = determineState(o);
            break;
        case MenulistPart:
        case MenulistButtonPart: {
            const bool isVistaOrLater = (windowsVersion() >= WindowsVista);
            result.m_part = isVistaOrLater ? CP_DROPDOWNBUTTONRIGHT : CP_DROPDOWNBUTTON;
            if (isVistaOrLater && documentIsInApplicationChromeMode(o->document())) {
                // The "readonly" look we use in application chrome mode
                // only uses a "normal" look for the drop down button.
                result.m_state = TS_NORMAL;
            } else
                result.m_state = determineState(o);
            break;
        }
        case MeterPart:
            result.m_part = PP_BAR;
            result.m_state = determineState(o);
            break;
        case RadioPart:
            result.m_part = BP_RADIO;
            result.m_state = determineState(o);
            break;
        case SearchFieldPart:
        case TextFieldPart:
        case TextAreaPart:
            result.m_part = (windowsVersion() >= WindowsVista) ? EP_EDITBORDER_NOSCROLL : TFP_TEXTFIELD;
            result.m_state = determineState(o);
            break;
        case SliderHorizontalPart:
            result.m_part = TKP_TRACK;
            result.m_state = TS_NORMAL;
            break;
        case SliderVerticalPart:
            result.m_part = TKP_TRACKVERT;
            result.m_state = TS_NORMAL;
            break;
        case SliderThumbHorizontalPart:
            result.m_part = TKP_THUMBBOTTOM;
            result.m_state = determineSliderThumbState(o);
            break;
        case SliderThumbVerticalPart:
            result.m_part = TKP_THUMBRIGHT;
            result.m_state = determineSliderThumbState(o);
            break;
        case InnerSpinButtonPart:
            result.m_part = subPart == SpinButtonUp ? SPNP_UP : SPNP_DOWN;
            result.m_state = determineSpinButtonState(o, subPart);
            break;
    }

    return result;
}

static void drawControl(GraphicsContext* context, RenderObject* o, HANDLE theme, const ThemeData& themeData, const IntRect& r)
{
    bool alphaBlend = false;
    if (theme)
        alphaBlend = IsThemeBackgroundPartiallyTransparent(theme, themeData.m_part, themeData.m_state);
    LocalWindowsContext windowsContext(context, r, alphaBlend);
    RECT widgetRect = r;
    if (theme)
        DrawThemeBackground(theme, windowsContext.hdc(), themeData.m_part, themeData.m_state, &widgetRect, 0);
    else {
        HDC hdc = windowsContext.hdc();
        if (themeData.m_part == TFP_TEXTFIELD) {
            ::DrawEdge(hdc, &widgetRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
            if (themeData.m_state == TS_DISABLED || themeData.m_state ==  TFS_READONLY)
                ::FillRect(hdc, &widgetRect, (HBRUSH)(COLOR_BTNFACE+1));
            else
                ::FillRect(hdc, &widgetRect, (HBRUSH)(COLOR_WINDOW+1));
        } else if (themeData.m_part == TKP_TRACK || themeData.m_part == TKP_TRACKVERT) {
            ::DrawEdge(hdc, &widgetRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
            ::FillRect(hdc, &widgetRect, (HBRUSH)GetStockObject(GRAY_BRUSH));
        } else if ((o->style()->appearance() == SliderThumbHorizontalPart ||
                    o->style()->appearance() == SliderThumbVerticalPart) && 
                   (themeData.m_part == TKP_THUMBBOTTOM || themeData.m_part == TKP_THUMBTOP || 
                    themeData.m_part == TKP_THUMBLEFT || themeData.m_part == TKP_THUMBRIGHT)) {
            ::DrawEdge(hdc, &widgetRect, EDGE_RAISED, BF_RECT | BF_SOFT | BF_MIDDLE | BF_ADJUST);
            if (themeData.m_state == TUS_DISABLED) {
                static WORD patternBits[8] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};
                HBITMAP patternBmp = ::CreateBitmap(8, 8, 1, 1, patternBits);
                if (patternBmp) {
                    HBRUSH brush = (HBRUSH) ::CreatePatternBrush(patternBmp);
                    COLORREF oldForeColor = ::SetTextColor(hdc, ::GetSysColor(COLOR_3DFACE));
                    COLORREF oldBackColor = ::SetBkColor(hdc, ::GetSysColor(COLOR_3DHILIGHT));
                    POINT p;
                    ::GetViewportOrgEx(hdc, &p);
                    ::SetBrushOrgEx(hdc, p.x + widgetRect.left, p.y + widgetRect.top, NULL);
                    HBRUSH oldBrush = (HBRUSH) ::SelectObject(hdc, brush);
                    ::FillRect(hdc, &widgetRect, brush);
                    ::SetTextColor(hdc, oldForeColor);
                    ::SetBkColor(hdc, oldBackColor);
                    ::SelectObject(hdc, oldBrush);
                    ::DeleteObject(brush); 
                } else
                    ::FillRect(hdc, &widgetRect, (HBRUSH)COLOR_3DHILIGHT);
                ::DeleteObject(patternBmp);
            }
        } else {
            // Push buttons, buttons, checkboxes and radios, and the dropdown arrow in menulists.
            if (o->style()->appearance() == DefaultButtonPart) {
                HBRUSH brush = ::GetSysColorBrush(COLOR_3DDKSHADOW);
                ::FrameRect(hdc, &widgetRect, brush);
                ::InflateRect(&widgetRect, -1, -1);
                ::DrawEdge(hdc, &widgetRect, BDR_RAISEDOUTER, BF_RECT | BF_MIDDLE);
            }
            ::DrawFrameControl(hdc, &widgetRect, themeData.m_part, themeData.m_state);
        }
    }

    if (!alphaBlend && !context->isInTransparencyLayer())
        DIBPixelData::setRGBABitmapAlpha(windowsContext.hdc(), r, 255);
}

bool RenderThemeWin::paintButton(RenderObject* o, const PaintInfo& i, const IntRect& r)
{  
    drawControl(i.context,  o, buttonTheme(), getThemeData(o), r);
    return false;
}

void RenderThemeWin::adjustInnerSpinButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    int width = ::GetSystemMetrics(SM_CXVSCROLL);
    if (width <= 0)
        width = 17; // Vista's default.
    style->setWidth(Length(width, Fixed));
    style->setMinWidth(Length(width, Fixed));
}

bool RenderThemeWin::paintInnerSpinButton(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    // We split the specified rectangle into two vertically. We can't draw a
    // spin button of which height is less than 2px.
    if (r.height() < 2)
        return false;
    IntRect upRect(r);
    upRect.setHeight(r.height() / 2);
    IntRect downRect(r);
    downRect.setY(upRect.maxY());
    downRect.setHeight(r.height() - upRect.height());
    drawControl(i.context, o, spinButtonTheme(), getThemeData(o, SpinButtonUp), upRect);
    drawControl(i.context, o, spinButtonTheme(), getThemeData(o, SpinButtonDown), downRect);
    return false;
}

void RenderThemeWin::setCheckboxSize(RenderStyle* style) const
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

bool RenderThemeWin::paintTextField(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    drawControl(i.context,  o, textFieldTheme(), getThemeData(o), r);
    return false;
}

bool RenderThemeWin::paintMenuList(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    HANDLE theme;
    int part;
    if (haveTheme && (windowsVersion() >= WindowsVista)) {
        theme = menuListTheme();
        if (documentIsInApplicationChromeMode(o->document()))
            part = CP_READONLY;
        else
            part = CP_BORDER;
    } else {
        theme = textFieldTheme();
        part = TFP_TEXTFIELD;
    }

    drawControl(i.context,  o, theme, ThemeData(part, determineState(o)), r);
    
    return paintMenuListButton(o, i, r);
}

void RenderThemeWin::adjustMenuListStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    style->resetBorder();
    adjustMenuListButtonStyle(styleResolver, style, e);
}

void RenderThemeWin::adjustMenuListButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
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

    style->setLineHeight(RenderStyle::initialLineHeight());
    
    // White-space is locked to pre
    style->setWhiteSpace(PRE);
}

bool RenderThemeWin::paintMenuListButton(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    // FIXME: Don't make hardcoded assumptions about the thickness of the textfield border.
    int borderThickness = haveTheme ? 1 : 2;

    // Paint the dropdown button on the inner edge of the text field,
    // leaving space for the text field's 1px border
    IntRect buttonRect(r);
    buttonRect.inflate(-borderThickness);
    if (o->style()->direction() == LTR)
        buttonRect.setX(buttonRect.maxX() - dropDownButtonWidth);
    buttonRect.setWidth(dropDownButtonWidth);

    if ((windowsVersion() >= WindowsVista)) {
        // Outset the top, right, and bottom borders of the button so that they coincide with the <select>'s border.
        buttonRect.setY(buttonRect.y() - vistaMenuListButtonOutset);
        buttonRect.setHeight(buttonRect.height() + 2 * vistaMenuListButtonOutset);
        buttonRect.setWidth(buttonRect.width() + vistaMenuListButtonOutset);
    }

    drawControl(i.context, o, menuListTheme(), getThemeData(o), buttonRect);

    return false;
}

const int trackWidth = 4;

bool RenderThemeWin::paintSliderTrack(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    IntRect bounds = r;
    
    if (o->style()->appearance() ==  SliderHorizontalPart) {
        bounds.setHeight(trackWidth);
        bounds.setY(r.y() + r.height() / 2 - trackWidth / 2);
    } else if (o->style()->appearance() == SliderVerticalPart) {
        bounds.setWidth(trackWidth);
        bounds.setX(r.x() + r.width() / 2 - trackWidth / 2);
    }
    
    drawControl(i.context,  o, sliderTheme(), getThemeData(o), bounds);
    return false;
}

bool RenderThemeWin::paintSliderThumb(RenderObject* o, const PaintInfo& i, const IntRect& r)
{   
    drawControl(i.context,  o, sliderTheme(), getThemeData(o), r);
    return false;
}

const int sliderThumbWidth = 7;
const int sliderThumbHeight = 15;

void RenderThemeWin::adjustSliderThumbSize(RenderStyle* style, Element*) const
{
    ControlPart part = style->appearance();
    if (part == SliderThumbVerticalPart) {
        style->setWidth(Length(sliderThumbHeight, Fixed));
        style->setHeight(Length(sliderThumbWidth, Fixed));
    } else if (part == SliderThumbHorizontalPart) {
        style->setWidth(Length(sliderThumbWidth, Fixed));
        style->setHeight(Length(sliderThumbHeight, Fixed));
    }
#if ENABLE(VIDEO)
    else if (part == MediaSliderThumbPart || part == MediaVolumeSliderThumbPart) 
        RenderMediaControls::adjustMediaSliderThumbSize(style);
#endif
}

bool RenderThemeWin::paintSearchField(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    return paintTextField(o, i, r);
}

void RenderThemeWin::adjustSearchFieldStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    // Override paddingSize to match AppKit text positioning.
    const int padding = 1;
    style->setPaddingLeft(Length(padding, Fixed));
    style->setPaddingRight(Length(padding, Fixed));
    style->setPaddingTop(Length(padding, Fixed));
    style->setPaddingBottom(Length(padding, Fixed));
    if (e && e->focused() && e->document()->frame()->selection()->isFocusedAndActive())
        style->setOutlineOffset(-2);
}

bool RenderThemeWin::paintSearchFieldCancelButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    IntRect bounds = r;
    ASSERT(o->parent());
    if (!o->parent() || !o->parent()->isBox())
        return false;
    
    RenderBox* parentRenderBox = toRenderBox(o->parent());

    IntRect parentBox = parentRenderBox->absoluteContentBox();
    
    // Make sure the scaled button stays square and will fit in its parent's box
    bounds.setHeight(min(parentBox.width(), min(parentBox.height(), bounds.height())));
    bounds.setWidth(bounds.height());

    // Center the button vertically.  Round up though, so if it has to be one pixel off-center, it will
    // be one pixel closer to the bottom of the field.  This tends to look better with the text.
    bounds.setY(parentBox.y() + (parentBox.height() - bounds.height() + 1) / 2);

    static Image* cancelImage = Image::loadPlatformResource("searchCancel").leakRef();
    static Image* cancelPressedImage = Image::loadPlatformResource("searchCancelPressed").leakRef();
    paintInfo.context->drawImage(isPressed(o) ? cancelPressedImage : cancelImage, o->style()->colorSpace(), bounds);
    return false;
}

void RenderThemeWin::adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Scale the button size based on the font size
    float fontScale = style->fontSize() / defaultControlFontPixelSize;
    int cancelButtonSize = lroundf(min(max(minCancelButtonSize, defaultCancelButtonSize * fontScale), maxCancelButtonSize));
    style->setWidth(Length(cancelButtonSize, Fixed));
    style->setHeight(Length(cancelButtonSize, Fixed));
}

void RenderThemeWin::adjustSearchFieldDecorationStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    IntSize emptySize(1, 11);
    style->setWidth(Length(emptySize.width(), Fixed));
    style->setHeight(Length(emptySize.height(), Fixed));
}

void RenderThemeWin::adjustSearchFieldResultsDecorationStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Scale the decoration size based on the font size
    float fontScale = style->fontSize() / defaultControlFontPixelSize;
    int magnifierSize = lroundf(min(max(minSearchFieldResultsDecorationSize, defaultSearchFieldResultsDecorationSize * fontScale), 
                                     maxSearchFieldResultsDecorationSize));
    style->setWidth(Length(magnifierSize, Fixed));
    style->setHeight(Length(magnifierSize, Fixed));
}

bool RenderThemeWin::paintSearchFieldResultsDecoration(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    IntRect bounds = r;
    ASSERT(o->parent());
    if (!o->parent() || !o->parent()->isBox())
        return false;
    
    RenderBox* parentRenderBox = toRenderBox(o->parent());
    IntRect parentBox = parentRenderBox->absoluteContentBox();
    
    // Make sure the scaled decoration stays square and will fit in its parent's box
    bounds.setHeight(min(parentBox.width(), min(parentBox.height(), bounds.height())));
    bounds.setWidth(bounds.height());

    // Center the decoration vertically.  Round up though, so if it has to be one pixel off-center, it will
    // be one pixel closer to the bottom of the field.  This tends to look better with the text.
    bounds.setY(parentBox.y() + (parentBox.height() - bounds.height() + 1) / 2);
    
    static Image* magnifierImage = Image::loadPlatformResource("searchMagnifier").leakRef();
    paintInfo.context->drawImage(magnifierImage, o->style()->colorSpace(), bounds);
    return false;
}

void RenderThemeWin::adjustSearchFieldResultsButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // Scale the button size based on the font size
    float fontScale = style->fontSize() / defaultControlFontPixelSize;
    int magnifierHeight = lroundf(min(max(minSearchFieldResultsDecorationSize, defaultSearchFieldResultsDecorationSize * fontScale), 
                                   maxSearchFieldResultsDecorationSize));
    int magnifierWidth = lroundf(magnifierHeight * defaultSearchFieldResultsButtonWidth / defaultSearchFieldResultsDecorationSize);
    style->setWidth(Length(magnifierWidth, Fixed));
    style->setHeight(Length(magnifierHeight, Fixed));
}

bool RenderThemeWin::paintSearchFieldResultsButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    IntRect bounds = r;
    ASSERT(o->parent());
    if (!o->parent())
        return false;
    if (!o->parent() || !o->parent()->isBox())
        return false;
    
    RenderBox* parentRenderBox = toRenderBox(o->parent());
    IntRect parentBox = parentRenderBox->absoluteContentBox();
    
    // Make sure the scaled decoration will fit in its parent's box
    bounds.setHeight(min(parentBox.height(), bounds.height()));
    bounds.setWidth(min<int>(parentBox.width(), bounds.height() * defaultSearchFieldResultsButtonWidth / defaultSearchFieldResultsDecorationSize));

    // Center the button vertically.  Round up though, so if it has to be one pixel off-center, it will
    // be one pixel closer to the bottom of the field.  This tends to look better with the text.
    bounds.setY(parentBox.y() + (parentBox.height() - bounds.height() + 1) / 2);

    static Image* magnifierImage = Image::loadPlatformResource("searchMagnifierResults").leakRef();
    paintInfo.context->drawImage(magnifierImage, o->style()->colorSpace(), bounds);
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

Color RenderThemeWin::systemColor(CSSValueID cssValueId) const
{
    int sysColorIndex = cssValueIdToSysColorIndex(cssValueId);
    if (sysColorIndex == -1)
        return RenderTheme::systemColor(cssValueId);

    COLORREF color = GetSysColor(sysColorIndex);
    return Color(GetRValue(color), GetGValue(color), GetBValue(color));
}

#if ENABLE(VIDEO)

String RenderThemeWin::extraMediaControlsStyleSheet()
{
    return String(mediaControlsQuickTimeUserAgentStyleSheet, sizeof(mediaControlsQuickTimeUserAgentStyleSheet));
}

#if ENABLE(FULLSCREEN_API)
String RenderThemeWin::extraFullScreenStyleSheet()
{
    return String(fullscreenQuickTimeUserAgentStyleSheet, sizeof(fullscreenQuickTimeUserAgentStyleSheet));
}
#endif

bool RenderThemeWin::supportsClosedCaptioning() const
{
    return true;
}

bool RenderThemeWin::paintMediaFullscreenButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaEnterFullscreenButton, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaMuteButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaMuteButton, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaPlayButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaPlayButton, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaRewindButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaRewindButton, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaSeekBackButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaSeekBackButton, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaSeekForwardButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaSeekForwardButton, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaSliderTrack(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaSlider, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaSliderThumb(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaSliderThumb, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaToggleClosedCaptionsButton(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaShowClosedCaptionsButton, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaControlsBackground(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaTimelineContainer, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaVolumeSliderContainer(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaVolumeSliderContainer, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaVolumeSliderTrack(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaVolumeSlider, o, paintInfo, r);
}

bool RenderThemeWin::paintMediaVolumeSliderThumb(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    return RenderMediaControls::paintMediaControlsPart(MediaVolumeSliderThumb, o, paintInfo, r);
}

IntPoint RenderThemeWin::volumeSliderOffsetFromMuteButton(RenderBox* muteButtonBox, const IntSize& size) const
{
    return RenderMediaControls::volumeSliderOffsetFromMuteButton(muteButtonBox, size);
}

#endif

#if ENABLE(METER_ELEMENT)
void RenderThemeWin::adjustMeterStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(nullptr);
}

bool RenderThemeWin::supportsMeter(ControlPart part) const
{
    switch (part) {
    case MeterPart:
        return true;
    default:
        return false;
    }
}

IntSize RenderThemeWin::meterSizeForBounds(const RenderMeter*, const IntRect& bounds) const
{
    return bounds.size();
}

bool RenderThemeWin::paintMeter(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    if (!renderObject->isMeter())
        return true;

    HTMLMeterElement* element = toRenderMeter(renderObject)->meterElement();

    ThemeData theme = getThemeData(renderObject);

    int remaining = static_cast<int>((1.0 - element->valueRatio()) * static_cast<double>(rect.size().width()));

    // Draw the background
    drawControl(paintInfo.context, renderObject, progressBarTheme(), theme, rect);

    // Draw the progress portion
    IntRect completedRect(rect);
    completedRect.contract(remaining, 0);

    theme.m_part = PP_FILL;
    drawControl(paintInfo.context, renderObject, progressBarTheme(), theme, completedRect);

    return true;
}

#endif


}

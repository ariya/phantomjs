/*
 * Copyright (C) 2007 Apple Inc.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2009 Kenneth Rohde Christiansen
 * Copyright (C) 2010 Igalia S.L.
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
#include "RenderThemeGtk.h"

#ifdef GTK_API_VERSION_2

// We need this to allow building while using GTK_WIDGET_SET_FLAGS. It's deprecated
// but some theme engines require it to ensure proper rendering of focus indicators.
#undef GTK_DISABLE_DEPRECATED

#include "CSSValueKeywords.h"
#include "Font.h"
#include "GraphicsContext.h"
#include "GtkVersioning.h"
#include "HTMLNames.h"
#include "MediaControlElements.h"
#include "PaintInfo.h"
#include "RenderObject.h"
#include "TextDirection.h"
#include "UserAgentStyleSheets.h"
#include "WidgetRenderingContext.h"
#include <gdk/gdk.h>
#include <gtk/gtk.h>

namespace WebCore {

// This is the default value defined by GTK+, where it was defined as MIN_ARROW_WIDTH in gtkspinbutton.c.
static const int minSpinButtonArrowSize = 6;

// This is not a static method, because we want to avoid having GTK+ headers in RenderThemeGtk.h.
extern GtkTextDirection gtkTextDirection(TextDirection);

void RenderThemeGtk::platformInit()
{
    m_themePartsHaveRGBAColormap = true;
    m_gtkWindow = 0;
    m_gtkContainer = 0;
    m_gtkButton = 0;
    m_gtkEntry = 0;
    m_gtkTreeView = 0;
    m_gtkVScale = 0;
    m_gtkHScale = 0;
    m_gtkRadioButton = 0;
    m_gtkCheckButton = 0;
    m_gtkProgressBar = 0;
    m_gtkComboBox = 0;
    m_gtkComboBoxButton = 0;
    m_gtkComboBoxArrow = 0;
    m_gtkComboBoxSeparator = 0;
    m_gtkVScrollbar = 0;
    m_gtkHScrollbar = 0;
    m_gtkSpinButton = 0;

    m_colormap = gdk_screen_get_rgba_colormap(gdk_screen_get_default());
    if (!m_colormap) {
        m_themePartsHaveRGBAColormap = false;
        m_colormap = gdk_screen_get_default_colormap(gdk_screen_get_default());
    }
}

RenderThemeGtk::~RenderThemeGtk()
{
    if (m_gtkWindow)
        gtk_widget_destroy(m_gtkWindow);
}

#if ENABLE(VIDEO)
void RenderThemeGtk::initMediaColors()
{
    GtkStyle* style = gtk_widget_get_style(GTK_WIDGET(gtkContainer()));
    m_panelColor = style->bg[GTK_STATE_NORMAL];
    m_sliderColor = style->bg[GTK_STATE_ACTIVE];
    m_sliderThumbColor = style->bg[GTK_STATE_SELECTED];
}
#endif

static void adjustRectForFocus(GtkWidget* widget, IntRect& rect, bool ignoreInteriorFocusProperty = false)
{
    gint focusWidth, focusPad;
    gboolean interiorFocus = 0;
    gtk_widget_style_get(widget,
                         "interior-focus", &interiorFocus,
                         "focus-line-width", &focusWidth,
                         "focus-padding", &focusPad, NULL);
    if (!ignoreInteriorFocusProperty && interiorFocus)
        return;
    rect.inflate(focusWidth + focusPad);
}

void RenderThemeGtk::adjustRepaintRect(const RenderObject* renderObject, IntRect& rect)
{
    ControlPart part = renderObject->style()->appearance();
    switch (part) {
    case CheckboxPart:
    case RadioPart: {
        // We ignore the interior focus property and always expand the focus rect. In GTK+, the
        // focus indicator is usually on the text next to a checkbox or radio button, but that doesn't
        // happen in WebCore. By expanding the focus rectangle unconditionally we increase its prominence.
        adjustRectForFocus(part == CheckboxPart ? gtkCheckButton() : gtkRadioButton(), rect, true);
        return;
    }
    case InnerSpinButtonPart:
        // See paintInnerSpinButton for an explanation of why we expand the painting rect.
        rect.inflateY(2);
        rect.setWidth(rect.width() + 2);
    default:
        return;
    }
}

static GtkStateType getGtkStateType(RenderThemeGtk* theme, RenderObject* object)
{
    if (!theme->isEnabled(object) || theme->isReadOnlyControl(object))
        return GTK_STATE_INSENSITIVE;
    if (theme->isPressed(object))
        return GTK_STATE_ACTIVE;
    if (theme->isHovered(object))
        return GTK_STATE_PRELIGHT;
    return GTK_STATE_NORMAL;
}

static void setToggleSize(const RenderThemeGtk* theme, RenderStyle* style, GtkWidget* widget)
{
    // The width and height are both specified, so we shouldn't change them.
    if (!style->width().isIntrinsicOrAuto() && !style->height().isAuto())
        return;

    gint indicatorSize;
    gtk_widget_style_get(widget, "indicator-size", &indicatorSize, NULL);
    if (style->width().isIntrinsicOrAuto())
        style->setWidth(Length(indicatorSize, Fixed));
    if (style->height().isAuto())
        style->setHeight(Length(indicatorSize, Fixed));
}

static void paintToggle(RenderThemeGtk* theme, RenderObject* renderObject, const PaintInfo& info, const IntRect& rect, GtkWidget* widget)
{
    // We do not call gtk_toggle_button_set_active here, because some themes begin a series of
    // animation frames in a "toggled" signal handler. This puts some checkboxes in a half-way
    // checked state. Every GTK+ theme I tested merely looks at the shadow type (and not the
    // 'active' property) to determine whether or not to draw the check.
    gtk_widget_set_sensitive(widget, theme->isEnabled(renderObject) && !theme->isReadOnlyControl(renderObject));
    gtk_widget_set_direction(widget, gtkTextDirection(renderObject->style()->direction()));

    bool indeterminate = theme->isIndeterminate(renderObject);
    gtk_toggle_button_set_inconsistent(GTK_TOGGLE_BUTTON(widget), indeterminate);

    GtkShadowType shadowType = GTK_SHADOW_OUT;
    if (indeterminate) // This originates from the Mozilla code.
        shadowType = GTK_SHADOW_ETCHED_IN;
    else if (theme->isChecked(renderObject))
        shadowType = GTK_SHADOW_IN;

    WidgetRenderingContext widgetContext(info.context, rect);
    IntRect buttonRect(IntPoint(), rect.size());
    GtkStateType toggleState = getGtkStateType(theme, renderObject);
    const char* detail = 0;
    if (GTK_IS_RADIO_BUTTON(widget)) {
        detail = "radiobutton";
        widgetContext.gtkPaintOption(buttonRect, widget, toggleState, shadowType, detail);
    } else {
        detail = "checkbutton";
        widgetContext.gtkPaintCheck(buttonRect, widget, toggleState, shadowType, detail);
    }

    if (theme->isFocused(renderObject)) {
        IntRect focusRect(buttonRect);
        adjustRectForFocus(widget, focusRect, true);
        widgetContext.gtkPaintFocus(focusRect, widget, toggleState, detail);
    }
}

void RenderThemeGtk::setCheckboxSize(RenderStyle* style) const
{
    setToggleSize(this, style, gtkCheckButton());
}

bool RenderThemeGtk::paintCheckbox(RenderObject* renderObject, const PaintInfo& info, const IntRect& rect)
{
    paintToggle(this, renderObject, info, rect, gtkCheckButton());
    return false;
}

void RenderThemeGtk::setRadioSize(RenderStyle* style) const
{
    setToggleSize(this, style, gtkRadioButton());
}

bool RenderThemeGtk::paintRadio(RenderObject* renderObject, const PaintInfo& info, const IntRect& rect)
{
    paintToggle(this, renderObject, info, rect, gtkRadioButton());
    return false;
}

static void setWidgetHasFocus(GtkWidget* widget, gboolean hasFocus)
{
    g_object_set(widget, "has-focus", hasFocus, NULL);

    // These functions are deprecated in GTK+ 2.22, yet theme engines still look
    // at these flags when determining if a widget has focus, so we must use them.
    if (hasFocus)
        GTK_WIDGET_SET_FLAGS(widget, GTK_HAS_FOCUS);
    else
        GTK_WIDGET_UNSET_FLAGS(widget, GTK_HAS_FOCUS);
}

bool RenderThemeGtk::paintButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    if (info.context->paintingDisabled())
        return false;

    GtkWidget* widget = gtkButton();
    IntRect buttonRect(IntPoint(), rect.size());
    IntRect focusRect(buttonRect);

    GtkStateType state = getGtkStateType(this, object);
    gtk_widget_set_state(widget, state);
    gtk_widget_set_direction(widget, gtkTextDirection(object->style()->direction()));

    if (isFocused(object)) {
        setWidgetHasFocus(widget, TRUE);

        gboolean interiorFocus = 0, focusWidth = 0, focusPadding = 0;
        gtk_widget_style_get(widget,
                             "interior-focus", &interiorFocus,
                             "focus-line-width", &focusWidth,
                             "focus-padding", &focusPadding, NULL);
        // If we are using exterior focus, we shrink the button rect down before
        // drawing. If we are using interior focus we shrink the focus rect. This
        // approach originates from the Mozilla theme drawing code (gtk2drawing.c).
        if (interiorFocus) {
            GtkStyle* style = gtk_widget_get_style(widget);
            focusRect.inflateX(-style->xthickness - focusPadding);
            focusRect.inflateY(-style->ythickness - focusPadding);
        } else {
            buttonRect.inflateX(-focusWidth - focusPadding);
            buttonRect.inflateY(-focusPadding - focusPadding);
        }
    }

    WidgetRenderingContext widgetContext(info.context, rect);
    GtkShadowType shadowType = state == GTK_STATE_ACTIVE ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
    widgetContext.gtkPaintBox(buttonRect, widget, state, shadowType, "button");
    if (isFocused(object))
        widgetContext.gtkPaintFocus(focusRect, widget, state, "button");

    setWidgetHasFocus(widget, FALSE);
    return false;
}

int RenderThemeGtk::getComboBoxSeparatorWidth() const
{
    GtkWidget* separator = gtkComboBoxSeparator();
    if (!separator)
        return 0;

    gboolean hasWideSeparators = FALSE;
    gint separatorWidth = 0;
    gtk_widget_style_get(separator,
                         "wide-separators", &hasWideSeparators,
                         "separator-width", &separatorWidth,
                         NULL);
    if (hasWideSeparators)
        return separatorWidth;
    return gtk_widget_get_style(separator)->xthickness;
}

int RenderThemeGtk::comboBoxArrowSize(RenderStyle* style) const
{
    // Taking the font size and reversing the DPI conversion seems to match
    // GTK+ rendering as closely as possible.
    return style->font().size() * (72.0 / RenderThemeGtk::getScreenDPI());
}

static void getButtonInnerBorder(GtkWidget* button, int& left, int& top, int& right, int& bottom)
{
    GtkStyle* style = gtk_widget_get_style(button);
    int outerBorder = gtk_container_get_border_width(GTK_CONTAINER(button));
    static GtkBorder defaultInnerBorder = {1, 1, 1, 1};
    GtkBorder* innerBorder;
    gtk_widget_style_get(button, "inner-border", &innerBorder, NULL);
    if (!innerBorder)
        innerBorder = &defaultInnerBorder;

    left = outerBorder + innerBorder->left + style->xthickness;
    right = outerBorder + innerBorder->right + style->xthickness;
    top = outerBorder + innerBorder->top + style->ythickness;
    bottom = outerBorder + innerBorder->bottom + style->ythickness;

    if (innerBorder != &defaultInnerBorder)
        gtk_border_free(innerBorder);
}


void RenderThemeGtk::getComboBoxPadding(RenderStyle* style, int& left, int& top, int& right, int& bottom) const
{
    // If this menu list button isn't drawn using the native theme, we
    // don't add any extra padding beyond what WebCore already uses.
    if (style->appearance() == NoControlPart)
        return;

    // A combo box button is a button with widgets packed into it.
    GtkStyle* buttonWidgetStyle = gtk_widget_get_style(gtkComboBoxButton());
    getButtonInnerBorder(gtkComboBoxButton(), left, top, right, bottom);

    // Add xthickness amount of padding for each side of the separator. This ensures
    // that the text does not bump up against the separator.
    int arrowAndSeperatorLength = comboBoxArrowSize(style) +
        getComboBoxSeparatorWidth() + (3 * buttonWidgetStyle->xthickness);

    if (style->direction() == RTL)
        left += arrowAndSeperatorLength;
    else
        right += arrowAndSeperatorLength;
}

int RenderThemeGtk::popupInternalPaddingLeft(RenderStyle* style) const
{
    int left = 0, top = 0, right = 0, bottom = 0;
    getComboBoxPadding(style, left, top, right, bottom);
    return left;
}

int RenderThemeGtk::popupInternalPaddingRight(RenderStyle* style) const
{
    int left = 0, top = 0, right = 0, bottom = 0;
    getComboBoxPadding(style, left, top, right, bottom);
    return right;
}

int RenderThemeGtk::popupInternalPaddingTop(RenderStyle* style) const
{
    int left = 0, top = 0, right = 0, bottom = 0;
    getComboBoxPadding(style, left, top, right, bottom);
    return top;
}

int RenderThemeGtk::popupInternalPaddingBottom(RenderStyle* style) const
{
    int left = 0, top = 0, right = 0, bottom = 0;
    getComboBoxPadding(style, left, top, right, bottom);
    return bottom;
}

bool RenderThemeGtk::paintMenuList(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    if (paintButton(object, info, rect))
        return true;

    // Menu list button painting strategy.
    // For buttons with appears-as-list set to false (having a separator):
    // | left border | Button text | xthickness | vseparator | xthickness | arrow | xthickness | right border |
    // For buttons with appears-as-list set to true (not having a separator):
    // | left border | Button text | arrow | xthickness | right border |

    int leftBorder = 0, rightBorder = 0, bottomBorder = 0, topBorder = 0;
    getButtonInnerBorder(gtkComboBoxButton(), leftBorder, topBorder, rightBorder, bottomBorder);
    RenderStyle* style = object->style();
    int arrowSize = comboBoxArrowSize(style);
    GtkStyle* buttonStyle = gtk_widget_get_style(gtkComboBoxButton());

    IntRect arrowRect(0, (rect.height() - arrowSize) / 2, arrowSize, arrowSize);
    if (style->direction() == RTL)
        arrowRect.setX(leftBorder + buttonStyle->xthickness);
    else
        arrowRect.setX(rect.width() - rightBorder - buttonStyle->xthickness - arrowSize);
    GtkShadowType shadowType = isPressed(object) ? GTK_SHADOW_IN : GTK_SHADOW_OUT;

    WidgetRenderingContext widgetContext(info.context, rect);
    GtkStateType stateType = getGtkStateType(this, object);
    widgetContext.gtkPaintArrow(arrowRect, gtkComboBoxArrow(), stateType, shadowType, GTK_ARROW_DOWN, "arrow");

    // Some combo boxes do not have a separator.
    GtkWidget* separator = gtkComboBoxSeparator();
    if (!separator)
        return false;

    // We want to decrease the height of the separator based on the focus padding of the button.
    gint focusPadding = 0, focusWidth = 0; 
    gtk_widget_style_get(gtkComboBoxButton(),
                         "focus-line-width", &focusWidth,
                         "focus-padding", &focusPadding, NULL);
    topBorder += focusPadding + focusWidth;
    bottomBorder += focusPadding + focusWidth;
    int separatorWidth = getComboBoxSeparatorWidth();
    IntRect separatorRect(0, topBorder, separatorWidth, rect.height() - topBorder - bottomBorder);
    if (style->direction() == RTL)
        separatorRect.setX(arrowRect.x() + arrowRect.width() + buttonStyle->xthickness + separatorWidth);
    else
        separatorRect.setX(arrowRect.x() - buttonStyle->xthickness - separatorWidth);

    gboolean hasWideSeparators = FALSE;
    gtk_widget_style_get(separator, "wide-separators", &hasWideSeparators, NULL);
    if (hasWideSeparators)
        widgetContext.gtkPaintBox(separatorRect, separator, GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_OUT, "vseparator");
    else
        widgetContext.gtkPaintVLine(separatorRect, separator, GTK_STATE_NORMAL, "vseparator");

    return false;
}

bool RenderThemeGtk::paintTextField(RenderObject* renderObject, const PaintInfo& info, const IntRect& rect)
{
    GtkWidget* widget = gtkEntry();

    bool enabled = isEnabled(renderObject) && !isReadOnlyControl(renderObject);
    GtkStateType backgroundState = enabled ? GTK_STATE_NORMAL : GTK_STATE_INSENSITIVE;
    gtk_widget_set_sensitive(widget, enabled);
    gtk_widget_set_direction(widget, gtkTextDirection(renderObject->style()->direction()));
    setWidgetHasFocus(widget, isFocused(renderObject));

    WidgetRenderingContext widgetContext(info.context, rect);
    IntRect textFieldRect(IntPoint(), rect.size());

    // The entry background is only painted over the interior part of the GTK+ entry, not
    // the entire frame. This happens in the Mozilla theme drawing code as well.
    IntRect interiorRect(textFieldRect);
    GtkStyle* style = gtk_widget_get_style(widget);
    interiorRect.inflateX(-style->xthickness);
    interiorRect.inflateY(-style->ythickness);
    widgetContext.gtkPaintFlatBox(interiorRect, widget, backgroundState, GTK_SHADOW_NONE, "entry_bg");

    // This is responsible for drawing the actual frame.
    widgetContext.gtkPaintShadow(textFieldRect, widget, GTK_STATE_NORMAL, GTK_SHADOW_IN, "entry");

    gboolean interiorFocus;
    gint focusWidth;
    gtk_widget_style_get(widget,
                         "interior-focus", &interiorFocus,
                         "focus-line-width", &focusWidth,  NULL);
    if (isFocused(renderObject) && !interiorFocus) {
        // When GTK+ paints a text entry with focus, it shrinks the size of the frame area by the
        // focus width and paints over the previously unfocused text entry. We need to emulate that
        // by drawing both the unfocused frame above and the focused frame here.
        IntRect shadowRect(textFieldRect);
        shadowRect.inflate(-focusWidth);
        widgetContext.gtkPaintShadow(shadowRect, widget, GTK_STATE_NORMAL, GTK_SHADOW_IN, "entry");

        widgetContext.gtkPaintFocus(textFieldRect, widget, GTK_STATE_NORMAL, "entry");
    }

    return false;
}

bool RenderThemeGtk::paintSliderTrack(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    if (info.context->paintingDisabled())
        return false;

    ControlPart part = object->style()->appearance();
    ASSERT(part == SliderHorizontalPart || part == SliderVerticalPart || part == MediaVolumeSliderPart);

    // We shrink the trough rect slightly to make room for the focus indicator.
    IntRect troughRect(IntPoint(), rect.size()); // This is relative to rect.
    GtkWidget* widget = 0;
    if (part == SliderHorizontalPart) {
        widget = gtkHScale();
        troughRect.inflateX(-gtk_widget_get_style(widget)->xthickness);
    } else {
        widget = gtkVScale();
        troughRect.inflateY(-gtk_widget_get_style(widget)->ythickness);
    }
    gtk_widget_set_direction(widget, gtkTextDirection(object->style()->direction()));

    WidgetRenderingContext widgetContext(info.context, rect);
    widgetContext.gtkPaintBox(troughRect, widget, GTK_STATE_ACTIVE, GTK_SHADOW_OUT, "trough");
    if (isFocused(object))
        widgetContext.gtkPaintFocus(IntRect(IntPoint(), rect.size()), widget, getGtkStateType(this, object), "trough");

    return false;
}

bool RenderThemeGtk::paintSliderThumb(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    if (info.context->paintingDisabled())
        return false;

    ControlPart part = object->style()->appearance();
    ASSERT(part == SliderThumbHorizontalPart || part == SliderThumbVerticalPart || part == MediaVolumeSliderThumbPart);

    GtkWidget* widget = 0;
    const char* detail = 0;
    GtkOrientation orientation;
    if (part == SliderThumbHorizontalPart) {
        widget = gtkHScale();
        detail = "hscale";
        orientation = GTK_ORIENTATION_HORIZONTAL;
    } else {
        widget = gtkVScale();
        detail = "vscale";
        orientation = GTK_ORIENTATION_VERTICAL;
    }
    gtk_widget_set_direction(widget, gtkTextDirection(object->style()->direction()));

    // Only some themes have slider thumbs respond to clicks and some don't. This information is
    // gathered via the 'activate-slider' property, but it's deprecated in GTK+ 2.22 and removed in
    // GTK+ 3.x. The drawback of not honoring it is that slider thumbs change color when you click
    // on them. 
    IntRect thumbRect(IntPoint(), rect.size());
    WidgetRenderingContext widgetContext(info.context, rect);
    widgetContext.gtkPaintSlider(thumbRect, widget, getGtkStateType(this, object), GTK_SHADOW_OUT, detail, orientation);
    return false;
}

void RenderThemeGtk::adjustSliderThumbSize(RenderStyle* style, Element*) const
{
    ControlPart part = style->appearance();
    if (part != SliderThumbHorizontalPart && part != SliderThumbVerticalPart)
        return;

    GtkWidget* widget = part == SliderThumbHorizontalPart ? gtkHScale() : gtkVScale();
    int length = 0, width = 0;
    gtk_widget_style_get(widget,
                         "slider_length", &length,
                         "slider_width", &width,
                         NULL);

    if (part == SliderThumbHorizontalPart) {
        style->setWidth(Length(length, Fixed));
        style->setHeight(Length(width, Fixed));
        return;
    }
    ASSERT(part == SliderThumbVerticalPart || part == MediaVolumeSliderThumbPart);
    style->setWidth(Length(width, Fixed));
    style->setHeight(Length(length, Fixed));
}

#if ENABLE(PROGRESS_ELEMENT)
bool RenderThemeGtk::paintProgressBar(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    GtkWidget* widget = gtkProgressBar();
    gtk_widget_set_direction(widget, gtkTextDirection(renderObject->style()->direction()));

    WidgetRenderingContext widgetContext(paintInfo.context, rect);
    IntRect fullProgressBarRect(IntPoint(), rect.size());
    widgetContext.gtkPaintBox(fullProgressBarRect, widget, GTK_STATE_NORMAL, GTK_SHADOW_IN, "trough");

    GtkStyle* style = gtk_widget_get_style(widget);
    IntRect progressRect(fullProgressBarRect);
    progressRect.inflateX(-style->xthickness);
    progressRect.inflateY(-style->ythickness);
    progressRect = RenderThemeGtk::calculateProgressRect(renderObject, progressRect);

    if (!progressRect.isEmpty())
        widgetContext.gtkPaintBox(progressRect, widget, GTK_STATE_PRELIGHT, GTK_SHADOW_OUT, "bar");

    return false;
}
#endif

void RenderThemeGtk::adjustInnerSpinButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    GtkStyle* gtkStyle = gtk_widget_get_style(gtkSpinButton());
    const PangoFontDescription* fontDescription = gtkStyle->font_desc;
    gint fontSize = pango_font_description_get_size(fontDescription);

    // Force an odd arrow size here. GTK+ 3.x forces even in this case, but
    // Nodoka-based themes look incorrect with an even arrow size.
    int width = max(PANGO_PIXELS(fontSize), minSpinButtonArrowSize);
    width += -((width % 2) - 1) + gtkStyle->xthickness;

    style->setWidth(Length(width, Fixed));
    style->setMinWidth(Length(width, Fixed));
}

bool RenderThemeGtk::paintInnerSpinButton(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    // We expand the painted area by 2 pixels on the top and bottom and 2 pixels on the right. This
    // is because GTK+ themes want to draw over the text box borders, but WebCore renders the inner
    // spin button inside the text box.
    IntRect expandedRect(rect);
    expandedRect.inflateY(2);
    expandedRect.setWidth(rect.width() + 2);

    WidgetRenderingContext widgetContext(paintInfo.context, expandedRect);
    GtkWidget* widget = gtkSpinButton();
    gtk_widget_set_direction(widget, gtkTextDirection(renderObject->style()->direction()));

    IntRect fullSpinButtonRect(IntPoint(), expandedRect.size());
    widgetContext.gtkPaintBox(fullSpinButtonRect, widget, GTK_STATE_NORMAL, GTK_SHADOW_IN, "spinbutton");

    bool upPressed = isSpinUpButtonPartPressed(renderObject);
    bool upHovered = isSpinUpButtonPartHovered(renderObject);
    bool controlActive = isEnabled(renderObject) && !isReadOnlyControl(renderObject);
    GtkShadowType shadowType = upPressed ? GTK_SHADOW_IN : GTK_SHADOW_OUT;

    GtkStateType stateType = GTK_STATE_INSENSITIVE;
    if (controlActive) {
        if (isPressed(renderObject) && upPressed)
            stateType = GTK_STATE_ACTIVE;
        else if (isHovered(renderObject) && upHovered)
            stateType = GTK_STATE_PRELIGHT;
        else
            stateType = GTK_STATE_NORMAL;
    }
    IntRect topRect(IntPoint(), expandedRect.size());
    topRect.setHeight(expandedRect.height() / 2);
    widgetContext.gtkPaintBox(topRect, widget, stateType, shadowType, "spinbutton_up");

    // The arrow size/position calculation here is based on the arbitrary gymnastics that happen
    // in gtkspinbutton.c. It isn't pretty there and it isn't pretty here. This manages to make
    // the button look native for many themes though.
    IntRect arrowRect;
    int arrowSize = (expandedRect.width() - 3) / 2;
    arrowSize -= (arrowSize % 2) - 1; // Force odd.
    arrowRect.setWidth(arrowSize);
    arrowRect.setHeight(arrowSize);
    arrowRect.move((expandedRect.width() - arrowRect.width()) / 2,
                   (topRect.height() - arrowRect.height()) / 2 + 1);
    widgetContext.gtkPaintArrow(arrowRect, widget, stateType, shadowType, GTK_ARROW_UP, "spinbutton");

    shadowType = isPressed(renderObject) && !upPressed ? GTK_SHADOW_IN : GTK_SHADOW_OUT;
    if (controlActive) {
        if (isPressed(renderObject) && !upPressed)
            stateType = GTK_STATE_ACTIVE;
        else if (isHovered(renderObject) && !upHovered)
            stateType = GTK_STATE_PRELIGHT;
        else
            stateType = GTK_STATE_NORMAL;
    }
    IntRect bottomRect(IntPoint(0, expandedRect.height() / 2), expandedRect.size());
    bottomRect.setHeight(expandedRect.height() - bottomRect.y());
    widgetContext.gtkPaintBox(bottomRect, widget, stateType, shadowType, "spinbutton_down");

    arrowRect.setY(arrowRect.y() + bottomRect.y() - 1);
    widgetContext.gtkPaintArrow(arrowRect, widget, stateType, shadowType, GTK_ARROW_DOWN, "spinbutton");

    return false;
}

GRefPtr<GdkPixbuf> getStockIconForWidgetType(GType widgetType, const char* iconName, gint direction, gint state, gint iconSize)
{
    ASSERT(widgetType == GTK_TYPE_CONTAINER || widgetType == GTK_TYPE_ENTRY);

    RenderThemeGtk* theme = static_cast<RenderThemeGtk*>(RenderTheme::defaultTheme().get());
    GtkWidget* widget = widgetType == GTK_TYPE_CONTAINER ? GTK_WIDGET(theme->gtkContainer()) : theme->gtkEntry();

    GtkStyle* style = gtk_widget_get_style(widget);
    GtkIconSet* iconSet = gtk_style_lookup_icon_set(style, iconName);
    return adoptGRef(gtk_icon_set_render_icon(iconSet, style,
                                              static_cast<GtkTextDirection>(direction),
                                              static_cast<GtkStateType>(state),
                                              static_cast<GtkIconSize>(iconSize), 0, 0));
}

GRefPtr<GdkPixbuf> getStockSymbolicIconForWidgetType(GType widgetType, const char* symbolicIconName, const char *fallbackStockIconName, gint direction, gint state, gint iconSize)
{
    return getStockIconForWidgetType(widgetType, fallbackStockIconName, direction, state, iconSize);
}

Color RenderThemeGtk::platformActiveSelectionBackgroundColor() const
{
    GtkWidget* widget = gtkEntry();
    return gtk_widget_get_style(widget)->base[GTK_STATE_SELECTED];
}

Color RenderThemeGtk::platformInactiveSelectionBackgroundColor() const
{
    GtkWidget* widget = gtkEntry();
    return gtk_widget_get_style(widget)->base[GTK_STATE_ACTIVE];
}

Color RenderThemeGtk::platformActiveSelectionForegroundColor() const
{
    GtkWidget* widget = gtkEntry();
    return gtk_widget_get_style(widget)->text[GTK_STATE_SELECTED];
}

Color RenderThemeGtk::platformInactiveSelectionForegroundColor() const
{
    GtkWidget* widget = gtkEntry();
    return gtk_widget_get_style(widget)->text[GTK_STATE_ACTIVE];
}

Color RenderThemeGtk::activeListBoxSelectionBackgroundColor() const
{
    GtkWidget* widget = gtkTreeView();
    return gtk_widget_get_style(widget)->base[GTK_STATE_SELECTED];
}

Color RenderThemeGtk::inactiveListBoxSelectionBackgroundColor() const
{
    GtkWidget* widget = gtkTreeView();
    return gtk_widget_get_style(widget)->base[GTK_STATE_ACTIVE];
}

Color RenderThemeGtk::activeListBoxSelectionForegroundColor() const
{
    GtkWidget* widget = gtkTreeView();
    return gtk_widget_get_style(widget)->text[GTK_STATE_SELECTED];
}

Color RenderThemeGtk::inactiveListBoxSelectionForegroundColor() const
{
    GtkWidget* widget = gtkTreeView();
    return gtk_widget_get_style(widget)->text[GTK_STATE_ACTIVE];
}

Color RenderThemeGtk::systemColor(CSSValueID cssValueId) const
{
    switch (cssValueId) {
    case CSSValueButtontext:
        return Color(gtk_widget_get_style(gtkButton())->fg[GTK_STATE_NORMAL]);
    case CSSValueCaptiontext:
        return Color(gtk_widget_get_style(gtkEntry())->fg[GTK_STATE_NORMAL]);
    default:
        return RenderTheme::systemColor(cssValueId);
    }
}

static void gtkStyleSetCallback(GtkWidget* widget, GtkStyle* previous, RenderTheme* renderTheme)
{
    // FIXME: Make sure this function doesn't get called many times for a single GTK+ style change signal.
    renderTheme->platformColorsDidChange();
}

static void setupWidget(GtkWidget* widget)
{
    gtk_widget_realize(widget);
    g_object_set_data(G_OBJECT(widget), "transparent-bg-hint", GINT_TO_POINTER(TRUE));
}

void RenderThemeGtk::setupWidgetAndAddToContainer(GtkWidget* widget, GtkWidget* window) const
{
    gtk_container_add(GTK_CONTAINER(window), widget);
    setupWidget(widget);

    // FIXME: Perhaps this should only be called for the containing window or parent container.
    g_signal_connect(widget, "style-set", G_CALLBACK(gtkStyleSetCallback), const_cast<RenderThemeGtk*>(this));
}

GtkWidget* RenderThemeGtk::gtkContainer() const
{
    if (m_gtkContainer)
        return m_gtkContainer;

    m_gtkWindow = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_widget_set_colormap(m_gtkWindow, m_colormap);
    setupWidget(m_gtkWindow);
    gtk_widget_set_name(m_gtkWindow, "MozillaGtkWidget");

    m_gtkContainer = gtk_fixed_new();
    setupWidgetAndAddToContainer(m_gtkContainer, m_gtkWindow);
    return m_gtkContainer;
}

GtkWidget* RenderThemeGtk::gtkButton() const
{
    if (m_gtkButton)
        return m_gtkButton;
    m_gtkButton = gtk_button_new();
    setupWidgetAndAddToContainer(m_gtkButton, gtkContainer());
    return m_gtkButton;
}

GtkWidget* RenderThemeGtk::gtkEntry() const
{
    if (m_gtkEntry)
        return m_gtkEntry;
    m_gtkEntry = gtk_entry_new();
    setupWidgetAndAddToContainer(m_gtkEntry, gtkContainer());
    return m_gtkEntry;
}

GtkWidget* RenderThemeGtk::gtkTreeView() const
{
    if (m_gtkTreeView)
        return m_gtkTreeView;
    m_gtkTreeView = gtk_tree_view_new();
    setupWidgetAndAddToContainer(m_gtkTreeView, gtkContainer());
    return m_gtkTreeView;
}

GtkWidget* RenderThemeGtk::gtkVScale() const
{
    if (m_gtkVScale)
        return m_gtkVScale;
    m_gtkVScale = gtk_vscale_new(0);
    setupWidgetAndAddToContainer(m_gtkVScale, gtkContainer());
    return m_gtkVScale;
}

GtkWidget* RenderThemeGtk::gtkHScale() const
{
    if (m_gtkHScale)
        return m_gtkHScale;
    m_gtkHScale = gtk_hscale_new(0);
    setupWidgetAndAddToContainer(m_gtkHScale, gtkContainer());
    return m_gtkHScale;
}

GtkWidget* RenderThemeGtk::gtkRadioButton() const
{
    if (m_gtkRadioButton)
        return m_gtkRadioButton;
    m_gtkRadioButton = gtk_radio_button_new(0);
    setupWidgetAndAddToContainer(m_gtkRadioButton, gtkContainer());
    return m_gtkRadioButton;
}

GtkWidget* RenderThemeGtk::gtkCheckButton() const
{
    if (m_gtkCheckButton)
        return m_gtkCheckButton;
    m_gtkCheckButton = gtk_check_button_new();
    setupWidgetAndAddToContainer(m_gtkCheckButton, gtkContainer());
    return m_gtkCheckButton;
}

GtkWidget* RenderThemeGtk::gtkProgressBar() const
{
    if (m_gtkProgressBar)
        return m_gtkProgressBar;
    m_gtkProgressBar = gtk_progress_bar_new();
    setupWidgetAndAddToContainer(m_gtkProgressBar, gtkContainer());
    return m_gtkProgressBar;
}

static void getGtkComboBoxButton(GtkWidget* widget, gpointer target)
{
    if (!GTK_IS_TOGGLE_BUTTON(widget))
        return;
    GtkWidget** widgetTarget = static_cast<GtkWidget**>(target);
    *widgetTarget = widget;
}

typedef struct {
    GtkWidget* arrow;
    GtkWidget* separator;
} ComboBoxWidgetPieces;

static void getGtkComboBoxPieces(GtkWidget* widget, gpointer data)
{
    if (GTK_IS_ARROW(widget)) {
        static_cast<ComboBoxWidgetPieces*>(data)->arrow = widget;
        return;
    }
    if (GTK_IS_SEPARATOR(widget)) 
        static_cast<ComboBoxWidgetPieces*>(data)->separator = widget;
}

GtkWidget* RenderThemeGtk::gtkComboBox() const
{
    if (m_gtkComboBox)
        return m_gtkComboBox;
    m_gtkComboBox = gtk_combo_box_new();
    setupWidgetAndAddToContainer(m_gtkComboBox, gtkContainer());
    return m_gtkComboBox;
}

void RenderThemeGtk::refreshComboBoxChildren() const
{
    gtkComboBox(); // Ensure that we've initialized the combo box.

    // Some themes look at widget ancestry to determine how to render widgets, so
    // get the GtkButton that is the actual child of the combo box.
    gtk_container_forall(GTK_CONTAINER(m_gtkComboBox), getGtkComboBoxButton, &m_gtkComboBoxButton);
    ASSERT(m_gtkComboBoxButton);
    setupWidget(m_gtkComboBoxButton);
    g_object_add_weak_pointer(G_OBJECT(m_gtkComboBoxButton), reinterpret_cast<gpointer*>(&m_gtkComboBoxButton));

    ComboBoxWidgetPieces pieces = { 0, 0 };
    GtkWidget* buttonChild = gtk_bin_get_child(GTK_BIN(gtkComboBoxButton()));
    if (GTK_IS_HBOX(buttonChild))
        gtk_container_forall(GTK_CONTAINER(buttonChild), getGtkComboBoxPieces, &pieces);
    else if (GTK_IS_ARROW(buttonChild))
        pieces.arrow = buttonChild;

    ASSERT(pieces.arrow);
    m_gtkComboBoxArrow = pieces.arrow;
    setupWidget(m_gtkComboBoxArrow);
    // When the style changes, the combo box may destroy its children.
    g_object_add_weak_pointer(G_OBJECT(m_gtkComboBoxArrow), reinterpret_cast<gpointer*>(&m_gtkComboBoxArrow));

    m_gtkComboBoxSeparator = pieces.separator;
    if (m_gtkComboBoxSeparator) {
        setupWidget(m_gtkComboBoxSeparator);
        // When the style changes, the combo box may destroy its children.
        g_object_add_weak_pointer(G_OBJECT(m_gtkComboBoxSeparator), reinterpret_cast<gpointer*>(&m_gtkComboBoxSeparator));
    }
}

GtkWidget* RenderThemeGtk::gtkComboBoxButton() const
{
    if (m_gtkComboBoxButton)
        return m_gtkComboBoxButton;
    refreshComboBoxChildren();
    ASSERT(m_gtkComboBoxButton);
    return m_gtkComboBoxButton;
}

GtkWidget* RenderThemeGtk::gtkComboBoxArrow() const
{
    if (m_gtkComboBoxArrow)
        return m_gtkComboBoxArrow;
    refreshComboBoxChildren();
    ASSERT(m_gtkComboBoxArrow);
    return m_gtkComboBoxArrow;
}

GtkWidget* RenderThemeGtk::gtkComboBoxSeparator() const
{
    // m_gtkComboBoxSeparator may be null either because we haven't initialized the combo box
    // or because the combo boxes in this theme don't have separators. If m_gtkComboBoxArrow
    // arrow isn't null, we definitely have initialized the combo box.
    if (m_gtkComboBoxArrow || m_gtkComboBoxButton)
        return m_gtkComboBoxSeparator;
    refreshComboBoxChildren();
    return m_gtkComboBoxSeparator;
}

GtkWidget* RenderThemeGtk::gtkHScrollbar() const
{
    if (m_gtkHScrollbar)
        return m_gtkHScrollbar;
    m_gtkHScrollbar = gtk_hscrollbar_new(0);
    setupWidgetAndAddToContainer(m_gtkHScrollbar, gtkContainer());
    return m_gtkHScrollbar;
}

GtkWidget* RenderThemeGtk::gtkVScrollbar() const
{
    if (m_gtkVScrollbar)
        return m_gtkVScrollbar;
    m_gtkVScrollbar = gtk_vscrollbar_new(0);
    setupWidgetAndAddToContainer(m_gtkVScrollbar, gtkContainer());
    return m_gtkVScrollbar;
}

GtkWidget* RenderThemeGtk::gtkSpinButton() const
{
    if (m_gtkSpinButton)
        return m_gtkSpinButton;
    m_gtkSpinButton = gtk_spin_button_new_with_range(0, 10, 1);
    setupWidgetAndAddToContainer(m_gtkSpinButton, gtkContainer());
    return m_gtkSpinButton;
}

} // namespace WebCore

#endif // GTK_API_VERSION_2

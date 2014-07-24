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

#include "CSSValueKeywords.h"
#include "ExceptionCodePlaceholder.h"
#include "FileList.h"
#include "FileSystem.h"
#include "FontDescription.h"
#include <wtf/gobject/GOwnPtr.h>
#include "Gradient.h"
#include "GraphicsContext.h"
#include "GtkVersioning.h"
#include "HTMLMediaElement.h"
#include "LocalizedStrings.h"
#include "MediaControlElements.h"
#include "PaintInfo.h"
#include "PlatformContextCairo.h"
#include "RenderBox.h"
#include "RenderObject.h"
#include "StringTruncator.h"
#include "TimeRanges.h"
#include "UserAgentStyleSheets.h"
#include <cmath>
#include <gdk/gdk.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <wtf/text/CString.h>

#if ENABLE(PROGRESS_ELEMENT)
#include "RenderProgress.h"
#endif

namespace WebCore {

// This would be a static method, except that forward declaring GType is tricky, since its
// definition depends on including glib.h, negating the benefit of using a forward declaration.
extern GRefPtr<GdkPixbuf> getStockIconForWidgetType(GType, const char* iconName, gint direction, gint state, gint iconSize);
extern GRefPtr<GdkPixbuf> getStockSymbolicIconForWidgetType(GType widgetType, const char* symbolicIconName, const char *fallbackStockIconName, gint direction, gint state, gint iconSize);

#if ENABLE(VIDEO)
static HTMLMediaElement* getMediaElementFromRenderObject(RenderObject* o)
{
    Node* node = o->node();
    Node* mediaNode = node ? node->shadowHost() : 0;
    if (!mediaNode)
        mediaNode = node;
    if (!mediaNode || !mediaNode->isElementNode() || !toElement(mediaNode)->isMediaElement())
        return 0;

    return toHTMLMediaElement(mediaNode);
}

void RenderThemeGtk::initMediaButtons()
{
    static bool iconsInitialized = false;

    if (iconsInitialized)
        return;

    GRefPtr<GtkIconFactory> iconFactory = adoptGRef(gtk_icon_factory_new());
    GtkIconSource* iconSource = gtk_icon_source_new();
    const char* icons[] = { "audio-volume-high", "audio-volume-muted" };

    gtk_icon_factory_add_default(iconFactory.get());

    for (size_t i = 0; i < G_N_ELEMENTS(icons); ++i) {
        gtk_icon_source_set_icon_name(iconSource, icons[i]);
        GtkIconSet* iconSet = gtk_icon_set_new();
        gtk_icon_set_add_source(iconSet, iconSource);
        gtk_icon_factory_add(iconFactory.get(), icons[i], iconSet);
        gtk_icon_set_unref(iconSet);
    }

    gtk_icon_source_free(iconSource);

    iconsInitialized = true;
}
#endif

PassRefPtr<RenderTheme> RenderThemeGtk::create()
{
    return adoptRef(new RenderThemeGtk());
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    static RenderTheme* rt = RenderThemeGtk::create().leakRef();
    return rt;
}

RenderThemeGtk::RenderThemeGtk()
    : m_panelColor(Color::white)
    , m_sliderColor(Color::white)
    , m_sliderThumbColor(Color::white)
    , m_mediaIconSize(16)
    , m_mediaSliderHeight(14)
{
    platformInit();
#if ENABLE(VIDEO)
    initMediaColors();
    initMediaButtons();
#endif
}

static bool supportsFocus(ControlPart appearance)
{
    switch (appearance) {
    case PushButtonPart:
    case ButtonPart:
    case TextFieldPart:
    case TextAreaPart:
    case SearchFieldPart:
    case MenulistPart:
    case RadioPart:
    case CheckboxPart:
    case SliderHorizontalPart:
    case SliderVerticalPart:
    case MediaPlayButtonPart:
    case MediaVolumeSliderPart:
    case MediaMuteButtonPart:
    case MediaEnterFullscreenButtonPart:
    case MediaSliderPart:
        return true;
    default:
        return false;
    }
}

bool RenderThemeGtk::supportsFocusRing(const RenderStyle* style) const
{
    return supportsFocus(style->appearance());
}

bool RenderThemeGtk::controlSupportsTints(const RenderObject* o) const
{
    return isEnabled(o);
}

int RenderThemeGtk::baselinePosition(const RenderObject* o) const
{
    if (!o->isBox())
        return 0;

    // FIXME: This strategy is possibly incorrect for the GTK+ port.
    if (o->style()->appearance() == CheckboxPart
        || o->style()->appearance() == RadioPart) {
        const RenderBox* box = toRenderBox(o);
        return box->marginTop() + box->height() - 2;
    }

    return RenderTheme::baselinePosition(o);
}

// This is used in RenderThemeGtk2 and RenderThemeGtk3. Normally, it would be in
// the RenderThemeGtk header (perhaps as a static method), but we want to avoid
// having to include GTK+ headers only for the GtkTextDirection enum.
GtkTextDirection gtkTextDirection(TextDirection direction)
{
    switch (direction) {
    case RTL:
        return GTK_TEXT_DIR_RTL;
    case LTR:
        return GTK_TEXT_DIR_LTR;
    default:
        return GTK_TEXT_DIR_NONE;
    }
}

static GtkStateType gtkIconState(RenderTheme* theme, RenderObject* renderObject)
{
    if (!theme->isEnabled(renderObject))
        return GTK_STATE_INSENSITIVE;
    if (theme->isPressed(renderObject))
        return GTK_STATE_ACTIVE;
    if (theme->isHovered(renderObject))
        return GTK_STATE_PRELIGHT;

    return GTK_STATE_NORMAL;
}

void RenderThemeGtk::adjustButtonStyle(StyleResolver*, RenderStyle* style, WebCore::Element*) const
{
    // Some layout tests check explicitly that buttons ignore line-height.
    if (style->appearance() == PushButtonPart)
        style->setLineHeight(RenderStyle::initialLineHeight());
}

void RenderThemeGtk::adjustMenuListStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // The tests check explicitly that select menu buttons ignore line height.
    style->setLineHeight(RenderStyle::initialLineHeight());

    // We cannot give a proper rendering when border radius is active, unfortunately.
    style->resetBorderRadius();
}

void RenderThemeGtk::adjustMenuListButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    adjustMenuListStyle(styleResolver, style, e);
}

bool RenderThemeGtk::paintMenuListButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintMenuList(object, info, rect);
}

bool RenderThemeGtk::paintTextArea(RenderObject* o, const PaintInfo& i, const IntRect& r)
{
    return paintTextField(o, i, r);
}

static void paintGdkPixbuf(GraphicsContext* context, const GdkPixbuf* icon, const IntRect& iconRect)
{
    IntSize iconSize(gdk_pixbuf_get_width(icon), gdk_pixbuf_get_height(icon));
    GRefPtr<GdkPixbuf> scaledIcon;
    if (iconRect.size() != iconSize) {
        // We could use cairo_scale() here but cairo/pixman downscale quality is quite bad.
        scaledIcon = adoptGRef(gdk_pixbuf_scale_simple(icon, iconRect.width(), iconRect.height(),
                                                       GDK_INTERP_BILINEAR));
        icon = scaledIcon.get();
    }

    cairo_t* cr = context->platformContext()->cr();
    cairo_save(cr);
    gdk_cairo_set_source_pixbuf(cr, icon, iconRect.x(), iconRect.y());
    cairo_paint(cr);
    cairo_restore(cr);
}

// Defined in GTK+ (gtk/gtkiconfactory.c)
static const gint gtkIconSizeMenu = 16;
static const gint gtkIconSizeSmallToolbar = 18;
static const gint gtkIconSizeButton = 20;
static const gint gtkIconSizeLargeToolbar = 24;
static const gint gtkIconSizeDnd = 32;
static const gint gtkIconSizeDialog = 48;

static GtkIconSize getIconSizeForPixelSize(gint pixelSize)
{
    if (pixelSize < gtkIconSizeSmallToolbar)
        return GTK_ICON_SIZE_MENU;
    if (pixelSize >= gtkIconSizeSmallToolbar && pixelSize < gtkIconSizeButton)
        return GTK_ICON_SIZE_SMALL_TOOLBAR;
    if (pixelSize >= gtkIconSizeButton && pixelSize < gtkIconSizeLargeToolbar)
        return GTK_ICON_SIZE_BUTTON;
    if (pixelSize >= gtkIconSizeLargeToolbar && pixelSize < gtkIconSizeDnd)
        return GTK_ICON_SIZE_LARGE_TOOLBAR;
    if (pixelSize >= gtkIconSizeDnd && pixelSize < gtkIconSizeDialog)
        return GTK_ICON_SIZE_DND;

    return GTK_ICON_SIZE_DIALOG;
}

void RenderThemeGtk::adjustSearchFieldResultsButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* e) const
{
    adjustSearchFieldCancelButtonStyle(styleResolver, style, e);
}

bool RenderThemeGtk::paintSearchFieldResultsButton(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    return paintSearchFieldResultsDecoration(o, i, rect);
}

static void adjustSearchFieldIconStyle(RenderStyle* style)
{
    style->resetBorder();
    style->resetPadding();

    // Get the icon size based on the font size.
    int fontSize = style->fontSize();
    if (fontSize < gtkIconSizeMenu) {
        style->setWidth(Length(fontSize, Fixed));
        style->setHeight(Length(fontSize, Fixed));
        return;
    }
    gint width = 0, height = 0;
    gtk_icon_size_lookup(getIconSizeForPixelSize(fontSize), &width, &height);
    style->setWidth(Length(width, Fixed));
    style->setHeight(Length(height, Fixed));
}

void RenderThemeGtk::adjustSearchFieldResultsDecorationStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    adjustSearchFieldIconStyle(style);
}

static IntRect centerRectVerticallyInParentInputElement(RenderObject* renderObject, const IntRect& rect)
{
    // Get the renderer of <input> element.
    Node* input = renderObject->node()->shadowHost();
    if (!input)
        input = renderObject->node();
    if (!input->renderer()->isBox())
        return IntRect();

    // If possible center the y-coordinate of the rect vertically in the parent input element.
    // We also add one pixel here to ensure that the y coordinate is rounded up for box heights
    // that are even, which looks in relation to the box text.
    IntRect inputContentBox = toRenderBox(input->renderer())->absoluteContentBox();

    // Make sure the scaled decoration stays square and will fit in its parent's box.
    int iconSize = std::min(inputContentBox.width(), std::min(inputContentBox.height(), rect.height()));
    IntRect scaledRect(rect.x(), inputContentBox.y() + (inputContentBox.height() - iconSize + 1) / 2, iconSize, iconSize);
    return scaledRect;
}

bool RenderThemeGtk::paintSearchFieldResultsDecoration(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    IntRect iconRect = centerRectVerticallyInParentInputElement(renderObject, rect);
    if (iconRect.isEmpty())
        return false;

    GRefPtr<GdkPixbuf> icon = getStockIconForWidgetType(GTK_TYPE_ENTRY, GTK_STOCK_FIND,
                                                        gtkTextDirection(renderObject->style()->direction()),
                                                        gtkIconState(this, renderObject),
                                                        getIconSizeForPixelSize(rect.height()));
    paintGdkPixbuf(paintInfo.context, icon.get(), iconRect);
    return false;
}

void RenderThemeGtk::adjustSearchFieldCancelButtonStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    adjustSearchFieldIconStyle(style);
}

bool RenderThemeGtk::paintSearchFieldCancelButton(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    IntRect iconRect = centerRectVerticallyInParentInputElement(renderObject, rect);
    if (iconRect.isEmpty())
        return false;

    GRefPtr<GdkPixbuf> icon = getStockIconForWidgetType(GTK_TYPE_ENTRY, GTK_STOCK_CLEAR,
                                                        gtkTextDirection(renderObject->style()->direction()),
                                                        gtkIconState(this, renderObject),
                                                        getIconSizeForPixelSize(rect.height()));
    paintGdkPixbuf(paintInfo.context, icon.get(), iconRect);
    return false;
}

void RenderThemeGtk::adjustSearchFieldStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    // We cannot give a proper rendering when border radius is active, unfortunately.
    style->resetBorderRadius();
    style->setLineHeight(RenderStyle::initialLineHeight());
}

bool RenderThemeGtk::paintSearchField(RenderObject* o, const PaintInfo& i, const IntRect& rect)
{
    return paintTextField(o, i, rect);
}

bool RenderThemeGtk::paintCapsLockIndicator(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    // The other paint methods don't need to check whether painting is disabled because RenderTheme already checks it
    // before calling them, but paintCapsLockIndicator() is called by RenderTextControlSingleLine which doesn't check it.
    if (paintInfo.context->paintingDisabled())
        return true;

    int iconSize = std::min(rect.width(), rect.height());
    GRefPtr<GdkPixbuf> icon = getStockIconForWidgetType(GTK_TYPE_ENTRY, GTK_STOCK_CAPS_LOCK_WARNING,
                                                        gtkTextDirection(renderObject->style()->direction()),
                                                        0, getIconSizeForPixelSize(iconSize));

    // Only re-scale the icon when it's smaller than the minimum icon size.
    if (iconSize >= gtkIconSizeMenu)
        iconSize = gdk_pixbuf_get_height(icon.get());

    // GTK+ locates the icon right aligned in the entry. The given rectangle is already
    // centered vertically by RenderTextControlSingleLine.
    IntRect iconRect(rect.x() + rect.width() - iconSize,
                     rect.y() + (rect.height() - iconSize) / 2,
                     iconSize, iconSize);
    paintGdkPixbuf(paintInfo.context, icon.get(), iconRect);
    return true;
}

void RenderThemeGtk::adjustSliderTrackStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(nullptr);
}

void RenderThemeGtk::adjustSliderThumbStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    RenderTheme::adjustSliderThumbStyle(styleResolver, style, element);
    style->setBoxShadow(nullptr);
}

double RenderThemeGtk::caretBlinkInterval() const
{
    GtkSettings* settings = gtk_settings_get_default();

    gboolean shouldBlink;
    gint time;

    g_object_get(settings, "gtk-cursor-blink", &shouldBlink, "gtk-cursor-blink-time", &time, NULL);

    if (!shouldBlink)
        return 0;

    return time / 2000.;
}

double RenderThemeGtk::getScreenDPI()
{
    // FIXME: Really this should be the widget's screen.
    GdkScreen* screen = gdk_screen_get_default();
    if (!screen)
        return 96; // Default to 96 DPI.

    float dpi = gdk_screen_get_resolution(screen);
    if (dpi <= 0)
        return 96;
    return dpi;
}

void RenderThemeGtk::systemFont(CSSValueID, FontDescription& fontDescription) const
{
    GtkSettings* settings = gtk_settings_get_default();
    if (!settings)
        return;

    // This will be a font selection string like "Sans 10" so we cannot use it as the family name.
    GOwnPtr<gchar> fontName;
    g_object_get(settings, "gtk-font-name", &fontName.outPtr(), NULL);

    PangoFontDescription* pangoDescription = pango_font_description_from_string(fontName.get());
    if (!pangoDescription)
        return;

    fontDescription.setOneFamily(pango_font_description_get_family(pangoDescription));

    int size = pango_font_description_get_size(pangoDescription) / PANGO_SCALE;
    // If the size of the font is in points, we need to convert it to pixels.
    if (!pango_font_description_get_size_is_absolute(pangoDescription))
        size = size * (getScreenDPI() / 72.0);

    fontDescription.setSpecifiedSize(size);
    fontDescription.setIsAbsoluteSize(true);
    fontDescription.setGenericFamily(FontDescription::NoFamily);
    fontDescription.setWeight(FontWeightNormal);
    fontDescription.setItalic(false);
    pango_font_description_free(pangoDescription);
}

void RenderThemeGtk::platformColorsDidChange()
{
#if ENABLE(VIDEO)
    initMediaColors();
#endif
    RenderTheme::platformColorsDidChange();
}

#if ENABLE(VIDEO)
String RenderThemeGtk::extraMediaControlsStyleSheet()
{
    return String(mediaControlsGtkUserAgentStyleSheet, sizeof(mediaControlsGtkUserAgentStyleSheet));
}

#if ENABLE(FULLSCREEN_API)
String RenderThemeGtk::extraFullScreenStyleSheet()
{
    return String();
}
#endif

bool RenderThemeGtk::paintMediaButton(RenderObject* renderObject, GraphicsContext* context, const IntRect& rect, const char* symbolicIconName, const char* fallbackStockIconName)
{
    IntRect iconRect(rect.x() + (rect.width() - m_mediaIconSize) / 2,
                     rect.y() + (rect.height() - m_mediaIconSize) / 2,
                     m_mediaIconSize, m_mediaIconSize);
    GRefPtr<GdkPixbuf> icon = getStockSymbolicIconForWidgetType(GTK_TYPE_CONTAINER, symbolicIconName, fallbackStockIconName,
        gtkTextDirection(renderObject->style()->direction()), gtkIconState(this, renderObject), iconRect.width());
    paintGdkPixbuf(context, icon.get(), iconRect);
    return false;
}

bool RenderThemeGtk::hasOwnDisabledStateHandlingFor(ControlPart part) const
{
    return (part != MediaMuteButtonPart);
}

bool RenderThemeGtk::paintMediaFullscreenButton(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    return paintMediaButton(renderObject, paintInfo.context, rect, "view-fullscreen-symbolic", GTK_STOCK_FULLSCREEN);
}

bool RenderThemeGtk::paintMediaMuteButton(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    HTMLMediaElement* mediaElement = getMediaElementFromRenderObject(renderObject);
    if (!mediaElement)
        return false;

    bool muted = mediaElement->muted();
    return paintMediaButton(renderObject, paintInfo.context, rect,
        muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic",
        muted ? "audio-volume-muted" : "audio-volume-high");
}

bool RenderThemeGtk::paintMediaPlayButton(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    Node* node = renderObject->node();
    if (!node)
        return false;
    if (!node->isMediaControlElement())
        return false;

    bool play = mediaControlElementType(node) == MediaPlayButton;
    return paintMediaButton(renderObject, paintInfo.context, rect,
        play ? "media-playback-start-symbolic" : "media-playback-pause-symbolic",
        play ? GTK_STOCK_MEDIA_PLAY : GTK_STOCK_MEDIA_PAUSE);
}

bool RenderThemeGtk::paintMediaSeekBackButton(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    return paintMediaButton(renderObject, paintInfo.context, rect, "media-seek-backward-symbolic", GTK_STOCK_MEDIA_REWIND);
}

bool RenderThemeGtk::paintMediaSeekForwardButton(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    return paintMediaButton(renderObject, paintInfo.context, rect, "media-seek-forward-symbolic", GTK_STOCK_MEDIA_FORWARD);
}

static RoundedRect::Radii borderRadiiFromStyle(RenderStyle* style)
{
    return RoundedRect::Radii(
        IntSize(style->borderTopLeftRadius().width().intValue(), style->borderTopLeftRadius().height().intValue()),
        IntSize(style->borderTopRightRadius().width().intValue(), style->borderTopRightRadius().height().intValue()),
        IntSize(style->borderBottomLeftRadius().width().intValue(), style->borderBottomLeftRadius().height().intValue()),
        IntSize(style->borderBottomRightRadius().width().intValue(), style->borderBottomRightRadius().height().intValue()));
}

bool RenderThemeGtk::paintMediaSliderTrack(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(o);
    if (!mediaElement)
        return false;

    GraphicsContext* context = paintInfo.context;
    context->save();
    context->setStrokeStyle(NoStroke);

    float mediaDuration = mediaElement->duration();
    float totalTrackWidth = r.width();
    RenderStyle* style = o->style();
    RefPtr<TimeRanges> timeRanges = mediaElement->buffered();
    for (unsigned index = 0; index < timeRanges->length(); ++index) {
        float start = timeRanges->start(index, IGNORE_EXCEPTION);
        float end = timeRanges->end(index, IGNORE_EXCEPTION);
        float startRatio = start / mediaDuration;
        float lengthRatio = (end - start) / mediaDuration;
        if (!lengthRatio)
            continue;

        IntRect rangeRect(r);
        rangeRect.setWidth(lengthRatio * totalTrackWidth);
        if (index)
            rangeRect.move(startRatio * totalTrackWidth, 0);
        context->fillRoundedRect(RoundedRect(rangeRect, borderRadiiFromStyle(style)), style->visitedDependentColor(CSSPropertyColor), style->colorSpace());
    }

    context->restore();
    return false;
}

bool RenderThemeGtk::paintMediaSliderThumb(RenderObject* o, const PaintInfo& paintInfo, const IntRect& r)
{
    RenderStyle* style = o->style();
    paintInfo.context->fillRoundedRect(RoundedRect(r, borderRadiiFromStyle(style)), style->visitedDependentColor(CSSPropertyColor), style->colorSpace());
    return false;
}

bool RenderThemeGtk::paintMediaVolumeSliderContainer(RenderObject*, const PaintInfo& paintInfo, const IntRect& rect)
{
    return true;
}

bool RenderThemeGtk::paintMediaVolumeSliderTrack(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    HTMLMediaElement* mediaElement = toParentMediaElement(renderObject);
    if (!mediaElement)
        return true;

    float volume = mediaElement->volume();
    if (!volume)
        return true;

    GraphicsContext* context = paintInfo.context;
    context->save();
    context->setStrokeStyle(NoStroke);

    int rectHeight = rect.height();
    float trackHeight = rectHeight * volume;
    RenderStyle* style = renderObject->style();
    IntRect volumeRect(rect);
    volumeRect.move(0, rectHeight - trackHeight);
    volumeRect.setHeight(ceil(trackHeight));

    context->fillRoundedRect(RoundedRect(volumeRect, borderRadiiFromStyle(style)),
        style->visitedDependentColor(CSSPropertyColor), style->colorSpace());
    context->restore();

    return false;
}

bool RenderThemeGtk::paintMediaVolumeSliderThumb(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    return paintMediaSliderThumb(renderObject, paintInfo, rect);
}

String RenderThemeGtk::formatMediaControlsCurrentTime(float currentTime, float duration) const
{
    return formatMediaControlsTime(currentTime) + " / " + formatMediaControlsTime(duration);
}

bool RenderThemeGtk::paintMediaCurrentTime(RenderObject* renderObject, const PaintInfo& paintInfo, const IntRect& rect)
{
    return false;
}
#endif

#if ENABLE(PROGRESS_ELEMENT)
void RenderThemeGtk::adjustProgressBarStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(nullptr);
}

// These values have been copied from RenderThemeChromiumSkia.cpp
static const int progressActivityBlocks = 5;
static const int progressAnimationFrames = 10;
static const double progressAnimationInterval = 0.125;
double RenderThemeGtk::animationRepeatIntervalForProgressBar(RenderProgress*) const
{
    return progressAnimationInterval;
}

double RenderThemeGtk::animationDurationForProgressBar(RenderProgress*) const
{
    return progressAnimationInterval * progressAnimationFrames * 2; // "2" for back and forth;
}

IntRect RenderThemeGtk::calculateProgressRect(RenderObject* renderObject, const IntRect& fullBarRect)
{
    IntRect progressRect(fullBarRect);
    RenderProgress* renderProgress = toRenderProgress(renderObject);
    if (renderProgress->isDeterminate()) {
        int progressWidth = progressRect.width() * renderProgress->position();
        if (renderObject->style()->direction() == RTL)
            progressRect.setX(progressRect.x() + progressRect.width() - progressWidth);
        progressRect.setWidth(progressWidth);
        return progressRect;
    }

    double animationProgress = renderProgress->animationProgress();

    // Never let the progress rect shrink smaller than 2 pixels.
    int newWidth = max(2, progressRect.width() / progressActivityBlocks);
    int movableWidth = progressRect.width() - newWidth;
    progressRect.setWidth(newWidth);

    // We want the first 0.5 units of the animation progress to represent the
    // forward motion and the second 0.5 units to represent the backward motion,
    // thus we multiply by two here to get the full sweep of the progress bar with
    // each direction.
    if (animationProgress < 0.5)
        progressRect.setX(progressRect.x() + (animationProgress * 2 * movableWidth));
    else
        progressRect.setX(progressRect.x() + ((1.0 - animationProgress) * 2 * movableWidth));
    return progressRect;
}
#endif

String RenderThemeGtk::fileListNameForWidth(const FileList* fileList, const Font& font, int width, bool multipleFilesAllowed) const
{
    if (width <= 0)
        return String();

    if (fileList->length() > 1)
        return StringTruncator::rightTruncate(multipleFileUploadText(fileList->length()), width, font, StringTruncator::EnableRoundingHacks);

    String string;
    if (fileList->length())
        string = pathGetFileName(fileList->item(0)->path());
    else if (multipleFilesAllowed)
        string = fileButtonNoFilesSelectedLabel();
    else
        string = fileButtonNoFileSelectedLabel();

    return StringTruncator::centerTruncate(string, width, font, StringTruncator::EnableRoundingHacks);
}

#if ENABLE(DATALIST_ELEMENT)
IntSize RenderThemeGtk::sliderTickSize() const
{
    // FIXME: We need to set this to the size of one tick mark.
    return IntSize(0, 0);
}

int RenderThemeGtk::sliderTickOffsetFromTrackCenter() const
{
    // FIXME: We need to set this to the position of the tick marks.
    return 0;
}
#endif

}

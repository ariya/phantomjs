/*
 * Copyright (C) 2004, 2005, 2006, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2006 James G. Speth (speth@end.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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

#import "config.h"

#import "CSSRule.h"
#import "CSSStyleSheet.h"
#import "CSSValue.h"
#import "DOMCSSCharsetRule.h"
#import "DOMCSSFontFaceRule.h"
#import "DOMCSSImportRule.h"
#import "DOMCSSMediaRule.h"
#import "DOMCSSPageRule.h"
#import "DOMCSSPrimitiveValue.h"
#import "DOMCSSRuleInternal.h"
#import "DOMCSSStyleDeclaration.h"
#import "DOMCSSStyleRule.h"
#import "DOMCSSStyleSheet.h"
#if ENABLE(CSS3_CONDITIONAL_RULES)
#import "DOMCSSSupportsRule.h"
#endif
#import "DOMCSSUnknownRule.h"
#import "DOMCSSValueInternal.h"
#import "DOMCSSValueList.h"
#import "DOMInternal.h"
#import "DOMStyleSheetInternal.h"
#import "DOMWebKitCSSKeyframeRule.h"
#import "DOMWebKitCSSKeyframesRule.h"
#import "DOMWebKitCSSTransformValue.h"

#if ENABLE(CSS_SHADERS)
#import "DOMWebKitCSSFilterRule.h"
#endif

#if ENABLE(CSS_FILTERS)
#import "DOMWebKitCSSFilterValue.h"
#endif

#if ENABLE(CSS_REGIONS)
#import "DOMWebKitCSSRegionRule.h"
#endif

#if ENABLE(CSS_DEVICE_ADAPTATION)
#import "DOMWebKitCSSViewportRule.h"
#endif

#if ENABLE(SHADOW_DOM)
#import "DOMCSSHostRule.h"
#endif

//------------------------------------------------------------------------------------------
// DOMStyleSheet

Class kitClass(WebCore::StyleSheet* impl)
{
    if (impl->isCSSStyleSheet())
        return [DOMCSSStyleSheet class];
    return [DOMStyleSheet class];
}

//------------------------------------------------------------------------------------------
// DOMCSSRule

Class kitClass(WebCore::CSSRule* impl)
{
    switch (impl->type()) {
        case DOM_UNKNOWN_RULE:
            return [DOMCSSUnknownRule class];
        case DOM_STYLE_RULE:
            return [DOMCSSStyleRule class];
        case DOM_CHARSET_RULE:
            return [DOMCSSCharsetRule class];
        case DOM_IMPORT_RULE:
            return [DOMCSSImportRule class];
        case DOM_MEDIA_RULE:
            return [DOMCSSMediaRule class];
        case DOM_FONT_FACE_RULE:
            return [DOMCSSFontFaceRule class];
        case DOM_PAGE_RULE:
            return [DOMCSSPageRule class];
        case DOM_WEBKIT_KEYFRAMES_RULE:
            return [DOMWebKitCSSKeyframesRule class];
        case DOM_WEBKIT_KEYFRAME_RULE:
            return [DOMWebKitCSSKeyframeRule class];
#if ENABLE(CSS3_CONDITIONAL_RULES)
        case DOM_SUPPORTS_RULE:
            return [DOMCSSSupportsRule class];
#endif
#if ENABLE(CSS_DEVICE_ADAPTATION)
        case DOM_WEBKIT_VIEWPORT_RULE:
            return [DOMWebKitCSSViewportRule class];
#endif
#if ENABLE(CSS_REGIONS)
        case DOM_WEBKIT_REGION_RULE:
            return [DOMWebKitCSSRegionRule class];
#endif
#if ENABLE(SHADOW_DOM)
        case DOM_HOST_RULE:
            return [DOMCSSHostRule class];
#endif
#if ENABLE(CSS_SHADERS)
        case DOM_WEBKIT_FILTER_RULE:
            return [DOMWebKitCSSFilterRule class];
#endif
    }
    ASSERT_NOT_REACHED();
    return nil;
}

//------------------------------------------------------------------------------------------
// DOMCSSValue

Class kitClass(WebCore::CSSValue* impl)
{
    switch (impl->cssValueType()) {
        case WebCore::CSSValue::CSS_PRIMITIVE_VALUE:
            return [DOMCSSPrimitiveValue class];
        case WebCore::CSSValue::CSS_VALUE_LIST:
            if (impl->isWebKitCSSTransformValue())
                return [DOMWebKitCSSTransformValue class];
#if ENABLE(CSS_FILTERS)
            if (impl->isWebKitCSSFilterValue())
                return [DOMWebKitCSSFilterValue class];
#endif
            return [DOMCSSValueList class];
        case WebCore::CSSValue::CSS_INHERIT:
        case WebCore::CSSValue::CSS_INITIAL:
            return [DOMCSSValue class];
        case WebCore::CSSValue::CSS_CUSTOM:
            return [DOMCSSValue class];
    }
    ASSERT_NOT_REACHED();
    return nil;
}

//------------------------------------------------------------------------------------------
// DOMCSSStyleDeclaration CSS2 Properties

@implementation DOMCSSStyleDeclaration (DOMCSS2Properties)

- (NSString *)azimuth
{
    return [self getPropertyValue:@"azimuth"];
}

- (void)setAzimuth:(NSString *)azimuth
{
    [self setProperty:@"azimuth" value:azimuth priority:@""];
}

- (NSString *)background
{
    return [self getPropertyValue:@"background"];
}

- (void)setBackground:(NSString *)background
{
    [self setProperty:@"background" value:background priority:@""];
}

- (NSString *)backgroundAttachment
{
    return [self getPropertyValue:@"background-attachment"];
}

- (void)setBackgroundAttachment:(NSString *)backgroundAttachment
{
    [self setProperty:@"background-attachment" value:backgroundAttachment priority:@""];
}

- (NSString *)backgroundColor
{
    return [self getPropertyValue:@"background-color"];
}

- (void)setBackgroundColor:(NSString *)backgroundColor
{
    [self setProperty:@"background-color" value:backgroundColor priority:@""];
}

- (NSString *)backgroundImage
{
    return [self getPropertyValue:@"background-image"];
}

- (void)setBackgroundImage:(NSString *)backgroundImage
{
    [self setProperty:@"background-image" value:backgroundImage priority:@""];
}

- (NSString *)backgroundPosition
{
    return [self getPropertyValue:@"background-position"];
}

- (void)setBackgroundPosition:(NSString *)backgroundPosition
{
    [self setProperty:@"background-position" value:backgroundPosition priority:@""];
}

- (NSString *)backgroundRepeat
{
    return [self getPropertyValue:@"background-repeat"];
}

- (void)setBackgroundRepeat:(NSString *)backgroundRepeat
{
    [self setProperty:@"background-repeat" value:backgroundRepeat priority:@""];
}

- (NSString *)border
{
    return [self getPropertyValue:@"border"];
}

- (void)setBorder:(NSString *)border
{
    [self setProperty:@"border" value:border priority:@""];
}

- (NSString *)borderCollapse
{
    return [self getPropertyValue:@"border-collapse"];
}

- (void)setBorderCollapse:(NSString *)borderCollapse
{
    [self setProperty:@"border-collapse" value:borderCollapse priority:@""];
}

- (NSString *)borderColor
{
    return [self getPropertyValue:@"border-color"];
}

- (void)setBorderColor:(NSString *)borderColor
{
    [self setProperty:@"border-color" value:borderColor priority:@""];
}

- (NSString *)borderSpacing
{
    return [self getPropertyValue:@"border-spacing"];
}

- (void)setBorderSpacing:(NSString *)borderSpacing
{
    [self setProperty:@"border-spacing" value:borderSpacing priority:@""];
}

- (NSString *)borderStyle
{
    return [self getPropertyValue:@"border-style"];
}

- (void)setBorderStyle:(NSString *)borderStyle
{
    [self setProperty:@"border-style" value:borderStyle priority:@""];
}

- (NSString *)borderTop
{
    return [self getPropertyValue:@"border-top"];
}

- (void)setBorderTop:(NSString *)borderTop
{
    [self setProperty:@"border-top" value:borderTop priority:@""];
}

- (NSString *)borderRight
{
    return [self getPropertyValue:@"border-right"];
}

- (void)setBorderRight:(NSString *)borderRight
{
    [self setProperty:@"border-right" value:borderRight priority:@""];
}

- (NSString *)borderBottom
{
    return [self getPropertyValue:@"border-bottom"];
}

- (void)setBorderBottom:(NSString *)borderBottom
{
    [self setProperty:@"border-bottom" value:borderBottom priority:@""];
}

- (NSString *)borderLeft
{
    return [self getPropertyValue:@"border-left"];
}

- (void)setBorderLeft:(NSString *)borderLeft
{
    [self setProperty:@"border-left" value:borderLeft priority:@""];
}

- (NSString *)borderTopColor
{
    return [self getPropertyValue:@"border-top-color"];
}

- (void)setBorderTopColor:(NSString *)borderTopColor
{
    [self setProperty:@"border-top-color" value:borderTopColor priority:@""];
}

- (NSString *)borderRightColor
{
    return [self getPropertyValue:@"border-right-color"];
}

- (void)setBorderRightColor:(NSString *)borderRightColor
{
    [self setProperty:@"border-right-color" value:borderRightColor priority:@""];
}

- (NSString *)borderBottomColor
{
    return [self getPropertyValue:@"border-bottom-color"];
}

- (void)setBorderBottomColor:(NSString *)borderBottomColor
{
    [self setProperty:@"border-bottom-color" value:borderBottomColor priority:@""];
}

- (NSString *)borderLeftColor
{
    return [self getPropertyValue:@"border-left-color"];
}

- (void)setBorderLeftColor:(NSString *)borderLeftColor
{
    [self setProperty:@"border-left-color" value:borderLeftColor priority:@""];
}

- (NSString *)borderTopStyle
{
    return [self getPropertyValue:@"border-top-style"];
}

- (void)setBorderTopStyle:(NSString *)borderTopStyle
{
    [self setProperty:@"border-top-style" value:borderTopStyle priority:@""];
}

- (NSString *)borderRightStyle
{
    return [self getPropertyValue:@"border-right-style"];
}

- (void)setBorderRightStyle:(NSString *)borderRightStyle
{
    [self setProperty:@"border-right-style" value:borderRightStyle priority:@""];
}

- (NSString *)borderBottomStyle
{
    return [self getPropertyValue:@"border-bottom-style"];
}

- (void)setBorderBottomStyle:(NSString *)borderBottomStyle
{
    [self setProperty:@"border-bottom-style" value:borderBottomStyle priority:@""];
}

- (NSString *)borderLeftStyle
{
    return [self getPropertyValue:@"border-left-style"];
}

- (void)setBorderLeftStyle:(NSString *)borderLeftStyle
{
    [self setProperty:@"border-left-style" value:borderLeftStyle priority:@""];
}

- (NSString *)borderTopWidth
{
    return [self getPropertyValue:@"border-top-width"];
}

- (void)setBorderTopWidth:(NSString *)borderTopWidth
{
    [self setProperty:@"border-top-width" value:borderTopWidth priority:@""];
}

- (NSString *)borderRightWidth
{
    return [self getPropertyValue:@"border-right-width"];
}

- (void)setBorderRightWidth:(NSString *)borderRightWidth
{
    [self setProperty:@"border-right-width" value:borderRightWidth priority:@""];
}

- (NSString *)borderBottomWidth
{
    return [self getPropertyValue:@"border-bottom-width"];
}

- (void)setBorderBottomWidth:(NSString *)borderBottomWidth
{
    [self setProperty:@"border-bottom-width" value:borderBottomWidth priority:@""];
}

- (NSString *)borderLeftWidth
{
    return [self getPropertyValue:@"border-left-width"];
}

- (void)setBorderLeftWidth:(NSString *)borderLeftWidth
{
    [self setProperty:@"border-left-width" value:borderLeftWidth priority:@""];
}

- (NSString *)borderWidth
{
    return [self getPropertyValue:@"border-width"];
}

- (void)setBorderWidth:(NSString *)borderWidth
{
    [self setProperty:@"border-width" value:borderWidth priority:@""];
}

- (NSString *)bottom
{
    return [self getPropertyValue:@"bottom"];
}

- (void)setBottom:(NSString *)bottom
{
    [self setProperty:@"bottom" value:bottom priority:@""];
}

- (NSString *)captionSide
{
    return [self getPropertyValue:@"caption-side"];
}

- (void)setCaptionSide:(NSString *)captionSide
{
    [self setProperty:@"caption-side" value:captionSide priority:@""];
}

- (NSString *)clear
{
    return [self getPropertyValue:@"clear"];
}

- (void)setClear:(NSString *)clear
{
    [self setProperty:@"clear" value:clear priority:@""];
}

- (NSString *)clip
{
    return [self getPropertyValue:@"clip"];
}

- (void)setClip:(NSString *)clip
{
    [self setProperty:@"clip" value:clip priority:@""];
}

- (NSString *)color
{
    return [self getPropertyValue:@"color"];
}

- (void)setColor:(NSString *)color
{
    [self setProperty:@"color" value:color priority:@""];
}

- (NSString *)content
{
    return [self getPropertyValue:@"content"];
}

- (void)setContent:(NSString *)content
{
    [self setProperty:@"content" value:content priority:@""];
}

- (NSString *)counterIncrement
{
    return [self getPropertyValue:@"counter-increment"];
}

- (void)setCounterIncrement:(NSString *)counterIncrement
{
    [self setProperty:@"counter-increment" value:counterIncrement priority:@""];
}

- (NSString *)counterReset
{
    return [self getPropertyValue:@"counter-reset"];
}

- (void)setCounterReset:(NSString *)counterReset
{
    [self setProperty:@"counter-reset" value:counterReset priority:@""];
}

- (NSString *)cue
{
    return [self getPropertyValue:@"cue"];
}

- (void)setCue:(NSString *)cue
{
    [self setProperty:@"cue" value:cue priority:@""];
}

- (NSString *)cueAfter
{
    return [self getPropertyValue:@"cue-after"];
}

- (void)setCueAfter:(NSString *)cueAfter
{
    [self setProperty:@"cue-after" value:cueAfter priority:@""];
}

- (NSString *)cueBefore
{
    return [self getPropertyValue:@"cue-before"];
}

- (void)setCueBefore:(NSString *)cueBefore
{
    [self setProperty:@"cue-before" value:cueBefore priority:@""];
}

- (NSString *)cursor
{
    return [self getPropertyValue:@"cursor"];
}

- (void)setCursor:(NSString *)cursor
{
    [self setProperty:@"cursor" value:cursor priority:@""];
}

- (NSString *)direction
{
    return [self getPropertyValue:@"direction"];
}

- (void)setDirection:(NSString *)direction
{
    [self setProperty:@"direction" value:direction priority:@""];
}

- (NSString *)display
{
    return [self getPropertyValue:@"display"];
}

- (void)setDisplay:(NSString *)display
{
    [self setProperty:@"display" value:display priority:@""];
}

- (NSString *)elevation
{
    return [self getPropertyValue:@"elevation"];
}

- (void)setElevation:(NSString *)elevation
{
    [self setProperty:@"elevation" value:elevation priority:@""];
}

- (NSString *)emptyCells
{
    return [self getPropertyValue:@"empty-cells"];
}

- (void)setEmptyCells:(NSString *)emptyCells
{
    [self setProperty:@"empty-cells" value:emptyCells priority:@""];
}

- (NSString *)cssFloat
{
    return [self getPropertyValue:@"css-float"];
}

- (void)setCssFloat:(NSString *)cssFloat
{
    [self setProperty:@"css-float" value:cssFloat priority:@""];
}

- (NSString *)font
{
    return [self getPropertyValue:@"font"];
}

- (void)setFont:(NSString *)font
{
    [self setProperty:@"font" value:font priority:@""];
}

- (NSString *)fontFamily
{
    return [self getPropertyValue:@"font-family"];
}

- (void)setFontFamily:(NSString *)fontFamily
{
    [self setProperty:@"font-family" value:fontFamily priority:@""];
}

- (NSString *)fontSize
{
    return [self getPropertyValue:@"font-size"];
}

- (void)setFontSize:(NSString *)fontSize
{
    [self setProperty:@"font-size" value:fontSize priority:@""];
}

- (NSString *)fontSizeAdjust
{
    return [self getPropertyValue:@"font-size-adjust"];
}

- (void)setFontSizeAdjust:(NSString *)fontSizeAdjust
{
    [self setProperty:@"font-size-adjust" value:fontSizeAdjust priority:@""];
}

- (NSString *)_fontSizeDelta
{
    return [self getPropertyValue:@"-webkit-font-size-delta"];
}

- (void)_setFontSizeDelta:(NSString *)fontSizeDelta
{
    [self setProperty:@"-webkit-font-size-delta" value:fontSizeDelta priority:@""];
}

- (NSString *)fontStretch
{
    return [self getPropertyValue:@"font-stretch"];
}

- (void)setFontStretch:(NSString *)fontStretch
{
    [self setProperty:@"font-stretch" value:fontStretch priority:@""];
}

- (NSString *)fontStyle
{
    return [self getPropertyValue:@"font-style"];
}

- (void)setFontStyle:(NSString *)fontStyle
{
    [self setProperty:@"font-style" value:fontStyle priority:@""];
}

- (NSString *)fontVariant
{
    return [self getPropertyValue:@"font-variant"];
}

- (void)setFontVariant:(NSString *)fontVariant
{
    [self setProperty:@"font-variant" value:fontVariant priority:@""];
}

- (NSString *)fontWeight
{
    return [self getPropertyValue:@"font-weight"];
}

- (void)setFontWeight:(NSString *)fontWeight
{
    [self setProperty:@"font-weight" value:fontWeight priority:@""];
}

- (NSString *)height
{
    return [self getPropertyValue:@"height"];
}

- (void)setHeight:(NSString *)height
{
    [self setProperty:@"height" value:height priority:@""];
}

- (NSString *)left
{
    return [self getPropertyValue:@"left"];
}

- (void)setLeft:(NSString *)left
{
    [self setProperty:@"left" value:left priority:@""];
}

- (NSString *)letterSpacing
{
    return [self getPropertyValue:@"letter-spacing"];
}

- (void)setLetterSpacing:(NSString *)letterSpacing
{
    [self setProperty:@"letter-spacing" value:letterSpacing priority:@""];
}

- (NSString *)lineHeight
{
    return [self getPropertyValue:@"line-height"];
}

- (void)setLineHeight:(NSString *)lineHeight
{
    [self setProperty:@"line-height" value:lineHeight priority:@""];
}

- (NSString *)listStyle
{
    return [self getPropertyValue:@"list-style"];
}

- (void)setListStyle:(NSString *)listStyle
{
    [self setProperty:@"list-style" value:listStyle priority:@""];
}

- (NSString *)listStyleImage
{
    return [self getPropertyValue:@"list-style-image"];
}

- (void)setListStyleImage:(NSString *)listStyleImage
{
    [self setProperty:@"list-style-image" value:listStyleImage priority:@""];
}

- (NSString *)listStylePosition
{
    return [self getPropertyValue:@"list-style-position"];
}

- (void)setListStylePosition:(NSString *)listStylePosition
{
    [self setProperty:@"list-style-position" value:listStylePosition priority:@""];
}

- (NSString *)listStyleType
{
    return [self getPropertyValue:@"list-style-type"];
}

- (void)setListStyleType:(NSString *)listStyleType
{
    [self setProperty:@"list-style-type" value:listStyleType priority:@""];
}

- (NSString *)margin
{
    return [self getPropertyValue:@"margin"];
}

- (void)setMargin:(NSString *)margin
{
    [self setProperty:@"margin" value:margin priority:@""];
}

- (NSString *)marginTop
{
    return [self getPropertyValue:@"margin-top"];
}

- (void)setMarginTop:(NSString *)marginTop
{
    [self setProperty:@"margin-top" value:marginTop priority:@""];
}

- (NSString *)marginRight
{
    return [self getPropertyValue:@"margin-right"];
}

- (void)setMarginRight:(NSString *)marginRight
{
    [self setProperty:@"margin-right" value:marginRight priority:@""];
}

- (NSString *)marginBottom
{
    return [self getPropertyValue:@"margin-bottom"];
}

- (void)setMarginBottom:(NSString *)marginBottom
{
    [self setProperty:@"margin-bottom" value:marginBottom priority:@""];
}

- (NSString *)marginLeft
{
    return [self getPropertyValue:@"margin-left"];
}

- (void)setMarginLeft:(NSString *)marginLeft
{
    [self setProperty:@"margin-left" value:marginLeft priority:@""];
}

- (NSString *)markerOffset
{
    return [self getPropertyValue:@"marker-offset"];
}

- (void)setMarkerOffset:(NSString *)markerOffset
{
    [self setProperty:@"marker-offset" value:markerOffset priority:@""];
}

- (NSString *)marks
{
    return [self getPropertyValue:@"marks"];
}

- (void)setMarks:(NSString *)marks
{
    [self setProperty:@"marks" value:marks priority:@""];
}

- (NSString *)maxHeight
{
    return [self getPropertyValue:@"max-height"];
}

- (void)setMaxHeight:(NSString *)maxHeight
{
    [self setProperty:@"max-height" value:maxHeight priority:@""];
}

- (NSString *)maxWidth
{
    return [self getPropertyValue:@"max-width"];
}

- (void)setMaxWidth:(NSString *)maxWidth
{
    [self setProperty:@"max-width" value:maxWidth priority:@""];
}

- (NSString *)minHeight
{
    return [self getPropertyValue:@"min-height"];
}

- (void)setMinHeight:(NSString *)minHeight
{
    [self setProperty:@"min-height" value:minHeight priority:@""];
}

- (NSString *)minWidth
{
    return [self getPropertyValue:@"min-width"];
}

- (void)setMinWidth:(NSString *)minWidth
{
    [self setProperty:@"min-width" value:minWidth priority:@""];
}

- (NSString *)orphans
{
    return [self getPropertyValue:@"orphans"];
}

- (void)setOrphans:(NSString *)orphans
{
    [self setProperty:@"orphans" value:orphans priority:@""];
}

- (NSString *)outline
{
    return [self getPropertyValue:@"outline"];
}

- (void)setOutline:(NSString *)outline
{
    [self setProperty:@"outline" value:outline priority:@""];
}

- (NSString *)outlineColor
{
    return [self getPropertyValue:@"outline-color"];
}

- (void)setOutlineColor:(NSString *)outlineColor
{
    [self setProperty:@"outline-color" value:outlineColor priority:@""];
}

- (NSString *)outlineStyle
{
    return [self getPropertyValue:@"outline-style"];
}

- (void)setOutlineStyle:(NSString *)outlineStyle
{
    [self setProperty:@"outline-style" value:outlineStyle priority:@""];
}

- (NSString *)outlineWidth
{
    return [self getPropertyValue:@"outline-width"];
}

- (void)setOutlineWidth:(NSString *)outlineWidth
{
    [self setProperty:@"outline-width" value:outlineWidth priority:@""];
}

- (NSString *)overflow
{
    return [self getPropertyValue:@"overflow"];
}

- (void)setOverflow:(NSString *)overflow
{
    [self setProperty:@"overflow" value:overflow priority:@""];
}

- (NSString *)padding
{
    return [self getPropertyValue:@"padding"];
}

- (void)setPadding:(NSString *)padding
{
    [self setProperty:@"padding" value:padding priority:@""];
}

- (NSString *)paddingTop
{
    return [self getPropertyValue:@"padding-top"];
}

- (void)setPaddingTop:(NSString *)paddingTop
{
    [self setProperty:@"padding-top" value:paddingTop priority:@""];
}

- (NSString *)paddingRight
{
    return [self getPropertyValue:@"padding-right"];
}

- (void)setPaddingRight:(NSString *)paddingRight
{
    [self setProperty:@"padding-right" value:paddingRight priority:@""];
}

- (NSString *)paddingBottom
{
    return [self getPropertyValue:@"padding-bottom"];
}

- (void)setPaddingBottom:(NSString *)paddingBottom
{
    [self setProperty:@"padding-bottom" value:paddingBottom priority:@""];
}

- (NSString *)paddingLeft
{
    return [self getPropertyValue:@"padding-left"];
}

- (void)setPaddingLeft:(NSString *)paddingLeft
{
    [self setProperty:@"padding-left" value:paddingLeft priority:@""];
}

- (NSString *)page
{
    return [self getPropertyValue:@"page"];
}

- (void)setPage:(NSString *)page
{
    [self setProperty:@"page" value:page priority:@""];
}

- (NSString *)pageBreakAfter
{
    return [self getPropertyValue:@"page-break-after"];
}

- (void)setPageBreakAfter:(NSString *)pageBreakAfter
{
    [self setProperty:@"page-break-after" value:pageBreakAfter priority:@""];
}

- (NSString *)pageBreakBefore
{
    return [self getPropertyValue:@"page-break-before"];
}

- (void)setPageBreakBefore:(NSString *)pageBreakBefore
{
    [self setProperty:@"page-break-before" value:pageBreakBefore priority:@""];
}

- (NSString *)pageBreakInside
{
    return [self getPropertyValue:@"page-break-inside"];
}

- (void)setPageBreakInside:(NSString *)pageBreakInside
{
    [self setProperty:@"page-break-inside" value:pageBreakInside priority:@""];
}

- (NSString *)pause
{
    return [self getPropertyValue:@"pause"];
}

- (void)setPause:(NSString *)pause
{
    [self setProperty:@"pause" value:pause priority:@""];
}

- (NSString *)pauseAfter
{
    return [self getPropertyValue:@"pause-after"];
}

- (void)setPauseAfter:(NSString *)pauseAfter
{
    [self setProperty:@"pause-after" value:pauseAfter priority:@""];
}

- (NSString *)pauseBefore
{
    return [self getPropertyValue:@"pause-before"];
}

- (void)setPauseBefore:(NSString *)pauseBefore
{
    [self setProperty:@"pause-before" value:pauseBefore priority:@""];
}

- (NSString *)pitch
{
    return [self getPropertyValue:@"pitch"];
}

- (void)setPitch:(NSString *)pitch
{
    [self setProperty:@"pitch" value:pitch priority:@""];
}

- (NSString *)pitchRange
{
    return [self getPropertyValue:@"pitch-range"];
}

- (void)setPitchRange:(NSString *)pitchRange
{
    [self setProperty:@"pitch-range" value:pitchRange priority:@""];
}

- (NSString *)playDuring
{
    return [self getPropertyValue:@"play-during"];
}

- (void)setPlayDuring:(NSString *)playDuring
{
    [self setProperty:@"play-during" value:playDuring priority:@""];
}

- (NSString *)position
{
    return [self getPropertyValue:@"position"];
}

- (void)setPosition:(NSString *)position
{
    [self setProperty:@"position" value:position priority:@""];
}

- (NSString *)quotes
{
    return [self getPropertyValue:@"quotes"];
}

- (void)setQuotes:(NSString *)quotes
{
    [self setProperty:@"quotes" value:quotes priority:@""];
}

- (NSString *)richness
{
    return [self getPropertyValue:@"richness"];
}

- (void)setRichness:(NSString *)richness
{
    [self setProperty:@"richness" value:richness priority:@""];
}

- (NSString *)right
{
    return [self getPropertyValue:@"right"];
}

- (void)setRight:(NSString *)right
{
    [self setProperty:@"right" value:right priority:@""];
}

- (NSString *)size
{
    return [self getPropertyValue:@"size"];
}

- (void)setSize:(NSString *)size
{
    [self setProperty:@"size" value:size priority:@""];
}

- (NSString *)speak
{
    return [self getPropertyValue:@"speak"];
}

- (void)setSpeak:(NSString *)speak
{
    [self setProperty:@"speak" value:speak priority:@""];
}

- (NSString *)speakHeader
{
    return [self getPropertyValue:@"speak-header"];
}

- (void)setSpeakHeader:(NSString *)speakHeader
{
    [self setProperty:@"speak-header" value:speakHeader priority:@""];
}

- (NSString *)speakNumeral
{
    return [self getPropertyValue:@"speak-numeral"];
}

- (void)setSpeakNumeral:(NSString *)speakNumeral
{
    [self setProperty:@"speak-numeral" value:speakNumeral priority:@""];
}

- (NSString *)speakPunctuation
{
    return [self getPropertyValue:@"speak-punctuation"];
}

- (void)setSpeakPunctuation:(NSString *)speakPunctuation
{
    [self setProperty:@"speak-punctuation" value:speakPunctuation priority:@""];
}

- (NSString *)speechRate
{
    return [self getPropertyValue:@"speech-rate"];
}

- (void)setSpeechRate:(NSString *)speechRate
{
    [self setProperty:@"speech-rate" value:speechRate priority:@""];
}

- (NSString *)stress
{
    return [self getPropertyValue:@"stress"];
}

- (void)setStress:(NSString *)stress
{
    [self setProperty:@"stress" value:stress priority:@""];
}

- (NSString *)tableLayout
{
    return [self getPropertyValue:@"table-layout"];
}

- (void)setTableLayout:(NSString *)tableLayout
{
    [self setProperty:@"table-layout" value:tableLayout priority:@""];
}

- (NSString *)textAlign
{
    return [self getPropertyValue:@"text-align"];
}

- (void)setTextAlign:(NSString *)textAlign
{
    [self setProperty:@"text-align" value:textAlign priority:@""];
}

- (NSString *)textDecoration
{
    return [self getPropertyValue:@"text-decoration"];
}

- (void)setTextDecoration:(NSString *)textDecoration
{
    [self setProperty:@"text-decoration" value:textDecoration priority:@""];
}

- (NSString *)textIndent
{
    return [self getPropertyValue:@"text-indent"];
}

- (void)setTextIndent:(NSString *)textIndent
{
    [self setProperty:@"text-indent" value:textIndent priority:@""];
}

- (NSString *)textShadow
{
    return [self getPropertyValue:@"text-shadow"];
}

- (void)setTextShadow:(NSString *)textShadow
{
    [self setProperty:@"text-shadow" value:textShadow priority:@""];
}

- (NSString *)textTransform
{
    return [self getPropertyValue:@"text-transform"];
}

- (void)setTextTransform:(NSString *)textTransform
{
    [self setProperty:@"text-transform" value:textTransform priority:@""];
}

- (NSString *)top
{
    return [self getPropertyValue:@"top"];
}

- (void)setTop:(NSString *)top
{
    [self setProperty:@"top" value:top priority:@""];
}

- (NSString *)unicodeBidi
{
    return [self getPropertyValue:@"unicode-bidi"];
}

- (void)setUnicodeBidi:(NSString *)unicodeBidi
{
    [self setProperty:@"unicode-bidi" value:unicodeBidi priority:@""];
}

- (NSString *)verticalAlign
{
    return [self getPropertyValue:@"vertical-align"];
}

- (void)setVerticalAlign:(NSString *)verticalAlign
{
    [self setProperty:@"vertical-align" value:verticalAlign priority:@""];
}

- (NSString *)visibility
{
    return [self getPropertyValue:@"visibility"];
}

- (void)setVisibility:(NSString *)visibility
{
    [self setProperty:@"visibility" value:visibility priority:@""];
}

- (NSString *)voiceFamily
{
    return [self getPropertyValue:@"voice-family"];
}

- (void)setVoiceFamily:(NSString *)voiceFamily
{
    [self setProperty:@"voice-family" value:voiceFamily priority:@""];
}

- (NSString *)volume
{
    return [self getPropertyValue:@"volume"];
}

- (void)setVolume:(NSString *)volume
{
    [self setProperty:@"volume" value:volume priority:@""];
}

- (NSString *)whiteSpace
{
    return [self getPropertyValue:@"white-space"];
}

- (void)setWhiteSpace:(NSString *)whiteSpace
{
    [self setProperty:@"white-space" value:whiteSpace priority:@""];
}

- (NSString *)widows
{
    return [self getPropertyValue:@"widows"];
}

- (void)setWidows:(NSString *)widows
{
    [self setProperty:@"widows" value:widows priority:@""];
}

- (NSString *)width
{
    return [self getPropertyValue:@"width"];
}

- (void)setWidth:(NSString *)width
{
    [self setProperty:@"width" value:width priority:@""];
}

- (NSString *)wordSpacing
{
    return [self getPropertyValue:@"word-spacing"];
}

- (void)setWordSpacing:(NSString *)wordSpacing
{
    [self setProperty:@"word-spacing" value:wordSpacing priority:@""];
}

- (NSString *)zIndex
{
    return [self getPropertyValue:@"z-index"];
}

- (void)setZIndex:(NSString *)zIndex
{
    [self setProperty:@"z-index" value:zIndex priority:@""];
}

@end

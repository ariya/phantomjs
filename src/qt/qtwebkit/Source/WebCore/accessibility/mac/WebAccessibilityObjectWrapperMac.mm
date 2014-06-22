/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "WebAccessibilityObjectWrapperMac.h"

#if HAVE(ACCESSIBILITY)

#import "AXObjectCache.h"
#import "AccessibilityARIAGridRow.h"
#import "AccessibilityList.h"
#import "AccessibilityListBox.h"
#import "AccessibilityRenderObject.h"
#import "AccessibilityScrollView.h"
#import "AccessibilitySpinButton.h"
#import "AccessibilityTable.h"
#import "AccessibilityTableCell.h"
#import "AccessibilityTableColumn.h"
#import "AccessibilityTableRow.h"
#import "Chrome.h"
#import "ChromeClient.h"
#import "ColorMac.h"
#import "ContextMenuController.h"
#import "Editor.h"
#import "Font.h"
#import "Frame.h"
#import "FrameLoaderClient.h"
#import "FrameSelection.h"
#import "HTMLAnchorElement.h"
#import "HTMLAreaElement.h"
#import "HTMLFrameOwnerElement.h"
#import "HTMLImageElement.h"
#import "HTMLInputElement.h"
#import "HTMLNames.h"
#import "HTMLTextAreaElement.h"
#import "LocalizedStrings.h"
#import "Page.h"
#import "RenderTextControl.h"
#import "RenderView.h"
#import "RenderWidget.h"
#import "ScrollView.h"
#import "SimpleFontData.h"
#import "TextCheckerClient.h"
#import "TextCheckingHelper.h"
#import "TextIterator.h"
#import "VisibleUnits.h"
#import "WebCoreFrameView.h"
#import "WebCoreObjCExtras.h"
#import "WebCoreSystemInterface.h"
#import "htmlediting.h"

using namespace WebCore;
using namespace HTMLNames;
using namespace std;

// Cell Tables
#ifndef NSAccessibilitySelectedCellsAttribute
#define NSAccessibilitySelectedCellsAttribute @"AXSelectedCells"
#endif

#ifndef NSAccessibilityVisibleCellsAttribute
#define NSAccessibilityVisibleCellsAttribute @"AXVisibleCells"
#endif

#ifndef NSAccessibilityRowHeaderUIElementsAttribute
#define NSAccessibilityRowHeaderUIElementsAttribute @"AXRowHeaderUIElements"
#endif

#ifndef NSAccessibilityRowIndexRangeAttribute
#define NSAccessibilityRowIndexRangeAttribute @"AXRowIndexRange"
#endif

#ifndef NSAccessibilityColumnIndexRangeAttribute
#define NSAccessibilityColumnIndexRangeAttribute @"AXColumnIndexRange"
#endif

#ifndef NSAccessibilityCellForColumnAndRowParameterizedAttribute
#define NSAccessibilityCellForColumnAndRowParameterizedAttribute @"AXCellForColumnAndRow"
#endif

#ifndef NSAccessibilityCellRole
#define NSAccessibilityCellRole @"AXCell"
#endif

// Lists
#ifndef NSAccessibilityContentListSubrole
#define NSAccessibilityContentListSubrole @"AXContentList"
#endif

#ifndef NSAccessibilityDefinitionListSubrole
#define NSAccessibilityDefinitionListSubrole @"AXDefinitionList"
#endif

#ifndef NSAccessibilityDescriptionListSubrole
#define NSAccessibilityDescriptionListSubrole @"AXDescriptionList"
#endif

// Miscellaneous
#ifndef NSAccessibilityBlockQuoteLevelAttribute
#define NSAccessibilityBlockQuoteLevelAttribute @"AXBlockQuoteLevel"
#endif

#ifndef NSAccessibilityAccessKeyAttribute
#define NSAccessibilityAccessKeyAttribute @"AXAccessKey"
#endif

#ifndef NSAccessibilityLanguageAttribute
#define NSAccessibilityLanguageAttribute @"AXLanguage"
#endif

#ifndef NSAccessibilityRequiredAttribute
#define NSAccessibilityRequiredAttribute @"AXRequired"
#endif

#ifndef NSAccessibilityInvalidAttribute
#define NSAccessibilityInvalidAttribute @"AXInvalid"
#endif

#ifndef NSAccessibilityOwnsAttribute
#define NSAccessibilityOwnsAttribute @"AXOwns"
#endif

#ifndef NSAccessibilityGrabbedAttribute
#define NSAccessibilityGrabbedAttribute @"AXGrabbed"
#endif

#ifndef NSAccessibilityDropEffectsAttribute
#define NSAccessibilityDropEffectsAttribute @"AXDropEffects"
#endif

#ifndef NSAccessibilityARIALiveAttribute
#define NSAccessibilityARIALiveAttribute @"AXARIALive"
#endif

#ifndef NSAccessibilityARIAAtomicAttribute
#define NSAccessibilityARIAAtomicAttribute @"AXARIAAtomic"
#endif

#ifndef NSAccessibilityARIARelevantAttribute
#define NSAccessibilityARIARelevantAttribute @"AXARIARelevant"
#endif

#ifndef NSAccessibilityARIABusyAttribute
#define NSAccessibilityARIABusyAttribute @"AXARIABusy"
#endif

#ifndef NSAccessibilityARIAPosInSetAttribute
#define NSAccessibilityARIAPosInSetAttribute @"AXARIAPosInSet"
#endif

#ifndef NSAccessibilityARIASetSizeAttribute
#define NSAccessibilityARIASetSizeAttribute @"AXARIASetSize"
#endif

#ifndef NSAccessibilityLoadingProgressAttribute
#define NSAccessibilityLoadingProgressAttribute @"AXLoadingProgress"
#endif

#ifndef NSAccessibilityHasPopupAttribute
#define NSAccessibilityHasPopupAttribute @"AXHasPopup"
#endif

#ifndef NSAccessibilityPlaceholderValueAttribute
#define NSAccessibilityPlaceholderValueAttribute @"AXPlaceholderValue"
#endif

// Search
#ifndef NSAccessibilityUIElementsForSearchPredicateParameterizedAttribute
#define NSAccessibilityUIElementsForSearchPredicateParameterizedAttribute @"AXUIElementsForSearchPredicate"
#endif

// Search Keys
#ifndef NSAccessibilityAnyTypeSearchKey
#define NSAccessibilityAnyTypeSearchKey @"AXAnyTypeSearchKey"
#endif

#ifndef NSAccessibilityBlockquoteSameLevelSearchKey
#define NSAccessibilityBlockquoteSameLevelSearchKey @"AXBlockquoteSameLevelSearchKey"
#endif

#ifndef NSAccessibilityBlockquoteSearchKey
#define NSAccessibilityBlockquoteSearchKey @"AXBlockquoteSearchKey"
#endif

#ifndef NSAccessibilityBoldFontSearchKey
#define NSAccessibilityBoldFontSearchKey @"AXBoldFontSearchKey"
#endif

#ifndef NSAccessibilityButtonSearchKey
#define NSAccessibilityButtonSearchKey @"AXButtonSearchKey"
#endif

#ifndef NSAccessibilityCheckBoxSearchKey
#define NSAccessibilityCheckBoxSearchKey @"AXCheckBoxSearchKey"
#endif

#ifndef NSAccessibilityControlSearchKey
#define NSAccessibilityControlSearchKey @"AXControlSearchKey"
#endif

#ifndef NSAccessibilityDifferentTypeSearchKey
#define NSAccessibilityDifferentTypeSearchKey @"AXDifferentTypeSearchKey"
#endif

#ifndef NSAccessibilityFontChangeSearchKey
#define NSAccessibilityFontChangeSearchKey @"AXFontChangeSearchKey"
#endif

#ifndef NSAccessibilityFontColorChangeSearchKey
#define NSAccessibilityFontColorChangeSearchKey @"AXFontColorChangeSearchKey"
#endif

#ifndef NSAccessibilityFrameSearchKey
#define NSAccessibilityFrameSearchKey @"AXFrameSearchKey"
#endif

#ifndef NSAccessibilityGraphicSearchKey
#define NSAccessibilityGraphicSearchKey @"AXGraphicSearchKey"
#endif

#ifndef NSAccessibilityHeadingLevel1SearchKey
#define NSAccessibilityHeadingLevel1SearchKey @"AXHeadingLevel1SearchKey"
#endif

#ifndef NSAccessibilityHeadingLevel2SearchKey
#define NSAccessibilityHeadingLevel2SearchKey @"AXHeadingLevel2SearchKey"
#endif

#ifndef NSAccessibilityHeadingLevel3SearchKey
#define NSAccessibilityHeadingLevel3SearchKey @"AXHeadingLevel3SearchKey"
#endif

#ifndef NSAccessibilityHeadingLevel4SearchKey
#define NSAccessibilityHeadingLevel4SearchKey @"AXHeadingLevel4SearchKey"
#endif

#ifndef NSAccessibilityHeadingLevel5SearchKey
#define NSAccessibilityHeadingLevel5SearchKey @"AXHeadingLevel5SearchKey"
#endif

#ifndef NSAccessibilityHeadingLevel6SearchKey
#define NSAccessibilityHeadingLevel6SearchKey @"AXHeadingLevel6SearchKey"
#endif

#ifndef NSAccessibilityHeadingSameLevelSearchKey
#define NSAccessibilityHeadingSameLevelSearchKey @"AXHeadingSameLevelSearchKey"
#endif

#ifndef NSAccessibilityHeadingSearchKey
#define NSAccessibilityHeadingSearchKey @"AXHeadingSearchKey"
#endif

#ifndef NSAccessibilityHighlightedSearchKey
#define NSAccessibilityHighlightedSearchKey @"AXHighlightedSearchKey"
#endif

#ifndef NSAccessibilityItalicFontSearchKey
#define NSAccessibilityItalicFontSearchKey @"AXItalicFontSearchKey"
#endif

#ifndef NSAccessibilityLandmarkSearchKey
#define NSAccessibilityLandmarkSearchKey @"AXLandmarkSearchKey"
#endif

#ifndef NSAccessibilityLinkSearchKey
#define NSAccessibilityLinkSearchKey @"AXLinkSearchKey"
#endif

#ifndef NSAccessibilityListSearchKey
#define NSAccessibilityListSearchKey @"AXListSearchKey"
#endif

#ifndef NSAccessibilityLiveRegionSearchKey
#define NSAccessibilityLiveRegionSearchKey @"AXLiveRegionSearchKey"
#endif

#ifndef NSAccessibilityMisspelledWordSearchKey
#define NSAccessibilityMisspelledWordSearchKey @"AXMisspelledWordSearchKey"
#endif

#ifndef NSAccessibilityPlainTextSearchKey
#define NSAccessibilityPlainTextSearchKey @"AXPlainTextSearchKey"
#endif

#ifndef NSAccessibilityRadioGroupSearchKey
#define NSAccessibilityRadioGroupSearchKey @"AXRadioGroupSearchKey"
#endif

#ifndef NSAccessibilitySameTypeSearchKey
#define NSAccessibilitySameTypeSearchKey @"AXSameTypeSearchKey"
#endif

#ifndef NSAccessibilityStaticTextSearchKey
#define NSAccessibilityStaticTextSearchKey @"AXStaticTextSearchKey"
#endif

#ifndef NSAccessibilityStyleChangeSearchKey
#define NSAccessibilityStyleChangeSearchKey @"AXStyleChangeSearchKey"
#endif

#ifndef NSAccessibilityTableSameLevelSearchKey
#define NSAccessibilityTableSameLevelSearchKey @"AXTableSameLevelSearchKey"
#endif

#ifndef NSAccessibilityTableSearchKey
#define NSAccessibilityTableSearchKey @"AXTableSearchKey"
#endif

#ifndef NSAccessibilityTextFieldSearchKey
#define NSAccessibilityTextFieldSearchKey @"AXTextFieldSearchKey"
#endif

#ifndef NSAccessibilityUnderlineSearchKey
#define NSAccessibilityUnderlineSearchKey @"AXUnderlineSearchKey"
#endif

#ifndef NSAccessibilityUnvisitedLinkSearchKey
#define NSAccessibilityUnvisitedLinkSearchKey @"AXUnvisitedLinkSearchKey"
#endif

#ifndef NSAccessibilityVisitedLinkSearchKey
#define NSAccessibilityVisitedLinkSearchKey @"AXVisitedLinkSearchKey"
#endif

#define NSAccessibilityTextMarkerIsValidParameterizedAttribute @"AXTextMarkerIsValid"
#define NSAccessibilityIndexForTextMarkerParameterizedAttribute @"AXIndexForTextMarker"
#define NSAccessibilityTextMarkerForIndexParameterizedAttribute @"AXTextMarkerForIndex"

#ifndef NSAccessibilityScrollToVisibleAction
#define NSAccessibilityScrollToVisibleAction @"AXScrollToVisible"
#endif

#ifndef NSAccessibilityPathAttribute
#define NSAccessibilityPathAttribute @"AXPath"
#endif

// Math attributes
#define NSAccessibilityMathRootRadicandAttribute @"AXMathRootRadicand"
#define NSAccessibilityMathRootIndexAttribute @"AXMathRootIndex"
#define NSAccessibilityMathFractionDenominatorAttribute @"AXMathFractionDenominator"
#define NSAccessibilityMathFractionNumeratorAttribute @"AXMathFractionNumerator"
#define NSAccessibilityMathBaseAttribute @"AXMathBase"
#define NSAccessibilityMathSubscriptAttribute @"AXMathSubscript"
#define NSAccessibilityMathSuperscriptAttribute @"AXMathSuperscript"
#define NSAccessibilityMathUnderAttribute @"AXMathUnder"
#define NSAccessibilityMathOverAttribute @"AXMathOver"
#define NSAccessibilityMathFencedOpenAttribute @"AXMathFencedOpen"
#define NSAccessibilityMathFencedCloseAttribute @"AXMathFencedClose"
#define NSAccessibilityMathLineThicknessAttribute @"AXMathLineThickness"
#define NSAccessibilityMathPrescriptsAttribute @"AXMathPrescripts"
#define NSAccessibilityMathPostscriptsAttribute @"AXMathPostscripts"

@implementation WebAccessibilityObjectWrapper

- (void)unregisterUniqueIdForUIElement
{
    wkUnregisterUniqueIdForElement(self);
}

- (void)detach
{
    // Send unregisterUniqueIdForUIElement unconditionally because if it is
    // ever accidentally not done (via other bugs in our AX implementation) you
    // end up with a crash like <rdar://problem/4273149>.  It is safe and not
    // expensive to send even if the object is not registered.
    [self unregisterUniqueIdForUIElement];
    [super detach];
}

- (id)attachmentView
{
    ASSERT(m_object->isAttachment());
    Widget* widget = m_object->widgetForAttachmentView();
    if (!widget)
        return nil;
    return NSAccessibilityUnignoredDescendant(widget->platformWidget());
}

#pragma mark SystemInterface wrappers

static inline id CFAutoreleaseHelper(CFTypeRef obj)
{
    if (obj)
        CFMakeCollectable(obj);
    [(id)obj autorelease];
    return (id)obj;
}

static inline BOOL AXObjectIsTextMarker(id obj)
{
    return obj != nil && CFGetTypeID(obj) == wkGetAXTextMarkerTypeID();
}

static inline BOOL AXObjectIsTextMarkerRange(id obj)
{
    return obj != nil && CFGetTypeID(obj) == wkGetAXTextMarkerRangeTypeID();
}

static id AXTextMarkerRange(id startMarker, id endMarker)
{
    ASSERT(startMarker != nil);
    ASSERT(endMarker != nil);
    ASSERT(CFGetTypeID(startMarker) == wkGetAXTextMarkerTypeID());
    ASSERT(CFGetTypeID(endMarker) == wkGetAXTextMarkerTypeID());
    return CFAutoreleaseHelper(wkCreateAXTextMarkerRange((CFTypeRef)startMarker, (CFTypeRef)endMarker));
}

static id AXTextMarkerRangeStart(id range)
{
    ASSERT(range != nil);
    ASSERT(CFGetTypeID(range) == wkGetAXTextMarkerRangeTypeID());
    return CFAutoreleaseHelper(wkCopyAXTextMarkerRangeStart(range));
}

static id AXTextMarkerRangeEnd(id range)
{
    ASSERT(range != nil);
    ASSERT(CFGetTypeID(range) == wkGetAXTextMarkerRangeTypeID());
    return CFAutoreleaseHelper(wkCopyAXTextMarkerRangeEnd(range));
}

#pragma mark Search helpers

typedef HashMap<String, AccessibilitySearchKey> AccessibilitySearchKeyMap;

struct SearchKeyEntry {
    String key;
    AccessibilitySearchKey value;
};

static AccessibilitySearchKeyMap* createAccessibilitySearchKeyMap()
{
    const SearchKeyEntry searchKeys[] = {
        { NSAccessibilityAnyTypeSearchKey, AnyTypeSearchKey },
        { NSAccessibilityBlockquoteSameLevelSearchKey, BlockquoteSameLevelSearchKey },
        { NSAccessibilityBlockquoteSearchKey, BlockquoteSearchKey },
        { NSAccessibilityBoldFontSearchKey, BoldFontSearchKey },
        { NSAccessibilityButtonSearchKey, ButtonSearchKey },
        { NSAccessibilityCheckBoxSearchKey, CheckBoxSearchKey },
        { NSAccessibilityControlSearchKey, ControlSearchKey },
        { NSAccessibilityDifferentTypeSearchKey, DifferentTypeSearchKey },
        { NSAccessibilityFontChangeSearchKey, FontChangeSearchKey },
        { NSAccessibilityFontColorChangeSearchKey, FontColorChangeSearchKey },
        { NSAccessibilityFrameSearchKey, FrameSearchKey },
        { NSAccessibilityGraphicSearchKey, GraphicSearchKey },
        { NSAccessibilityHeadingLevel1SearchKey, HeadingLevel1SearchKey },
        { NSAccessibilityHeadingLevel2SearchKey, HeadingLevel2SearchKey },
        { NSAccessibilityHeadingLevel3SearchKey, HeadingLevel3SearchKey },
        { NSAccessibilityHeadingLevel4SearchKey, HeadingLevel4SearchKey },
        { NSAccessibilityHeadingLevel5SearchKey, HeadingLevel5SearchKey },
        { NSAccessibilityHeadingLevel6SearchKey, HeadingLevel6SearchKey },
        { NSAccessibilityHeadingSameLevelSearchKey, HeadingSameLevelSearchKey },
        { NSAccessibilityHeadingSearchKey, HeadingSearchKey },
        { NSAccessibilityHighlightedSearchKey, HighlightedSearchKey },
        { NSAccessibilityItalicFontSearchKey, ItalicFontSearchKey },
        { NSAccessibilityLandmarkSearchKey, LandmarkSearchKey },
        { NSAccessibilityLinkSearchKey, LinkSearchKey },
        { NSAccessibilityListSearchKey, ListSearchKey },
        { NSAccessibilityLiveRegionSearchKey, LiveRegionSearchKey },
        { NSAccessibilityMisspelledWordSearchKey, MisspelledWordSearchKey },
        { NSAccessibilityPlainTextSearchKey, PlainTextSearchKey },
        { NSAccessibilityRadioGroupSearchKey, RadioGroupSearchKey },
        { NSAccessibilitySameTypeSearchKey, SameTypeSearchKey },
        { NSAccessibilityStaticTextSearchKey, StaticTextSearchKey },
        { NSAccessibilityStyleChangeSearchKey, StyleChangeSearchKey },
        { NSAccessibilityTableSameLevelSearchKey, TableSameLevelSearchKey },
        { NSAccessibilityTableSearchKey, TableSearchKey },
        { NSAccessibilityTextFieldSearchKey, TextFieldSearchKey },
        { NSAccessibilityUnderlineSearchKey, UnderlineSearchKey },
        { NSAccessibilityUnvisitedLinkSearchKey, UnvisitedLinkSearchKey },
        { NSAccessibilityVisitedLinkSearchKey, VisitedLinkSearchKey }
    };
    
    AccessibilitySearchKeyMap* searchKeyMap = new AccessibilitySearchKeyMap;
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(searchKeys); i++)
        searchKeyMap->set(searchKeys[i].key, searchKeys[i].value);
    
    return searchKeyMap;
}

static AccessibilitySearchKey accessibilitySearchKeyForString(const String& value)
{
    if (value.isEmpty())
        return AnyTypeSearchKey;
    
    static const AccessibilitySearchKeyMap* searchKeyMap = createAccessibilitySearchKeyMap();
    
    AccessibilitySearchKey searchKey = searchKeyMap->get(value);
    
    return searchKey ? searchKey : AnyTypeSearchKey;
}

#pragma mark Text Marker helpers

static id textMarkerForVisiblePosition(AXObjectCache* cache, const VisiblePosition& visiblePos)
{
    ASSERT(cache);
    
    TextMarkerData textMarkerData;
    cache->textMarkerDataForVisiblePosition(textMarkerData, visiblePos);
    if (!textMarkerData.axID)
        return nil;
    
    return CFAutoreleaseHelper(wkCreateAXTextMarker(&textMarkerData, sizeof(textMarkerData)));
}

- (id)textMarkerForVisiblePosition:(const VisiblePosition &)visiblePos
{
    return textMarkerForVisiblePosition(m_object->axObjectCache(), visiblePos);
}

static VisiblePosition visiblePositionForTextMarker(AXObjectCache* cache, CFTypeRef textMarker)
{
    ASSERT(cache);
    
    if (!textMarker)
        return VisiblePosition();
    TextMarkerData textMarkerData;
    if (!wkGetBytesFromAXTextMarker(textMarker, &textMarkerData, sizeof(textMarkerData)))
        return VisiblePosition();
    
    return cache->visiblePositionForTextMarkerData(textMarkerData);
}

- (VisiblePosition)visiblePositionForTextMarker:(id)textMarker
{
    return visiblePositionForTextMarker(m_object->axObjectCache(), textMarker);
}

static VisiblePosition visiblePositionForStartOfTextMarkerRange(AXObjectCache *cache, id textMarkerRange)
{
    return visiblePositionForTextMarker(cache, AXTextMarkerRangeStart(textMarkerRange));
}

static VisiblePosition visiblePositionForEndOfTextMarkerRange(AXObjectCache *cache, id textMarkerRange)
{
    return visiblePositionForTextMarker(cache, AXTextMarkerRangeEnd(textMarkerRange));
}

static id textMarkerRangeFromMarkers(id textMarker1, id textMarker2)
{
    if (!textMarker1 || !textMarker2)
        return nil;
    
    return AXTextMarkerRange(textMarker1, textMarker2);
}

// When modifying attributed strings, the range can come from a source which may provide faulty information (e.g. the spell checker).
// To protect against such cases the range should be validated before adding or removing attributes.
static BOOL AXAttributedStringRangeIsValid(NSAttributedString* attrString, NSRange range)
{
    return (range.location < [attrString length] && NSMaxRange(range) <= [attrString length]);
}

static void AXAttributeStringSetFont(NSMutableAttributedString* attrString, NSString* attribute, NSFont* font, NSRange range)
{
    if (!AXAttributedStringRangeIsValid(attrString, range))
        return;
    
    if (font) {
        NSDictionary* dict = [NSDictionary dictionaryWithObjectsAndKeys:
                              [font fontName]                             , NSAccessibilityFontNameKey,
                              [font familyName]                           , NSAccessibilityFontFamilyKey,
                              [font displayName]                          , NSAccessibilityVisibleNameKey,
                              [NSNumber numberWithFloat:[font pointSize]] , NSAccessibilityFontSizeKey,
                              nil];
        
        [attrString addAttribute:attribute value:dict range:range];
    } else
        [attrString removeAttribute:attribute range:range];
    
}

static CGColorRef CreateCGColorIfDifferent(NSColor* nsColor, CGColorRef existingColor)
{
    // get color information assuming NSDeviceRGBColorSpace
    NSColor* rgbColor = [nsColor colorUsingColorSpaceName:NSDeviceRGBColorSpace];
    if (rgbColor == nil)
        rgbColor = [NSColor blackColor];
    CGFloat components[4];
    [rgbColor getRed:&components[0] green:&components[1] blue:&components[2] alpha:&components[3]];
    
    // create a new CGColorRef to return
    CGColorSpaceRef cgColorSpace = CGColorSpaceCreateDeviceRGB();
    CGColorRef cgColor = CGColorCreate(cgColorSpace, components);
    CGColorSpaceRelease(cgColorSpace);
    
    // check for match with existing color
    if (existingColor && CGColorEqualToColor(cgColor, existingColor)) {
        CGColorRelease(cgColor);
        cgColor = 0;
    }
    
    return cgColor;
}

static void AXAttributeStringSetColor(NSMutableAttributedString* attrString, NSString* attribute, NSColor* color, NSRange range)
{
    if (!AXAttributedStringRangeIsValid(attrString, range))
        return;
    
    if (color) {
        CGColorRef existingColor = (CGColorRef) [attrString attribute:attribute atIndex:range.location effectiveRange:nil];
        CGColorRef cgColor = CreateCGColorIfDifferent(color, existingColor);
        if (cgColor) {
            [attrString addAttribute:attribute value:(id)cgColor range:range];
            CGColorRelease(cgColor);
        }
    } else
        [attrString removeAttribute:attribute range:range];
}

static void AXAttributeStringSetNumber(NSMutableAttributedString* attrString, NSString* attribute, NSNumber* number, NSRange range)
{
    if (!AXAttributedStringRangeIsValid(attrString, range))
        return;
    
    if (number)
        [attrString addAttribute:attribute value:number range:range];
    else
        [attrString removeAttribute:attribute range:range];
}

static void AXAttributeStringSetStyle(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    RenderStyle* style = renderer->style();
    
    // set basic font info
    AXAttributeStringSetFont(attrString, NSAccessibilityFontTextAttribute, style->font().primaryFont()->getNSFont(), range);
    
    // set basic colors
    AXAttributeStringSetColor(attrString, NSAccessibilityForegroundColorTextAttribute, nsColor(style->visitedDependentColor(CSSPropertyColor)), range);
    AXAttributeStringSetColor(attrString, NSAccessibilityBackgroundColorTextAttribute, nsColor(style->visitedDependentColor(CSSPropertyBackgroundColor)), range);
    
    // set super/sub scripting
    EVerticalAlign alignment = style->verticalAlign();
    if (alignment == SUB)
        AXAttributeStringSetNumber(attrString, NSAccessibilitySuperscriptTextAttribute, [NSNumber numberWithInt:(-1)], range);
    else if (alignment == SUPER)
        AXAttributeStringSetNumber(attrString, NSAccessibilitySuperscriptTextAttribute, [NSNumber numberWithInt:1], range);
    else
        [attrString removeAttribute:NSAccessibilitySuperscriptTextAttribute range:range];
    
    // set shadow
    if (style->textShadow())
        AXAttributeStringSetNumber(attrString, NSAccessibilityShadowTextAttribute, [NSNumber numberWithBool:YES], range);
    else
        [attrString removeAttribute:NSAccessibilityShadowTextAttribute range:range];
    
    // set underline and strikethrough
    int decor = style->textDecorationsInEffect();
    if ((decor & TextDecorationUnderline) == 0) {
        [attrString removeAttribute:NSAccessibilityUnderlineTextAttribute range:range];
        [attrString removeAttribute:NSAccessibilityUnderlineColorTextAttribute range:range];
    }
    
    if ((decor & TextDecorationLineThrough) == 0) {
        [attrString removeAttribute:NSAccessibilityStrikethroughTextAttribute range:range];
        [attrString removeAttribute:NSAccessibilityStrikethroughColorTextAttribute range:range];
    }
    
    if ((decor & (TextDecorationUnderline | TextDecorationLineThrough)) != 0) {
        // find colors using quirk mode approach (strict mode would use current
        // color for all but the root line box, which would use getTextDecorationColors)
        Color underline, overline, linethrough;
        renderer->getTextDecorationColors(decor, underline, overline, linethrough);
        
        if ((decor & TextDecorationUnderline) != 0) {
            AXAttributeStringSetNumber(attrString, NSAccessibilityUnderlineTextAttribute, [NSNumber numberWithBool:YES], range);
            AXAttributeStringSetColor(attrString, NSAccessibilityUnderlineColorTextAttribute, nsColor(underline), range);
        }
        
        if ((decor & TextDecorationLineThrough) != 0) {
            AXAttributeStringSetNumber(attrString, NSAccessibilityStrikethroughTextAttribute, [NSNumber numberWithBool:YES], range);
            AXAttributeStringSetColor(attrString, NSAccessibilityStrikethroughColorTextAttribute, nsColor(linethrough), range);
        }
    }
    
    // Indicate background highlighting.
    for (Node* node = renderer->node(); node; node = node->parentNode()) {
        if (node->hasTagName(markTag))
            AXAttributeStringSetNumber(attrString, @"AXHighlight", [NSNumber numberWithBool:YES], range);
    }
}

static void AXAttributeStringSetBlockquoteLevel(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    if (!AXAttributedStringRangeIsValid(attrString, range))
        return;
    
    AccessibilityObject* obj = renderer->document()->axObjectCache()->getOrCreate(renderer);
    int quoteLevel = obj->blockquoteLevel();
    
    if (quoteLevel)
        [attrString addAttribute:NSAccessibilityBlockQuoteLevelAttribute value:[NSNumber numberWithInt:quoteLevel] range:range];
    else
        [attrString removeAttribute:NSAccessibilityBlockQuoteLevelAttribute range:range];
}

static void AXAttributeStringSetSpelling(NSMutableAttributedString* attrString, Node* node, const UChar* chars, int charLength, NSRange range)
{
    if (unifiedTextCheckerEnabled(node->document()->frame())) {
        // Check the spelling directly since document->markersForNode() does not store the misspelled marking when the cursor is in a word.
        TextCheckerClient* checker = node->document()->frame()->editor().textChecker();
        
        // checkTextOfParagraph is the only spelling/grammar checker implemented in WK1 and WK2
        Vector<TextCheckingResult> results;
        checkTextOfParagraph(checker, chars, charLength, TextCheckingTypeSpelling, results);
        
        size_t size = results.size();
        NSNumber* trueValue = [NSNumber numberWithBool:YES];
        for (unsigned i = 0; i < size; i++) {
            const TextCheckingResult& result = results[i];
            AXAttributeStringSetNumber(attrString, NSAccessibilityMisspelledTextAttribute, trueValue, NSMakeRange(result.location + range.location, result.length));
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
            AXAttributeStringSetNumber(attrString, NSAccessibilityMarkedMisspelledTextAttribute, trueValue, NSMakeRange(result.location + range.location, result.length));
#endif
        }
        return;
    }
    
    int currentPosition = 0;
    while (charLength > 0) {
        const UChar* charData = chars + currentPosition;
        TextCheckerClient* checker = node->document()->frame()->editor().textChecker();
        
        int misspellingLocation = -1;
        int misspellingLength = 0;
        checker->checkSpellingOfString(charData, charLength, &misspellingLocation, &misspellingLength);
        if (misspellingLocation == -1 || !misspellingLength)
            break;
        
        NSRange spellRange = NSMakeRange(range.location + currentPosition + misspellingLocation, misspellingLength);
        AXAttributeStringSetNumber(attrString, NSAccessibilityMisspelledTextAttribute, [NSNumber numberWithBool:YES], spellRange);
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
        AXAttributeStringSetNumber(attrString, NSAccessibilityMarkedMisspelledTextAttribute, [NSNumber numberWithBool:YES], spellRange);
#endif
        charLength -= (misspellingLocation + misspellingLength);
        currentPosition += (misspellingLocation + misspellingLength);
    }
}

static void AXAttributeStringSetHeadingLevel(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    if (!renderer)
        return;
    
    if (!AXAttributedStringRangeIsValid(attrString, range))
        return;
    
    // Sometimes there are objects between the text and the heading.
    // In those cases the parent hierarchy should be queried to see if there is a heading level.
    int parentHeadingLevel = 0;
    AccessibilityObject* parentObject = renderer->document()->axObjectCache()->getOrCreate(renderer->parent());
    for (; parentObject; parentObject = parentObject->parentObject()) {
        parentHeadingLevel = parentObject->headingLevel();
        if (parentHeadingLevel)
            break;
    }
    
    if (parentHeadingLevel)
        [attrString addAttribute:@"AXHeadingLevel" value:[NSNumber numberWithInt:parentHeadingLevel] range:range];
    else
        [attrString removeAttribute:@"AXHeadingLevel" range:range];
}

static void AXAttributeStringSetElement(NSMutableAttributedString* attrString, NSString* attribute, AccessibilityObject* object, NSRange range)
{
    if (!AXAttributedStringRangeIsValid(attrString, range))
        return;
    
    if (object && object->isAccessibilityRenderObject()) {
        // make a serializable AX object
        
        RenderObject* renderer = static_cast<AccessibilityRenderObject*>(object)->renderer();
        if (!renderer)
            return;
        
        Document* doc = renderer->document();
        if (!doc)
            return;
        
        AXObjectCache* cache = doc->axObjectCache();
        if (!cache)
            return;
        
        AXUIElementRef axElement = wkCreateAXUIElementRef(object->wrapper());
        if (axElement) {
            [attrString addAttribute:attribute value:(id)axElement range:range];
            CFRelease(axElement);
        }
    } else
        [attrString removeAttribute:attribute range:range];
}

static void AXAttributedStringAppendText(NSMutableAttributedString* attrString, Node* node, const UChar* chars, int length)
{
    // skip invisible text
    if (!node->renderer())
        return;
    
    // easier to calculate the range before appending the string
    NSRange attrStringRange = NSMakeRange([attrString length], length);
    
    // append the string from this node
    [[attrString mutableString] appendString:[NSString stringWithCharacters:chars length:length]];
    
    // add new attributes and remove irrelevant inherited ones
    // NOTE: color attributes are handled specially because -[NSMutableAttributedString addAttribute: value: range:] does not merge
    // identical colors.  Workaround is to not replace an existing color attribute if it matches what we are adding.  This also means
    // we cannot just pre-remove all inherited attributes on the appended string, so we have to remove the irrelevant ones individually.
    
    // remove inherited attachment from prior AXAttributedStringAppendReplaced
    [attrString removeAttribute:NSAccessibilityAttachmentTextAttribute range:attrStringRange];
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    [attrString removeAttribute:NSAccessibilityMarkedMisspelledTextAttribute range:attrStringRange];
#endif
    [attrString removeAttribute:NSAccessibilityMisspelledTextAttribute range:attrStringRange];
    
    // set new attributes
    AXAttributeStringSetStyle(attrString, node->renderer(), attrStringRange);
    AXAttributeStringSetHeadingLevel(attrString, node->renderer(), attrStringRange);
    AXAttributeStringSetBlockquoteLevel(attrString, node->renderer(), attrStringRange);
    AXAttributeStringSetElement(attrString, NSAccessibilityLinkTextAttribute, AccessibilityObject::anchorElementForNode(node), attrStringRange);
    
    // do spelling last because it tends to break up the range
    AXAttributeStringSetSpelling(attrString, node, chars, length, attrStringRange);
}

static NSString* nsStringForReplacedNode(Node* replacedNode)
{
    // we should always be given a rendered node and a replaced node, but be safe
    // replaced nodes are either attachments (widgets) or images
    if (!replacedNode || !replacedNode->renderer() || !replacedNode->renderer()->isReplaced() || replacedNode->isTextNode()) {
        ASSERT_NOT_REACHED();
        return nil;
    }
    
    // create an AX object, but skip it if it is not supposed to be seen
    RefPtr<AccessibilityObject> obj = replacedNode->renderer()->document()->axObjectCache()->getOrCreate(replacedNode->renderer());
    if (obj->accessibilityIsIgnored())
        return nil;
    
    // use the attachmentCharacter to represent the replaced node
    const UniChar attachmentChar = NSAttachmentCharacter;
    return [NSString stringWithCharacters:&attachmentChar length:1];
}

- (NSAttributedString*)doAXAttributedStringForTextMarkerRange:(id)textMarkerRange
{
    if (!m_object)
        return nil;
    
    // extract the start and end VisiblePosition
    VisiblePosition startVisiblePosition = visiblePositionForStartOfTextMarkerRange(m_object->axObjectCache(), textMarkerRange);
    if (startVisiblePosition.isNull())
        return nil;
    
    VisiblePosition endVisiblePosition = visiblePositionForEndOfTextMarkerRange(m_object->axObjectCache(), textMarkerRange);
    if (endVisiblePosition.isNull())
        return nil;
    
    VisiblePositionRange visiblePositionRange(startVisiblePosition, endVisiblePosition);
    // iterate over the range to build the AX attributed string
    NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] init];
    TextIterator it(makeRange(startVisiblePosition, endVisiblePosition).get());
    while (!it.atEnd()) {
        // locate the node and starting offset for this range
        int exception = 0;
        Node* node = it.range()->startContainer(exception);
        ASSERT(node == it.range()->endContainer(exception));
        int offset = it.range()->startOffset(exception);
        
        // non-zero length means textual node, zero length means replaced node (AKA "attachments" in AX)
        if (it.length() != 0) {
            // Add the text of the list marker item if necessary.
            String listMarkerText = m_object->listMarkerTextForNodeAndPosition(node, VisiblePosition(it.range()->startPosition()));
            if (!listMarkerText.isEmpty())
                AXAttributedStringAppendText(attrString, node, listMarkerText.characters(), listMarkerText.length());
            
            AXAttributedStringAppendText(attrString, node, it.characters(), it.length());
        } else {
            Node* replacedNode = node->childNode(offset);
            NSString *attachmentString = nsStringForReplacedNode(replacedNode);
            if (attachmentString) {
                NSRange attrStringRange = NSMakeRange([attrString length], [attachmentString length]);
                
                // append the placeholder string
                [[attrString mutableString] appendString:attachmentString];
                
                // remove all inherited attributes
                [attrString setAttributes:nil range:attrStringRange];
                
                // add the attachment attribute
                AccessibilityObject* obj = replacedNode->renderer()->document()->axObjectCache()->getOrCreate(replacedNode->renderer());
                AXAttributeStringSetElement(attrString, NSAccessibilityAttachmentTextAttribute, obj, attrStringRange);
            }
        }
        it.advance();
    }
    
    return [attrString autorelease];
}

static id textMarkerRangeFromVisiblePositions(AXObjectCache *cache, VisiblePosition startPosition, VisiblePosition endPosition)
{
    id startTextMarker = textMarkerForVisiblePosition(cache, startPosition);
    id endTextMarker = textMarkerForVisiblePosition(cache, endPosition);
    return textMarkerRangeFromMarkers(startTextMarker, endTextMarker);
}

- (id)textMarkerRangeFromVisiblePositions:(VisiblePosition)startPosition endPosition:(VisiblePosition)endPosition
{
    return textMarkerRangeFromVisiblePositions(m_object->axObjectCache(), startPosition, endPosition);
}

- (NSArray*)accessibilityActionNames
{
    if (![self updateObjectBackingStore])
        return nil;
    
    // All elements should get ShowMenu and ScrollToVisible.
    // But certain earlier VoiceOver versions do not support scroll to visible, and it confuses them to see it in the list.
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1090
    static NSArray *defaultElementActions = [[NSArray alloc] initWithObjects:NSAccessibilityShowMenuAction, nil];
#else
    static NSArray *defaultElementActions = [[NSArray alloc] initWithObjects:NSAccessibilityShowMenuAction, NSAccessibilityScrollToVisibleAction, nil];
#endif

    // Action elements allow Press.
    // The order is important to VoiceOver, which expects the 'default' action to be the first action. In this case the default action should be press.
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1090
    static NSArray *actionElementActions = [[NSArray alloc] initWithObjects:NSAccessibilityPressAction, NSAccessibilityShowMenuAction, nil];
#else
    static NSArray *actionElementActions = [[NSArray alloc] initWithObjects:NSAccessibilityPressAction, NSAccessibilityShowMenuAction, NSAccessibilityScrollToVisibleAction, nil];
#endif

    // Menu elements allow Press and Cancel.
    static NSArray *menuElementActions = [[actionElementActions arrayByAddingObject:NSAccessibilityCancelAction] retain];

    // Slider elements allow Increment/Decrement.
    static NSArray *sliderActions = [[defaultElementActions arrayByAddingObjectsFromArray:[NSArray arrayWithObjects:NSAccessibilityIncrementAction, NSAccessibilityDecrementAction, nil]] retain];
    
    NSArray *actions;
    if (m_object->actionElement() || m_object->isButton())
        actions = actionElementActions;
    else if (m_object->isMenuRelated())
        actions = menuElementActions;
    else if (m_object->isSlider())
        actions = sliderActions;
    else if (m_object->isAttachment())
        actions = [[self attachmentView] accessibilityActionNames];
    else
        actions = defaultElementActions;
    
    return actions;
}

- (NSArray*)additionalAccessibilityAttributeNames
{
    if (!m_object)
        return nil;
    
    NSMutableArray *additional = [NSMutableArray array];
    if (m_object->supportsARIAOwns())
        [additional addObject:NSAccessibilityOwnsAttribute];
    
    if (m_object->supportsARIAExpanded())
        [additional addObject:NSAccessibilityExpandedAttribute];
    
    if (m_object->isScrollbar())
        [additional addObject:NSAccessibilityOrientationAttribute];
    
    if (m_object->supportsARIADragging())
        [additional addObject:NSAccessibilityGrabbedAttribute];
    
    if (m_object->supportsARIADropping())
        [additional addObject:NSAccessibilityDropEffectsAttribute];
    
    if (m_object->isAccessibilityTable() && static_cast<AccessibilityTable*>(m_object)->supportsSelectedRows())
        [additional addObject:NSAccessibilitySelectedRowsAttribute];
    
    if (m_object->supportsARIALiveRegion()) {
        [additional addObject:NSAccessibilityARIALiveAttribute];
        [additional addObject:NSAccessibilityARIARelevantAttribute];
    }
    
    if (m_object->supportsARIASetSize())
        [additional addObject:NSAccessibilityARIASetSizeAttribute];
    if (m_object->supportsARIAPosInSet())
        [additional addObject:NSAccessibilityARIAPosInSetAttribute];
    
    if (m_object->sortDirection() != SortDirectionNone)
        [additional addObject:NSAccessibilitySortDirectionAttribute];
    
    // If an object is a child of a live region, then add these
    if (m_object->isInsideARIALiveRegion())
        [additional addObject:NSAccessibilityARIAAtomicAttribute];
    // All objects should expose the ARIA busy attribute (ARIA 1.1 with ISSUE-538).
    [additional addObject:NSAccessibilityARIABusyAttribute];
    
    // Popup buttons on the Mac expose the value attribute.
    if (m_object->isPopUpButton()) {
        [additional addObject:NSAccessibilityValueAttribute];
    }

    if (m_object->supportsRequiredAttribute()) {
        [additional addObject:NSAccessibilityRequiredAttribute];
    }
    
    if (m_object->ariaHasPopup())
        [additional addObject:NSAccessibilityHasPopupAttribute];
    
    if (m_object->isMathRoot()) {
        // The index of a square root is always known, so there's no object associated with it.
        if (!m_object->isMathSquareRoot())
            [additional addObject:NSAccessibilityMathRootIndexAttribute];
        [additional addObject:NSAccessibilityMathRootRadicandAttribute];
    } else if (m_object->isMathFraction()) {
        [additional addObject:NSAccessibilityMathFractionNumeratorAttribute];
        [additional addObject:NSAccessibilityMathFractionDenominatorAttribute];
        [additional addObject:NSAccessibilityMathLineThicknessAttribute];
    } else if (m_object->isMathSubscriptSuperscript()) {
        [additional addObject:NSAccessibilityMathBaseAttribute];
        [additional addObject:NSAccessibilityMathSubscriptAttribute];
        [additional addObject:NSAccessibilityMathSuperscriptAttribute];
    } else if (m_object->isMathUnderOver()) {
        [additional addObject:NSAccessibilityMathBaseAttribute];
        [additional addObject:NSAccessibilityMathUnderAttribute];
        [additional addObject:NSAccessibilityMathOverAttribute];
    } else if (m_object->isMathFenced()) {
        [additional addObject:NSAccessibilityMathFencedOpenAttribute];
        [additional addObject:NSAccessibilityMathFencedCloseAttribute];
    } else if (m_object->isMathMultiscript()) {
        [additional addObject:NSAccessibilityMathBaseAttribute];
        [additional addObject:NSAccessibilityMathPrescriptsAttribute];
        [additional addObject:NSAccessibilityMathPostscriptsAttribute];
    }
    
    if (m_object->supportsPath())
        [additional addObject:NSAccessibilityPathAttribute];
    
    return additional;
}

- (NSArray*)accessibilityAttributeNames
{
    if (![self updateObjectBackingStore])
        return nil;
    
    if (m_object->isAttachment())
        return [[self attachmentView] accessibilityAttributeNames];
    
    static NSArray* attributes = nil;
    static NSArray* anchorAttrs = nil;
    static NSArray* webAreaAttrs = nil;
    static NSArray* textAttrs = nil;
    static NSArray* listAttrs = nil;
    static NSArray* listBoxAttrs = nil;
    static NSArray* rangeAttrs = nil;
    static NSArray* commonMenuAttrs = nil;
    static NSArray* menuAttrs = nil;
    static NSArray* menuBarAttrs = nil;
    static NSArray* menuItemAttrs = nil;
    static NSArray* menuButtonAttrs = nil;
    static NSArray* controlAttrs = nil;
    static NSArray* tableAttrs = nil;
    static NSArray* tableRowAttrs = nil;
    static NSArray* tableColAttrs = nil;
    static NSArray* tableCellAttrs = nil;
    static NSArray* groupAttrs = nil;
    static NSArray* inputImageAttrs = nil;
    static NSArray* passwordFieldAttrs = nil;
    static NSArray* tabListAttrs = nil;
    static NSArray* comboBoxAttrs = nil;
    static NSArray* outlineAttrs = nil;
    static NSArray* outlineRowAttrs = nil;
    static NSArray* buttonAttrs = nil;
    static NSArray* scrollViewAttrs = nil;
    static NSArray* incrementorAttrs = nil;
    NSMutableArray* tempArray;
    if (attributes == nil) {
        attributes = [[NSArray alloc] initWithObjects: NSAccessibilityRoleAttribute,
                      NSAccessibilitySubroleAttribute,
                      NSAccessibilityRoleDescriptionAttribute,
                      NSAccessibilityChildrenAttribute,
                      NSAccessibilityHelpAttribute,
                      NSAccessibilityParentAttribute,
                      NSAccessibilityPositionAttribute,
                      NSAccessibilitySizeAttribute,
                      NSAccessibilityTitleAttribute,
                      NSAccessibilityDescriptionAttribute,
                      NSAccessibilityValueAttribute,
                      NSAccessibilityFocusedAttribute,
                      NSAccessibilityEnabledAttribute,
                      NSAccessibilityWindowAttribute,
                      @"AXSelectedTextMarkerRange",
                      @"AXStartTextMarker",
                      @"AXEndTextMarker",
                      @"AXVisited",
                      NSAccessibilityLinkedUIElementsAttribute,
                      NSAccessibilitySelectedAttribute,
                      NSAccessibilityBlockQuoteLevelAttribute,
                      NSAccessibilityTopLevelUIElementAttribute,
                      nil];
    }
    if (commonMenuAttrs == nil) {
        commonMenuAttrs = [[NSArray alloc] initWithObjects: NSAccessibilityRoleAttribute,
                           NSAccessibilityRoleDescriptionAttribute,
                           NSAccessibilityChildrenAttribute,
                           NSAccessibilityParentAttribute,
                           NSAccessibilityEnabledAttribute,
                           NSAccessibilityPositionAttribute,
                           NSAccessibilitySizeAttribute,
                           nil];
    }
    if (anchorAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityURLAttribute];
        [tempArray addObject:NSAccessibilityAccessKeyAttribute];
        anchorAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (webAreaAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:@"AXLinkUIElements"];
        [tempArray addObject:@"AXLoaded"];
        [tempArray addObject:@"AXLayoutCount"];
        [tempArray addObject:NSAccessibilityLoadingProgressAttribute];
        [tempArray addObject:NSAccessibilityURLAttribute];
        webAreaAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (textAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityNumberOfCharactersAttribute];
        [tempArray addObject:NSAccessibilitySelectedTextAttribute];
        [tempArray addObject:NSAccessibilitySelectedTextRangeAttribute];
        [tempArray addObject:NSAccessibilityVisibleCharacterRangeAttribute];
        [tempArray addObject:NSAccessibilityInsertionPointLineNumberAttribute];
        [tempArray addObject:NSAccessibilityTitleUIElementAttribute];
        [tempArray addObject:NSAccessibilityAccessKeyAttribute];
        [tempArray addObject:NSAccessibilityRequiredAttribute];
        [tempArray addObject:NSAccessibilityInvalidAttribute];
        [tempArray addObject:NSAccessibilityPlaceholderValueAttribute];
        textAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (listAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilitySelectedChildrenAttribute];
        [tempArray addObject:NSAccessibilityVisibleChildrenAttribute];
        [tempArray addObject:NSAccessibilityOrientationAttribute];
        [tempArray addObject:NSAccessibilityTitleUIElementAttribute];
        listAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (listBoxAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:listAttrs];
        [tempArray addObject:NSAccessibilityAccessKeyAttribute];
        [tempArray addObject:NSAccessibilityRequiredAttribute];
        [tempArray addObject:NSAccessibilityInvalidAttribute];
        listBoxAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (rangeAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityMinValueAttribute];
        [tempArray addObject:NSAccessibilityMaxValueAttribute];
        [tempArray addObject:NSAccessibilityOrientationAttribute];
        [tempArray addObject:NSAccessibilityValueDescriptionAttribute];
        [tempArray addObject:NSAccessibilityTitleUIElementAttribute];
        rangeAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (menuBarAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:commonMenuAttrs];
        [tempArray addObject:NSAccessibilitySelectedChildrenAttribute];
        [tempArray addObject:NSAccessibilityVisibleChildrenAttribute];
        [tempArray addObject:NSAccessibilityTitleUIElementAttribute];
        menuBarAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (menuAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:commonMenuAttrs];
        [tempArray addObject:NSAccessibilitySelectedChildrenAttribute];
        [tempArray addObject:NSAccessibilityVisibleChildrenAttribute];
        [tempArray addObject:NSAccessibilityTitleUIElementAttribute];
        menuAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (menuItemAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:commonMenuAttrs];
        [tempArray addObject:NSAccessibilityTitleAttribute];
        [tempArray addObject:NSAccessibilityHelpAttribute];
        [tempArray addObject:NSAccessibilitySelectedAttribute];
        [tempArray addObject:(NSString*)kAXMenuItemCmdCharAttribute];
        [tempArray addObject:(NSString*)kAXMenuItemCmdVirtualKeyAttribute];
        [tempArray addObject:(NSString*)kAXMenuItemCmdGlyphAttribute];
        [tempArray addObject:(NSString*)kAXMenuItemCmdModifiersAttribute];
        [tempArray addObject:(NSString*)kAXMenuItemMarkCharAttribute];
        [tempArray addObject:(NSString*)kAXMenuItemPrimaryUIElementAttribute];
        [tempArray addObject:NSAccessibilityServesAsTitleForUIElementsAttribute];
        menuItemAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (menuButtonAttrs == nil) {
        menuButtonAttrs = [[NSArray alloc] initWithObjects:NSAccessibilityRoleAttribute,
                           NSAccessibilityRoleDescriptionAttribute,
                           NSAccessibilityParentAttribute,
                           NSAccessibilityPositionAttribute,
                           NSAccessibilitySizeAttribute,
                           NSAccessibilityWindowAttribute,
                           NSAccessibilityEnabledAttribute,
                           NSAccessibilityFocusedAttribute,
                           NSAccessibilityTitleAttribute,
                           NSAccessibilityChildrenAttribute, nil];
    }
    if (controlAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityTitleUIElementAttribute];
        [tempArray addObject:NSAccessibilityAccessKeyAttribute];
        [tempArray addObject:NSAccessibilityRequiredAttribute];
        [tempArray addObject:NSAccessibilityInvalidAttribute];
        controlAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (incrementorAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityIncrementButtonAttribute];
        [tempArray addObject:NSAccessibilityDecrementButtonAttribute];
        incrementorAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (buttonAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        // Buttons should not expose AXValue.
        [tempArray removeObject:NSAccessibilityValueAttribute];
        [tempArray addObject:NSAccessibilityTitleUIElementAttribute];
        [tempArray addObject:NSAccessibilityAccessKeyAttribute];
        buttonAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (comboBoxAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:controlAttrs];
        [tempArray addObject:NSAccessibilityExpandedAttribute];
        comboBoxAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (tableAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityRowsAttribute];
        [tempArray addObject:NSAccessibilityVisibleRowsAttribute];
        [tempArray addObject:NSAccessibilityColumnsAttribute];
        [tempArray addObject:NSAccessibilityVisibleColumnsAttribute];
        [tempArray addObject:NSAccessibilityVisibleCellsAttribute];
        [tempArray addObject:(NSString *)kAXColumnHeaderUIElementsAttribute];
        [tempArray addObject:NSAccessibilityRowHeaderUIElementsAttribute];
        [tempArray addObject:NSAccessibilityHeaderAttribute];
        tableAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (tableRowAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityIndexAttribute];
        tableRowAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (tableColAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityIndexAttribute];
        [tempArray addObject:NSAccessibilityHeaderAttribute];
        [tempArray addObject:NSAccessibilityRowsAttribute];
        [tempArray addObject:NSAccessibilityVisibleRowsAttribute];
        tableColAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (tableCellAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityRowIndexRangeAttribute];
        [tempArray addObject:NSAccessibilityColumnIndexRangeAttribute];
        tableCellAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (groupAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityTitleUIElementAttribute];
        groupAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (inputImageAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:buttonAttrs];
        [tempArray addObject:NSAccessibilityURLAttribute];
        inputImageAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (passwordFieldAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityTitleUIElementAttribute];
        [tempArray addObject:NSAccessibilityRequiredAttribute];
        [tempArray addObject:NSAccessibilityInvalidAttribute];
        [tempArray addObject:NSAccessibilityPlaceholderValueAttribute];
        passwordFieldAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (tabListAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityTabsAttribute];
        [tempArray addObject:NSAccessibilityContentsAttribute];
        tabListAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (outlineAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilitySelectedRowsAttribute];
        [tempArray addObject:NSAccessibilityRowsAttribute];
        [tempArray addObject:NSAccessibilityColumnsAttribute];
        outlineAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (outlineRowAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:tableRowAttrs];
        [tempArray addObject:NSAccessibilityDisclosingAttribute];
        [tempArray addObject:NSAccessibilityDisclosedByRowAttribute];
        [tempArray addObject:NSAccessibilityDisclosureLevelAttribute];
        [tempArray addObject:NSAccessibilityDisclosedRowsAttribute];
        outlineRowAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (scrollViewAttrs == nil) {
        tempArray = [[NSMutableArray alloc] initWithArray:attributes];
        [tempArray addObject:NSAccessibilityContentsAttribute];
        [tempArray addObject:NSAccessibilityHorizontalScrollBarAttribute];
        [tempArray addObject:NSAccessibilityVerticalScrollBarAttribute];
        scrollViewAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    
    NSArray *objectAttributes = attributes;
    
    if (m_object->isPasswordField())
        objectAttributes = passwordFieldAttrs;
    
    else if (m_object->isWebArea())
        objectAttributes = webAreaAttrs;
    
    else if (m_object->isTextControl())
        objectAttributes = textAttrs;
    
    else if (m_object->isAnchor() || m_object->isImage() || m_object->isLink())
        objectAttributes = anchorAttrs;
    
    else if (m_object->isAccessibilityTable())
        objectAttributes = tableAttrs;
    else if (m_object->isTableColumn())
        objectAttributes = tableColAttrs;
    else if (m_object->isTableCell())
        objectAttributes = tableCellAttrs;
    else if (m_object->isTableRow()) {
        // An ARIA table row can be collapsed and expanded, so it needs the extra attributes.
        if (m_object->isARIATreeGridRow())
            objectAttributes = outlineRowAttrs;
        else
            objectAttributes = tableRowAttrs;
    }
    
    else if (m_object->isTree())
        objectAttributes = outlineAttrs;
    else if (m_object->isTreeItem())
        objectAttributes = outlineRowAttrs;
    
    else if (m_object->isListBox())
        objectAttributes = listBoxAttrs;
    else if (m_object->isList())
        objectAttributes = listAttrs;
    
    else if (m_object->isComboBox())
        objectAttributes = comboBoxAttrs;
    
    else if (m_object->isProgressIndicator() || m_object->isSlider())
        objectAttributes = rangeAttrs;
    
    // These are processed in order because an input image is a button, and a button is a control.
    else if (m_object->isInputImage())
        objectAttributes = inputImageAttrs;
    else if (m_object->isButton())
        objectAttributes = buttonAttrs;
    else if (m_object->isControl())
        objectAttributes = controlAttrs;
    
    else if (m_object->isGroup() || m_object->isListItem())
        objectAttributes = groupAttrs;
    else if (m_object->isTabList())
        objectAttributes = tabListAttrs;
    else if (m_object->isScrollView())
        objectAttributes = scrollViewAttrs;
    else if (m_object->isSpinButton())
        objectAttributes = incrementorAttrs;
    
    else if (m_object->isMenu())
        objectAttributes = menuAttrs;
    else if (m_object->isMenuBar())
        objectAttributes = menuBarAttrs;
    else if (m_object->isMenuButton())
        objectAttributes = menuButtonAttrs;
    else if (m_object->isMenuItem())
        objectAttributes = menuItemAttrs;
    
    NSArray *additionalAttributes = [self additionalAccessibilityAttributeNames];
    if ([additionalAttributes count])
        objectAttributes = [objectAttributes arrayByAddingObjectsFromArray:additionalAttributes];
    
    return objectAttributes;
}

- (VisiblePositionRange)visiblePositionRangeForTextMarkerRange:(id)textMarkerRange
{
    if (!textMarkerRange)
        return VisiblePositionRange();
    AXObjectCache* cache = m_object->axObjectCache();
    return VisiblePositionRange(visiblePositionForStartOfTextMarkerRange(cache, textMarkerRange), visiblePositionForEndOfTextMarkerRange(cache, textMarkerRange));
}

- (NSArray*)renderWidgetChildren
{
    Widget* widget = m_object->widget();
    if (!widget)
        return nil;
    return [(widget->platformWidget()) accessibilityAttributeValue: NSAccessibilityChildrenAttribute];
}

- (id)remoteAccessibilityParentObject
{
    if (!m_object)
        return nil;
    
    Document* document = m_object->document();
    if (!document)
        return nil;
    
    Frame* frame = document->frame();
    if (!frame)
        return nil;
    
    return frame->loader()->client()->accessibilityRemoteObject();
}

static void convertToVector(NSArray* array, AccessibilityObject::AccessibilityChildrenVector& vector)
{
    unsigned length = [array count];
    vector.reserveInitialCapacity(length);
    for (unsigned i = 0; i < length; ++i) {
        AccessibilityObject* obj = [[array objectAtIndex:i] accessibilityObject];
        if (obj)
            vector.append(obj);
    }
}

static NSMutableArray* convertToNSArray(const AccessibilityObject::AccessibilityChildrenVector& vector)
{
    unsigned length = vector.size();
    NSMutableArray* array = [NSMutableArray arrayWithCapacity: length];
    for (unsigned i = 0; i < length; ++i) {
        WebAccessibilityObjectWrapper* wrapper = vector[i]->wrapper();
        ASSERT(wrapper);
        if (wrapper) {
            // we want to return the attachment view instead of the object representing the attachment.
            // otherwise, we get palindrome errors in the AX hierarchy
            if (vector[i]->isAttachment() && [wrapper attachmentView])
                [array addObject:[wrapper attachmentView]];
            else
                [array addObject:wrapper];
        }
    }
    return array;
}

- (id)textMarkerRangeForSelection
{
    VisibleSelection selection = m_object->selection();
    if (selection.isNone())
        return nil;
    return [self textMarkerRangeFromVisiblePositions:selection.visibleStart() endPosition:selection.visibleEnd()];
}

- (CGPoint)convertPointToScreenSpace:(FloatPoint &)point
{
    FrameView* frameView = m_object->documentFrameView();
    
    // WebKit1 code path... platformWidget() exists.
    if (frameView && frameView->platformWidget()) {
        
        NSPoint nsPoint = (NSPoint)point;
        NSView* view = frameView->documentView();
        nsPoint = [[view window] convertBaseToScreen:[view convertPoint:nsPoint toView:nil]];
        return CGPointMake(nsPoint.x, nsPoint.y);

    } else {
        
        // Find the appropriate scroll view to use to convert the contents to the window.
        ScrollView* scrollView = 0;
        AccessibilityObject* parent = 0;
        for (parent = m_object->parentObject(); parent; parent = parent->parentObject()) {
            if (parent->isAccessibilityScrollView()) {
                scrollView = toAccessibilityScrollView(parent)->scrollView();
                break;
            }
        }
        
        IntPoint intPoint = flooredIntPoint(point);
        if (scrollView)
            intPoint = scrollView->contentsToRootView(intPoint);
        
        Page* page = m_object->page();
        
        // If we have an empty chrome client (like SVG) then we should use the page
        // of the scroll view parent to help us get to the screen rect.
        if (parent && page && page->chrome().client()->isEmptyChromeClient())
            page = parent->page();
        
        if (page) {
            IntRect rect = IntRect(intPoint, IntSize(0, 0));            
            intPoint = page->chrome().rootViewToScreen(rect).location();
        }
        
        return intPoint;
    }
}

static void WebTransformCGPathToNSBezierPath(void *info, const CGPathElement *element)
{
    NSBezierPath *bezierPath = (NSBezierPath *)info;
    switch (element->type) {
    case kCGPathElementMoveToPoint:
        [bezierPath moveToPoint:NSPointFromCGPoint(element->points[0])];
        break;
    case kCGPathElementAddLineToPoint:
        [bezierPath lineToPoint:NSPointFromCGPoint(element->points[0])];
        break;
    case kCGPathElementAddCurveToPoint:
        [bezierPath curveToPoint:NSPointFromCGPoint(element->points[0]) controlPoint1:NSPointFromCGPoint(element->points[1]) controlPoint2:NSPointFromCGPoint(element->points[2])];
        break;
    case kCGPathElementCloseSubpath:
        [bezierPath closePath];
        break;
    default:
        break;
    }
}

- (NSBezierPath *)bezierPathFromPath:(CGPathRef)path
{
    NSBezierPath *bezierPath = [NSBezierPath bezierPath];
    CGPathApply(path, bezierPath, WebTransformCGPathToNSBezierPath);
    return bezierPath;
}

- (NSBezierPath *)path
{
    Path path = m_object->elementPath();
    if (path.isEmpty())
        return NULL;
    
    CGPathRef transformedPath = [self convertPathToScreenSpace:path];
    return [self bezierPathFromPath:transformedPath];
}

- (NSValue *)position
{
    IntRect rect = pixelSnappedIntRect(m_object->elementRect());
    
    // The Cocoa accessibility API wants the lower-left corner.
    FloatPoint floatPoint = FloatPoint(rect.x(), rect.maxY());

    CGPoint cgPoint = [self convertPointToScreenSpace:floatPoint];
    
    return [NSValue valueWithPoint:NSMakePoint(cgPoint.x, cgPoint.y)];
}

typedef HashMap<int, NSString*> AccessibilityRoleMap;

static const AccessibilityRoleMap& createAccessibilityRoleMap()
{
    struct RoleEntry {
        AccessibilityRole value;
        NSString* string;
    };
    
    static const RoleEntry roles[] = {
        { UnknownRole, NSAccessibilityUnknownRole },
        { ButtonRole, NSAccessibilityButtonRole },
        { RadioButtonRole, NSAccessibilityRadioButtonRole },
        { CheckBoxRole, NSAccessibilityCheckBoxRole },
        { SliderRole, NSAccessibilitySliderRole },
        { TabGroupRole, NSAccessibilityTabGroupRole },
        { TextFieldRole, NSAccessibilityTextFieldRole },
        { StaticTextRole, NSAccessibilityStaticTextRole },
        { TextAreaRole, NSAccessibilityTextAreaRole },
        { ScrollAreaRole, NSAccessibilityScrollAreaRole },
        { PopUpButtonRole, NSAccessibilityPopUpButtonRole },
        { MenuButtonRole, NSAccessibilityMenuButtonRole },
        { TableRole, NSAccessibilityTableRole },
        { ApplicationRole, NSAccessibilityApplicationRole },
        { GroupRole, NSAccessibilityGroupRole },
        { RadioGroupRole, NSAccessibilityRadioGroupRole },
        { ListRole, NSAccessibilityListRole },
        { DirectoryRole, NSAccessibilityListRole },
        { ScrollBarRole, NSAccessibilityScrollBarRole },
        { ValueIndicatorRole, NSAccessibilityValueIndicatorRole },
        { ImageRole, NSAccessibilityImageRole },
        { MenuBarRole, NSAccessibilityMenuBarRole },
        { MenuRole, NSAccessibilityMenuRole },
        { MenuItemRole, NSAccessibilityMenuItemRole },
        { ColumnRole, NSAccessibilityColumnRole },
        { RowRole, NSAccessibilityRowRole },
        { ToolbarRole, NSAccessibilityToolbarRole },
        { BusyIndicatorRole, NSAccessibilityBusyIndicatorRole },
        { ProgressIndicatorRole, NSAccessibilityProgressIndicatorRole },
        { WindowRole, NSAccessibilityWindowRole },
        { DrawerRole, NSAccessibilityDrawerRole },
        { SystemWideRole, NSAccessibilitySystemWideRole },
        { OutlineRole, NSAccessibilityOutlineRole },
        { IncrementorRole, NSAccessibilityIncrementorRole },
        { BrowserRole, NSAccessibilityBrowserRole },
        { ComboBoxRole, NSAccessibilityComboBoxRole },
        { SplitGroupRole, NSAccessibilitySplitGroupRole },
        { SplitterRole, NSAccessibilitySplitterRole },
        { ColorWellRole, NSAccessibilityColorWellRole },
        { GrowAreaRole, NSAccessibilityGrowAreaRole },
        { SheetRole, NSAccessibilitySheetRole },
        { HelpTagRole, NSAccessibilityHelpTagRole },
        { MatteRole, NSAccessibilityMatteRole },
        { RulerRole, NSAccessibilityRulerRole },
        { RulerMarkerRole, NSAccessibilityRulerMarkerRole },
        { LinkRole, NSAccessibilityLinkRole },
        { DisclosureTriangleRole, NSAccessibilityDisclosureTriangleRole },
        { GridRole, NSAccessibilityGridRole },
        { WebCoreLinkRole, NSAccessibilityLinkRole },
        { ImageMapLinkRole, NSAccessibilityLinkRole },
        { ImageMapRole, @"AXImageMap" },
        { ListMarkerRole, @"AXListMarker" },
        { WebAreaRole, @"AXWebArea" },
        { SeamlessWebAreaRole, NSAccessibilityGroupRole },
        { HeadingRole, @"AXHeading" },
        { ListBoxRole, NSAccessibilityListRole },
        { ListBoxOptionRole, NSAccessibilityStaticTextRole },
        { CellRole, NSAccessibilityCellRole },
        { TableHeaderContainerRole, NSAccessibilityGroupRole },
        { RowHeaderRole, NSAccessibilityGroupRole },
        { DefinitionRole, NSAccessibilityGroupRole },
        { DescriptionListDetailRole, NSAccessibilityGroupRole },
        { DescriptionListTermRole, NSAccessibilityGroupRole },
        { DescriptionListRole, NSAccessibilityListRole },
        { SliderThumbRole, NSAccessibilityValueIndicatorRole },
        { LandmarkApplicationRole, NSAccessibilityGroupRole },
        { LandmarkBannerRole, NSAccessibilityGroupRole },
        { LandmarkComplementaryRole, NSAccessibilityGroupRole },
        { LandmarkContentInfoRole, NSAccessibilityGroupRole },
        { LandmarkMainRole, NSAccessibilityGroupRole },
        { LandmarkNavigationRole, NSAccessibilityGroupRole },
        { LandmarkSearchRole, NSAccessibilityGroupRole },
        { ApplicationAlertRole, NSAccessibilityGroupRole },
        { ApplicationAlertDialogRole, NSAccessibilityGroupRole },
        { ApplicationDialogRole, NSAccessibilityGroupRole },
        { ApplicationLogRole, NSAccessibilityGroupRole },
        { ApplicationMarqueeRole, NSAccessibilityGroupRole },
        { ApplicationStatusRole, NSAccessibilityGroupRole },
        { ApplicationTimerRole, NSAccessibilityGroupRole },
        { DocumentRole, NSAccessibilityGroupRole },
        { DocumentArticleRole, NSAccessibilityGroupRole },
        { DocumentMathRole, NSAccessibilityGroupRole },
        { DocumentNoteRole, NSAccessibilityGroupRole },
        { DocumentRegionRole, NSAccessibilityGroupRole },
        { UserInterfaceTooltipRole, NSAccessibilityGroupRole },
        { TabRole, NSAccessibilityRadioButtonRole },
        { TabListRole, NSAccessibilityTabGroupRole },
        { TabPanelRole, NSAccessibilityGroupRole },
        { TreeRole, NSAccessibilityOutlineRole },
        { TreeItemRole, NSAccessibilityRowRole },
        { ListItemRole, NSAccessibilityGroupRole },
        { ParagraphRole, NSAccessibilityGroupRole },
        { LabelRole, NSAccessibilityGroupRole },
        { DivRole, NSAccessibilityGroupRole },
        { FormRole, NSAccessibilityGroupRole },
        { SpinButtonRole, NSAccessibilityIncrementorRole },
        { FooterRole, NSAccessibilityGroupRole },
        { ToggleButtonRole, NSAccessibilityButtonRole },
        { CanvasRole, NSAccessibilityImageRole },
        { SVGRootRole, NSAccessibilityGroupRole },
        { LegendRole, NSAccessibilityGroupRole },
        { MathElementRole, NSAccessibilityGroupRole }
    };
    AccessibilityRoleMap& roleMap = *new AccessibilityRoleMap;
    
    const unsigned numRoles = sizeof(roles) / sizeof(roles[0]);
    for (unsigned i = 0; i < numRoles; ++i)
        roleMap.set(roles[i].value, roles[i].string);
    return roleMap;
}

static NSString* roleValueToNSString(AccessibilityRole value)
{
    ASSERT(value);
    static const AccessibilityRoleMap& roleMap = createAccessibilityRoleMap();
    return roleMap.get(value);
}

- (NSString*)role
{
    if (m_object->isAttachment())
        return [[self attachmentView] accessibilityAttributeValue:NSAccessibilityRoleAttribute];
    AccessibilityRole role = m_object->roleValue();
    if (role == CanvasRole && m_object->canvasHasFallbackContent())
        role = GroupRole;
    NSString* string = roleValueToNSString(role);
    if (string != nil)
        return string;
    return NSAccessibilityUnknownRole;
}

- (NSString*)subrole
{
    if (m_object->isPasswordField())
        return NSAccessibilitySecureTextFieldSubrole;
    if (m_object->isSearchField())
        return NSAccessibilitySearchFieldSubrole;
    
    if (m_object->isAttachment()) {
        NSView* attachView = [self attachmentView];
        if ([[attachView accessibilityAttributeNames] containsObject:NSAccessibilitySubroleAttribute]) {
            return [attachView accessibilityAttributeValue:NSAccessibilitySubroleAttribute];
        }
    }
    
    if (m_object->isSpinButtonPart()) {
        if (toAccessibilitySpinButtonPart(m_object)->isIncrementor())
            return NSAccessibilityIncrementArrowSubrole;
        
        return NSAccessibilityDecrementArrowSubrole;
    }
    
    if (m_object->isFileUploadButton())
        return @"AXFileUploadButton";
    
    if (m_object->isTreeItem())
        return NSAccessibilityOutlineRowSubrole;
    
    if (m_object->isList()) {
        AccessibilityList* listObject = toAccessibilityList(m_object);
        if (listObject->isUnorderedList() || listObject->isOrderedList())
            return NSAccessibilityContentListSubrole;
        if (listObject->isDescriptionList()) {
#if __MAC_OS_X_VERSION_MIN_REQUIRED < 1090
            return NSAccessibilityDefinitionListSubrole;
#else
            return NSAccessibilityDescriptionListSubrole;
#endif
        }
    }
    
    // ARIA content subroles.
    switch (m_object->roleValue()) {
        case LandmarkApplicationRole:
            return @"AXLandmarkApplication";
        case LandmarkBannerRole:
            return @"AXLandmarkBanner";
        case LandmarkComplementaryRole:
            return @"AXLandmarkComplementary";
            // Footer roles should appear as content info types.
        case FooterRole:
        case LandmarkContentInfoRole:
            return @"AXLandmarkContentInfo";
        case LandmarkMainRole:
            return @"AXLandmarkMain";
        case LandmarkNavigationRole:
            return @"AXLandmarkNavigation";
        case LandmarkSearchRole:
            return @"AXLandmarkSearch";
        case ApplicationAlertRole:
            return @"AXApplicationAlert";
        case ApplicationAlertDialogRole:
            return @"AXApplicationAlertDialog";
        case ApplicationDialogRole:
            return @"AXApplicationDialog";
        case ApplicationLogRole:
            return @"AXApplicationLog";
        case ApplicationMarqueeRole:
            return @"AXApplicationMarquee";
        case ApplicationStatusRole:
            return @"AXApplicationStatus";
        case ApplicationTimerRole:
            return @"AXApplicationTimer";
        case DocumentRole:
            return @"AXDocument";
        case DocumentArticleRole:
            return @"AXDocumentArticle";
        case DocumentMathRole:
            return @"AXDocumentMath";
        case DocumentNoteRole:
            return @"AXDocumentNote";
        case DocumentRegionRole:
            return @"AXDocumentRegion";
        case UserInterfaceTooltipRole:
            return @"AXUserInterfaceTooltip";
        case TabPanelRole:
            return @"AXTabPanel";
        case DefinitionRole:
            return @"AXDefinition";
        case DescriptionListTermRole:
            return @"AXTerm";
        case DescriptionListDetailRole:
            return @"AXDescription";
            // Default doesn't return anything, so roles defined below can be chosen.
        default:
            break;
    }
    
    if (m_object->roleValue() == MathElementRole) {
        if (m_object->isMathFraction())
            return @"AXMathFraction";
        if (m_object->isMathFenced())
            return @"AXMathFenced";
        if (m_object->isMathSubscriptSuperscript())
            return @"AXMathSubscriptSuperscript";
        if (m_object->isMathRow())
            return @"AXMathRow";
        if (m_object->isMathUnderOver())
            return @"AXMathUnderOver";
        if (m_object->isMathSquareRoot())
            return @"AXMathSquareRoot";
        if (m_object->isMathRoot())
            return @"AXMathRoot";
        if (m_object->isMathText())
            return @"AXMathText";
        if (m_object->isMathNumber())
            return @"AXMathNumber";
        if (m_object->isMathIdentifier())
            return @"AXMathIdentifier";
        if (m_object->isMathTable())
            return @"AXMathTable";
        if (m_object->isMathTableRow())
            return @"AXMathTableRow";
        if (m_object->isMathTableCell())
            return @"AXMathTableCell";
        if (m_object->isMathFenceOperator())
            return @"AXMathFenceOperator";
        if (m_object->isMathSeparatorOperator())
            return @"AXMathSeparatorOperator";
        if (m_object->isMathOperator())
            return @"AXMathOperator";
        if (m_object->isMathMultiscript())
            return @"AXMathMultiscript";
    }
    
    if (m_object->isMediaTimeline())
        return NSAccessibilityTimelineSubrole;
    
    return nil;
}

- (NSString*)roleDescription
{
    if (!m_object)
        return nil;
    
    // attachments have the AXImage role, but a different subrole
    if (m_object->isAttachment())
        return [[self attachmentView] accessibilityAttributeValue:NSAccessibilityRoleDescriptionAttribute];
    
    NSString* axRole = [self role];
    
    if ([axRole isEqualToString:NSAccessibilityGroupRole]) {
        
        NSString *ariaLandmarkRoleDescription = [self ariaLandmarkRoleDescription];
        if (ariaLandmarkRoleDescription)
            return ariaLandmarkRoleDescription;
        
        switch (m_object->roleValue()) {
            case DefinitionRole:
                return AXDefinitionText();
            case DescriptionListTermRole:
                return AXDescriptionListTermText();
            case DescriptionListDetailRole:
                return AXDescriptionListDetailText();
            case FooterRole:
                return AXFooterRoleDescriptionText();
            default:
                return NSAccessibilityRoleDescription(NSAccessibilityGroupRole, [self subrole]);
        }
    }
    
    if ([axRole isEqualToString:@"AXWebArea"])
        return AXWebAreaText();
    
    if ([axRole isEqualToString:@"AXLink"])
        return AXLinkText();
    
    if ([axRole isEqualToString:@"AXListMarker"])
        return AXListMarkerText();
    
    if ([axRole isEqualToString:@"AXImageMap"])
        return AXImageMapText();
    
    if ([axRole isEqualToString:@"AXHeading"])
        return AXHeadingText();
    
    if (m_object->isFileUploadButton())
        return AXFileUploadButtonText();
    
    // Only returning for DL (not UL or OL) because description changed with HTML5 from 'definition list' to
    // superset 'description list' and does not return the same values in AX API on some OS versions. 
    if (m_object->isList()) {
        AccessibilityList* listObject = toAccessibilityList(m_object);
        if (listObject->isDescriptionList())
            return AXDescriptionListText();
    }
    
    // AppKit also returns AXTab for the role description for a tab item.
    if (m_object->isTabItem())
        return NSAccessibilityRoleDescription(@"AXTab", nil);
    
    // We should try the system default role description for all other roles.
    // If we get the same string back, then as a last resort, return unknown.
    NSString* defaultRoleDescription = NSAccessibilityRoleDescription(axRole, [self subrole]);
    
    // On earlier Mac versions (Lion), using a non-standard subrole would result in a role description
    // being returned that looked like AXRole:AXSubrole. To make all platforms have the same role descriptions
    // we should fallback on a role description ignoring the subrole in these cases.
    if ([defaultRoleDescription isEqualToString:[NSString stringWithFormat:@"%@:%@", axRole, [self subrole]]])
        defaultRoleDescription = NSAccessibilityRoleDescription(axRole, nil);
    
    if (![defaultRoleDescription isEqualToString:axRole])
        return defaultRoleDescription;
    
    return NSAccessibilityRoleDescription(NSAccessibilityUnknownRole, nil);
}

- (id)scrollViewParent
{
    if (!m_object || !m_object->isAccessibilityScrollView())
        return nil;
    
    // If this scroll view provides it's parent object (because it's a sub-frame), then
    // we should not find the remoteAccessibilityParent.
    if (m_object->parentObject())
        return nil;
    
    AccessibilityScrollView* scrollView = toAccessibilityScrollView(m_object);
    ScrollView* scroll = scrollView->scrollView();
    if (!scroll)
        return nil;
    
    if (scroll->platformWidget())
        return NSAccessibilityUnignoredAncestor(scroll->platformWidget());
    
    return [self remoteAccessibilityParentObject];
}

// FIXME: Different kinds of elements are putting the title tag to use in different
// AX fields. This should be rectified, but in the initial patch I want to achieve
// parity with existing behavior.
- (BOOL)titleTagShouldBeUsedInDescriptionField
{
    return (m_object->isLink() && !m_object->isImageMapLink()) || m_object->isImage();
}

// This should be the "visible" text that's actually on the screen if possible.
// If there's alternative text, that can override the title.
- (NSString *)accessibilityTitle
{
    // Static text objects should not have a title. Its content is communicated in its AXValue.
    if (m_object->roleValue() == StaticTextRole)
        return [NSString string];
    
    // A file upload button presents a challenge because it has button text and a value, but the
    // API doesn't support this paradigm.
    // The compromise is to return the button type in the role description and the value of the file path in the title
    if (m_object->isFileUploadButton())
        return m_object->stringValue();
    
    Vector<AccessibilityText> textOrder;
    m_object->accessibilityText(textOrder);
    
    unsigned length = textOrder.size();
    for (unsigned k = 0; k < length; k++) {
        const AccessibilityText& text = textOrder[k];
        
        // If we have alternative text, then we should not expose a title.
        if (text.textSource == AlternativeText)
            break;
        
        // Once we encounter visible text, or the text from our children that should be used foremost.
        if (text.textSource == VisibleText || text.textSource == ChildrenText)
            return text.text;
        
        // If there's an element that labels this object and it's not exposed, then we should use
        // that text as our title.
        if (text.textSource == LabelByElementText && !m_object->exposesTitleUIElement())
            return text.text;
        
        // FIXME: The title tag is used in certain cases for the title. This usage should
        // probably be in the description field since it's not "visible".
        if (text.textSource == TitleTagText && ![self titleTagShouldBeUsedInDescriptionField])
            return text.text;
    }
    
    return [NSString string];
}

- (NSString *)accessibilityDescription
{
    // Static text objects should not have a description. Its content is communicated in its AXValue.
    // One exception is the media control labels that have a value and a description. Those are set programatically.
    if (m_object->roleValue() == StaticTextRole && !m_object->isMediaControlLabel())
        return [NSString string];
    
    Vector<AccessibilityText> textOrder;
    m_object->accessibilityText(textOrder);
    
    unsigned length = textOrder.size();
    for (unsigned k = 0; k < length; k++) {
        const AccessibilityText& text = textOrder[k];
        
        if (text.textSource == AlternativeText)
            return text.text;
        
        if (text.textSource == TitleTagText && [self titleTagShouldBeUsedInDescriptionField])
            return text.text;
    }
    
    return [NSString string];
}

- (NSString *)accessibilityHelpText
{
    Vector<AccessibilityText> textOrder;
    m_object->accessibilityText(textOrder);
    
    unsigned length = textOrder.size();
    bool descriptiveTextAvailable = false;
    for (unsigned k = 0; k < length; k++) {
        const AccessibilityText& text = textOrder[k];
        
        if (text.textSource == HelpText || text.textSource == SummaryText)
            return text.text;
        
        // If an element does NOT have other descriptive text the title tag should be used as its descriptive text.
        // But, if those ARE available, then the title tag should be used for help text instead.
        switch (text.textSource) {
            case AlternativeText:
            case VisibleText:
            case ChildrenText:
            case LabelByElementText:
                descriptiveTextAvailable = true;
            default:
                break;
        }
        
        if (text.textSource == TitleTagText && descriptiveTextAvailable)
            return text.text;
    }
    
    return [NSString string];
}

// FIXME: split up this function in a better way.
// suggestions: Use a hash table that maps attribute names to function calls,
// or maybe pointers to member functions
- (id)accessibilityAttributeValue:(NSString*)attributeName
{
    if (![self updateObjectBackingStore])
        return nil;
    
    if ([attributeName isEqualToString: NSAccessibilityRoleAttribute])
        return [self role];
    
    if ([attributeName isEqualToString: NSAccessibilitySubroleAttribute])
        return [self subrole];
    
    if ([attributeName isEqualToString: NSAccessibilityRoleDescriptionAttribute])
        return [self roleDescription];
    
    if ([attributeName isEqualToString: NSAccessibilityParentAttribute]) {
        
        // This will return the parent of the AXWebArea, if this is a web area.
        id scrollViewParent = [self scrollViewParent];
        if (scrollViewParent)
            return scrollViewParent;
        
        // Tree item (changed to AXRows) can only report the tree (AXOutline) as its parent.
        if (m_object->isTreeItem()) {
            AccessibilityObject* parent = m_object->parentObjectUnignored();
            while (parent) {
                if (parent->isTree())
                    return parent->wrapper();
                parent = parent->parentObjectUnignored();
            }
        }
        
        AccessibilityObject* parent = m_object->parentObjectUnignored();
        if (!parent)
            return nil;
        
        // In WebKit1, the scroll view is provided by the system (the attachment view), so the parent
        // should be reported directly as such.
        if (m_object->isWebArea() && parent->isAttachment())
            return [parent->wrapper() attachmentView];
        
        return parent->wrapper();
    }
    
    if ([attributeName isEqualToString: NSAccessibilityChildrenAttribute]) {
        if (m_object->children().isEmpty()) {
            NSArray* children = [self renderWidgetChildren];
            if (children != nil)
                return children;
        }
        
        // The tree's (AXOutline) children are supposed to be its rows and columns.
        // The ARIA spec doesn't have columns, so we just need rows.
        if (m_object->isTree())
            return [self accessibilityAttributeValue:NSAccessibilityRowsAttribute];
        
        // A tree item should only expose its content as its children (not its rows)
        if (m_object->isTreeItem()) {
            AccessibilityObject::AccessibilityChildrenVector contentCopy;
            m_object->ariaTreeItemContent(contentCopy);
            return convertToNSArray(contentCopy);
        }
        
        return convertToNSArray(m_object->children());
    }
    
    if ([attributeName isEqualToString: NSAccessibilitySelectedChildrenAttribute]) {
        if (m_object->isListBox()) {
            AccessibilityObject::AccessibilityChildrenVector selectedChildrenCopy;
            m_object->selectedChildren(selectedChildrenCopy);
            return convertToNSArray(selectedChildrenCopy);
        }
        return nil;
    }
    
    if ([attributeName isEqualToString: NSAccessibilityVisibleChildrenAttribute]) {
        if (m_object->isListBox()) {
            AccessibilityObject::AccessibilityChildrenVector visibleChildrenCopy;
            m_object->visibleChildren(visibleChildrenCopy);
            return convertToNSArray(visibleChildrenCopy);
        }
        else if (m_object->isList())
            return [self accessibilityAttributeValue:NSAccessibilityChildrenAttribute];
        
        return nil;
    }
    
    
    if (m_object->isWebArea()) {
        if ([attributeName isEqualToString:@"AXLinkUIElements"]) {
            AccessibilityObject::AccessibilityChildrenVector links;
            static_cast<AccessibilityRenderObject*>(m_object)->getDocumentLinks(links);
            return convertToNSArray(links);
        }
        if ([attributeName isEqualToString:@"AXLoaded"])
            return [NSNumber numberWithBool:m_object->isLoaded()];
        if ([attributeName isEqualToString:@"AXLayoutCount"])
            return [NSNumber numberWithInt:m_object->layoutCount()];
        if ([attributeName isEqualToString:NSAccessibilityLoadingProgressAttribute])
            return [NSNumber numberWithDouble:m_object->estimatedLoadingProgress()];
    }
    
    if (m_object->isTextControl()) {
        if ([attributeName isEqualToString: NSAccessibilityNumberOfCharactersAttribute]) {
            int length = m_object->textLength();
            if (length < 0)
                return nil;
            return [NSNumber numberWithUnsignedInt:length];
        }
        if ([attributeName isEqualToString: NSAccessibilitySelectedTextAttribute]) {
            String selectedText = m_object->selectedText();
            if (selectedText.isNull())
                return nil;
            return (NSString*)selectedText;
        }
        if ([attributeName isEqualToString: NSAccessibilitySelectedTextRangeAttribute]) {
            PlainTextRange textRange = m_object->selectedTextRange();
            if (textRange.isNull())
                return [NSValue valueWithRange:NSMakeRange(0, 0)];
            return [NSValue valueWithRange:NSMakeRange(textRange.start, textRange.length)];
        }
        // TODO: Get actual visible range. <rdar://problem/4712101>
        if ([attributeName isEqualToString: NSAccessibilityVisibleCharacterRangeAttribute])
            return m_object->isPasswordField() ? nil : [NSValue valueWithRange: NSMakeRange(0, m_object->textLength())];
        if ([attributeName isEqualToString: NSAccessibilityInsertionPointLineNumberAttribute]) {
            // if selectionEnd > 0, then there is selected text and this question should not be answered
            if (m_object->isPasswordField() || m_object->selectionEnd() > 0)
                return nil;
            
            AccessibilityObject* focusedObject = m_object->focusedUIElement();
            if (focusedObject != m_object)
                return nil;
            
            VisiblePosition focusedPosition = focusedObject->visiblePositionForIndex(focusedObject->selectionStart(), true);
            int lineNumber = m_object->lineForPosition(focusedPosition);
            if (lineNumber < 0)
                return nil;
            
            return [NSNumber numberWithInt:lineNumber];
        }
    }
    
    if ([attributeName isEqualToString: NSAccessibilityURLAttribute]) {
        KURL url = m_object->url();
        if (url.isNull())
            return nil;
        return (NSURL*)url;
    }

    // Only native spin buttons have increment and decrement buttons.
    if (m_object->isNativeSpinButton()) {
        if ([attributeName isEqualToString:NSAccessibilityIncrementButtonAttribute])
            return toAccessibilitySpinButton(m_object)->incrementButton()->wrapper();
        if ([attributeName isEqualToString:NSAccessibilityDecrementButtonAttribute])
            return toAccessibilitySpinButton(m_object)->decrementButton()->wrapper();
    }
    
    if ([attributeName isEqualToString: @"AXVisited"])
        return [NSNumber numberWithBool: m_object->isVisited()];
    
    if ([attributeName isEqualToString: NSAccessibilityTitleAttribute]) {
        if (m_object->isAttachment()) {
            if ([[[self attachmentView] accessibilityAttributeNames] containsObject:NSAccessibilityTitleAttribute])
                return [[self attachmentView] accessibilityAttributeValue:NSAccessibilityTitleAttribute];
        }
        
        return [self accessibilityTitle];
    }
    
    if ([attributeName isEqualToString: NSAccessibilityDescriptionAttribute]) {
        if (m_object->isAttachment()) {
            if ([[[self attachmentView] accessibilityAttributeNames] containsObject:NSAccessibilityDescriptionAttribute])
                return [[self attachmentView] accessibilityAttributeValue:NSAccessibilityDescriptionAttribute];
        }
        return [self accessibilityDescription];
    }
    
    if ([attributeName isEqualToString: NSAccessibilityValueAttribute]) {
        if (m_object->isAttachment()) {
            if ([[[self attachmentView] accessibilityAttributeNames] containsObject:NSAccessibilityValueAttribute])
                return [[self attachmentView] accessibilityAttributeValue:NSAccessibilityValueAttribute];
        }
        if (m_object->supportsRangeValue())
            return [NSNumber numberWithFloat:m_object->valueForRange()];
        if (m_object->roleValue() == SliderThumbRole)
            return [NSNumber numberWithFloat:m_object->parentObject()->valueForRange()];
        if (m_object->isHeading())
            return [NSNumber numberWithInt:m_object->headingLevel()];
        
        if (m_object->isCheckboxOrRadio()) {
            switch (m_object->checkboxOrRadioValue()) {
                case ButtonStateOff:
                    return [NSNumber numberWithInt:0];
                case ButtonStateOn:
                    return [NSNumber numberWithInt:1];
                case ButtonStateMixed:
                    return [NSNumber numberWithInt:2];
            }
        }
        
        // radio groups return the selected radio button as the AXValue
        if (m_object->isRadioGroup()) {
            AccessibilityObject* radioButton = m_object->selectedRadioButton();
            if (!radioButton)
                return nil;
            return radioButton->wrapper();
        }
        
        if (m_object->isTabList()) {
            AccessibilityObject* tabItem = m_object->selectedTabItem();
            if (!tabItem)
                return nil;
            return tabItem->wrapper();
        }
        
        if (m_object->isTabItem())
            return [NSNumber numberWithInt:m_object->isSelected()];
        
        if (m_object->isColorWell()) {
            int r, g, b;
            m_object->colorValue(r, g, b);
            return [NSString stringWithFormat:@"rgb %7.5f %7.5f %7.5f 1", r / 255., g / 255., b / 255.];
        }
        
        return m_object->stringValue();
    }

    if ([attributeName isEqualToString:(NSString *)kAXMenuItemMarkCharAttribute]) {
        const unichar ch = 0x2713; // ✓ used on Mac for selected menu items.
        return (m_object->isChecked()) ? [NSString stringWithCharacters:&ch length:1] : nil;
    }

    if ([attributeName isEqualToString: NSAccessibilityMinValueAttribute])
        return [NSNumber numberWithFloat:m_object->minValueForRange()];
    
    if ([attributeName isEqualToString: NSAccessibilityMaxValueAttribute])
        return [NSNumber numberWithFloat:m_object->maxValueForRange()];
    
    if ([attributeName isEqualToString: NSAccessibilityHelpAttribute])
        return [self accessibilityHelpText];
    
    if ([attributeName isEqualToString: NSAccessibilityFocusedAttribute])
        return [NSNumber numberWithBool: m_object->isFocused()];
    
    if ([attributeName isEqualToString: NSAccessibilityEnabledAttribute])
        return [NSNumber numberWithBool: m_object->isEnabled()];
    
    if ([attributeName isEqualToString: NSAccessibilitySizeAttribute]) {
        IntSize s = m_object->pixelSnappedSize();
        return [NSValue valueWithSize: NSMakeSize(s.width(), s.height())];
    }
    
    if ([attributeName isEqualToString: NSAccessibilityPositionAttribute])
        return [self position];
    if ([attributeName isEqualToString:NSAccessibilityPathAttribute])
        return [self path];
    
    if ([attributeName isEqualToString: NSAccessibilityWindowAttribute] ||
        [attributeName isEqualToString: NSAccessibilityTopLevelUIElementAttribute]) {
        
        id remoteParent = [self remoteAccessibilityParentObject];
        if (remoteParent)
            return [remoteParent accessibilityAttributeValue:attributeName];
        
        FrameView* fv = m_object->documentFrameView();
        if (fv)
            return [fv->platformWidget() window];
        return nil;
    }
    
    if ([attributeName isEqualToString:NSAccessibilityAccessKeyAttribute]) {
        AtomicString accessKey = m_object->accessKey();
        if (accessKey.isNull())
            return nil;
        return accessKey;
    }
    
    if ([attributeName isEqualToString:NSAccessibilityTabsAttribute]) {
        if (m_object->isTabList()) {
            AccessibilityObject::AccessibilityChildrenVector tabsChildren;
            m_object->tabChildren(tabsChildren);
            return convertToNSArray(tabsChildren);
        }
    }
    
    if ([attributeName isEqualToString:NSAccessibilityContentsAttribute]) {
        // The contents of a tab list are all the children except the tabs.
        if (m_object->isTabList()) {
            AccessibilityObject::AccessibilityChildrenVector children = m_object->children();
            AccessibilityObject::AccessibilityChildrenVector tabsChildren;
            m_object->tabChildren(tabsChildren);
            
            AccessibilityObject::AccessibilityChildrenVector contents;
            unsigned childrenSize = children.size();
            for (unsigned k = 0; k < childrenSize; ++k) {
                if (tabsChildren.find(children[k]) == WTF::notFound)
                    contents.append(children[k]);
            }
            return convertToNSArray(contents);
        } else if (m_object->isScrollView()) {
            AccessibilityObject::AccessibilityChildrenVector children = m_object->children();
            
            // A scrollView's contents are everything except the scroll bars.
            AccessibilityObject::AccessibilityChildrenVector contents;
            unsigned childrenSize = children.size();
            for (unsigned k = 0; k < childrenSize; ++k) {
                if (!children[k]->isScrollbar())
                    contents.append(children[k]);
            }
            return convertToNSArray(contents);
        }
    }
    
    if (m_object->isAccessibilityTable()) {
        // TODO: distinguish between visible and non-visible rows
        if ([attributeName isEqualToString:NSAccessibilityRowsAttribute] ||
            [attributeName isEqualToString:NSAccessibilityVisibleRowsAttribute]) {
            return convertToNSArray(static_cast<AccessibilityTable*>(m_object)->rows());
        }
        // TODO: distinguish between visible and non-visible columns
        if ([attributeName isEqualToString:NSAccessibilityColumnsAttribute] ||
            [attributeName isEqualToString:NSAccessibilityVisibleColumnsAttribute]) {
            return convertToNSArray(static_cast<AccessibilityTable*>(m_object)->columns());
        }
        
        if ([attributeName isEqualToString:NSAccessibilitySelectedRowsAttribute]) {
            AccessibilityObject::AccessibilityChildrenVector selectedChildrenCopy;
            m_object->selectedChildren(selectedChildrenCopy);
            return convertToNSArray(selectedChildrenCopy);
        }
        
        // HTML tables don't support these
        if ([attributeName isEqualToString:NSAccessibilitySelectedColumnsAttribute] ||
            [attributeName isEqualToString:NSAccessibilitySelectedCellsAttribute])
            return nil;
        
        if ([attributeName isEqualToString:(NSString *)kAXColumnHeaderUIElementsAttribute]) {
            AccessibilityObject::AccessibilityChildrenVector columnHeaders;
            static_cast<AccessibilityTable*>(m_object)->columnHeaders(columnHeaders);
            return convertToNSArray(columnHeaders);
        }
        
        if ([attributeName isEqualToString:NSAccessibilityHeaderAttribute]) {
            AccessibilityObject* headerContainer = static_cast<AccessibilityTable*>(m_object)->headerContainer();
            if (headerContainer)
                return headerContainer->wrapper();
            return nil;
        }
        
        if ([attributeName isEqualToString:NSAccessibilityRowHeaderUIElementsAttribute]) {
            AccessibilityObject::AccessibilityChildrenVector rowHeaders;
            static_cast<AccessibilityTable*>(m_object)->rowHeaders(rowHeaders);
            return convertToNSArray(rowHeaders);
        }
        
        if ([attributeName isEqualToString:NSAccessibilityVisibleCellsAttribute]) {
            AccessibilityObject::AccessibilityChildrenVector cells;
            static_cast<AccessibilityTable*>(m_object)->cells(cells);
            return convertToNSArray(cells);
        }
    }
    
    if (m_object->isTableColumn()) {
        if ([attributeName isEqualToString:NSAccessibilityIndexAttribute])
            return [NSNumber numberWithInt:static_cast<AccessibilityTableColumn*>(m_object)->columnIndex()];
        
        // rows attribute for a column is the list of all the elements in that column at each row
        if ([attributeName isEqualToString:NSAccessibilityRowsAttribute] ||
            [attributeName isEqualToString:NSAccessibilityVisibleRowsAttribute]) {
            return convertToNSArray(static_cast<AccessibilityTableColumn*>(m_object)->children());
        }
        if ([attributeName isEqualToString:NSAccessibilityHeaderAttribute]) {
            AccessibilityObject* header = static_cast<AccessibilityTableColumn*>(m_object)->headerObject();
            if (!header)
                return nil;
            return header->wrapper();
        }
    }
    
    if (m_object->isTableCell()) {
        if ([attributeName isEqualToString:NSAccessibilityRowIndexRangeAttribute]) {
            pair<unsigned, unsigned> rowRange;
            static_cast<AccessibilityTableCell*>(m_object)->rowIndexRange(rowRange);
            return [NSValue valueWithRange:NSMakeRange(rowRange.first, rowRange.second)];
        }
        if ([attributeName isEqualToString:NSAccessibilityColumnIndexRangeAttribute]) {
            pair<unsigned, unsigned> columnRange;
            static_cast<AccessibilityTableCell*>(m_object)->columnIndexRange(columnRange);
            return [NSValue valueWithRange:NSMakeRange(columnRange.first, columnRange.second)];
        }
    }
    
    if (m_object->isTree()) {
        if ([attributeName isEqualToString:NSAccessibilitySelectedRowsAttribute]) {
            AccessibilityObject::AccessibilityChildrenVector selectedChildrenCopy;
            m_object->selectedChildren(selectedChildrenCopy);
            return convertToNSArray(selectedChildrenCopy);
        }
        if ([attributeName isEqualToString:NSAccessibilityRowsAttribute]) {
            AccessibilityObject::AccessibilityChildrenVector rowsCopy;
            m_object->ariaTreeRows(rowsCopy);
            return convertToNSArray(rowsCopy);
        }
        
        // TreeRoles do not support columns, but Mac AX expects to be able to ask about columns at the least.
        if ([attributeName isEqualToString:NSAccessibilityColumnsAttribute])
            return [NSArray array];
    }
    
    if ([attributeName isEqualToString:NSAccessibilityIndexAttribute]) {
        if (m_object->isTreeItem()) {
            AccessibilityObject* parent = m_object->parentObject();
            for (; parent && !parent->isTree(); parent = parent->parentObject())
            { }
            
            if (!parent)
                return nil;
            
            // Find the index of this item by iterating the parents.
            AccessibilityObject::AccessibilityChildrenVector rowsCopy;
            parent->ariaTreeRows(rowsCopy);
            size_t count = rowsCopy.size();
            for (size_t k = 0; k < count; ++k)
                if (rowsCopy[k]->wrapper() == self)
                    return [NSNumber numberWithUnsignedInt:k];
            
            return nil;
        }
        if (m_object->isTableRow()) {
            if ([attributeName isEqualToString:NSAccessibilityIndexAttribute])
                return [NSNumber numberWithInt:static_cast<AccessibilityTableRow*>(m_object)->rowIndex()];
        }
    }
    
    // The rows that are considered inside this row.
    if ([attributeName isEqualToString:NSAccessibilityDisclosedRowsAttribute]) {
        if (m_object->isTreeItem()) {
            AccessibilityObject::AccessibilityChildrenVector rowsCopy;
            m_object->ariaTreeItemDisclosedRows(rowsCopy);
            return convertToNSArray(rowsCopy);
        } else if (m_object->isARIATreeGridRow()) {
            AccessibilityObject::AccessibilityChildrenVector rowsCopy;
            static_cast<AccessibilityARIAGridRow*>(m_object)->disclosedRows(rowsCopy);
            return convertToNSArray(rowsCopy);
        }
    }
    
    // The row that contains this row. It should be the same as the first parent that is a treeitem.
    if ([attributeName isEqualToString:NSAccessibilityDisclosedByRowAttribute]) {
        if (m_object->isTreeItem()) {
            AccessibilityObject* parent = m_object->parentObject();
            while (parent) {
                if (parent->isTreeItem())
                    return parent->wrapper();
                // If the parent is the tree itself, then this value == nil.
                if (parent->isTree())
                    return nil;
                parent = parent->parentObject();
            }
            return nil;
        } else if (m_object->isARIATreeGridRow()) {
            AccessibilityObject* row = static_cast<AccessibilityARIAGridRow*>(m_object)->disclosedByRow();
            if (!row)
                return nil;
            return row->wrapper();
        }
    }
    
    if ([attributeName isEqualToString:NSAccessibilityDisclosureLevelAttribute]) {
        // Convert from 1-based level (from aria-level spec) to 0-based level (Mac)
        int level = m_object->hierarchicalLevel();
        if (level > 0)
            level -= 1;
        return [NSNumber numberWithInt:level];
    }
    if ([attributeName isEqualToString:NSAccessibilityDisclosingAttribute])
        return [NSNumber numberWithBool:m_object->isExpanded()];
    
    if ((m_object->isListBox() || m_object->isList()) && [attributeName isEqualToString:NSAccessibilityOrientationAttribute])
        return NSAccessibilityVerticalOrientationValue;
    
    if ([attributeName isEqualToString: @"AXSelectedTextMarkerRange"])
        return [self textMarkerRangeForSelection];
    
    if (m_object->renderer()) {
        if ([attributeName isEqualToString: @"AXStartTextMarker"])
            return [self textMarkerForVisiblePosition:startOfDocument(m_object->renderer()->document())];
        if ([attributeName isEqualToString: @"AXEndTextMarker"])
            return [self textMarkerForVisiblePosition:endOfDocument(m_object->renderer()->document())];
    }
    
    if ([attributeName isEqualToString:NSAccessibilityBlockQuoteLevelAttribute])
        return [NSNumber numberWithInt:m_object->blockquoteLevel()];
    if ([attributeName isEqualToString:@"AXTableLevel"])
        return [NSNumber numberWithInt:m_object->tableLevel()];
    
    if ([attributeName isEqualToString: NSAccessibilityLinkedUIElementsAttribute]) {
        AccessibilityObject::AccessibilityChildrenVector linkedUIElements;
        m_object->linkedUIElements(linkedUIElements);
        if (linkedUIElements.size() == 0)
            return nil;
        return convertToNSArray(linkedUIElements);
    }
    
    if ([attributeName isEqualToString: NSAccessibilitySelectedAttribute])
        return [NSNumber numberWithBool:m_object->isSelected()];
    
    if ([attributeName isEqualToString: NSAccessibilityServesAsTitleForUIElementsAttribute] && m_object->isMenuButton()) {
        AccessibilityObject* uiElement = static_cast<AccessibilityRenderObject*>(m_object)->menuForMenuButton();
        if (uiElement)
            return [NSArray arrayWithObject:uiElement->wrapper()];
    }
    
    if ([attributeName isEqualToString:NSAccessibilityTitleUIElementAttribute]) {
        if (!m_object->exposesTitleUIElement())
            return nil;
        
        AccessibilityObject* obj = m_object->titleUIElement();
        if (obj)
            return obj->wrapper();
        return nil;
    }
    
    if ([attributeName isEqualToString:NSAccessibilityValueDescriptionAttribute])
        return m_object->valueDescription();
    
    if ([attributeName isEqualToString:NSAccessibilityOrientationAttribute]) {
        AccessibilityOrientation elementOrientation = m_object->orientation();
        if (elementOrientation == AccessibilityOrientationVertical)
            return NSAccessibilityVerticalOrientationValue;
        if (elementOrientation == AccessibilityOrientationHorizontal)
            return NSAccessibilityHorizontalOrientationValue;
        return nil;
    }
    
    if ([attributeName isEqualToString:NSAccessibilityHorizontalScrollBarAttribute]) {
        AccessibilityObject* scrollBar = m_object->scrollBar(AccessibilityOrientationHorizontal);
        if (scrollBar)
            return scrollBar->wrapper();
        return nil;
    }
    if ([attributeName isEqualToString:NSAccessibilityVerticalScrollBarAttribute]) {
        AccessibilityObject* scrollBar = m_object->scrollBar(AccessibilityOrientationVertical);
        if (scrollBar)
            return scrollBar->wrapper();
        return nil;
    }
    
    if ([attributeName isEqualToString:NSAccessibilitySortDirectionAttribute]) {
        switch (m_object->sortDirection()) {
            case SortDirectionAscending:
                return NSAccessibilityAscendingSortDirectionValue;
            case SortDirectionDescending:
                return NSAccessibilityDescendingSortDirectionValue;
            default:
                return NSAccessibilityUnknownSortDirectionValue;
        }
    }
    
    if ([attributeName isEqualToString:NSAccessibilityLanguageAttribute])
        return m_object->language();
    
    if ([attributeName isEqualToString:NSAccessibilityExpandedAttribute])
        return [NSNumber numberWithBool:m_object->isExpanded()];
    
    if ([attributeName isEqualToString:NSAccessibilityRequiredAttribute])
        return [NSNumber numberWithBool:m_object->isRequired()];
    
    if ([attributeName isEqualToString:NSAccessibilityInvalidAttribute])
        return m_object->invalidStatus();
    
    if ([attributeName isEqualToString:NSAccessibilityOwnsAttribute]) {
        AccessibilityObject::AccessibilityChildrenVector ariaOwns;
        m_object->ariaOwnsElements(ariaOwns);
        return convertToNSArray(ariaOwns);
    }
    
    if ([attributeName isEqualToString:NSAccessibilityARIAPosInSetAttribute])
        return [NSNumber numberWithInt:m_object->ariaPosInSet()];
    if ([attributeName isEqualToString:NSAccessibilityARIASetSizeAttribute])
        return [NSNumber numberWithInt:m_object->ariaSetSize()];
    
    if ([attributeName isEqualToString:NSAccessibilityGrabbedAttribute])
        return [NSNumber numberWithBool:m_object->isARIAGrabbed()];
    
    if ([attributeName isEqualToString:NSAccessibilityDropEffectsAttribute]) {
        Vector<String> dropEffects;
        m_object->determineARIADropEffects(dropEffects);
        size_t length = dropEffects.size();
        
        NSMutableArray* dropEffectsArray = [NSMutableArray arrayWithCapacity:length];
        for (size_t i = 0; i < length; ++i)
            [dropEffectsArray addObject:dropEffects[i]];
        return dropEffectsArray;
    }
    
    if ([attributeName isEqualToString:NSAccessibilityPlaceholderValueAttribute])
        return m_object->placeholderValue();
    
    if ([attributeName isEqualToString:NSAccessibilityHasPopupAttribute])
        return [NSNumber numberWithBool:m_object->ariaHasPopup()];
    
    // ARIA Live region attributes.
    if ([attributeName isEqualToString:NSAccessibilityARIALiveAttribute])
        return m_object->ariaLiveRegionStatus();
    if ([attributeName isEqualToString:NSAccessibilityARIARelevantAttribute])
        return m_object->ariaLiveRegionRelevant();
    if ([attributeName isEqualToString:NSAccessibilityARIAAtomicAttribute])
        return [NSNumber numberWithBool:m_object->ariaLiveRegionAtomic()];
    if ([attributeName isEqualToString:NSAccessibilityARIABusyAttribute])
        return [NSNumber numberWithBool:m_object->ariaLiveRegionBusy()];
    
    // MathML Attributes.
    if (m_object->isMathElement()) {
        if ([attributeName isEqualToString:NSAccessibilityMathRootIndexAttribute])
            return (m_object->mathRootIndexObject()) ? m_object->mathRootIndexObject()->wrapper() : 0;
        if ([attributeName isEqualToString:NSAccessibilityMathRootRadicandAttribute])
            return (m_object->mathRadicandObject()) ? m_object->mathRadicandObject()->wrapper() : 0;
        if ([attributeName isEqualToString:NSAccessibilityMathFractionNumeratorAttribute])
            return (m_object->mathNumeratorObject()) ? m_object->mathNumeratorObject()->wrapper() : 0;
        if ([attributeName isEqualToString:NSAccessibilityMathFractionDenominatorAttribute])
            return (m_object->mathDenominatorObject()) ? m_object->mathDenominatorObject()->wrapper() : 0;
        if ([attributeName isEqualToString:NSAccessibilityMathBaseAttribute])
            return (m_object->mathBaseObject()) ? m_object->mathBaseObject()->wrapper() : 0;
        if ([attributeName isEqualToString:NSAccessibilityMathSubscriptAttribute])
            return (m_object->mathSubscriptObject()) ? m_object->mathSubscriptObject()->wrapper() : 0;
        if ([attributeName isEqualToString:NSAccessibilityMathSuperscriptAttribute])
            return (m_object->mathSuperscriptObject()) ? m_object->mathSuperscriptObject()->wrapper() : 0;
        if ([attributeName isEqualToString:NSAccessibilityMathUnderAttribute])
            return (m_object->mathUnderObject()) ? m_object->mathUnderObject()->wrapper() : 0;
        if ([attributeName isEqualToString:NSAccessibilityMathOverAttribute])
            return (m_object->mathOverObject()) ? m_object->mathOverObject()->wrapper() : 0;
        if ([attributeName isEqualToString:NSAccessibilityMathFencedOpenAttribute])
            return m_object->mathFencedOpenString();
        if ([attributeName isEqualToString:NSAccessibilityMathFencedCloseAttribute])
            return m_object->mathFencedCloseString();
        if ([attributeName isEqualToString:NSAccessibilityMathLineThicknessAttribute])
            return [NSNumber numberWithInteger:m_object->mathLineThickness()];
        if ([attributeName isEqualToString:NSAccessibilityMathPostscriptsAttribute])
            return [self accessibilityMathPostscriptPairs];
        if ([attributeName isEqualToString:NSAccessibilityMathPrescriptsAttribute])
            return [self accessibilityMathPrescriptPairs];
    }
    
    // this is used only by DumpRenderTree for testing
    if ([attributeName isEqualToString:@"AXClickPoint"])
        return [NSValue valueWithPoint:m_object->clickPoint()];
    
    // This is used by DRT to verify CSS3 speech works.
    if ([attributeName isEqualToString:@"AXDRTSpeechAttribute"]) {
        ESpeak speakProperty = m_object->speakProperty();
        switch (speakProperty) {
            case SpeakNone:
                return @"none";
            case SpeakSpellOut:
                return @"spell-out";
            case SpeakDigits:
                return @"digits";
            case SpeakLiteralPunctuation:
                return @"literal-punctuation";
            case SpeakNoPunctuation:
                return @"no-punctuation";
            default:
            case SpeakNormal:
                return @"normal";
        }
    }
    
    // Used by DRT to find an accessible node by its element id.
    if ([attributeName isEqualToString:@"AXDRTElementIdAttribute"])
        return m_object->getAttribute(idAttr);
    
    return nil;
}

- (NSString *)accessibilityPlatformMathSubscriptKey
{
    return NSAccessibilityMathSubscriptAttribute;
}

- (NSString *)accessibilityPlatformMathSuperscriptKey
{
    return NSAccessibilityMathSuperscriptAttribute;
}

- (id)accessibilityFocusedUIElement
{
    if (![self updateObjectBackingStore])
        return nil;
    
    RefPtr<AccessibilityObject> focusedObj = m_object->focusedUIElement();
    
    if (!focusedObj)
        return nil;
    
    return focusedObj->wrapper();
}

- (id)accessibilityHitTest:(NSPoint)point
{
    if (![self updateObjectBackingStore])
        return nil;
    
    m_object->updateChildrenIfNecessary();
    RefPtr<AccessibilityObject> axObject = m_object->accessibilityHitTest(IntPoint(point));
    if (axObject)
        return NSAccessibilityUnignoredAncestor(axObject->wrapper());
    return NSAccessibilityUnignoredAncestor(self);
}

- (BOOL)accessibilityIsAttributeSettable:(NSString*)attributeName
{
    if (![self updateObjectBackingStore])
        return nil;
    
    if ([attributeName isEqualToString: @"AXSelectedTextMarkerRange"])
        return YES;
    
    if ([attributeName isEqualToString: NSAccessibilityFocusedAttribute])
        return m_object->canSetFocusAttribute();
    
    if ([attributeName isEqualToString: NSAccessibilityValueAttribute])
        return m_object->canSetValueAttribute();
    
    if ([attributeName isEqualToString: NSAccessibilitySelectedAttribute])
        return m_object->canSetSelectedAttribute();
    
    if ([attributeName isEqualToString: NSAccessibilitySelectedChildrenAttribute])
        return m_object->canSetSelectedChildrenAttribute();
    
    if ([attributeName isEqualToString:NSAccessibilityDisclosingAttribute])
        return m_object->canSetExpandedAttribute();
    
    if ([attributeName isEqualToString:NSAccessibilitySelectedRowsAttribute])
        return YES;
    
    if ([attributeName isEqualToString: NSAccessibilitySelectedTextAttribute] ||
        [attributeName isEqualToString: NSAccessibilitySelectedTextRangeAttribute] ||
        [attributeName isEqualToString: NSAccessibilityVisibleCharacterRangeAttribute])
        return m_object->canSetTextRangeAttributes();
    
    if ([attributeName isEqualToString:NSAccessibilityGrabbedAttribute])
        return YES;
    
    return NO;
}

// accessibilityShouldUseUniqueId is an AppKit method we override so that
// objects will be given a unique ID, and therefore allow AppKit to know when they
// become obsolete (e.g. when the user navigates to a new web page, making this one
// unrendered but not deallocated because it is in the back/forward cache).
// It is important to call NSAccessibilityUnregisterUniqueIdForUIElement in the
// appropriate place (e.g. dealloc) to remove these non-retained references from
// AppKit's id mapping tables. We do this in detach by calling unregisterUniqueIdForUIElement.
//
// Registering an object is also required for observing notifications. Only registered objects can be observed.
- (BOOL)accessibilityIsIgnored
{
    if (![self updateObjectBackingStore])
        return YES;
    
    if (m_object->isAttachment())
        return [[self attachmentView] accessibilityIsIgnored];
    return m_object->accessibilityIsIgnored();
}

- (NSArray* )accessibilityParameterizedAttributeNames
{
    if (![self updateObjectBackingStore])
        return nil;
    
    if (m_object->isAttachment())
        return nil;
    
    static NSArray* paramAttrs = nil;
    static NSArray* textParamAttrs = nil;
    static NSArray* tableParamAttrs = nil;
    static NSArray* webAreaParamAttrs = nil;
    if (paramAttrs == nil) {
        paramAttrs = [[NSArray alloc] initWithObjects:
                      @"AXUIElementForTextMarker",
                      @"AXTextMarkerRangeForUIElement",
                      @"AXLineForTextMarker",
                      @"AXTextMarkerRangeForLine",
                      @"AXStringForTextMarkerRange",
                      @"AXTextMarkerForPosition",
                      @"AXBoundsForTextMarkerRange",
                      @"AXAttributedStringForTextMarkerRange",
                      @"AXTextMarkerRangeForUnorderedTextMarkers",
                      @"AXNextTextMarkerForTextMarker",
                      @"AXPreviousTextMarkerForTextMarker",
                      @"AXLeftWordTextMarkerRangeForTextMarker",
                      @"AXRightWordTextMarkerRangeForTextMarker",
                      @"AXLeftLineTextMarkerRangeForTextMarker",
                      @"AXRightLineTextMarkerRangeForTextMarker",
                      @"AXSentenceTextMarkerRangeForTextMarker",
                      @"AXParagraphTextMarkerRangeForTextMarker",
                      @"AXNextWordEndTextMarkerForTextMarker",
                      @"AXPreviousWordStartTextMarkerForTextMarker",
                      @"AXNextLineEndTextMarkerForTextMarker",
                      @"AXPreviousLineStartTextMarkerForTextMarker",
                      @"AXNextSentenceEndTextMarkerForTextMarker",
                      @"AXPreviousSentenceStartTextMarkerForTextMarker",
                      @"AXNextParagraphEndTextMarkerForTextMarker",
                      @"AXPreviousParagraphStartTextMarkerForTextMarker",
                      @"AXStyleTextMarkerRangeForTextMarker",
                      @"AXLengthForTextMarkerRange",
                      NSAccessibilityBoundsForRangeParameterizedAttribute,
                      NSAccessibilityStringForRangeParameterizedAttribute,
                      NSAccessibilityUIElementsForSearchPredicateParameterizedAttribute,
                      nil];
    }
    
    if (textParamAttrs == nil) {
        NSMutableArray* tempArray = [[NSMutableArray alloc] initWithArray:paramAttrs];
        [tempArray addObject:(NSString*)kAXLineForIndexParameterizedAttribute];
        [tempArray addObject:(NSString*)kAXRangeForLineParameterizedAttribute];
        [tempArray addObject:(NSString*)kAXStringForRangeParameterizedAttribute];
        [tempArray addObject:(NSString*)kAXRangeForPositionParameterizedAttribute];
        [tempArray addObject:(NSString*)kAXRangeForIndexParameterizedAttribute];
        [tempArray addObject:(NSString*)kAXBoundsForRangeParameterizedAttribute];
        [tempArray addObject:(NSString*)kAXRTFForRangeParameterizedAttribute];
        [tempArray addObject:(NSString*)kAXAttributedStringForRangeParameterizedAttribute];
        [tempArray addObject:(NSString*)kAXStyleRangeForIndexParameterizedAttribute];
        textParamAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (tableParamAttrs == nil) {
        NSMutableArray* tempArray = [[NSMutableArray alloc] initWithArray:paramAttrs];
        [tempArray addObject:NSAccessibilityCellForColumnAndRowParameterizedAttribute];
        tableParamAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    if (!webAreaParamAttrs) {
        NSMutableArray* tempArray = [[NSMutableArray alloc] initWithArray:paramAttrs];
        [tempArray addObject:NSAccessibilityTextMarkerForIndexParameterizedAttribute];
        [tempArray addObject:NSAccessibilityTextMarkerIsValidParameterizedAttribute];
        [tempArray addObject:NSAccessibilityIndexForTextMarkerParameterizedAttribute];
        webAreaParamAttrs = [[NSArray alloc] initWithArray:tempArray];
        [tempArray release];
    }
    
    if (m_object->isPasswordField())
        return [NSArray array];
    
    if (!m_object->isAccessibilityRenderObject())
        return paramAttrs;
    
    if (m_object->isTextControl())
        return textParamAttrs;
    
    if (m_object->isAccessibilityTable())
        return tableParamAttrs;
    
    if (m_object->isMenuRelated())
        return nil;
    
    if (m_object->isWebArea())
        return webAreaParamAttrs;
    
    return paramAttrs;
}

- (void)accessibilityPerformPressAction
{
    if (![self updateObjectBackingStore])
        return;
    
    if (m_object->isAttachment())
        [[self attachmentView] accessibilityPerformAction:NSAccessibilityPressAction];
    else
        m_object->press();
}

- (void)accessibilityPerformIncrementAction
{
    if (![self updateObjectBackingStore])
        return;
    
    if (m_object->isAttachment())
        [[self attachmentView] accessibilityPerformAction:NSAccessibilityIncrementAction];
    else
        m_object->increment();
}

- (void)accessibilityPerformDecrementAction
{
    if (![self updateObjectBackingStore])
        return;
    
    if (m_object->isAttachment())
        [[self attachmentView] accessibilityPerformAction:NSAccessibilityDecrementAction];
    else
        m_object->decrement();
}

- (void)accessibilityPerformShowMenuAction
{
    if (m_object->roleValue() == ComboBoxRole)
        m_object->setIsExpanded(true);
    else {
        // This needs to be performed in an iteration of the run loop that did not start from an AX call.
        // If it's the same run loop iteration, the menu open notification won't be sent
        [self performSelector:@selector(accessibilityShowContextMenu) withObject:nil afterDelay:0.0];
    }
}

- (void)accessibilityShowContextMenu
{
    Page* page = m_object->page();
    if (!page)
        return;
    
    IntRect rect = pixelSnappedIntRect(m_object->elementRect());
    FrameView* frameView = m_object->documentFrameView();
    
    // On WK2, we need to account for the scroll position.
    // On WK1, this isn't necessary, it's taken care of by the attachment views.
    if (frameView && !frameView->platformWidget()) {
        // Find the appropriate scroll view to use to convert the contents to the window.
        for (AccessibilityObject* parent = m_object->parentObject(); parent; parent = parent->parentObject()) {
            if (parent->isAccessibilityScrollView()) {
                ScrollView* scrollView = toAccessibilityScrollView(parent)->scrollView();
                rect = scrollView->contentsToRootView(rect);
                break;
            }
        }
    }
    
    page->contextMenuController()->showContextMenuAt(page->mainFrame(), rect.center());
}

- (void)accessibilityScrollToVisible
{
    m_object->scrollToMakeVisible();
}

- (void)accessibilityPerformAction:(NSString*)action
{
    if (![self updateObjectBackingStore])
        return;
    
    if ([action isEqualToString:NSAccessibilityPressAction])
        [self accessibilityPerformPressAction];
    
    else if ([action isEqualToString:NSAccessibilityShowMenuAction])
        [self accessibilityPerformShowMenuAction];
    
    else if ([action isEqualToString:NSAccessibilityIncrementAction])
        [self accessibilityPerformIncrementAction];
    
    else if ([action isEqualToString:NSAccessibilityDecrementAction])
        [self accessibilityPerformDecrementAction];
    
    else if ([action isEqualToString:NSAccessibilityScrollToVisibleAction])
        [self accessibilityScrollToVisible];
}

- (void)accessibilitySetValue:(id)value forAttribute:(NSString*)attributeName
{
    if (![self updateObjectBackingStore])
        return;
    
    id textMarkerRange = nil;
    NSNumber*               number = nil;
    NSString*               string = nil;
    NSRange                 range = {0, 0};
    NSArray*                array = nil;
    
    // decode the parameter
    if (AXObjectIsTextMarkerRange(value))
        textMarkerRange = value;
    
    else if ([value isKindOfClass:[NSNumber self]])
        number = value;
    
    else if ([value isKindOfClass:[NSString self]])
        string = value;
    
    else if ([value isKindOfClass:[NSValue self]])
        range = [value rangeValue];
    
    else if ([value isKindOfClass:[NSArray self]])
        array = value;
    
    // handle the command
    if ([attributeName isEqualToString: @"AXSelectedTextMarkerRange"]) {
        ASSERT(textMarkerRange);
        m_object->setSelectedVisiblePositionRange([self visiblePositionRangeForTextMarkerRange:textMarkerRange]);
    } else if ([attributeName isEqualToString: NSAccessibilityFocusedAttribute]) {
        ASSERT(number);
        
        bool focus = [number boolValue];
        
        // If focus is just set without making the view the first responder, then keyboard focus won't move to the right place.
        if (focus && m_object->isWebArea() && !m_object->document()->frame()->selection()->isFocusedAndActive()) {
            FrameView* frameView = m_object->documentFrameView();
            Page* page = m_object->page();
            if (page && frameView) {
                ChromeClient* client = page->chrome().client();
                client->focus();
                if (frameView->platformWidget())
                    client->makeFirstResponder(frameView->platformWidget());
                else
                    client->makeFirstResponder();
            }
        }
        
        m_object->setFocused(focus);
    } else if ([attributeName isEqualToString: NSAccessibilityValueAttribute]) {
        if (number && m_object->canSetNumericValue())
            m_object->setValue([number floatValue]);
        else if (string)
            m_object->setValue(string);
    } else if ([attributeName isEqualToString: NSAccessibilitySelectedAttribute]) {
        if (!number)
            return;
        m_object->setSelected([number boolValue]);
    } else if ([attributeName isEqualToString: NSAccessibilitySelectedChildrenAttribute]) {
        if (!array || m_object->roleValue() != ListBoxRole)
            return;
        AccessibilityObject::AccessibilityChildrenVector selectedChildren;
        convertToVector(array, selectedChildren);
        static_cast<AccessibilityListBox*>(m_object)->setSelectedChildren(selectedChildren);
    } else if (m_object->isTextControl()) {
        if ([attributeName isEqualToString: NSAccessibilitySelectedTextAttribute]) {
            m_object->setSelectedText(string);
        } else if ([attributeName isEqualToString: NSAccessibilitySelectedTextRangeAttribute]) {
            m_object->setSelectedTextRange(PlainTextRange(range.location, range.length));
        } else if ([attributeName isEqualToString: NSAccessibilityVisibleCharacterRangeAttribute]) {
            m_object->makeRangeVisible(PlainTextRange(range.location, range.length));
        }
    } else if ([attributeName isEqualToString:NSAccessibilityDisclosingAttribute])
        m_object->setIsExpanded([number boolValue]);
    else if ([attributeName isEqualToString:NSAccessibilitySelectedRowsAttribute]) {
        AccessibilityObject::AccessibilityChildrenVector selectedRows;
        convertToVector(array, selectedRows);
        if (m_object->isTree() || m_object->isAccessibilityTable())
            m_object->setSelectedRows(selectedRows);
    } else if ([attributeName isEqualToString:NSAccessibilityGrabbedAttribute])
        m_object->setARIAGrabbed([number boolValue]);
}

static RenderObject* rendererForView(NSView* view)
{
    if (![view conformsToProtocol:@protocol(WebCoreFrameView)])
        return 0;
    
    NSView<WebCoreFrameView>* frameView = (NSView<WebCoreFrameView>*)view;
    Frame* frame = [frameView _web_frame];
    if (!frame)
        return 0;
    
    Node* node = frame->document()->ownerElement();
    if (!node)
        return 0;
    
    return node->renderer();
}

- (id)_accessibilityParentForSubview:(NSView*)subview
{
    RenderObject* renderer = rendererForView(subview);
    if (!renderer)
        return nil;
    
    AccessibilityObject* obj = renderer->document()->axObjectCache()->getOrCreate(renderer);
    if (obj)
        return obj->parentObjectUnignored()->wrapper();
    return nil;
}

- (NSString*)accessibilityActionDescription:(NSString*)action
{
    // we have no custom actions
    return NSAccessibilityActionDescription(action);
}

// The CFAttributedStringType representation of the text associated with this accessibility
// object that is specified by the given range.
- (NSAttributedString*)doAXAttributedStringForRange:(NSRange)range
{
    PlainTextRange textRange = PlainTextRange(range.location, range.length);
    VisiblePositionRange visiblePosRange = m_object->visiblePositionRangeForRange(textRange);
    return [self doAXAttributedStringForTextMarkerRange:[self textMarkerRangeFromVisiblePositions:visiblePosRange.start endPosition:visiblePosRange.end]];
}

- (NSRange)_convertToNSRange:(Range*)range
{
    NSRange result = NSMakeRange(NSNotFound, 0);
    if (!range || !range->startContainer())
        return result;
    
    Document* document = m_object->document();
    if (!document)
        return result;
    
    size_t location;
    size_t length;
    TextIterator::getLocationAndLengthFromRange(document->documentElement(), range, location, length);
    result.location = location;
    result.length = length;
    
    return result;
}

- (NSInteger)_indexForTextMarker:(id)marker
{
    if (!marker)
        return NSNotFound;
    
    VisibleSelection selection([self visiblePositionForTextMarker:marker]);
    return [self _convertToNSRange:selection.toNormalizedRange().get()].location;
}

- (id)_textMarkerForIndex:(NSInteger)textIndex
{
    Document* document = m_object->document();
    if (!document)
        return nil;
    
    PassRefPtr<Range> textRange = TextIterator::rangeFromLocationAndLength(document->documentElement(), textIndex, 0);
    if (!textRange || !textRange->boundaryPointsValid())
        return nil;
    
    VisiblePosition position(textRange->startPosition());
    return [self textMarkerForVisiblePosition:position];
}

// The RTF representation of the text associated with this accessibility object that is
// specified by the given range.
- (NSData*)doAXRTFForRange:(NSRange)range
{
    NSAttributedString* attrString = [self doAXAttributedStringForRange:range];
    return [attrString RTFFromRange: NSMakeRange(0, [attrString length]) documentAttributes: nil];
}

- (id)accessibilityAttributeValue:(NSString*)attribute forParameter:(id)parameter
{
    id textMarker = nil;
    id textMarkerRange = nil;
    NSNumber* number = nil;
    NSArray* array = nil;
    NSDictionary* dictionary = nil;
    RefPtr<AccessibilityObject> uiElement = 0;
    NSPoint point = NSZeroPoint;
    bool pointSet = false;
    NSRange range = {0, 0};
    bool rangeSet = false;
    
    // basic parameter validation
    if (!m_object || !attribute || !parameter)
        return nil;
    
    if (![self updateObjectBackingStore])
        return nil;
    
    // common parameter type check/casting.  Nil checks in handlers catch wrong type case.
    // NOTE: This assumes nil is not a valid parameter, because it is indistinguishable from
    // a parameter of the wrong type.
    if (AXObjectIsTextMarker(parameter))
        textMarker = parameter;
    
    else if (AXObjectIsTextMarkerRange(parameter))
        textMarkerRange = parameter;
    
    else if ([parameter isKindOfClass:[WebAccessibilityObjectWrapper self]])
        uiElement = [(WebAccessibilityObjectWrapper*)parameter accessibilityObject];
    
    else if ([parameter isKindOfClass:[NSNumber self]])
        number = parameter;
    
    else if ([parameter isKindOfClass:[NSArray self]])
        array = parameter;
    
    else if ([parameter isKindOfClass:[NSDictionary self]])
        dictionary = parameter;
    
    else if ([parameter isKindOfClass:[NSValue self]] && strcmp([(NSValue*)parameter objCType], @encode(NSPoint)) == 0) {
        pointSet = true;
        point = [(NSValue*)parameter pointValue];
        
    } else if ([parameter isKindOfClass:[NSValue self]] && strcmp([(NSValue*)parameter objCType], @encode(NSRange)) == 0) {
        rangeSet = true;
        range = [(NSValue*)parameter rangeValue];
    } else {
        // Attribute type is not supported. Allow super to handle.
        return [super accessibilityAttributeValue:attribute forParameter:parameter];
    }
    
    // dispatch
    if ([attribute isEqualToString:NSAccessibilityUIElementsForSearchPredicateParameterizedAttribute]) {
        AccessibilityObject* startObject = 0;
        if ([[dictionary objectForKey:@"AXStartElement"] isKindOfClass:[WebAccessibilityObjectWrapper self]])
            startObject = [(WebAccessibilityObjectWrapper*)[dictionary objectForKey:@"AXStartElement"] accessibilityObject];
        
        AccessibilitySearchDirection searchDirection = SearchDirectionNext;
        if ([[dictionary objectForKey:@"AXDirection"] isKindOfClass:[NSString self]])
            searchDirection = ([(NSString*)[dictionary objectForKey:@"AXDirection"] isEqualToString:@"AXDirectionNext"]) ? SearchDirectionNext : SearchDirectionPrevious;

        String searchText;
        if ([[dictionary objectForKey:@"AXSearchText"] isKindOfClass:[NSString self]])
            searchText = (CFStringRef)[dictionary objectForKey:@"AXSearchText"];
        
        unsigned resultsLimit = 0;
        if ([[dictionary objectForKey:@"AXResultsLimit"] isKindOfClass:[NSNumber self]])
            resultsLimit = [(NSNumber*)[dictionary objectForKey:@"AXResultsLimit"] unsignedIntValue];
        
        BOOL visibleOnly = NO;
        if ([[dictionary objectForKey:@"AXVisibleOnly"] isKindOfClass:[NSNumber self]])
            visibleOnly = [(NSNumber*)[dictionary objectForKey:@"AXVisibleOnly"] boolValue];
        
        AccessibilitySearchCriteria criteria = AccessibilitySearchCriteria(startObject, searchDirection, &searchText, resultsLimit, visibleOnly);
                
        id searchKeyEntry = [dictionary objectForKey:@"AXSearchKey"];
        if ([searchKeyEntry isKindOfClass:[NSString class]])
            criteria.searchKeys.append(accessibilitySearchKeyForString((CFStringRef)searchKeyEntry));
        else if ([searchKeyEntry isKindOfClass:[NSArray class]]) {
            size_t length = static_cast<size_t>([(NSArray *)searchKeyEntry count]);
            criteria.searchKeys.reserveInitialCapacity(length);
            for (size_t i = 0; i < length; ++i) {
                id searchKey = [(NSArray *)searchKeyEntry objectAtIndex:i];
                if ([searchKey isKindOfClass:[NSString class]])
                    criteria.searchKeys.append(accessibilitySearchKeyForString((CFStringRef)searchKey));
            }
        }
        
        AccessibilityObject::AccessibilityChildrenVector results;
        m_object->findMatchingObjects(&criteria, results);
        
        return convertToNSArray(results);
    }
    
    if ([attribute isEqualToString:NSAccessibilityTextMarkerIsValidParameterizedAttribute]) {
        VisiblePosition pos = [self visiblePositionForTextMarker:textMarker];
        return [NSNumber numberWithBool:!pos.isNull()];
    }
    if ([attribute isEqualToString:NSAccessibilityIndexForTextMarkerParameterizedAttribute]) {
        return [NSNumber numberWithInteger:[self _indexForTextMarker:textMarker]];
    }
    if ([attribute isEqualToString:NSAccessibilityTextMarkerForIndexParameterizedAttribute]) {
        return [self _textMarkerForIndex:[number integerValue]];
    }
    
    if ([attribute isEqualToString:@"AXUIElementForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        AccessibilityObject* axObject = m_object->accessibilityObjectForPosition(visiblePos);
        if (!axObject)
            return nil;
        return axObject->wrapper();
    }
    
    if ([attribute isEqualToString:@"AXTextMarkerRangeForUIElement"]) {
        VisiblePositionRange vpRange = uiElement.get()->visiblePositionRange();
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXLineForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [NSNumber numberWithUnsignedInt:m_object->lineForPosition(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXTextMarkerRangeForLine"]) {
        VisiblePositionRange vpRange = m_object->visiblePositionRangeForLine([number intValue]);
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXStringForTextMarkerRange"]) {
        VisiblePositionRange visiblePosRange = [self visiblePositionRangeForTextMarkerRange:textMarkerRange];
        return m_object->stringForVisiblePositionRange(visiblePosRange);
    }
    
    if ([attribute isEqualToString:@"AXTextMarkerForPosition"]) {
        IntPoint webCorePoint = IntPoint(point);
        return pointSet ? [self textMarkerForVisiblePosition:m_object->visiblePositionForPoint(webCorePoint)] : nil;
    }
    
    if ([attribute isEqualToString:@"AXBoundsForTextMarkerRange"]) {
        VisiblePositionRange visiblePosRange = [self visiblePositionRangeForTextMarkerRange:textMarkerRange];
        NSRect rect = m_object->boundsForVisiblePositionRange(visiblePosRange);
        return [NSValue valueWithRect:rect];
    }
    
    if ([attribute isEqualToString:NSAccessibilityBoundsForRangeParameterizedAttribute]) {
        VisiblePosition start = m_object->visiblePositionForIndex(range.location);
        VisiblePosition end = m_object->visiblePositionForIndex(range.location+range.length);
        if (start.isNull() || end.isNull())
            return nil;
        NSRect rect = m_object->boundsForVisiblePositionRange(VisiblePositionRange(start, end));
        return [NSValue valueWithRect:rect];
    }
    
    if ([attribute isEqualToString:NSAccessibilityStringForRangeParameterizedAttribute]) {
        VisiblePosition start = m_object->visiblePositionForIndex(range.location);
        VisiblePosition end = m_object->visiblePositionForIndex(range.location+range.length);
        if (start.isNull() || end.isNull())
            return nil;
        return m_object->stringForVisiblePositionRange(VisiblePositionRange(start, end));
    }
    
    if ([attribute isEqualToString:@"AXAttributedStringForTextMarkerRange"])
        return [self doAXAttributedStringForTextMarkerRange:textMarkerRange];
    
    if ([attribute isEqualToString:@"AXTextMarkerRangeForUnorderedTextMarkers"]) {
        if ([array count] < 2)
            return nil;
        
        id textMarker1 = [array objectAtIndex:0];
        id textMarker2 = [array objectAtIndex:1];
        if (!AXObjectIsTextMarker(textMarker1) || !AXObjectIsTextMarker(textMarker2))
            return nil;
        
        VisiblePosition visiblePos1 = [self visiblePositionForTextMarker:(textMarker1)];
        VisiblePosition visiblePos2 = [self visiblePositionForTextMarker:(textMarker2)];
        VisiblePositionRange vpRange = m_object->visiblePositionRangeForUnorderedPositions(visiblePos1, visiblePos2);
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXNextTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->nextVisiblePosition(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXPreviousTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->previousVisiblePosition(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXLeftWordTextMarkerRangeForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        VisiblePositionRange vpRange = m_object->positionOfLeftWord(visiblePos);
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXRightWordTextMarkerRangeForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        VisiblePositionRange vpRange = m_object->positionOfRightWord(visiblePos);
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXLeftLineTextMarkerRangeForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        VisiblePositionRange vpRange = m_object->leftLineVisiblePositionRange(visiblePos);
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXRightLineTextMarkerRangeForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        VisiblePositionRange vpRange = m_object->rightLineVisiblePositionRange(visiblePos);
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXSentenceTextMarkerRangeForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        VisiblePositionRange vpRange = m_object->sentenceForPosition(visiblePos);
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXParagraphTextMarkerRangeForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        VisiblePositionRange vpRange = m_object->paragraphForPosition(visiblePos);
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXNextWordEndTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->nextWordEnd(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXPreviousWordStartTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->previousWordStart(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXNextLineEndTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->nextLineEndPosition(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXPreviousLineStartTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->previousLineStartPosition(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXNextSentenceEndTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->nextSentenceEndPosition(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXPreviousSentenceStartTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->previousSentenceStartPosition(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXNextParagraphEndTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->nextParagraphEndPosition(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXPreviousParagraphStartTextMarkerForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        return [self textMarkerForVisiblePosition:m_object->previousParagraphStartPosition(visiblePos)];
    }
    
    if ([attribute isEqualToString:@"AXStyleTextMarkerRangeForTextMarker"]) {
        VisiblePosition visiblePos = [self visiblePositionForTextMarker:(textMarker)];
        VisiblePositionRange vpRange = m_object->styleRangeForPosition(visiblePos);
        return [self textMarkerRangeFromVisiblePositions:vpRange.start endPosition:vpRange.end];
    }
    
    if ([attribute isEqualToString:@"AXLengthForTextMarkerRange"]) {
        VisiblePositionRange visiblePosRange = [self visiblePositionRangeForTextMarkerRange:textMarkerRange];
        int length = m_object->lengthForVisiblePositionRange(visiblePosRange);
        if (length < 0)
            return nil;
        return [NSNumber numberWithInt:length];
    }
    
    // Used only by DumpRenderTree (so far).
    if ([attribute isEqualToString:@"AXStartTextMarkerForTextMarkerRange"]) {
        VisiblePositionRange visiblePosRange = [self visiblePositionRangeForTextMarkerRange:textMarkerRange];
        return [self textMarkerForVisiblePosition:visiblePosRange.start];
    }
    
    if ([attribute isEqualToString:@"AXEndTextMarkerForTextMarkerRange"]) {
        VisiblePositionRange visiblePosRange = [self visiblePositionRangeForTextMarkerRange:textMarkerRange];
        return [self textMarkerForVisiblePosition:visiblePosRange.end];
    }
    
    if (m_object->isAccessibilityTable()) {
        if ([attribute isEqualToString:NSAccessibilityCellForColumnAndRowParameterizedAttribute]) {
            if (array == nil || [array count] != 2)
                return nil;
            AccessibilityTableCell* cell = static_cast<AccessibilityTable*>(m_object)->cellForColumnAndRow([[array objectAtIndex:0] unsignedIntValue], [[array objectAtIndex:1] unsignedIntValue]);
            if (!cell)
                return nil;
            
            return cell->wrapper();
        }
    }
    
    if (m_object->isTextControl()) {
        if ([attribute isEqualToString: (NSString *)kAXLineForIndexParameterizedAttribute]) {
            int lineNumber = m_object->doAXLineForIndex([number intValue]);
            if (lineNumber < 0)
                return nil;
            return [NSNumber numberWithUnsignedInt:lineNumber];
        }
        
        if ([attribute isEqualToString: (NSString *)kAXRangeForLineParameterizedAttribute]) {
            PlainTextRange textRange = m_object->doAXRangeForLine([number intValue]);
            return [NSValue valueWithRange: NSMakeRange(textRange.start, textRange.length)];
        }
        
        if ([attribute isEqualToString: (NSString*)kAXStringForRangeParameterizedAttribute]) {
            PlainTextRange plainTextRange = PlainTextRange(range.location, range.length);
            return rangeSet ? (id)(m_object->doAXStringForRange(plainTextRange)) : nil;
        }
        
        if ([attribute isEqualToString: (NSString*)kAXRangeForPositionParameterizedAttribute]) {
            if (!pointSet)
                return nil;
            IntPoint webCorePoint = IntPoint(point);
            PlainTextRange textRange = m_object->doAXRangeForPosition(webCorePoint);
            return [NSValue valueWithRange: NSMakeRange(textRange.start, textRange.length)];
        }
        
        if ([attribute isEqualToString: (NSString*)kAXRangeForIndexParameterizedAttribute]) {
            PlainTextRange textRange = m_object->doAXRangeForIndex([number intValue]);
            return [NSValue valueWithRange: NSMakeRange(textRange.start, textRange.length)];
        }
        
        if ([attribute isEqualToString: (NSString*)kAXBoundsForRangeParameterizedAttribute]) {
            if (!rangeSet)
                return nil;
            PlainTextRange plainTextRange = PlainTextRange(range.location, range.length);
            NSRect rect = m_object->doAXBoundsForRange(plainTextRange);
            return [NSValue valueWithRect:rect];
        }
        
        if ([attribute isEqualToString: (NSString*)kAXRTFForRangeParameterizedAttribute])
            return rangeSet ? [self doAXRTFForRange:range] : nil;
        
        if ([attribute isEqualToString: (NSString*)kAXAttributedStringForRangeParameterizedAttribute])
            return rangeSet ? [self doAXAttributedStringForRange:range] : nil;
        
        if ([attribute isEqualToString: (NSString*)kAXStyleRangeForIndexParameterizedAttribute]) {
            PlainTextRange textRange = m_object->doAXStyleRangeForIndex([number intValue]);
            return [NSValue valueWithRange: NSMakeRange(textRange.start, textRange.length)];
        }
    }
    
    // There are some parameters that super handles that are not explicitly returned by the list of the element's attributes.
    // In that case it must be passed to super.
    return [super accessibilityAttributeValue:attribute forParameter:parameter];
}

- (BOOL)accessibilitySupportsOverriddenAttributes
{
    return YES;
}

- (BOOL)accessibilityShouldUseUniqueId
{
    // All AX object wrappers should use unique ID's because it's faster within AppKit to look them up.
    return YES;
}

// API that AppKit uses for faster access
- (NSUInteger)accessibilityIndexOfChild:(id)child
{
    if (![self updateObjectBackingStore])
        return NSNotFound;
    
    // Tree objects return their rows as their children. We can use the original method
    // here, because we won't gain any speed up.
    if (m_object->isTree())
        return [super accessibilityIndexOfChild:child];
    
    const AccessibilityObject::AccessibilityChildrenVector& children = m_object->children();
    
    if (children.isEmpty())
        return [[self renderWidgetChildren] indexOfObject:child];
    
    unsigned count = children.size();
    for (unsigned k = 0; k < count; ++k) {
        WebAccessibilityObjectWrapper* wrapper = children[k]->wrapper();
        if (wrapper == child || (children[k]->isAttachment() && [wrapper attachmentView] == child))
            return k;
    }
    
    return NSNotFound;
}

- (NSUInteger)accessibilityArrayAttributeCount:(NSString *)attribute
{
    if (![self updateObjectBackingStore])
        return 0;
    
    if ([attribute isEqualToString:NSAccessibilityChildrenAttribute]) {
        // Tree items object returns a different set of children than those that are in children()
        // because an AXOutline (the mac role is becomes) has some odd stipulations.
        if (m_object->isTree() || m_object->isTreeItem())
            return [[self accessibilityAttributeValue:NSAccessibilityChildrenAttribute] count];
        
        const AccessibilityObject::AccessibilityChildrenVector& children = m_object->children();
        if (children.isEmpty())
            return [[self renderWidgetChildren] count];
        
        return children.size();
    }
    
    return [super accessibilityArrayAttributeCount:attribute];
}

- (NSArray *)accessibilityArrayAttributeValues:(NSString *)attribute index:(NSUInteger)index maxCount:(NSUInteger)maxCount
{
    if (![self updateObjectBackingStore])
        return nil;
    
    if ([attribute isEqualToString:NSAccessibilityChildrenAttribute]) {
        if (m_object->children().isEmpty()) {
            NSArray *children = [self renderWidgetChildren];
            if (!children)
                return nil;
            
            NSUInteger childCount = [children count];
            if (index >= childCount)
                return nil;
            
            NSUInteger arrayLength = min(childCount - index, maxCount);
            return [children subarrayWithRange:NSMakeRange(index, arrayLength)];
        } else if (m_object->isTree()) {
            // Tree objects return their rows as their children. We can use the original method in this case.
            return [super accessibilityArrayAttributeValues:attribute index:index maxCount:maxCount];
        }
        
        const AccessibilityObject::AccessibilityChildrenVector& children = m_object->children();
        unsigned childCount = children.size();
        if (index >= childCount)
            return nil;
        
        unsigned available = min(childCount - index, maxCount);
        
        NSMutableArray *subarray = [NSMutableArray arrayWithCapacity:available];
        for (unsigned added = 0; added < available; ++index, ++added) {
            WebAccessibilityObjectWrapper* wrapper = children[index]->wrapper();
            if (wrapper) {
                // The attachment view should be returned, otherwise AX palindrome errors occur.
                if (children[index]->isAttachment() && [wrapper attachmentView])
                    [subarray addObject:[wrapper attachmentView]];
                else
                    [subarray addObject:wrapper];
            }
        }
        
        return subarray;
    }
    
    return [super accessibilityArrayAttributeValues:attribute index:index maxCount:maxCount];
}

@end

#endif // HAVE(ACCESSIBILITY)

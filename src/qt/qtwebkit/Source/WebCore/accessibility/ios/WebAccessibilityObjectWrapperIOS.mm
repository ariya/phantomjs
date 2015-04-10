/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
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
#import "WebAccessibilityObjectWrapperIOS.h"

#if HAVE(ACCESSIBILITY) && PLATFORM(IOS)

#import "AccessibilityRenderObject.h"
#import "AccessibilityTable.h"
#import "AccessibilityTableCell.h"
#import "Font.h"
#import "Frame.h"
#import "FrameSelection.h"
#import "FrameView.h"
#import "HitTestResult.h"
#import "HTMLFrameOwnerElement.h"
#import "HTMLInputElement.h"
#import "HTMLNames.h"
#import "IntRect.h"
#import "IntSize.h"
#import "Range.h"
#import "RenderView.h"
#import "RuntimeApplicationChecksIOS.h"
#import "SVGNames.h"
#import "TextIterator.h"
#import "WAKScrollView.h"
#import "WAKView.h"
#import "WAKWindow.h"
#import "WebCoreThread.h"
#import "visible_units.h"

#import <GraphicsServices/GraphicsServices.h>

@interface NSObject (AccessibilityPrivate)
- (void)_accessibilityUnregister;
- (NSString *)accessibilityLabel;
- (NSString *)accessibilityValue;
- (BOOL)isAccessibilityElement;
- (NSInteger)accessibilityElementCount;
- (id)accessibilityElementAtIndex:(NSInteger)index;
- (NSInteger)indexOfAccessibilityElement:(id)element;
@end

@interface WebAccessibilityObjectWrapper (AccessibilityPrivate)
- (id)_accessibilityWebDocumentView;
- (id)accessibilityContainer;
- (void)setAccessibilityLabel:(NSString *)label;
- (void)setAccessibilityValue:(NSString *)value;
- (BOOL)containsUnnaturallySegmentedChildren;
- (NSInteger)positionForTextMarker:(id)marker;
@end

@interface WAKView (iOSAccessibility)
- (BOOL)accessibilityIsIgnored;
@end

using namespace WebCore;
using namespace HTMLNames;

// These are tokens accessibility uses to denote attributes. 
static NSString * const UIAccessibilityTokenBlockquoteLevel = @"UIAccessibilityTokenBlockquoteLevel";
static NSString * const UIAccessibilityTokenHeadingLevel = @"UIAccessibilityTokenHeadingLevel";
static NSString * const UIAccessibilityTokenFontName = @"UIAccessibilityTokenFontName";
static NSString * const UIAccessibilityTokenFontFamily = @"UIAccessibilityTokenFontFamily";
static NSString * const UIAccessibilityTokenFontSize = @"UIAccessibilityTokenFontSize";
static NSString * const UIAccessibilityTokenBold = @"UIAccessibilityTokenBold";
static NSString * const UIAccessibilityTokenItalic = @"UIAccessibilityTokenItalic";
static NSString * const UIAccessibilityTokenUnderline = @"UIAccessibilityTokenUnderline";

static AccessibilityObjectWrapper* AccessibilityUnignoredAncestor(AccessibilityObjectWrapper *wrapper)
{
    while (wrapper && ![wrapper isAccessibilityElement]) {
        AccessibilityObject* object = [wrapper accessibilityObject];
        if (!object)
            break;
        
        if ([wrapper isAttachment] && ![[wrapper attachmentView] accessibilityIsIgnored])
            break;
            
        AccessibilityObject* parentObject = object->parentObjectUnignored();
        if (!parentObject)
            break;

        wrapper = parentObject->wrapper();
    }
    return wrapper;
}

#pragma mark Accessibility Text Marker

@interface WebAccessibilityTextMarker : NSObject
{
    AXObjectCache* _cache;
    TextMarkerData _textMarkerData;
}

+ (WebAccessibilityTextMarker *)textMarkerWithVisiblePosition:(VisiblePosition&)visiblePos cache:(AXObjectCache*)cache;

@end

@implementation WebAccessibilityTextMarker

- (id)initWithTextMarker:(TextMarkerData *)data cache:(AXObjectCache*)cache
{
    if (!(self = [super init]))
        return nil;
    
    _cache = cache;
    memcpy(&_textMarkerData, data, sizeof(TextMarkerData));
    return self;
}

- (id)initWithData:(NSData *)data cache:(AXObjectCache*)cache
{
    if (!(self = [super init]))
        return nil;
    
    _cache = cache;
    [data getBytes:&_textMarkerData length:sizeof(TextMarkerData)];
    
    return self;
}

// This is needed for external clients to be able to create a text marker without having a pointer to the cache.
- (id)initWithData:(NSData *)data accessibilityObject:(AccessibilityObjectWrapper *)wrapper
{
    WebCore::AccessibilityObject* axObject = [wrapper accessibilityObject]; 
    if (!axObject)
        return nil;
    
    return [self initWithData:data cache:axObject->axObjectCache()];
}

+ (WebAccessibilityTextMarker *)textMarkerWithVisiblePosition:(VisiblePosition&)visiblePos cache:(AXObjectCache*)cache
{
    TextMarkerData textMarkerData;
    cache->textMarkerDataForVisiblePosition(textMarkerData, visiblePos);
    
    return [[[WebAccessibilityTextMarker alloc] initWithTextMarker:&textMarkerData cache:cache] autorelease];
}

- (NSData *)dataRepresentation
{
    return [NSData dataWithBytes:&_textMarkerData length:sizeof(TextMarkerData)];
}

- (VisiblePosition)visiblePosition
{
    return _cache->visiblePositionForTextMarkerData(_textMarkerData);
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"[AXTextMarker %p] = node: %p offset: %d", self, _textMarkerData.node, _textMarkerData.offset];
}

@end

@implementation WebAccessibilityObjectWrapper

- (id)initWithAccessibilityObject:(AccessibilityObject*)axObject
{
    self = [super initWithAccessibilityObject:axObject];
    if (!self)
        return nil;
    
    // Initialize to a sentinel value.
    m_accessibilityTraitsFromAncestor = ULLONG_MAX;
    m_isAccessibilityElement = -1;
    
    return self;
}

- (void)detach
{
    // rdar://8798960 Make sure the object is gone early, so that anything _accessibilityUnregister 
    // does can't call back into the render tree.
    m_object = 0;

    if ([self respondsToSelector:@selector(_accessibilityUnregister)])
        [self _accessibilityUnregister];
}

- (void)dealloc
{
    // We should have been detached before deallocated.
    ASSERT(!m_object);
    [super dealloc];
}

- (BOOL)_prepareAccessibilityCall
{
    // rdar://7980318 if we start a call, then block in WebThreadLock(), then we're dealloced on another, thread, we could
    // crash, so we should retain ourself for the duration of usage here.
    [[self retain] autorelease];

    WebThreadLock();
    
    // If we came back from our thread lock and we were detached, we will no longer have an m_object.
    if (!m_object)
        return NO;
    
    m_object->updateBackingStore();
    if (!m_object)
        return NO;
    
    return YES;
}

// These are here so that we don't have to import AXRuntime.
// The methods will be swizzled when the accessibility bundle is loaded.

- (uint64_t)_axLinkTrait { return (1 << 0); }
- (uint64_t)_axVisitedTrait { return (1 << 1); }
- (uint64_t)_axHeaderTrait { return (1 << 2); }
- (uint64_t)_axContainedByListTrait { return (1 << 3); }
- (uint64_t)_axContainedByTableTrait { return (1 << 4); }
- (uint64_t)_axContainedByLandmarkTrait { return (1 << 5); }
- (uint64_t)_axWebContentTrait { return (1 << 6); }
- (uint64_t)_axSecureTextFieldTrait { return (1 << 7); }
- (uint64_t)_axTextEntryTrait { return (1 << 8); }
- (uint64_t)_axHasTextCursorTrait { return (1 << 9); }
- (uint64_t)_axTextOperationsAvailableTrait { return (1 << 10); }
- (uint64_t)_axImageTrait { return (1 << 11); }
- (uint64_t)_axTabButtonTrait { return (1 << 12); }
- (uint64_t)_axButtonTrait { return (1 << 13); }
- (uint64_t)_axToggleTrait { return (1 << 14); }
- (uint64_t)_axPopupButtonTrait { return (1 << 15); }
- (uint64_t)_axStaticTextTrait { return (1 << 16); }
- (uint64_t)_axAdjustableTrait { return (1 << 17); }
- (uint64_t)_axMenuItemTrait { return (1 << 18); }
- (uint64_t)_axSelectedTrait { return (1 << 19); }
- (uint64_t)_axNotEnabledTrait { return (1 << 20); }
- (uint64_t)_axRadioButtonTrait { return (1 << 21); }

- (BOOL)accessibilityCanFuzzyHitTest
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    AccessibilityRole role = m_object->roleValue();
    // Elements that can be returned when performing fuzzy hit testing.
    switch (role) {
    case ButtonRole:
    case CheckBoxRole:
    case ComboBoxRole:
    case DisclosureTriangleRole:
    case HeadingRole:
    case ImageMapLinkRole:
    case ImageRole:
    case LinkRole:
    case ListBoxRole:
    case ListBoxOptionRole:
    case MenuButtonRole:
    case MenuItemRole:
    case PopUpButtonRole:
    case RadioButtonRole:
    case ScrollBarRole:
    case SliderRole:
    case StaticTextRole:
    case TabRole:
    case TextFieldRole:
        return !m_object->accessibilityIsIgnored();
    default:
        return false;
    }
}

- (AccessibilityObjectWrapper *)accessibilityPostProcessHitTest:(CGPoint)point
{
    UNUSED_PARAM(point);
    // The UIKit accessibility wrapper will override this and perform the post process hit test.
    return nil;
}

- (id)accessibilityHitTest:(CGPoint)point
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    // Try a fuzzy hit test first to find an accessible element.
    RefPtr<AccessibilityObject> axObject;
    {
        AXAttributeCacheEnabler enableCache(m_object->axObjectCache());
        axObject = m_object->accessibilityHitTest(IntPoint(point));
    }

    if (!axObject)
        return nil;
    
    // If this is a good accessible object to return, no extra work is required.
    if ([axObject->wrapper() accessibilityCanFuzzyHitTest])
        return AccessibilityUnignoredAncestor(axObject->wrapper());
    
    // Check to see if we can post-process this hit test to find a better candidate.
    AccessibilityObjectWrapper* wrapper = [axObject->wrapper() accessibilityPostProcessHitTest:point];
    if (wrapper)
        return AccessibilityUnignoredAncestor(wrapper);
    
    // Fall back to default behavior.
    return AccessibilityUnignoredAncestor(axObject->wrapper());    
}

- (NSInteger)accessibilityElementCount
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AXAttributeCacheEnabler enableCache(m_object->axObjectCache());
    if ([self isAttachment])
        return [[self attachmentView] accessibilityElementCount];
    
    return m_object->children().size();
}

- (id)accessibilityElementAtIndex:(NSInteger)index
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AXAttributeCacheEnabler enableCache(m_object->axObjectCache());
    if ([self isAttachment])
        return [[self attachmentView] accessibilityElementAtIndex:index];
    
    AccessibilityObject::AccessibilityChildrenVector children = m_object->children();
    if (static_cast<unsigned>(index) >= children.size())
        return nil;
    
    AccessibilityObjectWrapper* wrapper = children[index]->wrapper();
    if (children[index]->isAttachment())
        return [wrapper attachmentView];

    return wrapper;
}

- (NSInteger)indexOfAccessibilityElement:(id)element
{
    if (![self _prepareAccessibilityCall])
        return NSNotFound;
    
    AXAttributeCacheEnabler enableCache(m_object->axObjectCache());
    if ([self isAttachment])
        return [[self attachmentView] indexOfAccessibilityElement:element];
    
    AccessibilityObject::AccessibilityChildrenVector children = m_object->children();
    unsigned count = children.size();
    for (unsigned k = 0; k < count; ++k) {
        AccessibilityObjectWrapper* wrapper = children[k]->wrapper();
        if (wrapper == element || (children[k]->isAttachment() && [wrapper attachmentView] == element))
            return k;
    }
    
    return NSNotFound;
}

- (CGPathRef)_accessibilityPath
{
    if (![self _prepareAccessibilityCall])
        return NULL;

    if (!m_object->supportsPath())
        return NULL;
    
    Path path = m_object->elementPath();
    if (path.isEmpty())
        return NULL;
    
    return [self convertPathToScreenSpace:path];
}

- (NSString *)accessibilityLanguage
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->language();
}

- (BOOL)_accessibilityIsLandmarkRole:(AccessibilityRole)role
{
    switch (role) {
        case LandmarkApplicationRole:
        case LandmarkBannerRole:
        case LandmarkComplementaryRole:
        case LandmarkContentInfoRole:
        case LandmarkMainRole:
        case LandmarkNavigationRole:
        case LandmarkSearchRole:
            return YES;
        default:
            return NO;
    }    
}

- (AccessibilityObjectWrapper*)_accessibilityListAncestor
{
    for (AccessibilityObject* parent = m_object->parentObject(); parent != nil; parent = parent->parentObject()) {
        AccessibilityRole role = parent->roleValue();
        if (role == ListRole || role == ListBoxRole)
            return parent->wrapper();
    }
    
    return nil;
}

- (AccessibilityObjectWrapper*)_accessibilityLandmarkAncestor
{
    for (AccessibilityObject* parent = m_object->parentObject(); parent != nil; parent = parent->parentObject()) {
        if ([self _accessibilityIsLandmarkRole:parent->roleValue()])
            return parent->wrapper();
    }

    return nil;
}

- (AccessibilityObjectWrapper*)_accessibilityTableAncestor
{
    for (AccessibilityObject* parent = m_object->parentObject(); parent != nil; parent = parent->parentObject()) {
        if (parent->roleValue() == TableRole)
            return parent->wrapper();   
    }
    
    return nil;
}

- (uint64_t)_accessibilityTraitsFromAncestors
{
    uint64_t traits = 0;
    AccessibilityRole role = m_object->roleValue();
    
    // Trait information also needs to be gathered from the parents above the object.
    // The parentObject is needed instead of the unignoredParentObject, because a table might be ignored, but information still needs to be gathered from it.    
    for (AccessibilityObject* parent = m_object->parentObject(); parent != nil; parent = parent->parentObject()) {
        AccessibilityRole parentRole = parent->roleValue();
        if (parentRole == WebAreaRole)
            break;
        
        switch (parentRole) {
            case LinkRole:
            case WebCoreLinkRole:
                traits |= [self _axLinkTrait];
                if (parent->isVisited())
                    traits |= [self _axVisitedTrait];
                break;
            case HeadingRole:
            {
                traits |= [self _axHeaderTrait];
                // If this object has the header trait, we should set the value
                // to the heading level. If it was a static text element, we need to store
                // the value as the label, because the heading level needs to the value.
                AccessibilityObjectWrapper* wrapper = parent->wrapper();
                if (role == StaticTextRole) 
                    [self setAccessibilityLabel:m_object->stringValue()];                
                [self setAccessibilityValue:[wrapper accessibilityValue]];
                break;
            }
            case ListBoxRole:
            case ListRole:
                traits |= [self _axContainedByListTrait];
                break;
            case TableRole:
                traits |= [self _axContainedByTableTrait];
                break;
            default:
                if ([self _accessibilityIsLandmarkRole:parentRole])
                    traits |= [self _axContainedByLandmarkTrait];
                break;
        }
    }
    
    return traits;
}

- (uint64_t)accessibilityTraits
{
    if (![self _prepareAccessibilityCall])
        return 0;
    
    AccessibilityRole role = m_object->roleValue();
    uint64_t traits = [self _axWebContentTrait];
    switch (role) {
        case LinkRole:
        case WebCoreLinkRole:
            traits |= [self _axLinkTrait];
            if (m_object->isVisited())
                traits |= [self _axVisitedTrait];
            break;
        // TextFieldRole is intended to fall through to TextAreaRole, in order to pick up the text entry and text cursor traits.
        case TextFieldRole:
            if (m_object->isPasswordField())
                traits |= [self _axSecureTextFieldTrait];
        case TextAreaRole:
            traits |= [self _axTextEntryTrait];            
            if (m_object->isFocused())
                traits |= ([self _axHasTextCursorTrait] | [self _axTextOperationsAvailableTrait]);
            break;
        case ImageRole:
            traits |= [self _axImageTrait];
            break;
        case TabRole:
            traits |= [self _axTabButtonTrait];
            break;
        case ButtonRole:
            traits |= [self _axButtonTrait];
            if (m_object->isPressed())
                traits |= [self _axToggleTrait];
            break;
        case PopUpButtonRole:
            traits |= [self _axPopupButtonTrait];
            break;
        case RadioButtonRole:
            traits |= [self _axRadioButtonTrait] | [self _axToggleTrait];
            break;
        case CheckBoxRole:
            traits |= ([self _axButtonTrait] | [self _axToggleTrait]);
            break;
        case HeadingRole:
            traits |= [self _axHeaderTrait];
            break;
        case StaticTextRole:
            traits |= [self _axStaticTextTrait];
            break;
        case SliderRole:
            traits |= [self _axAdjustableTrait];
            break;
        case MenuButtonRole:
        case MenuItemRole:
            traits |= [self _axMenuItemTrait];
            break;
        default:
            break;
    }

    if (m_object->isSelected())
        traits |= [self _axSelectedTrait];

    if (!m_object->isEnabled())
        traits |= [self _axNotEnabledTrait];
    
    if (m_accessibilityTraitsFromAncestor == ULLONG_MAX)
        m_accessibilityTraitsFromAncestor = [self _accessibilityTraitsFromAncestors];
    
    traits |= m_accessibilityTraitsFromAncestor;

    return traits;
}

- (BOOL)isSVGGroupElement
{
    // If an SVG group element has a title, it should be an accessible element on iOS.
#if ENABLE(SVG)
    Node* node = m_object->node();
    if (node && node->hasTagName(SVGNames::gTag) && [[self accessibilityLabel] length] > 0)
        return YES;
#endif
    
    return NO;
}

- (BOOL)determineIsAccessibilityElement
{
    if (!m_object)
        return false;
    
    // Honor when something explicitly makes this an element (super will contain that logic) 
    if ([super isAccessibilityElement])
        return YES;
    
    m_object->updateBackingStore();
    
    switch (m_object->roleValue()) {
        case TextFieldRole:
        case TextAreaRole:
        case ButtonRole:
        case PopUpButtonRole:
        case CheckBoxRole:
        case RadioButtonRole:
        case SliderRole:
        case MenuButtonRole:
        case ValueIndicatorRole:
        case ImageRole:
        case ProgressIndicatorRole:
        case MenuItemRole:
        case IncrementorRole:
        case ComboBoxRole:
        case DisclosureTriangleRole:
        case ImageMapRole:
        case ListMarkerRole:
        case ListBoxOptionRole:
        case TabRole:
        case DocumentMathRole:
            return true;
        case StaticTextRole:
        {
            // Many text elements only contain a space.
            if (![[[self accessibilityLabel] stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] length])
                return false;

            // Text elements that are just pieces of links or headers should not be exposed.
            if ([AccessibilityUnignoredAncestor([self accessibilityContainer]) containsUnnaturallySegmentedChildren])
                return false;
            return true;
        }
            
        // Don't expose headers as elements; instead expose their children as elements, with the header trait (unless they have no children)
        case HeadingRole:
            if (![self accessibilityElementCount])
                return true;
            return false;
            
        // Links can sometimes be elements (when they only contain static text or don't contain anything).
        // They should not be elements when containing text and other types.
        case WebCoreLinkRole:
        case LinkRole:
            if ([self containsUnnaturallySegmentedChildren] || ![self accessibilityElementCount])
                return true;
            return false;
        case GroupRole:
            if ([self isSVGGroupElement])
                return true;
        // All other elements are ignored on the iphone.
        default:
        case UnknownRole:
        case TabGroupRole:
        case ScrollAreaRole:
        case TableRole:
        case ApplicationRole:
        case RadioGroupRole:
        case ListRole:
        case ListBoxRole:
        case ScrollBarRole:
        case MenuBarRole:
        case MenuRole:
        case ColumnRole:
        case RowRole:
        case ToolbarRole:
        case BusyIndicatorRole:
        case WindowRole:
        case DrawerRole:
        case SystemWideRole:
        case OutlineRole:
        case BrowserRole:
        case SplitGroupRole:
        case SplitterRole:
        case ColorWellRole:
        case GrowAreaRole:
        case SheetRole:
        case HelpTagRole:
        case MatteRole:
        case RulerRole:
        case RulerMarkerRole:
        case GridRole:
        case WebAreaRole:
            return false;
    }
}

- (BOOL)isAccessibilityElement
{
    if (![self _prepareAccessibilityCall])
        return NO;
    
    if (m_isAccessibilityElement == -1)
        m_isAccessibilityElement = [self determineIsAccessibilityElement];
    
    return m_isAccessibilityElement;
}

- (BOOL)stringValueShouldBeUsedInLabel
{
    if (m_object->isTextControl())
        return NO;
    if (m_object->roleValue() == PopUpButtonRole)
        return NO;
    if (m_object->isFileUploadButton())
        return NO;

    return YES;
}

- (BOOL)fileUploadButtonReturnsValueInTitle
{
    return NO;
}

static void appendStringToResult(NSMutableString *result, NSString *string)
{
    ASSERT(result);
    if (![string length])
        return;
    if ([result length])
        [result appendString:@", "];
    [result appendString:string];
}

- (CGFloat)_accessibilityMinValue
{
    return m_object->minValueForRange();
}

- (CGFloat)_accessibilityMaxValue
{
    return m_object->maxValueForRange();
}

- (NSString *)accessibilityLabel
{
    if (![self _prepareAccessibilityCall])
        return nil;

    // check if the label was overriden
    NSString *label = [super accessibilityLabel];
    if (label)
        return label;

    // iOS doesn't distinguish between a title and description field,
    // so concatentation will yield the best result.
    NSString *axTitle = [self accessibilityTitle];
    NSString *axDescription = [self accessibilityDescription];
    NSString *landmarkDescription = [self ariaLandmarkRoleDescription];

    NSMutableString *result = [NSMutableString string];

    appendStringToResult(result, axTitle);
    appendStringToResult(result, axDescription);
    if ([self stringValueShouldBeUsedInLabel]) {
        NSString *valueLabel = m_object->stringValue();
        valueLabel = [valueLabel stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
        appendStringToResult(result, valueLabel);
    }
    appendStringToResult(result, landmarkDescription);
    
    return [result length] ? result : nil;
}

- (AccessibilityTableCell*)tableCellParent
{
    // Find if this element is in a table cell.
    AccessibilityObject* cell = 0;
    for (cell = m_object; cell && !cell->isTableCell(); cell = cell->parentObject()) 
    { }
    
    if (!cell)
        return 0;

    return static_cast<AccessibilityTableCell*>(cell);
}

- (AccessibilityTable*)tableParent
{
    // Find if the parent table for the table cell.
    AccessibilityObject* parentTable = 0;
    for (parentTable = m_object; parentTable && !parentTable->isDataTable(); parentTable = parentTable->parentObject()) 
    { }
    
    if (!parentTable)
        return 0;
    
    return static_cast<AccessibilityTable*>(parentTable);
}

- (id)accessibilityTitleElement
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AccessibilityObject* titleElement = m_object->titleUIElement();
    if (titleElement)
        return titleElement->wrapper();

    return nil;
}

// Meant to return row or column headers (or other things as the future permits).
- (NSArray *)accessibilityHeaderElements
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AccessibilityTableCell* tableCell = [self tableCellParent];
    if (!tableCell)
        return nil;
    
    AccessibilityTable* table = [self tableParent];
    if (!table)
        return nil;
    
    // Get the row and column range, so we can use them to find the headers.
    pair<unsigned, unsigned> rowRange;
    pair<unsigned, unsigned> columnRange;
    tableCell->rowIndexRange(rowRange);
    tableCell->columnIndexRange(columnRange);
    
    AccessibilityObject::AccessibilityChildrenVector rowHeaders;
    AccessibilityObject::AccessibilityChildrenVector columnHeaders;
    table->rowHeaders(rowHeaders);
    table->columnHeaders(columnHeaders);
    
    NSMutableArray *headers = [NSMutableArray array];
    
    unsigned columnRangeIndex = static_cast<unsigned>(columnRange.first);
    if (columnRangeIndex < columnHeaders.size()) {
        RefPtr<AccessibilityObject> columnHeader = columnHeaders[columnRange.first];
        AccessibilityObjectWrapper* wrapper = columnHeader->wrapper();
        if (wrapper)
            [headers addObject:wrapper];
    }

    unsigned rowRangeIndex = static_cast<unsigned>(rowRange.first);
    if (rowRangeIndex < rowHeaders.size()) {
        RefPtr<AccessibilityObject> rowHeader = rowHeaders[rowRange.first];
        AccessibilityObjectWrapper* wrapper = rowHeader->wrapper();
        if (wrapper)
            [headers addObject:wrapper];
    }
        
    return headers;
}

- (id)accessibilityElementForRow:(NSInteger)row andColumn:(NSInteger)column
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AccessibilityTable* table = [self tableParent];
    if (!table)
        return nil;

    AccessibilityTableCell* cell = table->cellForColumnAndRow(column, row);
    if (!cell)
        return nil;
    return cell->wrapper();
}

- (NSRange)accessibilityRowRange
{
    if (![self _prepareAccessibilityCall])
        return NSMakeRange(NSNotFound, 0);

    if (m_object->isRadioButton()) {
        AccessibilityObject::AccessibilityChildrenVector radioButtonSiblings;
        m_object->linkedUIElements(radioButtonSiblings);
        if (radioButtonSiblings.size() <= 1)
            return NSMakeRange(NSNotFound, 0);
        
        return NSMakeRange(radioButtonSiblings.find(m_object), radioButtonSiblings.size());
    }
    
    AccessibilityTableCell* tableCell = [self tableCellParent];
    if (!tableCell)
        return NSMakeRange(NSNotFound, 0);
    
    pair<unsigned, unsigned> rowRange;
    tableCell->rowIndexRange(rowRange);
    return NSMakeRange(rowRange.first, rowRange.second);
}

- (NSRange)accessibilityColumnRange
{
    if (![self _prepareAccessibilityCall])
        return NSMakeRange(NSNotFound, 0);

    AccessibilityTableCell* tableCell = [self tableCellParent];
    if (!tableCell)
        return NSMakeRange(NSNotFound, 0);
    
    pair<unsigned, unsigned> columnRange;
    tableCell->columnIndexRange(columnRange);
    return NSMakeRange(columnRange.first, columnRange.second);
}

- (NSString *)accessibilityPlaceholderValue
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->placeholderValue();
}

- (NSString *)accessibilityValue
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    // check if the value was overriden
    NSString *value = [super accessibilityValue];
    if (value)
        return value;
    
    if (m_object->isCheckboxOrRadio()) {
        switch (m_object->checkboxOrRadioValue()) {
        case ButtonStateOff:
            return [NSString stringWithFormat:@"%d", 0];
        case ButtonStateOn:
            return [NSString stringWithFormat:@"%d", 1];
        case ButtonStateMixed:
            return [NSString stringWithFormat:@"%d", 2];
        }
        ASSERT_NOT_REACHED();
        return [NSString stringWithFormat:@"%d", 0];
    }
    
    if (m_object->isButton() && m_object->isPressed())
        return [NSString stringWithFormat:@"%d", 1];

    // rdar://8131388 WebKit should expose the same info as UIKit for its password fields.
    if (m_object->isPasswordField()) {
        int passwordLength = m_object->accessibilityPasswordFieldLength();
        NSMutableString* string = [NSMutableString string];
        for (int k = 0; k < passwordLength; ++k)
            [string appendString:@"•"];
        return string;
    }
    
    // A text control should return its text data as the axValue (per iPhone AX API).
    if (![self stringValueShouldBeUsedInLabel])
        return m_object->stringValue();
    
    if (m_object->isProgressIndicator() || m_object->isSlider()) {
        // Prefer a valueDescription if provided by the author (through aria-valuetext).
        String valueDescription = m_object->valueDescription();
        if (!valueDescription.isEmpty())
            return valueDescription;

        return [NSString stringWithFormat:@"%.2f", m_object->valueForRange()];
    }

    if (m_object->isHeading())
        return [NSString stringWithFormat:@"%d", m_object->headingLevel()];
    
    return nil;
}

- (BOOL)accessibilityIsComboBox
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return m_object->roleValue() == ComboBoxRole;
}

- (NSString *)accessibilityHint
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return [self accessibilityHelpText];
}

- (NSURL *)accessibilityURL
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    KURL url = m_object->url();
    if (url.isNull())
        return nil;
    return (NSURL*)url;
}

- (CGPoint)convertPointToScreenSpace:(FloatPoint &)point
{
    if (!m_object)
        return CGPointZero;
    
    CGPoint cgPoint = CGPointMake(point.x(), point.y());
    
    FrameView* frameView = m_object->documentFrameView();
    if (frameView) {
        WAKView* view = frameView->documentView();
        cgPoint = [view convertPoint:cgPoint toView:nil];
    }
    
    // we need the web document view to give us our final screen coordinates
    // because that can take account of the scroller
    id webDocument = [self _accessibilityWebDocumentView];
    if (webDocument)
        cgPoint = [webDocument convertPoint:cgPoint toView:nil];
    
    return cgPoint;
}

- (CGRect)convertRectToScreenSpace:(IntRect &)rect
{
    if (!m_object)
        return CGRectZero;
    
    CGSize size = CGSizeMake(rect.size().width(), rect.size().height());
    CGPoint point = CGPointMake(rect.x(), rect.y());
    
    CGRect frame = CGRectMake(point.x, point.y, size.width, size.height);
    
    FrameView* frameView = m_object->documentFrameView();
    if (frameView) {
        WAKView* view = frameView->documentView();
        frame = [view convertRect:frame toView:nil];
    }
    
    // we need the web document view to give us our final screen coordinates
    // because that can take account of the scroller
    id webDocument = [self _accessibilityWebDocumentView];
    if (webDocument)
        frame = [webDocument convertRect:frame toView:nil];
    
    return frame;
}

// Used by UIKit accessibility bundle to help determine distance during a hit-test.
- (CGRect)accessibilityElementRect
{
    if (![self _prepareAccessibilityCall])
        return CGRectZero;
    
    LayoutRect rect = m_object->elementRect();
    return CGRectMake(rect.x(), rect.y(), rect.width(), rect.height());
}

// The "center point" is where VoiceOver will "press" an object. This may not be the actual
// center of the accessibilityFrame
- (CGPoint)accessibilityActivationPoint
{
    if (![self _prepareAccessibilityCall])
        return CGPointZero;
    
    IntRect rect = pixelSnappedIntRect(m_object->boundingBoxRect());
    CGRect cgRect = [self convertRectToScreenSpace:rect];
    return CGPointMake(CGRectGetMidX(cgRect), CGRectGetMidY(cgRect));
}

- (CGRect)accessibilityFrame
{
    if (![self _prepareAccessibilityCall])
        return CGRectZero;
    
    IntRect rect = pixelSnappedIntRect(m_object->elementRect());
    return [self convertRectToScreenSpace:rect];
}

// Checks whether a link contains only static text and images (and has been divided unnaturally by <spans> and other nefarious mechanisms).
- (BOOL)containsUnnaturallySegmentedChildren
{
    if (!m_object)
        return NO;
    
    AccessibilityRole role = m_object->roleValue();
    if (role != LinkRole && role != WebCoreLinkRole)
        return NO;
    
    AccessibilityObject::AccessibilityChildrenVector children = m_object->children();
    unsigned childrenSize = children.size();

    // If there's only one child, then it doesn't have segmented children. 
    if (childrenSize == 1)
        return NO;
    
    for (unsigned i = 0; i < childrenSize; ++i) {
        AccessibilityRole role = children[i]->roleValue();
        if (role != StaticTextRole && role != ImageRole && role != GroupRole)
            return NO;
    }
    
    return YES;
}

- (id)accessibilityContainer
{
    if (![self _prepareAccessibilityCall])
        return nil;

    AXAttributeCacheEnabler enableCache(m_object->axObjectCache());
    
    // As long as there's a parent wrapper, that's the correct chain to climb.
    AccessibilityObject* parent = m_object->parentObjectUnignored(); 
    if (parent)
        return parent->wrapper();

    // The only object without a parent wrapper should be a scroll view.
    ASSERT(m_object->isAccessibilityScrollView());
    
    // Verify this is the top document. If not, we might need to go through the platform widget.
    FrameView* frameView = m_object->documentFrameView();
    Document* document = m_object->document();
    if (document && frameView && document != document->topDocument())
        return frameView->platformWidget();
    
    // The top scroll view's parent is the web document view.
    return [self _accessibilityWebDocumentView];
}

- (id)accessibilityFocusedUIElement
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    AccessibilityObject* focusedObj = m_object->focusedUIElement();
    
    if (!focusedObj)
        return nil;
    
    return focusedObj->wrapper();
}

- (id)_accessibilityWebDocumentView
{
    if (![self _prepareAccessibilityCall])
        return nil;

    // This method performs the crucial task of connecting to the UIWebDocumentView.
    // This is needed to correctly calculate the screen position of the AX object.
    static Class webViewClass = nil;
    if (!webViewClass)
        webViewClass = NSClassFromString(@"WebView");

    if (!webViewClass)
        return nil;
    
    FrameView* frameView = m_object->documentFrameView();

    if (!frameView)
        return nil;
    
    // If this is the top level frame, the UIWebDocumentView should be returned.
    id parentView = frameView->documentView();
    while (parentView && ![parentView isKindOfClass:webViewClass])
        parentView = [parentView superview];
    
    // The parentView should have an accessibilityContainer, if the UIKit accessibility bundle was loaded. 
    // The exception is DRT, which tests accessibility without the entire system turning accessibility on. Hence,
    // this check should be valid for everything except DRT.
    ASSERT([parentView accessibilityContainer] || applicationIsDumpRenderTree());
    
    return [parentView accessibilityContainer];
}

- (NSArray *)_accessibilityNextElementsWithCount:(UInt32)count
{    
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return [[self _accessibilityWebDocumentView] _accessibilityNextElementsWithCount:count];
}

- (NSArray *)_accessibilityPreviousElementsWithCount:(UInt32)count
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return [[self _accessibilityWebDocumentView] _accessibilityPreviousElementsWithCount:count];
}

- (BOOL)accessibilityRequired
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return m_object->isRequired();
}

- (NSArray *)accessibilityFlowToElements
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    AccessibilityObject::AccessibilityChildrenVector children;
    m_object->ariaFlowToElements(children);
    
    unsigned length = children.size();
    NSMutableArray* array = [NSMutableArray arrayWithCapacity:length];
    for (unsigned i = 0; i < length; ++i) {
        AccessibilityObjectWrapper* wrapper = children[i]->wrapper();
        ASSERT(wrapper);
        if (!wrapper)
            continue;

        if (children[i]->isAttachment() && [wrapper attachmentView])
            [array addObject:[wrapper attachmentView]];
        else
            [array addObject:wrapper];
    }
    return array;
}

- (id)accessibilityLinkedElement
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    // If this static text inside of a link, it should use its parent's linked element.
    AccessibilityObject* element = m_object;
    if (m_object->roleValue() == StaticTextRole && m_object->parentObjectUnignored()->isLink())
        element = m_object->parentObjectUnignored();
    
    AccessibilityObject::AccessibilityChildrenVector children;
    element->linkedUIElements(children);
    if (children.size() == 0)
        return nil;
    
    return children[0]->wrapper();
}


- (BOOL)isAttachment
{
    if (!m_object)
        return NO;
    
    return m_object->isAttachment();
}

- (void)_accessibilityActivate
{
    if (![self _prepareAccessibilityCall])
        return;

    m_object->press();
}

- (id)attachmentView
{
    if (![self _prepareAccessibilityCall])
        return nil;

    ASSERT([self isAttachment]);
    Widget* widget = m_object->widgetForAttachmentView();
    if (!widget)
        return nil;
    return widget->platformWidget();    
}

static RenderObject* rendererForView(WAKView* view)
{
    if (![view conformsToProtocol:@protocol(WebCoreFrameView)])
        return 0;
    
    WAKView<WebCoreFrameView>* frameView = (WAKView<WebCoreFrameView>*)view;
    Frame* frame = [frameView _web_frame];
    if (!frame)
        return 0;
    
    Node* node = frame->document()->ownerElement();
    if (!node)
        return 0;
    
    return node->renderer();
}

- (id)_accessibilityParentForSubview:(id)subview
{   
    RenderObject* renderer = rendererForView(subview);
    if (!renderer)
        return nil;
    
    AccessibilityObject* obj = renderer->document()->axObjectCache()->getOrCreate(renderer);
    if (obj)
        return obj->parentObjectUnignored()->wrapper();
    return nil;
}

- (void)postFocusChangeNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.
}

- (void)postSelectedTextChangeNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.    
}

- (void)postLayoutChangeNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.        
}

- (void)postLiveRegionChangeNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.    
}

- (void)postLoadCompleteNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.    
}

- (void)postChildrenChangedNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.    
}

- (void)postInvalidStatusChangedNotification
{
    // The UIKit accessibility wrapper will override and post appropriate notification.        
}

- (void)accessibilityElementDidBecomeFocused
{
    if (![self _prepareAccessibilityCall])
        return;
    
    // The focused VoiceOver element might be the text inside a link.
    // In those cases we should focus on the link itself.
    for (AccessibilityObject* object = m_object; object != nil; object = object->parentObject()) {
        if (object->roleValue() == WebAreaRole)
            break;

        if (object->canSetFocusAttribute()) {
            object->setFocused(true);
            break;
        }
    }
}

- (void)accessibilityModifySelection:(TextGranularity)granularity increase:(BOOL)increase
{
    if (![self _prepareAccessibilityCall])
        return;
    
    FrameSelection* frameSelection = m_object->document()->frame()->selection();
    VisibleSelection selection = m_object->selection();
    VisiblePositionRange range = m_object->visiblePositionRange();
    
    // Before a selection with length exists, the cursor position needs to move to the right starting place.
    // That should be the beginning of this element (range.start). However, if the cursor is already within the 
    // range of this element (the cursor is represented by selection), then the cursor does not need to move.
    if (frameSelection->isNone() && (selection.visibleStart() < range.start || selection.visibleEnd() > range.end))
        frameSelection->moveTo(range.start, UserTriggered);
    
    frameSelection->modify(FrameSelection::AlterationExtend, (increase) ? DirectionRight : DirectionLeft, granularity, UserTriggered);
}

- (void)accessibilityIncreaseSelection:(TextGranularity)granularity
{
    [self accessibilityModifySelection:granularity increase:YES];
}

- (void)accessibilityDecreaseSelection:(TextGranularity)granularity
{
    [self accessibilityModifySelection:granularity increase:NO];
}

- (void)accessibilityMoveSelectionToMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return;
    
    VisiblePosition visiblePosition = [marker visiblePosition];
    if (visiblePosition.isNull())
        return;

    FrameSelection* frameSelection = m_object->document()->frame()->selection();
    frameSelection->moveTo(visiblePosition, UserTriggered);
}

- (void)accessibilityIncrement
{
    if (![self _prepareAccessibilityCall])
        return;

    m_object->increment();
}

- (void)accessibilityDecrement
{
    if (![self _prepareAccessibilityCall])
        return;

    m_object->decrement();
}

#pragma mark Accessibility Text Marker Handlers

- (BOOL)_addAccessibilityObject:(AccessibilityObject*)axObject toTextMarkerArray:(NSMutableArray *)array
{
    if (!axObject)
        return NO;
    
    AccessibilityObjectWrapper* wrapper = axObject->wrapper();
    if (!wrapper)
        return NO;

    // Don't add the same object twice, but since this has already been added, we should return
    // YES because we want to inform that it's in the array
    if ([array containsObject:wrapper])
        return YES;
    
    // Explicity set that this is now an element (in case other logic tries to override).
    [wrapper setValue:[NSNumber numberWithBool:YES] forKey:@"isAccessibilityElement"];    
    [array addObject:wrapper];
    return YES;
}

- (NSString *)stringForTextMarkers:(NSArray *)markers
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    if ([markers count] != 2)
        return nil;
    
    WebAccessibilityTextMarker* startMarker = [markers objectAtIndex:0];
    WebAccessibilityTextMarker* endMarker = [markers objectAtIndex:1];
    if (![startMarker isKindOfClass:[WebAccessibilityTextMarker class]] || ![endMarker isKindOfClass:[WebAccessibilityTextMarker class]])
        return nil;
    
    // extract the start and end VisiblePosition
    VisiblePosition startVisiblePosition = [startMarker visiblePosition];
    if (startVisiblePosition.isNull())
        return nil;
    
    VisiblePosition endVisiblePosition = [endMarker visiblePosition];
    if (endVisiblePosition.isNull())
        return nil;

    VisiblePositionRange visiblePosRange = VisiblePositionRange(startVisiblePosition, endVisiblePosition);
    return m_object->stringForVisiblePositionRange(visiblePosRange);
}

static int blockquoteLevel(RenderObject* renderer)
{
    if (!renderer)
        return 0;
    
    int result = 0;
    for (Node* node = renderer->node(); node; node = node->parentNode()) {
        if (node->hasTagName(blockquoteTag))
            result += 1;
    }
    
    return result;
}

static void AXAttributeStringSetBlockquoteLevel(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    int quoteLevel = blockquoteLevel(renderer);
    
    if (quoteLevel)
        [attrString addAttribute:UIAccessibilityTokenBlockquoteLevel value:[NSNumber numberWithInt:quoteLevel] range:range];
    else
        [attrString removeAttribute:UIAccessibilityTokenBlockquoteLevel range:range];
}

static void AXAttributeStringSetHeadingLevel(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    if (!renderer)
        return;
    
    AccessibilityObject* parentObject = renderer->document()->axObjectCache()->getOrCreate(renderer->parent());
    int parentHeadingLevel = parentObject->headingLevel();
    
    if (parentHeadingLevel)
        [attrString addAttribute:UIAccessibilityTokenHeadingLevel value:[NSNumber numberWithInt:parentHeadingLevel] range:range];
    else
        [attrString removeAttribute:UIAccessibilityTokenHeadingLevel range:range];
}

static void AXAttributeStringSetFont(NSMutableAttributedString* attrString, GSFontRef font, NSRange range)
{
    if (!font)
        return;
    
    const char* nameCStr = GSFontGetFullName(font);
    const char* familyCStr = GSFontGetFamilyName(font);
    NSNumber* size = [NSNumber numberWithFloat:GSFontGetSize(font)];
    NSNumber* bold = [NSNumber numberWithBool:GSFontIsBold(font)];
    GSFontTraitMask traits = GSFontGetTraits(font);
    if (nameCStr)
        [attrString addAttribute:UIAccessibilityTokenFontName value:[NSString stringWithUTF8String:nameCStr] range:range];
    if (familyCStr)
        [attrString addAttribute:UIAccessibilityTokenFontFamily value:[NSString stringWithUTF8String:familyCStr] range:range];
    if ([size boolValue])
        [attrString addAttribute:UIAccessibilityTokenFontSize value:size range:range];
    if ([bold boolValue] || (traits & GSBoldFontMask))
        [attrString addAttribute:UIAccessibilityTokenBold value:[NSNumber numberWithBool:YES] range:range];
    if (traits & GSItalicFontMask)
        [attrString addAttribute:UIAccessibilityTokenItalic value:[NSNumber numberWithBool:YES] range:range];

}

static void AXAttributeStringSetNumber(NSMutableAttributedString* attrString, NSString* attribute, NSNumber* number, NSRange range)
{
    if (number)
        [attrString addAttribute:attribute value:number range:range];
    else
        [attrString removeAttribute:attribute range:range];
}

static void AXAttributeStringSetStyle(NSMutableAttributedString* attrString, RenderObject* renderer, NSRange range)
{
    RenderStyle* style = renderer->style();
    
    // set basic font info
    AXAttributeStringSetFont(attrString, style->font().primaryFont()->getGSFont(), range);
                
    int decor = style->textDecorationsInEffect();
    if ((decor & (TextDecorationUnderline | TextDecorationLineThrough)) != 0) {
        // find colors using quirk mode approach (strict mode would use current
        // color for all but the root line box, which would use getTextDecorationColors)
        Color underline, overline, linethrough;
        renderer->getTextDecorationColors(decor, underline, overline, linethrough);
        
        if (decor & TextDecorationUnderline)
            AXAttributeStringSetNumber(attrString, UIAccessibilityTokenUnderline, [NSNumber numberWithBool:YES], range);
    }
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
    
    // set new attributes
    AXAttributeStringSetStyle(attrString, node->renderer(), attrStringRange);
    AXAttributeStringSetHeadingLevel(attrString, node->renderer(), attrStringRange);
    AXAttributeStringSetBlockquoteLevel(attrString, node->renderer(), attrStringRange);    
}


// This method is intended to return an array of strings and accessibility elements that 
// represent the objects on one line of rendered web content. The array of markers sent
// in should be ordered and contain only a start and end marker.
- (NSArray *)arrayOfTextForTextMarkers:(NSArray *)markers attributed:(BOOL)attributed
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if ([markers count] != 2)
        return nil;
    
    WebAccessibilityTextMarker* startMarker = [markers objectAtIndex:0];
    WebAccessibilityTextMarker* endMarker = [markers objectAtIndex:1];
    if (![startMarker isKindOfClass:[WebAccessibilityTextMarker class]] || ![endMarker isKindOfClass:[WebAccessibilityTextMarker class]])
        return nil;
    
    // extract the start and end VisiblePosition
    VisiblePosition startVisiblePosition = [startMarker visiblePosition];
    if (startVisiblePosition.isNull())
        return nil;
    
    VisiblePosition endVisiblePosition = [endMarker visiblePosition];
    if (endVisiblePosition.isNull())
        return nil;
    
    // iterate over the range to build the AX attributed string
    NSMutableArray* array = [[NSMutableArray alloc] init];
    TextIterator it(makeRange(startVisiblePosition, endVisiblePosition).get());
    for (; !it.atEnd(); it.advance()) {
        // locate the node and starting offset for this range
        int exception = 0;
        Node* node = it.range()->startContainer(exception);
        ASSERT(node == it.range()->endContainer(exception));
        int offset = it.range()->startOffset(exception);
        
        // non-zero length means textual node, zero length means replaced node (AKA "attachments" in AX)
        if (it.length() != 0) {
            if (!attributed) {
                // First check if this is represented by a link.
                AccessibilityObject* linkObject = AccessibilityObject::anchorElementForNode(node);
                if ([self _addAccessibilityObject:linkObject toTextMarkerArray:array])
                    continue;
                
                // Next check if this region is represented by a heading.
                AccessibilityObject* headingObject = AccessibilityObject::headingElementForNode(node);
                if ([self _addAccessibilityObject:headingObject toTextMarkerArray:array])
                    continue;
                
                String listMarkerText = m_object->listMarkerTextForNodeAndPosition(node, VisiblePosition(it.range()->startPosition())); 
                
                if (!listMarkerText.isEmpty()) 
                    [array addObject:[NSString stringWithCharacters:listMarkerText.characters() length:listMarkerText.length()]];
                // There was not an element representation, so just return the text.
                [array addObject:[NSString stringWithCharacters:it.characters() length:it.length()]];
            }
            else
            {
                String listMarkerText = m_object->listMarkerTextForNodeAndPosition(node, VisiblePosition(it.range()->startPosition())); 

                if (!listMarkerText.isEmpty()) {
                    NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] init];
                    AXAttributedStringAppendText(attrString, node, listMarkerText.characters(), listMarkerText.length());
                    [array addObject:attrString];
                    [attrString release];
                }
                
                NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] init];
                AXAttributedStringAppendText(attrString, node, it.characters(), it.length());
                [array addObject:attrString];
                [attrString release];
            }
        } else {
            Node* replacedNode = node->childNode(offset);
            if (replacedNode) {
                AccessibilityObject* obj = m_object->axObjectCache()->getOrCreate(replacedNode->renderer());
                if (obj && !obj->accessibilityIsIgnored())
                    [self _addAccessibilityObject:obj toTextMarkerArray:array];
            }
        }
    }
    
    return [array autorelease];
}

- (NSRange)_convertToNSRange:(Range *)range
{
    if (!range || !range->startContainer())
        return NSMakeRange(NSNotFound, 0);
    
    Document* document = m_object->document();
    FrameSelection* frameSelection = document->frame()->selection();
    
    Element* selectionRoot = frameSelection->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : document->documentElement();
    
    // Mouse events may cause TSM to attempt to create an NSRange for a portion of the view
    // that is not inside the current editable region.  These checks ensure we don't produce
    // potentially invalid data when responding to such requests.
    if (range->startContainer() != scope && !range->startContainer()->isDescendantOf(scope))
        return NSMakeRange(NSNotFound, 0);
    if (range->endContainer() != scope && !range->endContainer()->isDescendantOf(scope))
        return NSMakeRange(NSNotFound, 0);
    
    RefPtr<Range> testRange = Range::create(scope->document(), scope, 0, range->startContainer(), range->startOffset());
    ASSERT(testRange->startContainer() == scope);
    int startPosition = TextIterator::rangeLength(testRange.get());
    
    ExceptionCode ec;
    testRange->setEnd(range->endContainer(), range->endOffset(), ec);
    ASSERT(testRange->startContainer() == scope);
    int endPosition = TextIterator::rangeLength(testRange.get());
    return NSMakeRange(startPosition, endPosition - startPosition);
}

- (PassRefPtr<Range>)_convertToDOMRange:(NSRange)nsrange
{
    if (nsrange.location > INT_MAX)
        return 0;
    if (nsrange.length > INT_MAX || nsrange.location + nsrange.length > INT_MAX)
        nsrange.length = INT_MAX - nsrange.location;
        
    // our critical assumption is that we are only called by input methods that
    // concentrate on a given area containing the selection
    // We have to do this because of text fields and textareas. The DOM for those is not
    // directly in the document DOM, so serialization is problematic. Our solution is
    // to use the root editable element of the selection start as the positional base.
    // That fits with AppKit's idea of an input context.
    Document* document = m_object->document();
    FrameSelection* frameSelection = document->frame()->selection();
    Element* selectionRoot = frameSelection->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : document->documentElement();
    return TextIterator::rangeFromLocationAndLength(scope, nsrange.location, nsrange.length);
}

// This method is intended to take a text marker representing a VisiblePosition and convert it
// into a normalized location within the document.
- (NSInteger)positionForTextMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return NSNotFound;

    if (!marker)
        return NSNotFound;    

    VisibleSelection selection([marker visiblePosition]);
    RefPtr<Range> range = selection.toNormalizedRange();
    NSRange nsRange = [self _convertToNSRange:range.get()];
    return nsRange.location;
}

- (NSArray *)textMarkerRange
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    VisiblePositionRange range = m_object->visiblePositionRange();
    VisiblePosition startPosition = range.start;
    VisiblePosition endPosition = range.end;
    WebAccessibilityTextMarker* start = [WebAccessibilityTextMarker textMarkerWithVisiblePosition:startPosition cache:m_object->axObjectCache()];
    WebAccessibilityTextMarker* end = [WebAccessibilityTextMarker textMarkerWithVisiblePosition:endPosition cache:m_object->axObjectCache()];
    
    return [NSArray arrayWithObjects:start, end, nil];
}

// A method to get the normalized text cursor range of an element. Used in DumpRenderTree.
- (NSRange)elementTextRange
{
    if (![self _prepareAccessibilityCall])
        return NSMakeRange(NSNotFound, 0);

    NSArray *markers = [self textMarkerRange];
    if ([markers count] != 2)
        return NSMakeRange(NSNotFound, 0);
    
    WebAccessibilityTextMarker *startMarker = [markers objectAtIndex:0];
    WebAccessibilityTextMarker *endMarker = [markers objectAtIndex:1];
    
    NSInteger startPosition = [self positionForTextMarker:startMarker];
    NSInteger endPosition = [self positionForTextMarker:endMarker];
    
    return NSMakeRange(startPosition, endPosition - startPosition);
}

- (AccessibilityObjectWrapper *)accessibilityObjectForTextMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    VisiblePosition visiblePosition = [marker visiblePosition];
    AccessibilityObject* obj = m_object->accessibilityObjectForPosition(visiblePosition);
    if (!obj)
        return nil;
    
    return AccessibilityUnignoredAncestor(obj->wrapper());
}

- (NSArray *)textMarkerRangeForSelection
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    VisibleSelection selection = m_object->selection();
    if (selection.isNone())
        return nil;
    VisiblePosition startPosition = selection.visibleStart();
    VisiblePosition endPosition = selection.visibleEnd();

    WebAccessibilityTextMarker* startMarker = [WebAccessibilityTextMarker textMarkerWithVisiblePosition:startPosition cache:m_object->axObjectCache()];
    WebAccessibilityTextMarker* endMarker = [WebAccessibilityTextMarker textMarkerWithVisiblePosition:endPosition cache:m_object->axObjectCache()];
    if (!startMarker || !endMarker)
        return nil;
    
    return [NSArray arrayWithObjects:startMarker, endMarker, nil];
}

- (WebAccessibilityTextMarker *)textMarkerForPosition:(NSInteger)position
{
    if (![self _prepareAccessibilityCall])
        return nil;

    PassRefPtr<Range> range = [self _convertToDOMRange:NSMakeRange(position, 0)];
    if (!range)
        return nil;
    
    VisibleSelection selection = VisibleSelection(range.get(), DOWNSTREAM);

    VisiblePosition visiblePosition = selection.visibleStart();
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:visiblePosition cache:m_object->axObjectCache()];
}

- (id)_stringForRange:(NSRange)range attributed:(BOOL)attributed
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    WebAccessibilityTextMarker* startMarker = [self textMarkerForPosition:range.location];
    WebAccessibilityTextMarker* endMarker = [self textMarkerForPosition:NSMaxRange(range)];
    
    // Clients don't always know the exact range, rather than force them to compute it,
    // allow clients to overshoot and use the max text marker range.
    if (!startMarker || !endMarker) {
        NSArray *markers = [self textMarkerRange];
        if ([markers count] != 2)
            return nil;
        if (!startMarker)
            startMarker = [markers objectAtIndex:0];
        if (!endMarker)
            endMarker = [markers objectAtIndex:1];
    }
    
    NSArray* array = [self arrayOfTextForTextMarkers:[NSArray arrayWithObjects:startMarker, endMarker, nil] attributed:attributed];
    Class returnClass = attributed ? [NSMutableAttributedString class] : [NSMutableString class];
    id returnValue = [[[returnClass alloc] init] autorelease];
    
    NSInteger count = [array count];
    for (NSInteger k = 0; k < count; ++k) {
        id object = [array objectAtIndex:k];

        if (![object isKindOfClass:returnClass])
            continue;
        
        if (attributed)
            [(NSMutableAttributedString *)returnValue appendAttributedString:object];
        else
            [(NSMutableString *)returnValue appendString:object];
    }
    return returnValue;
}


// A convenience method for getting the text of a NSRange. Currently used only by DRT.
- (NSString *)stringForRange:(NSRange)range
{
    return [self _stringForRange:range attributed:NO];
}

- (NSAttributedString *)attributedStringForRange:(NSRange)range
{
    return [self _stringForRange:range attributed:YES];
}

// A convenience method for getting the accessibility objects of a NSRange. Currently used only by DRT.
- (NSArray *)elementsForRange:(NSRange)range
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    WebAccessibilityTextMarker* startMarker = [self textMarkerForPosition:range.location];
    WebAccessibilityTextMarker* endMarker = [self textMarkerForPosition:NSMaxRange(range)];
    if (!startMarker || !endMarker)
        return nil;
    
    NSArray* array = [self arrayOfTextForTextMarkers:[NSArray arrayWithObjects:startMarker, endMarker, nil] attributed:NO];
    NSMutableArray* elements = [NSMutableArray array];
    for (id element in array) {
        if (![element isKindOfClass:[AccessibilityObjectWrapper class]])
            continue;
        [elements addObject:element];
    }
    return elements;
}

- (NSString *)selectionRangeString
{
    NSArray *markers = [self textMarkerRangeForSelection];
    return [self stringForTextMarkers:markers];
}

- (WebAccessibilityTextMarker *)selectedTextMarker
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    VisibleSelection selection = m_object->selection();
    VisiblePosition position = selection.visibleStart();
    
    // if there's no selection, start at the top of the document
    if (position.isNull())
        position = startOfDocument(m_object->document());
    
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:position cache:m_object->axObjectCache()];
}

// This method is intended to return the marker at the end of the line starting at
// the marker that is passed into the method.
- (WebAccessibilityTextMarker *)lineEndMarkerForMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    VisiblePosition start = [marker visiblePosition];
    VisiblePosition lineEnd = m_object->nextLineEndPosition(start);
    
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:lineEnd cache:m_object->axObjectCache()];
}

// This method is intended to return the marker at the start of the line starting at
// the marker that is passed into the method.
- (WebAccessibilityTextMarker *)lineStartMarkerForMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    VisiblePosition start = [marker visiblePosition];
    VisiblePosition lineStart = m_object->previousLineStartPosition(start);
    
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:lineStart cache:m_object->axObjectCache()];
}

- (WebAccessibilityTextMarker *)nextMarkerForMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    VisiblePosition start = [marker visiblePosition];
    VisiblePosition nextMarker = m_object->nextVisiblePosition(start);
    
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:nextMarker cache:m_object->axObjectCache()];
}

// This method is intended to return the marker at the start of the line starting at
// the marker that is passed into the method.
- (WebAccessibilityTextMarker *)previousMarkerForMarker:(WebAccessibilityTextMarker *)marker
{
    if (![self _prepareAccessibilityCall])
        return nil;

    if (!marker)
        return nil;
    
    VisiblePosition start = [marker visiblePosition];
    VisiblePosition previousMarker = m_object->previousVisiblePosition(start);
    
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:previousMarker cache:m_object->axObjectCache()];
}

// This method is intended to return the bounds of a text marker range in screen coordinates.
- (CGRect)frameForTextMarkers:(NSArray *)array
{
    if (![self _prepareAccessibilityCall])
        return CGRectZero;

    if ([array count] != 2)
        return CGRectZero;
    
    WebAccessibilityTextMarker* startMarker = [array objectAtIndex:0];
    WebAccessibilityTextMarker* endMarker = [array objectAtIndex:1];
    if (![startMarker isKindOfClass:[WebAccessibilityTextMarker class]] || ![endMarker isKindOfClass:[WebAccessibilityTextMarker class]])
        return CGRectZero;

    IntRect rect = m_object->boundsForVisiblePositionRange(VisiblePositionRange([startMarker visiblePosition], [endMarker visiblePosition]));
    return [self convertRectToScreenSpace:rect];
}

- (WebAccessibilityTextMarker *)textMarkerForPoint:(CGPoint)point
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    VisiblePosition pos = m_object->visiblePositionForPoint(IntPoint(point));
    return [WebAccessibilityTextMarker textMarkerWithVisiblePosition:pos cache:m_object->axObjectCache()];
}

- (NSString *)accessibilityIdentifier
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->getAttribute(HTMLNames::idAttr);
}

- (NSString *)accessibilitySpeechHint
{
    if (![self _prepareAccessibilityCall])
        return nil;

    switch (m_object->speakProperty()) {
    default:
    case SpeakNormal:
        return @"normal";
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
    }
    
    return nil;
}

- (BOOL)accessibilityARIAIsBusy
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->ariaLiveRegionBusy();
}

- (NSString *)accessibilityARIALiveRegionStatus
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->ariaLiveRegionStatus();
}

- (NSString *)accessibilityARIARelevantStatus
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->ariaLiveRegionRelevant();
}

- (BOOL)accessibilityARIALiveRegionIsAtomic
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->ariaLiveRegionAtomic();
}

- (NSString *)accessibilityInvalidStatus
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return m_object->invalidStatus();
}

- (WebAccessibilityObjectWrapper *)accessibilityMathRootIndexObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathRootIndexObject() ? m_object->mathRootIndexObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathRadicandObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathRadicandObject() ? m_object->mathRadicandObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathNumeratorObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathNumeratorObject() ? m_object->mathNumeratorObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathDenominatorObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathDenominatorObject() ? m_object->mathDenominatorObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathBaseObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathBaseObject() ? m_object->mathBaseObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathSubscriptObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathSubscriptObject() ? m_object->mathSubscriptObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathSuperscriptObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathSuperscriptObject() ? m_object->mathSuperscriptObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathUnderObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathUnderObject() ? m_object->mathUnderObject()->wrapper() : 0;
}

- (WebAccessibilityObjectWrapper *)accessibilityMathOverObject
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathOverObject() ? m_object->mathOverObject()->wrapper() : 0;
}

- (NSString *)accessibilityPlatformMathSubscriptKey
{
    return @"AXMSubscriptObject";
}

- (NSString *)accessibilityPlatformMathSuperscriptKey
{
    return @"AXMSuperscriptObject";
}

- (NSArray *)accessibilityMathPostscripts
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return [self accessibilityMathPostscriptPairs];
}

- (NSArray *)accessibilityMathPrescripts
{
    if (![self _prepareAccessibilityCall])
        return nil;
    
    return [self accessibilityMathPrescriptPairs];
}

- (NSString *)accessibilityMathFencedOpenString
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathFencedOpenString();
}

- (NSString *)accessibilityMathFencedCloseString
{
    if (![self _prepareAccessibilityCall])
        return nil;

    return m_object->mathFencedCloseString();
}

- (BOOL)accessibilityIsMathTopObject
{
    if (![self _prepareAccessibilityCall])
        return NO;

    return m_object->roleValue() == DocumentMathRole;
}

- (NSInteger)accessibilityMathLineThickness
{
    if (![self _prepareAccessibilityCall])
        return 0;

    return m_object->mathLineThickness();
}

- (NSString *)accessibilityMathType
{
    if (![self _prepareAccessibilityCall])
        return nil;

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
    
    return nil;
}

- (CGPoint)accessibilityClickPoint
{
    return m_object->clickPoint();
}

// These are used by DRT so that it can know when notifications are sent.
// Since they are static, only one callback can be installed at a time (that's all DRT should need).
typedef void (*AXPostedNotificationCallback)(id element, NSString* notification, void* context);
static AXPostedNotificationCallback AXNotificationCallback = 0;
static void* AXPostedNotificationContext = 0;

- (void)accessibilitySetPostedNotificationCallback:(AXPostedNotificationCallback)function withContext:(void*)context
{
    AXNotificationCallback = function;
    AXPostedNotificationContext = context;
}

- (void)accessibilityPostedNotification:(NSString *)notificationName
{
    if (AXNotificationCallback && notificationName)
        AXNotificationCallback(self, notificationName, AXPostedNotificationContext);
}

#ifndef NDEBUG
- (NSString *)description
{
    CGRect frame = [self accessibilityFrame];
    return [NSString stringWithFormat:@"Role: (%d) - Text: %@: Value: %@ -- Frame: %f %f %f %f", m_object ? m_object->roleValue() : 0, [self accessibilityLabel], [self accessibilityValue], frame.origin.x, frame.origin.y, frame.size.width, frame.size.height];
}
#endif

@end

#endif // HAVE(ACCESSIBILITY) && PLATFORM(IOS)

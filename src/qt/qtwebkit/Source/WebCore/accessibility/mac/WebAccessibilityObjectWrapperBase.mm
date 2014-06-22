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
#import "WebAccessibilityObjectWrapperBase.h"

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

static NSArray *convertMathPairsToNSArray(const AccessibilityObject::AccessibilityMathMultiscriptPairs& pairs, NSString *subscriptKey, NSString *superscriptKey)
{
    unsigned length = pairs.size();
    NSMutableArray *array = [NSMutableArray arrayWithCapacity:length];
    for (unsigned i = 0; i < length; ++i) {
        NSMutableDictionary *pairDictionary = [NSMutableDictionary dictionary];
        pair<AccessibilityObject*, AccessibilityObject*> pair = pairs[i];
        if (pair.first && pair.first->wrapper() && !pair.first->accessibilityIsIgnored())
            [pairDictionary setObject:pair.first->wrapper() forKey:subscriptKey];
        if (pair.second && pair.second->wrapper() && !pair.second->accessibilityIsIgnored())
            [pairDictionary setObject:pair.second->wrapper() forKey:superscriptKey];
        [array addObject:pairDictionary];
    }
    return array;
}

@implementation WebAccessibilityObjectWrapperBase

- (id)initWithAccessibilityObject:(AccessibilityObject*)axObject
{
    if (!(self = [super init]))
        return nil;

    m_object = axObject;
    return self;
}

- (void)detach
{
    m_object = 0;
}

- (BOOL)updateObjectBackingStore
{
    // Calling updateBackingStore() can invalidate this element so self must be retained.
    // If it does become invalidated, m_object will be nil.
    [[self retain] autorelease];
    
    if (!m_object)
        return NO;
    
    m_object->updateBackingStore();
    if (!m_object)
        return NO;
    
    return YES;
}

- (id)attachmentView
{
    return nil;
}

- (AccessibilityObject*)accessibilityObject
{
    return m_object;
}

// FIXME: Different kinds of elements are putting the title tag to use in different
// AX fields. This should be rectified, but in the initial patch I want to achieve
// parity with existing behavior.
- (BOOL)titleTagShouldBeUsedInDescriptionField
{
    return (m_object->isLink() && !m_object->isImageMapLink()) || m_object->isImage();
}

// On iOS, we don't have to return the value in the title. We can return the actual title, given the API.
- (BOOL)fileUploadButtonReturnsValueInTitle
{
    return YES;
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
    if (m_object->isFileUploadButton() && [self fileUploadButtonReturnsValueInTitle])
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

struct PathConversionInfo {
    WebAccessibilityObjectWrapperBase *wrapper;
    CGMutablePathRef path;
};

static void ConvertPathToScreenSpaceFunction(void* info, const PathElement* element)
{
    PathConversionInfo* conversion = (PathConversionInfo*)info;
    WebAccessibilityObjectWrapperBase *wrapper = conversion->wrapper;
    CGMutablePathRef newPath = conversion->path;
    switch (element->type) {
    case PathElementMoveToPoint:
    {
        CGPoint newPoint = [wrapper convertPointToScreenSpace:element->points[0]];
        CGPathMoveToPoint(newPath, nil, newPoint.x, newPoint.y);
        break;
    }
    case PathElementAddLineToPoint:
    {
        CGPoint newPoint = [wrapper convertPointToScreenSpace:element->points[0]];
        CGPathAddLineToPoint(newPath, nil, newPoint.x, newPoint.y);
        break;
    }
    case PathElementAddQuadCurveToPoint:
    {
        CGPoint newPoint1 = [wrapper convertPointToScreenSpace:element->points[0]];
        CGPoint newPoint2 = [wrapper convertPointToScreenSpace:element->points[1]];
        CGPathAddQuadCurveToPoint(newPath, nil, newPoint1.x, newPoint1.y, newPoint2.x, newPoint2.y);
        break;
    }
    case PathElementAddCurveToPoint:
    {
        CGPoint newPoint1 = [wrapper convertPointToScreenSpace:element->points[0]];
        CGPoint newPoint2 = [wrapper convertPointToScreenSpace:element->points[1]];
        CGPoint newPoint3 = [wrapper convertPointToScreenSpace:element->points[2]];
        CGPathAddCurveToPoint(newPath, nil, newPoint1.x, newPoint1.y, newPoint2.x, newPoint2.y, newPoint3.x, newPoint3.y);
        break;
    }
    case PathElementCloseSubpath:
    {
        CGPathCloseSubpath(newPath);
        break;
    }
    }
}

- (CGPathRef)convertPathToScreenSpace:(Path &)path
{
    PathConversionInfo conversion = { self, CGPathCreateMutable() };
    path.apply(&conversion, ConvertPathToScreenSpaceFunction);    
    return (CGPathRef)[(id)conversion.path autorelease];
}

- (CGPoint)convertPointToScreenSpace:(FloatPoint &)point
{
    UNUSED_PARAM(point);
    ASSERT_NOT_REACHED();
    return CGPointZero;
}

- (NSString *)ariaLandmarkRoleDescription
{
    switch (m_object->roleValue()) {
    case LandmarkApplicationRole:
        return AXARIAContentGroupText(@"ARIALandmarkApplication");
    case LandmarkBannerRole:
        return AXARIAContentGroupText(@"ARIALandmarkBanner");
    case LandmarkComplementaryRole:
        return AXARIAContentGroupText(@"ARIALandmarkComplementary");
    case LandmarkContentInfoRole:
        return AXARIAContentGroupText(@"ARIALandmarkContentInfo");
    case LandmarkMainRole:
        return AXARIAContentGroupText(@"ARIALandmarkMain");
    case LandmarkNavigationRole:
        return AXARIAContentGroupText(@"ARIALandmarkNavigation");
    case LandmarkSearchRole:
        return AXARIAContentGroupText(@"ARIALandmarkSearch");
    case ApplicationAlertRole:
        return AXARIAContentGroupText(@"ARIAApplicationAlert");
    case ApplicationAlertDialogRole:
        return AXARIAContentGroupText(@"ARIAApplicationAlertDialog");
    case ApplicationDialogRole:
        return AXARIAContentGroupText(@"ARIAApplicationDialog");
    case ApplicationLogRole:
        return AXARIAContentGroupText(@"ARIAApplicationLog");
    case ApplicationMarqueeRole:
        return AXARIAContentGroupText(@"ARIAApplicationMarquee");
    case ApplicationStatusRole:
        return AXARIAContentGroupText(@"ARIAApplicationStatus");
    case ApplicationTimerRole:
        return AXARIAContentGroupText(@"ARIAApplicationTimer");
    case DocumentRole:
        return AXARIAContentGroupText(@"ARIADocument");
    case DocumentArticleRole:
        return AXARIAContentGroupText(@"ARIADocumentArticle");
    case DocumentMathRole:
        return AXARIAContentGroupText(@"ARIADocumentMath");
    case DocumentNoteRole:
        return AXARIAContentGroupText(@"ARIADocumentNote");
    case DocumentRegionRole:
        return AXARIAContentGroupText(@"ARIADocumentRegion");
    case UserInterfaceTooltipRole:
        return AXARIAContentGroupText(@"ARIAUserInterfaceTooltip");
    case TabPanelRole:
        return AXARIAContentGroupText(@"ARIATabPanel");
    default:
        return nil;
    }
}

- (NSString *)accessibilityPlatformMathSubscriptKey
{
    ASSERT_NOT_REACHED();
    return nil;
}

- (NSString *)accessibilityPlatformMathSuperscriptKey
{
    ASSERT_NOT_REACHED();
    return nil;    
}

- (NSArray *)accessibilityMathPostscriptPairs
{
    AccessibilityObject::AccessibilityMathMultiscriptPairs pairs;
    m_object->mathPostscripts(pairs);
    return convertMathPairsToNSArray(pairs, [self accessibilityPlatformMathSubscriptKey], [self accessibilityPlatformMathSuperscriptKey]);
}

- (NSArray *)accessibilityMathPrescriptPairs
{
    AccessibilityObject::AccessibilityMathMultiscriptPairs pairs;
    m_object->mathPrescripts(pairs);
    return convertMathPairsToNSArray(pairs, [self accessibilityPlatformMathSubscriptKey], [self accessibilityPlatformMathSuperscriptKey]);
}

// This is set by DRT when it wants to listen for notifications.
static BOOL accessibilityShouldRepostNotifications;
+ (void)accessibilitySetShouldRepostNotifications:(BOOL)repost
{
    accessibilityShouldRepostNotifications = repost;
}

- (void)accessibilityPostedNotification:(NSString *)notificationName
{
    if (accessibilityShouldRepostNotifications) {
        NSDictionary* userInfo = [NSDictionary dictionaryWithObjectsAndKeys:notificationName, @"notificationName", nil];
        [[NSNotificationCenter defaultCenter] postNotificationName:@"AXDRTNotification" object:self userInfo:userInfo];
    }
}

@end

#endif // HAVE(ACCESSIBILITY)

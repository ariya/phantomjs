/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "DumpRenderTree.h"
#import "AccessibilityCommonMac.h"
#import "AccessibilityNotificationHandler.h"
#import "AccessibilityUIElement.h"

#import <Foundation/Foundation.h>
#import <JavaScriptCore/JSRetainPtr.h>
#import <JavaScriptCore/JSStringRef.h>
#import <JavaScriptCore/JSStringRefCF.h>
#import <WebKit/WebFrame.h>
#import <WebKit/WebHTMLView.h>
#import <WebKit/WebTypesInternal.h>
#import <wtf/RetainPtr.h>
#import <wtf/Vector.h>


#ifndef NSAccessibilityOwnsAttribute
#define NSAccessibilityOwnsAttribute @"AXOwns"
#endif

#ifndef NSAccessibilityGrabbedAttribute
#define NSAccessibilityGrabbedAttribute @"AXGrabbed"
#endif

#ifndef NSAccessibilityDropEffectsAttribute
#define NSAccessibilityDropEffectsAttribute @"AXDropEffects"
#endif

#ifndef NSAccessibilityPathAttribute
#define NSAccessibilityPathAttribute @"AXPath"
#endif

typedef void (*AXPostedNotificationCallback)(id element, NSString* notification, void* context);

@interface NSObject (WebKitAccessibilityAdditions)
- (NSArray *)accessibilityArrayAttributeValues:(NSString *)attribute index:(NSUInteger)index maxCount:(NSUInteger)maxCount;
- (NSUInteger)accessibilityIndexOfChild:(id)child;
- (NSUInteger)accessibilityArrayAttributeCount:(NSString *)attribute;
@end

AccessibilityUIElement::AccessibilityUIElement(PlatformUIElement element)
    : m_element(element)
    , m_notificationHandler(0)
{
    // FIXME: ap@webkit.org says ObjC objects need to be CFRetained/CFRelease to be GC-compliant on the mac.
    [m_element retain];
}

AccessibilityUIElement::AccessibilityUIElement(const AccessibilityUIElement& other)
    : m_element(other.m_element)
    , m_notificationHandler(0)
{
    [m_element retain];
}

AccessibilityUIElement::~AccessibilityUIElement()
{
    // The notification handler should be nil because removeNotificationListener() should have been called in the test.
    ASSERT(!m_notificationHandler);
    [m_element release];
}

static NSString* descriptionOfValue(id valueObject, id focusedAccessibilityObject)
{
    if (!valueObject)
        return NULL;

    if ([valueObject isKindOfClass:[NSArray class]])
        return [NSString stringWithFormat:@"<array of size %lu>", static_cast<unsigned long>([(NSArray*)valueObject count])];

    if ([valueObject isKindOfClass:[NSNumber class]])
        return [(NSNumber*)valueObject stringValue];

    if ([valueObject isKindOfClass:[NSValue class]]) {
        NSString* type = [NSString stringWithCString:[valueObject objCType] encoding:NSASCIIStringEncoding];
        NSValue* value = (NSValue*)valueObject;
        if ([type rangeOfString:@"NSRect"].length > 0)
            return [NSString stringWithFormat:@"NSRect: %@", NSStringFromRect([value rectValue])];
        if ([type rangeOfString:@"NSPoint"].length > 0)
            return [NSString stringWithFormat:@"NSPoint: %@", NSStringFromPoint([value pointValue])];
        if ([type rangeOfString:@"NSSize"].length > 0)
            return [NSString stringWithFormat:@"NSSize: %@", NSStringFromSize([value sizeValue])];
        if ([type rangeOfString:@"NSRange"].length > 0)
            return [NSString stringWithFormat:@"NSRange: %@", NSStringFromRange([value rangeValue])];
    }

    // Strip absolute URL paths
    NSString* description = [valueObject description];
    NSRange range = [description rangeOfString:@"LayoutTests"];
    if (range.length)
        return [description substringFromIndex:range.location];

    // Strip pointer locations
    if ([description rangeOfString:@"0x"].length) {
        NSString* role = [focusedAccessibilityObject accessibilityAttributeValue:NSAccessibilityRoleAttribute];
        NSString* title = [focusedAccessibilityObject accessibilityAttributeValue:NSAccessibilityTitleAttribute];
        if ([title length])
            return [NSString stringWithFormat:@"<%@: '%@'>", role, title];
        return [NSString stringWithFormat:@"<%@>", role];
    }
    
    return [valueObject description];
}

static NSString* attributesOfElement(id accessibilityObject)
{
    NSArray* supportedAttributes = [accessibilityObject accessibilityAttributeNames];

    NSMutableString* attributesString = [NSMutableString string];
    for (NSUInteger i = 0; i < [supportedAttributes count]; ++i) {
        NSString* attribute = [supportedAttributes objectAtIndex:i];
        
        // Right now, position provides useless and screen-specific information, so we do not
        // want to include it for the sake of universally passing tests.
        if ([attribute isEqualToString:@"AXPosition"])
            continue;
        
        // accessibilityAttributeValue: can throw an if an attribute is not returned.
        // For DumpRenderTree's purpose, we should ignore those exceptions
        BEGIN_AX_OBJC_EXCEPTIONS
        id valueObject = [accessibilityObject accessibilityAttributeValue:attribute];
        NSString* value = descriptionOfValue(valueObject, accessibilityObject);
        [attributesString appendFormat:@"%@: %@\n", attribute, value];
        END_AX_OBJC_EXCEPTIONS
    }
    
    return attributesString;
}

static JSStringRef concatenateAttributeAndValue(NSString* attribute, NSString* value)
{
    Vector<UniChar> buffer([attribute length]);
    [attribute getCharacters:buffer.data()];
    buffer.append(':');
    buffer.append(' ');

    Vector<UniChar> valueBuffer([value length]);
    [value getCharacters:valueBuffer.data()];
    buffer.appendVector(valueBuffer);

    return JSStringCreateWithCharacters(buffer.data(), buffer.size());
}

static void convertNSArrayToVector(NSArray* array, Vector<AccessibilityUIElement>& elementVector)
{
    NSUInteger count = [array count];
    for (NSUInteger i = 0; i < count; ++i)
        elementVector.append(AccessibilityUIElement([array objectAtIndex:i]));
}

static JSStringRef descriptionOfElements(Vector<AccessibilityUIElement>& elementVector)
{
    NSMutableString* allElementString = [NSMutableString string];
    size_t size = elementVector.size();
    for (size_t i = 0; i < size; ++i) {
        NSString* attributes = attributesOfElement(elementVector[i].platformUIElement());
        [allElementString appendFormat:@"%@\n------------\n", attributes];
    }
    
    return [allElementString createJSStringRef];
}

void AccessibilityUIElement::getLinkedUIElements(Vector<AccessibilityUIElement>& elementVector)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* linkedElements = [m_element accessibilityAttributeValue:NSAccessibilityLinkedUIElementsAttribute];
    convertNSArrayToVector(linkedElements, elementVector);
    END_AX_OBJC_EXCEPTIONS
}

void AccessibilityUIElement::getDocumentLinks(Vector<AccessibilityUIElement>& elementVector)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* linkElements = [m_element accessibilityAttributeValue:@"AXLinkUIElements"];
    convertNSArrayToVector(linkElements, elementVector);
    END_AX_OBJC_EXCEPTIONS
}

void AccessibilityUIElement::getChildren(Vector<AccessibilityUIElement>& elementVector)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* children = [m_element accessibilityAttributeValue:NSAccessibilityChildrenAttribute];
    convertNSArrayToVector(children, elementVector);
    END_AX_OBJC_EXCEPTIONS
}

void AccessibilityUIElement::getChildrenWithRange(Vector<AccessibilityUIElement>& elementVector, unsigned location, unsigned length)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* children = [m_element accessibilityArrayAttributeValues:NSAccessibilityChildrenAttribute index:location maxCount:length];
    convertNSArrayToVector(children, elementVector);
    END_AX_OBJC_EXCEPTIONS
}

int AccessibilityUIElement::childrenCount()
{
    Vector<AccessibilityUIElement> children;
    getChildren(children);
    
    return children.size();
}

AccessibilityUIElement AccessibilityUIElement::elementAtPoint(int x, int y)
{
    id element = [m_element accessibilityHitTest:NSMakePoint(x, y)];
    if (!element)
        return nil;
    
    return AccessibilityUIElement(element); 
}

unsigned AccessibilityUIElement::indexOfChild(AccessibilityUIElement* element)
{
    return [m_element accessibilityIndexOfChild:element->platformUIElement()];
}

AccessibilityUIElement AccessibilityUIElement::getChildAtIndex(unsigned index)
{
    Vector<AccessibilityUIElement> children;
    getChildrenWithRange(children, index, 1);

    if (children.size() == 1)
        return children[0];
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::linkedUIElementAtIndex(unsigned index)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* objects = [m_element accessibilityAttributeValue:NSAccessibilityLinkedUIElementsAttribute];
    if (index < [objects count])
        return [objects objectAtIndex:index];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaOwnsElementAtIndex(unsigned index)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* objects = [m_element accessibilityAttributeValue:NSAccessibilityOwnsAttribute];
    if (index < [objects count])
        return [objects objectAtIndex:index];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::ariaFlowToElementAtIndex(unsigned index)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* objects = [m_element accessibilityAttributeValue:NSAccessibilityLinkedUIElementsAttribute];
    if (index < [objects count])
        return [objects objectAtIndex:index];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::disclosedRowAtIndex(unsigned index)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* rows = [m_element accessibilityAttributeValue:NSAccessibilityDisclosedRowsAttribute];
    if (index < [rows count])
        return [rows objectAtIndex:index];
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

AccessibilityUIElement AccessibilityUIElement::selectedChildAtIndex(unsigned index) const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* array = [m_element accessibilityAttributeValue:NSAccessibilitySelectedChildrenAttribute];
    if (index < [array count])
        return [array objectAtIndex:index];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

unsigned AccessibilityUIElement::selectedChildrenCount() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    return [m_element accessibilityArrayAttributeCount:NSAccessibilitySelectedChildrenAttribute];
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

AccessibilityUIElement AccessibilityUIElement::selectedRowAtIndex(unsigned index)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* rows = [m_element accessibilityAttributeValue:NSAccessibilitySelectedRowsAttribute];
    if (index < [rows count])
        return [rows objectAtIndex:index];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::rowAtIndex(unsigned index)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* rows = [m_element accessibilityAttributeValue:NSAccessibilityRowsAttribute];
    if (index < [rows count])
        return [rows objectAtIndex:index];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::titleUIElement()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id accessibilityObject = [m_element accessibilityAttributeValue:NSAccessibilityTitleUIElementAttribute];
    if (accessibilityObject)
        return AccessibilityUIElement(accessibilityObject);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::parentElement()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id accessibilityObject = [m_element accessibilityAttributeValue:NSAccessibilityParentAttribute];
    if (accessibilityObject)
        return AccessibilityUIElement(accessibilityObject);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::disclosedByRow()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id accessibilityObject = [m_element accessibilityAttributeValue:NSAccessibilityDisclosedByRowAttribute];
    if (accessibilityObject)
        return AccessibilityUIElement(accessibilityObject);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfLinkedUIElements()
{
    Vector<AccessibilityUIElement> linkedElements;
    getLinkedUIElements(linkedElements);
    return descriptionOfElements(linkedElements);
}

JSStringRef AccessibilityUIElement::attributesOfDocumentLinks()
{
    Vector<AccessibilityUIElement> linkElements;
    getDocumentLinks(linkElements);
    return descriptionOfElements(linkElements);
}

JSStringRef AccessibilityUIElement::attributesOfChildren()
{
    Vector<AccessibilityUIElement> children;
    getChildren(children);
    return descriptionOfElements(children);
}

JSStringRef AccessibilityUIElement::allAttributes()
{
    NSString* attributes = attributesOfElement(m_element);
    return [attributes createJSStringRef];
}

JSStringRef AccessibilityUIElement::stringAttributeValue(JSStringRef attribute)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:[NSString stringWithJSStringRef:attribute]];
    if ([value isKindOfClass:[NSString class]])
        return [value createJSStringRef];
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

AccessibilityUIElement AccessibilityUIElement::uiElementAttributeValue(JSStringRef attribute) const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id uiElement = [m_element accessibilityAttributeValue:[NSString stringWithJSStringRef:attribute]];
    return AccessibilityUIElement(uiElement);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}


double AccessibilityUIElement::numberAttributeValue(JSStringRef attribute)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:[NSString stringWithJSStringRef:attribute]];
    if ([value isKindOfClass:[NSNumber class]])
        return [value doubleValue];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

bool AccessibilityUIElement::boolAttributeValue(JSStringRef attribute)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:[NSString stringWithJSStringRef:attribute]];
    if ([value isKindOfClass:[NSNumber class]])
        return [value boolValue];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

bool AccessibilityUIElement::isAttributeSettable(JSStringRef attribute)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    return [m_element accessibilityIsAttributeSettable:[NSString stringWithJSStringRef:attribute]];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

bool AccessibilityUIElement::isAttributeSupported(JSStringRef attribute)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    return [[m_element accessibilityAttributeNames] containsObject:[NSString stringWithJSStringRef:attribute]];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

JSStringRef AccessibilityUIElement::parameterizedAttributeNames()
{
    NSArray* supportedParameterizedAttributes = [m_element accessibilityParameterizedAttributeNames];
    
    NSMutableString* attributesString = [NSMutableString string];
    for (NSUInteger i = 0; i < [supportedParameterizedAttributes count]; ++i) {
        [attributesString appendFormat:@"%@\n", [supportedParameterizedAttributes objectAtIndex:i]];
    }
    
    return [attributesString createJSStringRef];
}

JSStringRef AccessibilityUIElement::role()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSString *role = descriptionOfValue([m_element accessibilityAttributeValue:NSAccessibilityRoleAttribute], m_element);
    return concatenateAttributeAndValue(@"AXRole", role);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::subrole()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSString* role = descriptionOfValue([m_element accessibilityAttributeValue:NSAccessibilitySubroleAttribute], m_element);
    return concatenateAttributeAndValue(@"AXSubrole", role);
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

JSStringRef AccessibilityUIElement::roleDescription()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSString* role = descriptionOfValue([m_element accessibilityAttributeValue:NSAccessibilityRoleDescriptionAttribute], m_element);
    return concatenateAttributeAndValue(@"AXRoleDescription", role);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::title()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSString* title = descriptionOfValue([m_element accessibilityAttributeValue:NSAccessibilityTitleAttribute], m_element);
    return concatenateAttributeAndValue(@"AXTitle", title);
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

JSStringRef AccessibilityUIElement::description()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id description = descriptionOfValue([m_element accessibilityAttributeValue:NSAccessibilityDescriptionAttribute], m_element);
    return concatenateAttributeAndValue(@"AXDescription", description);
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

JSStringRef AccessibilityUIElement::orientation() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id description = descriptionOfValue([m_element accessibilityAttributeValue:NSAccessibilityOrientationAttribute], m_element);
    return concatenateAttributeAndValue(@"AXOrientation", description);    
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

JSStringRef AccessibilityUIElement::stringValue()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id description = descriptionOfValue([m_element accessibilityAttributeValue:NSAccessibilityValueAttribute], m_element);
    return concatenateAttributeAndValue(@"AXValue", description);
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

JSStringRef AccessibilityUIElement::language()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id description = descriptionOfValue([m_element accessibilityAttributeValue:@"AXLanguage"], m_element);
    return concatenateAttributeAndValue(@"AXLanguage", description);
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

JSStringRef AccessibilityUIElement::helpText() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id description = descriptionOfValue([m_element accessibilityAttributeValue:NSAccessibilityHelpAttribute], m_element);
    return concatenateAttributeAndValue(@"AXHelp", description);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

double AccessibilityUIElement::x()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSValue* positionValue = [m_element accessibilityAttributeValue:NSAccessibilityPositionAttribute];
    return static_cast<double>([positionValue pointValue].x);    
    END_AX_OBJC_EXCEPTIONS
    
    return 0.0f;
}

double AccessibilityUIElement::y()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSValue* positionValue = [m_element accessibilityAttributeValue:NSAccessibilityPositionAttribute];
    return static_cast<double>([positionValue pointValue].y);    
    END_AX_OBJC_EXCEPTIONS
    
    return 0.0f;
}

double AccessibilityUIElement::width()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSValue* sizeValue = [m_element accessibilityAttributeValue:NSAccessibilitySizeAttribute];
    return static_cast<double>([sizeValue sizeValue].width);
    END_AX_OBJC_EXCEPTIONS
    
    return 0.0f;
}

double AccessibilityUIElement::height()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSValue* sizeValue = [m_element accessibilityAttributeValue:NSAccessibilitySizeAttribute];
    return static_cast<double>([sizeValue sizeValue].height);
    END_AX_OBJC_EXCEPTIONS
    
    return 0.0f;
}

double AccessibilityUIElement::clickPointX()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSValue* positionValue = [m_element accessibilityAttributeValue:@"AXClickPoint"];
    return static_cast<double>([positionValue pointValue].x);        
    END_AX_OBJC_EXCEPTIONS
    
    return 0.0f;
}

double AccessibilityUIElement::clickPointY()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSValue* positionValue = [m_element accessibilityAttributeValue:@"AXClickPoint"];
    return static_cast<double>([positionValue pointValue].y);
    END_AX_OBJC_EXCEPTIONS
    
    return 0.0f;
}

double AccessibilityUIElement::intValue() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityValueAttribute];
    if ([value isKindOfClass:[NSNumber class]])
        return [(NSNumber*)value doubleValue]; 
    END_AX_OBJC_EXCEPTIONS

    return 0.0f;
}

double AccessibilityUIElement::minValue()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityMinValueAttribute];
    if ([value isKindOfClass:[NSNumber class]])
        return [(NSNumber*)value doubleValue]; 
    END_AX_OBJC_EXCEPTIONS

    return 0.0f;
}

double AccessibilityUIElement::maxValue()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityMaxValueAttribute];
    if ([value isKindOfClass:[NSNumber class]])
        return [(NSNumber*)value doubleValue]; 
    END_AX_OBJC_EXCEPTIONS

    return 0.0;
}

JSStringRef AccessibilityUIElement::valueDescription()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSString* valueDescription = [m_element accessibilityAttributeValue:NSAccessibilityValueDescriptionAttribute];
    if ([valueDescription isKindOfClass:[NSString class]])
         return [valueDescription createJSStringRef];

    END_AX_OBJC_EXCEPTIONS
    return 0;
}

int AccessibilityUIElement::insertionPointLineNumber()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityInsertionPointLineNumberAttribute];
    if ([value isKindOfClass:[NSNumber class]])
        return [(NSNumber *)value intValue]; 
    END_AX_OBJC_EXCEPTIONS
    
    return -1;
}

bool AccessibilityUIElement::isPressActionSupported()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* actions = [m_element accessibilityActionNames];
    return [actions containsObject:NSAccessibilityPressAction];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

bool AccessibilityUIElement::isIncrementActionSupported()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* actions = [m_element accessibilityActionNames];
    return [actions containsObject:NSAccessibilityIncrementAction];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

bool AccessibilityUIElement::isDecrementActionSupported()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* actions = [m_element accessibilityActionNames];
    return [actions containsObject:NSAccessibilityDecrementAction];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

bool AccessibilityUIElement::isEnabled()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityEnabledAttribute];
    if ([value isKindOfClass:[NSNumber class]])
        return [value boolValue];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

bool AccessibilityUIElement::isRequired() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:@"AXRequired"];
    if ([value isKindOfClass:[NSNumber class]])
        return [value boolValue];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

bool AccessibilityUIElement::isFocused() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isSelected() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilitySelectedAttribute];
    if ([value isKindOfClass:[NSNumber class]])
        return [value boolValue];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

bool AccessibilityUIElement::isExpanded() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityExpandedAttribute];
    if ([value isKindOfClass:[NSNumber class]])
        return [value boolValue];
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

bool AccessibilityUIElement::isChecked() const
{
    // On the Mac, intValue()==1 if a a checkable control is checked.
    return intValue() == 1;
}

int AccessibilityUIElement::hierarchicalLevel() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityDisclosureLevelAttribute];
    if ([value isKindOfClass:[NSNumber class]])
        return [value intValue];
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

JSStringRef AccessibilityUIElement::speak()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:@"AXDRTSpeechAttribute"];
    if ([value isKindOfClass:[NSString class]])
        return [value createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
        
    return 0;
}

bool AccessibilityUIElement::ariaIsGrabbed() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityGrabbedAttribute];
    if ([value isKindOfClass:[NSNumber class]])
        return [value boolValue];
    END_AX_OBJC_EXCEPTIONS

    return false;
}

JSStringRef AccessibilityUIElement::ariaDropEffects() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityDropEffectsAttribute];
    if (![value isKindOfClass:[NSArray class]])
        return 0;

    NSMutableString* dropEffects = [NSMutableString string];
    NSInteger length = [value count];
    for (NSInteger k = 0; k < length; ++k) {
        [dropEffects appendString:[value objectAtIndex:k]];
        if (k < length - 1)
            [dropEffects appendString:@","];
    }
    
    return [dropEffects createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

// parameterized attributes
int AccessibilityUIElement::lineForIndex(int index)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityLineForIndexParameterizedAttribute forParameter:[NSNumber numberWithInt:index]];
    if ([value isKindOfClass:[NSNumber class]])
        return [(NSNumber *)value intValue]; 
    END_AX_OBJC_EXCEPTIONS

    return -1;
}

JSStringRef AccessibilityUIElement::rangeForLine(int line)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityRangeForLineParameterizedAttribute forParameter:[NSNumber numberWithInt:line]];
    if ([value isKindOfClass:[NSValue class]])
        return [NSStringFromRange([value rangeValue]) createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::rangeForPosition(int x, int y)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityRangeForPositionParameterizedAttribute forParameter:[NSValue valueWithPoint:NSMakePoint(x, y)]];
    if ([value isKindOfClass:[NSValue class]])
        return [NSStringFromRange([value rangeValue]) createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}


JSStringRef AccessibilityUIElement::boundsForRange(unsigned location, unsigned length)
{
    NSRange range = NSMakeRange(location, length);
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:NSAccessibilityBoundsForRangeParameterizedAttribute forParameter:[NSValue valueWithRange:range]];
    NSRect rect = NSMakeRect(0,0,0,0);
    if ([value isKindOfClass:[NSValue class]])
        rect = [value rectValue]; 
    
    // don't return position information because it is platform dependent
    NSMutableString* boundsDescription = [NSMutableString stringWithFormat:@"{{%f, %f}, {%f, %f}}",-1.0f,-1.0f,rect.size.width,rect.size.height];
    return [boundsDescription createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::stringForRange(unsigned location, unsigned length)
{
    NSRange range = NSMakeRange(location, length);
    BEGIN_AX_OBJC_EXCEPTIONS
    id string = [m_element accessibilityAttributeValue:NSAccessibilityStringForRangeParameterizedAttribute forParameter:[NSValue valueWithRange:range]];
    if (![string isKindOfClass:[NSString class]])
        return 0;
    
    return [string createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::attributedStringForRange(unsigned location, unsigned length)
{
    NSRange range = NSMakeRange(location, length);
    BEGIN_AX_OBJC_EXCEPTIONS
    NSAttributedString* string = [m_element accessibilityAttributeValue:NSAccessibilityAttributedStringForRangeParameterizedAttribute forParameter:[NSValue valueWithRange:range]];
    if (![string isKindOfClass:[NSAttributedString class]])
        return 0;
    
    NSString* stringWithAttrs = [string description];
    return [stringWithAttrs createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

bool AccessibilityUIElement::attributedStringRangeIsMisspelled(unsigned location, unsigned length)
{
    NSRange range = NSMakeRange(location, length);
    BEGIN_AX_OBJC_EXCEPTIONS
    NSAttributedString* string = [m_element accessibilityAttributeValue:NSAccessibilityAttributedStringForRangeParameterizedAttribute forParameter:[NSValue valueWithRange:range]];
    if (![string isKindOfClass:[NSAttributedString class]])
        return false;

    NSDictionary* attrs = [string attributesAtIndex:0 effectiveRange:nil];
    BOOL misspelled = [[attrs objectForKey:NSAccessibilityMisspelledTextAttribute] boolValue];
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    if (misspelled)
        misspelled = [[attrs objectForKey:NSAccessibilityMarkedMisspelledTextAttribute] boolValue];
#endif
    return misspelled;
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

AccessibilityUIElement AccessibilityUIElement::uiElementForSearchPredicate(JSContextRef context, AccessibilityUIElement* startElement, bool isDirectionNext, JSValueRef searchKey, JSStringRef searchText, bool visibleOnly)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSMutableDictionary* parameter = [NSMutableDictionary dictionary];
    [parameter setObject:(isDirectionNext) ? @"AXDirectionNext" : @"AXDirectionPrevious" forKey:@"AXDirection"];
    if (visibleOnly)
        [parameter setObject:[NSNumber numberWithBool:YES] forKey:@"AXVisibleOnly"];
    [parameter setObject:[NSNumber numberWithInt:1] forKey:@"AXResultsLimit"];
    if (startElement && startElement->platformUIElement())
        [parameter setObject:(id)startElement->platformUIElement() forKey:@"AXStartElement"];
    if (searchKey) {
        if (JSValueIsString(context, searchKey)) {
            NSString *searchKeyParameter = nil;
            JSStringRef singleSearchKey = JSValueToStringCopy(context, searchKey, 0);
            if (singleSearchKey) {
                searchKeyParameter = [NSString stringWithJSStringRef:singleSearchKey];
                JSStringRelease(singleSearchKey);
                if (searchKeyParameter)
                    [parameter setObject:searchKeyParameter forKey:@"AXSearchKey"];
            }
        }
        else if (JSValueIsObject(context, searchKey)) {
            NSMutableArray *searchKeyParameter = nil;
            JSObjectRef array = const_cast<JSObjectRef>(searchKey);
            unsigned arrayLength = 0;
            JSRetainPtr<JSStringRef> arrayLengthString(Adopt, JSStringCreateWithUTF8CString("length"));
            JSValueRef arrayLengthValue = JSObjectGetProperty(context, array, arrayLengthString.get(), 0);
            if (arrayLengthValue && JSValueIsNumber(context, arrayLengthValue))
                arrayLength = static_cast<unsigned>(JSValueToNumber(context, arrayLengthValue, 0));
            
            for (unsigned i = 0; i < arrayLength; ++i) {
                JSValueRef exception = 0;
                JSValueRef value = JSObjectGetPropertyAtIndex(context, array, i, &exception);
                if (exception)
                    break;
                JSStringRef singleSearchKey = JSValueToStringCopy(context, value, &exception);
                if (exception)
                    break;
                if (singleSearchKey) {
                    if (!searchKeyParameter)
                        searchKeyParameter = [NSMutableArray array];
                    [searchKeyParameter addObject:[NSString stringWithJSStringRef:singleSearchKey]];
                    JSStringRelease(singleSearchKey);
                }
            }
            if (searchKeyParameter)
                [parameter setObject:searchKeyParameter forKey:@"AXSearchKey"];
        }
    }
    if (searchText && JSStringGetLength(searchText))
        [parameter setObject:[NSString stringWithJSStringRef:searchText] forKey:@"AXSearchText"];
    
    id uiElement = [[m_element accessibilityAttributeValue:@"AXUIElementsForSearchPredicate" forParameter:parameter] lastObject];
    return AccessibilityUIElement(uiElement);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfColumnHeaders()
{
    // not yet defined in AppKit... odd
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* columnHeadersArray = [m_element accessibilityAttributeValue:@"AXColumnHeaderUIElements"];
    Vector<AccessibilityUIElement> columnHeadersVector;
    convertNSArrayToVector(columnHeadersArray, columnHeadersVector);
    return descriptionOfElements(columnHeadersVector);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfRowHeaders()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* rowHeadersArray = [m_element accessibilityAttributeValue:@"AXRowHeaderUIElements"];
    Vector<AccessibilityUIElement> rowHeadersVector;
    convertNSArrayToVector(rowHeadersArray, rowHeadersVector);
    return descriptionOfElements(rowHeadersVector);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfColumns()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* columnsArray = [m_element accessibilityAttributeValue:NSAccessibilityColumnsAttribute];
    Vector<AccessibilityUIElement> columnsVector;
    convertNSArrayToVector(columnsArray, columnsVector);
    return descriptionOfElements(columnsVector);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfRows()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* rowsArray = [m_element accessibilityAttributeValue:NSAccessibilityRowsAttribute];
    Vector<AccessibilityUIElement> rowsVector;
    convertNSArrayToVector(rowsArray, rowsVector);
    return descriptionOfElements(rowsVector);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfVisibleCells()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* cellsArray = [m_element accessibilityAttributeValue:@"AXVisibleCells"];
    Vector<AccessibilityUIElement> cellsVector;
    convertNSArrayToVector(cellsArray, cellsVector);
    return descriptionOfElements(cellsVector);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::attributesOfHeader()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id headerObject = [m_element accessibilityAttributeValue:NSAccessibilityHeaderAttribute];
    if (!headerObject)
        return [@"" createJSStringRef];
    
    Vector<AccessibilityUIElement> headerVector;
    headerVector.append(headerObject);
    return descriptionOfElements(headerVector);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

int AccessibilityUIElement::rowCount()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    return [m_element accessibilityArrayAttributeCount:NSAccessibilityRowsAttribute];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

int AccessibilityUIElement::columnCount()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    return [m_element accessibilityArrayAttributeCount:NSAccessibilityColumnsAttribute];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

int AccessibilityUIElement::indexInTable()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSNumber* indexNumber = [m_element accessibilityAttributeValue:NSAccessibilityIndexAttribute];
    if (indexNumber)
        return [indexNumber intValue];
    END_AX_OBJC_EXCEPTIONS

    return -1;
}

JSStringRef AccessibilityUIElement::rowIndexRange()
{
    NSRange range = NSMakeRange(0, 0);
    BEGIN_AX_OBJC_EXCEPTIONS
    NSValue* indexRange = [m_element accessibilityAttributeValue:@"AXRowIndexRange"];
    if (indexRange)
        range = [indexRange rangeValue];
    NSMutableString* rangeDescription = [NSMutableString stringWithFormat:@"{%lu, %lu}", static_cast<unsigned long>(range.location), static_cast<unsigned long>(range.length)];
    return [rangeDescription createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::columnIndexRange()
{
    NSRange range = NSMakeRange(0, 0);
    BEGIN_AX_OBJC_EXCEPTIONS
    NSNumber* indexRange = [m_element accessibilityAttributeValue:@"AXColumnIndexRange"];
    if (indexRange)
        range = [indexRange rangeValue];
    NSMutableString* rangeDescription = [NSMutableString stringWithFormat:@"{%lu, %lu}",static_cast<unsigned long>(range.location), static_cast<unsigned long>(range.length)];
    return [rangeDescription createJSStringRef];    
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::cellForColumnAndRow(unsigned col, unsigned row)
{
    NSArray *colRowArray = [NSArray arrayWithObjects:[NSNumber numberWithUnsignedInt:col], [NSNumber numberWithUnsignedInt:row], nil];
    BEGIN_AX_OBJC_EXCEPTIONS
    return [m_element accessibilityAttributeValue:@"AXCellForColumnAndRow" forParameter:colRowArray];
    END_AX_OBJC_EXCEPTIONS    

    return 0;
}

AccessibilityUIElement AccessibilityUIElement::horizontalScrollbar() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    return AccessibilityUIElement([m_element accessibilityAttributeValue:NSAccessibilityHorizontalScrollBarAttribute]);
    END_AX_OBJC_EXCEPTIONS    
    
    return 0;
}

AccessibilityUIElement AccessibilityUIElement::verticalScrollbar() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    return AccessibilityUIElement([m_element accessibilityAttributeValue:NSAccessibilityVerticalScrollBarAttribute]);
    END_AX_OBJC_EXCEPTIONS        

    return 0;
}

JSStringRef AccessibilityUIElement::pathDescription() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSMutableString *result = [NSMutableString stringWithString:@"\nStart Path\n"];
    NSBezierPath *bezierPath = [m_element accessibilityAttributeValue:NSAccessibilityPathAttribute];
    
    NSUInteger elementCount = [bezierPath elementCount];
    for (NSUInteger i = 0; i < elementCount; i++) {
        switch ([bezierPath elementAtIndex:i]) {
        case NSMoveToBezierPathElement:
            [result appendString:@"\tMove to point\n"];
            break;
            
        case NSLineToBezierPathElement:
            [result appendString:@"\tLine to\n"];
            break;
            
        case NSCurveToBezierPathElement:
            [result appendString:@"\tCurve to\n"];
            break;
            
        case NSClosePathBezierPathElement:
            [result appendString:@"\tClose\n"];
            break;
        }
    }
    
    return [result createJSStringRef];
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

JSStringRef AccessibilityUIElement::selectedTextRange()
{
    NSRange range = NSMakeRange(NSNotFound, 0);
    BEGIN_AX_OBJC_EXCEPTIONS
    NSValue *indexRange = [m_element accessibilityAttributeValue:NSAccessibilitySelectedTextRangeAttribute];
    if (indexRange)
        range = [indexRange rangeValue];
    NSMutableString *rangeDescription = [NSMutableString stringWithFormat:@"{%lu, %lu}", static_cast<unsigned long>(range.location), static_cast<unsigned long>(range.length)];
    return [rangeDescription createJSStringRef];    
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

void AccessibilityUIElement::setSelectedTextRange(unsigned location, unsigned length)
{
    NSRange textRange = NSMakeRange(location, length);
    NSValue *textRangeValue = [NSValue valueWithRange:textRange];
    BEGIN_AX_OBJC_EXCEPTIONS
    [m_element accessibilitySetValue:textRangeValue forAttribute:NSAccessibilitySelectedTextRangeAttribute];
    END_AX_OBJC_EXCEPTIONS
}

void AccessibilityUIElement::increment()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    [m_element accessibilityPerformAction:NSAccessibilityIncrementAction];
    END_AX_OBJC_EXCEPTIONS
}

void AccessibilityUIElement::decrement()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    [m_element accessibilityPerformAction:NSAccessibilityDecrementAction];
    END_AX_OBJC_EXCEPTIONS
}

void AccessibilityUIElement::showMenu()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    [m_element accessibilityPerformAction:NSAccessibilityShowMenuAction];
    END_AX_OBJC_EXCEPTIONS
}

void AccessibilityUIElement::press()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    [m_element accessibilityPerformAction:NSAccessibilityPressAction];
    END_AX_OBJC_EXCEPTIONS
}

void AccessibilityUIElement::setSelectedChild(AccessibilityUIElement* element) const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* array = [NSArray arrayWithObject:element->platformUIElement()];
    [m_element accessibilitySetValue:array forAttribute:NSAccessibilitySelectedChildrenAttribute];
    END_AX_OBJC_EXCEPTIONS    
}

JSStringRef AccessibilityUIElement::accessibilityValue() const
{
    // FIXME: implement
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::documentEncoding()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::documentURI()
{
    return JSStringCreateWithCharacters(0, 0);
}

JSStringRef AccessibilityUIElement::url()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSURL *url = [m_element accessibilityAttributeValue:NSAccessibilityURLAttribute];
    return [[url absoluteString] createJSStringRef];    
    END_AX_OBJC_EXCEPTIONS
    
    return nil;
}

bool AccessibilityUIElement::addNotificationListener(JSObjectRef functionCallback)
{
    if (!functionCallback)
        return false;
 
    // Mac programmers should not be adding more than one notification listener per element.
    // Other platforms may be different.
    if (m_notificationHandler)
        return false;
    m_notificationHandler = [[AccessibilityNotificationHandler alloc] init];
    [m_notificationHandler setPlatformElement:platformUIElement()];
    [m_notificationHandler setCallback:functionCallback];
    [m_notificationHandler startObserving];

    return true;
}

void AccessibilityUIElement::removeNotificationListener()
{
    // Mac programmers should not be trying to remove a listener that's already removed.
    ASSERT(m_notificationHandler);

    [m_notificationHandler release];
    m_notificationHandler = nil;
}

bool AccessibilityUIElement::isFocusable() const
{
    bool result = false;
    BEGIN_AX_OBJC_EXCEPTIONS
    result = [m_element accessibilityIsAttributeSettable:NSAccessibilityFocusedAttribute];
    END_AX_OBJC_EXCEPTIONS
    
    return result;
}

bool AccessibilityUIElement::isSelectable() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isMultiSelectable() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isSelectedOptionActive() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isVisible() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isOffScreen() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isCollapsed() const
{
    // FIXME: implement
    return false;
}

bool AccessibilityUIElement::isIgnored() const
{
    BOOL result = NO;
    BEGIN_AX_OBJC_EXCEPTIONS
    result = [m_element accessibilityIsIgnored];
    END_AX_OBJC_EXCEPTIONS
    return result;
}

bool AccessibilityUIElement::hasPopup() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id value = [m_element accessibilityAttributeValue:@"AXHasPopup"];
    if ([value isKindOfClass:[NSNumber class]])
        return [value boolValue];
    END_AX_OBJC_EXCEPTIONS

    return false;
}

void AccessibilityUIElement::takeFocus()
{
    // FIXME: implement
}

void AccessibilityUIElement::takeSelection()
{
    // FIXME: implement
}

void AccessibilityUIElement::addSelection()
{
    // FIXME: implement
}

void AccessibilityUIElement::removeSelection()
{
    // FIXME: implement
}

#if SUPPORTS_AX_TEXTMARKERS

// Text markers
AccessibilityTextMarkerRange AccessibilityUIElement::textMarkerRangeForElement(AccessibilityUIElement* element)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id textMarkerRange = [m_element accessibilityAttributeValue:@"AXTextMarkerRangeForUIElement" forParameter:element->platformUIElement()];
    return AccessibilityTextMarkerRange(textMarkerRange);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

int AccessibilityUIElement::textMarkerRangeLength(AccessibilityTextMarkerRange* range)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSNumber* lengthValue = [m_element accessibilityAttributeValue:@"AXLengthForTextMarkerRange" forParameter:(id)range->platformTextMarkerRange()];
    return [lengthValue intValue];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

bool AccessibilityUIElement::attributedStringForTextMarkerRangeContainsAttribute(JSStringRef attribute, AccessibilityTextMarkerRange* range)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSAttributedString* string = [m_element accessibilityAttributeValue:@"AXAttributedStringForTextMarkerRange" forParameter:(id)range->platformTextMarkerRange()];
    if (![string isKindOfClass:[NSAttributedString class]])
        return false;
    
    NSDictionary* attrs = [string attributesAtIndex:0 effectiveRange:nil];
    if ([attrs objectForKey:[NSString stringWithJSStringRef:attribute]])
        return true;    
    END_AX_OBJC_EXCEPTIONS
    
    return false;
}

int AccessibilityUIElement::indexForTextMarker(AccessibilityTextMarker* marker)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSNumber* indexNumber = [m_element accessibilityAttributeValue:@"AXIndexForTextMarker" forParameter:(id)marker->platformTextMarker()];
    return [indexNumber intValue];
    END_AX_OBJC_EXCEPTIONS
    
    return -1;
}

AccessibilityTextMarker AccessibilityUIElement::textMarkerForIndex(int textIndex)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id textMarker = [m_element accessibilityAttributeValue:@"AXTextMarkerForIndex" forParameter:[NSNumber numberWithInteger:textIndex]];
    return AccessibilityTextMarker(textMarker);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

bool AccessibilityUIElement::isTextMarkerValid(AccessibilityTextMarker* textMarker)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSNumber* validNumber = [m_element accessibilityAttributeValue:@"AXTextMarkerIsValid" forParameter:(id)textMarker->platformTextMarker()];
    return [validNumber boolValue];
    END_AX_OBJC_EXCEPTIONS

    return false;
}

AccessibilityTextMarker AccessibilityUIElement::previousTextMarker(AccessibilityTextMarker* textMarker)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id previousMarker = [m_element accessibilityAttributeValue:@"AXPreviousTextMarkerForTextMarker" forParameter:(id)textMarker->platformTextMarker()];
    return AccessibilityTextMarker(previousMarker);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityTextMarker AccessibilityUIElement::nextTextMarker(AccessibilityTextMarker* textMarker)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id nextMarker = [m_element accessibilityAttributeValue:@"AXNextTextMarkerForTextMarker" forParameter:(id)textMarker->platformTextMarker()];
    return AccessibilityTextMarker(nextMarker);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::stringForTextMarkerRange(AccessibilityTextMarkerRange* markerRange)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id textString = [m_element accessibilityAttributeValue:@"AXStringForTextMarkerRange" forParameter:(id)markerRange->platformTextMarkerRange()];
    return [textString createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityTextMarkerRange AccessibilityUIElement::textMarkerRangeForMarkers(AccessibilityTextMarker* startMarker, AccessibilityTextMarker* endMarker)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray* textMarkers = [NSArray arrayWithObjects:(id)startMarker->platformTextMarker(), (id)endMarker->platformTextMarker(), nil];
    id textMarkerRange = [m_element accessibilityAttributeValue:@"AXTextMarkerRangeForUnorderedTextMarkers" forParameter:textMarkers];
    return AccessibilityTextMarkerRange(textMarkerRange);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

AccessibilityTextMarker AccessibilityUIElement::startTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange* range)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id textMarker = [m_element accessibilityAttributeValue:@"AXStartTextMarkerForTextMarkerRange" forParameter:(id)range->platformTextMarkerRange()];
    return AccessibilityTextMarker(textMarker);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;    
}

AccessibilityTextMarker AccessibilityUIElement::endTextMarkerForTextMarkerRange(AccessibilityTextMarkerRange* range)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id textMarker = [m_element accessibilityAttributeValue:@"AXEndTextMarkerForTextMarkerRange" forParameter:(id)range->platformTextMarkerRange()];
    return AccessibilityTextMarker(textMarker);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;    
}

AccessibilityTextMarker AccessibilityUIElement::textMarkerForPoint(int x, int y)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id textMarker = [m_element accessibilityAttributeValue:@"AXTextMarkerForPosition" forParameter:[NSValue valueWithPoint:NSMakePoint(x, y)]];
    return AccessibilityTextMarker(textMarker);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;    
}

AccessibilityUIElement AccessibilityUIElement::accessibilityElementForTextMarker(AccessibilityTextMarker* marker)
{
    BEGIN_AX_OBJC_EXCEPTIONS
    id uiElement = [m_element accessibilityAttributeValue:@"AXUIElementForTextMarker" forParameter:(id)marker->platformTextMarker()];
    return AccessibilityUIElement(uiElement);
    END_AX_OBJC_EXCEPTIONS
    
    return 0;  
}

#endif // SUPPORTS_AX_TEXTMARKERS

JSStringRef AccessibilityUIElement::supportedActions()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray *names = [m_element accessibilityActionNames];
    return [[names componentsJoinedByString:@","] createJSStringRef];
    END_AX_OBJC_EXCEPTIONS

    return 0;
}

static NSString *convertMathMultiscriptPairsToString(NSArray *pairs)
{
    __block NSMutableString *result = [NSMutableString string];
    [pairs enumerateObjectsUsingBlock:^(id pair, NSUInteger index, BOOL *stop) {
        for (NSString *key in pair)
            [result appendFormat:@"\t%lu. %@ = %@\n", (unsigned long)index, key, [[pair objectForKey:key] accessibilityAttributeValue:NSAccessibilitySubroleAttribute]];
    }];
    
    return result;
}

JSStringRef AccessibilityUIElement::mathPostscriptsDescription() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray *pairs = [m_element accessibilityAttributeValue:@"AXMathPostscripts"];
    return [convertMathMultiscriptPairsToString(pairs) createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}

JSStringRef AccessibilityUIElement::mathPrescriptsDescription() const
{
    BEGIN_AX_OBJC_EXCEPTIONS
    NSArray *pairs = [m_element accessibilityAttributeValue:@"AXMathPrescripts"];
    return [convertMathMultiscriptPairsToString(pairs) createJSStringRef];
    END_AX_OBJC_EXCEPTIONS
    
    return 0;
}


void AccessibilityUIElement::scrollToMakeVisible()
{
    BEGIN_AX_OBJC_EXCEPTIONS
    [m_element accessibilityPerformAction:@"AXScrollToVisible"];
    END_AX_OBJC_EXCEPTIONS
}

void AccessibilityUIElement::scrollToMakeVisibleWithSubFocus(int x, int y, int width, int height)
{
    // FIXME: implement
}

void AccessibilityUIElement::scrollToGlobalPoint(int x, int y)
{
    // FIXME: implement
}

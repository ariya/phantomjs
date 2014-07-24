/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#import "AccessibilityObject.h"
#import "AccessibilityRenderObject.h"
#import "HTMLInputElement.h"
#import "RenderObject.h"
#import "WAKView.h"

#if HAVE(ACCESSIBILITY) && PLATFORM(IOS)

#import "WebAccessibilityObjectWrapperIOS.h"

@interface WAKView (iOSAccessibility)
- (BOOL)accessibilityIsIgnored;
@end

@implementation WAKView (iOSAccessibility)

- (BOOL)accessibilityIsIgnored
{
    return YES;
}

@end

namespace WebCore {
    
void AccessibilityObject::detachFromParent()
{
}

void AccessibilityObject::overrideAttachmentParent(AccessibilityObject*)
{
}
    
// In iPhone only code for now. It's debateable whether this is desired on all platforms.
int AccessibilityObject::accessibilityPasswordFieldLength()
{
    if (!isPasswordField())
        return 0;
    RenderObject* renderObject = static_cast<AccessibilityRenderObject*>(this)->renderer();
    
    if (!renderObject || !renderObject->node() || !renderObject->node()->isHTMLElement())
        return false;
    
    HTMLInputElement* inputElement = toHTMLInputElement(renderObject->node());
    if (!inputElement)
        return false;
    
    return inputElement->value().length();
}

bool AccessibilityObject::accessibilityIgnoreAttachment() const
{
    return [[wrapper() attachmentView] accessibilityIsIgnored];
}
    
AccessibilityObjectInclusion AccessibilityObject::accessibilityPlatformIncludesObject() const
{
    return DefaultBehavior;
}
    
} // WebCore

#endif // HAVE(ACCESSIBILITY) && PLATFORM(IOS)

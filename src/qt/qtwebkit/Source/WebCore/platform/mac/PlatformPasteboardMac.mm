/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#import "Color.h"
#import "KURL.h"
#import "PlatformPasteboard.h"

namespace WebCore {

PlatformPasteboard::PlatformPasteboard(const String& pasteboardName)
    : m_pasteboard([NSPasteboard pasteboardWithName:pasteboardName])
{
    ASSERT(pasteboardName);
}

void PlatformPasteboard::getTypes(Vector<String>& types)
{
    NSArray *pasteboardTypes = [m_pasteboard.get() types];
    
    for (NSUInteger i = 0; i < [pasteboardTypes count]; i++)
        types.append([pasteboardTypes objectAtIndex:i]);
}

PassRefPtr<SharedBuffer> PlatformPasteboard::bufferForType(const String& pasteboardType)
{
    NSData *data = [m_pasteboard.get() dataForType:pasteboardType];
    if (!data)
        return 0;
    return SharedBuffer::wrapNSData([[data copy] autorelease]);
}

void PlatformPasteboard::getPathnamesForType(Vector<String>& pathnames, const String& pasteboardType)
{
    NSArray* paths = [m_pasteboard.get() propertyListForType:pasteboardType];
    if ([paths isKindOfClass:[NSString class]]) {
        pathnames.append((NSString *)paths);
        return;        
    }
    for (NSUInteger i = 0; i < [paths count]; i++)
        pathnames.append([paths objectAtIndex:i]);
}

String PlatformPasteboard::stringForType(const String& pasteboardType)
{
    if (pasteboardType == String(NSURLPboardType))
        return [[NSURL URLFromPasteboard:m_pasteboard.get()] absoluteString];

    return [m_pasteboard.get() stringForType:pasteboardType];
}

int PlatformPasteboard::changeCount() const
{
    return [m_pasteboard.get() changeCount];
}

String PlatformPasteboard::uniqueName()
{
    return [[NSPasteboard pasteboardWithUniqueName] name];
}

Color PlatformPasteboard::color()
{
    NSColor *color = [NSColor colorFromPasteboard:m_pasteboard.get()];
    
    // The color may not be in an RGB colorspace. This commonly occurs when a color is 
    // dragged from the NSColorPanel grayscale picker.
    if ([[color colorSpace] colorSpaceModel] != NSRGBColorSpaceModel)
        color = [color colorUsingColorSpaceName:NSCalibratedRGBColorSpace];
    
    return makeRGBA((int)([color redComponent] * 255.0 + 0.5), (int)([color greenComponent] * 255.0 + 0.5), 
                    (int)([color blueComponent] * 255.0 + 0.5), (int)([color alphaComponent] * 255.0 + 0.5));    
}

KURL PlatformPasteboard::url()
{
    return [NSURL URLFromPasteboard:m_pasteboard.get()];
}

void PlatformPasteboard::copy(const String& fromPasteboard)
{
    NSPasteboard* pasteboard = [NSPasteboard pasteboardWithName:fromPasteboard];
    NSArray* types = [pasteboard types];
    
    [m_pasteboard.get() addTypes:types owner:nil];
    for (NSUInteger i = 0; i < [types count]; i++) {
        NSString* type = [types objectAtIndex:i];
        [m_pasteboard.get() setData:[pasteboard dataForType:type] forType:type];
    }    
}

void PlatformPasteboard::addTypes(const Vector<String>& pasteboardTypes)
{
    RetainPtr<NSMutableArray> types = adoptNS([[NSMutableArray alloc] init]);
    for (size_t i = 0; i < pasteboardTypes.size(); ++i)
        [types.get() addObject:pasteboardTypes[i]];

    [m_pasteboard.get() addTypes:types.get() owner:nil];
}

void PlatformPasteboard::setTypes(const Vector<String>& pasteboardTypes)
{
    if (pasteboardTypes.isEmpty()) {
        [m_pasteboard.get() declareTypes:nil owner:nil];
        return;
    }

    RetainPtr<NSMutableArray> types = adoptNS([[NSMutableArray alloc] init]);
    for (size_t i = 0; i < pasteboardTypes.size(); ++i)
        [types.get() addObject:pasteboardTypes[i]];

    [m_pasteboard.get() declareTypes:types.get() owner:nil];
}

void PlatformPasteboard::setBufferForType(PassRefPtr<SharedBuffer> buffer, const String& pasteboardType)
{
    [m_pasteboard.get() setData:buffer ? [buffer->createNSData() autorelease] : nil forType:pasteboardType];
}

void PlatformPasteboard::setPathnamesForType(const Vector<String>& pathnames, const String& pasteboardType)
{
    RetainPtr<NSMutableArray> paths = adoptNS([[NSMutableArray alloc] init]);    
    for (size_t i = 0; i < pathnames.size(); ++i)
        [paths.get() addObject: [NSArray arrayWithObject:pathnames[i]]];
    [m_pasteboard.get() setPropertyList:paths.get() forType:pasteboardType];
}

void PlatformPasteboard::setStringForType(const String& string, const String& pasteboardType)
{
    if (pasteboardType == String(NSURLPboardType))
        [[NSURL URLWithString:string] writeToPasteboard:m_pasteboard.get()];
    else
        [m_pasteboard.get() setString:string forType:pasteboardType];
}

}

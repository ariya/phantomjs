/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "ArgumentCodersMac.h"

#import "ArgumentCodersCF.h"
#import "ArgumentDecoder.h"
#import "ArgumentEncoder.h"
#import "WebCoreArgumentCoders.h"
#import <WebCore/ColorMac.h>

using namespace WebCore;

namespace CoreIPC {

enum NSType {
    NSAttributedStringType,
#if USE(APPKIT)
    NSColorType,
#endif
    NSDictionaryType,
    NSArrayType,
#if USE(APPKIT)
    NSFontType,
#endif
    NSNumberType,
    NSStringType,
    NSDateType,
    NSDataType,
    Unknown,
};

static NSType typeFromObject(id object)
{
    ASSERT(object);

    if ([object isKindOfClass:[NSAttributedString class]])
        return NSAttributedStringType;
#if USE(APPKIT)
    if ([object isKindOfClass:[NSColor class]])
        return NSColorType;
#endif
    if ([object isKindOfClass:[NSDictionary class]])
        return NSDictionaryType;
#if USE(APPKIT)
    if ([object isKindOfClass:[NSFont class]])
        return NSFontType;
#endif
    if ([object isKindOfClass:[NSNumber class]])
        return NSNumberType;
    if ([object isKindOfClass:[NSString class]])
        return NSStringType;
    if ([object isKindOfClass:[NSArray class]])
        return NSArrayType;
    if ([object isKindOfClass:[NSDate class]])
        return NSDateType;
    if ([object isKindOfClass:[NSData class]])
        return NSDataType;

    ASSERT_NOT_REACHED();
    return Unknown;
}

void encode(ArgumentEncoder& encoder, id object)
{
    NSType type = typeFromObject(object);
    encoder.encodeEnum(type);

    switch (type) {
    case NSAttributedStringType:
        encode(encoder, static_cast<NSAttributedString *>(object));
        return;
#if USE(APPKIT)
    case NSColorType:
        encode(encoder, static_cast<NSColor *>(object));
        return;
#endif
    case NSDictionaryType:
        encode(encoder, static_cast<NSDictionary *>(object));
        return;
#if USE(APPKIT)
    case NSFontType:
        encode(encoder, static_cast<NSFont *>(object));
        return;
#endif
    case NSNumberType:
        encode(encoder, static_cast<NSNumber *>(object));
        return;
    case NSStringType:
        encode(encoder, static_cast<NSString *>(object));
        return;
    case NSArrayType:
        encode(encoder, static_cast<NSArray *>(object));
        return;
    case NSDateType:
        encode(encoder, static_cast<NSDate *>(object));
        return;
    case NSDataType:
        encode(encoder, static_cast<NSData *>(object));
        return;
    case Unknown:
        break;
    }

    ASSERT_NOT_REACHED();
}

bool decode(ArgumentDecoder& decoder, RetainPtr<id>& result)
{
    NSType type;
    if (!decoder.decodeEnum(type))
        return false;

    switch (type) {
    case NSAttributedStringType: {
        RetainPtr<NSAttributedString> string;
        if (!decode(decoder, string))
            return false;
        result = string;
        return true;
    }
#if USE(APPKIT)
    case NSColorType: {
        RetainPtr<NSColor> color;
        if (!decode(decoder, color))
            return false;
        result = color;
        return true;
    }
#endif
    case NSDictionaryType: {
        RetainPtr<NSDictionary> dictionary;
        if (!decode(decoder, dictionary))
            return false;
        result = dictionary;
        return true;
    }
#if USE(APPKIT)
    case NSFontType: {
        RetainPtr<NSFont> font;
        if (!decode(decoder, font))
            return false;
        result = font;
        return true;
    }
#endif
    case NSNumberType: {
        RetainPtr<NSNumber> number;
        if (!decode(decoder, number))
            return false;
        result = number;
        return true;
    }
    case NSStringType: {
        RetainPtr<NSString> string;
        if (!decode(decoder, string))
            return false;
        result = string;
        return true;
    }
    case NSArrayType: {
        RetainPtr<NSArray> array;
        if (!decode(decoder, array))
            return false;
        result = array;
        return true;
    }
    case NSDateType: {
        RetainPtr<NSDate> date;
        if (!decode(decoder, date))
            return false;
        result = date;
        return true;
    }
    case NSDataType: {
        RetainPtr<NSData> data;
        if (!decode(decoder, data))
            return false;
        result = data;
        return true;
    }
    case Unknown:
        ASSERT_NOT_REACHED();
        return false;
    }

    return false;
}

void encode(ArgumentEncoder& encoder, NSAttributedString *string)
{
    // Even though NSAttributedString is toll free bridged with CFAttributedStringRef, attributes' values may be not, so we should stay within this file's code.

    NSString *plainString = [string string];
    NSUInteger length = [plainString length];
    CoreIPC::encode(encoder, plainString);

    Vector<pair<NSRange, RetainPtr<NSDictionary>>> ranges;

    NSUInteger position = 0;
    while (position < length) {
        // Collect ranges in a vector, becasue the total count should be encoded first.
        NSRange effectiveRange;
        RetainPtr<NSDictionary> attributesAtIndex = [string attributesAtIndex:position effectiveRange:&effectiveRange];
        ASSERT(effectiveRange.location == position);
        ASSERT(effectiveRange.length);
        ASSERT(NSMaxRange(effectiveRange) <= length);

        ranges.append(std::make_pair(effectiveRange, attributesAtIndex));

        position = NSMaxRange(effectiveRange);
    }

    encoder << static_cast<uint64_t>(ranges.size());

    for (size_t i = 0; i < ranges.size(); ++i) {
        encoder << static_cast<uint64_t>(ranges[i].first.location);
        encoder << static_cast<uint64_t>(ranges[i].first.length);
        CoreIPC::encode(encoder, ranges[i].second.get());
    }
}

bool decode(ArgumentDecoder& decoder, RetainPtr<NSAttributedString>& result)
{
    RetainPtr<NSString> plainString;
    if (!CoreIPC::decode(decoder, plainString))
        return false;

    NSUInteger stringLength = [plainString.get() length];

    RetainPtr<NSMutableAttributedString> resultString = adoptNS([[NSMutableAttributedString alloc] initWithString:plainString.get()]);

    uint64_t rangeCount;
    if (!decoder.decode(rangeCount))
        return false;

    while (rangeCount--) {
        uint64_t rangeLocation;
        uint64_t rangeLength;
        RetainPtr<NSDictionary> attributes;
        if (!decoder.decode(rangeLocation))
            return false;
        if (!decoder.decode(rangeLength))
            return false;

        ASSERT(rangeLocation + rangeLength > rangeLocation);
        ASSERT(rangeLocation + rangeLength <= stringLength);
        if (rangeLocation + rangeLength <= rangeLocation || rangeLocation + rangeLength > stringLength)
            return false;

        if (!CoreIPC::decode(decoder, attributes))
            return false;
        [resultString.get() addAttributes:attributes.get() range:NSMakeRange(rangeLocation, rangeLength)];
    }

    result = adoptNS(resultString.leakRef());
    return true;
}

#if USE(APPKIT)
void encode(ArgumentEncoder& encoder, NSColor *color)
{
    encoder << colorFromNSColor(color);
}

bool decode(ArgumentDecoder& decoder, RetainPtr<NSColor>& result)
{
    Color color;
    if (!decoder.decode(color))
        return false;

    result = nsColor(color);
    return true;
}
#endif

void encode(ArgumentEncoder& encoder, NSDictionary *dictionary)
{
    // Even though NSDictionary is toll free bridged with CFDictionaryRef, values may be not, so we should stay within this file's code.

    NSUInteger size = [dictionary count];
    NSArray *keys = [dictionary allKeys];
    NSArray *values = [dictionary allValues];

    encoder << static_cast<uint64_t>(size);

    for (NSUInteger i = 0; i < size; ++i) {
        id key = [keys objectAtIndex:i];
        id value = [values objectAtIndex:i];
        ASSERT(key);
        ASSERT([key isKindOfClass:[NSString class]]);
        ASSERT(value);

        // Ignore values we don't recognize.
        if (typeFromObject(value) == Unknown)
            continue;

        encode(encoder, (NSString *)key);
        encode(encoder, value);
    }
}

bool decode(ArgumentDecoder& decoder, RetainPtr<NSDictionary>& result)
{
    uint64_t size;
    if (!decoder.decode(size))
        return false;

    RetainPtr<NSMutableDictionary> dictionary = adoptNS([[NSMutableDictionary alloc] initWithCapacity:size]);
    for (uint64_t i = 0; i < size; ++i) {
        // Try to decode the key name.
        RetainPtr<NSString> key;
        if (!decode(decoder, key))
            return false;

        RetainPtr<id> value;
        if (!decode(decoder, value))
            return false;

        [dictionary.get() setObject:value.get() forKey:key.get()];
    }

    result = adoptNS(dictionary.leakRef());
    return true;
}

#if USE(APPKIT)
void encode(ArgumentEncoder& encoder, NSFont *font)
{
    // NSFont could use CTFontRef code if we had it in ArgumentCodersCF.
    encode(encoder, [[font fontDescriptor] fontAttributes]);
}

bool decode(ArgumentDecoder& decoder, RetainPtr<NSFont>& result)
{
    RetainPtr<NSDictionary> fontAttributes;
    if (!decode(decoder, fontAttributes))
        return false;

    NSFontDescriptor *fontDescriptor = [NSFontDescriptor fontDescriptorWithFontAttributes:fontAttributes.get()];
    result = [NSFont fontWithDescriptor:fontDescriptor size:0];

    return true;
}
#endif

void encode(ArgumentEncoder& encoder, NSNumber *number)
{
    encode(encoder, (CFNumberRef)number);
}

bool decode(ArgumentDecoder& decoder, RetainPtr<NSNumber>& result)
{
    RetainPtr<CFNumberRef> number;
    if (!decode(decoder, number))
        return false;

    result = adoptNS((NSNumber *)number.leakRef());
    return true;
}

void encode(ArgumentEncoder& encoder, NSString *string)
{
    encode(encoder, (CFStringRef)string);
}

bool decode(ArgumentDecoder& decoder, RetainPtr<NSString>& result)
{
    RetainPtr<CFStringRef> string;
    if (!decode(decoder, string))
        return false;

    result = adoptNS((NSString *)string.leakRef());
    return true;
}

void encode(ArgumentEncoder& encoder, NSArray *array)
{
    NSUInteger size = [array count];
    encoder << static_cast<uint64_t>(size);

    for (NSUInteger i = 0; i < size; ++i) {
        id value = [array objectAtIndex:i];

        // Ignore values we don't recognize.
        if (typeFromObject(value) == Unknown)
            continue;

        encode(encoder, value);
    }
}

bool decode(ArgumentDecoder& decoder, RetainPtr<NSArray>& result)
{
    uint64_t size;
    if (!decoder.decode(size))
        return false;

    RetainPtr<NSMutableArray> array = adoptNS([[NSMutableArray alloc] initWithCapacity:size]);
    for (uint64_t i = 0; i < size; ++i) {
        RetainPtr<id> value;
        if (!decode(decoder, value))
            return false;

        [array.get() addObject:value.get()];
    }

    result = adoptNS(array.leakRef());
    return true;
}

void encode(ArgumentEncoder& encoder, NSDate *date)
{
    encode(encoder, (CFDateRef)date);
}

bool decode(ArgumentDecoder& decoder, RetainPtr<NSDate>& result)
{
    RetainPtr<CFDateRef> date;
    if (!decode(decoder, date))
        return false;

    result = adoptNS((NSDate *)date.leakRef());
    return true;
}

void encode(ArgumentEncoder& encoder, NSData *data)
{
    encode(encoder, (CFDataRef)data);
}

bool decode(ArgumentDecoder& decoder, RetainPtr<NSData>& result)
{
    RetainPtr<CFDataRef> data;
    if (!decode(decoder, data))
        return false;

    result = adoptNS((NSData *)data.leakRef());
    return true;
}

} // namespace CoreIPC

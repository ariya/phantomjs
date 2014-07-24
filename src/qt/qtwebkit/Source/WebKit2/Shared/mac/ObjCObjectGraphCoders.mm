/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#import "ObjCObjectGraphCoders.h"

#import "ArgumentCodersMac.h"
#import "WKTypeRefWrapper.h"

// For UIProcess side encoding/decoding
#import "WKAPICast.h"
#import "WKBrowsingContextControllerInternal.h"
#import "WKBrowsingContextControllerPrivate.h"
#import "WebContextUserMessageCoders.h"
#import "WebPageProxy.h"
#import "WebProcessProxy.h"

// For WebProcess side encoding/decoding
#import "InjectedBundleUserMessageCoders.h"
#import "WKBundleAPICast.h"
#import "WKWebProcessPlugInBrowserContextControllerInternal.h"
#import "WKWebProcessPlugInBrowserContextControllerPrivate.h"
#import "WKWebProcessPlugInInternal.h"
#import "WebPage.h"
#import "WebProcess.h"

namespace WebKit {

enum WebKitNSType {
    NullType,
    NSDictionaryType,
    NSArrayType,
    NSStringType,
    NSNumberType,
    NSDateType,
    NSDataType,
#if defined(__LP64__) && defined(__clang__)
    WKBrowsingContextControllerType,
    WKTypeRefWrapperType,
#endif
    UnknownType,
};

static WebKitNSType typeFromObject(id object)
{
    ASSERT(object);

    if ([object isKindOfClass:[NSDictionary class]])
        return NSDictionaryType;
    if ([object isKindOfClass:[NSString class]])
        return NSStringType;
    if ([object isKindOfClass:[NSArray class]])
        return NSArrayType;
    if ([object isKindOfClass:[NSNumber class]])
        return NSNumberType;
    if ([object isKindOfClass:[NSDate class]])
        return NSDateType;
    if ([object isKindOfClass:[NSData class]])
        return NSDataType;
#if defined(__LP64__) && defined(__clang__)
    if ([object isKindOfClass:[WKBrowsingContextController class]] || [object isKindOfClass:[WKWebProcessPlugInBrowserContextController class]])
        return WKBrowsingContextControllerType;
    if ([object isKindOfClass:[WKTypeRefWrapper class]])
        return WKTypeRefWrapperType;
#endif

    return UnknownType;
}

template<typename Owner>
class ObjCObjectGraphEncoder {
public:
    bool baseEncode(CoreIPC::ArgumentEncoder& encoder, WebKitNSType& type) const
    {
        if (!m_root) {
            encoder << static_cast<uint32_t>(NullType);
            return true;
        }

        type = typeFromObject(m_root);
        if (type == UnknownType) {
            [NSException raise:NSInvalidArgumentException format:@"Can not encode objects of class type '%@'", static_cast<NSString *>(NSStringFromClass([m_root class]))];
        }

        encoder << static_cast<uint32_t>(type);

        switch (type) {
        case NSStringType: {
            CoreIPC::encode(encoder, static_cast<NSString *>(m_root));
            return true;
        }
        case NSArrayType: {
            NSArray *array = static_cast<NSArray *>(m_root);

            NSUInteger size = [array count];
            encoder << static_cast<uint64_t>(size);

            for (NSUInteger i = 0; i < size; ++i)
                encoder << Owner([array objectAtIndex:i]);
            return true;
        }
        case NSDictionaryType: {
            NSDictionary* dictionary = static_cast<NSDictionary *>(m_root);

            NSUInteger size = [dictionary count];
            encoder << static_cast<uint64_t>(size);

            NSArray *keys = [dictionary allKeys];
            NSArray *values = [dictionary allValues];
            for (NSUInteger i = 0; i < size; ++i) {
                encoder << Owner([keys objectAtIndex:i]);
                encoder << Owner([values objectAtIndex:i]);
            }

            return true;
        }
        case NSNumberType: {
            CoreIPC::encode(encoder, static_cast<NSNumber *>(m_root));
            return true;
        }
        case NSDateType: {
            CoreIPC::encode(encoder, static_cast<NSDate *>(m_root));
            return true;
        }
        case NSDataType: {
            CoreIPC::encode(encoder, static_cast<NSData *>(m_root));
            return true;
        }
        default:
            break;
        }

        return false;
    }

protected:
    ObjCObjectGraphEncoder(id root)
        : m_root(root)
    {
    }

    id m_root;
};

template<typename Owner>
class ObjCObjectGraphDecoder {
public:
    static bool baseDecode(CoreIPC::ArgumentDecoder& decoder, Owner& coder, WebKitNSType& type)
    {
        uint32_t typeAsUInt32;
        if (!decoder.decode(typeAsUInt32))
            return false;

        type = static_cast<WebKitNSType>(typeAsUInt32);

        switch (type) {
        case NSStringType: {
            RetainPtr<NSString> string;
            if (!CoreIPC::decode(decoder, string))
                return false;
            coder.m_root = string;
            break;
        }
        case NSArrayType: {
            uint64_t size;
            if (!decoder.decode(size))
                return false;

            RetainPtr<NSMutableArray> array = adoptNS([[NSMutableArray alloc] initWithCapacity:size]);
            for (uint64_t i = 0; i < size; ++i) {
                RetainPtr<id> value;
                Owner messageCoder(coder, value);
                if (!decoder.decode(messageCoder))
                    return false;

                [array.get() addObject:value.get()];
            }

            coder.m_root = array;
            break;
        }
        case NSDictionaryType: {
            uint64_t size;
            if (!decoder.decode(size))
                return false;

            RetainPtr<NSMutableDictionary> dictionary = adoptNS([[NSMutableDictionary alloc] initWithCapacity:size]);
            for (uint64_t i = 0; i < size; ++i) {
                // Try to decode the key name.
                RetainPtr<id> key;
                Owner keyMessageCoder(coder, key);
                if (!decoder.decode(keyMessageCoder))
                    return false;

                RetainPtr<id> value;
                Owner valueMessageCoder(coder, value);
                if (!decoder.decode(valueMessageCoder))
                    return false;

                [dictionary.get() setObject:value.get() forKey:key.get()];
            }

            coder.m_root = dictionary;
            break;
        }
        case NSNumberType: {
            RetainPtr<NSNumber> number;
            if (!CoreIPC::decode(decoder, number))
                return false;
            coder.m_root = number;
            break;
        }
        case NSDateType: {
            RetainPtr<NSDate> date;
            if (!CoreIPC::decode(decoder, date))
                return false;
            coder.m_root = date;
            break;
        }
        case NSDataType: {
            RetainPtr<NSData> data;
            if (!CoreIPC::decode(decoder, data))
                return false;
            coder.m_root = data;
            break;
        }
        default:
            break;
        }

        return true;
    }

protected:
    ObjCObjectGraphDecoder(RetainPtr<id>& root)
        : m_root(root)
    {
    }

    RetainPtr<id>& m_root;
};


// WebContext Additions

class WebContextObjCObjectGraphEncoderImpl : public ObjCObjectGraphEncoder<WebContextObjCObjectGraphEncoderImpl> {
public:
    typedef ObjCObjectGraphEncoder<WebContextObjCObjectGraphEncoderImpl> Base;

    explicit WebContextObjCObjectGraphEncoderImpl(id root)
        : Base(root)
    {
    }

    void encode(CoreIPC::ArgumentEncoder& encoder) const
    {
        WebKitNSType type = NullType;
        if (baseEncode(encoder, type))
            return;

        switch (type) {
#if defined(__LP64__) && defined(__clang__)
        case WKBrowsingContextControllerType: {
            WKBrowsingContextController *browsingContextController = static_cast<WKBrowsingContextController *>(m_root);

            encoder << toImpl(browsingContextController._pageRef)->pageID();
            break;
        }
        case WKTypeRefWrapperType: {
            WKTypeRefWrapper *wrapper = static_cast<WKTypeRefWrapper *>(m_root);
            encoder << WebContextUserMessageEncoder(toImpl(wrapper.object));
            break;
        }
#endif
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }
};


class WebContextObjCObjectGraphDecoderImpl : public ObjCObjectGraphDecoder<WebContextObjCObjectGraphDecoderImpl> {
public:
    typedef ObjCObjectGraphDecoder<WebContextObjCObjectGraphDecoderImpl> Base;

    WebContextObjCObjectGraphDecoderImpl(RetainPtr<id>& root, WebProcessProxy* process)
        : Base(root)
        , m_process(process)
    {
    }

    WebContextObjCObjectGraphDecoderImpl(WebContextObjCObjectGraphDecoderImpl& userMessageDecoder, RetainPtr<id>& root)
        : Base(root)
        , m_process(userMessageDecoder.m_process)
    {
    }

    static bool decode(CoreIPC::ArgumentDecoder& decoder, WebContextObjCObjectGraphDecoderImpl& coder)
    {
        WebKitNSType type = NullType;
        if (!Base::baseDecode(decoder, coder, type))
            return false;

        if (coder.m_root)
            return true;

        if (type == NullType || type == UnknownType) {
            coder.m_root = [NSNull null];
            return true;
        }

        switch (type) {
#if defined(__LP64__) && defined(__clang__)
        case WKBrowsingContextControllerType: {
            uint64_t pageID;
            if (!decoder.decode(pageID))
                return false;

            WebPageProxy* webPage = coder.m_process->webPage(pageID);
            if (!webPage)
                coder.m_root = [NSNull null];
            else 
                coder.m_root = [WKBrowsingContextController _browsingContextControllerForPageRef:toAPI(webPage)];
            break;
        }
        case WKTypeRefWrapperType: {
            RefPtr<APIObject> object;
            WebContextUserMessageDecoder objectDecoder(object, coder.m_process);
            if (!decoder.decode(objectDecoder))
                return false;
            coder.m_root = adoptNS([[WKTypeRefWrapper alloc] initWithObject:toAPI(object.get())]);
            break;
        }
#endif
        default:
            return false;
        }

        return true;
    }

private:
    WebProcessProxy* m_process;
};


// InjectedBundle Additions

class InjectedBundleObjCObjectGraphEncoderImpl : public ObjCObjectGraphEncoder<InjectedBundleObjCObjectGraphEncoderImpl> {
public:
    typedef ObjCObjectGraphEncoder<InjectedBundleObjCObjectGraphEncoderImpl> Base;

    explicit InjectedBundleObjCObjectGraphEncoderImpl(id root)
        : Base(root)
    {
    }

    void encode(CoreIPC::ArgumentEncoder& encoder) const
    {
        WebKitNSType type = NullType;
        if (baseEncode(encoder, type))
            return;

        switch (type) {
#if defined(__LP64__) && defined(__clang__)
        case WKBrowsingContextControllerType: {
            WKWebProcessPlugInBrowserContextController *browserContextController = static_cast<WKWebProcessPlugInBrowserContextController *>(m_root);

            encoder << toImpl(browserContextController._bundlePageRef)->pageID();
            break;
        }
        case WKTypeRefWrapperType: {
            WKTypeRefWrapper *wrapper = static_cast<WKTypeRefWrapper *>(m_root);
            encoder << InjectedBundleUserMessageEncoder(toImpl(wrapper.object));
        }
#endif
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }
};

class InjectedBundleObjCObjectGraphDecoderImpl : public ObjCObjectGraphDecoder<InjectedBundleObjCObjectGraphDecoderImpl> {
public:
    typedef ObjCObjectGraphDecoder<InjectedBundleObjCObjectGraphDecoderImpl> Base;

    InjectedBundleObjCObjectGraphDecoderImpl(RetainPtr<id>& root, WebProcess* process)
        : Base(root)
        , m_process(process)
    {
    }

    InjectedBundleObjCObjectGraphDecoderImpl(InjectedBundleObjCObjectGraphDecoderImpl& userMessageDecoder, RetainPtr<id>& root)
        : Base(root)
        , m_process(userMessageDecoder.m_process)
    {
    }

    static bool decode(CoreIPC::ArgumentDecoder& decoder, InjectedBundleObjCObjectGraphDecoderImpl& coder)
    {
        WebKitNSType type = NullType;
        if (!Base::baseDecode(decoder, coder, type))
            return false;

        if (coder.m_root)
            return true;

        if (type == NullType || type == UnknownType) {
            coder.m_root = [NSNull null];
            return true;
        }

        switch (type) {
#if defined(__LP64__) && defined(__clang__)
        case WKBrowsingContextControllerType: {
            uint64_t pageID;
            if (!decoder.decode(pageID))
                return false;

            WebPage* webPage = coder.m_process->webPage(pageID);
            if (!webPage)
                coder.m_root = [NSNull null];
            else 
                coder.m_root = [[WKWebProcessPlugInController _shared] _browserContextControllerForBundlePageRef:toAPI(webPage)];
            break;
        }
        case WKTypeRefWrapperType: {
            RefPtr<APIObject> object;
            InjectedBundleUserMessageDecoder objectDecoder(object);
            if (!decoder.decode(objectDecoder))
                return false;
            coder.m_root = adoptNS([[WKTypeRefWrapper alloc] initWithObject:toAPI(object.get())]);
            break;
        }
#endif
        default:
            return false;
        }

        return true;
    }

private:
    WebProcess* m_process;
};


// Adaptors

WebContextObjCObjectGraphEncoder::WebContextObjCObjectGraphEncoder(ObjCObjectGraph* objectGraph)
    : m_objectGraph(objectGraph)
{
}

void WebContextObjCObjectGraphEncoder::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << WebContextObjCObjectGraphEncoderImpl(m_objectGraph->rootObject());
}

WebContextObjCObjectGraphDecoder::WebContextObjCObjectGraphDecoder(RefPtr<ObjCObjectGraph>& objectGraph, WebProcessProxy* process)
    : m_objectGraph(objectGraph)
    , m_process(process)
{
}

bool WebContextObjCObjectGraphDecoder::decode(CoreIPC::ArgumentDecoder& decoder, WebContextObjCObjectGraphDecoder& coder)
{
    RetainPtr<id> root;
    WebContextObjCObjectGraphDecoderImpl coderImpl(root, coder.m_process);
    if (!decoder.decode(coderImpl))
        return false;

    coder.m_objectGraph = ObjCObjectGraph::create(root.get());
    return true;
}

InjectedBundleObjCObjectGraphEncoder::InjectedBundleObjCObjectGraphEncoder(ObjCObjectGraph* objectGraph)
    : m_objectGraph(objectGraph)
{
}

void InjectedBundleObjCObjectGraphEncoder::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << InjectedBundleObjCObjectGraphEncoderImpl(m_objectGraph->rootObject());
}

InjectedBundleObjCObjectGraphDecoder::InjectedBundleObjCObjectGraphDecoder(RefPtr<ObjCObjectGraph>& objectGraph, WebProcess* process)
    : m_objectGraph(objectGraph)
    , m_process(process)
{
}

bool InjectedBundleObjCObjectGraphDecoder::decode(CoreIPC::ArgumentDecoder& decoder, InjectedBundleObjCObjectGraphDecoder& coder)
{
    RetainPtr<id> root;
    InjectedBundleObjCObjectGraphDecoderImpl coderImpl(root, coder.m_process);
    if (!decoder.decode(coderImpl))
        return false;

    coder.m_objectGraph = ObjCObjectGraph::create(root.get());
    return true;
}

} // namespace WebKit

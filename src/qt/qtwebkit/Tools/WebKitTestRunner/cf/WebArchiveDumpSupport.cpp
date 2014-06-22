/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebArchiveDumpSupport.h"

#include <CFNetwork/CFNetwork.h>
#include <CoreFoundation/CoreFoundation.h>
#include <wtf/RetainPtr.h>

extern "C" {

CFURLRef CFURLResponseGetURL(CFURLResponseRef);
CFStringRef CFURLResponseGetMIMEType(CFURLResponseRef);
CFStringRef CFURLResponseGetTextEncodingName(CFURLResponseRef);
SInt64 CFURLResponseGetExpectedContentLength(CFURLResponseRef);
CFHTTPMessageRef CFURLResponseGetHTTPResponse(CFURLResponseRef);

CFTypeID CFURLResponseGetTypeID(void);

}

static void convertMIMEType(CFMutableStringRef mimeType)
{
    // Workaround for <rdar://problem/6234318> with Dashcode 2.0
    if (CFStringCompare(mimeType, CFSTR("application/x-javascript"), kCFCompareAnchored | kCFCompareCaseInsensitive) == kCFCompareEqualTo)
        CFStringReplaceAll(mimeType, CFSTR("text/javascript"));
}

static void convertWebResourceDataToString(CFMutableDictionaryRef resource)
{
    CFMutableStringRef mimeType = (CFMutableStringRef)CFDictionaryGetValue(resource, CFSTR("WebResourceMIMEType"));
    CFStringLowercase(mimeType, CFLocaleGetSystem());
    convertMIMEType(mimeType);

    CFArrayRef supportedMIMETypes = supportedNonImageMIMETypes();
    if (CFStringHasPrefix(mimeType, CFSTR("text/")) || CFArrayContainsValue(supportedMIMETypes, CFRangeMake(0, CFArrayGetCount(supportedMIMETypes)), mimeType)) {
        CFStringRef textEncodingName = static_cast<CFStringRef>(CFDictionaryGetValue(resource, CFSTR("WebResourceTextEncodingName")));
        CFStringEncoding stringEncoding;
        if (textEncodingName && CFStringGetLength(textEncodingName))
            stringEncoding = CFStringConvertIANACharSetNameToEncoding(textEncodingName);
        else
            stringEncoding = kCFStringEncodingUTF8;

        CFDataRef data = static_cast<CFDataRef>(CFDictionaryGetValue(resource, CFSTR("WebResourceData")));
        RetainPtr<CFStringRef> dataAsString = adoptCF(CFStringCreateFromExternalRepresentation(kCFAllocatorDefault, data, stringEncoding));
        if (dataAsString)
            CFDictionarySetValue(resource, CFSTR("WebResourceData"), dataAsString.get());
    }
}

static void normalizeHTTPResponseHeaderFields(CFMutableDictionaryRef fields)
{
    // Normalize headers
    if (CFDictionaryContainsKey(fields, CFSTR("Date")))
        CFDictionarySetValue(fields, CFSTR("Date"), CFSTR("Sun, 16 Nov 2008 17:00:00 GMT"));
    if (CFDictionaryContainsKey(fields, CFSTR("Last-Modified")))
        CFDictionarySetValue(fields, CFSTR("Last-Modified"), CFSTR("Sun, 16 Nov 2008 16:55:00 GMT"));
    if (CFDictionaryContainsKey(fields, CFSTR("Etag")))
        CFDictionarySetValue(fields, CFSTR("Etag"), CFSTR("\"301925-21-45c7d72d3e780\""));
    if (CFDictionaryContainsKey(fields, CFSTR("Server")))
        CFDictionarySetValue(fields, CFSTR("Server"), CFSTR("Apache/2.2.9 (Unix) mod_ssl/2.2.9 OpenSSL/0.9.7l PHP/5.2.6"));

    // Remove headers
    CFDictionaryRemoveValue(fields, CFSTR("Connection"));
    CFDictionaryRemoveValue(fields, CFSTR("Keep-Alive"));
}

static void normalizeWebResourceURL(CFMutableStringRef webResourceURL)
{
    static CFIndex fileUrlLength = CFStringGetLength(CFSTR("file://"));
    CFRange layoutTestsWebArchivePathRange = CFStringFind(webResourceURL, CFSTR("/LayoutTests/"), kCFCompareBackwards);
    if (layoutTestsWebArchivePathRange.location == kCFNotFound)
        return;
    CFRange currentWorkingDirectoryRange = CFRangeMake(fileUrlLength, layoutTestsWebArchivePathRange.location - fileUrlLength);
    CFStringReplace(webResourceURL, currentWorkingDirectoryRange, CFSTR(""));
}

static void convertWebResourceResponseToDictionary(CFMutableDictionaryRef propertyList)
{
    CFDataRef responseData = static_cast<CFDataRef>(CFDictionaryGetValue(propertyList, CFSTR("WebResourceResponse"))); // WebResourceResponseKey in WebResource.m
    if (CFGetTypeID(responseData) != CFDataGetTypeID())
        return;

    RetainPtr<CFURLResponseRef> response = adoptCF(createCFURLResponseFromResponseData(responseData));
    if (!response)
        return;

    RetainPtr<CFMutableDictionaryRef> responseDictionary = adoptCF(CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    RetainPtr<CFMutableStringRef> urlString = adoptCF(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, CFURLGetString(CFURLResponseGetURL(response.get()))));
    normalizeWebResourceURL(urlString.get());
    CFDictionarySetValue(responseDictionary.get(), CFSTR("URL"), urlString.get());

    RetainPtr<CFMutableStringRef> mimeTypeString = adoptCF(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, CFURLResponseGetMIMEType(response.get())));
    convertMIMEType(mimeTypeString.get());
    CFDictionarySetValue(responseDictionary.get(), CFSTR("MIMEType"), mimeTypeString.get());

    CFStringRef textEncodingName = CFURLResponseGetTextEncodingName(response.get());
    if (textEncodingName)
        CFDictionarySetValue(responseDictionary.get(), CFSTR("textEncodingName"), textEncodingName);

    SInt64 expectedContentLength = CFURLResponseGetExpectedContentLength(response.get());
    RetainPtr<CFNumberRef> expectedContentLengthNumber = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &expectedContentLength));
    CFDictionarySetValue(responseDictionary.get(), CFSTR("expectedContentLength"), expectedContentLengthNumber.get());

    if (CFHTTPMessageRef httpMessage = CFURLResponseGetHTTPResponse(response.get())) {
        RetainPtr<CFDictionaryRef> allHeaders = adoptCF(CFHTTPMessageCopyAllHeaderFields(httpMessage));
        RetainPtr<CFMutableDictionaryRef> allHeaderFields = adoptCF(CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, allHeaders.get()));
        normalizeHTTPResponseHeaderFields(allHeaderFields.get());
        CFDictionarySetValue(responseDictionary.get(), CFSTR("allHeaderFields"), allHeaderFields.get());

        CFIndex statusCode = CFHTTPMessageGetResponseStatusCode(httpMessage);
        RetainPtr<CFNumberRef> statusCodeNumber = adoptCF(CFNumberCreate(kCFAllocatorDefault, kCFNumberCFIndexType, &statusCode));
        CFDictionarySetValue(responseDictionary.get(), CFSTR("statusCode"), statusCodeNumber.get());
    }

    CFDictionarySetValue(propertyList, CFSTR("WebResourceResponse"), responseDictionary.get());
}

static CFComparisonResult compareResourceURLs(const void *val1, const void *val2, void *context)
{
    CFStringRef url1 = static_cast<CFStringRef>(CFDictionaryGetValue(static_cast<CFDictionaryRef>(val1), CFSTR("WebResourceURL")));
    CFStringRef url2 = static_cast<CFStringRef>(CFDictionaryGetValue(static_cast<CFDictionaryRef>(val2), CFSTR("WebResourceURL")));

    return CFStringCompare(url1, url2, kCFCompareAnchored);
}

CFStringRef createXMLStringFromWebArchiveData(CFDataRef webArchiveData)
{
    CFErrorRef error = 0;
    CFPropertyListFormat format = kCFPropertyListBinaryFormat_v1_0;
    RetainPtr<CFMutableDictionaryRef> propertyList = adoptCF((CFMutableDictionaryRef)CFPropertyListCreateWithData(kCFAllocatorDefault, webArchiveData, kCFPropertyListMutableContainersAndLeaves, &format, &error));

    if (!propertyList) {
        if (error)
            return CFErrorCopyDescription(error);
        return static_cast<CFStringRef>(CFRetain(CFSTR("An unknown error occurred converting data to property list.")));
    }

    RetainPtr<CFMutableArrayRef> resources = adoptCF(CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks));
    CFArrayAppendValue(resources.get(), propertyList.get());

    while (CFArrayGetCount(resources.get())) {
        RetainPtr<CFMutableDictionaryRef> resourcePropertyList = (CFMutableDictionaryRef)CFArrayGetValueAtIndex(resources.get(), 0);
        CFArrayRemoveValueAtIndex(resources.get(), 0);

        CFMutableDictionaryRef mainResource = (CFMutableDictionaryRef)CFDictionaryGetValue(resourcePropertyList.get(), CFSTR("WebMainResource"));
        normalizeWebResourceURL((CFMutableStringRef)CFDictionaryGetValue(mainResource, CFSTR("WebResourceURL")));
        convertWebResourceDataToString(mainResource);

        // Add subframeArchives to list for processing
        CFMutableArrayRef subframeArchives = (CFMutableArrayRef)CFDictionaryGetValue(resourcePropertyList.get(), CFSTR("WebSubframeArchives")); // WebSubframeArchivesKey in WebArchive.m
        if (subframeArchives)
            CFArrayAppendArray(resources.get(), subframeArchives, CFRangeMake(0, CFArrayGetCount(subframeArchives)));

        CFMutableArrayRef subresources = (CFMutableArrayRef)CFDictionaryGetValue(resourcePropertyList.get(), CFSTR("WebSubresources")); // WebSubresourcesKey in WebArchive.m
        if (!subresources)
            continue;

        CFIndex subresourcesCount = CFArrayGetCount(subresources);
        for (CFIndex i = 0; i < subresourcesCount; ++i) {
            CFMutableDictionaryRef subresourcePropertyList = (CFMutableDictionaryRef)CFArrayGetValueAtIndex(subresources, i);
            normalizeWebResourceURL((CFMutableStringRef)CFDictionaryGetValue(subresourcePropertyList, CFSTR("WebResourceURL")));
            convertWebResourceResponseToDictionary(subresourcePropertyList);
            convertWebResourceDataToString(subresourcePropertyList);
        }

        // Sort the subresources so they're always in a predictable order for the dump
        CFArraySortValues(subresources, CFRangeMake(0, CFArrayGetCount(subresources)), compareResourceURLs, 0);
    }

    error = 0;

    RetainPtr<CFDataRef> xmlData = adoptCF(CFPropertyListCreateData(kCFAllocatorDefault, propertyList.get(), kCFPropertyListXMLFormat_v1_0, 0, &error));

    if (!xmlData) {
        if (error)
            return CFErrorCopyDescription(error);
        return static_cast<CFStringRef>(CFRetain(CFSTR("An unknown error occurred converting property list to data.")));
    }

    RetainPtr<CFStringRef> xmlString = adoptCF(CFStringCreateFromExternalRepresentation(kCFAllocatorDefault, xmlData.get(), kCFStringEncodingUTF8));
    RetainPtr<CFMutableStringRef> string = adoptCF(CFStringCreateMutableCopy(kCFAllocatorDefault, 0, xmlString.get()));

    // Replace "Apple Computer" with "Apple" in the DTD declaration.
    CFStringFindAndReplace(string.get(), CFSTR("-//Apple Computer//"), CFSTR("-//Apple//"), CFRangeMake(0, CFStringGetLength(string.get())), 0);

    return string.leakRef();
}

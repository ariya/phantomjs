/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#import "WebNSPasteboardExtras.h"

#import "DOMElementInternal.h"
#import "WebArchive.h"
#import "WebFrameInternal.h"
#import "WebHTMLViewInternal.h"
#import "WebNSURLExtras.h"
#import "WebResourcePrivate.h"
#import "WebURLsWithTitles.h"
#import "WebViewPrivate.h"
#import <WebCore/CachedImage.h>
#import <WebCore/Element.h>
#import <WebCore/Image.h>
#import <WebCore/MIMETypeRegistry.h>
#import <WebCore/RenderImage.h>
#import <WebKit/DOMExtensions.h>
#import <WebKit/DOMPrivate.h>
#import <WebKitSystemInterface.h>
#import <wtf/Assertions.h>
#import <wtf/RetainPtr.h>
#import <wtf/StdLibExtras.h>

using namespace WebCore;

NSString *WebURLPboardType = @"public.url";
NSString *WebURLNamePboardType = @"public.url-name";

@implementation NSPasteboard (WebExtras)

+ (NSArray *)_web_writableTypesForURL
{
    DEFINE_STATIC_LOCAL(RetainPtr<NSArray>, types, ([[NSArray alloc] initWithObjects:
        WebURLsWithTitlesPboardType,
        NSURLPboardType,
        WebURLPboardType,
        WebURLNamePboardType,
        NSStringPboardType,
        nil]));
    return types.get();
}

static inline NSArray *_createWritableTypesForImageWithoutArchive()
{
    NSMutableArray *types = [[NSMutableArray alloc] initWithObjects:NSTIFFPboardType, nil];
    [types addObjectsFromArray:[NSPasteboard _web_writableTypesForURL]];
    return types;
}

static NSArray *_writableTypesForImageWithoutArchive (void)
{
    DEFINE_STATIC_LOCAL(RetainPtr<NSArray>, types, (_createWritableTypesForImageWithoutArchive()));
    return types.get();
}

static inline NSArray *_createWritableTypesForImageWithArchive()
{
    NSMutableArray *types = [_writableTypesForImageWithoutArchive() mutableCopy];
    [types addObject:NSRTFDPboardType];
    [types addObject:WebArchivePboardType];
    return types;
}

static NSArray *_writableTypesForImageWithArchive (void)
{
    DEFINE_STATIC_LOCAL(RetainPtr<NSArray>, types, (_createWritableTypesForImageWithArchive()));
    return types.get();
}

+ (NSArray *)_web_writableTypesForImageIncludingArchive:(BOOL)hasArchive
{
    return hasArchive 
        ? _writableTypesForImageWithArchive()
        : _writableTypesForImageWithoutArchive();
}

+ (NSArray *)_web_dragTypesForURL
{
    return [NSArray arrayWithObjects:
        WebURLsWithTitlesPboardType,
        NSURLPboardType,
        WebURLPboardType,
        WebURLNamePboardType,
        NSStringPboardType,
        NSFilenamesPboardType,
        nil];
}

- (NSURL *)_web_bestURL
{
    NSArray *types = [self types];

    if ([types containsObject:NSURLPboardType]) {
        NSURL *URLFromPasteboard = [NSURL URLFromPasteboard:self];
        NSString *scheme = [URLFromPasteboard scheme];
        if ([scheme isEqualToString:@"http"] || [scheme isEqualToString:@"https"]) {
            return [URLFromPasteboard _webkit_canonicalize];
        }
    }

    if ([types containsObject:NSStringPboardType]) {
        NSString *URLString = [self stringForType:NSStringPboardType];
        if ([URLString _webkit_looksLikeAbsoluteURL]) {
            NSURL *URL = [[NSURL _web_URLWithUserTypedString:URLString] _webkit_canonicalize];
            if (URL) {
                return URL;
            }
        }
    }

    if ([types containsObject:NSFilenamesPboardType]) {
        NSArray *files = [self propertyListForType:NSFilenamesPboardType];
        // FIXME: Maybe it makes more sense to allow multiple files and only use the first one?
        if ([files count] == 1) {
            NSString *file = [files objectAtIndex:0];
            // FIXME: We are filtering out directories because that's what the original code used to
            // do. Without this check, if the URL points to a local directory, Safari will open the
            // parent directory of the directory in Finder. This check should go away as soon as
            // possible.
            BOOL isDirectory;
            if ([[NSFileManager defaultManager] fileExistsAtPath:file isDirectory:&isDirectory] && isDirectory)
                return nil;
            return [[NSURL fileURLWithPath:file] _webkit_canonicalize];
        }
    }

    return nil;
}

- (void)_web_writeURL:(NSURL *)URL andTitle:(NSString *)title types:(NSArray *)types
{
    ASSERT(URL);

    if ([title length] == 0) {
        title = [[URL path] lastPathComponent];
        if ([title length] == 0)
            title = [URL _web_userVisibleString];
    }
    
    if ([types containsObject:NSURLPboardType])
        [URL writeToPasteboard:self];
    if ([types containsObject:WebURLPboardType])
        [self setString:[URL _web_originalDataAsString] forType:WebURLPboardType];
    if ([types containsObject:WebURLNamePboardType])
        [self setString:title forType:WebURLNamePboardType];
    if ([types containsObject:NSStringPboardType])
        [self setString:[URL _web_userVisibleString] forType:NSStringPboardType];
    if ([types containsObject:WebURLsWithTitlesPboardType])
        [WebURLsWithTitles writeURLs:[NSArray arrayWithObject:URL] andTitles:[NSArray arrayWithObject:title] toPasteboard:self];
}

+ (int)_web_setFindPasteboardString:(NSString *)string withOwner:(id)owner
{
    NSPasteboard *findPasteboard = [NSPasteboard pasteboardWithName:NSFindPboard];
    [findPasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:owner];
    [findPasteboard setString:string forType:NSStringPboardType];
    return [findPasteboard changeCount];
}

- (void)_web_writeFileWrapperAsRTFDAttachment:(NSFileWrapper *)wrapper
{
    NSTextAttachment *attachment = [[NSTextAttachment alloc] initWithFileWrapper:wrapper];
    
    NSAttributedString *string = [NSAttributedString attributedStringWithAttachment:attachment];
    [attachment release];
    
    NSData *RTFDData = [string RTFDFromRange:NSMakeRange(0, [string length]) documentAttributes:nil];
    [self setData:RTFDData forType:NSRTFDPboardType];
}


- (void)_web_writePromisedRTFDFromArchive:(WebArchive*)archive containsImage:(BOOL)containsImage
{
    ASSERT(archive);
    // This image data is either the only subresource of an archive (HTML image case)
    // or the main resource (standalone image case).
    NSArray *subresources = [archive subresources];
    WebResource *resource = [archive mainResource];
    if (containsImage && [subresources count] > 0 
        && MIMETypeRegistry::isSupportedImageResourceMIMEType([[subresources objectAtIndex:0] MIMEType]))
        resource = (WebResource *)[subresources objectAtIndex:0];
    ASSERT(resource != nil);
    
    ASSERT(!containsImage || MIMETypeRegistry::isSupportedImageResourceMIMEType([resource MIMEType]));
    if (!containsImage || MIMETypeRegistry::isSupportedImageResourceMIMEType([resource MIMEType]))
        [self _web_writeFileWrapperAsRTFDAttachment:[resource _fileWrapperRepresentation]];
    
}

static CachedImage* imageFromElement(DOMElement *domElement)
{
    Element* element = core(domElement);
    if (!element)
        return 0;
    
    RenderObject* renderer = element->renderer();
    RenderImage* imageRenderer = toRenderImage(renderer);
    if (!imageRenderer->cachedImage() || imageRenderer->cachedImage()->errorOccurred()) 
        return 0;        
    return imageRenderer->cachedImage();
}

- (void)_web_writeImage:(NSImage *)image
                element:(DOMElement *)element
                    URL:(NSURL *)URL 
                  title:(NSString *)title
                archive:(WebArchive *)archive
                  types:(NSArray *)types
                 source:(WebHTMLView *)source
{
    ASSERT(image || element);
    ASSERT(URL);

    [self _web_writeURL:URL andTitle:title types:types];
    
    if ([types containsObject:NSTIFFPboardType]) {
        if (image)
            [self setData:[image TIFFRepresentation] forType:NSTIFFPboardType];
        else if (source && element)
            [source setPromisedDragTIFFDataSource:imageFromElement(element)];
        else if (element)
            [self setData:[element _imageTIFFRepresentation] forType:NSTIFFPboardType];
    }
    
    if (archive) {
        if ([types containsObject:WebArchivePboardType])
            [self setData:[archive data] forType:WebArchivePboardType];
        return;
    }

    // We should not have declared types that we aren't going to write (4031826).
    ASSERT(![types containsObject:NSRTFDPboardType]);
    ASSERT(![types containsObject:WebArchivePboardType]);
}

- (id)_web_declareAndWriteDragImageForElement:(DOMElement *)element
                                       URL:(NSURL *)URL 
                                     title:(NSString *)title
                                   archive:(WebArchive *)archive
                                    source:(WebHTMLView *)source
{
    ASSERT(self == [NSPasteboard pasteboardWithName:NSDragPboard]);

    NSString *extension = @"";
    if (RenderObject* renderer = core(element)->renderer()) {
        if (renderer->isImage()) {
            if (CachedImage* image = toRenderImage(renderer)->cachedImage()) {
                extension = image->image()->filenameExtension();
                if (![extension length])
                    return 0;
            }
        }
    }

    NSMutableArray *types = [[NSMutableArray alloc] initWithObjects:NSFilesPromisePboardType, nil];
    [types addObjectsFromArray:[NSPasteboard _web_writableTypesForImageIncludingArchive:(archive != nil)]];
    [self declareTypes:types owner:source];    
    [self _web_writeImage:nil element:element URL:URL title:title archive:archive types:types source:source];
    [types release];

    NSArray *extensions = [[NSArray alloc] initWithObjects:extension, nil];
    [self setPropertyList:extensions forType:NSFilesPromisePboardType];
    [extensions release];

    return source;
}

@end

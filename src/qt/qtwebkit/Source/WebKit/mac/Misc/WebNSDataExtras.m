/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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

#import <WebKit/WebNSDataExtras.h>
#import <WebKit/WebNSDataExtrasPrivate.h>

#import <wtf/Assertions.h>

@interface NSString (WebNSDataExtrasInternal)
- (NSString *)_web_capitalizeRFC822HeaderFieldName;
@end

@implementation NSString (WebNSDataExtrasInternal)

-(NSString *)_web_capitalizeRFC822HeaderFieldName
{
    CFStringRef name = (CFStringRef)self;
    NSString *result = nil;

    CFIndex i; 
    CFIndex len = CFStringGetLength(name);
    char *charPtr = NULL;
    UniChar *uniCharPtr = NULL;
    Boolean useUniCharPtr = FALSE;
    Boolean shouldCapitalize = TRUE;
    Boolean somethingChanged = FALSE;
    
    for (i = 0; i < len; i ++) {
        UniChar ch = CFStringGetCharacterAtIndex(name, i);
        Boolean replace = FALSE;
        if (shouldCapitalize && ch >= 'a' && ch <= 'z') {
            ch = ch + 'A' - 'a';
            replace = TRUE;
        } 
        else if (!shouldCapitalize && ch >= 'A' && ch <= 'Z') {
            ch = ch + 'a' - 'A';
            replace = TRUE;
        }
        if (replace) {
            if (!somethingChanged) {
                somethingChanged = TRUE;
                if (CFStringGetBytes(name, CFRangeMake(0, len), kCFStringEncodingISOLatin1, 0, FALSE, NULL, 0, NULL) == len) {
                    // Can be encoded in ISOLatin1
                    useUniCharPtr = FALSE;
                    charPtr = CFAllocatorAllocate(NULL, len + 1, 0);
                    CFStringGetCString(name, charPtr, len+1, kCFStringEncodingISOLatin1);
                } 
                else {
                    useUniCharPtr = TRUE;
                    uniCharPtr = CFAllocatorAllocate(NULL, len * sizeof(UniChar), 0);
                    CFStringGetCharacters(name, CFRangeMake(0, len), uniCharPtr);
                }
            }
            if (useUniCharPtr) {
                uniCharPtr[i] = ch;
            } 
            else {
                charPtr[i] = ch;
            }
        }
        if (ch == '-') {
            shouldCapitalize = TRUE;
        } 
        else {
            shouldCapitalize = FALSE;
        }
    }
    if (somethingChanged) {
        if (useUniCharPtr) {
            result = (NSString *)CFMakeCollectable(CFStringCreateWithCharactersNoCopy(NULL, uniCharPtr, len, NULL));
        } 
        else {
            result = (NSString *)CFMakeCollectable(CFStringCreateWithCStringNoCopy(NULL, charPtr, kCFStringEncodingISOLatin1, NULL));
        }
    } 
    else {
        result = [self retain];
    }
    
    return [result autorelease];
}

@end

@implementation NSData (WebKitExtras)

-(NSString *)_webkit_guessedMIMETypeForXML
{
    int length = [self length];
    const UInt8 *bytes = [self bytes];
    
#define CHANNEL_TAG_LENGTH 7
    
    const char *p = (const char *)bytes;
    int remaining = MIN(length, WEB_GUESS_MIME_TYPE_PEEK_LENGTH) - (CHANNEL_TAG_LENGTH - 1);
    
    BOOL foundRDF = false;
    
    while (remaining > 0) {
        // Look for a "<".
        const char *hit = memchr(p, '<', remaining);
        if (!hit) {
            break;
        }
        
        // We are trying to identify RSS or Atom. RSS has a top-level
        // element of either <rss> or <rdf>. However, there are
        // non-RSS RDF files, so in the case of <rdf> we further look
        // for a <channel> element. In the case of an Atom file, a
        // top-level <feed> element is all we need to see. Only tags
        // starting with <? or <! can precede the root element. We
        // bail if we don't find an <rss>, <feed> or <rdf> element
        // right after those.
        
        if (foundRDF) {
            if (strncasecmp(hit, "<channel", strlen("<channel")) == 0) {
                return @"application/rss+xml";
            }
        } else if (strncasecmp(hit, "<rdf", strlen("<rdf")) == 0) {
            foundRDF = TRUE;
        } else if (strncasecmp(hit, "<rss", strlen("<rss")) == 0) {
            return @"application/rss+xml";
        } else if (strncasecmp(hit, "<feed", strlen("<feed")) == 0) {
            return @"application/atom+xml";
        } else if (strncasecmp(hit, "<?", strlen("<?")) != 0 && strncasecmp(hit, "<!", strlen("<!")) != 0) {
            return nil;
        }
        
        // Skip the "<" and continue.
        remaining -= (hit + 1) - p;
        p = hit + 1;
    }
    
    return nil;
}

-(NSString *)_webkit_guessedMIMEType
{
#define JPEG_MAGIC_NUMBER_LENGTH 4
#define SCRIPT_TAG_LENGTH 7
#define TEXT_HTML_LENGTH 9
#define VCARD_HEADER_LENGTH 11
#define VCAL_HEADER_LENGTH 15
    
    NSString *MIMEType = [self _webkit_guessedMIMETypeForXML];
    if ([MIMEType length])
        return MIMEType;
    
    int length = [self length];
    const char *bytes = [self bytes];
    
    const char *p = bytes;
    int remaining = MIN(length, WEB_GUESS_MIME_TYPE_PEEK_LENGTH) - (SCRIPT_TAG_LENGTH - 1);
    while (remaining > 0) {
        // Look for a "<".
        const char *hit = memchr(p, '<', remaining);
        if (!hit) {
            break;
        }
        
        // If we found a "<", look for "<html>" or "<a " or "<script".
        if (strncasecmp(hit, "<html>",  strlen("<html>")) == 0 ||
            strncasecmp(hit, "<a ",     strlen("<a ")) == 0 ||
            strncasecmp(hit, "<script", strlen("<script")) == 0 ||
            strncasecmp(hit, "<title>", strlen("<title>")) == 0) {
            return @"text/html";
        }
        
        // Skip the "<" and continue.
        remaining -= (hit + 1) - p;
        p = hit + 1;
    }
    
    // Test for a broken server which has sent the content type as part of the content.
    // This code could be improved to look for other mime types.
    p = bytes;
    remaining = MIN(length, WEB_GUESS_MIME_TYPE_PEEK_LENGTH) - (TEXT_HTML_LENGTH - 1);
    while (remaining > 0) {
        // Look for a "t" or "T".
        const char *hit = NULL;
        const char *lowerhit = memchr(p, 't', remaining);
        const char *upperhit = memchr(p, 'T', remaining);
        if (!lowerhit && !upperhit) {
            break;
        }
        if (!lowerhit) {
            hit = upperhit;
        }
        else if (!upperhit) {
            hit = lowerhit;
        }
        else {
            hit = MIN(lowerhit, upperhit);
        }
        
        // If we found a "t/T", look for "text/html".
        if (strncasecmp(hit, "text/html", TEXT_HTML_LENGTH) == 0) {
            return @"text/html";
        }
        
        // Skip the "t/T" and continue.
        remaining -= (hit + 1) - p;
        p = hit + 1;
    }
    
    if ((length >= VCARD_HEADER_LENGTH) && strncmp(bytes, "BEGIN:VCARD", VCARD_HEADER_LENGTH) == 0) {
        return @"text/vcard";
    }
    if ((length >= VCAL_HEADER_LENGTH) && strncmp(bytes, "BEGIN:VCALENDAR", VCAL_HEADER_LENGTH) == 0) {
        return @"text/calendar";
    }
    
    // Test for plain text.
    int i;
    for(i=0; i<length; i++){
        char c = bytes[i];
        if ((c < 0x20 || c > 0x7E) && (c != '\t' && c != '\r' && c != '\n')) {
            break;
        }
    }
    if (i == length) {
        // Didn't encounter any bad characters, looks like plain text.
        return @"text/plain";
    }
    
    // Looks like this is a binary file.
    
    // Sniff for the JPEG magic number.
    if ((length >= JPEG_MAGIC_NUMBER_LENGTH) && strncmp(bytes, "\xFF\xD8\xFF\xE0", JPEG_MAGIC_NUMBER_LENGTH) == 0) {
        return @"image/jpeg";
    }
    
#undef JPEG_MAGIC_NUMBER_LENGTH
#undef SCRIPT_TAG_LENGTH
#undef TEXT_HTML_LENGTH
#undef VCARD_HEADER_LENGTH
#undef VCAL_HEADER_LENGTH
    
    return nil;
}

@end

@implementation NSData (WebNSDataExtras)

-(BOOL)_web_isCaseInsensitiveEqualToCString:(const char *)string
{
    ASSERT(string);
    
    const char *bytes = [self bytes];
    return strncasecmp(bytes, string, [self length]) == 0;
}

static const UInt8 *_findEOL(const UInt8 *bytes, CFIndex len) {
    
    // According to the HTTP specification EOL is defined as
    // a CRLF pair.  Unfortunately, some servers will use LF
    // instead.  Worse yet, some servers will use a combination
    // of both (e.g. <header>CRLFLF<body>), so findEOL needs
    // to be more forgiving.  It will now accept CRLF, LF, or
    // CR.
    //
    // It returns NULL if EOL is not found or it will return
    // a pointer to the first terminating character.
    CFIndex i;
    for (i = 0;  i < len; i++)
    {
        UInt8 c = bytes[i];
        if ('\n' == c) return bytes + i;
        if ('\r' == c)
        {
            // Check to see if spanning buffer bounds
            // (CRLF is across reads).  If so, wait for
            // next read.
            if (i + 1 == len) break;
                
            return bytes + i;
        }
    }
    
    return NULL;
}

-(NSMutableDictionary *)_webkit_parseRFC822HeaderFields
{
    NSMutableDictionary *headerFields = [NSMutableDictionary dictionary];

    const UInt8 *bytes = [self bytes];
    unsigned length = [self length];
    NSString *lastKey = nil;
    const UInt8 *eol;

    // Loop over lines until we're past the header, or we can't find any more end-of-lines
    while ((eol = _findEOL(bytes, length))) {
        const UInt8 *line = bytes;
        SInt32 lineLength = eol - bytes;

        // Move bytes to the character after the terminator as returned by _findEOL.
        bytes = eol + 1;
        if (('\r' == *eol) && ('\n' == *bytes)) {
            bytes++; // Safe since _findEOL won't return a spanning CRLF.
        }

        length -= (bytes - line);
        if (lineLength == 0) {
            // Blank line; we're at the end of the header
            break;
        }
        else if (*line == ' ' || *line == '\t') {
            // Continuation of the previous header
            if (!lastKey) {
                // malformed header; ignore it and continue
                continue;
            }
            else {
                // Merge the continuation of the previous header
                NSString *currentValue = [headerFields objectForKey:lastKey];
                NSString *newValue = (NSString *)CFMakeCollectable(CFStringCreateWithBytes(NULL, line, lineLength, kCFStringEncodingISOLatin1, FALSE));
                ASSERT(currentValue);
                ASSERT(newValue);
                NSString *mergedValue = [[NSString alloc] initWithFormat:@"%@%@", currentValue, newValue];
                [headerFields setObject:(NSString *)mergedValue forKey:lastKey];
                [newValue release];
                [mergedValue release];
                // Note: currentValue is autoreleased
            }
        }
        else {
            // Brand new header
            const UInt8 *colon;
            for (colon = line; *colon != ':' && colon != eol; colon ++) {
                // empty loop
            }
            if (colon == eol) {
                // malformed header; ignore it and continue
                continue;
            }
            else {
                lastKey = (NSString *)CFMakeCollectable(CFStringCreateWithBytes(NULL, line, colon - line, kCFStringEncodingISOLatin1, FALSE));
                [lastKey autorelease];
                NSString *value = [lastKey _web_capitalizeRFC822HeaderFieldName];
                lastKey = value;
                for (colon++; colon != eol; colon++) {
                    if (*colon != ' ' && *colon != '\t') {
                        break;
                    }
                }
                if (colon == eol) {
                    value = [[NSString alloc] initWithString:@""];
                    [value autorelease];
                }
                else {
                    value = (NSString *)CFMakeCollectable(CFStringCreateWithBytes(NULL, colon, eol-colon, kCFStringEncodingISOLatin1, FALSE));
                    [value autorelease];
                }
                NSString *oldValue = [headerFields objectForKey:lastKey];
                if (oldValue) {
                    NSString *newValue = [[NSString alloc] initWithFormat:@"%@, %@", oldValue, value];
                    value = newValue;
                    [newValue autorelease];
                }
                [headerFields setObject:(NSString *)value forKey:lastKey];
            }
        }
    }

    return headerFields;
}

- (BOOL)_web_startsWithBlankLine
{
    return [self length] > 0 && ((const char *)[self bytes])[0] == '\n';
}

- (NSInteger)_web_locationAfterFirstBlankLine
{
    const char *bytes = (const char *)[self bytes];
    unsigned length = [self length];
    
    unsigned i;
    for (i = 0; i < length - 4; i++) {
        
        //  Support for Acrobat. It sends "\n\n".
        if (bytes[i] == '\n' && bytes[i+1] == '\n') {
            return i+2;
        }
        
        // Returns the position after 2 CRLF's or 1 CRLF if it is the first line.
        if (bytes[i] == '\r' && bytes[i+1] == '\n') {
            i += 2;
            if (i == 2) {
                return i;
            } else if (bytes[i] == '\n') {
                // Support for Director. It sends "\r\n\n" (3880387).
                return i+1;
            } else if (bytes[i] == '\r' && bytes[i+1] == '\n') {
                // Support for Flash. It sends "\r\n\r\n" (3758113).
                return i+2;
            }
        }
    }
    return NSNotFound;
}

@end

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

#import <WebKit/WebNSImageExtras.h>

#import <WebKit/WebKitLogging.h>

@implementation NSImage (WebExtras)

- (void)_web_scaleToMaxSize:(NSSize)size
{
    float heightResizeDelta = 0.0f, widthResizeDelta = 0.0f, resizeDelta = 0.0f;
    NSSize originalSize = [self size];

    if(originalSize.width > size.width){
        widthResizeDelta = size.width / originalSize.width;
        resizeDelta = widthResizeDelta;
    }

    if(originalSize.height > size.height){
        heightResizeDelta = size.height / originalSize.height;
        if((resizeDelta == 0.0) || (resizeDelta > heightResizeDelta)){
            resizeDelta = heightResizeDelta;
        }
    }
    
    if(resizeDelta > 0.0){
        NSSize newSize = NSMakeSize((originalSize.width * resizeDelta), (originalSize.height * resizeDelta));
        [self setScalesWhenResized:YES];
        [self setSize:newSize];
    }
}

- (void)_web_dissolveToFraction:(float)delta
{
    NSImage *dissolvedImage = [[NSImage alloc] initWithSize:[self size]];

    NSPoint point = [self isFlipped] ? NSMakePoint(0, [self size].height) : NSZeroPoint;
    
    // In this case the dragging image is always correct.
    [dissolvedImage setFlipped:[self isFlipped]];

    [dissolvedImage lockFocus];
    [self dissolveToPoint:point fraction: delta];
    [dissolvedImage unlockFocus];

    [self lockFocus];
    [dissolvedImage compositeToPoint:point operation:NSCompositeCopy];
    [self unlockFocus];

    [dissolvedImage release];
}

- (void)_web_saveAndOpen
{
    char path[] = "/tmp/XXXXXX.tiff";
    int fd = mkstemps(path, 5);
    if (fd != -1) {
        NSData *data = [self TIFFRepresentation];
        write(fd, [data bytes], [data length]);
        close(fd);
        [[NSWorkspace sharedWorkspace] openFile:[NSString stringWithUTF8String:path]];
    }
}

@end

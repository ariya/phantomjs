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

#ifndef __LP64__

#include "CarbonUtils.h"
#import <WebKitSystemInterface.h>

extern CGImageRef _NSCreateImageRef( unsigned char *const bitmapData[5], int pixelsWide, int pixelsHigh, int bitsPerSample, int samplesPerPixel, int bitsPerPixel, int bytesPerRow, BOOL isPlanar, BOOL hasAlpha, NSString *colorSpaceName, CGColorSpaceRef customColorSpace, id sourceObj);

static void				PoolCleaner( EventLoopTimerRef inTimer, EventLoopIdleTimerMessage inState, void *inUserData );

static NSAutoreleasePool*	sPool;
static unsigned numPools;
static EventLoopRef poolLoop;

void                    HIWebViewRegisterClass( void );

void
WebInitForCarbon()
{
    static bool			sAppKitLoaded;

    if ( !sAppKitLoaded )
    {
        ProcessSerialNumber    process;

        // Force us to register with process server, this ensure that the process
        // "flavour" is correctly established.
        GetCurrentProcess( &process ); 
        NSApplicationLoad();
                
        sPool = [[NSAutoreleasePool allocWithZone:NULL] init];
        numPools = WKGetNSAutoreleasePoolCount();
        
        poolLoop = GetCurrentEventLoop ();

        InstallEventLoopIdleTimer( GetMainEventLoop(), 1.0, 0, PoolCleaner, 0, NULL );
        
        sAppKitLoaded = true;
                
        HIWebViewRegisterClass();
    }
}

/*
    The pool cleaner is required because Carbon applications do not have
    an autorelease pool provided by their event loops.  Importantly,
    carbon applications that nest event loops, using any of the various
    techniques available to Carbon apps, MUST create their our pools around
    their nested event loops.
*/
static void
PoolCleaner( EventLoopTimerRef inTimer, EventLoopIdleTimerMessage inState, void *inUserData )
{
    if ( inState == kEventLoopIdleTimerStarted ) {
        CFStringRef mode = CFRunLoopCopyCurrentMode( (CFRunLoopRef)GetCFRunLoopFromEventLoop( GetCurrentEventLoop() ));
        EventLoopRef thisLoop = GetCurrentEventLoop ();
        if ( CFEqual( mode, kCFRunLoopDefaultMode ) && thisLoop == poolLoop) {
            unsigned currentNumPools = WKGetNSAutoreleasePoolCount()-1;            
            if (currentNumPools == numPools){
                [sPool drain];
                
                sPool = [[NSAutoreleasePool allocWithZone:NULL] init];
                numPools = WKGetNSAutoreleasePoolCount();
            }
        }
        CFRelease( mode );
    }
}

CGImageRef
WebConvertNSImageToCGImageRef(
	NSImage*         inImage )
{
	NSArray*				reps = [inImage representations];
	NSBitmapImageRep*		rep = NULL;
	CGImageRef				image = NULL;
	unsigned				i;

	for ( i=0; i < [reps count]; i++ )
	{
        if ( [[reps objectAtIndex:i] isKindOfClass:[NSBitmapImageRep class]] )
        {
            rep = [reps objectAtIndex:i];
            break;
        }
	}
    
	if ( rep )
	{
        //CGColorSpaceRef csync = (CGColorSpaceRef)[rep valueForProperty:NSImageColorSyncProfileData];
        
        unsigned char* planes[5];

        [rep getBitmapDataPlanes:planes];

        image = _NSCreateImageRef( planes, [rep pixelsWide], [rep pixelsHigh],
                    [rep bitsPerSample], [rep samplesPerPixel], [rep bitsPerPixel],
                    [rep bytesPerRow], [rep isPlanar], [rep hasAlpha], [rep colorSpaceName],
                    NULL, rep);
	}
    
	return image;
}

#endif

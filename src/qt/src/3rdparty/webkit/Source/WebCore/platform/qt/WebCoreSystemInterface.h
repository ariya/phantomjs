/*
 * Copyright 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef WebCoreSystemInterface_h
#define WebCoreSystemInterface_h

#include <objc/objc.h>

#ifdef NSGEOMETRY_TYPES_SAME_AS_CGGEOMETRY_TYPES
typedef struct CGPoint NSPoint;
typedef struct CGRect NSRect;
#else
typedef struct _NSPoint NSPoint;
typedef struct _NSRect NSRect;
#endif

#ifdef __OBJC__
@class NSArray;
@class NSControl;
@class NSString;
@class NSURL;
@class NSView;
@class NSWindow;
@class QTMovie;
@class QTMovieView;
#else
class NSArray;
class NSControl;
class NSString;
class NSURL;
class NSView;
class NSWindow;
class QTMovie;
class QTMovieView;
#endif

extern "C" {

// In alphabetical order.
extern NSView *(*wkCreateMediaUIBackgroundView)(void);
extern NSControl *(*wkCreateMediaUIControl)(int);
extern unsigned (*wkQTIncludeOnlyModernMediaFileTypes)(void);
extern int (*wkQTMovieDataRate)(QTMovie*);
extern void (*wkQTMovieDisableComponent)(uint32_t[5]);
extern float (*wkQTMovieMaxTimeLoaded)(QTMovie*);
extern NSString *(*wkQTMovieMaxTimeLoadedChangeNotification)(void);
extern float (*wkQTMovieMaxTimeSeekable)(QTMovie*);
extern int (*wkQTMovieGetType)(QTMovie*);
extern BOOL (*wkQTMovieHasClosedCaptions)(QTMovie*);
extern NSURL *(*wkQTMovieResolvedURL)(QTMovie*);
extern void (*wkQTMovieSetShowClosedCaptions)(QTMovie*, BOOL);
extern void (*wkQTMovieSelectPreferredAlternates)(QTMovie*);
extern void (*wkQTMovieViewSetDrawSynchronously)(QTMovieView*, BOOL);
extern NSArray *(*wkQTGetSitesInMediaDownloadCache)();
extern void (*wkQTClearMediaDownloadCacheForSite)(NSString *site);
extern void (*wkQTClearMediaDownloadCache)();
extern void (*wkWindowSetAlpha)(NSWindow *, float);
extern void (*wkWindowSetScaledFrame)(NSWindow *, NSRect, NSRect);

typedef enum {
    wkMediaUIControlTimeline,
    wkMediaUIControlSlider,
    wkMediaUIControlPlayPauseButton,
    wkMediaUIControlExitFullscreenButton,
    wkMediaUIControlRewindButton,
    wkMediaUIControlFastForwardButton,
    wkMediaUIControlVolumeUpButton,
    wkMediaUIControlVolumeDownButton
} wkMediaUIControlType;

}

#endif

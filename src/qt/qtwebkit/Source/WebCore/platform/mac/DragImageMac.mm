/*
 * Copyright (C) 2007, 2009, 2012 Apple Inc. All rights reserved.
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
#import "DragImage.h"

#if ENABLE(DRAG_SUPPORT)
#import "BitmapImage.h"
#import "Font.h"
#import "FontCache.h"
#import "FontDescription.h"
#import "FontSelector.h"
#import "GraphicsContext.h"
#import "Image.h"
#import "KURL.h"
#import "ResourceResponse.h"
#import "StringTruncator.h"
#import "TextRun.h"

namespace WebCore {

IntSize dragImageSize(RetainPtr<NSImage> image)
{
    return (IntSize)[image.get() size];
}

void deleteDragImage(RetainPtr<NSImage>)
{
    // Since this is a RetainPtr, there's nothing additional we need to do to
    // delete it. It will be released when it falls out of scope.
}

RetainPtr<NSImage> scaleDragImage(RetainPtr<NSImage> image, FloatSize scale)
{
    NSSize originalSize = [image.get() size];
    NSSize newSize = NSMakeSize((originalSize.width * scale.width()), (originalSize.height * scale.height()));
    newSize.width = roundf(newSize.width);
    newSize.height = roundf(newSize.height);
    [image.get() setScalesWhenResized:YES];
    [image.get() setSize:newSize];
    return image;
}
    
RetainPtr<NSImage> dissolveDragImageToFraction(RetainPtr<NSImage> image, float delta)
{
    if (!image)
        return nil;

    RetainPtr<NSImage> dissolvedImage = adoptNS([[NSImage alloc] initWithSize:[image.get() size]]);
    
    [dissolvedImage.get() lockFocus];
    [image.get() drawAtPoint:NSZeroPoint fromRect:NSMakeRect(0, 0, [image size].width, [image size].height) operation:NSCompositeCopy fraction:delta];
    [dissolvedImage.get() unlockFocus];

    return dissolvedImage;
}
        
RetainPtr<NSImage> createDragImageFromImage(Image* image, RespectImageOrientationEnum shouldRespectImageOrientation)
{
    IntSize size = image->size();

    if (image->isBitmapImage()) {
        ImageOrientation orientation = DefaultImageOrientation;
        BitmapImage* bitmapImage = static_cast<BitmapImage *>(image);
        IntSize sizeRespectingOrientation = bitmapImage->sizeRespectingOrientation();

        if (shouldRespectImageOrientation == RespectImageOrientation)
            orientation = bitmapImage->currentFrameOrientation();

        if (orientation != DefaultImageOrientation) {
            // Construct a correctly-rotated copy of the image to use as the drag image.
            FloatRect destRect(FloatPoint(), sizeRespectingOrientation);

            RetainPtr<NSImage> rotatedDragImage = adoptNS([[NSImage alloc] initWithSize:(NSSize)(sizeRespectingOrientation)]);
            [rotatedDragImage.get() lockFocus];

            // ImageOrientation uses top-left coordinates, need to flip to bottom-left, apply...
            CGAffineTransform transform = CGAffineTransformMakeTranslation(0, destRect.height());
            transform = CGAffineTransformScale(transform, 1, -1);
            transform = CGAffineTransformConcat(orientation.transformFromDefault(sizeRespectingOrientation), transform);

            if (orientation.usesWidthAsHeight())
                destRect = FloatRect(destRect.x(), destRect.y(), destRect.height(), destRect.width());

            // ...and flip back.
            transform = CGAffineTransformTranslate(transform, 0, destRect.height());
            transform = CGAffineTransformScale(transform, 1, -1);

            RetainPtr<NSAffineTransform> cocoaTransform = adoptNS([[NSAffineTransform alloc] init]);
            [cocoaTransform.get() setTransformStruct:*(NSAffineTransformStruct*)&transform];
            [cocoaTransform.get() concat];

            [image->getNSImage() drawInRect:destRect fromRect:NSMakeRect(0, 0, size.width(), size.height()) operation:NSCompositeSourceOver fraction:1.0];
            [rotatedDragImage.get() unlockFocus];

            return rotatedDragImage;
        }
    }

    RetainPtr<NSImage> dragImage = adoptNS([image->getNSImage() copy]);
    [dragImage.get() setSize:(NSSize)size];
    return dragImage;
}
    
RetainPtr<NSImage> createDragImageIconForCachedImageFilename(const String& filename)
{
    NSString *extension = nil;
    size_t dotIndex = filename.reverseFind('.');
    
    if (dotIndex != notFound && dotIndex < (filename.length() - 1)) // require that a . exists after the first character and before the last
        extension = filename.substring(dotIndex + 1);
    else {
        // It might be worth doing a further lookup to pull the extension from the MIME type.
        extension = @"";
    }
    
    return [[NSWorkspace sharedWorkspace] iconForFileType:extension];
}


const float DragLabelBorderX = 4;
//Keep border_y in synch with DragController::LinkDragBorderInset
const float DragLabelBorderY = 2;
const float DragLabelRadius = 5;
const float LabelBorderYOffset = 2;

const float MinDragLabelWidthBeforeClip = 120;
const float MaxDragLabelWidth = 320;

const float DragLinkLabelFontsize = 11;
const float DragLinkUrlFontSize = 10;

// FIXME - we should move all the functionality of NSString extras to WebCore
    
static Font& fontFromNSFont(NSFont *font)
{
    static NSFont *currentFont;
    DEFINE_STATIC_LOCAL(Font, currentRenderer, ());
    
    if ([font isEqual:currentFont])
        return currentRenderer;
    if (currentFont)
        CFRelease(currentFont);
    currentFont = font;
    CFRetain(currentFont);
    FontPlatformData f(font, [font pointSize]);
    currentRenderer = Font(f, ![[NSGraphicsContext currentContext] isDrawingToScreen]);
    return currentRenderer;
}

static bool canUseFastRenderer(const UniChar* buffer, unsigned length)
{
    unsigned i;
    for (i = 0; i < length; i++) {
        UCharDirection direction = u_charDirection(buffer[i]);
        if (direction == U_RIGHT_TO_LEFT || direction > U_OTHER_NEUTRAL)
            return false;
    }
    return true;
}
    
static float widthWithFont(NSString *string, NSFont *font)
{
    unsigned length = [string length];
    Vector<UniChar, 2048> buffer(length);
    
    [string getCharacters:buffer.data()];
    
    if (canUseFastRenderer(buffer.data(), length)) {
        FontCachePurgePreventer fontCachePurgePreventer;

        Font webCoreFont(FontPlatformData(font, [font pointSize]), ![[NSGraphicsContext currentContext] isDrawingToScreen]);
        TextRun run(buffer.data(), length);
        run.disableRoundingHacks();
        return webCoreFont.width(run);
    }
    
    return [string sizeWithAttributes:[NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, nil]].width;
}

static inline CGFloat webkit_CGCeiling(CGFloat value)
{
    if (sizeof(value) == sizeof(float))
        return ceilf(value);
    return static_cast<CGFloat>(ceil(value));
}
    
static void drawAtPoint(NSString *string, NSPoint point, NSFont *font, NSColor *textColor)
{
    unsigned length = [string length];
    Vector<UniChar, 2048> buffer(length);
    
    [string getCharacters:buffer.data()];
    
    if (canUseFastRenderer(buffer.data(), length)) {
        FontCachePurgePreventer fontCachePurgePreventer;

        // The following is a half-assed attempt to match AppKit's rounding rules for drawAtPoint.
        // It's probably incorrect for high DPI.
        // If you change this, be sure to test all the text drawn this way in Safari, including
        // the status bar, bookmarks bar, tab bar, and activity window.
        point.y = webkit_CGCeiling(point.y);
        
        NSGraphicsContext *nsContext = [NSGraphicsContext currentContext];
        CGContextRef cgContext = static_cast<CGContextRef>([nsContext graphicsPort]);
        GraphicsContext graphicsContext(cgContext);    
        
        // Safari doesn't flip the NSGraphicsContext before calling WebKit, yet WebCore requires a flipped graphics context.
        BOOL flipped = [nsContext isFlipped];
        if (!flipped)
            CGContextScaleCTM(cgContext, 1, -1);
            
        Font webCoreFont(FontPlatformData(font, [font pointSize]), ![nsContext isDrawingToScreen], Antialiased);
        TextRun run(buffer.data(), length);
        run.disableRoundingHacks();

        CGFloat red;
        CGFloat green;
        CGFloat blue;
        CGFloat alpha;
        [[textColor colorUsingColorSpaceName:NSDeviceRGBColorSpace] getRed:&red green:&green blue:&blue alpha:&alpha];
        graphicsContext.setFillColor(makeRGBA(red * 255, green * 255, blue * 255, alpha * 255), ColorSpaceDeviceRGB);
        
        webCoreFont.drawText(&graphicsContext, run, FloatPoint(point.x, (flipped ? point.y : (-1 * point.y))));
        
        if (!flipped)
            CGContextScaleCTM(cgContext, 1, -1);
    } else {
        // The given point is on the baseline.
        if ([[NSView focusView] isFlipped])
            point.y -= [font ascender];
        else
            point.y += [font descender];
                
        [string drawAtPoint:point withAttributes:[NSDictionary dictionaryWithObjectsAndKeys:font, NSFontAttributeName, textColor, NSForegroundColorAttributeName, nil]];
    }
}
    
static void drawDoubledAtPoint(NSString *string, NSPoint textPoint, NSColor *topColor, NSColor *bottomColor, NSFont *font)
{
        // turn off font smoothing so translucent text draws correctly (Radar 3118455)
        drawAtPoint(string, textPoint, font, bottomColor);
        
        textPoint.y += 1;
        drawAtPoint(string, textPoint, font, topColor);
}

DragImageRef createDragImageForLink(KURL& url, const String& title, FontRenderingMode)
{
    NSString *label = nsStringNilIfEmpty(title);
    NSURL *cocoaURL = url;
    NSString *urlString = [cocoaURL absoluteString];

    BOOL drawURLString = YES;
    BOOL clipURLString = NO;
    BOOL clipLabelString = NO;

    if (!label) {
        drawURLString = NO;
        label = urlString;
    }

    NSFont *labelFont = [[NSFontManager sharedFontManager] convertFont:[NSFont systemFontOfSize:DragLinkLabelFontsize]
                                                           toHaveTrait:NSBoldFontMask];
    NSFont *urlFont = [NSFont systemFontOfSize:DragLinkUrlFontSize];
    NSSize labelSize;
    labelSize.width = widthWithFont(label, labelFont);
    labelSize.height = [labelFont ascender] - [labelFont descender];
    if (labelSize.width > MaxDragLabelWidth){
        labelSize.width = MaxDragLabelWidth;
        clipLabelString = YES;
    }

    NSSize imageSize;
    imageSize.width = labelSize.width + DragLabelBorderX * 2;
    imageSize.height = labelSize.height + DragLabelBorderY * 2;
    if (drawURLString) {
        NSSize urlStringSize;
        urlStringSize.width = widthWithFont(urlString, urlFont);
        urlStringSize.height = [urlFont ascender] - [urlFont descender];
        imageSize.height += urlStringSize.height;
        if (urlStringSize.width > MaxDragLabelWidth) {
            imageSize.width = std::max(MaxDragLabelWidth + DragLabelBorderY * 2, MinDragLabelWidthBeforeClip);
            clipURLString = YES;
        } else
            imageSize.width = std::max(labelSize.width + DragLabelBorderX * 2, urlStringSize.width + DragLabelBorderX * 2);
    }
    NSImage *dragImage = [[[NSImage alloc] initWithSize: imageSize] autorelease];
    [dragImage lockFocus];

    [[NSColor colorWithDeviceRed: 0.7f green: 0.7f blue: 0.7f alpha: 0.8f] set];

    // Drag a rectangle with rounded corners
    NSBezierPath *path = [NSBezierPath bezierPath];
    [path appendBezierPathWithOvalInRect: NSMakeRect(0, 0, DragLabelRadius * 2, DragLabelRadius * 2)];
    [path appendBezierPathWithOvalInRect: NSMakeRect(0, imageSize.height - DragLabelRadius * 2, DragLabelRadius * 2, DragLabelRadius * 2)];
    [path appendBezierPathWithOvalInRect: NSMakeRect(imageSize.width - DragLabelRadius * 2, imageSize.height - DragLabelRadius * 2, DragLabelRadius * 2, DragLabelRadius * 2)];
    [path appendBezierPathWithOvalInRect: NSMakeRect(imageSize.width - DragLabelRadius * 2, 0, DragLabelRadius * 2, DragLabelRadius * 2)];

    [path appendBezierPathWithRect: NSMakeRect(DragLabelRadius, 0, imageSize.width - DragLabelRadius * 2, imageSize.height)];
    [path appendBezierPathWithRect: NSMakeRect(0, DragLabelRadius, DragLabelRadius + 10, imageSize.height - 2 * DragLabelRadius)];
    [path appendBezierPathWithRect: NSMakeRect(imageSize.width - DragLabelRadius - 20, DragLabelRadius, DragLabelRadius + 20, imageSize.height - 2 * DragLabelRadius)];
    [path fill];

    NSColor *topColor = [NSColor colorWithDeviceWhite:0.0f alpha:0.75f];
    NSColor *bottomColor = [NSColor colorWithDeviceWhite:1.0f alpha:0.5f];
    if (drawURLString) {
        if (clipURLString)
            urlString = StringTruncator::centerTruncate(urlString, imageSize.width - (DragLabelBorderX * 2), fontFromNSFont(urlFont));

       drawDoubledAtPoint(urlString, NSMakePoint(DragLabelBorderX, DragLabelBorderY - [urlFont descender]), topColor, bottomColor, urlFont);
    }

    if (clipLabelString)
        label = StringTruncator::rightTruncate(label, imageSize.width - (DragLabelBorderX * 2), fontFromNSFont(labelFont));
    drawDoubledAtPoint(label, NSMakePoint(DragLabelBorderX, imageSize.height - LabelBorderYOffset - [labelFont pointSize]), topColor, bottomColor, labelFont);

    [dragImage unlockFocus];

    return dragImage;
}
   
} // namespace WebCore

#endif // ENABLE(DRAG_SUPPORT)

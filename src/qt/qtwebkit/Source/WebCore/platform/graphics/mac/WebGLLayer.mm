/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)
#if ENABLE(WEBGL)

#import "WebGLLayer.h"

#import "GraphicsContext3D.h"
#import "GraphicsLayer.h"
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl.h>
#import <wtf/FastMalloc.h>
#import <wtf/RetainPtr.h>

using namespace WebCore;

@implementation WebGLLayer

-(id)initWithGraphicsContext3D:(GraphicsContext3D*)context
{
    m_context = context;
    self = [super init];
    return self;
}

-(CGLPixelFormatObj)copyCGLPixelFormatForDisplayMask:(uint32_t)mask
{
    // FIXME: The mask param tells you which display (on a multi-display system)
    // is to be used. But since we are now getting the pixel format from the 
    // Canvas CGL context, we don't use it. This seems to do the right thing on
    // one multi-display system. But there may be cases where this is not the case.
    // If needed we will have to set the display mask in the Canvas CGLContext and
    // make sure it matches.
    UNUSED_PARAM(mask);
    return CGLRetainPixelFormat(CGLGetPixelFormat(m_context->platformGraphicsContext3D()));
}

-(CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)pixelFormat
{
    CGLContextObj contextObj;
    CGLCreateContext(pixelFormat, m_context->platformGraphicsContext3D(), &contextObj);
    return contextObj;
}

-(void)drawInCGLContext:(CGLContextObj)glContext pixelFormat:(CGLPixelFormatObj)pixelFormat forLayerTime:(CFTimeInterval)timeInterval displayTime:(const CVTimeStamp *)timeStamp
{
    m_context->prepareTexture();

    CGLSetCurrentContext(glContext);

    CGRect frame = [self frame];
        
    // draw the FBO into the layer
    glViewport(0, 0, frame.size.width, frame.size.height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_context->platformTexture());
    
    glBegin(GL_TRIANGLE_FAN);
        glTexCoord2f(0, 0);
        glVertex2f(-1, -1);
        glTexCoord2f(1, 0);
        glVertex2f(1, -1);
        glTexCoord2f(1, 1);
        glVertex2f(1, 1);
        glTexCoord2f(0, 1);
        glVertex2f(-1, 1);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    // Call super to finalize the drawing. By default all it does is call glFlush().
    [super drawInCGLContext:glContext pixelFormat:pixelFormat forLayerTime:timeInterval displayTime:timeStamp];
}

static void freeData(void *, const void *data, size_t /* size */)
{
    fastFree(const_cast<void *>(data));
}

-(CGImageRef)copyImageSnapshotWithColorSpace:(CGColorSpaceRef)colorSpace
{
    CGLSetCurrentContext(m_context->platformGraphicsContext3D());

    RetainPtr<CGColorSpaceRef> imageColorSpace = colorSpace;
    if (!imageColorSpace)
        imageColorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());

    CGRect layerBounds = CGRectIntegral([self bounds]);
    
    size_t width = layerBounds.size.width;
    size_t height = layerBounds.size.height;

    size_t rowBytes = (width * 4 + 15) & ~15;
    size_t dataSize = rowBytes * height;
    void* data = fastMalloc(dataSize);
    if (!data)
        return 0;

    glPixelStorei(GL_PACK_ROW_LENGTH, rowBytes / 4);
    glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);

    CGDataProviderRef provider = CGDataProviderCreateWithData(0, data, dataSize, freeData);
    CGImageRef image = CGImageCreate(width, height, 8, 32, rowBytes, imageColorSpace.get(),
                                                 kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host,
                                                 provider, 0, true,
                                                 kCGRenderingIntentDefault);
    CGDataProviderRelease(provider);
    return image;
}

- (void)display
{
    [super display];
    if (m_layerOwner)
        m_layerOwner->layerDidDisplay(self);
}

@end

@implementation WebGLLayer(WebGLLayerAdditions)

-(void)setLayerOwner:(GraphicsLayer*)aLayer
{
    m_layerOwner = aLayer;
}

-(GraphicsLayer*)layerOwner
{
    return m_layerOwner;
}

@end

#endif // ENABLE(WEBGL)
#endif // USE(ACCELERATED_COMPOSITING)

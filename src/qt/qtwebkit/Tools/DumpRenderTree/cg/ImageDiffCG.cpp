/*
 * Copyright (C) 2005, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2005 Ben La Monica <ben.lamonica@gmail.com>.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(WIN32) || defined(_WIN32)
#define max max
#define min min
#endif

// FIXME: We need to be able to include these defines from a config.h somewhere.
#define JS_EXPORT_PRIVATE
#define WTF_EXPORT_PRIVATE

#include <stdio.h>
#include <wtf/Platform.h>
#include <wtf/RetainPtr.h>

#if PLATFORM(WIN)
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <wtf/MathExtras.h>
#endif

#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGImage.h>
#include <ImageIO/CGImageDestination.h>

#if PLATFORM(MAC)
#include <LaunchServices/UTCoreTypes.h>
#endif

#ifndef CGFLOAT_DEFINED
#ifdef __LP64__
typedef double CGFloat;
#else
typedef float CGFloat;
#endif
#define CGFLOAT_DEFINED 1
#endif

using namespace std;

#if PLATFORM(WIN)
static inline float strtof(const char *nptr, char **endptr)
{
    return strtod(nptr, endptr);
}
static const CFStringRef kUTTypePNG = CFSTR("public.png");
#endif

static RetainPtr<CGImageRef> createImageFromStdin(int bytesRemaining)
{
    unsigned char buffer[2048];
    RetainPtr<CFMutableDataRef> data = adoptCF(CFDataCreateMutable(0, bytesRemaining));

    while (bytesRemaining > 0) {
        size_t bytesToRead = min(bytesRemaining, 2048);
        size_t bytesRead = fread(buffer, 1, bytesToRead, stdin);
        CFDataAppendBytes(data.get(), buffer, static_cast<CFIndex>(bytesRead));
        bytesRemaining -= static_cast<int>(bytesRead);
    }
    RetainPtr<CGDataProviderRef> dataProvider = adoptCF(CGDataProviderCreateWithCFData(data.get()));
    return adoptCF(CGImageCreateWithPNGDataProvider(dataProvider.get(), 0, false, kCGRenderingIntentDefault));
}

static void releaseMallocBuffer(void* info, const void* data, size_t size)
{
    free((void*)data);
}

static RetainPtr<CGImageRef> createDifferenceImage(CGImageRef baseImage, CGImageRef testImage, float& difference)
{
    size_t width = CGImageGetWidth(baseImage);
    size_t height = CGImageGetHeight(baseImage);
    size_t rowBytes = width * 4;

    // Draw base image in bitmap context
    void* baseBuffer = calloc(height, rowBytes);
    RetainPtr<CGContextRef> baseContext = adoptCF(CGBitmapContextCreate(baseBuffer, width, height, 8, rowBytes, CGImageGetColorSpace(baseImage), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));
    CGContextDrawImage(baseContext.get(), CGRectMake(0, 0, width, height), baseImage);

    // Draw test image in bitmap context
    void* buffer = calloc(height, rowBytes);
    RetainPtr<CGContextRef> context = adoptCF(CGBitmapContextCreate(buffer, width, height, 8, rowBytes, CGImageGetColorSpace(testImage), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));
    CGContextDrawImage(context.get(), CGRectMake(0, 0, width, height), testImage);

    // Compare the content of the 2 bitmaps
    void* diffBuffer = malloc(width * height);
    float count = 0.0f;
    float sum = 0.0f;
    float maxDistance = 0.0f;
    unsigned char* basePixel = (unsigned char*)baseBuffer;
    unsigned char* pixel = (unsigned char*)buffer;
    unsigned char* diff = (unsigned char*)diffBuffer;
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            float red = (pixel[0] - basePixel[0]) / max<float>(255 - basePixel[0], basePixel[0]);
            float green = (pixel[1] - basePixel[1]) / max<float>(255 - basePixel[1], basePixel[1]);
            float blue = (pixel[2] - basePixel[2]) / max<float>(255 - basePixel[2], basePixel[2]);
            float alpha = (pixel[3] - basePixel[3]) / max<float>(255 - basePixel[3], basePixel[3]);
            float distance = sqrtf(red * red + green * green + blue * blue + alpha * alpha) / 2.0f;
            
            *diff++ = (unsigned char)(distance * 255.0f);
            
            if (distance >= 1.0f / 255.0f) {
                count += 1.0f;
                sum += distance;
                if (distance > maxDistance)
                    maxDistance = distance;
            }
            
            basePixel += 4;
            pixel += 4;
        }
    }
    
    // Compute the difference as a percentage combining both the number of different pixels and their difference amount i.e. the average distance over the entire image
    if (count > 0.0f)
        difference = 100.0f * sum / (height * width);
    else
        difference = 0.0f;

    RetainPtr<CGImageRef> diffImage;
    // Generate a normalized diff image if there is any difference
    if (difference > 0.0f) {
        if (maxDistance < 1.0f) {
            diff = (unsigned char*)diffBuffer;
            for(size_t p = 0; p < height * width; ++p)
                diff[p] = diff[p] / maxDistance;
        }
        
        static CGColorSpaceRef diffColorspace = CGColorSpaceCreateDeviceGray();
        RetainPtr<CGDataProviderRef> provider = adoptCF(CGDataProviderCreateWithData(0, diffBuffer, width * height, releaseMallocBuffer));
        diffImage = adoptCF(CGImageCreate(width, height, 8, 8, width, diffColorspace, 0, provider.get(), 0, false, kCGRenderingIntentDefault));
    }
    else
        free(diffBuffer);
    
    // Destroy drawing buffers
    if (buffer)
        free(buffer);
    if (baseBuffer)
        free(baseBuffer);
    
    return diffImage;
}

static inline bool imageHasAlpha(CGImageRef image)
{
    CGImageAlphaInfo info = CGImageGetAlphaInfo(image);
    
    return (info >= kCGImageAlphaPremultipliedLast) && (info <= kCGImageAlphaFirst);
}

int main(int argc, const char* argv[])
{
#if PLATFORM(WIN)
    _setmode(0, _O_BINARY);
    _setmode(1, _O_BINARY);
#endif

    float tolerance = 0.0f;

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--tolerance")) {
            if (i >= argc - 1)
                exit(1);
            tolerance = strtof(argv[i + 1], 0);
            ++i;
            continue;
        }
    }

    char buffer[2048];
    RetainPtr<CGImageRef> actualImage;
    RetainPtr<CGImageRef> baselineImage;

    while (fgets(buffer, sizeof(buffer), stdin)) {
        // remove the CR
        char* newLineCharacter = strchr(buffer, '\n');
        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (!strncmp("Content-Length: ", buffer, 16)) {
            strtok(buffer, " ");
            int imageSize = strtol(strtok(0, " "), 0, 10);

            if (imageSize > 0 && !actualImage)
                actualImage = createImageFromStdin(imageSize);
            else if (imageSize > 0 && !baselineImage)
                baselineImage = createImageFromStdin(imageSize);
            else
                fputs("Error: image size must be specified.\n", stderr);
        }

        if (actualImage && baselineImage) {
            RetainPtr<CGImageRef> diffImage;
            float difference = 100.0f;

            if ((CGImageGetWidth(actualImage.get()) == CGImageGetWidth(baselineImage.get())) && (CGImageGetHeight(actualImage.get()) == CGImageGetHeight(baselineImage.get())) && (imageHasAlpha(actualImage.get()) == imageHasAlpha(baselineImage.get()))) {
                diffImage = createDifferenceImage(actualImage.get(), baselineImage.get(), difference); // difference is passed by reference
                if (difference <= tolerance)
                    difference = 0.0f;
                else {
                    difference = roundf(difference * 100.0f) / 100.0f;
                    difference = max(difference, 0.01f); // round to 2 decimal places
                }
            } else {
                if (CGImageGetWidth(actualImage.get()) != CGImageGetWidth(baselineImage.get()) || CGImageGetHeight(actualImage.get()) != CGImageGetHeight(baselineImage.get()))
                    fprintf(stderr, "Error: test and reference images have different sizes. Test image is %lux%lu, reference image is %lux%lu\n",
                        CGImageGetWidth(actualImage.get()), CGImageGetHeight(actualImage.get()),
                        CGImageGetWidth(baselineImage.get()), CGImageGetHeight(baselineImage.get()));
                else if (imageHasAlpha(actualImage.get()) != imageHasAlpha(baselineImage.get()))
                    fprintf(stderr, "Error: test and reference images differ in alpha. Test image %s alpha, reference image %s alpha.\n",
                        imageHasAlpha(actualImage.get()) ? "has" : "does not have",
                        imageHasAlpha(baselineImage.get()) ? "has" : "does not have");
            }
            
            if (difference > 0.0f) {
                if (diffImage) {
                    RetainPtr<CFMutableDataRef> imageData = adoptCF(CFDataCreateMutable(0, 0));
                    RetainPtr<CGImageDestinationRef> imageDest = adoptCF(CGImageDestinationCreateWithData(imageData.get(), kUTTypePNG, 1, 0));
                    CGImageDestinationAddImage(imageDest.get(), diffImage.get(), 0);
                    CGImageDestinationFinalize(imageDest.get());
                    printf("Content-Length: %lu\n", CFDataGetLength(imageData.get()));
                    fwrite(CFDataGetBytePtr(imageData.get()), 1, CFDataGetLength(imageData.get()), stdout);
                }

                fprintf(stdout, "diff: %01.2f%% failed\n", difference);
            } else
                fprintf(stdout, "diff: %01.2f%% passed\n", difference);
            
            actualImage = 0;
            baselineImage = 0;
        }

        fflush(stdout);
    }

    return 0;
}

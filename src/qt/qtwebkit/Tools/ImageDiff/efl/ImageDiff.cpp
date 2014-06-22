/*
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
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

#include "config.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/efl/RefPtrEfl.h>

enum PixelComponent {
    Red,
    Green,
    Blue,
    Alpha
};

static OwnPtr<Ecore_Evas> gEcoreEvas;
static double gTolerance = 0;

static void abortWithErrorMessage(const char* errorMessage);

static unsigned char* pixelFromImageData(unsigned char* imageData, int rowStride, int x, int y)
{
    return imageData + (y * rowStride) + (x << 2);
}

static Evas_Object* differenceImageFromDifferenceBuffer(Evas* evas, unsigned char* buffer, int width, int height)
{
    Evas_Object* image = evas_object_image_filled_add(evas);
    if (!image)
        abortWithErrorMessage("could not create difference image");

    evas_object_image_size_set(image, width, height);
    evas_object_image_colorspace_set(image, EVAS_COLORSPACE_ARGB8888);

    unsigned char* diffPixels = static_cast<unsigned char*>(evas_object_image_data_get(image, EINA_TRUE));
    const int rowStride = evas_object_image_stride_get(image);
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            unsigned char* diffPixel = pixelFromImageData(diffPixels, rowStride, x, y);
            diffPixel[Red] = diffPixel[Green] = diffPixel[Blue] = *buffer++;
            diffPixel[Alpha] = 0xff;
        }
    }

    evas_object_image_data_set(image, diffPixels);

    return image;
}

static float computeDistanceBetweenPixelComponents(unsigned char actualComponent, unsigned char baseComponent)
{
    return (actualComponent - baseComponent) / std::max<float>(255 - baseComponent, baseComponent);
}

static float computeDistanceBetweenPixelComponents(unsigned char* actualPixel, unsigned char* basePixel, PixelComponent component)
{
    return computeDistanceBetweenPixelComponents(actualPixel[component], basePixel[component]);
}

static float calculatePixelDifference(unsigned char* basePixel, unsigned char* actualPixel)
{
    const float red = computeDistanceBetweenPixelComponents(actualPixel, basePixel, Red);
    const float green = computeDistanceBetweenPixelComponents(actualPixel, basePixel, Green);
    const float blue = computeDistanceBetweenPixelComponents(actualPixel, basePixel, Blue);
    const float alpha = computeDistanceBetweenPixelComponents(actualPixel, basePixel, Alpha);
    return sqrtf(red * red + green * green + blue * blue + alpha * alpha) / 2.0f;
}

static float calculateDifference(Evas_Object* baselineImage, Evas_Object* actualImage, RefPtr<Evas_Object>& differenceImage)
{
    int width, height, baselineWidth, baselineHeight;
    evas_object_image_size_get(actualImage, &width, &height);
    evas_object_image_size_get(baselineImage, &baselineWidth, &baselineHeight);

    if (width != baselineWidth || height != baselineHeight) {
        printf("Error, test and reference image have different sizes.\n");
        return 100; // Completely different.
    }

    OwnArrayPtr<unsigned char> diffBuffer = adoptArrayPtr(new unsigned char[width * height]);
    if (!diffBuffer)
        abortWithErrorMessage("could not create difference buffer");

    const int actualRowStride = evas_object_image_stride_get(actualImage);
    const int baseRowStride = evas_object_image_stride_get(baselineImage);
    unsigned numberOfDifferentPixels = 0;
    float totalDistance = 0;
    float maxDistance = 0;
    unsigned char* actualPixels = static_cast<unsigned char*>(evas_object_image_data_get(actualImage, EINA_FALSE));
    unsigned char* basePixels = static_cast<unsigned char*>(evas_object_image_data_get(baselineImage, EINA_FALSE));
    unsigned char* currentDiffPixel = diffBuffer.get();

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            unsigned char* actualPixel = pixelFromImageData(actualPixels, actualRowStride, x, y);
            unsigned char* basePixel = pixelFromImageData(basePixels, baseRowStride, x, y);

            const float distance = calculatePixelDifference(basePixel, actualPixel);
            *currentDiffPixel++ = static_cast<unsigned char>(distance * 255.0f);

            if (distance >= 1.0f / 255.0f) {
                ++numberOfDifferentPixels;
                totalDistance += distance;
                maxDistance = std::max<float>(maxDistance, distance);
            }
        }
    }

    // When using evas_object_image_data_get(), a complementary evas_object_data_set() must be
    // issued to balance the reference count, even if the image hasn't been changed.
    evas_object_image_data_set(baselineImage, basePixels);
    evas_object_image_data_set(actualImage, actualPixels);

    // Compute the difference as a percentage combining both the number of
    // different pixels and their difference amount i.e. the average distance
    // over the entire image
    float difference = 0;
    if (numberOfDifferentPixels)
        difference = 100.0f * totalDistance / (height * width);
    if (difference <= gTolerance)
        difference = 0;
    else {
        difference = roundf(difference * 100.0f) / 100.0f;
        difference = std::max(difference, 0.01f); // round to 2 decimal places

        differenceImage = adoptRef(differenceImageFromDifferenceBuffer(evas_object_evas_get(baselineImage), diffBuffer.get(), width, height));
    }

    return difference;
}

static int getTemporaryFile(char *fileName, size_t fileNameLength)
{
    char* tempDirectory = getenv("TMPDIR");
    if (!tempDirectory)
        tempDirectory = getenv("TEMP");

    if (tempDirectory)
        snprintf(fileName, fileNameLength, "%s/ImageDiffXXXXXX.png", tempDirectory);
    else {
#if __linux__
        strcpy(fileName, "/dev/shm/ImageDiffXXXXXX.png");
        const int fileDescriptor = mkstemps(fileName, sizeof(".png") - 1);
        if (fileDescriptor >= 0)
            return fileDescriptor;
#endif // __linux__

        strcpy(fileName, "ImageDiffXXXXXX.png");
    }

    return mkstemps(fileName, sizeof(".png") - 1);
}

static void printImage(Evas_Object* image)
{
    char fileName[PATH_MAX];

    const int tempImageFd = getTemporaryFile(fileName, PATH_MAX);
    if (tempImageFd == -1)
        abortWithErrorMessage("could not create temporary file");

    evas_render(evas_object_evas_get(image));

    if (evas_object_image_save(image, fileName, 0, 0)) {
        struct stat fileInfo;
        if (!stat(fileName, &fileInfo)) {
            printf("Content-Length: %ld\n", fileInfo.st_size);
            fflush(stdout);

            unsigned char buffer[2048];
            ssize_t bytesRead;
            while ((bytesRead = read(tempImageFd, buffer, sizeof(buffer))) > 0) {
                ssize_t bytesWritten = 0;
                ssize_t count;
                do {
                    if ((count = write(1, buffer + bytesWritten, bytesRead - bytesWritten)) <= 0)
                        break;
                    bytesWritten += count;
                } while (bytesWritten < bytesRead);
            }
        }
    }
    close(tempImageFd);
    unlink(fileName);
}

static void printImageDifferences(Evas_Object* baselineImage, Evas_Object* actualImage)
{
    RefPtr<Evas_Object> differenceImage;
    const float difference = calculateDifference(baselineImage, actualImage, differenceImage);

    if (difference > 0.0f) {
        if (differenceImage)
            printImage(differenceImage.get());

        printf("diff: %01.2f%% failed\n", difference);
    } else
        printf("diff: %01.2f%% passed\n", difference);
}

static void resizeEcoreEvasIfNeeded(Evas_Object* image)
{
    int newWidth, newHeight;
    evas_object_image_size_get(image, &newWidth, &newHeight);

    int currentWidth, currentHeight;
    ecore_evas_screen_geometry_get(gEcoreEvas.get(), 0, 0, &currentWidth, &currentHeight);

    if (newWidth > currentWidth)
        currentWidth = newWidth;
    if (newHeight > currentHeight)
        currentHeight = newHeight;

    ecore_evas_resize(gEcoreEvas.get(), currentWidth, currentHeight);
}

static PassRefPtr<Evas_Object> readImageFromStdin(Evas* evas, long imageSize)
{
    OwnArrayPtr<unsigned char> imageBuffer = adoptArrayPtr(new unsigned char[imageSize]);
    if (!imageBuffer)
        abortWithErrorMessage("cannot allocate image");

    const size_t bytesRead = fread(imageBuffer.get(), 1, imageSize, stdin);
    if (!bytesRead)
        return PassRefPtr<Evas_Object>();

    Evas_Object* image = evas_object_image_filled_add(evas);
    evas_object_image_colorspace_set(image, EVAS_COLORSPACE_ARGB8888);
    evas_object_image_memfile_set(image, imageBuffer.get(), bytesRead, 0, 0);

    resizeEcoreEvasIfNeeded(image);

    return adoptRef(image);
}

static bool parseCommandLineOptions(int argc, char** argv)
{
    static const option options[] = {
        { "tolerance", required_argument, 0, 't' },
        { 0, 0, 0, 0 }
    };
    int option;

    while ((option = getopt_long(argc, (char* const*)argv, "t:", options, 0)) != -1) {
        switch (option) {
        case 't':
            gTolerance = atof(optarg);
            break;
        case '?':
        case ':':
            return false;
        }
    }

    return true;
}

static void shutdownEfl()
{
    ecore_evas_shutdown();
    ecore_shutdown();
    evas_shutdown();
}

static void abortWithErrorMessage(const char* errorMessage)
{
    shutdownEfl();

    printf("Error, %s.\n", errorMessage);
    exit(EXIT_FAILURE);
}

static Evas* initEfl()
{
    evas_init();
    ecore_init();
    ecore_evas_init();

    gEcoreEvas = adoptPtr(ecore_evas_buffer_new(1, 1));
    Evas* evas = ecore_evas_get(gEcoreEvas.get());
    if (!evas)
        abortWithErrorMessage("could not create Ecore_Evas buffer");

    return evas;
}

int main(int argc, char* argv[])
{
    if (!parseCommandLineOptions(argc, argv))
        return EXIT_FAILURE;

    Evas* evas = initEfl();

    RefPtr<Evas_Object> actualImage;
    RefPtr<Evas_Object> baselineImage;

    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        char* contentLengthStart = strstr(buffer, "Content-Length: ");
        if (!contentLengthStart)
            continue;
        long imageSize;
        if (sscanf(contentLengthStart, "Content-Length: %ld", &imageSize) == 1) {
            if (imageSize <= 0)
                abortWithErrorMessage("image size must be specified");

            if (!actualImage)
                actualImage = readImageFromStdin(evas, imageSize);
            else if (!baselineImage) {
                baselineImage = readImageFromStdin(evas, imageSize);

                printImageDifferences(baselineImage.get(), actualImage.get());

                actualImage.clear();
                baselineImage.clear();
            }
        }

        fflush(stdout);
    }

    gEcoreEvas.clear(); // Make sure ecore_evas_free is called before the EFL are shut down

    shutdownEfl();
    return EXIT_SUCCESS;
}

/*
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2010 Igalia S.L.
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

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <gdk/gdk.h>

using namespace std;

static double tolerance = 0;
static GOptionEntry commandLineOptionEntries[] = 
{
    { "tolerance", 0, 0, G_OPTION_ARG_DOUBLE, &tolerance, "Percentage difference between images before considering them different", "T" },
    { 0, 0, 0, G_OPTION_ARG_NONE, 0, 0, 0 },
};

GdkPixbuf* readPixbufFromStdin(long imageSize)
{
    unsigned char imageBuffer[2048];
    GdkPixbufLoader* loader = gdk_pixbuf_loader_new_with_type("png", 0);
    GError* error = 0;

    while (imageSize > 0) {
        size_t bytesToRead = min<int>(imageSize, 2048);
        size_t bytesRead = fread(imageBuffer, 1, bytesToRead, stdin);

        if (!gdk_pixbuf_loader_write(loader, reinterpret_cast<const guchar*>(imageBuffer), bytesRead, &error)) {
            g_error_free(error);
            gdk_pixbuf_loader_close(loader, 0);
            g_object_unref(loader);
            return 0;
        }

        imageSize -= static_cast<int>(bytesRead);
    }

    gdk_pixbuf_loader_close(loader, 0);
    GdkPixbuf* decodedImage = gdk_pixbuf_loader_get_pixbuf(loader);
    g_object_ref(decodedImage);
    return decodedImage;
}

GdkPixbuf* differenceImageFromDifferenceBuffer(unsigned char* buffer, int width, int height)
{
    GdkPixbuf* image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
    if (!image)
        return image;

    int rowStride = gdk_pixbuf_get_rowstride(image);
    unsigned char* diffPixels = gdk_pixbuf_get_pixels(image);
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            unsigned char* diffPixel = diffPixels + (y * rowStride) + (x * 3);
            diffPixel[0] = diffPixel[1] = diffPixel[2] = *buffer++;
        }
    }

    return image;
}

float calculateDifference(GdkPixbuf* baselineImage, GdkPixbuf* actualImage, GdkPixbuf** differenceImage)
{
    int width = gdk_pixbuf_get_width(actualImage);
    int height = gdk_pixbuf_get_height(actualImage);
    int numberOfChannels = gdk_pixbuf_get_n_channels(actualImage);
    if ((width != gdk_pixbuf_get_width(baselineImage))
        || (height != gdk_pixbuf_get_height(baselineImage))
        || (numberOfChannels != gdk_pixbuf_get_n_channels(baselineImage))
        || (gdk_pixbuf_get_has_alpha(actualImage) != gdk_pixbuf_get_has_alpha(baselineImage))) {
        fprintf(stderr, "Error, test and reference image have different properties.\n");
        return 100; // Completely different.
    }

    unsigned char* diffBuffer = static_cast<unsigned char*>(malloc(width * height));
    float count = 0;
    float sum = 0;
    float maxDistance = 0;
    int actualRowStride = gdk_pixbuf_get_rowstride(actualImage);
    int baseRowStride = gdk_pixbuf_get_rowstride(baselineImage);
    unsigned char* actualPixels = gdk_pixbuf_get_pixels(actualImage);
    unsigned char* basePixels = gdk_pixbuf_get_pixels(baselineImage);
    unsigned char* currentDiffPixel = diffBuffer;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            unsigned char* actualPixel = actualPixels + (y * actualRowStride) + (x * numberOfChannels);
            unsigned char* basePixel = basePixels + (y * baseRowStride) + (x * numberOfChannels);

            float red = (actualPixel[0] - basePixel[0]) / max<float>(255 - basePixel[0], basePixel[0]);
            float green = (actualPixel[1] - basePixel[1]) / max<float>(255 - basePixel[1], basePixel[1]);
            float blue = (actualPixel[2] - basePixel[2]) / max<float>(255 - basePixel[2], basePixel[2]);
            float alpha = (actualPixel[3] - basePixel[3]) / max<float>(255 - basePixel[3], basePixel[3]);
            float distance = sqrtf(red * red + green * green + blue * blue + alpha * alpha) / 2.0f;

            *currentDiffPixel++ = (unsigned char)(distance * 255.0f);

            if (distance >= 1.0f / 255.0f) {
                count += 1.0f;
                sum += distance;
                maxDistance = max<float>(maxDistance, distance);
            }
        }
    }

    // Compute the difference as a percentage combining both the number of
    // different pixels and their difference amount i.e. the average distance
    // over the entire image
    float difference = 0;
    if (count > 0.0f)
        difference = 100.0f * sum / (height * width);
    if (difference <= tolerance)
        difference = 0;
    else {
        difference = roundf(difference * 100.0f) / 100.0f;
        difference = max(difference, 0.01f); // round to 2 decimal places
        *differenceImage = differenceImageFromDifferenceBuffer(diffBuffer, width, height);
    }

    free(diffBuffer);
    return difference;
}

void printImage(GdkPixbuf* image)
{
    char* buffer;
    gsize bufferSize;
    GError* error = 0;
    if (!gdk_pixbuf_save_to_buffer(image, &buffer, &bufferSize, "png", &error, NULL)) {
        g_error_free(error);
        return; // Don't bail out, as we can still use the percentage output.
    }

    printf("Content-Length: %" G_GSIZE_FORMAT "\n", bufferSize);
    fwrite(buffer, 1, bufferSize, stdout);
}

void printImageDifferences(GdkPixbuf* baselineImage, GdkPixbuf* actualImage)
{
    GdkPixbuf* differenceImage = 0;
    float difference = calculateDifference(baselineImage, actualImage, &differenceImage);
    if (difference > 0.0f) {
        if (differenceImage) {
            printImage(differenceImage);
            g_object_unref(differenceImage);
        }
        printf("diff: %01.2f%% failed\n", difference);
    } else
        printf("diff: %01.2f%% passed\n", difference);
}

int main(int argc, char* argv[])
{
    GError* error = 0;
    GOptionContext* context = g_option_context_new("- compare two image files, printing their percentage difference and the difference image to stdout");
    g_option_context_add_main_entries(context, commandLineOptionEntries, 0);
    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        printf("Option parsing failed: %s\n", error->message);
        g_error_free(error);
        return 1;
    }

    GdkPixbuf* actualImage = 0;
    GdkPixbuf* baselineImage = 0;
    char buffer[2048];
    while (fgets(buffer, sizeof(buffer), stdin)) {
        // Convert the first newline into a NUL character so that strtok doesn't produce it.
        char* newLineCharacter = strchr(buffer, '\n');
        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (!strncmp("Content-Length: ", buffer, 16)) {
            gchar** tokens = g_strsplit(buffer, " ", 0);
            if (!tokens[1]) {
                g_strfreev(tokens);
                printf("Error, image size must be specified..\n");
                return 1;
            }

            long imageSize = strtol(tokens[1], 0, 10);
            g_strfreev(tokens);
            if (imageSize > 0 && !actualImage) {
                if (!(actualImage = readPixbufFromStdin(imageSize))) {
                    printf("Error, could not read actual image.\n");
                    return 1;
                }
            } else if (imageSize > 0 && !baselineImage) {
                if (!(baselineImage = readPixbufFromStdin(imageSize))) {
                    printf("Error, could not read baseline image.\n");
                    return 1;
                }
            } else {
                printf("Error, image size must be specified..\n");
                return 1;
            }
        }

        if (actualImage && baselineImage) {
            printImageDifferences(baselineImage, actualImage);
            g_object_unref(actualImage);
            g_object_unref(baselineImage);
            actualImage = 0;
            baselineImage = 0;
        }

        fflush(stdout);
    }

    return 0;
}

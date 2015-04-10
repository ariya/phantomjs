/*
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Portions of this file are Copyright 2005 Google Inc.
 */

#include "config.h"
#include "PNGImageEncoder.h"


extern "C" {
#include "png.h"
}

#include <wtf/OwnArrayPtr.h>

// This code is almost a mirror of the code in WebCore/platform/image-encoders/skia/PNGImageEncoder.cpp
// since we can't include this private WebCore file in a WebKit-client application.

// Keep the premultipied for as it is the most faithful information
static void BGRAtoRGBA(const unsigned char* input, int numberOfPixels, unsigned char* output)
{
    for (int x = 0; x < numberOfPixels; x++) {
        output[0] = input[2];
        output[1] = input[1];
        output[2] = input[0];
        output[3] = input[3];
        input += 4;
        output += 4;
    }
}

// Passed around as the io_ptr in the png structs so our callbacks know where
// to write data.
struct PNGEncoderState {
    PNGEncoderState(Vector<unsigned char>* o) : m_out(o) { }
    Vector<unsigned char>* m_out;
};

// Called by libpng to flush its internal buffer to ours.
void encoderWriteCallback(png_structp png, png_bytep data, png_size_t size)
{
    PNGEncoderState* state = static_cast<PNGEncoderState*>(png_get_io_ptr(png));
    ASSERT(state->m_out);

    size_t oldSize = state->m_out->size();
    state->m_out->resize(oldSize + size);
    memcpy(&(*state->m_out)[oldSize], data, size);
}

// Automatically destroys the given write structs on destruction to make
// cleanup and error handling code cleaner.
class PNGWriteStructDestroyer {
public:
    PNGWriteStructDestroyer(png_struct** ps, png_info** pi)
        : m_pngStruct(ps)
        , m_pngInfo(pi)
    {
    }

    ~PNGWriteStructDestroyer()
    {
        png_destroy_write_struct(m_pngStruct, m_pngInfo);
    }

private:
    png_struct** m_pngStruct;
    png_info** m_pngInfo;
};

typedef void (*PixelConversionFunc)(const unsigned char*, int, unsigned char*);
static bool encodeImpl(const unsigned char* input, int imageWidth, int imageHeight, int bytesPerRow, Vector<unsigned char>* output, PixelConversionFunc conversionFunc)
{
    int inputColorComponents = 4;
    int outputColorComponents = 4;
    int pngOutputColorType = PNG_COLOR_TYPE_RGB_ALPHA;

    if (imageWidth < 0)
        imageWidth = 0;

    if (imageHeight < 0)
        imageHeight = 0;

    // Row stride should be at least as long as the length of the data.
    if (inputColorComponents * imageWidth > bytesPerRow) {
        ASSERT(false);
        return false;
    }

    png_struct* pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!pngPtr)
        return false;

    png_info* infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr) {
        png_destroy_write_struct(&pngPtr, 0);
        return false;
    }
    PNGWriteStructDestroyer destroyer(&pngPtr, &infoPtr);

    if (setjmp(png_jmpbuf(pngPtr))) {
        // The destroyer will ensure that the structures are cleaned up in this
        // case, even though we may get here as a jump from random parts of the
        // PNG library called below.
        return false;
    }

    // Set our callback for libpng to give us the data.
    PNGEncoderState state(output);
    png_set_write_fn(pngPtr, &state, encoderWriteCallback, 0);

    png_set_IHDR(pngPtr, infoPtr, imageWidth, imageHeight, 8, pngOutputColorType,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);
    png_write_info(pngPtr, infoPtr);

    OwnArrayPtr<unsigned char> rowPixels = adoptArrayPtr(new unsigned char[imageWidth * outputColorComponents]);
    for (int y = 0; y < imageHeight; y ++) {
        conversionFunc(&input[y * bytesPerRow], imageWidth, rowPixels.get());
        png_write_row(pngPtr, rowPixels.get());
    }

    png_write_end(pngPtr, infoPtr);
    return true;
}

bool encodeBitmapToPNG(unsigned char* data, int width, int height, Vector<unsigned char>* output)
{
    bool result = encodeImpl(data, width, height, width * 4, output, BGRAtoRGBA);
    return result;
}

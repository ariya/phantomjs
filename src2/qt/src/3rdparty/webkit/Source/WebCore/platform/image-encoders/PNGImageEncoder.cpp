/*
 * Copyright (c) 2006-2009, Google Inc. All rights reserved.
 * Copyright (c) 2009 Torch Mobile, Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PNGImageEncoder.h"

#include "IntSize.h"
#include "png.h"
#include <wtf/Vector.h>

namespace WebCore {

// Encoder --------------------------------------------------------------------
//
// This section of the code is based on nsPNGEncoder.cpp in Mozilla
// (Copyright 2005 Google Inc.)

// Passed around as the io_ptr in the png structs so our callbacks know where
// to write data.
struct PNGEncoderState {
    PNGEncoderState(Vector<char>* o) : m_dump(o) {}
    Vector<char>* m_dump;
};

// Called by libpng to flush its internal buffer to ours.
void encoderWriteCallback(png_structp png, png_bytep data, png_size_t size)
{
    PNGEncoderState* state = static_cast<PNGEncoderState*>(png_get_io_ptr(png));
    ASSERT(state->m_dump);

    size_t oldSize = state->m_dump->size();
    state->m_dump->resize(oldSize + size);
    char* destination = state->m_dump->data() + oldSize;
    memcpy(destination, data, size);
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

bool compressRGBABigEndianToPNG(unsigned char* rgbaBigEndianData, const IntSize& size, Vector<char>& pngData)
{
    png_struct* pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, png_voidp_NULL, png_error_ptr_NULL, png_error_ptr_NULL);
    if (!pngPtr)
        return false;

    png_info* infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr) {
        png_destroy_write_struct(&pngPtr, 0);
        return false;
    }
    PNGWriteStructDestroyer destroyer(&pngPtr, &infoPtr);

    // The destroyer will ensure that the structures are cleaned up in this
    // case, even though we may get here as a jump from random parts of the
    // PNG library called below.
    if (setjmp(png_jmpbuf(pngPtr)))
        return false;

    // Set our callback for libpng to give us the data.
    PNGEncoderState state(&pngData);
    png_set_write_fn(pngPtr, &state, encoderWriteCallback, 0);

    int pngOutputColorType = PNG_COLOR_TYPE_RGB_ALPHA;

    png_set_IHDR(pngPtr, infoPtr, size.width(), size.height(), 8, pngOutputColorType,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(pngPtr, infoPtr);

    unsigned bytesPerRow = size.width() * 4;
    for (unsigned y = 0; y < size.height(); ++y) {
        png_write_row(pngPtr, rgbaBigEndianData);
        rgbaBigEndianData += bytesPerRow;
    }

    png_write_end(pngPtr, infoPtr);
    return true;
}

} // namespace WebCore

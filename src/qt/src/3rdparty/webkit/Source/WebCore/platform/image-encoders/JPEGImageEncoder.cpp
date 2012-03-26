/*
 * Copyright (C) 2009 Torch Mobile, Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */


#include "config.h"
#include "JPEGImageEncoder.h"

#include "IntSize.h"
// FIXME: jpeglib.h requires stdio.h to be included first for FILE
#include <stdio.h>
#include "jpeglib.h"
#include <setjmp.h>

namespace WebCore {

class JPEGDestinationManager : public jpeg_destination_mgr {
public:
    explicit JPEGDestinationManager(Vector<char>& toDump)
        : m_dump(toDump)
    {
        // Zero base class memory.
        jpeg_destination_mgr* base = this;
        memset(base, 0, sizeof(jpeg_destination_mgr));
    }
    Vector<char> m_buffer;
    Vector<char>& m_dump;
};

class JPEGCompressErrorMgr : public jpeg_error_mgr {
public:
    JPEGCompressErrorMgr()
    {
        // Zero memory
        memset(this, 0, sizeof(JPEGCompressErrorMgr));
    }

    jmp_buf m_setjmpBuffer;
};

static void jpegInitializeDestination(j_compress_ptr compressData)
{
    JPEGDestinationManager* dest = static_cast<JPEGDestinationManager*>(compressData->dest);
    dest->m_buffer.resize(4096);
    dest->next_output_byte = reinterpret_cast<JOCTET*>(dest->m_buffer.data());
    dest->free_in_buffer = dest->m_buffer.size();
}

static boolean jpegEmptyOutputBuffer(j_compress_ptr compressData)
{
    JPEGDestinationManager* dest = static_cast<JPEGDestinationManager*>(compressData->dest);
    dest->m_dump.append(dest->m_buffer.data(), dest->m_buffer.size());
    dest->next_output_byte  = reinterpret_cast<JOCTET*>(dest->m_buffer.data());
    dest->free_in_buffer    = dest->m_buffer.size();
    return TRUE;
}

static void jpegTerminateDestination(j_compress_ptr compressData)
{
    JPEGDestinationManager* dest = static_cast<JPEGDestinationManager*>(compressData->dest);
    dest->m_dump.append(dest->m_buffer.data(), dest->m_buffer.size() - dest->free_in_buffer);
}

static void jpegErrorExit(j_common_ptr compressData)
{
    JPEGCompressErrorMgr* err = static_cast<JPEGCompressErrorMgr*>(compressData->err);
    longjmp(err->m_setjmpBuffer, -1);
}

bool compressRGBABigEndianToJPEG(unsigned char* rgbaBigEndianData, const IntSize& size, Vector<char>& jpegData)
{
    struct jpeg_compress_struct compressData = { 0 };
    JPEGCompressErrorMgr err;
    compressData.err = jpeg_std_error(&err);
    err.error_exit = jpegErrorExit;

    jpeg_create_compress(&compressData);

    JPEGDestinationManager dest(jpegData);
    compressData.dest = &dest;
    dest.init_destination = jpegInitializeDestination;
    dest.empty_output_buffer = jpegEmptyOutputBuffer;
    dest.term_destination = jpegTerminateDestination;

    compressData.image_width = size.width();
    compressData.image_height = size.height();
    compressData.input_components = 3;
    compressData.in_color_space = JCS_RGB;
    jpeg_set_defaults(&compressData);
    jpeg_set_quality(&compressData, 65, FALSE);

    // rowBuffer must be defined here so that its destructor is always called even when "setjmp" catches an error.
    Vector<JSAMPLE, 600 * 3> rowBuffer;

    if (setjmp(err.m_setjmpBuffer))
        return false;

    jpeg_start_compress(&compressData, TRUE);
    rowBuffer.resize(compressData.image_width * 3);

    const unsigned char* pixel = rgbaBigEndianData;
    const unsigned char* pixelEnd = pixel + compressData.image_width * compressData.image_height * 4;
    while (pixel < pixelEnd) {
        JSAMPLE* output = rowBuffer.data();
        for (const unsigned char* rowEnd = pixel + compressData.image_width * 4; pixel < rowEnd;) {
            *output++ = static_cast<JSAMPLE>(*pixel++ & 0xFF); // red
            *output++ = static_cast<JSAMPLE>(*pixel++ & 0xFF); // green
            *output++ = static_cast<JSAMPLE>(*pixel++ & 0xFF); // blue
            ++pixel; // skip alpha
        }
        output = rowBuffer.data();
        jpeg_write_scanlines(&compressData, &output, 1);
    }

    jpeg_finish_compress(&compressData);
    return true;
}

} // namespace WebCore

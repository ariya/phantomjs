/*
 * Copyright (C) 2011 Brent Fulgham <bfulgham@webkit.org>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DIBPixelData.h"

namespace WebCore {

static const WORD bitmapType = 0x4d42; // BMP format
static const WORD bitmapPixelsPerMeter = 2834; // 72 dpi

DIBPixelData::DIBPixelData(HBITMAP bitmap)
{
    initialize(bitmap);
}

void DIBPixelData::initialize(HBITMAP bitmap)
{
    BITMAP bmpInfo;
    GetObject(bitmap, sizeof(bmpInfo), &bmpInfo);

    m_bitmapBuffer = reinterpret_cast<UInt8*>(bmpInfo.bmBits);
    m_bitmapBufferLength = bmpInfo.bmWidthBytes * bmpInfo.bmHeight;
    m_size = IntSize(bmpInfo.bmWidth, bmpInfo.bmHeight);
    m_bytesPerRow = bmpInfo.bmWidthBytes;
    m_bitsPerPixel = bmpInfo.bmBitsPixel;
}

#ifndef NDEBUG
void DIBPixelData::writeToFile(LPCWSTR filePath)
{
    HANDLE hFile = ::CreateFile(filePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (INVALID_HANDLE_VALUE == hFile)
        return;

    BITMAPFILEHEADER header;
    header.bfType = bitmapType;
    header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    header.bfReserved1 = 0;
    header.bfReserved2 = 0;
    header.bfSize = sizeof(BITMAPFILEHEADER);

    BITMAPINFOHEADER info;
    info.biSize = sizeof(BITMAPINFOHEADER);
    info.biWidth = m_size.width();
    info.biHeight = m_size.height();
    info.biPlanes = 1;
    info.biBitCount = m_bitsPerPixel;
    info.biCompression = BI_RGB;
    info.biSizeImage = bufferLength();
    info.biXPelsPerMeter = bitmapPixelsPerMeter;
    info.biYPelsPerMeter = bitmapPixelsPerMeter;
    info.biClrUsed = 0;
    info.biClrImportant = 0;

    DWORD bytesWritten = 0;
    ::WriteFile(hFile, &header, sizeof(header), &bytesWritten, 0);
    ::WriteFile(hFile, &info, sizeof(info), &bytesWritten, 0);
    ::WriteFile(hFile, buffer(), bufferLength(), &bytesWritten, 0);

    ::CloseHandle(hFile);
}
#endif

} // namespace WebCore
